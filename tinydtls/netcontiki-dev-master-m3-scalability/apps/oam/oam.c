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

#include "oam.h"
#include <stdio.h>
#include <string.h>
#include "sys/clock.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "encap.h"
#include "api.h"
#include "tlv.h"
#include "net/ipv6/sicslowpan.h"
#ifdef HAVE_INSTANCE_SLEEP
#include "instance-sleep.h"
#endif /*HAVE_INSTANCE_SLEEP */

#undef DEBUG
#define DEBUG 0

#if DEBUG
#undef PRINTF
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

#define TXBUF_SIZE 1280

#define UDP_HDR ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

static struct uip_udp_conn *oam_sock;
static void process_request(process_event_t ev, process_data_t data);
static void send_event();

static uip_ipaddr_t last_destination;
static unsigned short last_port;
static char have_destination = 0;
static struct etimer oam_rx_timer;
const uip_ipaddr_t uip_all_zeroes_addr = { { 0x0, /* rest is 0 */ } };
static clock_time_t wait_time;
static uint64_t last_oam_rx_from_uc = 0;
static uint64_t last_oam_rx = 0;
static uint64_t last_oam_tx = 0;
static uint8_t send_wakeup_now = FALSE;

PROCESS(oam, "OAM");

PROCESS_THREAD(oam, ev, data) {
  uint64_t now;

  PROCESS_BEGIN();

  oam_sock = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(oam_sock, UIP_HTONS(OAM_PORT));

  while(TRUE) {
    now = boottimer_read();

    /* Default to 3 seconds */
    wait_time = CLOCK_CONF_SECOND * 3;

    /* Then check for pending event */
    if(event_backoff_timer(EVENT_BACKOFF_TIMER_IS_ACTIVE)) {
      uint64_t next_event = event_backoff_timer(EVENT_BACKOFF_TIMER_GET_NEXT_TX_TIME);
      if(next_event < now) {
        wait_time = 1;
      } else {
        wait_time = next_event - now;
      }
    }

    /* now check for wake up event */
    if(send_wakeup_now) {
      wait_time = 1;
    }

    etimer_set(&oam_rx_timer, wait_time);

    PROCESS_WAIT_EVENT();

    if(ev == tcpip_event) {
      if(uip_newdata()) {
        process_request(ev, data);
      } else {
        printf("OAM  : tcpip event w/o new data\n");
      }
      continue;
    }

    /*
     * Here there is a poll or timeout. In both cases, check if it is
     * time to send event.
     */
    if(event_backoff_timer(EVENT_BACKOFF_TIMER_ARE_WE_THERE_YET) || send_wakeup_now) {
      send_event();
      /*
       * If a wakeup notification was schedued, it has been taken care
       * of by the event.
       */
      send_wakeup_now = FALSE;
      continue;
    }
  } /* while(TRUE) */
  PROCESS_END();
}
/*----------------------------------------------------------------*/

