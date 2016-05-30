/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
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
 */

/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *    IPv6 data structure manipulation.
 *    Comprises part of the Neighbor discovery (RFC 4861)
 *    and auto configuration (RFC 4862) state machines.
 * \author Mathilde Durvy <mdurvy@cisco.com>
 * \author Julien Abeille <jabeille@cisco.com>
 */

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "lib/random.h"
#include "net/ipv6/uip-nd6.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-context.h"
#include "net/ip/uip-packetqueue.h"

#if CONF_6LOWPAN_ND_OPTI_START && UIP_CONF_6L_ROUTER
#include "net/rpl/rpl.h"
#endif

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

struct etimer uip_ds6_timer_periodic;                           /** \brief Timer for maintenance of data structures */

#if UIP_CONF_ROUTER
struct stimer uip_ds6_timer_ra;                                 /** \brief RA timer, to schedule RA sending */
#if UIP_ND6_SEND_RA
static uint8_t racount;                                         /** \brief number of RA already sent */
static uint16_t rand_time;                                      /** \brief random time value for timers */
#endif
#endif /* UIP_CONF_ROUTER */
#if !UIP_CONF_ROUTER || CONF_6LOWPAN_ND
struct etimer uip_ds6_timer_rs;                                 /** \brief RS timer, to schedule RS sending */
static uint8_t rscount;                                         /** \brief number of rs already sent */
#endif /* UIP_CONF_ROUTER || CONF_6LOWPAN_ND */

/** \name "DS6" Data structures */
/** @{ */
uip_ds6_netif_t uip_ds6_if;                                       /** \brief The single interface */
uip_ds6_prefix_t uip_ds6_prefix_list[UIP_DS6_PREFIX_NB];          /** \brief Prefix list */
#if CONF_6LOWPAN_ND
#if UIP_CONF_6LBR
uip_ds6_dup_addr_t uip_ds6_dup_addr_list[UIP_DS6_DUPADDR_NB];  /** \brief Duplication Address Detection list */
#endif /* UIP_CONF_6LBR */
#endif /* CONF_6LOWPAN_ND */

/* Used by Cooja to enable extraction of addresses from memory.*/
uint8_t uip_ds6_addr_size;
uint8_t uip_ds6_netif_addr_list_offset;

/** @} */

/* "full" (as opposed to pointer) ip address used in this file,  */
static uip_ipaddr_t loc_fipaddr;

/* Pointers used in this file */
static uip_ds6_addr_t *locaddr;
static uip_ds6_maddr_t *locmaddr;
static uip_ds6_aaddr_t *locaaddr;
static uip_ds6_prefix_t *locprefix;

extern uint8_t uip_nd6_send_rs;

#if CONF_6LOWPAN_ND
#if UIP_CONF_6LBR
static uip_ds6_dup_addr_t *locdad;
#endif /* UIP_CONF_6LBR */
#if UIP_CONF_ROUTER
static uip_ds6_border_router_t *locbr;
#endif /* UIP_CONF_ROUTER */
#endif /* CONF_6LOWPAN_ND */
#if CONF_6LOWPAN_ND_OPTI_START && UIP_CONF_6L_ROUTER
uint8_t rpl_started;
#endif /* CONF_6LOWPAN_ND_OPTI_START && UIP_CONF_6L_ROUTER */

