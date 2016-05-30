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
 */

/**
 *
 * Button, implements product 0x0090DA030201001D.
 *
 */

#include "contiki.h"
#include "dev/button-sensor.h"
#include "api.h"
#include "oam.h"
#include "encap.h"
#include "instance-button.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifndef INSTANCE_BUTTON
#error No INSTANCE_BUTTON has been specified in this project!
#endif

#ifdef MAX_INSTANCES
#if INSTANCE_BUTTON >= MAX_INSTANCES
#error INSTANCE_BUTTON is too large. Please check definition of MAX_INSTANCES.
#endif
#else
#if INSTANCE_BUTTON >= INSTANCES
#error INSTANCE_BUTTON is too large. Please check definition of INSTANCES.
#endif
#endif

PROCESS(instance_button_process, "Button instance");

static instanceControl *localIC = NULL;

static uint32_t trigger_counter = 0;
static uint8_t trigger_type[4] = { 0, };

/**
 * Process a request TLV.
 *
 * Writes a response TLV starting at "reply", no more than "len" bytes.
 *
 * if the "request" is of type vectorAbortIfEqualRequest or
 * abortIfEqualRequest, and processing should be aborted,
 * "endProcessing" is set to OAM_END_PROCESSING_ABORT_TLV_STACK.
 *
 * Returns number of bytes written to "reply".
 */
static size_t
process_request(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing)
{
  uint32_t data32;

  if(request->opcode == TLV_OPCODE_SET_REQUEST) {
    /*
     *   #    #  #####      #     #####  ######
     *   #    #  #    #     #       #    #
     *   #    #  #    #     #       #    #####
     *   # ## #  #####      #       #    #
     *   ##  ##  #   #      #       #    #
     *   #    #  #    #     #       #    ######
     */
    if(request->variable == VARIABLE_GPIO_TRIGGER_TYPE) {
      /*
       * Trigger type
       */
      trigger_type[0] = request->data[0];
      trigger_type[1] = request->data[1];
      trigger_type[2] = request->data[2];
      trigger_type[3] = request->data[3];
      return writeReply32Tlv(request, reply, len, NULL);
    }

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  } else if(request->opcode == TLV_OPCODE_GET_REQUEST) {
    /*
     *   #####   ######    ##    #####
     *   #    #  #        #  #   #    #
     *   #    #  #####   #    #  #    #
     *   #####   #       ######  #    #
     *   #   #   #       #    #  #    #
     *   #    #  ######  #    #  #####
     */
    if(request->variable == VARIABLE_GPIO_INPUT) {
      /*
       * GPIO input
       */
      data32 = button_sensor.value(0);
      return writeReply32intTlv(request, reply, len, data32);
    } else if(request->variable == VARIABLE_GPIO_TRIGGER_COUNTER) {
      /*
       * Trigger counter
       */
      data32 = trigger_counter;
      return writeReply32intTlv(request, reply, len, data32);
    } else if(request->variable == VARIABLE_GPIO_TRIGGER_TYPE) {
      /*
       * Trigger type
       */
      return writeReply32Tlv(request, reply, len, trigger_type);
    }

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  } else {
    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
  }
}
/*----------------------------------------------------------------*/
/**
 * Process a poll request.
 * Write a reply TLV to "reply", no more than "len" bytes.
 * Return number of bytes written to "reply".
 */
static size_t
poll(uint8_t instance, uint8_t *reply, size_t len, oam_poll_type poll_type)
{
  int replyLen = 0;
  TLV T;

  if(poll_type == OAM_POLL_TYPE_EVENT) {
    T.version = TLV_VERSION;
    T.instance = instance;
    T.opcode = TLV_OPCODE_GET_RESPONSE;
    T.error = TLV_ERROR_NO_ERROR;
    T.elementSize = 4;
    T.variable = VARIABLE_GPIO_TRIGGER_COUNTER;

    replyLen += writeReply32intTlv(&T, &reply[replyLen], len - replyLen, trigger_counter);
  }

  return replyLen;
}
/*----------------------------------------------------------------*/
/*
 * Init called from oam_init.
 */
void
instance_button_init(instanceControl IC[])
{
  localIC = IC;

  IC[INSTANCE_BUTTON].eventArray[1] = 1;

  IC[INSTANCE_BUTTON].processRequest = process_request;
  IC[INSTANCE_BUTTON].poll = poll;
  IC[INSTANCE_BUTTON].productType = 0x0090DA030201001DULL; /* button */
  strncat((char *)IC[INSTANCE_BUTTON].label, "Button", 32 - 1);

  process_start(&instance_button_process, NULL);
}
/*----------------------------------------------------------------*/
uint32_t
instance_button_get()
{
  return button_sensor.value(0);
}
/*----------------------------------------------------------------*/
uint32_t
instance_button_get_counter()
{
  return trigger_counter;
}
/*----------------------------------------------------------------*/
PROCESS_THREAD(instance_button_process, ev, data)
{
  PROCESS_BEGIN();

  SENSORS_ACTIVATE(button_sensor);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    trigger_counter++;
    PRINTF("button: pressed %lu times\n", trigger_counter);

    /* If event is not armed, it did not happen */
    if(localIC[INSTANCE_BUTTON].eventArray[7] == EVENTSTATE_EVENT_ARMED) {
      trig_event(INSTANCE_BUTTON);
    } else {
      PRINTF("button: event not armed (%d)\n", localIC[INSTANCE_BUTTON].eventArray[7]);
    }
  }

  PROCESS_END();
}
/*----------------------------------------------------------------*/
