/*
 * Copyright (c) 2014, Hasso-Plattner-Institut.
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
 *         802.15.4 security implementation, which uses a network-wide key
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

/**
 * \addtogroup noncoresec
 * @{
 */

#include "net/llsec/noncoresec/noncoresec.h"
#include "net/llsec/anti-replay.h"
#include "net/llsec/llsec802154.h"
#include "net/llsec/ccm-star-packetbuf.h"
#include "net/mac/frame802154.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/nbr-table.h"
#include "net/linkaddr.h"
#include "lib/ccm-star.h"
#include <string.h>

#ifdef NONCORESEC_CONF_KEY
#define NONCORESEC_KEY NONCORESEC_CONF_KEY
#else /* NONCORESEC_CONF_KEY */
#define NONCORESEC_KEY { 0x00, 0x01, 0x02, 0x03, \
                         0x04, 0x05, 0x06, 0x07, \
                         0x08, 0x09, 0x0A, 0x0B, \
                         0x0C, 0x0D, 0x0E, 0x0F }
#endif /* NONCORESEC_CONF_KEY */

#define SECURITY_HEADER_LENGTH 5

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#include <inttypes.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

/* network-wide CCM* key */
static uint8_t key[16] = NONCORESEC_KEY;
/* runtime LLSEC802154_SECURITY_LEVEL */
static uint8_t security_level = LLSEC802154_SECURITY_LEVEL;
/* runtime LLSEC802154_SECURITY_LEVEL */
static uint8_t mic_length = LLSEC802154_MIC_LENGTH;
NBR_TABLE(struct anti_replay_info, anti_replay_table);