/*---------------------------------------------------------------------------*/
void
uip_ds6_init(void)
{

  uip_ds6_neighbors_init();
  uip_ds6_route_init();

  PRINTF("Init of IPv6 data structures\n");
  PRINTF("%u neighbors\n%u default routers\n%u prefixes\n%u routes\n%u unicast addresses\n%u multicast addresses\n%u anycast addresses\n",
     NBR_TABLE_MAX_NEIGHBORS, UIP_DS6_DEFRT_NB, UIP_DS6_PREFIX_NB, UIP_DS6_ROUTE_NB,
     UIP_DS6_ADDR_NB, UIP_DS6_MADDR_NB, UIP_DS6_AADDR_NB);
  memset(uip_ds6_prefix_list, 0, sizeof(uip_ds6_prefix_list));
  memset(&uip_ds6_if, 0, sizeof(uip_ds6_if));
  uip_ds6_context_init();
#if CONF_6LOWPAN_ND
  memset(uip_ds6_br_list, 0, sizeof(uip_ds6_br_list));
#if UIP_CONF_6LBR
  memset(uip_ds6_dup_addr_list, 0, sizeof(uip_ds6_dup_addr_list));
#endif /* UIP_CONF_6LBR */
#endif /* CONF_6LOWPAN_ND */
  uip_ds6_addr_size = sizeof(struct uip_ds6_addr);
  uip_ds6_netif_addr_list_offset = offsetof(struct uip_ds6_netif, addr_list);

  /* Set interface parameters */
  uip_ds6_if.link_mtu = UIP_LINK_MTU;
  uip_ds6_if.cur_hop_limit = UIP_TTL;
  uip_ds6_if.base_reachable_time = UIP_ND6_REACHABLE_TIME;
  uip_ds6_if.reachable_time = uip_ds6_compute_reachable_time();
  uip_ds6_if.retrans_timer = UIP_ND6_RETRANS_TIMER;
  uip_ds6_if.maxdadns = UIP_ND6_DEF_MAXDADNS;

  /* Create link local address, prefix, multicast addresses, anycast addresses */
  uip_create_linklocal_prefix(&loc_fipaddr);
#if UIP_CONF_ROUTER
  uip_ds6_prefix_add(&loc_fipaddr, UIP_DEFAULT_PREFIX_LEN, 0, 0, 0, 0);
#else /* UIP_CONF_ROUTER */
  uip_ds6_prefix_add(&loc_fipaddr, UIP_DEFAULT_PREFIX_LEN, 0);
#endif /* UIP_CONF_ROUTER */
  uip_ds6_set_addr_iid(&loc_fipaddr, &uip_lladdr);
  uip_ds6_addr_add(&loc_fipaddr, 0, ADDR_AUTOCONF);

  uip_create_linklocal_allnodes_mcast(&loc_fipaddr);
  uip_ds6_maddr_add(&loc_fipaddr);
#if UIP_CONF_ROUTER
  uip_create_linklocal_allrouters_mcast(&loc_fipaddr);
  uip_ds6_maddr_add(&loc_fipaddr);
#if UIP_ND6_SEND_RA
#if !CONF_6LOWPAN_ND || UIP_ND6_RA_PERIODIC
  PRINTF("Will Sending periodic RA\n");
  stimer_set(&uip_ds6_timer_ra, 2);     /* wait to have a link local IP address */
#endif /* #if !CONF_6LOWPAN_ND || UIP_ND6_RA_PERIODIC */
#endif /* UIP_ND6_SEND_RA */
#if UIP_CONF_6LR
  etimer_set(&uip_ds6_timer_rs,
             random_rand() % (UIP_ND6_MAX_RTR_SOLICITATION_INTERVAL *
                              CLOCK_SECOND));
#endif /* UIP_CONF_6LR */
#else /* UIP_CONF_ROUTER */
#if CONF_6LOWPAN_ND
  etimer_set(&uip_ds6_timer_rs,
             random_rand() % (UIP_ND6_MAX_RTR_SOLICITATION_INTERVAL *
                              CLOCK_SECOND));
#else
  etimer_set(&uip_ds6_timer_rs,
             random_rand() % (UIP_ND6_MAX_RTR_SOLICITATION_DELAY *
                              CLOCK_SECOND));
#endif /* CONF_6LOWPAN_ND */
#endif /* UIP_CONF_ROUTER */
  etimer_set(&uip_ds6_timer_periodic, UIP_DS6_PERIOD);
#if CONF_6LOWPAN_ND_OPTI_START && UIP_CONF_6L_ROUTER
  rpl_started = 0;
#endif /* CONF_6LOWPAN_ND_OPTI_START && UIP_CONF_6L_ROUTER */

  return;
}


