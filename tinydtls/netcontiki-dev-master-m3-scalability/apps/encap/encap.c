/*
 * Copyright (c) 2015, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    This product includes software developed by Yanzi Networks AB.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include "api.h"
#include "encap.h"
#include "crc.h"

uint32_t get_rx_seq(void);
void set_r_seq(uint32_t seq);
uint32_t get_tx_seq(void);

static const struct encap_crypto *crypto = NULL;

/*
 * set crypto driver to use
 */
void
encap_set_crypto_driver(const struct encap_crypto *crypto_driver)
{
  crypto = crypto_driver;
}

/*
 * Process encapsulation header.
 * write payload type to "payload_type" in non-null.
 * write payload length to "payload_len" in non-null.
 * Return number of bytes consumed by encapsulation, or (negative) error.
 */
int32_t
verify_and_decrypt_message(const uint8_t *p, size_t len, pdu_info *pinfo)
{
  int16_t retval = 4;

  pinfo->payload_len = -1;
  pinfo->dont_reply = 0;

  if(len < 4) {
    return ENC_ERROR_SHORT;
  }

  pinfo->version = (p[0] >> 4);
  if(pinfo->version > ENC_VERSION_MAX) {    /* WANNA REJECT HIGHER VERSIONS THAN 2 */
    return ENC_ERROR_BAD_VERSION;
  }

  if((p[1] == 0x00) || (p[1] == 0x02) || (p[1] > 0x11)) {
    return ENC_ERROR_BAD_PAYLOAD_TYPE;
  }

  if(p[2] != 0) {
    return ENC_ERROR_REQUEST_WITH_ERROR;
  }

  pinfo->fpmode = (p[3] >> 4) & 0x0f;
  if(pinfo->fpmode > ENC_FINGERPRINT_MODE_MAX) {
    return ENC_ERROR_BAD_FINGERPRINT_MODE;
  }

  pinfo->ivmode = p[3] & 0x0f;
  if(pinfo->ivmode > ENC_IVMODE_MAX) {
    return ENC_ERROR_BAD_INITVECTOR_MODE;
  }

  pinfo->fpmatchlen = ENC_FINGERPRINT_MATCHLEN_NONE;
  pinfo->fp = (uint8_t *)&p[4];
  pinfo->key_class = ENC_KEY_CLASS_NONE;
  switch(pinfo->fpmode) {
  case ENC_FINGERPRINT_MODE_NONE:
    pinfo->fplen = 0;
    pinfo->fp = NULL;
    pinfo->encrypt = FALSE;
    break;
  case ENC_FINGERPRINT_MODE_DEVID:
    pinfo->fplen = 8;
    pinfo->encrypt = FALSE;
    break;
  case ENC_FINGERPRINT_MODE_FIP4:
    if(pinfo->version < ENC_VERSION2) {
      return ENC_ERROR_BAD_FINGERPRINT_MODE;   /* FP Mode not supported in this version */
    }
    pinfo->fplen = 4;
    pinfo->fpmatchlen = 3;
    pinfo->encrypt = TRUE;
    pinfo->key_class = pinfo->fp[3];
    break;
  case ENC_FINGERPRINT_MODE_LENOPT:
    pinfo->fplen = 4;
    pinfo->encrypt = FALSE;
    break;
  case ENC_FINGERPRINT_MODE_DID_AND_FP:
    if(pinfo->version < ENC_VERSION2) {
      return ENC_ERROR_BAD_FINGERPRINT_MODE;   /* FP Mode not supported in this version */
    }
    pinfo->fplen = 16;
    pinfo->fpmatchlen = 12;   /* do not match data field of fingerprint. */
    pinfo->encrypt = TRUE;
    pinfo->key_class = pinfo->fp[15];
    break;
  }
  if(pinfo->fpmatchlen == ENC_FINGERPRINT_MATCHLEN_NONE) {
    pinfo->fpmatchlen = pinfo->fplen;
  }
  pinfo->iv = (uint8_t *)&p[4 + pinfo->fplen];
  retval += pinfo->fplen;

  if(len < retval) {
    return ENC_ERROR_SHORT;
  }

  if(pinfo->fpmode == ENC_FINGERPRINT_MODE_LENOPT) {
    pinfo->payload_len = ((uint16_t)p[6] << 8) | p[7];
    if(p[5] == ENC_FINGERPRINT_LENOPT_OPTION_CRC
       || p[5] == ENC_FINGERPRINT_LENOPT_OPTION_SEQNO_CRC) {
      if(len < retval + pinfo->payload_len + 4) {
        return ENC_ERROR_SHORT;
      }
      if(!crc_check_crc32(p, len)) {
        return ENC_ERROR_BAD_CHECKSUM;
      }
    }
  }

  switch(pinfo->ivmode) {
  case ENC_IVMODE_128BIT:
    pinfo->ivlen = 16;
    break;
  case ENC_IVMODE_NONE:
    pinfo->ivlen = 0;
    break;
  }
  retval += pinfo->ivlen;

  if(len < retval) {
    return ENC_ERROR_SHORT;
  }

  if(pinfo->encrypt) {
    /* Encryption not supported */
    return ENC_ERROR_UNKNOWN_KEY;
  }

  pinfo->payload_type = p[1];

  if(pinfo->payload_len < 0) {
    pinfo->payload_len = len - retval;
  }
  return retval;
}
/*----------------------------------------------------------------*/

