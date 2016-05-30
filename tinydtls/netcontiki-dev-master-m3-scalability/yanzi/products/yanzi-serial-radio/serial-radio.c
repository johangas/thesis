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
 *         Serial-radio driver
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */
#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/slip.h"
#include "dev/watchdog.h"
#include <string.h>
#include "net/netstack.h"
#include "net/packetbuf.h"

#include "cmd.h"
#include "serial-radio.h"
#include "sniffer-rdc.h"
#include "radio-scan.h"
#include "packetutils.h"
#include "radio-frontpanel.h"
#include "serial-radio-stats.h"
#ifdef CONTIKI_TARGET_ANANAS
#include "dev/uart1.h"
#endif /* CONTIKI_TARGET_ANANAS */

#include "crc.h"
#include "encap.h"
#include "oam.h"

#ifndef SERIAL_RADIO_CONTROL_API_VERSION
#error "No SERIAL_RADIO_CONTROL_API_VERSION specified. Please set in project-conf.h!"
#endif

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define BOOT_CMD "!!SERIAL RADIO BOOTED"

void slip_send_packet(const uint8_t *ptr, int len);
void slip_send_packet_payload_type(const uint8_t *ptr, int len, uint8_t payload_type);
extern void felicia_spi_init(void);

/* Current mode - default from boot is energy scan */
radio_mode_t serial_radio_mode = RADIO_MODE_IDLE;
static uint8_t active_channel;

/* max 32 packets at the same time??? */
static uint8_t packet_ids[32];
static int packet_pos;

static int serial_radio_cmd_handler(const uint8_t *data, int len);

uint8_t verbose_output = 1;

