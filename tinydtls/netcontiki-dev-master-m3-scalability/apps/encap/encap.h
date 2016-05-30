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

#ifndef ENCAP_H_
#define ENCAP_H_

#include "string.h"
#include <stdint.h>

#define ENC_VERSION1 1
#define ENC_VERSION2 2
#define ENC_VERSION_MAX 2

#define SHIFT_VERSION(x) (x << 4)

#define ENC_PAYLOAD_TLV                    1
#define ENC_PAYLOAD_PORTAL_SELECTION_PROTO 3
#define ENC_PAYLOAD_ECHO_REQUEST           4
#define ENC_PAYLOAD_ECHO_REPLY             5
#define ENC_PAYLOAD_STOP_TRANSMITTER       6
#define ENC_PAYLOAD_START_TRANSMITTER      7
#define ENC_PAYLOAD_SERIAL                 8
#define ENC_PAYLOAD_DEBUG                  9
#define ENC_PAYLOAD_SERIAL_WITH_SEQNO     10
#define ENC_PAYLOAD_RECEIVE_REPORT        11

#define ENC_ERROR_OK                       0
#define ENC_ERROR_SHORT                   -1
#define ENC_ERROR_BAD_VERSION             -2
#define ENC_ERROR_BAD_PAYLOAD_TYPE        -3
#define ENC_ERROR_REQUEST_WITH_ERROR      -4
#define ENC_ERROR_BAD_FINGERPRINT_MODE    -5
#define ENC_ERROR_BAD_INITVECTOR_MODE     -6
#define ENC_ERROR_UNKNOWN_KEY             -7
#define ENC_ERROR_BAD_SEQUENCE            -8
#define ENC_ERROR_BAD_CHECKSUM            -9

typedef enum {
  ENC_FINGERPRINT_MODE_NONE       = 0, /* 0 bytes */
  ENC_FINGERPRINT_MODE_DEVID      = 1, /* 8 bytes */
  ENC_FINGERPRINT_MODE_FIP4       = 2, /* 4 bytes */
  ENC_FINGERPRINT_MODE_LENOPT     = 3, /* 4 bytes */
  ENC_FINGERPRINT_MODE_DID_AND_FP = 4, /* 16 bytes */
  ENC_FINGERPRINT_MODE_MAX        = 4,
} fingerprintmode;

#define ENC_FINGERPRINT_LENOPT_OPTION_CRC       1
#define ENC_FINGERPRINT_LENOPT_OPTION_SEQNO_CRC 2

typedef enum {
  ENC_IVMODE_NONE   = 0,
  ENC_IVMODE_128BIT = 1,
  ENC_IVMODE_MAX    = 1,
} initvectormode;

typedef struct {
  uint32_t *rx_seq;
  uint32_t *tx_seq;
  uint8_t *key;
  uint8_t *iv;
  uint8_t keylen;
  uint8_t fpmatchlen;
  uint8_t key_class;
} key_meta_info;

#define ENC_KEY_CLASS_NONE    0xff
#define ENC_KEY_CLASS_SESSION 0
#define ENC_KEY_CLASS_PEER    1
#define ENC_KEY_CLASS_OWNER   2
#define ENC_KEY_CLASS_VENDOR  3

#define ENC_FINGERPRINT_MATCHLEN_NONE 0xff

typedef struct {
  uint8_t version;
  uint8_t *fp;
  uint8_t *iv;
  int payload_len;
  uint8_t payload_type;
  fingerprintmode fpmode;
  uint8_t fplen;
  initvectormode ivmode;
  uint8_t ivlen;
  uint8_t encrypt;
  uint8_t fpmatchlen;
  uint8_t key_class;
  uint8_t dont_reply;
} pdu_info;

/**
 * The structure of an encap crypto driver in Contiki.
 */
struct encap_crypto {
  int32_t (*decrypt)(uint8_t *data, size_t len, uint8_t *iv, uint8_t *key);
  int32_t (*encrypt)(uint8_t *data, size_t len, uint8_t *iv, uint8_t *key);
  key_meta_info *(*get_key_meta_info)(uint8_t *fingerprint, uint8_t len, uint8_t key_class);
  uint8_t (*is_keys_set)(void);
  void (*event_to_encrypt)(pdu_info *pinfo);
  uint8_t (*filter)(uint16_t variable, pdu_info *pinfo);
  void (*write_fingerprint)(pdu_info *pinfo, uint8_t *fp);
};

/* some defines that previously was in vargen.h */
#ifndef FALSE
#define FALSE	0
#endif /* FALSE */
#ifndef TRUE
#define TRUE	!FALSE
#endif /* TRUE */

uint8_t *getDid(void);

void encap_set_crypto_driver(const struct encap_crypto *crypto_driver);
int32_t verify_and_decrypt_message(const uint8_t *p, size_t len, pdu_info *info);
int32_t write_encap_header(uint8_t *buf, size_t len, pdu_info *info, uint8_t error);
int32_t finalize_and_encrypt_message(uint8_t *buf, size_t maxlen, pdu_info *pinfo, int32_t pdulen);
uint8_t is_keys_set();
uint8_t get_authorized_action(uint16_t variable, pdu_info *pinfo);

void init_pdu_info_for_event(pdu_info *pinfo);

#endif /* ENCAP_H_ */
