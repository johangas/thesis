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

#include "contiki.h"
#include "net/netstack.h"
#include "net/ip/uip.h"
#include "net/packetbuf.h"
#include "dev/slip.h"
#include "serial-radio.h"
#include "serial-radio-stats.h"
#include "encap.h"
#include "crc.h"
#include <stdio.h>

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

#define DEBUG 0

void slip_input_callback(pdu_info *pinfo, uint8_t *data, int enclen);
void slip_input_net_callback(void);
void slip_send_packet_payload_type(const uint8_t *ptr, int len, uint8_t payload_type);

extern uint8_t verbose_output;

static struct timer send_timer;
static int debug_frame = 0;

static uint16_t slip_fragment_delay = 1000;
static uint16_t slip_fragment_size = 62;
/*---------------------------------------------------------------------------*/
#ifdef CONTIKI_TARGET_ANANAS
#include "dev/uart1.h"

#define WRITE_STATUS_OK UART1_TX_OK
#define WRITEB(b) uart1_writeb(b)

void
slip_arch_init(unsigned long ubr)
{
  uart1_set_input(slip_input_byte);
}

void
slip_arch_writeb(unsigned char c)
{
  slip_writeb(c);
}

#else /* CONTIKI_TARGET_ANANAS */

#define WRITE_STATUS_OK 1

