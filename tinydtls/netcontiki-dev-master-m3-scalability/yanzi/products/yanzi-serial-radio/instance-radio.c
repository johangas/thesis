/*
 * Copyright (c) 2013-2015, Yanzi Networks AB.
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
 */

#include "contiki.h"
#include "oam.h"
#include "api.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "cmd.h"
#include "serial-radio.h"
#include "serial-radio-stats.h"
#include "radio-scan.h"
#include "radio-frontpanel.h"
#include <stdio.h>
#include "net/mac/handler-802154.h"

uint32_t serial_radio_stats[SERIAL_RADIO_STATS_MAX];
uint32_t serial_radio_stats_debug[SERIAL_RADIO_STATS_DEBUG_MAX];
static uint16_t reset_info = 0;

static instanceControl *localIC = NULL;
extern uint8_t zeroes[16];

#define HIGHEST_RADIO_CHANNEL 26
#define LOWEST_CHANNEL 11
#define LINK_LAYER_KEY_SIZE 16

/*---------------------------------------------------------------------------*/
/*
 * Calculate beacon payload length.
 * Return length or -1 on error.
 */
static int
calculate_beacon_payload_length(const uint8_t *beacon, int maxlen)
{
  int i;

  if(beacon[0] != 0xfe) {
    return -1;
  }

  for(i = 1; beacon[i] != 0; i += beacon[i]) {
    if(i >= maxlen) {
      return -1;
    }
  }
  i++; /* include the last 0-length indicator */
  return i;
}
/*----------------------------------------------------------------*/
uint32_t
radio_get_reset_cause(void)
{
  return reset_info;
}
/*----------------------------------------------------------------*/
/**
 * Process a request TLV.
 *
 * Writes a response TLV starting at "reply", ni more than "len"
 * bytes.
 *
 * if the "request" is of type vectorAbortIfEqualRequest or
 * abortIfEqualRequest, and processing should be aborted,
 * "endProcessing" is set to TRUE.
 *
 * Returns number of bytes written to "reply".
 */