#ifdef CONTIKI_TARGET_ANANAS
#define TEST_CTS_RTS  (1 << 0)
#define TEST_BAUDRATE (1 << 1)
#define TEST_PERIOD   (30 * CLOCK_SECOND)
static struct ctimer test_timer;
static uint8_t active_test;
#endif /* CONTIKI_TARGET_ANANAS */
/*---------------------------------------------------------------------------*/
#ifdef CONTIKI_TARGET_ANANAS
static void
configure_cts_rts(int onoroff)
{
  uart1_flow_enable(onoroff);
}
#endif /* CONTIKI_TARGET_ANANAS */
/*---------------------------------------------------------------------------*/
#ifdef CONTIKI_TARGET_ANANAS
static void
test_callback(void *ptr)
{
  if(active_test & TEST_CTS_RTS) {
    /* Disable CTS/RTS */
    active_test &= ~TEST_CTS_RTS;
    configure_cts_rts(0);
    clock_delay_usec(50);
    printf("End of test CTS/RTS\n");
  }
  if(active_test & TEST_BAUDRATE) {
    /* Revert to default baudrate */
    active_test &= ~TEST_BAUDRATE;
    uart1_init(SERIAL_BAUDRATE);
    clock_delay_usec(50);
    printf("End of test baudrate\n");
  }
}
#endif /* CONTIKI_TARGET_ANANAS */
/*---------------------------------------------------------------------------*/
#ifdef CONTIKI_TARGET_FELICIA
/* Use different product name for USB on platform Felicia */
struct product {
  uint8_t size;
  uint8_t type;
  uint16_t string[18];
};
static const struct product product = {
  sizeof(product),
  3,
  {
    'Y','a','n','z','i',' ','S','e','r','i','a','l',' ','R','a','d','i','o'
  }
};
uint8_t *
serial_radio_get_product_description(void)
{
  return (uint8_t *)&product;
}
#endif /* CONTIKI_TARGET_FELICIA */
/*---------------------------------------------------------------------------*/
static void
sniffer_callback(void)
{
  uint8_t *p;
  if(packetbuf_hdralloc(2)) {
    p = (uint8_t *)packetbuf_hdrptr();
    p[0] = '!';
    p[1] = 'h';
    cmd_send(p, packetbuf_totlen());
  } else {
    /* packetbuf + hdr should always be sufficient large. */
    PRINTF("serial-radio: too much packet data from sniffer\n");
  }
}
/*---------------------------------------------------------------------------*/
radio_mode_t
radio_get_mode(void)
{
  return serial_radio_mode;
}
/*---------------------------------------------------------------------------*/
void
radio_set_mode(radio_mode_t mode)
{
  if(serial_radio_mode == mode) {
    /* no change */
    return;
  }

  /* Actions when leaving a radio mode */
  switch(serial_radio_mode) {
  case RADIO_MODE_IDLE:
  case RADIO_MODE_NORMAL:
    break;
  case RADIO_MODE_SNIFFER:
    NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE,
                             RADIO_RX_MODE_ADDRESS_FILTER | RADIO_RX_MODE_AUTOACK);
    break;
  case RADIO_MODE_SCAN:
    radio_scan_stop();
    /* Restore the active channel */
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, active_channel);
    break;
  }

  serial_radio_mode = mode;

  /* Actions when entering a radio mode */
  switch(mode) {
  case RADIO_MODE_IDLE:
  case RADIO_MODE_NORMAL:
    break;
  case RADIO_MODE_SNIFFER:
    sniffer_rdc_set_sniffer(sniffer_callback);
    NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, 0);
    break;
  case RADIO_MODE_SCAN:
    /* Activate the scanning */
    radio_scan_start();
    break;
  }
}
/*---------------------------------------------------------------------------*/
unsigned
serial_radio_get_channel(void)
{
  return active_channel;
}
/*---------------------------------------------------------------------------*/
void
serial_radio_set_channel(unsigned channel)
{
  active_channel = channel & 0xff;
  printf("radio: setting channel: %u\n", active_channel);
  if(serial_radio_mode == RADIO_MODE_SCAN) {
    /* Notify the radio scanner about the channel change */
    radio_scan_set_channel(active_channel);
  } else {
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, active_channel);
  }
}
/*---------------------------------------------------------------------------*/
void
serial_radio_send_watchdog_warning(uint16_t seconds_left)
{
  uint8_t buf[8];
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = 'W';
  buf[3] = (seconds_left >> 8) & 0xff;
  buf[4] = seconds_left & 0xff;
  cmd_send(buf, 5);
}
/*---------------------------------------------------------------------------*/
#ifdef CMD_CONF_HANDLERS
CMD_HANDLERS(serial_radio_cmd_handler, CMD_CONF_HANDLERS);
#else
CMD_HANDLERS(serial_radio_cmd_handler);
#endif
/*---------------------------------------------------------------------------*/
static void
packet_sent(void *ptr, int status, int transmissions)
{
  uint8_t buf[20];
  uint8_t sid;
  int pos;
  sid = *((uint8_t *)ptr);
  PRINTF("Serial-radio: packet sent! sid: %d, status: %d, tx: %d\n",
         sid, status, transmissions);
  /* packet callback from lower layers */
  /*  neighbor_info_packet_sent(status, transmissions); */
  pos = 0;
  buf[pos++] = '!';
  buf[pos++] = 'R';
  buf[pos++] = sid;
  buf[pos++] = status; /* one byte ? */
  buf[pos++] = transmissions;
  cmd_send(buf, pos);
}
/*---------------------------------------------------------------------------*/
static void
send_version(int radio_restarted)
{
  uint8_t buf[6];
  buf[0] = '!';
  buf[1] = radio_restarted ? '!' : 'v';
  buf[2] = (SERIAL_RADIO_CONTROL_API_VERSION >> 24) & 0xff;
  buf[3] = (SERIAL_RADIO_CONTROL_API_VERSION >> 16) & 0xff;
  buf[4] = (SERIAL_RADIO_CONTROL_API_VERSION >>  8) & 0xff;
  buf[5] = (SERIAL_RADIO_CONTROL_API_VERSION >>  0) & 0xff;
  cmd_send(buf, 6);
}
/*---------------------------------------------------------------------------*/

