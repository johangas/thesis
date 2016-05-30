/*
 * Copyright (c) 2013, CETIC.
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
 *         Main 6LBR process and initialisation
 * \author
 *         6LBR Team <6lbr@cetic.be>
 */

#define LOG6LBR_MODULE "6LBR"

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-nd6.h"
#include "net/ipv6/uip-ds6-route-info.h"
#include "net/rpl/rpl.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "log-6lbr.h"

#include "cetic-6lbr.h"
#include "packet-filter.h"
#include "eth-drv.h"

#if CETIC_NODE_INFO
#include "node-info.h"
#endif

#include "border-router.h"

#if CONTIKI_TARGET_NATIVE
#include "br-config.h"
#include <arpa/inet.h>
#endif
#include "watchdog.h"

void send_purge_na(uip_ipaddr_t *prefix);

#define UIP_IP_BUF              ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF            ((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_ND6_NA_BUF          ((uip_nd6_na *)&uip_buf[uip_l2_l3_icmp_hdr_len])
extern uint8_t uip_nd6_send_rs;
/*---------------------------------------------------------------------------*/
/* Called when regular route-lookup failed */
uip_ipaddr_t *
cetic_6lbr_nexthop_lookup(uip_ipaddr_t *addr)
{
  uip_ipaddr_t *nexthop = NULL;
  if(br_config_mode == BR_MODE_SMART_BRIDGE) {
    if (uip_ipaddr_prefixcmp(&wsn_net_prefix, &UIP_IP_BUF->destipaddr, 64)) {
      /* In smart-bridge mode, there is no route towards hosts on the Ethernet side
	 Therefore we have to check the destination and assume the host is on-link */
      nexthop = &UIP_IP_BUF->destipaddr;
    }
  }
  return nexthop;
}



#if CONTIKI_TARGET_NATIVE
void
cetic_6lbr_save_ip(void)
{
  if (ip_config_file_name) {
    char str[INET6_ADDRSTRLEN];
#if CETIC_6LBR_SMARTBRIDGE
    inet_ntop(AF_INET6, (struct sockaddr_in6 *)&wsn_ip_addr, str, INET6_ADDRSTRLEN);
#else
    inet_ntop(AF_INET6, (struct sockaddr_in6 *)&eth_ip_addr, str, INET6_ADDRSTRLEN);
#endif
    FILE *ip_config_file = fopen(ip_config_file_name, "w");
    fprintf(ip_config_file, "%s\n", str);
    fclose(ip_config_file);
  }
}
#endif /* CONTIKI_TARGET_NATIVE */

void
cetic_6lbr_rpl_route_callback(uip_ipaddr_t *prefix, uint8_t status)
{
  if(status == RPL_ROUTE_STATUS_NEW || status == RPL_ROUTE_STATUS_CHANGED) {
    send_purge_na(prefix);
  }
}

/* should be used for smart-bridge only */
int
cetic_6lbr_uip_ds6_na_callback()
{
  uip_ds6_route_t * route;

  /* if not smart bridge - this should just return 0 */
  if(br_config_mode != BR_MODE_SMART_BRIDGE) {
    return 0;
  }

  /* Address Advertisement */
  if((br_config_attrs & BR_MODE_ATTR_MULTIPLE) != 0) {
    if (uip_is_addr_mcast(&UIP_IP_BUF->destipaddr) && uip_is_mcast_group_id_all_nodes(&UIP_IP_BUF->destipaddr)) {
      LOG6LBR_6ADDR(INFO, &UIP_ND6_NA_BUF->tgtipaddr, "Received purge NA for ");

#if CETIC_NODE_INFO
      node_info_rm_by_addr(&UIP_ND6_NA_BUF->tgtipaddr);
#endif
      route = uip_ds6_route_lookup(&UIP_ND6_NA_BUF->tgtipaddr);
      if (route != NULL ) {
	uip_ds6_route_rm(route);
      }
      /* discard packet */
      return 1;
    }
  }
  return 0;
}

void
cetic_6lbr_set_prefix(uip_ipaddr_t * prefix, unsigned len,
                      uip_ipaddr_t * ipaddr)
{
#if CETIC_6LBR_SMARTBRIDGE
  int new_prefix = !uip_ipaddr_prefixcmp(&wsn_net_prefix, prefix, len);
  int new_dag_prefix = wsn_rpl_dag != NULL && !uip_ipaddr_prefixcmp(&wsn_rpl_dag->prefix_info.prefix, prefix, len);
  if(uip_nd6_ignore_ra) {
    LOG6LBR_DEBUG("Ignoring RA\n");
    return;
  }

  printf("*** GOT PREFIX new?: %d\n", new_prefix);

  if(new_prefix) {
    LOG6LBR_6ADDR(INFO, prefix, "Setting prefix : ");

    uip_ipaddr_copy(&wsn_ip_addr, ipaddr);
    uip_ipaddr_copy(&wsn_net_prefix, prefix);
    wsn_net_prefix_len = len;
    LOG6LBR_6ADDR(INFO, &wsn_ip_addr, "Tentative global IPv6 address : ");
#if CONTIKI_TARGET_NATIVE
    cetic_6lbr_save_ip();
#endif /* CONTIKI_TARGET_NATIVE */
  }
  if(new_dag_prefix) {
    rpl_set_prefix(wsn_rpl_dag, prefix, len);
    LOG6LBR_6ADDR(INFO, prefix, "Setting DAG prefix : ");
    rpl_repair_root(wsn_rpl_instance_id);
  }
#endif /* CETIC_6LBR_SMARTBRIDGE */
}