#define WRITEB(b) writeb(b)
static CC_INLINE int
writeb(unsigned char b)
{
  slip_arch_writeb(b);
  return WRITE_STATUS_OK;
}
#endif /* CONTIKI_TARGET_ANANAS */
/*---------------------------------------------------------------------------*/
#if 0
static void
send_data(void *ptr)
{
  static unsigned int sent = 0;
  uint8_t c;

  if(!ctimer_expired(&send_timer) && slip_fragment_delay > 0) {
    return;
  }

  while(send_begin != send_end) {
    c = send_buffer[send_begin];
    send_begin = (send_begin + 1) % sizeof(send_buffer);
    send_count--;
    sent++;

    WRITEB(c);

    if((sent >= slip_fragment_size || c == SLIP_END)
       && slip_fragment_size > 0
       && slip_fragment_delay > 0) {
      ctimer_restart(&send_timer);
      sent = 0;
      break;
    }
  }
}
#endif
/*---------------------------------------------------------------------------*/
int
slip_writeb(unsigned char c)
{
  static int send_count = 0;
  int status;

  if(slip_fragment_delay == 0 || timer_expired(&send_timer)) {
    send_count = 0;
  }
  /* send_data(NULL); */
  send_count++;
  status = WRITEB(c);
  if(send_count >= slip_fragment_size && slip_fragment_size > 0) {
    send_count = 0;
    clock_delay_usec(slip_fragment_delay);
  }
  if(slip_fragment_delay > 0) {
    timer_set(&send_timer, slip_fragment_delay / 1000 + 1);
  }
  return status;
}
/*--------------------------------------------------------------------------*/
void
slip_set_fragment_size(uint16_t size)
{
  slip_fragment_size = size;
}
/*--------------------------------------------------------------------------*/
void
slip_set_fragment_delay(uint16_t delay)
{
  slip_fragment_delay = delay;
}
/*--------------------------------------------------------------------------*/
static int
slip_frame_start(void)
{
  if(debug_frame) {
    debug_frame = 0;
    return slip_writeb(SLIP_END);
  }
  return WRITE_STATUS_OK;
}
static int
slip_frame_end(void)
{
  int status;
  status = slip_writeb(SLIP_END);
  debug_frame = 0;
  return status;
}
/*---------------------------------------------------------------------------*/
#if !SERIAL_RADIO_CONF_NO_PUTCHAR
#undef putchar
#error hej
int
putchar(int c)
{
  if(!debug_frame) {            /* Start of debug output */
    if(slip_writeb(SLIP_END) != WRITE_STATUS_OK) {
      debug_frame = 2;
    } else {
      slip_writeb('\r');     /* Type debug line == '\r' */
      debug_frame = 1;
    }
  }

  if(debug_frame == 1) {
    /* Need to also print '\n' because for example COOJA will not show
       any output before line end */
    slip_writeb((char)c);
  }

  /*
   * Line buffered output, a newline marks the end of debug output and
   * implicitly flushes debug output.
   */
  if(c == '\n') {
    if(debug_frame == 1) {
      slip_writeb(SLIP_END);
    }
    debug_frame = 0;
  }
  return c;
}
#endif /* SERIAL_RADIO_CONF_NO_PUTCHAR */
/*---------------------------------------------------------------------------*/
uint32_t
serial_get_mode(void)
{
  uint32_t v = 0;
  return v;
}
/*---------------------------------------------------------------------------*/
void
serial_set_mode(uint32_t mode)
{
}
/*---------------------------------------------------------------------------*/
void
slipnet_init(void)
{
#ifndef BAUD2UBR
#define BAUD2UBR(baud) baud
#endif
  slip_arch_init(BAUD2UBR(115200));
  process_start(&slip_process, NULL);
  slip_set_input_callback(slip_input_net_callback);
}
/*---------------------------------------------------------------------------*/
static int
send_to_slip(const uint8_t *buf, int len)
{
  int i, s;
  uint8_t c;
  for(i = 0; i < len; ++i) {
    c = buf[i];
    if(c == SLIP_END) {
      s = slip_writeb(SLIP_ESC);
      if(s != WRITE_STATUS_OK) {
        return s;
      }
      c = SLIP_ESC_END;
    } else if(c == SLIP_ESC) {
      s = slip_writeb(SLIP_ESC);
      if(s != WRITE_STATUS_OK) {
        return s;
      }
      c = SLIP_ESC_ESC;
    }
    s = slip_writeb(c);
    if(s != WRITE_STATUS_OK) {
      return s;
    }
  }
  return WRITE_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
void
slip_input_net_callback(void)
{
  int payload_len, enclen;
  pdu_info pinfo;
  uint8_t *payload;

  payload = uip_buf;
  payload_len = uip_len;
  uip_len = 0;

  SERIAL_RADIO_STATS_ADD(SERIAL_RADIO_STATS_ENCAP_RECV, payload_len);

  /* decode packet data - encap header + crc32 check */
  enclen = verify_and_decrypt_message(payload, payload_len, &pinfo);

  if(enclen == ENC_ERROR_BAD_CHECKSUM) {
    if(verbose_output) {
      printf("Packet input failed Bad CRC, len:%d, enclen:%d\n", payload_len, enclen);
    }
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_ERRORS);
    return;
  }

  if(enclen < 0) {
    if(verbose_output) {
      printf("Packet input failed: %d (len:%d)\n", enclen, payload_len);
    }
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_ERRORS);
    return;
  }

  if(pinfo.fpmode != ENC_FINGERPRINT_MODE_LENOPT) {
    if(verbose_output) {
      printf("Packet input failed: illegal fpmode %d (len:%d)\n", pinfo.fpmode, payload_len);
    }
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_ERRORS);
    return;
  }

  if(pinfo.fplen != 4 || pinfo.fp == NULL ||
     (pinfo.fp[1] != ENC_FINGERPRINT_LENOPT_OPTION_CRC
      && pinfo.fp[1] != ENC_FINGERPRINT_LENOPT_OPTION_SEQNO_CRC)) {
    if(verbose_output) {
      printf("Packet input failed: no CRC in fingerprint (len:%d)\n", payload_len);
    }
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_ERRORS);
    return;
  }

  if(pinfo.fpmode == ENC_FINGERPRINT_MODE_LENOPT
     && pinfo.fplen == 4 && pinfo.fp
     && pinfo.fp[1] == ENC_FINGERPRINT_LENOPT_OPTION_SEQNO_CRC) {
    /* Packet includes sequence number */
    /* uint32_t seqno; */
    /* seqno = payload[enclen + 0] << 24; */
    /* seqno |= payload[enclen + 1] << 16; */
    /* seqno |= payload[enclen + 2] << 8; */
    /* seqno |= payload[enclen + 3]; */
    enclen += 4;
  }

  /* Pass the packet over to the serial radio */
  slip_input_callback(&pinfo, payload, enclen);
}
/*---------------------------------------------------------------------------*/
void
slip_send_packet(const uint8_t *ptr, int len)
{
  slip_send_packet_payload_type(ptr, len, ENC_PAYLOAD_SERIAL);
}

