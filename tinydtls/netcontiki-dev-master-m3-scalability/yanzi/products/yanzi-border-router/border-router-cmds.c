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
 *         Sets up some commands for the border router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "cmd.h"
#include "border-router.h"
#include "border-router-cmds.h"
#include "br-config.h"
#include "instance-brm.h"
#include "dev/serial-line.h"
#include "net/rpl/rpl.h"
#include "net/mac/handler-802154.h"
#include "net/mac/framer-802154.h"
#include "vargen.h"
#include <string.h>
#include <stdlib.h>
#include "net/llsec/noncoresec/noncoresec.h"

#define YLOG_LEVEL YLOG_LEVEL_DEBUG
#define YLOG_NAME  "br-cmd"
#include "lib/ylog.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#ifndef BORDER_ROUTER_CONTROL_API_VERSION
#error "No BORDER_ROUTER_CONTROL_API_VERSION specified. Please set in project-conf.h!"
#endif

#ifndef BORDER_ROUTER_PRINT_RSSI
#define BORDER_ROUTER_PRINT_RSSI 0
#endif

static uint32_t radio_control_version;
extern uint8_t radio_info;
extern uint8_t verbose_output;

uint8_t command_context;
uint32_t router_status = 0;

void packet_sent(uint8_t sessionid, uint8_t status, uint8_t tx);