static size_t
radio_process_request(TLV *request, uint8_t *reply, size_t len, uint8_t *endProcessing)
{
  uint32_t localData;
  if((request->opcode == TLV_OPCODE_SET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_SET_REQUEST)) {

#ifdef VARIABLE_RADIO_CHANNEL
    if(request->variable == VARIABLE_RADIO_CHANNEL) {
      localData = get_int32_from_data(request->data);
      if((localData <= HIGHEST_RADIO_CHANNEL) && (localData >= LOWEST_CHANNEL)) {
        serial_radio_set_channel(localData);
        return writeReply32Tlv(request, reply, len, 0);
      } else {
        return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
    }
#endif /* VARIABLE_RADIO_CHANNEL */

#ifdef VARIABLE_RADIO_PAN_ID
    if(request->variable == VARIABLE_RADIO_PAN_ID) {
      uint16_t pan_id = request->data[2] << 8 | request->data[3];
      printf("radio: setting pan id : %u (0x%04x)\n", pan_id, pan_id);
      NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, pan_id);
      {
        /* Tell our border router */
        uint8_t buf[5];
        buf[0] = '!';
        buf[1] = 'P';
        buf[2] = request->data[2];
        buf[3] = request->data[3];
        cmd_send(buf, 4);
      }
      return writeReply32Tlv(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_PAN_ID */

#ifdef VARIABLE_RADIO_BEACON_RESPONSE
    if(request->variable == VARIABLE_RADIO_BEACON_RESPONSE) {
      int i;
      /* Validate beacon response type and basic length */
      if(request->data[0] != 0xfe) {
        return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      if(request->elementSize * request->elements > BEACON_PAYLOAD_BUFFER_SIZE) {
        return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      i = calculate_beacon_payload_length(request->data, request->elementSize * request->elements);
      if(i > 0) {
        handler_802154_set_beacon_payload(request->data, i);
      } else {
        return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      printf("radio: setting beacon : ");
      for(localData = 0; localData < i; localData++) {
        printf("%02x", request->data[localData]);
      }
      printf("\n");
      {
        /* Tell our border router */
        uint8_t buf[BEACON_PAYLOAD_BUFFER_SIZE + 4];
        buf[0] = '!';
        buf[1] = 'B';
        buf[2] = i;
        memcpy(&buf[3], request->data, i);
        cmd_send(buf, i + 3);
      }
      return writeReplyVectorTlv(request, reply, len, zeroes);
    }
#endif /* VARIABLE_RADIO_BEACON_RESPONSE */

#ifdef VARIABLE_RADIO_MODE
    if(request->variable == VARIABLE_RADIO_MODE) {
      localData = get_int32_from_data(request->data);
      radio_set_mode(localData);
      {
        /* Tell our border router */
        uint8_t buf[4];
        buf[0] = '!';
        buf[1] = 'm';
        buf[2] = radio_get_mode() & 0xff;
        cmd_send(buf, 3);
      }
      return writeReply32Tlv(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_MODE */

#ifdef VARIABLE_RADIO_SCAN_MODE
    if(request->variable == VARIABLE_RADIO_SCAN_MODE) {
      localData = get_int32_from_data(request->data);
      radio_scan_set_mode(localData);
      return writeReply32Tlv(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_SCAN_MODE */

#ifdef VARIABLE_RADIO_SERIAL_MODE
    if(request->variable == VARIABLE_RADIO_SERIAL_MODE) {
      localData = get_int32_from_data(request->data);
      serial_set_mode(localData);
      return writeReply32Tlv(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_SERIAL_MODE */

#ifdef HAVE_RADIO_FRONTPANEL

#ifdef VARIABLE_RADIO_WATCHDOG
    if(request->variable == VARIABLE_RADIO_WATCHDOG) {
      localData = get_int32_from_data(request->data);
      radio_frontpanel_set_watchdog(localData);
      return writeReply32intTlv(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_WATCHDOG */

#ifdef VARIABLE_RADIO_FRONTPANEL_INFO
    if(request->variable == VARIABLE_RADIO_FRONTPANEL_INFO) {
      localData = get_int32_from_data(request->data);
      radio_frontpanel_set_info(localData);
      return writeReply32intTlv(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_FRONTPANEL_INFO */

#ifdef VARIABLE_RADIO_FRONTPANEL_ERROR_CODE
    if(request->variable == VARIABLE_RADIO_FRONTPANEL_ERROR_CODE) {
      localData = get_int32_from_data(request->data);
      radio_frontpanel_set_error_code(localData);
      return writeReply32intTlv(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_FRONTPANEL_INFO */

#endif /* HAVE_RADIO_FRONTPANEL */

#ifdef VARIABLE_RADIO_RESET_CAUSE
    if(request->variable == VARIABLE_RADIO_RESET_CAUSE) {
      /* Ignore the incoming value - just clear the reset cause */
      reset_info = (RADIO_RESET_CAUSE_NONE << 8) | RADIO_EXT_RESET_CAUSE_NONE;
      localData = 0;
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_RESET_CAUSE */

#ifdef VARIABLE_RADIO_LINK_LAYER_KEY
    if(request->variable == VARIABLE_RADIO_LINK_LAYER_KEY) {
      uint8_t valid = 0;
      int i;

      // All 0 is not a valid key
      for(i = 0; i < LINK_LAYER_KEY_SIZE ; i++) {
	if(request->data[i] != 0) {
	  valid = TRUE;
	  break;
	}
      }

      if(valid == FALSE) {
	return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }

      printf("radio: setting link layer key : ");
      for(localData = 0; localData < LINK_LAYER_KEY_SIZE ; localData++) {
        printf("%02x", request->data[localData]);
      }
      printf("\n");
      {
        /* Tell our border router */
        uint8_t buf[LINK_LAYER_KEY_SIZE + 2];
        buf[0] = '!';
        buf[1] = 'K';
        memcpy(&buf[2], request->data, LINK_LAYER_KEY_SIZE);
        cmd_send(buf, LINK_LAYER_KEY_SIZE + 2);
      }
      return writeReply128Tlv(request, reply, len, zeroes);
    }
#endif /* VARIABLE_RADIO_LINK_LAYER_KEY */

#ifdef VARIABLE_RADIO_LINK_LAYER_SECURITY_LEVEL
    if(request->variable == VARIABLE_RADIO_LINK_LAYER_SECURITY_LEVEL) {
      uint8_t security_level = request->data[3];
      
      if(request->data[3] > LLSEC802154_SECURITY_LEVEL_MAX) {
	printf("Invalid link layer security level : %u\n", security_level);
	return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }

      printf("radio: setting link layer security level : %u\n", security_level);

      {
        /* Tell our border router */
        uint8_t buf[3];
        buf[0] = '!';
        buf[1] = 'L';
        buf[2] = request->data[3];
        cmd_send(buf, 3);
      }
      return writeReply32Tlv(request, reply, len, zeroes);
    }
#endif /* VARIABLE_RADIO_LINK_LAYER_SECURITY_LEVEL */

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }

  if((request->opcode == TLV_OPCODE_GET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_GET_REQUEST)) {

#ifdef VARIABLE_RADIO_CHANNEL
    if(request->variable == VARIABLE_RADIO_CHANNEL) {
      radio_value_t value;
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &value) == RADIO_RESULT_OK) {
        localData = value;
        return writeReply32intTlv(request, reply, len, localData);
      }
      return writeErrorTlv(request, TLV_ERROR_HARDWARE_ERROR, reply, len);
    }
#endif /* VARIABLE_RADIO_CHANNEL */

#ifdef VARIABLE_RADIO_PAN_ID
    if(request->variable == VARIABLE_RADIO_PAN_ID) {
      radio_value_t value;
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_PAN_ID, &value) == RADIO_RESULT_OK) {
        localData = value;
        return writeReply32intTlv(request, reply, len, localData);
      }
      return writeErrorTlv(request, TLV_ERROR_HARDWARE_ERROR, reply, len);
    }
#endif /* VARIABLE_RADIO_PAN_ID */

#ifdef VARIABLE_RADIO_BEACON_RESPONSE
    if(request->variable == VARIABLE_RADIO_BEACON_RESPONSE) {
      uint8_t *beacon_payload;
      uint8_t beacon_payload_len;
      beacon_payload = handler_802154_get_beacon_payload(&beacon_payload_len);
      request->elements = beacon_payload_len / request->elementSize;
      if(beacon_payload_len % request->elementSize) {
        request->elements++;
      }
      return writeReplyVectorTlv(request, reply, len, beacon_payload);
    }
#endif /* VARIABLE_RADIO_BEACON_RESPONSE */

#ifdef VARIABLE_RADIO_MODE
    if(request->variable == VARIABLE_RADIO_MODE) {
      localData = radio_get_mode();
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_MODE */

#ifdef VARIABLE_RADIO_SCAN_MODE
    if(request->variable == VARIABLE_RADIO_SCAN_MODE) {
      localData = radio_scan_get_mode();
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_SCAN_MODE */

#ifdef VARIABLE_RADIO_SERIAL_MODE
    if(request->variable == VARIABLE_RADIO_SERIAL_MODE) {
      localData = serial_get_mode();
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_SERIAL_MODE */

#ifdef VARIABLE_RADIO_STAT_LENGTH
    if(request->variable == VARIABLE_RADIO_STAT_LENGTH) {
      localData = SERIAL_RADIO_STATS_MAX;
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_STAT_LENGTH */

#ifdef VARIABLE_RADIO_STAT_DATA
    if(request->variable == VARIABLE_RADIO_STAT_DATA) {
      if(request->opcode == TLV_OPCODE_VECTOR_GET_REQUEST) {
        if(request->offset >= SERIAL_RADIO_STATS_MAX) {
          request->elements = 0;
        } else {
          request->elements = MIN(request->elements, (SERIAL_RADIO_STATS_MAX - request->offset));
        }
        return writeReplyVectorTlv(request, reply, len, ((uint8_t *)serial_radio_stats));
      } else {
        return writeErrorTlv(request, TLV_ERROR_NO_VECTOR_ACCESS, reply, len);
      }
    }
#endif /* VARIABLE_RADIO_STAT_DATA */

#ifdef VARIABLE_RADIO_CONTROL_API_VERSION
    if(request->variable == VARIABLE_RADIO_CONTROL_API_VERSION) {
      localData = SERIAL_RADIO_CONTROL_API_VERSION;
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_CONTROL_API_VERSION */

#ifdef HAVE_RADIO_FRONTPANEL

#ifdef VARIABLE_RADIO_SUPPLY_STATUS
    if(request->variable == VARIABLE_RADIO_SUPPLY_STATUS) {
      localData = radio_frontpanel_get_supply_status();
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_SUPPLY_STATUS */

#ifdef VARIABLE_RADIO_BATTERY_SUPPLY_TIME
    if(request->variable == VARIABLE_RADIO_BATTERY_SUPPLY_TIME) {
      localData = radio_frontpanel_get_battery_supply_time();
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_BATTERY_SUPPLY_TIME */

#ifdef VARIABLE_RADIO_SUPPLY_VOLTAGE
    if(request->variable == VARIABLE_RADIO_SUPPLY_VOLTAGE) {
      localData = radio_frontpanel_get_supply_voltage();
      if(localData != 0xffffffffL) {
        return writeReply32intTlv(request, reply, len, localData);
      } else {
        return writeErrorTlv(request, TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }
#endif /* VARIABLE_RADIO_SUPPLY_VOLTAGE */

#ifdef VARIABLE_RADIO_WATCHDOG
    if(request->variable == VARIABLE_RADIO_WATCHDOG) {
      localData = radio_frontpanel_get_watchdog();
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_WATCHDOG */

#ifdef VARIABLE_RADIO_FRONTPANEL_INFO
    if(request->variable == VARIABLE_RADIO_FRONTPANEL_INFO) {
      localData = radio_frontpanel_get_info();
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_FRONTPANEL_INFO */

#ifdef VARIABLE_RADIO_FRONTPANEL_ERROR_CODE
    if(request->variable == VARIABLE_RADIO_FRONTPANEL_ERROR_CODE) {
      localData = radio_frontpanel_get_error_code();
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_FRONTPANEL_ERROR_CODE */

#endif /* HAVE_RADIO_FRONTPANEL */

#ifdef VARIABLE_RADIO_RESET_CAUSE
    if(request->variable == VARIABLE_RADIO_RESET_CAUSE) {
      localData = reset_info;
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_RESET_CAUSE */

    /* Debug variables */

#ifdef VARIABLE_RADIO_UNIT_BOOT_TIMER
    if(request->variable == VARIABLE_RADIO_UNIT_BOOT_TIMER) {
      if(request->elementSize == 8) {
        return writeReply64intTlv(request, reply, len, boottimerIEEE64());
      }
      if(request->elementSize == 4) {
        return writeReply32intTlv(request, reply, len, boottimerIEEE64in32());
      }
      return writeErrorTlv(request, TLV_ERROR_UNKNOWN_ELEMENT_SIZE, reply, len);
    }
#endif /* VARIABLE_RADIO_UNIT_BOOT_TIMER */

#ifdef VARIABLE_RADIO_STAT_DEBUG_LENGTH
    if(request->variable == VARIABLE_RADIO_STAT_DEBUG_LENGTH) {
      localData = SERIAL_RADIO_STATS_DEBUG_MAX;
      return writeReply32intTlv(request, reply, len, localData);
    }
#endif /* VARIABLE_RADIO_STAT_DEBUG_LENGTH */

#ifdef VARIABLE_RADIO_STAT_DEBUG_DATA
    if(request->variable == VARIABLE_RADIO_STAT_DEBUG_DATA) {
      if(request->opcode == TLV_OPCODE_VECTOR_GET_REQUEST) {
        if(request->offset >= SERIAL_RADIO_STATS_DEBUG_MAX) {
          request->elements = 0;
        } else {
          request->elements = MIN(request->elements, (SERIAL_RADIO_STATS_DEBUG_MAX - request->offset));
        }
        return writeReplyVectorTlv(request, reply, len, ((uint8_t *)serial_radio_stats_debug));
      } else {
        return writeErrorTlv(request, TLV_ERROR_NO_VECTOR_ACCESS, reply, len);
      }
    }
#endif /* VARIABLE_RADIO_STAT_DEBUG_DATA */

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }

  return writeErrorTlv(request, TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}
/*----------------------------------------------------------------*/

/*
 * Init called from oam_init.
 */
void
instance_radio_init(instanceControl IC[])
{
  uint8_t me = INSTANCE_RADIO;
  localIC = IC;

  IC[me].productType = 0x0090DA0303010014ULL;
  IC[me].processRequest = radio_process_request;
  strncat((char *)IC[me].label, "Radio", sizeof (IC[me].label) - 1);

  IC[me].eventArray[0] = 0;
  IC[me].eventArray[1] = 1;
  IC[me].eventArray[2] = 0;
  IC[me].eventArray[3] = 0;
  IC[me].eventArray[4] = 0;
  IC[me].eventArray[5] = 0;
  IC[me].eventArray[6] = 0;
  IC[me].eventArray[7] = 0;

  IC[me].productSubType[0] = 0x00;
  IC[me].productSubType[1] = 0x00;
  IC[me].productSubType[2] = 0x00;
  IC[me].productSubType[3] = 0x00;

  /* Do other init here. */

  if(reset_info == 0) {
    reset_info = platform_get_reset_cause();
    reset_info <<= 8;

#if HAVE_BOOT_DATA
    {
      struct boot_data *boot_data = get_boot_data();
      if(boot_data != NULL) {
        reset_info |= boot_data->extended_reset_cause & 0xff;

        /* Clear the reset cause for next reboot */
        boot_data->extended_reset_cause = EXTENDED_RESET_CAUSE_NORMAL;
      }
    }
#endif /* HAVE_BOOT_DATA */
  }
}