int recvcnt = 0;
static int
serial_radio_cmd_handler(const uint8_t *data, int len)
{
  int i;

  if(data[0] == '!') {
    /* should send out stuff to the radio - ignore it as IP */
    /* --- s e n d --- */
    switch(data[1]) {
    case 'S': {
      int pos;
      packet_ids[packet_pos] = data[2];

      packetbuf_clear();
      pos = packetutils_deserialize_atts(&data[3], len - 3);
      if(pos < 0) {
        PRINTF("serial-radio: illegal packet attributes\n");
        return 1;
      }
      pos += 3;
      len -= pos;
      if(len > PACKETBUF_SIZE) {
        len = PACKETBUF_SIZE;
      }
      memcpy(packetbuf_dataptr(), &data[pos], len);
      packetbuf_set_datalen(len);

      PRINTF("serial-radio: sending %u (%d bytes)\n",
             data[2], packetbuf_datalen());

      /* parse frame before sending to get addresses, etc. */
      NETSTACK_FRAMER.parse();
      NETSTACK_MAC.send(packet_sent, &packet_ids[packet_pos]);

      packet_pos++;
      if(packet_pos >= sizeof(packet_ids)) {
        packet_pos = 0;
      }

      return 1;
    }
    case 's': {
      /* Send directly to radio */
      if(serial_radio_mode == RADIO_MODE_NORMAL) {
        PRINTF("serial-radio: direct sending %d bytes\n", len);
        packetbuf_clear();
        NETSTACK_RADIO.send(&data[2], len - 2);
      }
      return 1;
    }
    case 'C': {
      serial_radio_set_channel(data[2]);
      return 1;
    }
    case 'T': {
      int8_t txpower = data[2];
      printf("radio: setting tx power: %d dBm\n", txpower);
      NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, txpower);
      return 1;
    }
    case 'P': {
      uint16_t pan_id = ((uint16_t)data[2] << 8) | data[3];
      printf("radio: setting pan id: %u (0x%04x)\n", pan_id, pan_id);
      NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, pan_id);
      return 1;
    }
    case 'V': {
      int type = ((uint16_t)data[2] << 8) | data[3];
      int value = ((uint16_t)data[4] << 8) | data[5];
      int param = packetutils_to_radio_param(type);
      if(param < 0) {
        printf("radio: unknown parameter %d (can not set to %d)\n", type, value);
      } else {
        printf("radio: setting parameter %d to %d\n", param, value);
        NETSTACK_RADIO.set_value(param, value);
      }
      return 1;
    }
    case 'm': {
      if(serial_radio_mode != data[2]) {
        radio_set_mode(data[2]);
        printf("radio: mode set to %u\n", serial_radio_mode);
      } else {
        printf("radio: already in mode %u\n", serial_radio_mode);
      }
      return 1;
    }
    case 'c': {
      radio_scan_set_mode(data[2]);
      printf("radio scan: mode set to %u\n", data[2]);
      return 1;
    }
    case 'd': {
      verbose_output = data[2];
      printf("verbose output is set to %u\n", verbose_output);
      return 1;
    }
    case 'x': {
      unsigned long rate;
      switch(data[2]) {
#ifdef CONTIKI_TARGET_ANANAS
      case 'c':
        active_test &= ~TEST_CTS_RTS;
        if(data[3] == 2) {
          /* Enable CTS/RTS */
          printf("Enabling CTS/RTS\n");
          clock_delay_usec(2000);
          configure_cts_rts(1);
        } else if(data[3] == 1) {
          /* Temporary test CTS/RTS */
          printf("Testing CTS/RTS\n");
          clock_delay_usec(2000);
          configure_cts_rts(1);
          active_test |= TEST_CTS_RTS;
          ctimer_set(&test_timer, TEST_PERIOD, test_callback, NULL);
        } else {
          /* Disable CTS/RTS */
          configure_cts_rts(0);
          printf("Disabling CTS/RTS\n");
        }
        return 1;
      case 'u':
      case 'U':
        rate = (data[3] << 24) + (data[4] << 16) + (data[5] << 8) + data[6];
        if(rate < 1200) {
          printf("Illegal baudrate: %lu\n", rate);
          return 1;
        }
        if(data[2] == 'U') {
          printf("Testing baudrate %lu\n", rate);
          active_test |= TEST_BAUDRATE;
          ctimer_set(&test_timer, TEST_PERIOD, test_callback, NULL);
        } else {
          printf("Changing baudrate to %lu\n", rate);
          active_test &= ~TEST_BAUDRATE;
        }
        clock_delay_usec(2000);
        uart1_init(rate);
        return 1;
#endif /* CONTIKI_TARGET_ANANAS */
#ifdef HAVE_RADIO_FRONTPANEL
      case 'l':
        rate = (data[3] << 8) + data[4];
        radio_frontpanel_set_info(rate);
        printf("Set frontpanel info to %lu\n", rate);
        return 1;
#endif /* HAVE_RADIO_FRONTPANEL */
      case 'd':
        rate = (data[3] << 8) + data[4];
        slip_set_fragment_delay(rate & 0xffff);
        printf("Changing fragment delay to %lu\n", rate);
        return 1;
      case 'b':
        rate = (data[3] << 8) + data[4];
        slip_set_fragment_size(rate & 0xffff);
        printf("Changing fragment size to %lu\n", rate);
        return 1;
      case 'r':
        printf("Rebooting node!\n");
        clock_delay_usec(1000);
        if(len > 2) {
          platform_reboot_to_selected_image(data[3]);
        } else {
          platform_reboot();
        }
        return 1;
#ifdef HAVE_RADIO_FRONTPANEL
      case 'W':
        rate = (data[3] << 8) + data[4];
        radio_frontpanel_set_watchdog(rate);
        PRINTF("Set radio watchdog to trig in %lu sec\n", rate);
        return 1;
#endif /* HAVE_RADIO_FRONTPANEL */
      }
      return 1;
    }
    }
  } else if(data[0] == '?') {
    radio_value_t value;

    PRINTF("Got request message of type %c\n", data[1]);
    switch(data[1]) {
    case 'M': {
      uint8_t buf[10];
      buf[0] = '!';
      buf[1] = 'M';
      for(i = 0; i < 8; i++) {
        buf[2 + i] = uip_lladdr.addr[i];
      }
      cmd_send(buf, 10);
      return 1;
    }
    case 'C': {
      uint8_t buf[4];
      /* buf[2] = active_channel; */
      /* Return the channel currently being used by the radio */
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &value) == RADIO_RESULT_OK) {
        buf[0] = '!';
        buf[1] = 'C';
        buf[2] = value & 0xff;
        cmd_send(buf, 3);
      } else {
        PRINTF("Failed to get channel\n");
      }
      return 1;
    }
    case 'T': {
      uint8_t buf[4];
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &value) == RADIO_RESULT_OK) {
        buf[0] = '!';
        buf[1] = 'T';
        buf[2] = value & 0xff;
        cmd_send(buf, 3);
      } else {
        PRINTF("Failed to get txpower\n");
      }
      return 1;
    }
    case 'P': {
      uint8_t buf[5];
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_PAN_ID, &value) == RADIO_RESULT_OK) {
        buf[0] = '!';
        buf[1] = 'P';
        buf[2] = (value >> 8) & 0xff;
        buf[3] = value & 0xff;
        cmd_send(buf, 4);
      } else {
        PRINTF("failed to get pan id\n");
      }
      return 1;
    }
    case 'V': {
      int type = ((uint16_t)data[2] << 8) | data[3];
      int param = packetutils_to_radio_param(type);
      uint8_t buf[7];

      if(param < 0) {
        printf("radio: unknown parameter %d\n", type);
      } else if(NETSTACK_RADIO.get_value(param, &value) == RADIO_RESULT_OK) {
        buf[0] = '!';
        buf[1] = 'V';
        buf[2] = (type >> 8) & 0xff;
        buf[3] = type & 0xff;
        buf[4] = (value >> 8) & 0xff;
        buf[5] = value & 0xff;
        cmd_send(buf, 6);
      } else {
        printf("radio: could not get value for %d\n", param);
      }
      return 1;
    }
    case 'm': {
      uint8_t buf[3];
      buf[0] = '!';
      buf[1] = 'm';
      buf[2] = serial_radio_mode;
      cmd_send(buf, 3);
      return 1;
    }
    case 'c': {
      uint8_t buf[3];
      buf[0] = '!';
      buf[1] = 'c';
      buf[2] = radio_scan_get_mode();
      cmd_send(buf, 3);
      return 1;
    }
    case 'v': {
      send_version(0);
      return 1;
    }
    case 'd': {
      uint8_t buf[3];
      buf[0] = '!';
      buf[1] = 'd';
      buf[2] = verbose_output;
      cmd_send(buf, 3);
      return 1;
    }
    case 'S':
      /* Status */
      printf("ENCAP: received %lu bytes payload, %lu errors\n",
             SERIAL_RADIO_STATS_GET(SERIAL_RADIO_STATS_ENCAP_RECV),
             SERIAL_RADIO_STATS_GET(SERIAL_RADIO_STATS_ENCAP_ERRORS));
      printf("SERIAL: received %lu bytes, %lu frames\n",
             SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_RECV),
             SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_FRAMES));
      printf("SLIP: %lu overflows, %lu dropped bytes, %lu errors\n",
             SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_OVERFLOWS),
             SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_DROPPED),
             SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_ERRORS));