/*---------------------------------------------------------------------------*/
PROCESS(border_router_cmd_process, "Border router cmd process");
/*---------------------------------------------------------------------------*/
static const uint8_t *
hextoi(const uint8_t *buf, int len, int *v)
{
  *v = 0;
  for(; len > 0; len--, buf++) {
    if(*buf >= '0' && *buf <= '9') {
      *v = (*v << 4) + ((*buf - '0') & 0xf);
    } else if(*buf >= 'a' && *buf <= 'f') {
      *v = (*v << 4) + ((*buf - 'a' + 10) & 0xf);
    } else if(*buf >= 'A' && *buf <= 'F') {
      *v = (*v << 4) + ((*buf - 'A' + 10) & 0xf);
    } else {
      break;
    }
  }
  return buf;
}
/*---------------------------------------------------------------------------*/
static const uint8_t *
dectoi(const uint8_t *buf, int len, int *v)
{
  int negative = 0;
  *v = 0;
  if(len == 0) {
    return buf;
  }
  if(*buf == '$') {
    return hextoi(buf + 1, len - 1, v);
  }
  if(*buf == '0' && *(buf + 1) == 'x' && len > 2) {
    return hextoi(buf + 2, len - 2, v);
  }
  if(*buf == '-') {
    negative = 1;
    buf++;
  }
  for(; len > 0; len--, buf++) {
    if(*buf < '0' || *buf > '9') {
      break;
    }
    *v = (*v * 10) + ((*buf - '0') & 0xf);
  }
  if(negative) {
    *v = - *v;
  }
  return buf;
}
/*---------------------------------------------------------------------------*/
static void
printhex(char c)
{
  const char *hex = "0123456789ABCDEF";
  putchar(hex[(c >> 4) & 0xf]);
  putchar(hex[c & 0xf]);
}
/*---------------------------------------------------------------------------*/
int
border_router_cmd_handler(const uint8_t *data, int len)
{
  /* handle global repair, etc here */
  if(data[0] == '!') {
    PRINTF("Got configuration message of type %c\n", data[1]);
    switch(data[1]) {
    case '!':
      if(command_context == CMD_CONTEXT_RADIO) {
	/* Slip radio has rebooted */
	if(memcmp("SERIAL RADIO BOOTED", data + 2, 19) == 0) {
	  YLOG_INFO("Serial radio has rebooted. Exiting border router.\n");
	  exit(EXIT_FAILURE);
	}
      }
      return 1;
    case 'G':
      if(command_context == CMD_CONTEXT_STDIO) {
        YLOG_INFO("Performing Global Repair...\n");
        rpl_repair_root(wsn_rpl_instance_id);
      }
      return 1;
    case 'M':
      if(command_context == CMD_CONTEXT_RADIO) {
        /* We need to know that this is from the slip-radio here. */
        PRINTF("Setting MAC address\n");
        border_router_set_mac(&data[2]);
      }
      return 1;
    case 'C':
      if(command_context == CMD_CONTEXT_RADIO) {
        /* We need to know that this is from the slip-radio here. */
        YLOG_INFO("Channel is: %d\n", data[2]);
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        int channel;
        dectoi(&data[2], len - 2, &channel);
        if(channel >= 11 && channel <= 26) {
          uint8_t buf[5];
          buf[0] = '!';
          buf[1] = 'C';
          buf[2] = channel & 0xff;
          write_to_slip(buf, 3);

          border_router_radio_set_value(RADIO_PARAM_CHANNEL, channel);
          return 1;
        }
        YLOG_ERROR("*** illegal channel: %u\n", channel);
        return 1;
      }
      return 1;
    case 'T':
      if(command_context == CMD_CONTEXT_RADIO) {
        YLOG_INFO("Radio tx power is %d dBm\n", (int8_t)data[2]);
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[5];
        int power;
        dectoi(&data[2], len - 2, &power);
        buf[0] = '!';
        buf[1] = 'T';
        buf[2] = power & 0xff;
        write_to_slip(buf, 3);
        return 1;
      }
      return 1;
    case 'P':
      if(command_context == CMD_CONTEXT_RADIO) {
        uint16_t pan_id = ((uint16_t)data[2] << 8) | data[3];
        YLOG_INFO("Pan id is: %u (0x%04x)\n", pan_id, pan_id);
	router_status |= BORDER_ROUTER_STATUS_CONFIGURATION_PAN_ID;
        handler_802154_join(pan_id, 1);

        border_router_radio_set_value(RADIO_PARAM_PAN_ID, pan_id);
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        int pan_id;
        dectoi(&data[2], len - 2, &pan_id);
        /* printf("PAN ID: %u (%04x)\n", pan_id, pan_id); */
        uint8_t buf[5];
        buf[0] = '!';
        buf[1] = 'P';
        buf[2] = (pan_id >> 8) & 0xff;
        buf[3] = pan_id & 0xff;
        write_to_slip(buf, 4);
        handler_802154_join(pan_id, 1);

        border_router_radio_set_value(RADIO_PARAM_PAN_ID, pan_id);
        return 1;
      }
      return 1;
    case 'R':
      if(command_context == CMD_CONTEXT_RADIO) {
        /* We need to know that this is from the slip-radio here. */
        PRINTF("Packet data report for sid:%d st:%d tx:%d\n",
               data[2], data[3], data[4]);
        packet_sent(data[2], data[3], data[4]);
      }
      return 1;
    case 'E':
      if(command_context == CMD_CONTEXT_RADIO) {
        /* Error from the radio */
        YLOG_ERROR("SR: Unknown command: %c%c (0x%02x%02x)\n",
                   data[2], data[3],
                   data[2], data[3]);
      }
      return 1;
    case 'B':
      if(command_context == CMD_CONTEXT_RADIO) {
        handler_802154_set_beacon_payload((uint8_t *)&data[3], data[2]);
	router_status |= BORDER_ROUTER_STATUS_CONFIGURATION_BEACON;

        /* Beacon has been set - set radio mode to normal mode */
        /* TODO radio mode should be set by Java! */
        border_router_set_radio_mode(0x01);
        return 1;
      }
      return 1;
    case 'h':
      /* Print the sniffed packet as hex to standard out for now. */
      if(command_context == CMD_CONTEXT_RADIO) {
        int i;
        printf("h:");
        for(i = 2; i < len; i++) {
          printhex(data[i]);
        }
        printf("\n");
      }
      return 1;
    case 'e':
      /* Energy */
      if(command_context == CMD_CONTEXT_RADIO) {
        uint32_t count = (data[3] << 8) + data[4];
        uint8_t channel = data[2];
        int8_t avg = data[5];
        int8_t max = data[6];
        int8_t min = data[7];
        if(verbose_output > 1) {
          printf("E: %02d: %02d max %02d min %02d (%u measures)\n",
                 channel, avg, max, min, (unsigned)count);
        }

        if(radio_control_version < 2) {
          /* Disable energy scan mode on older serial radios because
             when leaving energy scan mode, they might restore a
             earlier radio channel. */
          border_router_set_radio_mode(0x01);
          YLOG_ERROR("*** old serial radio: disabling energy scan\n");
        }
      }
      return 1;
    case 'r':
      /* RSSI */
      if(command_context == CMD_CONTEXT_RADIO) {
#if BORDER_ROUTER_PRINT_RSSI
        int pos;
        int8_t rssi;
        if(data[2] == 'A') {
          /* Scan over all channels */
          printf("RSSI:");
          for(pos = 3; pos < len; pos++) {
            rssi = (int8_t)data[pos];
            printf(" %d", rssi);
          }
          printf("\n");

        } else if(data[2] == 'C') {
          /* Scan over single channel */
          printf("RSSI (C):");
          for(pos = 3; pos < len; pos++) {
            rssi = (int8_t)data[pos];
            printf(" %d", rssi);
          }
          printf("\n");
        } else {
          YLOG_ERROR("*** unknown RSSI from radio: %u\n", data[2]);
        }
#endif /* BORDER_ROUTER_PRINT_RSSI */
      }
      return 1;
    case 'm':
      if(command_context == CMD_CONTEXT_RADIO) {
        YLOG_INFO("Radio mode: %u\n", data[2]);
	// Clear router_status variable when radio mode has been set to 0.
	if(data[2] == 0) {
	  router_status = 0;
	} else {
	  router_status |=BORDER_ROUTER_STATUS_CONFIGURATION_MODE;
	}
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[3];
        int mode;
        dectoi(&data[2], len - 2, &mode);
        buf[0] = '!';
        buf[1] = 'm';
        buf[2] = mode & 0xff;
        write_to_slip(buf, 3);
        return 1;
      }
      return 1;
    case 'c':
      if(command_context == CMD_CONTEXT_RADIO) {
        YLOG_INFO("Radio scan mode: %u\n", data[2]);
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[3];
        int mode;
        dectoi(&data[2], len - 2, &mode);
        buf[0] = '!';
        buf[1] = 'c';
        buf[2] = mode & 0xff;
        write_to_slip(buf, 3);
        return 1;
      }
      return 1;
    case 'H':
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[3];
        int mode;
        dectoi(&data[2], len - 2, &mode);
        buf[0] = '!';
        buf[1] = 'm';
        buf[2] = mode == 0 ? 0 : 3;
        write_to_slip(buf, 3);
        return 1;
      }
      return 1;
    case 'd':
      if(command_context == CMD_CONTEXT_STDIO) {
        int verbose;
        uint8_t buf[3];
        dectoi(&data[2], len - 2, &verbose);
        verbose_output = verbose;
        buf[0] = '!';
        buf[1] = 'd';
        buf[2] = verbose & 0xff;
        write_to_slip(buf, 3);
        return 1;
      }
      if(command_context == CMD_CONTEXT_RADIO) {
        YLOG_INFO("Radio verbose: %u  NBR verbose: %u\n", data[2], verbose_output);
        return 1;
      }
      return 1;
    case 'v':
      if(command_context == CMD_CONTEXT_RADIO) {
        uint32_t v;
        v  = data[2] << 24;
        v |= data[3] << 16;
        v |= data[4] <<  8;
        v |= data[5];
        radio_control_version = v;
        YLOG_INFO("Radio protocol version: %u\n", v);
      }
      return 1;
    case 'K':
      if(command_context == CMD_CONTEXT_RADIO) {
	uint8_t new_key[16];
	memcpy(new_key, &data[2], 16);
	noncoresec_set_key(new_key, 16);
      }
      return 1;
    case 'L':
      if(command_context == CMD_CONTEXT_RADIO) {
	uint8_t new_security_level;
	new_security_level = data[2];
	noncoresec_set_security_level(new_security_level);
      }
      return 1;
    case 'x': {
      if(command_context == CMD_CONTEXT_STDIO) {
	uint8_t buf[10];
	int rate, pos;
        buf[0] = '!';
        buf[1] = 'x';
        buf[2] = data[2];
	switch(data[2]) {
        case 'c':
          dectoi(&data[3], len - 3, &rate);
          YLOG_INFO("Configuring serial CTS/RTS to %u\n", rate);
          border_router_set_ctsrts(rate);
          return 1;
	case 'u':
	case 'U':
	  dectoi(&data[3], len - 3, &rate);
          if(rate < 1200) {
            YLOG_ERROR("Illegal baudrate for serial radio: %u!\n", rate);
            return 1;
          }
	  YLOG_INFO("Configuring serial radio for %u baud\n", rate);
          border_router_set_baudrate(rate, data[2] == 'U' ? 1 : 0);
	  return 1;
	case 'r':
	  /* Forward to serial radio */
	  if(len > 2) {
	    dectoi(&data[3], len - 3, &pos);
	    if(pos > 0 && pos < 3) {
	      YLOG_INFO("Requesting serial radio to reboot to image %u!\n", pos);
	      printf("requesting serial radio to reboot to image %u!\n", pos);
	      buf[3] = pos & 0xff;
	      write_to_slip(buf, 4);
	      return 1;
	    }
	  }
	  YLOG_INFO("Requesting serial radio to reboot!\n");
	  write_to_slip(data, len);
	  return 1;
        case 'W':
          /* Set radio watchdog */
          dectoi(&data[3], len - 3, &rate);
          YLOG_DEBUG("Setting radio watchdog to %u\n", rate);
          border_router_set_radio_watchdog(rate);
          return 1;
        case 'l':
          dectoi(&data[3], len - 3, &rate);
          YLOG_DEBUG("Setting frontpanel info: %u\n", rate);
          buf[3] = (rate >> 8) & 0xff;
          buf[4] = rate & 0xff;
          write_to_slip(buf, 5);
          return 1;
	case 'd':
	  dectoi(&data[3], len - 3, &rate);
	  YLOG_DEBUG("Configuring serial radio for fragment delay %u\n", rate);
	  buf[3] = (rate >> 8) & 0xff;
	  buf[4] = rate & 0xff;
	  write_to_slip(buf, 5);
	  return 1;
	case 'b':
	  dectoi(&data[3], len - 3, &rate);
	  YLOG_DEBUG("Configuring serial radio for fragment size %u\n", rate);
	  buf[3] = (rate >> 8) & 0xff;
	  buf[4] = rate & 0xff;
	  write_to_slip(buf, 5);
	  return 1;
	}
      }
      if(command_context == CMD_CONTEXT_RADIO) {
	switch(data[2]) {
        case 'W': {
          uint16_t sec;
          int reboot_status;
          /* The radio is telling us the radio watchdog soon will expire */
          sec = (data[3] << 8) + data[4];
          YLOG_INFO("THE RADIO WATCHDOG WILL REBOOT SYSTEM IN %u SECONDS!\n", sec);

          reboot_status = system(BORDER_ROUTER_REBOOT_PLATFORM_COMMAND);
          YLOG_INFO("executing %s returned %d\n", BORDER_ROUTER_REBOOT_PLATFORM_COMMAND, reboot_status);
          break;
        }
	}
      }
      return 1;
    }
    }
  } else if(data[0] == '?') {
    PRINTF("Got request message of type %c\n", data[1]);
    switch(data[1]) {
    case 'M':
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[20];
        const char *hexchar = "0123456789abcdef";
        int j;
        /* this is just a test so far... just to see if it works */
        buf[0] = '!';
        buf[1] = 'M';
        for(j = 0; j < 8; j++) {
          buf[2 + j * 2] = hexchar[uip_lladdr.addr[j] >> 4];
          buf[3 + j * 2] = hexchar[uip_lladdr.addr[j] & 15];
        }
        cmd_send(buf, 18);
      }
      return 1;
    case 'C':
    case 'T':
    case 'P':
    case 'm':
    case 'c':
    case 'x':
      if(command_context == CMD_CONTEXT_STDIO) {
        /* send on! */
        write_to_slip(data, len);
        return 1;
      }
      return 1;
    case 'v':
      if(command_context == CMD_CONTEXT_STDIO) {
        /* send on! */
        write_to_slip(data, len);
        YLOG_INFO("Border router version: %u\n", BORDER_ROUTER_CONTROL_API_VERSION);
        return 1;
      }
      return 1;
    case 'V':
      if(command_context == CMD_CONTEXT_STDIO) {
        /* TODO Radio parameter */
        return 1;
      }
      return 1;
    case 'd':
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[3];
        buf[0] = '?';
        buf[1] = 'd';
        write_to_slip(buf, 2);
        return 1;
      }
      return 1;
    case 'S': {
      if(command_context == CMD_CONTEXT_STDIO) {
	uint8_t buf[3];
	border_router_print_stat();
	buf[0] = '?';
	buf[1] = 'S';
	write_to_slip(buf, 2);
	return 1;
      }
      return 1;
    }
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
border_router_cmd_output(const uint8_t *data, int data_len)
{
  int i;
  YLOG_INFO("CMD output: ");
  for(i = 0; i < data_len; i++) {
    printf("%c", data[i]);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
void
border_router_cmd_error(const uint8_t *data, int data_len)
{
  YLOG_ERROR("Unknown command: %c%c (0x%02x%02x)\n",
             data[0], data[1], data[0], data[1]);
}
/*---------------------------------------------------------------------------*/
uint32_t
border_router_get_status()
{
  return router_status;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_cmd_process, ev, data)
{
  PROCESS_BEGIN();
  PRINTF("Started br-cmd process\n");
  while(1) {
    PROCESS_YIELD();
    if(ev == serial_line_event_message && data != NULL) {
      PRINTF("Got serial data!!! %s of len: %d\n",
	     (char *)data, strlen((char *)data));
      command_context = CMD_CONTEXT_STDIO;
      cmd_input(data, strlen((char *)data));
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