void
cetic_6lbr_init(void)
{
  uip_ds6_addr_t *local = uip_ds6_get_link_local(-1);

  uip_ipaddr_copy(&wsn_ip_local_addr, &local->ipaddr);

  LOG6LBR_6ADDR(INFO, &wsn_ip_local_addr, "Tentative local IPv6 address ");

#if CETIC_6LBR_SMARTBRIDGE

  if(uip_nd6_ignore_ra) {   //Manual configuration
    uip_nd6_send_rs = 0; /* do not send RS */
    if((br_config_attrs & BR_MODE_ATTR_AUTOCONF) != 0) { /* Address auto configuration */
      uip_ipaddr_copy(&wsn_ip_addr, &wsn_net_prefix);
      uip_ds6_set_addr_iid(&wsn_ip_addr, &uip_lladdr);
      uip_ds6_addr_add(&wsn_ip_addr, 0, ADDR_AUTOCONF);
    } else {
      uip_ds6_addr_add(&wsn_ip_addr, 0, ADDR_MANUAL);
    }
    LOG6LBR_6ADDR(INFO, &wsn_ip_addr, "Tentative global IPv6 address ");
    if(!uip_is_addr_unspecified(&eth_dft_router)) {
      uip_ds6_defrt_add(&eth_dft_router, 0);
    }
  } else {                            //End manual configuration
    uip_create_unspecified(&wsn_net_prefix);
    wsn_net_prefix_len = 0;
    uip_create_unspecified(&wsn_ip_addr);
  }
#endif /* CETIC_6LBR_SMARTBRIDGE */
}

void
cetic_6lbr_init_finalize(void)
{
  /* The border router will setup RPL root based on wsn_rpl_dag_id */

#if UIP_CONF_IPV6_RPL && CETIC_6LBR_DODAG_ROOT
  if((br_config_attrs & BR_MODE_ATTR_MANUAL_DODAG) != 0) {
    /* Manual DODAG ID */
    /* wsn_rpl_dag = rpl_set_root(wsn_rpl_instance_id, &wsn_rpl_dag_id); */
  } else if((br_config_attrs & BR_MODE_ATTR_GLOBAL_DODAG) != 0) {
    /* DODAGID = global address used ! */
    uip_ipaddr_copy(&wsn_rpl_dag_id, &wsn_ip_addr);
    /* wsn_rpl_dag = rpl_set_root(wsn_rpl_instance_id, &wsn_ip_addr); */
  } else {
    /* DODAGID = link-local address used ! */
    uip_ipaddr_copy(&wsn_rpl_dag_id, &wsn_ip_local_addr);
    /* wsn_rpl_dag = rpl_set_root(wsn_rpl_instance_id, &wsn_ip_local_addr); */
  }

#if CETIC_6LBR_SMARTBRIDGE
  /* if(uip_nd6_ignore_ra) { */
    /* rpl_set_prefix(wsn_rpl_dag, &wsn_net_prefix, wsn_net_prefix_len); */
  /* } */
#else
  /* rpl_set_prefix(wsn_rpl_dag, &wsn_net_prefix, wsn_net_prefix_len); */
#endif
  /* LOG6LBR_6ADDR(INFO, &wsn_rpl_dag->dag_id, "Configured as DODAG Root "); */
#endif /* UIP_CONF_IPV6_RPL && CETIC_6LBR_DODAG_ROOT */

#if CONTIKI_TARGET_NATIVE
  cetic_6lbr_save_ip();
#endif
}



/*------------------------------------------------------------------*/
static void
create_llao(uint8_t *llao, uint8_t type) {
  llao[UIP_ND6_OPT_TYPE_OFFSET] = type;
  llao[UIP_ND6_OPT_LEN_OFFSET] = UIP_ND6_OPT_LLAO_LEN >> 3;
  memcpy(&llao[UIP_ND6_OPT_DATA_OFFSET], &uip_lladdr, UIP_LLADDR_LEN);
  /* padding on some */
  memset(&llao[UIP_ND6_OPT_DATA_OFFSET + UIP_LLADDR_LEN], 0,
         UIP_ND6_OPT_LLAO_LEN - 2 - UIP_LLADDR_LEN);
}

/*------------------------------------------------------------------*/
void
send_purge_na(uip_ipaddr_t *prefix)
{
  if((br_config_attrs & BR_MODE_ATTR_MULTIPLE) == 0) {
    return;
  }
  LOG6LBR_6ADDR(INFO, prefix, "Send purge NA - TGT: ");

  uip_ext_len = 0;
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->len[0] = 0;       /* length will not be more than 255 */
  UIP_IP_BUF->len[1] = UIP_ICMPH_LEN + UIP_ND6_NA_LEN + UIP_ND6_OPT_LLAO_LEN;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = UIP_ND6_HOP_LIMIT;

  uip_create_linklocal_allnodes_mcast(&UIP_IP_BUF->destipaddr);
  uip_ipaddr_copy(&UIP_IP_BUF->srcipaddr, prefix);

  UIP_ICMP_BUF->type = ICMP6_NA;
  UIP_ICMP_BUF->icode = 0;

  UIP_ND6_NA_BUF->flagsreserved = UIP_ND6_NA_FLAG_OVERRIDE;
  memcpy(&UIP_ND6_NA_BUF->tgtipaddr, prefix, sizeof(uip_ipaddr_t));

  create_llao(&uip_buf[uip_l2_l3_icmp_hdr_len + UIP_ND6_NA_LEN],
	      UIP_ND6_OPT_TLLAO);

  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();

  uip_len =
    UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_NA_LEN + UIP_ND6_OPT_LLAO_LEN;

  tcpip_ipv6_output();
}
/*------------------------------------------------------------------*/
