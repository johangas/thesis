/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/**
 * \file
 *         debug-node
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/watchdog.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"

#include "net/netstack.h"
#include "net/mac/framer-802154.h"
#include "net/mac/handler-802154.h"
#include "dev/slip.h"
#include "cmd.h"
#include "border-router.h"
#include "border-router-cmds.h"
#include "br-config.h"
#include "brm-stats.h"
#include "br-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "udp-cmd.h"
#include "uip-icmp6.h"

#ifdef HAVE_OAM
#include <time.h>
#include "oam.h"
#include "api.h"
#include "encap.h"
void instance_nstats_init(instanceControl IC[]);
#endif /* HAVE_OAM */

#ifdef HAVE_NETSCAN
#include "netscan.h"
#endif /* HAVE_NETSCAN */

#if WITH_IPSO
#include "ipso-objects.h"
/* static uip_ipaddr_t server_addr; */
#endif

#define YLOG_LEVEL YLOG_LEVEL_DEBUG
#define YLOG_NAME  "ndn"
#include "lib/ylog.h"

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

extern uip_ds6_nbr_t uip_ds6_nbr_cache[];
extern uip_ds6_route_t uip_ds6_routing_table[];

extern unsigned long slip_sent;
extern unsigned long slip_sent_to_fd;
long slip_buffered(void);

static uint8_t mac_set;

extern int contiki_argc;
extern char **contiki_argv;
extern const char *slip_config_ipaddr;
extern const char *slip_config_run_command;

uint32_t brm_stats[BRM_STATS_MAX];
uint32_t brm_stats_debug[BRM_STATS_DEBUG_MAX];

uint8_t radio_info = 0;

/* this is the minimum rank that is acceptable - useful for creating multihop */
int debug_join_limit = 0;

struct uip_icmp6_echo_reply_notification reply_notification;

extern clock_time_t ping_time;

#ifdef MY_APP_PROCESS
PROCESS_NAME(MY_APP_PROCESS);
#endif /* MY_APP_PROCESS */

#ifdef MY_APP_INIT_FUNCTION
void MY_APP_INIT_FUNCTION(void);
#endif /* MY_APP_INIT_FUNCTION */

/*---------------------------------------------------------------------------*/
static void
echo_reply_callback(uip_ipaddr_t *source, uint8_t ttl,
                    uint8_t *data, uint16_t datalen)
{
  clock_time_t latency;
  latency = (1000 * (clock_time() - ping_time)) / CLOCK_SECOND;

  printf("Got ping reply from ");
  uip_debug_ipaddr_print(source);
  printf(" - ttl:%d latency: %d (ms)\n", ttl, (int) latency);
}


void
debug_node_dio_input(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  int rank = dio->rank / (int) dio->dag_min_hoprankinc;
  PRINTF("DIO incoming with rank: %d => %d\n", dio->rank, rank);
  if(rank < debug_join_limit) {
    /* fool RPL to not consider this good... */
    PRINTF("Got too low ranked DIO -> faking high rank\n");
    dio->rank = 65535;
  }
}

int debug_cmd_handler(const uint8_t *data, int len);

CMD_HANDLERS(border_router_cmd_handler, debug_cmd_handler);

PROCESS(native_node_process, "Border router init process");
PROCESS(native_node_started_process, "Border router loop process");
PROCESS_NAME(serial_input_process);

AUTOSTART_PROCESSES(&native_node_started_process);

/*---------------------------------------------------------------------------*/
static int mac_req_init = 0;
int
contiki_wait_for_mac(void)
{
  /* need to wait more if radio_mac_addr_ready is not 1 */
  if(!mac_req_init) {
    /* start the process... */
    process_start(&native_node_process, NULL);
    mac_req_init = 1;
  }

  return mac_set != 1;
}

