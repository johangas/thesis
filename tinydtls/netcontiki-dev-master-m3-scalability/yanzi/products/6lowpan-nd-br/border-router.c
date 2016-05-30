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
 *
 * This file is part of the Contiki operating system.
 *
 */
/**
 * \file
 *         border-router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

#define LOG6LBR_MODULE "BORDER-ROUTER"

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
#include "ip-bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_WEBSERVER
#include "webserver.h"
#endif

#include "udp-cmd.h"
#ifdef HAVE_OAM
#include <time.h>
#include "oam.h"
#include "api.h"
#include "encap.h"
void rtable_init(instanceControl IC[]);
void radio_instances_init(instanceControl IC[]);
void br_init(instanceControl IC[]);
void instance_brm_init(instanceControl IC[]);
void instance_nstats_init(instanceControl IC[]);
#endif /* HAVE_OAM */

#define YLOG_LEVEL YLOG_LEVEL_DEBUG
#define YLOG_NAME  "nbr"
#include "lib/ylog.h"

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

#ifndef BORDER_ROUTER_SEND_INITIAL_DIS
#define BORDER_ROUTER_SEND_INITIAL_DIS 4
#endif /* BORDER_ROUTER_SEND_INITIAL_DIS */

extern unsigned long slip_sent;
extern unsigned long slip_sent_to_fd;
unsigned long slip_buffered(void);

static uint8_t dag_init_version = 0;
static uint8_t has_dag_version = 0;
static uint8_t setup_done = 0;

extern int contiki_argc;
extern char **contiki_argv;
extern const char *slip_config_ipaddr;
extern const char *slip_config_run_command;
extern uint8_t slip_config_is_slave;
extern uint8_t uip_nd6_proxy_active;
extern uint8_t uip_nd6_process_ra_onlink;
uint8_t radio_info = 0;

#ifdef HAVE_BORDER_ROUTER_SSDP
void ssdp_responder_init(uint16_t port);
#endif /* HAVE_BORDER_ROUTER_SSDP */

CMD_HANDLERS(border_router_cmd_handler);

PROCESS(border_router_process, "Border router process");
PROCESS(border_router_started_process, "Border router process");
PROCESS_NAME(serial_input_process);

AUTOSTART_PROCESSES(&border_router_started_process);