static void
process_request(process_event_t ev, process_data_t data)
{
  int rxlen;
  uint8_t *txbuf;
  pdu_info pinfo;
  int encap_header_length;
  uint8_t *buf;
  int l;
  uint8_t *did = getDid();
  size_t txsize;

  rxlen = uip_datalen();
  buf = uip_appdata;

  encap_header_length = verify_and_decrypt_message(buf, rxlen, &pinfo);
  if(encap_header_length < 0) {
    printf("OAM: decode message: error %d\r\n", encap_header_length);
    return;
  }

  last_oam_rx = boottimer_read();
  if(is_uc_address((uint8_t *)&UDP_HDR->srcipaddr, 16)) {
    last_oam_rx_from_uc = last_oam_rx;
  }
  PRINTF("oam: RX %d bytes\n", rxlen);

  txbuf = sicslowpan_alloc_fragbufs(TXBUF_SIZE, &txsize);
  if(txbuf == NULL) {
    /* Woops - no memory to allocate */
    printf("OAM: no tx buffer\n");
    return;
  }

  if(!pinfo.encrypt) {
    pinfo.fpmode = ENC_FINGERPRINT_MODE_DEVID;
    pinfo.fp = &did[8];
    pinfo.fplen = 8;
  }

  if(pinfo.payload_type == ENC_PAYLOAD_TLV) {
    /* process stack */
    l = write_encap_header(txbuf, txsize - 2, &pinfo, ENC_ERROR_OK);
    l += process_TLV_request(buf + encap_header_length, rxlen - encap_header_length,
                             txbuf + l, txsize - l - 2, &pinfo);

    /* Sleepy nodes will try to sleep soon after processing a message */
#ifdef HAVE_INSTANCE_SLEEP
    message_processed();

    /* If sleepy node, always add time sleep */
#ifdef VARIABLE_SLEEP_DEFAULT_AWAKE_TIME
    TLV T;
    memset(&T, 0, sizeof(T));
    T.variable = VARIABLE_SLEEP_DEFAULT_AWAKE_TIME;
    T.opcode = TLV_OPCODE_GET_RESPONSE;
    l += writeReply32intTlv(&T, txbuf + l, txsize - l - 2, get_awake_time());
#endif /* VARIABLE_SLEEP_DEFAULT_AWAKE_TIME */
#endif /* HAVE_INSTANCE_SLEEP */

    /* add a null TLV */
    txbuf[l++] = 0;
    txbuf[l++] = 0;
  } else if(pinfo.payload_type == ENC_PAYLOAD_PORTAL_SELECTION_PROTO) {
    int local_len;
    l = write_encap_header(txbuf, txsize - 2, &pinfo, ENC_ERROR_OK);
    local_len = process_TLV_request(buf + encap_header_length, rxlen - encap_header_length,
                                    txbuf + l, txsize - l - 2, &pinfo);
    if(local_len == 0) {
      l = 0;
    } else {
      l += local_len;
      /* add a null TLV */
      txbuf[l++] = 0;
      txbuf[l++] = 0;
    }
  } else if(pinfo.payload_type == ENC_PAYLOAD_ECHO_REQUEST) {
    pinfo.payload_type = ENC_PAYLOAD_ECHO_REPLY;
    l = write_encap_header(txbuf, txsize - 4, &pinfo, ENC_ERROR_OK);
    memcpy(txbuf + l, buf + encap_header_length, 4);
    l += 4;
  } else if(pinfo.payload_type == ENC_PAYLOAD_ECHO_REPLY) {
    oam_echo_replies(TRUE);
    l = 0; /* silent drop */
  } else {
    l = write_encap_header(txbuf, txsize, &pinfo, ENC_ERROR_BAD_PAYLOAD_TYPE);
    l = 0;
  }

  l = finalize_and_encrypt_message(txbuf, txsize, &pinfo, l);

  if(l > 0) {
    /* Copy the information about the sender to the oam_sock in order to reply */
    uip_ipaddr_copy(&oam_sock->ripaddr, &UDP_HDR->srcipaddr);  /* ip address */
    oam_sock->rport = UDP_HDR->srcport; /* UDP port */

    /* And save them for next event */
    uip_ipaddr_copy(&last_destination, &UDP_HDR->srcipaddr); /* ip address */
    last_port = UDP_HDR->srcport; /* UDP port */
    have_destination = 1;

    /* Send the reply datagram */
    uip_udp_packet_send(oam_sock, txbuf, l);
    last_oam_tx = boottimer_read();

    /* Restore the oam_sock to previous setting in order to receive other packets */
    uip_ipaddr_copy(&oam_sock->ripaddr, &uip_all_zeroes_addr);
    oam_sock->rport = 0;

    reply_sent(1);
    PRINTF("oam: TX %d bytes (reply)\n", l);
  } else {
    reply_sent(0);
  }
  /* free buffers */
  sicslowpan_free_fragbufs(txbuf);
}
/*----------------------------------------------------------------*/

uint8_t
oam_echo_replies(uint8_t increment)
{
  static uint8_t replies = 0;
  if(increment) {
    replies++;
  }
  return replies;
}
/*----------------------------------------------------------------*/