/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTA("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINTA(" %p: =>", &uip_ds6_if.addr_list[i]);
      uip_debug_ipaddr_print(&(uip_ds6_if.addr_list[i]).ipaddr);
      PRINTA("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
void
border_router_set_radio_mode(uint8_t mode)
{
  uint8_t buf[3];
  buf[0] = '!';
  buf[1] = 'm';
  buf[2] = mode;
  write_to_slip(buf, 3);
}
/*---------------------------------------------------------------------------*/
static void
request_mac(void)
{
  write_to_slip((uint8_t *)"?M", 2);
}

/*---------------------------------------------------------------------------*/
static int
node_join_dag(rpl_dio_t *dio)
{
  int rank = dio->rank / (int) dio->dag_min_hoprankinc;
  PRINTF("JOIN RPL at: %d (%d) = %d?\n", dio->rank , dio->dag_min_hoprankinc, rank);
  if(rank >= debug_join_limit) {
    PRINTF("Joining DAG with parent rank = %d\n", rank);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
border_router_set_mac(const uint8_t *data)
{
  uint8_t buf[5];
  uint16_t pan_id;

  if(1) {
    int i;
    printf("Setting address : ");
    for(i = 0; i < sizeof(uip_lladdr.addr); i++) {
      printf("%02x", data[i]);
      if((i + 1) % 2 == 0) {
        printf(":");
      }
    }
    printf("\n");
  }
  memcpy(uip_lladdr.addr, data, sizeof(uip_lladdr.addr));
  linkaddr_set_node_addr((linkaddr_t *)uip_lladdr.addr);

#ifdef RF_CHANNEL
  /* Set radio channel in slip radio*/
  buf[0] = '!';
  buf[1] = 'C';
  buf[2] = RF_CHANNEL;
  write_to_slip(buf, 3);
#endif
  /* Set pan id in slip radio */
  pan_id = framer_802154_get_pan_id();
  buf[0] = '!';
  buf[1] = 'P';
  buf[2] = (pan_id >> 8) & 0xff;
  buf[3] = pan_id & 0xff;
  write_to_slip(buf, 4);

  /* set the mode to 1 */
  buf[0] = '!';
  buf[1] = 'm';
  buf[2] = 1;
  write_to_slip(buf, 3);

  /* is this ok - should instead remove all addresses and
     add them back again - a bit messy... ?*/

  mac_set = 1;

  /* Enable beacon request responses */
  handler_802154_join(pan_id, 1);
}
/*---------------------------------------------------------------------------*/
void
border_router_set_radio_watchdog(uint16_t seconds)
{
  uint8_t buf[5];
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = 'W';
  buf[3] = (seconds >> 8) & 0xff;
  buf[4] = seconds & 0xff;
  write_to_slip(buf, 5);
}
/*---------------------------------------------------------------------------*/
void
border_router_set_frontpanel_info(uint16_t info)
{
  uint8_t buf[5];
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = 'l';
  buf[3] = (info >> 8) & 0xff;
  buf[4] = info & 0xff;
  write_to_slip(buf, 5);
}
/*---------------------------------------------------------------------------*/
void
border_router_print_stat()
{
  YLOG_INFO("ENCAP: received %u bytes payload, %u errors\n",
            BRM_STATS_GET(BRM_STATS_ENCAP_RECV),
            BRM_STATS_GET(BRM_STATS_ENCAP_ERRORS));
  YLOG_INFO("SLIP: received %u bytes, %u frames, %u overflows, %u dropped\n",
            BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_RECV),
            BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_FRAMES),
            BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_OVERFLOWS),
            BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_DROPPED));
  YLOG_INFO("SLIP: sent %lu bytes, %lu frames, %ld packets pending\n",
            slip_sent_to_fd, slip_sent, slip_buffered());
}
/*---------------------------------------------------------------------------*/
void
border_router_set_baudrate(unsigned rate, int testonly)
{
}
void
border_router_set_ctsrts(uint32_t ctsrts)
{
}
void
process_tlv_from_radio(uint8_t *stack, int stacklen)
{
}
/*---------------------------------------------------------------------------*/
int
devopen(const char *dev, int flags)
{
  char t[1024];
  strcpy(t, "/dev/");
  strncat(t, dev, sizeof(t) - 1 - 5);
  return open(t, flags);
}
/*---------------------------------------------------------------------------*/
static struct etimer et;
PROCESS_THREAD(native_node_process, ev, data)
{
  PROCESS_BEGIN();

  watchdog_init();
  /* Start watchdog immediately */
  /* watchdog_start(); */

  PROCESS_PAUSE();

  watchdog_periodic();

  YLOG_INFO("Debug node started\n");
  br_config_handle_arguments(contiki_argc, contiki_argv);

  /* First init slip so we can get the mac address from the radio */
  slip_init();

  process_start(&border_router_cmd_process, NULL);
  process_start(&serial_input_process, NULL);

  etimer_set(&et, CLOCK_SECOND / 32);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  watchdog_periodic();

#ifdef HAVE_OAM

  YLOG_INFO("Native-Debug-Node (RPL):\n");
  YLOG_INFO("  sw revision: %s\n", getswrevision());
  YLOG_INFO("  Compiled at: %s\n", getcompiletime());
#endif /* HAVE_OAM */

  YLOG_INFO("Network Configuration:\n");
  printf("  %u routes\n  %u neighbors\n  %u default routers\n  %u prefixes\n"
         "  %u unicast addresses\n  %u multicast addresses\n"
         "  %u anycast addresses\n",
         UIP_DS6_ROUTE_NB, NBR_TABLE_MAX_NEIGHBORS, UIP_DS6_DEFRT_NB,
         UIP_DS6_PREFIX_NB, UIP_DS6_ADDR_NB, UIP_DS6_MADDR_NB, UIP_DS6_AADDR_NB);
  printf("  %lu seconds ND6 reachable\n",
         (unsigned long)(UIP_ND6_REACHABLE_TIME / 1000));
#if RPL_DEFAULT_LIFETIME == 0xff && RPL_DEFAULT_LIFETIME_UNIT == 0xffff
  printf("  Infinite default lifetime for RPL\n");
#else
  printf("  %u default lifetime units, %u seconds per unit, for RPL\n",
         RPL_DEFAULT_LIFETIME, RPL_DEFAULT_LIFETIME_UNIT);
#endif

  YLOG_DEBUG("Registering Join Callback\n");
  rpl_set_join_callback(node_join_dag);

#if CONF_6LOWPAN_ND
  printf("Config ND: %d\n", CONF_6LOWPAN_ND);
  printf("  SEND_RA: %d\n", UIP_CONF_ND6_SEND_RA);
  printf("  SEND_NA: %d\n", UIP_CONF_ND6_SEND_NA);
  printf(" SEND_6LR: %d\n", UIP_CONF_6LR);
#endif
  /* Request radio protocol version */
  write_to_slip((uint8_t *)"?v", 2);

  while(!mac_set) {
    printf("Requesting MAC\n");
    request_mac();
    etimer_set(&et, CLOCK_SECOND / 8);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    watchdog_periodic();
  }
  PROCESS_END();
}