/*---------------------------------------------------------------------------*/
static void
send_periodic(void) {
#ifdef HAVE_OAM
  send_poll_response();
#endif /* HAVE_OAM */
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
int
border_router_is_slave(void)
{
  return slip_config_is_slave;
}
/*---------------------------------------------------------------------------*/
void
border_router_set_slave(int slave)
{
  slip_config_is_slave = slave != 0;
}
/*---------------------------------------------------------------------------*/
int
border_router_is_slave_active(void)
{
#if HAVE_BORDER_ROUTER_SERVER
  return border_router_server_has_client_connection();
#else /* HAVE_BORDER_ROUTER_SERVER */
  return 0;
#endif /* HAVE_BORDER_ROUTER_SERVER */
}
/*---------------------------------------------------------------------------*/
int
contiki_wait_for_mac(void)
{
  static int mac_req_init = 0;

  /* need to wait more if radio_mac_addr_ready is not 1 */
  if(!mac_req_init) {
    /* start the process... */
    process_start(&border_router_process, NULL);
    mac_req_init = 1;
  }

  return radio_mac_addr_ready != 1 || !setup_done;
}
/*---------------------------------------------------------------------------*/
void
border_router_set_mac(const uint8_t *data)
{
  uint8_t buf[5];
  uint16_t pan_id;

  if (1) {
    int i;
    printf("Setting address : ");
    for (i = 0 ; i < sizeof(uip_lladdr.addr); i++) {
      printf("%02x", data[i]);
      if ((i +1) % 2 == 0) {
        printf(":");
      }
    }
    printf("\n");
  }
  memcpy(uip_lladdr.addr, data, sizeof(uip_lladdr.addr));
  linkaddr_set_node_addr((linkaddr_t *)uip_lladdr.addr);
  radio_mac_addr_ready = 1;

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

  /* Enable beacon request responses */
  handler_802154_join(pan_id, 1);
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
static uint8_t pending_on;
static uint8_t pending_off;
static unsigned int original_baudrate;
static unsigned int pending_baudrate;
static struct ctimer pending_on_timer;
static struct ctimer pending_off_timer;
#define PENDING_CTSRTS       (1 << 0)
#define PENDING_BAUDRATE     (1 << 1)
#define PENDING_START_DELAY  (CLOCK_SECOND / 2)
#define PENDING_TEST_PERIOD  (CLOCK_SECOND * 30)
/*---------------------------------------------------------------------------*/
static void
pending_on_callback(void *ptr)
{
  if(pending_on & PENDING_CTSRTS) {
    pending_on &= ~PENDING_CTSRTS;
    serial_set_ctsrts(1);
  }

  if(pending_on & PENDING_BAUDRATE) {
    pending_on &= ~PENDING_BAUDRATE;
    serial_set_baudrate(pending_baudrate);
  }
}
/*---------------------------------------------------------------------------*/
static void
pending_off_callback(void *ptr)
{
  if(pending_off & PENDING_CTSRTS) {
    pending_off &= ~PENDING_CTSRTS;
    serial_set_ctsrts(0);
    YLOG_INFO("Disabling CTS/RTS\n");
  }

  if(pending_off & PENDING_BAUDRATE) {
    pending_off &= ~PENDING_BAUDRATE;
    serial_set_baudrate(original_baudrate);
    YLOG_INFO("Restoring baudrate\n");
  }
}
/*---------------------------------------------------------------------------*/
void
border_router_set_ctsrts(uint32_t ctsrts)
{
  uint8_t buf[8];
  pending_on &= ~PENDING_CTSRTS;
  pending_off &= ~PENDING_CTSRTS;
  if(ctsrts == 2) {
    printf("Configuring serial radio to use CTS/RTS\n");
    pending_on |= PENDING_CTSRTS;
    ctimer_set(&pending_on_timer, PENDING_START_DELAY,
               pending_on_callback, NULL);
  } else if(ctsrts == 1) {
    printf("Configuring serial radio to test CTS/RTS\n");
    pending_on |= PENDING_CTSRTS;
    ctimer_set(&pending_on_timer, PENDING_START_DELAY,
               pending_on_callback, NULL);
    pending_off |= PENDING_CTSRTS;
    ctimer_set(&pending_off_timer, PENDING_TEST_PERIOD,
               pending_off_callback, NULL);
  } else {
    printf("Configuring serial radio to not use CTS/RTS\n");
    ctsrts = 0;
    serial_set_ctsrts(0);
  }
  /* Configure serial radio */
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = 'c';
  buf[3] = ctsrts & 0xff;
  write_to_slip(buf, 4);
}
/*---------------------------------------------------------------------------*/
void
border_router_set_baudrate(unsigned rate, int testonly)
{
  uint8_t buf[8];
  if(rate < 1200) {
    YLOG_ERROR("Illegal baudrate: %u!\n", rate);
    return;
  }
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = testonly ? 'U' : 'u';
  buf[3] = (rate >> 24) & 0xff;
  buf[4] = (rate >> 16) & 0xff;
  buf[5] = (rate >> 8) & 0xff;
  buf[6] = rate & 0xff;
  write_to_slip(buf, 7);
  if(original_baudrate == 0) {
    original_baudrate = serial_get_baudrate();
  }
  pending_baudrate = rate;
  if(testonly) {
    pending_off |= PENDING_BAUDRATE;
    ctimer_set(&pending_off_timer, PENDING_TEST_PERIOD,
               pending_off_callback, NULL);
  } else {
    pending_off &= ~PENDING_BAUDRATE;
  }
  pending_on |= PENDING_BAUDRATE;
  ctimer_set(&pending_on_timer, PENDING_START_DELAY,
             pending_on_callback, NULL);
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
static void
set_prefix_64(const uip_ipaddr_t *prefix_64)
{
  uip_ds6_addr_t *lladdr;

  /* configure the dag_id as this nodes global IPv6 address */
  memcpy(&wsn_net_prefix, prefix_64, 16);
  wsn_net_prefix_len = 64;
  memcpy(&wsn_ip_addr, prefix_64, 16);
  memcpy(&wsn_rpl_dag_id, prefix_64, 16);

  /* update lladdr */
  lladdr = uip_ds6_get_link_local(-1);
  uip_ds6_set_addr_iid(&lladdr->ipaddr, &uip_lladdr);
  uip_ds6_set_addr_iid(&wsn_rpl_dag_id, &uip_lladdr);
  uip_ds6_set_addr_iid(&wsn_ip_addr, &uip_lladdr);
  uip_ds6_addr_add(&wsn_rpl_dag_id, 0, ADDR_AUTOCONF);

  /* prefixes and 6lowpan nd config */
  uip_ds6_prefix_add(&wsn_net_prefix, 64, 1, 0xc0, 86400, 14400);
  uip_ds6_context_pref_add(&wsn_net_prefix, 16, 10);
  uip_ds6_br_config();
}
/*---------------------------------------------------------------------------*/
static void
setup_config(void)
{
  uip_ds6_addr_t *local = uip_ds6_get_link_local(-1);
  if(local != NULL) {
    uip_ipaddr_copy(&wsn_ip_local_addr, &local->ipaddr);
  }
}
/*---------------------------------------------------------------------------*/
static int
br_join_dag(rpl_dio_t *dio)
{
  static uint8_t last_version;

  if(uip_ip6addr_cmp(&dio->dag_id, &wsn_rpl_dag_id)) {
    /* Only print if we see another network */
    if(dag_init_version != dio->version && last_version != dio->version) {
      YLOG_DEBUG("DIO: Version:%d InstanceID:%d\n", dio->version, dio->instance_id);
      YLOG_DEBUG("Found network with me as root!\n");
      last_version = dio->version;
    }
    /* here we should check that the last dag_init_version was less than
     * dio->version so that we get the highest version to start with...
     * main problem is if the lollipop version is in the start region so that
     * it is impossible to detect reboot */
    if(dio->version > RPL_LOLLIPOP_CIRCULAR_REGION) {
      dag_init_version = dio->version;
      has_dag_version = 1;
    }
  } else {
    YLOG_DEBUG("DIO: Version:%d InstanceID:%d\n", dio->version, dio->instance_id);
    YLOG_DEBUG("Found network with other root ");
    PRINT6ADDR(&dio->dag_id);
    PRINTF(" me:");
    PRINT6ADDR(&wsn_rpl_dag_id);
    PRINTF("\n");
  }

  /* never join */
  return 0;
}
/*---------------------------------------------------------------------------*/
/* shared etimer */
static struct etimer et;

PROCESS_THREAD(border_router_process, ev, data)
{
  PROCESS_BEGIN();

  watchdog_init();
  /* Start watchdog immediately */
  /* watchdog_start(); */

  process_start(&border_router_cmd_process, NULL);
  process_start(&serial_input_process, NULL);

  PROCESS_PAUSE();

  watchdog_periodic();

  YLOG_INFO("RPL-Border router started\n");
  br_config_handle_arguments(contiki_argc, contiki_argv);

  YLOG_DEBUG("Registering Join Callback\n");
  rpl_set_join_callback(br_join_dag);

  slip_init();

#ifdef HAVE_OAM
  udp_cmd_init();
#endif /* HAVE_OAM */

  etimer_set(&et, CLOCK_SECOND / 32);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  watchdog_periodic();

  if(slip_config_run_command != NULL) {
    YLOG_DEBUG("Calling command '%s'\n", slip_config_run_command);
    command_context = CMD_CONTEXT_STDIO;
    cmd_input((uint8_t *)slip_config_run_command, strlen(slip_config_run_command));
    etimer_set(&et, 2 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    /* Command executing - exiting border router */
    exit(EXIT_SUCCESS);
  }

#ifdef HAVE_BORDER_ROUTER_CTRL
  border_router_ctrl_init();
#endif /* HAVE_BORDER_ROUTER_CTRL */

#ifdef HAVE_OAM
  YLOG_INFO("Border router:\n");
  YLOG_INFO("  sw revision: %s\n", getswrevision());
  YLOG_INFO("  Compiled at: %s\n", getcompiletime());
#endif /* HAVE_OAM */
#if CLOCK_USE_CLOCK_GETTIME
  YLOG_INFO("  System clock: clock_gettime()\n");
#endif
#if CLOCK_USE_RELATIVE_TIME
  YLOG_INFO("  System clock: relative time\n");
#endif
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
  /* Request radio protocol version */
  write_to_slip((uint8_t *)"?v", 2);

#ifdef BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING
  /* Set initial front panel info at first start */
  border_router_set_frontpanel_info(BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING);
#endif /* BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING */

  while((radio_info & RADIO_INFO_NEEDED_INFO) != RADIO_INFO_NEEDED_INFO) {
    etimer_set(&et, CLOCK_SECOND);
    if(radio_info != 0) {
      YLOG_DEBUG("Requesting radio info (0x%02x)...\n", RADIO_INFO_NEEDED_INFO - radio_info);
    } else {
      YLOG_DEBUG("Requesting radio info...\n");
    }
    get_radio_info();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    watchdog_periodic();
  }

  /* DONE - let the next process kick in with the autostart */
  printf("***** Setup Done *****\n");
  /* Radio Info => MAC SET */
  setup_done = 1;

  PROCESS_END();
}

/* ---------- */

PROCESS_THREAD(border_router_started_process, ev, data)
{
  static int send_dis = BORDER_ROUTER_SEND_INITIAL_DIS;
  static uint64_t wait_started;
  static int wait_turns;

#ifdef HAVE_OAM
  static void (*initfuncs[])(instanceControl[]) = {
    br_init, rtable_init, radio_instances_init, instance_brm_init, instance_nstats_init, NULL
  };
#endif /* HAVE_OAM */

  PROCESS_BEGIN();

  YLOG_DEBUG("Radio info received!\n");
  YLOG_INFO("Radio sw revision: %s\n", instance_brm_get_radio_sw_revision());
  YLOG_INFO("      bootloader:  %lu  capabilities: 0x%lx%08lx\n",
            instance_brm_get_radio_bootloader_version(),
            (unsigned long)((instance_brm_get_radio_capabilities() >> 32ULL) & 0xffffffffUL),
            (unsigned long)(instance_brm_get_radio_capabilities() & 0xffffffffUL));

  if (((instance_brm_get_radio_capabilities() & GLOBAL_CHASSIS_CAPABILITY_CTS_RTS) &&
      ((instance_brm_get_radio_capabilities() & GLOBAL_CHASSIS_CAPABILITY_MASK) == 0))) {
    YLOG_INFO("Enabling CTS/RTS\n");

    border_router_set_ctsrts(1);

    etimer_set(&et, CLOCK_SECOND / 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    YLOG_DEBUG("Waiting for radio info with CTS/RTS\n");

    /* Request number of instances again to test the serial communication */
    radio_info &= ~RADIO_INFO_INSTANCES;
    wait_turns = 0;
    while((radio_info & RADIO_INFO_NEEDED_INFO) != RADIO_INFO_NEEDED_INFO) {
      etimer_set(&et, CLOCK_SECOND);
      get_radio_info();
      wait_turns++;
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      if(wait_turns > 45) {
        break;
      }
      watchdog_periodic();
    }

    if(wait_turns > 15) {
      YLOG_INFO("No working CTS/RTS after %d seconds\n", wait_turns + 1);
    } else {
      YLOG_INFO("CTS/RTS seems to work after %d seconds!\n", wait_turns + 1);
      border_router_set_ctsrts(2);
      etimer_set(&et, CLOCK_SECOND / 2);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    }
  }

  watchdog_periodic();

  if(!wait_for_address) {
    /* Stand alone mode - configure radio to defaults */
    border_router_set_radio_mode(0x01);
  }

#ifdef HAVE_OAM
  /* Have all info from radio, init OAM and UDP server */
  udp_cmd_start();
  oam_init(initfuncs);
#endif /* HAVE_OAM */

#ifdef HAVE_BORDER_ROUTER_SERVER
  border_router_server_init();
#endif /* HAVE_BORDER_ROUTER_SERVER */

#if defined(HAVE_BORDER_ROUTER_SERVER) && defined(HAVE_BORDER_ROUTER_SSDP)
  if(border_router_is_slave()) {
    ssdp_responder_init(border_router_server_get_port());
  }
#endif /* defined(HAVE_BORDER_ROUTER_SERVER) && defined(HAVE_BORDER_ROUTER_SSDP) */

  if(slip_config_is_slave) {
    YLOG_INFO("in slave mode\n");

    while(slip_config_is_slave || border_router_is_slave_active()) {
      etimer_set(&et, CLOCK_SECOND * 30);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
#if BORDER_ROUTER_SET_RADIO_WATCHDOG > 0 && defined(HAVE_BORDER_ROUTER_SERVER)
      /* Keep the radio watchdog updated if in slave mode without server connection. */
      if(! border_router_is_slave_active()) {
        border_router_set_radio_watchdog(BORDER_ROUTER_SET_RADIO_WATCHDOG);
      }
#endif /* BORDER_ROUTER_SET_RADIO_WATCHDOG > 0 && defined(HAVE_BORDER_ROUTER_SERVER) */
    }
  }

  if(wait_for_address) {
      YLOG_DEBUG("RPL-Border router wait for address\n");
      wait_turns = 0;
      wait_started = boottimer_read();
      while(wait_for_address) {
          etimer_set(&et, CLOCK_SECOND * 2);
          PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
          if(slip_config_is_slave || border_router_is_slave_active()) {
            /* wait until no longer slave */
            continue;
          }
          send_periodic();
          if (wait_turns > 5) {
            if(boottimer_elapsed(wait_started) > (10 * 60 * 1000UL)) {
              /* The border router appears to have been "waiting" for
                 address more than 10 minutes. This probably means the system
                 time has changed a lot and we should restart the border
                 router to avoid further problems. */
              YLOG_ERROR("The system time has changed %lu minutes while RPL-border router has been waiting for address\n", boottimer_elapsed(wait_started) / (60 * 1000UL));
              YLOG_ERROR("Exiting RPL-border router now!\n");
              exit(EXIT_FAILURE);
            }
          }
          if (wait_turns++ > 100) {
            YLOG_ERROR("RPL-Border router giving up waiting for address.\n");
            exit(EXIT_FAILURE);
          }
          watchdog_periodic();
      }
      YLOG_DEBUG("RPL-Border router have address\n");
  }

  printf("**** NETWORK UP *****\n");

#if CONF_6LOWPAN_ND
  printf(" Config ND: %d\n", CONF_6LOWPAN_ND);
  printf("   SEND_RA: %d\n", UIP_CONF_ND6_SEND_RA);
  printf("   SEND_NA: %d\n", UIP_CONF_ND6_SEND_NA);
  printf(" SEND_6LBR: %d\n", UIP_CONF_6LBR);
  printf("UIP_ROUTER: %d\n", UIP_CONF_ROUTER);
#endif


  if(slip_config_ipaddr != NULL) {
    uip_ipaddr_t prefix;

    if(uiplib_ipaddrconv(slip_config_ipaddr, &prefix)) {
      YLOG_INFO("Setting prefix ");
      PRINT6ADDR(&prefix);
      printf("\n");
      set_prefix_64(&prefix);
    } else {
      YLOG_ERROR("Parse error: %s\n", slip_config_ipaddr);
      exit(EXIT_FAILURE);
    }
  }

  setup_config();

  if(send_dis > 0) {
    etimer_set(&et, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    /* send a couple of DIS and see if we get something back... */
    while(send_dis > 0) {
      YLOG_DEBUG("RPL - Sending DIS %d\n", send_dis);
      dis_output(NULL);
      etimer_set(&et, CLOCK_SECOND * 3);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      watchdog_periodic();
      send_dis--;
    }
  }

  IP_BRIDGE.init();

  if(has_dag_version) {
    /* increase if we found a version */
    RPL_LOLLIPOP_INCREMENT(dag_init_version);
    wsn_rpl_dag = rpl_set_root_with_version(wsn_rpl_instance_id,
                                            &wsn_rpl_dag_id,
                                            dag_init_version);
  } else {
    wsn_rpl_dag = rpl_set_root(wsn_rpl_instance_id, &wsn_rpl_dag_id);
  }

  if(wsn_rpl_dag != NULL) {
    /* rpl_set_prefix(wsn_rpl_dag, &wsn_net_prefix, wsn_net_prefix_len); */
    if(has_dag_version) {
      YLOG_DEBUG("created a new RPL dag with version %u\n", dag_init_version);
    } else {
      YLOG_DEBUG("created a new RPL dag\n");
    }
  }

  print_local_addresses();

#ifdef HAVE_WEBSERVER
  webserver_init();
#endif /* HAVE_WEBSERVER */

#if WITH_COAPSERVER
  /* Initialize the CoAP server and activate resources */
  rest_init_engine();
  rest_activate_resource(&resource_ipv6_neighbors, (char *)"ipv6/neighbors");
  rest_activate_resource(&resource_ipv6_routes, (char *)"ipv6/routes");
  rest_activate_resource(&resource_rpl_info, (char *)"rpl-info");
  rest_activate_resource(&resource_rpl_parent, (char *)"rpl-info/parent");
  rest_activate_resource(&resource_rpl_rank, (char *)"rpl-info/rank");
  rest_activate_resource(&resource_rpl_link_metric, (char *)"rpl-info/link-metric");
#endif

  while(1) {
    etimer_set(&et, CLOCK_SECOND * 10);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    watchdog_periodic();
    /* Only send periodic information when not running as slave */
    if(!slip_config_is_slave && !border_router_is_slave_active()) {
      send_periodic();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