/*
 * Initialize a pdu_info for use when transmiting event or other
 * unsolicited and unencrypted TLV pdus.
 */
void
init_pdu_info_for_event(pdu_info *pinfo)
{
  uint8_t *did;
  if(pinfo == NULL) {
    return;
  }
  did = getDid();

  pinfo->version = ENC_VERSION1;
  pinfo->fp = &did[8];
  pinfo->iv = NULL;
  pinfo->payload_len = 0;
  pinfo->payload_type = ENC_PAYLOAD_TLV;
  pinfo->fpmode = ENC_FINGERPRINT_MODE_DEVID;
  pinfo->fplen = 8;
  pinfo->ivmode = ENC_IVMODE_NONE;
  pinfo->ivlen = 0;
  pinfo->encrypt = 0;
  pinfo->fpmatchlen = ENC_FINGERPRINT_MATCHLEN_NONE;
  pinfo->key_class = ENC_KEY_CLASS_NONE;
  pinfo->dont_reply = 0;
  if(crypto != NULL && crypto->event_to_encrypt != NULL) {
    crypto->event_to_encrypt(pinfo);
  }
}
/*----------------------------------------------------------------*/

/*
 * Write encapsulation header to "buf", no more than "len" bytes and
 * return number of bytes written.
 *
 * "pinfo" should be filled-in by verify_and_decrypt_message().
 * finger print and init vector pointers within "pinfo" are changed to
 * point to within "buf".
 */
int32_t
write_encap_header(uint8_t *buf, size_t len, pdu_info *pinfo, uint8_t error)
{
  int32_t retval;

  if(pinfo == NULL) {
    return 0;
  }

  retval = 4; /* encap header */
  retval += pinfo->fplen; /* finger print */
  retval += pinfo->ivlen; /* init vector */
  retval += (pinfo->encrypt) ? 4 : 0;  /* sequence number */

  if(len < retval) {
    return 0;
  }

  buf[0] = SHIFT_VERSION(pinfo->version);
  buf[1] = pinfo->payload_type;
  buf[2] = error;

  buf[3] = pinfo->fpmode << 4;
  /* copy finger print */
  if(pinfo->fplen > 0) {
    if(pinfo->fp != NULL) {
      memcpy(&buf[4], pinfo->fp, pinfo->fplen);
    } else {
      /* Generated event, encrypted, don't have fp */
      if(crypto != NULL && crypto->write_fingerprint != NULL) {
        crypto->write_fingerprint(pinfo, &buf[4]);
      }
    }
  }
  /* set finger print pointer in pinfo */
  pinfo->fp = &buf[4];

  buf[3] |= (pinfo->ivmode & 0x0f);
  if(pinfo->ivlen > 0) {
    if(pinfo->iv == NULL) {
      random_fill(&buf[8], pinfo->ivlen);
      pinfo->iv = &buf[4 + pinfo->fplen];
    } else {
      memcpy(&buf[4 + pinfo->fplen], pinfo->iv, pinfo->ivlen);
    }
  }
  pinfo->iv = &buf[4 + pinfo->fplen];

  return retval;
}
/*----------------------------------------------------------------*/

/*
 * Add sequence number, if needed, and encypyt payload.
 *
 * Return length of pdu, or 0 on failure to encrypt a pdu that should
 * be encrypted.
 */
int32_t
finalize_and_encrypt_message(uint8_t *buf, size_t maxlen, pdu_info *pinfo, int32_t pdulen)
{
  if(!pinfo->encrypt) {
    return pdulen;
  }
  printf("finalize: encryption not supported\n");
  return 0;
}
/*----------------------------------------------------------------*/

/*
 * If no encryption instance has installed its get keys function return 0.
 */
uint8_t
is_keys_set()
{
  if(crypto != NULL && crypto->is_keys_set != NULL) {
    return crypto->is_keys_set();
  }
  return 0;
}
/*----------------------------------------------------------------*/
uint8_t
get_authorized_action(uint16_t variable, pdu_info *pinfo)
{
  if(crypto != NULL && crypto->filter != NULL) {
    return crypto->filter(variable, pinfo);
  }
  return 0;
}
/*----------------------------------------------------------------*/