/*---------------------------------------------------------------------------*/
void
uip_ds6_periodic(void)
{

#if CONF_6LOWPAN_ND_OPTI_START && UIP_CONF_6L_ROUTER
  /* Start RPL only when the device has global IPv6 */
  if(!rpl_started && uip_ds6_get_global(ADDR_PREFERRED)) {
    rpl_init();
    rpl_started = 1;
  }
#endif /* CONF_6LOWPAN_ND_OPTI_START && UIP_CONF_6L_ROUTER */

  /* Periodic processing on unicast addresses */
  for(locaddr = uip_ds6_if.addr_list;
      locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; locaddr++) {
    if(locaddr->isused) {
      if((!locaddr->isinfinite) && (stimer_expired(&locaddr->vlifetime))) {
        PRINTF("Removing addr ");
        PRINT6ADDR(&locaddr->ipaddr);
        PRINTF("\n");
        uip_ds6_addr_rm(locaddr);
#if UIP_ND6_DEF_MAXDADNS > 0
      } else if((locaddr->state == ADDR_TENTATIVE)
                && (locaddr->dadnscount <= uip_ds6_if.maxdadns)
                && (timer_expired(&locaddr->dadtimer))
                && (uip_len == 0)) {
        uip_ds6_dad(locaddr);
#endif /* UIP_ND6_DEF_MAXDADNS > 0 */
      }
    }
  }

  /* Periodic processing on default routers */
  uip_ds6_defrt_periodic();
  /*  for(locdefrt = uip_ds6_defrt_list;
      locdefrt < uip_ds6_defrt_list + UIP_DS6_DEFRT_NB; locdefrt++) {
    if((locdefrt->isused) && (!locdefrt->isinfinite) &&
       (stimer_expired(&(locdefrt->lifetime)))) {
      uip_ds6_defrt_rm(locdefrt);
    }
    }*/

#if CONF_6LOWPAN_ND
  uip_ds6_context_periodic();
#endif /* CONF_6LOWPAN_ND */

#if !UIP_CONF_ROUTER
  /* Periodic processing on prefixes */
  for(locprefix = uip_ds6_prefix_list;
      locprefix < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB;
      locprefix++) {
    if(locprefix->isused && !locprefix->isinfinite) {
      if(stimer_expired(&(locprefix->vlifetime))) {
        uip_ds6_prefix_rm(locprefix);
#if UIP_CONF_6LR || UIP_CONF_6LN
      } else if(is_timeout_percent(&locprefix->vlifetime, UIP_DS6_RS_PERCENT_LIFETIME_RETRAN,
                                   UIP_DS6_RS_MINLIFETIME_RETRAN)) {
        if(locprefix->br->state != BR_ST_SENDING_RS) {
          locprefix->br->state = BR_ST_MUST_SEND_RS;
        }
#endif /* UIP_CONF_6LR || UIP_CONF_6LN */
      }
    }
  }
#endif /* !UIP_CONF_ROUTER */

#if CONF_6LOWPAN_ND
#if UIP_CONF_6LBR
  /* Periodic processing on Duplication Address*/
  for(locdad = uip_ds6_dup_addr_list;
      locdad < uip_ds6_dup_addr_list + UIP_DS6_DUPADDR_NB;
      locdad++) {
    if(locdad->isused && stimer_expired(&locdad->lifetime)) {
      uip_ds6_dup_addr_rm(locdad);
      PRINTF("Removing dup addr ");
      PRINT6ADDR(&locdad->ipaddr);
      PRINTF("\n");
    }
  }
#else
  /* If not 6LBR handle the border router list */
  uip_ds6_br_periodic();
#endif /* UIP_CONF_6LBR */
#endif /* CONF_6LOWPAN_ND */

  uip_ds6_neighbor_periodic();

#if UIP_CONF_ROUTER && UIP_ND6_SEND_RA
#if !CONF_6LOWPAN_ND || UIP_ND6_RA_PERIODIC
  /* Periodic RA sending */
  if(stimer_expired(&uip_ds6_timer_ra) && (uip_len == 0)) {
    uip_ds6_send_ra_periodic();
  }
#endif /* !CONF_6LOWPAN_ND || UIP_ND6_RA_PERIODIC */
#endif /* UIP_CONF_ROUTER && UIP_ND6_SEND_RA */
  etimer_reset(&uip_ds6_timer_periodic);
  return;
}

/*---------------------------------------------------------------------------*/
uint8_t
uip_ds6_list_loop(uip_ds6_element_t *list, uint8_t size,
                  uint16_t elementsize, uip_ipaddr_t *ipaddr,
                  uint8_t ipaddrlen, uip_ds6_element_t **out_element)
{
  uip_ds6_element_t *element;

  *out_element = NULL;

  for(element = list;
      element <
      (uip_ds6_element_t *)((uint8_t *)list + (size * elementsize));
      element = (uip_ds6_element_t *)((uint8_t *)element + elementsize)) {
    if(element->isused) {
      if(uip_ipaddr_prefixcmp(&element->ipaddr, ipaddr, ipaddrlen)) {
        *out_element = element;
        return FOUND;
      }
    } else {
      *out_element = element;
    }
  }

  return *out_element != NULL ? FREESPACE : NOSPACE;
}

/*---------------------------------------------------------------------------*/
#if UIP_CONF_ROUTER || UIP_CONF_6L_ROUTER
/*---------------------------------------------------------------------------*/
uip_ds6_prefix_t *
uip_ds6_prefix_add(uip_ipaddr_t *ipaddr, uint8_t ipaddrlen,
                   uint8_t advertise, uint8_t flags, unsigned long vtime,
                   unsigned long ptime)
{
  if(uip_ds6_list_loop
     ((uip_ds6_element_t *)uip_ds6_prefix_list, UIP_DS6_PREFIX_NB,
      sizeof(uip_ds6_prefix_t), ipaddr, ipaddrlen,
      (uip_ds6_element_t **)&locprefix) == FREESPACE) {
    locprefix->isused = 1;
    uip_ipaddr_copy(&locprefix->ipaddr, ipaddr);
    locprefix->length = ipaddrlen;
    locprefix->advertise = advertise;
    locprefix->l_a_reserved = flags;
    locprefix->vlifetime_val = vtime;
    locprefix->plifetime = ptime;
#if UIP_CONF_6LR
    if(vtime != 0) {
      stimer_set(&(locprefix->vlifetime), vtime);
      locprefix->isinfinite = 0;
    } else {
      locprefix->isinfinite = 1;
    }
#endif /* UIP_CONF_6LR */
#if UIP_CONF_6LBR
    /* Increase version in border router */
    locbr = uip_ds6_br_lookup(NULL);
    if(locbr != NULL) {
      locbr->state = BR_ST_NEW_VERSION;
    }
#endif /* UIP_CONF_6LBR */
    PRINTF("Adding prefix ");
    PRINT6ADDR(&locprefix->ipaddr);
    PRINTF("length %u, flags %x, Valid lifetime %lx, Preffered lifetime %lx\n",
       ipaddrlen, flags, vtime, ptime);
    return locprefix;
  } else {
    PRINTF("No more space in Prefix list\n");
  }
  return NULL;
}


