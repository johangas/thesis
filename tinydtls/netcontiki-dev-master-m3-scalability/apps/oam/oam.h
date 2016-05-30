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

#ifndef OAM_H_
#define OAM_H_

#include "contiki.h"
#include "vargen.h"
#include <string.h>
#include <stdio.h>
#include "net/ip/uip.h"

PROCESS_NAME(oam);

#ifndef HAVE_INSTANCE_SLEEP
PROCESS_NAME(periodic_response_process);
#endif /* HAVE_INSTANCE_SLEEP */

#ifndef INSTANCES
#error Define INSTANCES in project-conf.h or similar non-common place.
#endif /* INSTANCES */

#ifndef OAM_PORT
#define OAM_PORT 49111
#endif /* OAM_PORT */

#define DEBUG_PRINT_(...) printf(__VA_ARGS__)
#define DEBUG_PRINT_I(...) printf(__VA_ARGS__)
#define DEBUG_PRINT_II(...) printf(__VA_ARGS__)

#ifdef OAM_CONF_NON_SLEEP_PERIODIC_INTERVAL
#define OAM_NON_SLEEP_PERIODIC_INTERVAL OAM_CONF_NON_SLEEP_PERIODIC_INTERVAL
#else
#define OAM_NON_SLEEP_PERIODIC_INTERVAL 60000
#endif /* OAM_CONF_NON_SLEEP_PERIODIC_INTERVAL */

#ifdef OAM_CONF_NON_SLEEP_REPAIR_DELAY
#define OAM_NON_SLEEP_REPAIR_DELAY OAM_CONF_NON_SLEEP_REPAIR_DELAY
#else
#define OAM_NON_SLEEP_REPAIR_DELAY 123000
#endif /* OAM_CONF_NON_SLEEP_REPAIR_DELAY */

#ifdef OAM_CONF_MIN_DIS_INTERVAL_MS
#define OAM_MIN_DIS_INTERVAL_MS OAM_CONF_MIN_DIS_INTERVAL_MS
#else
#define OAM_MIN_DIS_INTERVAL_MS 17000 /* 17 seconds ok? */
#endif /* OAM_CONF_MIN_DIS_INTERVAL_MS */

#define OAM_CONTROL_QUEUE_DEPTH  16
#define OAM_CONTROL_PORTAL_TX_START   2
#define OAM_CONTROL_PORTAL_TX_STOP    3
#define OAM_CONTROL_EVENT             4
#define OAM_CONTROL_PORTAL_TX_START_NOW 5

#define UNIT_CONTROLLER_MODE_FREE 0
#define UNIT_CONTROLLER_MODE_OWNED 1
#define UNIT_CONTROLLER_IMPLEMENTED_WATCHDOG_BITS 22
#define UNIT_CONTROLLER_ADDRESS_TYPES_IPV4 0x01
#define UNIT_CONTROLLER_ADDRESS_TYPES_IPV6 0x02
/* This implementation handles IPv4/UDP and IPv6/UDP */
#define UNIT_CONTROLLER_IMPLEMENTED_ADDRESS_TYPES UNIT_CONTROLLER_ADDRESS_TYPES_IPV4 | UNIT_CONTROLLER_ADDRESS_TYPES_IPV6
#define UNIT_CONTROLLER_CRYPTO_AES128 0x01
#define UNIT_CONTROLLER_IMPLEMENTED_CRYPTO_METHODS 0x00 /* no crypto yet */

typedef enum {
  EVENT_BACKOFF_TIMER_START,
  EVENT_BACKOFF_TIMER_STOP,
  EVENT_BACKOFF_TIMER_ARE_WE_THERE_YET,
  EVENT_BACKOFF_TIMER_IS_ACTIVE,
  EVENT_BACKOFF_TIMER_GET_NEXT_TX_TIME,
} event_backoff_timer_cmd;

#define UC_SAME_STACK_NEW  0x00
#define UC_SAME_STACK_TYPE 0x01
#define UC_SAME_STACK_PORT 0x02
#define UC_SAME_STACK_ADDR 0x04
#define UC_SAME_STACK_CHECK 0xff

void ucComplete(uint8_t happening);

struct connection {
  char *address;
  uint16_t port;
};

typedef enum {
  OAM_ACTION_NEW_PDU,
  OAM_ACTION_UC_WD_EXPIRE,
  OAM_ACTION_SLEEP_WAKE,
  OAM_ACTION_REPLY_SENT,
  OAM_ACTION_END_PDU,
} oam_action;

typedef enum {
  OAM_ACTION_SLEEP_WAKE_SLEEP,
  OAM_ACTION_SLEEP_WAKE_WAKE,
} oam_action_sleep_wake;

#define OAM_END_PROCESSING_NEW 0x00
#define OAM_END_PROCESSING_ABORT_TLV_STACK 0x01
#define OAM_END_PROCESSING_DO_NOT_REPLY 0x02
#define OAM_END_PROCESSING_ENCRYPT_KEY_1 0x04
#define OAM_END_PROCESSING_ENCRYPT_KEY_3 0x08

typedef uint8_t oam_end_processing;

typedef enum {
  OAM_POLL_TYPE_POLL,
  OAM_POLL_TYPE_EVENT,
} oam_poll_type;

