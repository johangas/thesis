/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *         6LoWPAN-Context DS
 * \author Sebastien Defauw
 *
 */
/*
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
 *
 *
 */

#ifndef UIP_DS6_CONTEXT_H_
#define UIP_DS6_CONTEXT_H_

#include "net/ip/uip.h"
#include "sys/stimer.h"
/* The size of uip_ds6_addr_t depends on UIP_ND6_DEF_MAXDADNS. Include uip-nd6.h to define it. */
#include "net/ipv6/uip-nd6.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-ds6-nbr.h"

/*--------------------------------------------------*/
/** Configuration. For all tables (Neighbor cache, Prefix List, Routing Table,
 * Default Router List, Unicast address list, multicast address list, anycast address list),
 * we define:
 * - the number of elements requested by the user in contiki configuration (name suffixed by _NBU)
 * - the number of elements assigned by the system (name suffixed by _NBS)
 * - the total number of elements is the sum (name suffixed by _NB)
 *
 *
 * Note: this is now intended to be used even if there is not 6lowpan-nd
 * actively adding contexts to compress.
 */

/* Context Prefix list - supports 4 if not defined */
#ifndef UIP_CONF_DS6_CONTEXT_PREF_NB
#define UIP_DS6_CONTEXT_PREF_NB  4
#else
#define UIP_DS6_CONTEXT_PREF_NB UIP_CONF_DS6_CONTEXT_PREF_NB
#endif

/** \brief Possible states for context prefix states */
#define CONTEXT_PREF_ST_FREE 0
#define CONTEXT_PREF_ST_COMPRESS 1
#define CONTEXT_PREF_ST_UNCOMPRESSONLY 2
#define CONTEXT_PREF_ST_SENDING 3
#define CONTEXT_PREF_ST_ADD 4
#define CONTEXT_PREF_ST_RM 5
#define CONTEXT_PREF_USE_COMPRESS(X) (X == CONTEXT_PREF_ST_COMPRESS || X == CONTEXT_PREF_ST_SENDING)
#define CONTEXT_PREF_USE_UNCOMPRESS(X) (X != CONTEXT_PREF_ST_FREE)

/** \brief Possible states for context prefix states */
#define BR_ST_FREE 0
#define BR_ST_USED 1
#define BR_ST_NEW_VERSION 2
#define BR_ST_MUST_SEND_RS 3
#define BR_ST_SENDING_RS 4

/** \brief Possible states for default router */
#define DEFRT_ST_RA_RCV 0
#define DEFRT_ST_SENDING_RS 1

/** \brief A Context prefix list entry */
typedef struct uip_ds6_context_pref {
  uint8_t state;
  uip_ipaddr_t ipaddr;
  uint8_t length;
  uint8_t cid;
  struct stimer lifetime;
  uint16_t vlifetime;
#if !UIP_CONF_6LBR
  uint16_t router_lifetime;
#endif /* !UIP_CONF_6LBR */
  uip_ds6_border_router_t *br;
} uip_ds6_context_pref_t;

extern uip_ds6_context_pref_t uip_ds6_context_pref_list[UIP_DS6_CONTEXT_PREF_NB];

/*---------------------------------------------------------------------------*/
/** \brief Initialize data structures */
void uip_ds6_context_init(void);

/** \name Context prefix list basic routines */
/** @{ */
#if UIP_CONF_6LBR
uip_ds6_context_pref_t *uip_ds6_context_pref_add(uip_ipaddr_t *ipaddr, uint8_t length,
                                                 uint16_t lifetime);
#else /* UIP_CONF_6LBR */
uip_ds6_context_pref_t *uip_ds6_context_pref_add(uip_ipaddr_t *ipaddr, uint8_t length,
                                                 uint8_t c_cid, uint16_t lifetime,
                                                 uint16_t router_lifetime);
#endif /* UIP_CONF_6LBR */
void uip_ds6_context_pref_rm(uip_ds6_context_pref_t *prefix);
void uip_ds6_context_pref_rm_all(uip_ds6_border_router_t *br);
uip_ds6_context_pref_t *uip_ds6_context_pref_lookup(uip_ipaddr_t *ipaddr);
uip_ds6_context_pref_t *uip_ds6_context_pref_lookup_by_cid(uint8_t cid);
void uip_ds6_context_periodic(void);

/** @} */

#endif /* UIP_DS6_CONTEXT_H_ */