#ifdef HAVE_RADIO_FRONTPANEL
      {
        uint32_t r = radio_get_reset_cause();
        uint32_t wd = radio_frontpanel_get_watchdog();
        uint32_t s = radio_frontpanel_get_supply_status();

        printf("Frontpanel: info %lu, reset 0x%02x, ext reset 0x%02x\n",
               (unsigned long)radio_frontpanel_get_info(),
               (unsigned)((r >> 8) & 0xff), (unsigned)(r & 0xff));
        printf("Frontpanel: supply type %u status %u, on battery %lu seconds, %ld mV\n",
               (unsigned)((s >> 8) & 0xff), (unsigned)(s & 0xff),
               (unsigned long)radio_frontpanel_get_battery_supply_time(),
               (long)radio_frontpanel_get_supply_voltage());
        if(wd == 0) {
          printf("Frontpanel: Radio watchdog is disabled\n");
        } else {
          printf("Frontpanel: Radio watchdog will expire in %lu seconds\n", (unsigned long)wd);
        }
      }
#endif /* HAVE_RADIO_FRONTPANEL */
      return 1;
    case 'x': {
      switch(data[2]) {
#ifdef HAVE_RADIO_FRONTPANEL
      case 'W': {
        uint32_t wd = radio_frontpanel_get_watchdog();
        if(wd == 0) {
          printf("Radio watchdog is disabled\n");
        } else {
          printf("Radio watchdog will expire in %lu seconds\n", (unsigned long)wd);
        }
        return 1;
      }
#endif /* HAVE_RADIO_FRONTPANEL */
      default:
        return 1;
      }
    }
    }
  } /* if(data[0] == '?' */
  return 0;
}
/*---------------------------------------------------------------------------*/
void
serial_radio_cmd_output(const uint8_t *data, int data_len)
{
  slip_send_packet(data, data_len);
}
/*---------------------------------------------------------------------------*/
void
serial_radio_cmd_error(const uint8_t *data, int data_len)
{
  uint8_t buf[4];
  /* This packet was not accepted by any command handler */
  buf[0] = '!';
  buf[1] = 'E';
  buf[2] = *data;
  buf[3] = *(data + 1);
  slip_send_packet(buf, 4);
}
/*---------------------------------------------------------------------------*/
void
slip_input_callback(pdu_info *pinfo, uint8_t *payload, int enclen)
{
  size_t l = 0;
  int len;
  uint8_t txbuf[1280];

  len = pinfo->payload_len;

  switch(pinfo->payload_type) {

  case ENC_PAYLOAD_TLV:
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_TLV);

    l += process_TLV_request(payload + enclen, len, txbuf + l, sizeof(txbuf) - l, pinfo);
    txbuf[l++] = 0;
    txbuf[l++] = 0;
    slip_send_packet_payload_type(txbuf, l, pinfo->payload_type);
    break;

  case ENC_PAYLOAD_SERIAL:
    /* printf("enclen:%d '%c':'%c' (%i)\n", enclen, ndata[0], ndata[1], recvcnt++); */
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_SERIAL);
    cmd_input(payload + enclen, len);
    break;

  default:
    printf("Unprocessed payload type %d\n", pinfo->payload_type);
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_UNPROCESSED);
    break;
  } /* switch (payload_type) */
}
/*---------------------------------------------------------------------------*/
PROCESS(serial_radio_process, "Serial radio process");
AUTOSTART_PROCESSES(&serial_radio_process);
/*---------------------------------------------------------------------------*/
static void
init_config(void)
{
  radio_value_t value;
  if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &value) == RADIO_RESULT_OK) {
    active_channel = value;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(serial_radio_process, ev, data)
{
  PROCESS_BEGIN();

  packet_pos = 0;

#if SLIP_ARCH_CONF_SPI == 1
  felicia_spi_init();
#endif
  init_config();

  NETSTACK_RDC.off(1);

  printf("Serial Radio started...\n");

#ifdef HAVE_RADIO_FRONTPANEL
  radio_frontpanel_init();
#endif

  /* Notify border router that the serial radio has started (or rebooted) */
  cmd_send((uint8_t *)BOOT_CMD, sizeof(BOOT_CMD));

  /* Scan all channels from start */
  radio_set_mode(RADIO_MODE_SCAN);

  while(1) {
    PROCESS_YIELD();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
