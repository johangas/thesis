/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file   Prefix management of 6LoWPAN-ND
 *
 * \author Sebastien Defauw, Laurent Deru
 */
/*
 * Copyright (c) 2015, Cetic
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
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "lib/random.h"
#include "net/ipv6/uip-nd6.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-context.h"
#include "net/ip/uip-packetqueue.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

uip_ds6_context_pref_t uip_ds6_context_pref_list[UIP_DS6_CONTEXT_PREF_NB];  /** \brief Prefix list */



/*---------------------------------------------------------------------------*/
void
uip_ds6_context_init(void)
{
  memset(uip_ds6_context_pref_list, 0, sizeof(uip_ds6_context_pref_list));
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_context_periodic(void)
{
  uip_ds6_context_pref_t *loccontext;
  /* Periodic processing on context prefixes */
  for(loccontext = uip_ds6_context_pref_list;
      loccontext < uip_ds6_context_pref_list + UIP_DS6_CONTEXT_PREF_NB;
      loccontext++) {
    if(loccontext->state != CONTEXT_PREF_ST_FREE) {
#if UIP_CONF_6LBR
      if(stimer_expired(&loccontext->lifetime) &&
         loccontext->br->state != BR_ST_NEW_VERSION) {
        switch(loccontext->state) {
        case CONTEXT_PREF_ST_RM:
          /* Valid lifetime expired, so remove */
          loccontext->state = CONTEXT_PREF_ST_FREE;
          break;
        case CONTEXT_PREF_ST_ADD:
          /* before c=0, now c=1 */
          loccontext->state = CONTEXT_PREF_ST_COMPRESS;
          stimer_set(&loccontext->lifetime, loccontext->vlifetime * 60);
          break;
        }
      }
#else /* UIP_CONF_6LBR */
      if(stimer_expired(&loccontext->lifetime)) {
        switch(loccontext->state) {
        case CONTEXT_PREF_ST_UNCOMPRESSONLY:
        case CONTEXT_PREF_ST_RM:
          /* Valid lifetime expired, so remove */
          loccontext->state = CONTEXT_PREF_ST_FREE;
          break;
        case CONTEXT_PREF_ST_SENDING:
          /* receive-only mode for a period of twice the default Router Lifetime */
          loccontext->state = CONTEXT_PREF_ST_UNCOMPRESSONLY;
          stimer_set(&loccontext->lifetime, loccontext->router_lifetime * 2);
          break;
        case CONTEXT_PREF_ST_ADD:
          /* before c=0, now c=1 */
          loccontext->state = CONTEXT_PREF_ST_COMPRESS;
          stimer_set(&loccontext->lifetime, loccontext->vlifetime * 60);
          break;
        }
      } else if(is_timeout_percent(&loccontext->lifetime, UIP_DS6_RS_PERCENT_LIFETIME_RETRAN,
                                   UIP_DS6_RS_MINLIFETIME_RETRAN)
                && loccontext->state == CONTEXT_PREF_ST_COMPRESS) {
        if(loccontext->br->state != BR_ST_SENDING_RS) {
          loccontext->br->state = BR_ST_MUST_SEND_RS;
        }
        loccontext->state = CONTEXT_PREF_ST_SENDING;
      }
#endif /* UIP_CONF_6LBR */
    }
  }
}

/*---------------------------------------------------------------------------*/
#if UIP_CONF_6LBR
uip_ds6_context_pref_t *
uip_ds6_context_pref_add(uip_ipaddr_t *ipaddr, uint8_t length, uint16_t lifetime)
{
  uip_ds6_border_router_t *locbr;
  uip_ds6_context_pref_t *loccontext;
  if(lifetime == 0) {
    return NULL;
  }
  /* search a free space */
  uint8_t cid;
  cid = -1;
  do {
    cid++;
    loccontext = &uip_ds6_context_pref_list[cid];
  } while(cid < UIP_DS6_CONTEXT_PREF_NB && loccontext->state != CONTEXT_PREF_ST_FREE);
  if(cid == UIP_DS6_CONTEXT_PREF_NB) {
    /* Table full */
    return NULL;
  }
  /* install a new context */
  loccontext = &uip_ds6_context_pref_list[cid];
  if(loccontext->state != CONTEXT_PREF_ST_FREE) {
    PRINTF("Overwriting because Context Prefix is already in the list\n");
  }
  loccontext->state = CONTEXT_PREF_ST_ADD;
  uip_ipaddr_copy(&loccontext->ipaddr, ipaddr);
  loccontext->length = length;
  loccontext->cid = cid;
  loccontext->vlifetime = lifetime;
  stimer_set(&loccontext->lifetime, UIP_ND6_MIN_CONTEXT_CHANGE_DELAY);
  /* Increase version in border router */
  locbr = uip_ds6_br_lookup(NULL);
  if(locbr != NULL) {
    locbr->state = BR_ST_NEW_VERSION;
    loccontext->br = locbr;
  }
  PRINTF("Adding context prefix ");
  PRINT6ADDR(&loccontext->ipaddr);
  PRINTF(" length %u, cid %x, lifetime %dmin\n",
         length, cid, lifetime);
  return loccontext;
}
#else /* UIP_CONF_6LBR */
/*---------------------------------------------------------------------------*/
uip_ds6_context_pref_t *
uip_ds6_context_pref_add(uip_ipaddr_t *ipaddr, uint8_t length,
                         uint8_t c_cid, uint16_t lifetime,
                         uint16_t router_lifetime)
{
  uip_ds6_context_pref_t *loccontext;
  loccontext = &uip_ds6_context_pref_list[(c_cid & UIP_ND6_6CO_FLAG_CID)];
  if(loccontext->state == CONTEXT_PREF_ST_FREE) {
    loccontext->state = (c_cid & UIP_ND6_6CO_FLAG_C) ?
      CONTEXT_PREF_ST_COMPRESS : CONTEXT_PREF_ST_ADD;
    uip_ipaddr_copy(&loccontext->ipaddr, ipaddr);
    loccontext->length = length;
    loccontext->cid = c_cid & UIP_ND6_6CO_FLAG_CID;
    loccontext->router_lifetime = router_lifetime;
    loccontext->vlifetime = lifetime;
    if(loccontext->state == CONTEXT_PREF_ST_ADD) {
      stimer_set(&loccontext->lifetime, UIP_ND6_MIN_CONTEXT_CHANGE_DELAY);
    } else {
      stimer_set(&loccontext->lifetime, lifetime * 60);
    }
    PRINTF("Adding context prefix ");
    PRINT6ADDR(&loccontext->ipaddr);
    PRINTF(" length %u, c %x, cid %x, lifetime %dmin\n",
           length, c_cid & 0x10, c_cid & 0x0f, lifetime);
    return loccontext;
  } else {
    PRINTF("No more space in Context Prefix list\n");
  }
  return NULL;
}
#endif /* UIP_CONF_6LBR */

/*---------------------------------------------------------------------------*/
void
uip_ds6_context_pref_rm(uip_ds6_context_pref_t *prefix)
{
  if(prefix != NULL && prefix->state != CONTEXT_PREF_ST_RM) {
    prefix->state = CONTEXT_PREF_ST_RM;
    stimer_set(&prefix->lifetime, UIP_ND6_MIN_CONTEXT_CHANGE_DELAY);
  }
  return;
}
/*---------------------------------------------------------------------------*/
void
uip_ds6_context_pref_rm_all(uip_ds6_border_router_t *br)
{
  uip_ds6_context_pref_t *loccontext;
  if(br == NULL) {
    return;
  }
  for(loccontext = uip_ds6_context_pref_list;
      loccontext < uip_ds6_context_pref_list + UIP_DS6_CONTEXT_PREF_NB;
      loccontext++) {
    if(loccontext->br == br) {
      uip_ds6_context_pref_rm(loccontext);
    }
  }
}
/*---------------------------------------------------------------------------*/
uip_ds6_context_pref_t *
uip_ds6_context_pref_lookup(uip_ipaddr_t *ipaddr)
{
  uip_ds6_context_pref_t *loccontext;
  for(loccontext = uip_ds6_context_pref_list;
      loccontext < uip_ds6_context_pref_list + UIP_DS6_CONTEXT_PREF_NB;
      loccontext++) {
    if(loccontext->state != CONTEXT_PREF_ST_FREE &&
       uip_ipaddr_prefixcmp(ipaddr, &loccontext->ipaddr, loccontext->length)) {
      return loccontext;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
uip_ds6_context_pref_t *
uip_ds6_context_pref_lookup_by_cid(uint8_t cid)
{
  uip_ds6_context_pref_t *loccontext;
  if(cid >= UIP_DS6_CONTEXT_PREF_NB) {
    return NULL;
  }
  loccontext = &uip_ds6_context_pref_list[cid];
  return loccontext->state == CONTEXT_PREF_ST_FREE ? NULL : loccontext;
}

/** @}*/