typedef struct {
  uint64_t productType;
  size_t (*processRequest)(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing);
  size_t (*poll)(uint8_t instance, uint8_t *reply, size_t len, oam_poll_type type);
  uint8_t (*oam_do_action)(oam_action o, uint8_t operation);
  uint8_t label[32];
  uint8_t productID[16];
  uint8_t eventArray[8]; /* 2 * 32bit, as byte array. keep it in network byte order. */
  uint8_t productSubType[4];
  uint8_t instances;
  uint8_t process; /* unused, external, ... */
  uint8_t flags; /* newPDU.. */
} instanceControl;

typedef struct oam_do_action_handle {
  struct oam_do_action_handle *next;
  uint8_t (*oam_do_action)(oam_action o, uint8_t operation);
} oam_do_action_handle;

#define NEW_TLV_PDU_OPERATION_NEW      0 /* resets state, called when new PDU start, returns FALSE */
#define NEW_TLV_PDU_OPERATION_USE      1 /* increments use count, and returns TRUE if ths was first use since reset. */

#define INSTANCE_PROCESS_NORMAL        0
#define INSTANCE_PROCESS_EXTERNAL      1
#define INSTANCE_PROCESS_SILENT_IGNORE 2

typedef enum {
  OAM_ACTIVITY_TYPE_ANY,
  OAM_ACTIVITY_TYPE_TX,
  OAM_ACTIVITY_TYPE_RX,
  OAM_ACTIVITY_TYPE_RX_FROM_UC,
} oam_activity_type;

void oam_init(void (*initfunc[])(instanceControl[]));
void oam_event_from_ISR(uint8_t who);

/*
 * tools
 */
void print_tlv(TLV *t);
size_t tlvLength(uint8_t *p);
uint32_t get_int32_from_data(uint8_t *p);
void write_int32_to_buf(uint8_t *p, uint32_t d);
uint64_t get_int64_from_data(uint8_t *p);
void write_int64_to_buf(uint8_t *p, uint64_t d);
uint8_t *byteorderize32void(uint32_t (*f)(void));
uint8_t *byteorderize32uint8(uint32_t (*f)(uint8_t), uint8_t arg);
size_t tlv_from_bytes(TLV *t, const uint8_t *data);
size_t tlv_to_bytes(TLV *t, uint8_t *data, size_t len);
size_t writeErrorTlv(TLV *request, uint8_t error, uint8_t *reply, size_t len);
size_t writeReply32Tlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data);
size_t writeReply64Tlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data);
size_t writeReply128Tlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data);
size_t writeReply256Tlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data);
size_t writeReply512Tlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data);
size_t writeReplyBlobTlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data, size_t dataLen);
size_t writeReply32intTlv(TLV *request, uint8_t *reply, size_t len, uint32_t data);
size_t writeReply64intTlv(TLV *request, uint8_t *reply, size_t len, uint64_t data);

void replyify(TLV *t);
TLVerror checkTLV(TLV *t, int instances, instanceControl IC[], uint8_t PSP);
size_t writeReplyVectorTlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data);
void restart_event_backoff_timer();
uint64_t event_backoff_timer(event_backoff_timer_cmd cmd);
size_t generate_event_reply(instanceControl IC[], uint8_t *reply, size_t len);
void update_event_arrays(instanceControl IC[]);
int16_t verifyAndDecryptMessage(uint8_t *p, size_t len, uint8_t *payloadType);
uint8_t unitControllerSetAddress(uint8_t *p, uint8_t justAddress);
uint8_t *unitControllerGetAddress();
uint32_t unitControllerGetWatchdog(uint8_t *p);
uint8_t unitControllerSetWatchdog(uint8_t *p, uint32_t wd);
void stackBegin();
void stackEnd();
void unitControllerGetStatus(uint8_t *p);
void getCommunicationStyle(uint8_t *p);
uint8_t is_uc_address(uint8_t *a, size_t address_len);

uint8_t doEventAction(uint8_t *state, uint8_t action);
uint8_t isDsicoveryVariable(uint16_t variable);
size_t processDiscoveryRequest(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing, instanceControl *localIC);
uint8_t is_discovery_variable(TLV *t);
uint8_t is_special_variable(TLV *t);

void oam_send_packet(uip_ipaddr_t destination, uint16_t port, uint8_t *data, int len);

uint32_t get_location_id();
uint8_t set_location_id(uint32_t id);
uint32_t get_owner_id();
uint8_t set_owner_id(uint32_t id);
uint8_t oam_echo_replies(uint8_t increment);
instanceControl *get_instances();
void trig_event(uint8_t instance);
void oam_tx_activity();
uint64_t oam_last_activity(oam_activity_type type);
uint32_t oam_get_time_since_last_good_rx_from_uc();
void instance0_oam_do_action_add(oam_do_action_handle *new_handle);
void instance0_oam_do_action_remove(oam_do_action_handle *old_handle);
void reply_sent(uint8_t sent_or_skipped);
uint32_t get_awake_time();
void set_get_awake_time_function(uint32_t (*func)(void));
void set_watchdog_address_set_function(void (*func)(void));
uint64_t get_last_wd_expire();

void schedule_wakeup_notification();
int oam_network_repair(uint8_t prefer_unicast);

#include "tlv.h"

#endif /* OAM_H_ */