void
slip_send_packet_payload_type(const uint8_t *ptr, int len, uint8_t payload_type)
{
  uint8_t header[64];
  uint8_t finger[4];
  uint32_t crc_value;
  int enc_res;
  pdu_info pinfo;

  finger[0] = 0;
  finger[1] = ENC_FINGERPRINT_LENOPT_OPTION_CRC;
  finger[2] = (len >> 8);
  finger[3] = len & 0xff;

  pinfo.version = ENC_VERSION1;
  pinfo.fp = finger;
  pinfo.iv = NULL;
  pinfo.payload_len = len;
  pinfo.payload_type = payload_type;
  pinfo.fpmode = ENC_FINGERPRINT_MODE_LENOPT;
  pinfo.fplen = 4;
  pinfo.ivmode = ENC_IVMODE_NONE;
  pinfo.ivlen = 0;
  pinfo.encrypt = FALSE;

  enc_res = write_encap_header(header, sizeof (header), &pinfo, ENC_ERROR_OK);

  if(! enc_res) {
    printf("*** failed to send encap\n");
    return;
  }

  /* copy the data into the buffer */
  /* memcpy(buffer + enc_res, ptr, len); */

  /* do a crc32 calculation of the whole message */
  crc_value = crc_segmented_start();
  crc_value = crc_segmented_add_bytes(crc_value, header, enc_res);
  crc_value = crc_segmented_add_bytes(crc_value, ptr, len);
  crc_value = crc_segmented_finalize(crc_value);
  /* crc_value = crcFast(buffer, len + enc_res); */
  /* printf("CRC: %lx  len:%d, enc_res:%d\n", crc_value, len, enc_res); */

  /* Store the calculated CRC */
  finger[0] = (crc_value >> 0L) & 0xff;
  finger[1] = (crc_value >> 8L) & 0xff;
  finger[2] = (crc_value >> 16L) & 0xff;
  finger[3] = (crc_value >> 24L) & 0xff;

  /* printf("CRC?:%d\n", checkCRC32(buffer, len + enc_res + 4)); */

  /* out with the whole message */
  if(slip_frame_start() == WRITE_STATUS_OK) {
    if(send_to_slip(header, enc_res) == WRITE_STATUS_OK &&
       send_to_slip(ptr, len) == WRITE_STATUS_OK &&
       send_to_slip(finger, 4) == WRITE_STATUS_OK) {
      /* all fragments successfully sent */
    }

    slip_frame_end();
  }
}
/*---------------------------------------------------------------------------*/
void
slipnet_input(void)
{
  int i;
  /* radio should be configured for filtering so this should be simple */
  /* this should be sent over SLIP! */
  /* so just copy into uip-but and send!!! */
  /* Format: !R<data> ? */
  uip_len = packetbuf_datalen();
  i = packetbuf_copyto(uip_buf);

  if(DEBUG) {
    printf("Slipnet got input of len: %d, copied: %d\n",
           packetbuf_datalen(), i);

    for(i = 0; i < uip_len; i++) {
      printf("%02x", (unsigned char)uip_buf[i]);
      if((i & 15) == 15) {
        printf("\n");
      } else if((i & 7) == 7) {
        printf(" ");
      }
    }
    printf("\n");
  }

  /* printf("SUT: %u\n", uip_len); */
  slip_send_packet(uip_buf, uip_len);
}
/*---------------------------------------------------------------------------*/
const struct network_driver slipnet_driver = {
  "slipnet",
  slipnet_init,
  slipnet_input
};
/*---------------------------------------------------------------------------*/