/* Started when network is up */
PROCESS_THREAD(native_node_started_process, ev, data)
{
  PROCESS_BEGIN();
#ifdef HAVE_OAM
  static void (*initfuncs[])(instanceControl[]) = {
    instance_nstats_init, NULL
  };
  oam_init(initfuncs);
  process_start(&oam, NULL);
#endif /* HAVE_OAM */

  /* The border router runs with a 100% duty cycle in order to ensure high
     packet reception rates. */
  NETSTACK_MAC.off(1);

#if DEBUG
  print_local_addresses();
#endif

#ifdef HAVE_NETSCAN
  netscan_init();
#endif /* HAVE_NETSCAN */

#if WITH_IPSO
  ipso_objects_init();
  /* bbbb::20c:29ff:fe0c:1b84 */
  /* uip_ip6addr(&server_addr, 0xbbbb, 0, 0, 0, 0x20c, 0x29ff, 0xfe0c, 0x1b84); */
  /* uip_ip6addr(&server_addr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x1); */
  /* lwm2m_register_with(&server_addr); */
#endif

#ifdef MY_APP_INIT_FUNCTION
  MY_APP_INIT_FUNCTION();
#endif /* MY_APP_INIT_FUNCTION */

#ifdef MY_APP_PROCESS
  process_start(&MY_APP_PROCESS, NULL);
#endif /* MY_APP_PROCESS */

  uip_icmp6_echo_reply_callback_add(&reply_notification, echo_reply_callback);

  while(1) {
    etimer_set(&et, CLOCK_SECOND * 10);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    watchdog_periodic();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