/*---------------------------------------------------------------------------*/
static const uint8_t *
get_extended_address(const linkaddr_t *addr)
#if LINKADDR_SIZE == 2
{
  /* workaround for short addresses: derive EUI64 as in RFC 6282 */
  static linkaddr_extended_t template = { { 0x00, 0x00, 0x00,
                                            0xFF, 0xFE, 0x00, 0x00, 0x00 } };

  template.u16[3] = LLSEC802154_HTONS(addr->u16);

  return template.u8;
}
#else /* LINKADDR_SIZE == 2 */
{
  return addr->u8;
}
#endif /* LINKADDR_SIZE == 2 */
/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent, void *ptr)
{
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
  packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL, security_level);
  anti_replay_set_counter();
  NETSTACK_MAC.send(sent, ptr);
}
/*---------------------------------------------------------------------------*/
static int
on_frame_created(void)
{
  uint8_t *dataptr = packetbuf_dataptr();
  uint8_t data_len = packetbuf_datalen();

  ccm_star_mic_packetbuf(get_extended_address(&linkaddr_node_addr), dataptr + data_len, mic_length);
  if(security_level & (1 << 2)) {
    ccm_star_ctr_packetbuf(get_extended_address(&linkaddr_node_addr));
  }
  packetbuf_set_datalen(data_len + mic_length);

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
input(void)
{
  uint8_t generated_mic[mic_length];
  uint8_t *received_mic;
  const linkaddr_t *sender;
  struct anti_replay_info* info;
  uint8_t *dataptr = packetbuf_dataptr();
  uint8_t data_len = packetbuf_datalen();
  
  if(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) != security_level) {
    PRINTF("noncoresec: received frame with wrong security level\n");
    return;
  }
  sender = packetbuf_addr(PACKETBUF_ADDR_SENDER);
  if(linkaddr_cmp(sender, &linkaddr_node_addr)) {
    PRINTF("noncoresec: frame from ourselves\n");
    return;
  }
  
  data_len -= mic_length;
  packetbuf_set_datalen(data_len);
  
  if(security_level & (1 << 2)) {
    ccm_star_ctr_packetbuf(get_extended_address(sender));
  }
  ccm_star_mic_packetbuf(get_extended_address(sender), generated_mic, mic_length);
  
  received_mic = dataptr + data_len;
  if(memcmp(generated_mic, received_mic, mic_length) != 0) {
    PRINTF("noncoresec: received nonauthentic frame %"PRIu 32 "\n",
           anti_replay_get_counter());
    return;
  }

  if(security_level) {
    info = nbr_table_get_from_lladdr(anti_replay_table, sender);
    if(!info) {
      info = nbr_table_add_lladdr(anti_replay_table, sender);
      if(!info) {
        PRINTF("noncoresec: could not get nbr_table_item\n");
        return;
      }

      /*
       * Locking avoids replay attacks due to removed neighbor table items.
       * Unfortunately, an attacker can mount a memory-based DoS attack
       * on this by replaying broadcast frames from other network parts.
       * However, this is not an issue as long as the network size does not
       * exceed NBR_TABLE_MAX_NEIGHBORS.
       *
       * To avoid locking, we could swap anti-replay information
       * to external flash. Locking is also unnecessary when using
       * pairwise session keys, as done in coresec.
       */
      if(!nbr_table_lock(anti_replay_table, info)) {
        nbr_table_remove(anti_replay_table, info);
        PRINTF("noncoresec: could not lock\n");
        return;
      }

      anti_replay_init_info(info);
    } else if(anti_replay_was_replayed(info)) {
      PRINTF("noncoresec: received replayed frame %"PRIu 32 "\n",
             anti_replay_get_counter());
      return;
    }
  }

  NETSTACK_NETWORK.input();
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_overhead(void)
{
  return SECURITY_HEADER_LENGTH + mic_length;
}
/*---------------------------------------------------------------------------*/
static void
bootstrap(llsec_on_bootstrapped_t on_bootstrapped)
{
  CCM_STAR.set_key(key);
  nbr_table_register(anti_replay_table, NULL);
  if(security_level != 0) {
    anti_replay_set_anti_replay_check(1);
  }
  on_bootstrapped();
}
/*---------------------------------------------------------------------------*/
static int
key_is_set()
{
  int i;

  for(i = 0; i < 16; i++) {
    if(key[i] != 0) {
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
clear_replay_table()
{
  nbr_table_item_t *next;
  while((next = nbr_table_head(anti_replay_table)) != NULL) {
    nbr_table_remove(anti_replay_table, next);
  }
}
/*---------------------------------------------------------------------------*/
int
noncoresec_set_key(uint8_t *new_key, size_t length)
{
  int i;
  uint8_t valid_key = 0;

  if(new_key == NULL || length != 16) {
    return 0;
  }
  for(i = 0; i < 16; i++) {
    if(new_key[i] != 0) {
      valid_key = 1;
      break;
    }
  }
  if(valid_key) {
    memcpy(key, new_key, 16);
    CCM_STAR.set_key(key);
    clear_replay_table();
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
noncoresec_set_security_level(uint8_t new_security_level)
{
  if(new_security_level > LLSEC802154_SECURITY_LEVEL_MAX) {
    return 0;
  }
  if(new_security_level == security_level) {
    return 1;
  }
  if(new_security_level > 0 && key_is_set() == 0) {
    PRINTF("Key is not set, cannot set security level > 0\n");
    return 0;
  }
  security_level = new_security_level;

  if(security_level & 3) {
    mic_length = (2 << (security_level & 3));
  } else {
    mic_length = 0;
  }
  PRINTF("Noncoresec new security level %u new mic length %u\n", security_level, mic_length);
  clear_replay_table();

  /* Set the flag indicating that llsec is checking for replayed packets. */
  if(security_level == 0) {
    anti_replay_set_anti_replay_check(0);
  } else {
    anti_replay_set_anti_replay_check(1);
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
const struct llsec_driver noncoresec_driver = {
  "noncoresec",
  bootstrap,
  send,
  on_frame_created,
  input,
  get_overhead
};
/*---------------------------------------------------------------------------*/

/** @} */