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
 *         Sets up some commands for the native node
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "cmd.h"
#include "border-router-cmds.h"
#include "dev/serial-line.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/mac/handler-802154.h"
#include "net/mac/framer-802154.h"
#include "vargen.h"
#include <string.h>
#include <stdlib.h>
#include "net/ipv6/uip-icmp6.h"
#include "net/ip/uiplib.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define PING6_NB 5
#define PING6_DATALEN 16

#define UIP_IP_BUF                ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF            ((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])


extern int debug_join_limit;
clock_time_t ping_time = 0;

/*---------------------------------------------------------------------------*/
static const uint8_t *
hextoi(const uint8_t *buf, int len, int *v)
{
  *v = 0;
  for(; len > 0; len--, buf++) {
    if(*buf >= '0' && *buf <= '9') {
      *v = (*v << 4) + ((*buf - '0') & 0xf);
    } else if(*buf >= 'a' && *buf <= 'f') {
      *v = (*v << 4) + ((*buf - 'a' + 10) & 0xf);
    } else if(*buf >= 'A' && *buf <= 'F') {
      *v = (*v << 4) + ((*buf - 'A' + 10) & 0xf);
    } else {
      break;
    }
  }
  return buf;
}
/*---------------------------------------------------------------------------*/
static const uint8_t *
dectoi(const uint8_t *buf, int len, int *v)
{
  int negative = 0;
  *v = 0;
  if(len == 0) {
    return buf;
  }
  if(*buf == '$') {
    return hextoi(buf + 1, len - 1, v);
  }
  if(*buf == '0' && *(buf + 1) == 'x' && len > 2) {
    return hextoi(buf + 2, len - 2, v);
  }
  if(*buf == '-') {
    negative = 1;
    buf++;
  }
  for(; len > 0; len--, buf++) {
    if(*buf < '0' || *buf > '9') {
      break;
    }
    *v = (*v * 10) + ((*buf - '0') & 0xf);
  }
  if(negative) {
    *v = - *v;
  }
  return buf;
}
/*---------------------------------------------------------------------------*/
static void
printhex(char c)
{
  const char *hex = "0123456789ABCDEF";
  putchar(hex[(c >> 4) & 0xf]);
  putchar(hex[c & 0xf]);
}
/*---------------------------------------------------------------------------*/
void
send_ping(uip_ipaddr_t *dest_addr) 
{
  static uint8_t count = 0;
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 1;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = uip_ds6_if.cur_hop_limit;
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, dest_addr);
  uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);

  UIP_ICMP_BUF->type = ICMP6_ECHO_REQUEST;
  UIP_ICMP_BUF->icode = 0;
  /* set identifier and sequence number to 0 */
  memset((uint8_t *)UIP_ICMP_BUF + UIP_ICMPH_LEN, 0, 4);
  /* put one byte of data */
  memset((uint8_t *)UIP_ICMP_BUF + UIP_ICMPH_LEN + UIP_ICMP6_ECHO_REQUEST_LEN,
	 count, PING6_DATALEN);

  uip_len = UIP_ICMPH_LEN + UIP_ICMP6_ECHO_REQUEST_LEN + UIP_IPH_LEN
    + PING6_DATALEN;
  UIP_IP_BUF->len[0] = (uint8_t)((uip_len - 40) >> 8);
  UIP_IP_BUF->len[1] = (uint8_t)((uip_len - 40) & 0x00FF);
  
  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();

  tcpip_ipv6_output();
  ping_time = clock_time();
}
  
/*---------------------------------------------------------------------------*/
int
debug_cmd_handler(const uint8_t *data, int len)
{
  /* handle global repair, etc here */
  if(strncmp("jl=", (const char *)data, 3) == 0) {
    int limit = 0;
    dectoi(&data[3], len - 3, &limit);
    printf("*** Join limit set to:%d\n", limit);
    debug_join_limit = limit;
    return 1;
  } else if(strncmp("dis", (const char *)data, 3) == 0) {
    printf("*** sending DIS\n");
    dis_output(NULL);
    return 1;
  } else if(strncmp("rep", (const char *)data, 3) == 0) {
    /* reset RPL state */
    rpl_dag_t *dag = rpl_get_any_dag();
    if(dag != NULL) {
      printf("*** Local repair of dag instance\n");
      rpl_local_repair(dag->instance);
    }
    return 1;
  } else if(strncmp("rpl", (const char *)data, 3) == 0) {
    /* reset RPL state */
    rpl_dag_t *dag = rpl_get_any_dag();
    if(dag != NULL) {
      printf("DAG: rank:%d version:%d\n", dag->rank, dag->version);
    } else {
      printf("No DAG found - not joined yet\n");
    }
    return 1;
  } else if(strncmp("ping ", (const char *)data, 5) == 0) {
    uip_ip6addr_t ipaddr;
    if(uiplib_ip6addrconv((const char *) &data[5], &ipaddr)) {
      printf("Pinging: ");
      uip_debug_ipaddr_print(&ipaddr);
      printf("\n");
      send_ping(&ipaddr);
    } else {
      printf("Error: Could not parse address to ping. "
             "Please specify an IPv6 address.\n");
    }
    return 1;
  } else if(strncmp("nbr", (const char *)data, 6) == 0) {
    uip_ds6_nbr_t *nbr;
    printf("Neighbors:\n");
    for(nbr = nbr_table_head(ds6_neighbors);
        nbr != NULL;
        nbr = nbr_table_next(ds6_neighbors, nbr)) {
      uip_debug_ipaddr_print(&nbr->ipaddr);
      printf(" %lu %d\n", stimer_remaining(&nbr->reachable), nbr->state);
    }
    return 1;
  } else if(strncmp("routes", (const char *)data, 6) == 0) {
    uip_ds6_route_t *r;
    uip_ds6_defrt_t *defrt;
    uip_ipaddr_t *ipaddr;
    defrt = NULL;
    if((ipaddr = uip_ds6_defrt_choose()) != NULL) {
      defrt = uip_ds6_defrt_lookup(ipaddr);
    }
    if(defrt != NULL) {
      printf("DefRT: :: -> ");
      uip_debug_ipaddr_print(&defrt->ipaddr);
      printf(" lt:%lu inf:%d\n", stimer_remaining(&defrt->lifetime),
             defrt->isinfinite);
    } else {
      printf("DefRT: :: -> NULL\n");
    }

    printf("Routes:\n");
    for(r = uip_ds6_route_head();
        r != NULL;
        r = uip_ds6_route_next(r)) {
      uip_debug_ipaddr_print(&r->ipaddr);
      printf(" -> ");
      if(uip_ds6_route_nexthop(r) != NULL) {
        uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
      } else {
        printf("NULL");
      }
      if(r->state.lifetime < 600) {
        printf(" %d s\n", r->state.lifetime);
      } else {
        printf(" >600 s\n");
      }
    }
    return 1;
  }
  if(0) {
    printhex(14);
  }

  return 0;
}