#else /* UIP_CONF_ROUTER || UIP_CONF_6L_ROUTER */
uip_ds6_prefix_t *
uip_ds6_prefix_add(uip_ipaddr_t *ipaddr, uint8_t ipaddrlen,
                   unsigned long interval)
{
  if(uip_ds6_list_loop
     ((uip_ds6_element_t *)uip_ds6_prefix_list, UIP_DS6_PREFIX_NB,
      sizeof(uip_ds6_prefix_t), ipaddr, ipaddrlen,
      (uip_ds6_element_t **)&locprefix) == FREESPACE) {
    locprefix->isused = 1;
    uip_ipaddr_copy(&locprefix->ipaddr, ipaddr);
    locprefix->length = ipaddrlen;
    if(interval != 0) {
      stimer_set(&(locprefix->vlifetime), interval);
      locprefix->isinfinite = 0;
    } else {
      locprefix->isinfinite = 1;
    }
    PRINTF("Adding prefix ");
    PRINT6ADDR(&locprefix->ipaddr);
    PRINTF("length %u, vlifetime%lu\n", ipaddrlen, interval);
    return locprefix;
  }
  return NULL;
}
#endif /* UIP_CONF_ROUTER || UIP_CONF_6L_ROUTER */

/*---------------------------------------------------------------------------*/
void
uip_ds6_prefix_rm(uip_ds6_prefix_t *prefix)
{
  if(prefix != NULL) {
    prefix->isused = 0;
  }
  return;
}
/*---------------------------------------------------------------------------*/
uip_ds6_prefix_t *
uip_ds6_prefix_lookup(uip_ipaddr_t *ipaddr, uint8_t ipaddrlen)
{
  if(uip_ds6_list_loop((uip_ds6_element_t *)uip_ds6_prefix_list,
		       UIP_DS6_PREFIX_NB, sizeof(uip_ds6_prefix_t),
		       ipaddr, ipaddrlen,
		       (uip_ds6_element_t **)&locprefix) == FOUND) {
    return locprefix;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
#if CONF_6LOWPAN_ND
void
uip_ds6_prefix_rm_all(uip_ds6_border_router_t *border_router)
{
  for(locprefix = uip_ds6_prefix_list;
      locprefix < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB;
      locprefix++) {
    if(locprefix->br == border_router) {
      locprefix->isused = 0;
    }
  }
}
/*---------------------------------------------------------------------------*/
uip_ds6_prefix_t *
uip_ds6_prefix_lookup_from_ipaddr(uip_ipaddr_t *ipaddr)
{
  for(locprefix = uip_ds6_prefix_list;
      locprefix < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB;
      locprefix++) {
    if(locprefix->isused &&
       uip_ipaddr_prefixcmp(&locprefix->ipaddr, ipaddr, locprefix->length)) {
      return locprefix;
    }
  }
  return NULL;
}
#endif /* CONF_6LOWPAN_ND */

/*---------------------------------------------------------------------------*/
uint8_t
uip_ds6_is_addr_onlink(uip_ipaddr_t *ipaddr)
{
#if CONF_6LOWPAN_ND
  return uip_is_addr_link_local(ipaddr);
#else /* CONF_6LOWPAN_ND */
  for(locprefix = uip_ds6_prefix_list;
      locprefix < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB; locprefix++) {
    if(locprefix->isused &&
       uip_ipaddr_prefixcmp(&locprefix->ipaddr, ipaddr, locprefix->length)) {
      return 1;
    }
  }
  return 0;
#endif /* CONF_6LOWPAN_ND */
}
/*---------------------------------------------------------------------------*/
#if UIP_CONF_6LBR
void
uip_ds6_br_config()
{
  uip_ds6_context_pref_t *loccontext;
  /* default value of 10,000 (~one week) */
  locbr = uip_ds6_br_add(0, 0x0, &uip_ds6_get_global(ADDR_PREFERRED)->ipaddr);
  /* link all context to border router */
  for(loccontext = uip_ds6_context_pref_list;
      loccontext < uip_ds6_context_pref_list + UIP_DS6_CONTEXT_PREF_NB;
      loccontext++) {
    if(loccontext->state != CONTEXT_PREF_ST_FREE) {
      loccontext->br = locbr;
    }
  }
  /* link all prefixes to border router */
  for(locprefix = uip_ds6_prefix_list;
      locprefix < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB;
      locprefix++) {
    if(locprefix->isused) {
      locprefix->br = locbr;
    }
  }
}
#endif /* UIP_CONF_6LBR */

/*---------------------------------------------------------------------------*/
#if CONF_6LOWPAN_ND
#if UIP_CONF_6LBR
/*---------------------------------------------------------------------------*/
uip_ds6_dup_addr_t *
uip_ds6_dup_addr_add(uip_ipaddr_t *ipaddr, uint16_t lifetime,
                     uip_lladdr_t *eui64)
{
  if(uip_ds6_list_loop
       ((uip_ds6_element_t *)uip_ds6_dup_addr_list, UIP_DS6_DUPADDR_NB,
       sizeof(uip_ds6_dup_addr_t), ipaddr, 128,
       (uip_ds6_element_t **)&locdad) == FREESPACE) {
    locdad->isused = 1;
    uip_ipaddr_copy(&locdad->ipaddr, ipaddr);
    if(lifetime != 0) {
      stimer_set(&locdad->lifetime, lifetime * 60);
    }
    memcpy(&locdad->eui64, eui64, UIP_LLADDR_LEN);
    return locdad;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
uip_ds6_dup_addr_rm(uip_ds6_dup_addr_t *dad)
{
  if(dad != NULL) {
    dad->isused = 0;
  }
}
/*---------------------------------------------------------------------------*/
uip_ds6_dup_addr_t *
uip_ds6_dup_addr_lookup(uip_ipaddr_t *ipaddr)
{
  if(uip_ds6_list_loop
       ((uip_ds6_element_t *)uip_ds6_dup_addr_list, UIP_DS6_DUPADDR_NB,
       sizeof(uip_ds6_dup_addr_t), ipaddr, 128,
       (uip_ds6_element_t **)&locdad) == FOUND) {
    return locdad;
  }
  return NULL;
}
#endif /* UIP_CONF_6LBR */

#endif /* CONF_6LOWPAN_ND */

/*---------------------------------------------------------------------------*/
uip_ds6_addr_t *
uip_ds6_addr_add(uip_ipaddr_t *ipaddr, unsigned long vlifetime, uint8_t type)
{
  if(uip_ds6_list_loop
     ((uip_ds6_element_t *)uip_ds6_if.addr_list, UIP_DS6_ADDR_NB,
      sizeof(uip_ds6_addr_t), ipaddr, 128,
      (uip_ds6_element_t **)&locaddr) == FREESPACE) {
    locaddr->isused = 1;
    uip_ipaddr_copy(&locaddr->ipaddr, ipaddr);
    locaddr->type = type;
    if(vlifetime == 0) {
      locaddr->isinfinite = 1;
    } else {
      locaddr->isinfinite = 0;
      stimer_set(&(locaddr->vlifetime), vlifetime);
    }
#if UIP_ND6_DEF_MAXDADNS > 0
    locaddr->state = ADDR_TENTATIVE;
    timer_set(&locaddr->dadtimer,
              random_rand() % (UIP_ND6_MAX_RTR_SOLICITATION_DELAY *
                               CLOCK_SECOND));
    locaddr->dadnscount = 0;
#else /* UIP_ND6_DEF_MAXDADNS > 0 */
#if UIP_CONF_6LN || UIP_CONF_6LR
    locaddr->state = uip_is_addr_link_local(ipaddr) ?
      ADDR_PREFERRED : ADDR_TENTATIVE;
#else /* UIP_CONF_6LN || UIP_CONF_6LR */
    locaddr->state = ADDR_PREFERRED;
#endif /* UIP_CONF_6LN || UIP_CONF_6LR */
#endif /* UIP_ND6_DEF_MAXDADNS > 0 */
    uip_create_solicited_node(ipaddr, &loc_fipaddr);
    uip_ds6_maddr_add(&loc_fipaddr);
    return locaddr;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_addr_rm(uip_ds6_addr_t *addr)
{
  if(addr != NULL) {
    uip_create_solicited_node(&addr->ipaddr, &loc_fipaddr);
    if((locmaddr = uip_ds6_maddr_lookup(&loc_fipaddr)) != NULL) {
      uip_ds6_maddr_rm(locmaddr);
    }
    addr->isused = 0;
  }
  return;
}

/*---------------------------------------------------------------------------*/
uip_ds6_addr_t *
uip_ds6_addr_lookup(uip_ipaddr_t *ipaddr)
{
  if(uip_ds6_list_loop
     ((uip_ds6_element_t *)uip_ds6_if.addr_list, UIP_DS6_ADDR_NB,
      sizeof(uip_ds6_addr_t), ipaddr, 128,
      (uip_ds6_element_t **)&locaddr) == FOUND) {
    return locaddr;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
/*
 * get a link local address -
 * state = -1 => any address is ok. Otherwise state = desired state of addr.
 * (TENTATIVE, PREFERRED, DEPRECATED)
 */
uip_ds6_addr_t *
uip_ds6_get_link_local(int8_t state)
{
  for(locaddr = uip_ds6_if.addr_list;
      locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; locaddr++) {
    if(locaddr->isused && (state == -1 || locaddr->state == state)
       && (uip_is_addr_link_local(&locaddr->ipaddr))) {
      return locaddr;
    }
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
/*
 * get a global address -
 * state = -1 => any address is ok. Otherwise state = desired state of addr.
 * (TENTATIVE, PREFERRED, DEPRECATED)
 */
uip_ds6_addr_t *
uip_ds6_get_global(int8_t state)
{
  for(locaddr = uip_ds6_if.addr_list;
      locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; locaddr++) {
    if(locaddr->isused && (state == -1 || locaddr->state == state)
       && !(uip_is_addr_link_local(&locaddr->ipaddr))) {
      return locaddr;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/*
 * get a global address with Border Router
 * state = -1 => any address is ok. Otherwise state = desired state of addr.
 * (TENTATIVE, PREFERRED, DEPRECATED)
 */
#if CONF_6LOWPAN_ND
uip_ds6_addr_t *
uip_ds6_get_global_br(int8_t state, uip_ds6_border_router_t *br)
{
  for(locaddr = uip_ds6_if.addr_list;
      locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; locaddr++) {
    if(locaddr->isused && (state == -1 || locaddr->state == state)
       && !(uip_is_addr_link_local(&locaddr->ipaddr))
       && (locprefix = uip_ds6_prefix_lookup_from_ipaddr(&locaddr->ipaddr))
       && locprefix->br == br) {
      return locaddr;
    }
  }
  return NULL;
}
#endif /* CONF_6LOWPAN_ND */

/*---------------------------------------------------------------------------*/
/*
 * get the number of configured address -
 * state = -1 => any address is ok. Otherwise state = desired state of addr.
 * (TENTATIVE, PREFERRED, DEPRECATED)
 */
int
uip_ds6_get_addr_number(int8_t state)
{
  int count = 0;
  for(locaddr = uip_ds6_if.addr_list;
      locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; locaddr++) {
    if(locaddr->isused && (state == -1 || locaddr->state == state)) {
      count++;
    }
  }
  return count;
}

/*---------------------------------------------------------------------------*/
uip_ds6_maddr_t *
uip_ds6_maddr_add(const uip_ipaddr_t *ipaddr)
{
  if(uip_ds6_list_loop
     ((uip_ds6_element_t *)uip_ds6_if.maddr_list, UIP_DS6_MADDR_NB,
      sizeof(uip_ds6_maddr_t), (void*)ipaddr, 128,
      (uip_ds6_element_t **)&locmaddr) == FREESPACE) {
    locmaddr->isused = 1;
    uip_ipaddr_copy(&locmaddr->ipaddr, ipaddr);
    return locmaddr;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_maddr_rm(uip_ds6_maddr_t *maddr)
{
  if(maddr != NULL) {
    maddr->isused = 0;
  }
  return;
}

/*---------------------------------------------------------------------------*/
uip_ds6_maddr_t *
uip_ds6_maddr_lookup(const uip_ipaddr_t *ipaddr)
{
  if(uip_ds6_list_loop
     ((uip_ds6_element_t *)uip_ds6_if.maddr_list, UIP_DS6_MADDR_NB,
      sizeof(uip_ds6_maddr_t), (void*)ipaddr, 128,
      (uip_ds6_element_t **)&locmaddr) == FOUND) {
    return locmaddr;
  }
  return NULL;
}


/*---------------------------------------------------------------------------*/
uip_ds6_aaddr_t *
uip_ds6_aaddr_add(uip_ipaddr_t *ipaddr)
{
  if(uip_ds6_list_loop
     ((uip_ds6_element_t *)uip_ds6_if.aaddr_list, UIP_DS6_AADDR_NB,
      sizeof(uip_ds6_aaddr_t), ipaddr, 128,
      (uip_ds6_element_t **)&locaaddr) == FREESPACE) {
    locaaddr->isused = 1;
    uip_ipaddr_copy(&locaaddr->ipaddr, ipaddr);
    return locaaddr;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_aaddr_rm(uip_ds6_aaddr_t *aaddr)
{
  if(aaddr != NULL) {
    aaddr->isused = 0;
  }
  return;
}

/*---------------------------------------------------------------------------*/
uip_ds6_aaddr_t *
uip_ds6_aaddr_lookup(uip_ipaddr_t *ipaddr)
{
  if(uip_ds6_list_loop((uip_ds6_element_t *)uip_ds6_if.aaddr_list,
		       UIP_DS6_AADDR_NB, sizeof(uip_ds6_aaddr_t), ipaddr, 128,
		       (uip_ds6_element_t **)&locaaddr) == FOUND) {
    return locaaddr;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_select_src(uip_ipaddr_t *src, uip_ipaddr_t *dst)
{
  uint8_t best = 0;             /* number of bit in common with best match */
  uint8_t n = 0;
  uip_ds6_addr_t *matchaddr = NULL;

  if(!uip_is_addr_link_local(dst) && !uip_is_addr_mcast(dst)) {
    /* find longest match */
    for(locaddr = uip_ds6_if.addr_list;
        locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; locaddr++) {
      /* Only preferred global (not link-local) addresses */
      if(locaddr->isused && locaddr->state == ADDR_PREFERRED &&
         !uip_is_addr_link_local(&locaddr->ipaddr)) {
        n = get_match_length(dst, &locaddr->ipaddr);
        if(n >= best) {
          best = n;
          matchaddr = locaddr;
        }
      }
    }
#if UIP_IPV6_MULTICAST
  } else if(uip_is_addr_mcast_routable(dst)) {
    matchaddr = uip_ds6_get_global(ADDR_PREFERRED);
#endif
  } else {
    matchaddr = uip_ds6_get_link_local(ADDR_PREFERRED);
  }

  /* use the :: (unspecified address) as source if no match found */
  if(matchaddr == NULL) {
    uip_create_unspecified(src);
  } else {
    uip_ipaddr_copy(src, &matchaddr->ipaddr);
  }
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_set_addr_iid(uip_ipaddr_t *ipaddr, uip_lladdr_t *lladdr)
{
  /* We consider only links with IEEE EUI-64 identifier or
   * IEEE 48-bit MAC addresses */
#if (UIP_LLADDR_LEN == 8)
  memcpy(ipaddr->u8 + 8, lladdr, UIP_LLADDR_LEN);
  ipaddr->u8[8] ^= 0x02;
#elif (UIP_LLADDR_LEN == 6)
  memcpy(ipaddr->u8 + 8, lladdr, 3);
  ipaddr->u8[11] = 0xff;
  ipaddr->u8[12] = 0xfe;
  memcpy(ipaddr->u8 + 13, (uint8_t *)lladdr + 3, 3);
  ipaddr->u8[8] ^= 0x02;
#else
#error uip-ds6.c cannot build interface address when UIP_LLADDR_LEN is not 6 or 8
#endif
}

/*---------------------------------------------------------------------------*/
uint8_t
get_match_length(uip_ipaddr_t *src, uip_ipaddr_t *dst)
{
  uint8_t j, k, x_or;
  uint8_t len = 0;

  for(j = 0; j < 16; j++) {
    if(src->u8[j] == dst->u8[j]) {
      len += 8;
    } else {
      x_or = src->u8[j] ^ dst->u8[j];
      for(k = 0; k < 8; k++) {
        if((x_or & 0x80) == 0) {
          len++;
          x_or <<= 1;
        } else {
          break;
        }
      }
      break;
    }
  }
  return len;
}

/*---------------------------------------------------------------------------*/
#if UIP_ND6_DEF_MAXDADNS > 0
void
uip_ds6_dad(uip_ds6_addr_t *addr)
{
  /* send maxdadns NS for DAD  */
  if(addr->dadnscount < uip_ds6_if.maxdadns) {
    uip_nd6_ns_output(NULL, NULL, &addr->ipaddr);
    addr->dadnscount++;
    timer_set(&addr->dadtimer,
              uip_ds6_if.retrans_timer / 1000 * CLOCK_SECOND);
    return;
  }
  /*
   * If we arrive here it means DAD succeeded, otherwise the dad process
   * would have been interrupted in ds6_dad_ns/na_input
   */
  PRINTF("DAD succeeded, ipaddr:");
  PRINT6ADDR(&addr->ipaddr);
  PRINTF("\n");

  addr->state = ADDR_PREFERRED;
  return;
}

/*---------------------------------------------------------------------------*/
/*
 * Calling code must handle when this returns 0 (e.g. link local
 * address can not be used).
 */
int
uip_ds6_dad_failed(uip_ds6_addr_t *addr)
{
  if(uip_is_addr_link_local(&addr->ipaddr)) {
    PRINTF("Contiki shutdown, DAD for link local address failed\n");
    return 0;
  }
  uip_ds6_addr_rm(addr);
  return 1;
}
#endif /*UIP_ND6_DEF_MAXDADNS > 0 */

/*---------------------------------------------------------------------------*/
#if UIP_CONF_ROUTER
#if UIP_ND6_SEND_RA
void
uip_ds6_send_ra_sollicited(void)
{
  /* We have a pb here: RA timer max possible value is 1800s,
   * hence we have to use stimers. However, when receiving a RS, we
   * should delay the reply by a random value between 0 and 500ms timers.
   * stimers are in seconds, hence we cannot do this. Therefore we just send
   * the RA (setting the timer to 0 below). We keep the code logic for
   * the days contiki will support appropriate timers */
  rand_time = 0;
  PRINTF("Solicited RA, random time %u\n", rand_time);

  if(stimer_remaining(&uip_ds6_timer_ra) > rand_time) {
    if(stimer_elapsed(&uip_ds6_timer_ra) < UIP_ND6_MIN_DELAY_BETWEEN_RAS) {
      /* Ensure that the RAs are rate limited */
/*      stimer_set(&uip_ds6_timer_ra, rand_time +
                 UIP_ND6_MIN_DELAY_BETWEEN_RAS -
                 stimer_elapsed(&uip_ds6_timer_ra));
  */ } else {
      stimer_set(&uip_ds6_timer_ra, rand_time);
    }
  }
}

#if CONF_6LOWPAN_ND
void
uip_ds6_send_ra_unicast_sollicited(uip_ipaddr_t *dest)
{
  for(locbr = uip_ds6_br_list;
      locbr < uip_ds6_br_list + UIP_DS6_BR_NB;
      locbr++) {
    uip_nd6_ra_output(dest, locbr);
    tcpip_ipv6_output();
  }
}
#endif /* CONF_6LOWPAN_ND */

/*---------------------------------------------------------------------------*/
void
uip_ds6_send_ra_periodic(void)
{
  if(racount > 0) {
    /* send previously scheduled RA */
#if CONF_6LOWPAN_ND
    for(locbr = uip_ds6_br_list;
        locbr < uip_ds6_br_list + UIP_DS6_BR_NB;
        locbr++) {
      uip_nd6_ra_output(NULL, locbr);
    }
#else
    uip_nd6_ra_output(NULL);
#endif /* CONF_6LOWPAN_ND */
    PRINTF("Sending periodic RA\n");
  }

  rand_time = UIP_ND6_MIN_RA_INTERVAL + random_rand() %
    (uint16_t) (UIP_ND6_MAX_RA_INTERVAL - UIP_ND6_MIN_RA_INTERVAL);
  PRINTF("Random time 1 = %u\n", rand_time);

  if(racount < UIP_ND6_MAX_INITIAL_RAS) {
    if(rand_time > UIP_ND6_MAX_INITIAL_RA_INTERVAL) {
      rand_time = UIP_ND6_MAX_INITIAL_RA_INTERVAL;
      PRINTF("Random time 2 = %u\n", rand_time);
    }
    racount++;
  }
  PRINTF("Random time 3 = %u\n", rand_time);
  stimer_set(&uip_ds6_timer_ra, rand_time);
}

#endif /* UIP_ND6_SEND_RA */
#endif /* UIP_CONF_ROUTER */
#if !UIP_CONF_ROUTER || CONF_6LOWPAN_ND
/*---------------------------------------------------------------------------*/
void
uip_ds6_send_rs(void)
{
#if CONF_6LOWPAN_ND
  uint16_t r;
#endif /* CONF_6LOWPAN_ND */

  /* check if RS should be sent now or not */
  if(!uip_nd6_send_rs) {
    return;
  }

  if((uip_ds6_defrt_choose() == NULL)
     && (rscount < UIP_ND6_MAX_RTR_SOLICITATIONS)) {
    PRINTF("Sending RS %u\n", rscount);
    uip_nd6_rs_output();
    rscount++;
    etimer_set(&uip_ds6_timer_rs,
               UIP_ND6_RTR_SOLICITATION_INTERVAL * CLOCK_SECOND);
#if CONF_6LOWPAN_ND
  } else if(uip_ds6_defrt_choose() == NULL) {
    /* Slower retransmissions */
    PRINTF("Sending RS slower %u\n", rscount);
    uip_nd6_rs_output();
    rscount++;
    r = (((uint16_t)random_rand()) % (((2 << (rscount - 1)) - 1) + 1)) * UIP_ND6_RTR_SOLICITATION_INTERVAL;
    if(r >= UIP_ND6_MAX_RTR_SOLICITATION_INTERVAL) {
      r = UIP_ND6_MAX_RTR_SOLICITATION_INTERVAL;
    }
    etimer_set(&uip_ds6_timer_rs, r * CLOCK_SECOND);
#endif /* CONF_6LOWPAN_ND */
  } else {
    PRINTF("Router found ? (boolean): %u\n",
           (uip_ds6_defrt_choose() != NULL));
    etimer_stop(&uip_ds6_timer_rs);
  }
  return;
}
#endif /* !UIP_CONF_ROUTER || CONF_6LOWPAN_ND */
/*---------------------------------------------------------------------------*/
uint32_t
uip_ds6_compute_reachable_time(void)
{
  return (uint32_t) (UIP_ND6_MIN_RANDOM_FACTOR
                     (uip_ds6_if.base_reachable_time)) +
    ((uint16_t) (random_rand() << 8) +
     (uint16_t) random_rand()) %
    (uint32_t) (UIP_ND6_MAX_RANDOM_FACTOR(uip_ds6_if.base_reachable_time) -
                UIP_ND6_MIN_RANDOM_FACTOR(uip_ds6_if.base_reachable_time));
}
/*---------------------------------------------------------------------------*/

/** @}*/
