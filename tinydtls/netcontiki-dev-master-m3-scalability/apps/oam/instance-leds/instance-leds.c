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
 * Leds, implements product 0x0090DA030201001E.
 *
 */

#include "contiki.h"
#include "dev/leds.h"
#include "api.h"
#include "oam.h"
#include "encap.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifndef INSTANCE_LEDS
#error No INSTANCE_LEDS has been specified in this project!
#endif

#ifdef MAX_INSTANCES
#if INSTANCE_LEDS >= MAX_INSTANCES
#error INSTANCE_LEDS is too large. Please check definition of MAX_INSTANCES.
#endif
#else
#if INSTANCE_LEDS >= INSTANCES
#error INSTANCE_LEDS is too large. Please check definition of INSTANCES.
#endif
#endif

static instanceControl *localIC = NULL;

#ifdef INSTANCE_LEDS_CONF_NUMBER_OF_LEDS
#define LEDS_COUNT() INSTANCE_LEDS_CONF_NUMBER_OF_LEDS
#else
/* Count the number of set bits in LEDS_ALL if no configuraiton is set */
static int
get_number_of_leds(void)
{
  int c;

  for(c = 0; c < 32; c++) {
    if(((1 << c) & LEDS_ALL) == 0) {
      break;
    }
  }

  return c;
}
#define LEDS_COUNT get_number_of_leds
#endif /* INSTANCE_LEDS_NUMBER_OF_LEDS */
/*----------------------------------------------------------------*/
/**
 * Process a request TLV.
 *
 * Writes a response TLV starting at "reply", no more than "len" bytes.
 *
 * If the "request" is of type vectorAbortIfEqualRequest or
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
    if(request->variable == VARIABLE_LED_CONTROL) {
      leds_set(request->data[3] & LEDS_ALL);
      return writeReply32Tlv(request, reply, len, NULL);
    }

#ifdef VARIABLE_LED_SET
    if(request->variable == VARIABLE_LED_SET) {
      leds_on(request->data[3] & LEDS_ALL);
      return writeReply32Tlv(request, reply, len, NULL);
    }
#endif /* VARIABLE_LED_SET */

#ifdef VARIABLE_LED_CLEAR
    if(request->variable == VARIABLE_LED_CLEAR) {
      leds_off(request->data[3] & LEDS_ALL);
      return writeReply32Tlv(request, reply, len, NULL);
    }
#endif /* VARIABLE_LED_CLEAR */

#ifdef VARIABLE_LED_TOGGLE
    if(request->variable == VARIABLE_LED_TOGGLE) {
      leds_toggle(request->data[3] & LEDS_ALL);
      return writeReply32Tlv(request, reply, len, NULL);
    }
#endif /* VARIABLE_LED_TOGGLE */

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
    if(request->variable == VARIABLE_NUMBER_OF_LEDS) {
      data32 = LEDS_COUNT();
      return writeReply32intTlv(request, reply, len, data32);

    } else if(request->variable == VARIABLE_LED_CONTROL) {
      data32 = leds_get() & LEDS_ALL;
      return writeReply32intTlv(request, reply, len, data32);

    }

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  } else {
    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
  }
}
/*----------------------------------------------------------------*/
/*
 * Init called from oam_init.
 */
void
instance_leds_init(instanceControl IC[])
{
  localIC = IC;

  IC[INSTANCE_LEDS].processRequest = process_request;
  IC[INSTANCE_LEDS].productType = 0x0090DA030201001EULL; /* leds */
  strncat((char *)IC[INSTANCE_LEDS].label, "LEDs", 32 - 1);
}
/*----------------------------------------------------------------*/