void
oam_send_packet(uip_ipaddr_t destination, uint16_t port, uint8_t *data, int len)
{
  /* Set address */
  uip_ipaddr_copy(&oam_sock->ripaddr, &destination);
  oam_sock->rport = port;

  /* Send datagram */
  uip_udp_packet_send(oam_sock, data, len);
  last_oam_tx = boottimer_read();

  /* Restore the oam_sock to previous setting in order to receive other packets */
  uip_ipaddr_copy(&oam_sock->ripaddr, &uip_all_zeroes_addr);
  oam_sock->rport = 0;
  PRINTF("oam: TX %d bytes (send packet)\n", len);
}
/*----------------------------------------------------------------*/

static void
send_event()
{
  uint8_t *txbuf;
  size_t txlen;
  instanceControl *localIC;
  pdu_info pinfo;
  uint16_t port;
  uint8_t *p;
  uip_ipaddr_t destination_address;
  size_t txsize;

  localIC = get_instances();
  p = unitControllerGetAddress();
  port = (p[7] << 8) | p[6];
  memcpy(&destination_address.u8[0], &p[8], 16);
  if(port == 0) {
    return;
  }
  txbuf = sicslowpan_alloc_fragbufs(TXBUF_SIZE, &txsize);
  if(txbuf == NULL) {
    /* Woops - no memory to allocate */
    return;
  }

  init_pdu_info_for_event(&pinfo);
  txlen = write_encap_header(txbuf, txsize -2, &pinfo, ENC_ERROR_OK);
  txlen += generate_event_reply(localIC, &txbuf[txlen], txsize - txlen -2);
  /* null tlv */
  txbuf[txlen++] = 0;
  txbuf[txlen++] = 0;

  txlen = finalize_and_encrypt_message(txbuf, txsize, &pinfo, txlen);

  if(txlen > 0) {
    oam_send_packet(destination_address, port, txbuf, txlen);
  }

  /* free the buffer */
  sicslowpan_free_fragbufs(txbuf);
}
/*----------------------------------------------------------------*/

void
schedule_wakeup_notification()
{
  if(!send_wakeup_now) {
    send_wakeup_now = TRUE;
    process_poll(&oam);
  }
}
/*----------------------------------------------------------------*/

/*
 * register transmit activity.
 */
void
oam_tx_activity()
{
  last_oam_tx = boottimer_read();
}
/*----------------------------------------------------------------*/

/*
 * Get time stamp of last oam activity.
 */
uint64_t
oam_last_activity(oam_activity_type type)
{
  uint64_t retval = 0;

  switch(type) {

  case OAM_ACTIVITY_TYPE_TX:
    retval = last_oam_tx;
    break;

  case OAM_ACTIVITY_TYPE_RX:
    retval = last_oam_rx;
    break;

  case OAM_ACTIVITY_TYPE_RX_FROM_UC:
    retval = last_oam_rx_from_uc;
    break;

  case OAM_ACTIVITY_TYPE_ANY:
    if(last_oam_tx > last_oam_rx) {
      retval = last_oam_tx;
    } else {
      retval = last_oam_rx;
    }
    break;
  }
  return retval;
}
/****************************************************************/

/*
 * Get milliseconds since last good packet received from unit controller.
 *
 * Return 0xffffffff if unit controller is unset or never receved.
 */
uint32_t
oam_get_time_since_last_good_rx_from_uc()
{
  uint64_t when;
  uint64_t elapsed;

  when = oam_last_activity(OAM_ACTIVITY_TYPE_RX_FROM_UC);
  if(when == 0) {
    /* never received */
    return 0xffffffffL;
  }

  if(unitControllerGetWatchdog(NULL) == 0) {
    /* No unit controlelr */
    return 0xffffffffL;
  }

  elapsed = boottimer_elapsed(when);
  if(elapsed > 0xffffffffLL) {
    return 0xffffffffL;
  }

  return (uint32_t)elapsed;
}
/****************************************************************/
