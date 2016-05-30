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

#include "contiki-conf.h"
#define __VARGEN_CODE__
#include "vargen.h"
#include "stdint.h"
#include "oam.h"
#include "api.h"

extern uint8_t zeroes[16];

void
print_tlv(TLV *t) {
    DEBUG_PRINT_I("oam: Version     = %d\n", t->version);
    DEBUG_PRINT_I("oam: Length      = %u\n", t->length);
    DEBUG_PRINT_I("oam: Variable    = 0x%03x\n", t->variable);
    DEBUG_PRINT_I("oam: Instance    = %d\n", t->instance);
    DEBUG_PRINT_I("oam: Opcode      = %d\n", t->opcode);
    DEBUG_PRINT_I("oam: ElementSize = %d\n", t->elementSize);
    DEBUG_PRINT_I("oam: Error       = %d\n", t->error);
    if (t->opcode & TLV_OPCODE_VECTOR_MASK) {
        DEBUG_PRINT_I("oam: Offset      = %lu\n", (long unsigned)t->offset);
        DEBUG_PRINT_I("oam: Elements    = %lu\n", (long unsigned)t->elements);
    }
}

/*
 * Return length of TLV pointed to by "p", in number of bytes.
 */
size_t
tlvLength(uint8_t *p) {
    return (((p[0] & 0xf) << 8) + p[1]) * 4;
}

/*
 * Combine 4 bytes of data from an array to a host byte order int 32.
 */
uint32_t
get_int32_from_data(uint8_t *p) {
  return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
}

/*
 * Write 32 bit host byte order value to byte array in network byte order.
 */
void
write_int32_to_buf(uint8_t *p, uint32_t d) {
  p[0] = (d >> 24) & 0xff;
  p[1] = (d >> 16) & 0xff;
  p[2] = (d >> 8) & 0xff;
  p[3] = d & 0xff;
}

/*
 * Combine 8 bytes of data from an array to a host byte order int 64.
 */
uint64_t
get_int64_from_data(uint8_t *p) {
  uint64_t retval = 0;
  int i;
  for (i = 0 ; i < 8; i++) {
    retval |= (p[i] << ((7 - i)*8));
  }
  return retval;
}

/*
 * Write 64 bit host byte order value to byte array in network byte order.
 */
void
write_int64_to_buf(uint8_t *p, uint64_t d) {
  int i;
  for (i = 0; i < 8; i++) {
    p[i] = (d >> ((7 - i)*8)) & 0xff;
  }
}

/*
 * Return an array that contains the network byte order representation
 * of an uint32_t supplied by the funcion "f" that takes no argument.
 */
uint8_t *
byteorderize32void(uint32_t (*f)(void)) {
    static uint8_t data[4];
    uint32_t d;
    d = f();
    data[0] = (d >> 24) & 0xff;
    data[1] = (d >> 16) & 0xff;
    data[2] = (d >> 8) & 0xff;
    data[3] = d & 0xff;
    
    return (uint8_t *)&data;
}

/*
 * Return an array that contains the network byte order representation
 * of an uint32_t supplied by the funcion "f" that takes one argument
 * uint8_t.
 */
uint8_t *
byteorderize32uint8(uint32_t (*f)(uint8_t), uint8_t arg) {
    static uint8_t data[4];
    uint32_t d;
    d = f(arg);
    data[0] = (d >> 24) & 0xff;
    data[1] = (d >> 16) & 0xff;
    data[2] = (d >> 8) & 0xff;
    data[3] = d & 0xff;
    
    return data;
}

/*
 * Parse a TLV in a byte array.
 * Note that lenght and elementsize fields are converted to indicate length in bytes.
 * Data (if any) is not copied, tlv->data is set to point to it.
 * Return number of bytes consumed by this TLV.
 */
size_t
tlv_from_bytes(TLV *t, const uint8_t *in_data) {
    t->version = in_data[0] >> 4;
    t->length = (((in_data[0] & 0xf) << 8) + in_data[1]) * 4;
    t->variable = (in_data[2] << 8) + in_data[3];
    t->instance = in_data[4];
    t->opcode = in_data[5];
    t->elementSize = 1 << (in_data[6] + 2);
    t->error = in_data[7];

    if (t->opcode & TLV_OPCODE_VECTOR_MASK) {
        t->offset = (in_data[8] << 24) + (in_data[9] << 16) + (in_data[10] << 8) + in_data[11];
        t->elements = (in_data[12] << 24) + (in_data[13] << 16) + (in_data[14] << 8) + in_data[15];
        t->data = (uint8_t *)&in_data[16];
    } else {
        t->offset = 0;
        t->elements = 1;
        t->data = (uint8_t *)in_data + 8;
    }
    if (!withData(t->opcode)) {
        t->data = NULL;
    }
    return t->length;
}

/*
 * Write a TLV, including copy the data (if any), to a destination
 * buffer, no more than "len" bytes.
 *
 * Return number of bytes written.
 */
size_t
tlv_to_bytes(TLV *t, uint8_t *data, size_t len) {
    size_t retval = 8;
    size_t dataSize = 0;
    uint8_t *dataStart = NULL;
    uint8_t i;
    if (t->opcode & TLV_OPCODE_VECTOR_MASK) {
        retval += 8;
    }
    if (withData(t->opcode)) {
        if (t->opcode & TLV_OPCODE_VECTOR_MASK) {
            dataSize = t->elements * t->elementSize;
            dataStart = data + 16;
        } else {
            dataSize = t->elementSize;
            dataStart = data + 8;
        }
    } else {
        dataSize = 0;
        dataStart = NULL;
    }
    retval += dataSize;
     
    // no room
    if (retval > len) {
        return 0;
    }
    //retval contains length in bytes so shifts are +2 to convert to 32bit words.
    data[0] = ((t->version & 0x0f) << 4) + ((retval >> 10) & 0xff);
    data[1] = (retval >> 2) & 0xff;
    data[2] = t->variable >> 8;
    data[3] = t->variable & 0xff;
    data[4] = t->instance;
    data[5] = t->opcode;
    for (i = 0; t->elementSize > (1 << (i + 2)); i++);
    data[6] = i;
    data[7] = t->error;
    if (t->opcode & TLV_OPCODE_VECTOR_MASK) {
        data[8] = (t->offset >> 24) & 0xff;
        data[9] = (t->offset >> 16) & 0xff;
        data[10] = (t->offset >> 8) & 0xff;
        data[11] = t->offset & 0xff;
        data[12] = (t->elements >> 24) & 0xff;
        data[13] = (t->elements >> 16) & 0xff;
        data[14] = (t->elements >> 8) & 0xff;
        data[15] = t->elements & 0xff;
    }

    /*
     * If there is data to copy and the source for data is zero, clear
     * destiantiom, else copy data if it's not already in place.
     */
    if ((dataStart != NULL) && (dataSize > 0)) {
      if (t->data == 0) {
        memset(dataStart, 0, dataSize);
      } else if (dataStart != t->data) {
        memcpy(dataStart, t->data, dataSize);
      }
    }
    return retval;
}

/*
 * Construct an error TLV with the given error, without data. All
 * other fields are taken from "request".
 * Error TLV is written to "reply", no more than "len" bytes.
 * Return number of bytes written.
 */
size_t
writeErrorTlv(TLV *request, uint8_t error, uint8_t *reply, size_t len) {
    TLV T;
    T.version = request->version;
    if (request->opcode & TLV_OPCODE_VECTOR_MASK) {
        T.length = 16;
        T.elements = request->elements;
    } else {
        T.length = 8;
    }
    T.variable = request->variable;
    T.instance = request->instance;
    T.elementSize = request->elementSize;
    T.offset = request->offset;
    T.error = error;
    T.opcode = request->opcode;
    T.data = 0;
    replyify(&T);
    return tlv_to_bytes(&T, reply, len);
}

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more than "len" bytes".
 * The payload data is read from "data", and should already be in network byte order.
 */
size_t
writeReply32Tlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data) {
    TLV T;

    if (request->opcode & TLV_OPCODE_VECTOR_MASK) {
        return 0;
    }
    T.version = request->version;    
    T.length = 12; // single 32bit reply
    T.variable = request->variable;
    T.instance = request->instance;
    T.elementSize = 4;
    T.offset = request->offset;
    T.error = TLV_ERROR_NO_ERROR;
    T.opcode = request->opcode;
    replyify(&T);
    T.data = data;
    return tlv_to_bytes(&T, reply, len);
}

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more than "len" bytes".
 * The payload data is read from "data", and is converted to network byte order.
 */
size_t
writeReply32intTlv(TLV *request, uint8_t *reply, size_t len, uint32_t data) {
    TLV T;
    uint8_t p[4];

    if (request->opcode & TLV_OPCODE_VECTOR_MASK) {
        return 0;
    }
    T.version = request->version;    
    T.length = 12; // single 32bit reply
    T.variable = request->variable;
    T.instance = request->instance;
    T.elementSize = 4;
    T.offset = request->offset;
    T.error = TLV_ERROR_NO_ERROR;
    T.opcode = request->opcode;
    replyify(&T);
    T.data = p;
    p[0] = (data >> 24) & 0xff;
    p[1] = (data >> 16) & 0xff;
    p[2] = (data >> 8) & 0xff;
    p[3] = data & 0xff;
    return tlv_to_bytes(&T, reply, len);
}

/*
 * Write a vector reply TLV based on "request" to "reply", no more than "len" bytes".
 * The payload data is read from "data" with offset and elements from request.
 * No byteorder conversion is done.
 */
size_t
writeReplyVectorTlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data) {
    TLV T;

    if (!(request->opcode & TLV_OPCODE_VECTOR_MASK)) {
        DEBUG_PRINT_("writeReplyVectorTlv: not a vector\n");
        return 0;
    }
    T.version = request->version;    
    T.elements = request->elements;
    T.elementSize = request->elementSize;
    T.length = 16 + (T.elements * T.elementSize);
    T.variable = request->variable;
    T.instance = request->instance;
    T.offset = request->offset;
    T.error = TLV_ERROR_NO_ERROR;
    T.opcode = request->opcode;
    replyify(&T);
    if(data == NULL) {
      T.data = NULL;
    } else {
      T.data = data + (T.elementSize * T.offset);
    }
    return tlv_to_bytes(&T, reply, len);
}


/*
 * Write a non-vector Blob reply TLV based on "request" to "reply", no
 * more than "len" bytes".
 *
 * The payload data is read from "data", "dataLen" number of bytes,
 * and should already be in network byte order. Data should already be
 * padded with zero to n*4 bytes.
 */
size_t
writeReplyBlobTlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data, size_t dataLen) {
    TLV T;
    if ((request->opcode & TLV_OPCODE_VECTOR_MASK) == 0) {
        return 0;
    }
    T.version = request->version;
    T.length = 16 + dataLen;
    T.variable = request->variable;
    T.instance = request->instance;
    T.elementSize = 4;
    T.offset = 0;
    T.elements = dataLen / 4;
    T.error = TLV_ERROR_NO_ERROR;
    T.opcode = request->opcode;
    replyify(&T);
    T.data = data;
    return tlv_to_bytes(&T, reply, len);
}

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more than "len" bytes".
 * The payload data is read from "data", and should already be in network byte order.
 */
size_t
writeReply64Tlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data) {
    TLV T;

    if (request->opcode & TLV_OPCODE_VECTOR_MASK) {
        return 0;
    }
    T.version = request->version;
    T.length = 16; // single 64bit reply
    T.variable = request->variable;
    T.instance = request->instance;
    T.elementSize = 8;
    T.offset = request->offset;
    T.error = TLV_ERROR_NO_ERROR;
    T.opcode = request->opcode;
    replyify(&T);
    T.data = data;
    return tlv_to_bytes(&T, reply, len);
}

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more than "len" bytes",
 * with payload "data". Data is converted to network byte order.
 */
size_t
writeReply64intTlv(TLV *request, uint8_t *reply, size_t len, uint64_t data) {
    TLV T;
    uint8_t p[8];

    if (request->opcode & TLV_OPCODE_VECTOR_MASK) {
        return 0;
    }
    T.version = request->version;    
    T.length = 16; // single 64bit reply
    T.variable = request->variable;
    T.instance = request->instance;
    T.elementSize = 8;
    T.offset = request->offset;
    T.error = TLV_ERROR_NO_ERROR;
    T.opcode = request->opcode;
    replyify(&T);
    T.data = p;

    p[0] = (data >> 56) & 0xff;
    p[1] = (data >> 48) & 0xff;
    p[2] = (data >> 40) & 0xff;
    p[3] = (data >> 32) & 0xff;

    p[4] = (data >> 24) & 0xff;
    p[5] = (data >> 16) & 0xff;
    p[6] = (data >> 8) & 0xff;
    p[7] = data & 0xff;

    
    return tlv_to_bytes(&T, reply, len);
}

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more than "len" bytes".
 * The payload data is read from "data", and should already be in network byte order.
 */
size_t
writeReply128Tlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data) {
    TLV T;

    if (request->opcode & TLV_OPCODE_VECTOR_MASK) {
        return 0;
    }
    T.version = request->version;    
    T.length = 24; // single 128bit reply
    T.variable = request->variable;
    T.instance = request->instance;
    T.elementSize = 16;
    T.offset = request->offset;
    T.error = TLV_ERROR_NO_ERROR;
    T.opcode = request->opcode;
    replyify(&T);
    T.data = data;
    
    return tlv_to_bytes(&T, reply, len);
}

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more than "len" bytes".
 * The payload data is read from "data", and should already be in network byte order.
 */
size_t
writeReply256Tlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data) {
    TLV T;

    if (request->opcode & TLV_OPCODE_VECTOR_MASK) {
        return 0;
    }
    T.version = request->version;    
    T.length = 40; // single 32byte reply
    T.variable = request->variable;
    T.instance = request->instance;
    T.elementSize = 32;
    T.offset = 0;
    T.error = TLV_ERROR_NO_ERROR;
    T.opcode = request->opcode;
    replyify(&T);
    T.data = data;
    
    return tlv_to_bytes(&T, reply, len);
}

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more than "len" bytes".
 * The payload data is read from "data", and should already be in network byte order.
 */
size_t
writeReply512Tlv(TLV *request, uint8_t *reply, size_t len, uint8_t *data) {
    TLV T;

    if (request->opcode & TLV_OPCODE_VECTOR_MASK) {
        return 0;
    }
    T.version = request->version;
    T.length = 8 + 64;
    T.variable = request->variable;
    T.instance = request->instance;
    T.elementSize = 64;
    T.offset = 0;
    T.error = TLV_ERROR_NO_ERROR;
    T.opcode = request->opcode;
    replyify(&T);
    T.data = data;

    return tlv_to_bytes(&T, reply, len);
}

/*
 * Set reply bit on opcode in TLV pointed to by "t".
 */
void
replyify(TLV *t) {
    t->opcode = t->opcode | TLV_OPCODE_REPLY_MASK;
}

/*
 * Verify that this TLV correct and allowed.
 */
TLVerror
checkTLV(TLV *t, int instances, instanceControl IC[], uint8_t PSP) {
    int i;
    int expectedLength;
    uint64_t productType;
    const variable *v = NULL;

    // check version
    if (t->version != TLV_VERSION) {
        return TLV_ERROR_UNKNOWN_VERSION;
    }
    // Check instance
    if (t->instance >= instances) {
        return TLV_ERROR_UNKNOWN_INSTANCE;
    }

    // Verify that is is a request, if it isn't a PSP
    if (!PSP) {
        if (t->opcode & TLV_OPCODE_REPLY_MASK) {
            return TLV_ERROR_UNPROCESSED_TLV; // silently skip responses
        }
    }

    productType = IC[t->instance].productType;
    for (i = 0; i < sizeof (allVariables) / sizeof (variable); i++) {
        if ((allVariables[i].number == t->variable) && (allVariables[i].productType == productType)) { 
            v = &allVariables[i];
            break;
        }
    }

    if (!v) {
        return TLV_ERROR_UNKNOWN_VARIABLE;
    }

    // Vector request checks
    if (t->opcode & TLV_OPCODE_VECTOR_MASK) {
        /* No offset or length verification necessary on blobs */
        if (t->opcode != TLV_OPCODE_VECTOR_BLOB_REQUEST) {
            if (v->vectorDepth < 1) {
                DEBUG_PRINT_("not-blob vector is no vector\n");
                print_tlv(t);
               return TLV_ERROR_NO_VECTOR_ACCESS;
            }
            if (v->vectorDepth != VECTOR_DEPTH_DONT_CHECK) {
                if (t->offset > (v->vectorDepth -1)) {
                    return TLV_ERROR_INVALID_VECTOR_OFFSET;
                }
            }
        }
    } else {
        /* Blob MUST be vector */
        if (t->opcode == TLV_OPCODE_BLOB_REQUEST) {
            DEBUG_PRINT_("Blob is no vector\n");
            return TLV_ERROR_NO_VECTOR_ACCESS;
        }
    }

    /*
     * Verify element size. All vaiables with format TIME is 64bit,
     * enforced by vargen, also allow it to be accessed as 32bit.
     */
    if (v->elementSize != t->elementSize) {
#ifndef HAVE_FORMAT_TIME
      return TLV_ERROR_UNKNOWN_ELEMENT_SIZE;
#else  /* HAVE_FORMAT_TIME */
      if (v->format != FORMAT_TIME) {
        return TLV_ERROR_UNKNOWN_ELEMENT_SIZE;
      }
      if (v->elementSize != 4) {
        return TLV_ERROR_UNKNOWN_ELEMENT_SIZE;
      }
#endif /* HAVE_FORMAT_TIME */
    }




    // Verify element size
    if (v->elementSize != t->elementSize) {
        return TLV_ERROR_UNKNOWN_ELEMENT_SIZE;
    }

    // Verify length
    if (t->opcode & TLV_OPCODE_VECTOR_MASK) {
        expectedLength = 16;
    } else {
        expectedLength = 8;
    }
    if (withData(t->opcode)) {
        if (t->opcode & TLV_OPCODE_VECTOR_MASK) {
            expectedLength += (t->elementSize * t->elements);
        } else {
            expectedLength += t->elementSize;
        }
    }

    if (expectedLength != t->length) {
        return TLV_ERROR_BAD_LENGTH;
    }

    // write access
    if ((v->writability == WRITABILITY_RO) && (
            (t->opcode == TLV_OPCODE_SET_REQUEST) ||
            (t->opcode == TLV_OPCODE_MASKED_SET_REQUEST) ||
            (t->opcode == TLV_OPCODE_VECTOR_SET_REQUEST) ||
            (t->opcode == TLV_OPCODE_VECTOR_MASKED_SET_REQUEST))) {
        return TLV_ERROR_WRITE_ACCESS_DENIED;
    }

    return TLV_ERROR_NO_ERROR;
}

/*
 * Called from interrupt when a new event happens.
 *
 * Return non-zero on command EVENT_BACKOFF_TIMER_ARE_WE_THERE_YET if
 * it is time to transmit event.
 *
 * Return boottimer time when next event is due when called with
 * command EVENT_BACKOFF_TIMER_GET_NEXT_TX_TIME, och zero if timee is
 * not active.
 */
#define START_INCREMENT 250;
uint64_t
event_backoff_timer(event_backoff_timer_cmd cmd) {
    static uint64_t next_event_tx = 0;
    static uint32_t increment;

    switch (cmd) {
    case EVENT_BACKOFF_TIMER_START:
      next_event_tx = boottimer_read();
      increment = START_INCREMENT;
      process_poll(&oam);
      break;
    case EVENT_BACKOFF_TIMER_STOP:
      next_event_tx = 0;
      break;
    case EVENT_BACKOFF_TIMER_ARE_WE_THERE_YET:
      if ((next_event_tx > 0) && (boottimer_read() >= next_event_tx)) {
        next_event_tx += increment;
        if (increment < 32767) {
          increment *= 2;
        } else {
#ifdef STOP_EVENT_TRANSMIT_AT_MAXIMUM_INCREMENT
          next_event_tx = 0;
#endif /* STOP_EVENT_TRANSMIT_AT_MAXIMUM_INCREMENT */
        }
        return 1;
      }
      break;
    case EVENT_BACKOFF_TIMER_IS_ACTIVE:
      if (next_event_tx > 0) {
        return TRUE;
      } else {
        return FALSE;
      }
      break;
    case EVENT_BACKOFF_TIMER_GET_NEXT_TX_TIME:
      return next_event_tx;
      break;
    }
    return 0;
}
/*----------------------------------------------------------------*/

/*
 * Trig event. Typically called from interrupt to set event state to
 * trigged, and start event transmission if events are enabled in
 * instance 0.
 */
void
trig_event(uint8_t instance)
{
  instanceControl *localIC = get_instances();

  if (localIC == NULL) {
    return;
  }
  if (instance >= INSTANCES) {
    return;
  }
  if (localIC[instance].eventArray[7] == EVENTSTATE_EVENT_ARMED) {
    /* changed from armed to trigged */
    localIC[instance].eventArray[7] = EVENTSTATE_EVENT_TRIGGED;
    localIC[instance].eventArray[3] |= EVENTSTATE_OFFSET0_TRIGGED_MASK;
    localIC[0].eventArray[3] |= EVENTSTATE_OFFSET0_TRIGGED_MASK;

    /* If both this instance and instance zero have events enabled then reset backoff timer */
    if ((localIC[instance].eventArray[3] & EVENTSTATE_OFFSET0_ENABLE_MASK) &&
        (localIC[0].eventArray[3] & EVENTSTATE_OFFSET0_ENABLE_MASK)) {
      event_backoff_timer(EVENT_BACKOFF_TIMER_START);
    }
  }
}
/*----------------------------------------------------------------*/

/*
 * Called whenever an event is acknowledged.
 */
void
update_event_arrays(instanceControl IC[]) {
    int i;
    int instance;
    int offset;
    int instanceTrigged;
    int unitTrigged = 0;
    uint8_t vectorDepth;

    for (instance = 1; instance < INSTANCES ; instance++) {
        vectorDepth = VECTOR_DEPTH_DONT_CHECK;
        instanceTrigged = 0;
        for (i = 0; i < sizeof (allVariables) / sizeof (variable); i++) {
            if ((allVariables[i].number == VARIABLE_EVENT_ARRAY) && (allVariables[i].productType == IC[instance].productType)) {
                vectorDepth = allVariables[i].vectorDepth;
                break;
            }
        }
        if (vectorDepth == VECTOR_DEPTH_DONT_CHECK) {
            continue;
        }
        for (offset = 1; offset <vectorDepth; offset++) {
            if (IC[instance].eventArray[(offset * 4) + 3] == EVENTSTATE_EVENT_TRIGGED) {
                instanceTrigged++;
            }
        }
        if (instanceTrigged > 0) {
            IC[instance].eventArray[3] |= EVENTSTATE_OFFSET0_TRIGGED_MASK;
        } else {
            IC[instance].eventArray[3] &= EVENTSTATE_OFFSET0_ENABLE_MASK;
        }
        unitTrigged += instanceTrigged;
    }
    if (unitTrigged > 0) {
        IC[0].eventArray[3] |= EVENTSTATE_OFFSET0_TRIGGED_MASK;
    } else {
        IC[0].eventArray[3] &= EVENTSTATE_OFFSET0_ENABLE_MASK;
        event_backoff_timer(EVENT_BACKOFF_TIMER_STOP);
    }
}

/*
 * Write event TLV(s) to "reply", no more than "len" bytes.
 * Return number of bytes written.
 */
size_t
generate_event_reply(instanceControl IC[], uint8_t *reply, size_t len) {
    TLV T;
    size_t retval = 0;
    int i;

    /* Boot timer read response */
    memset(&T, 0, sizeof (T));
    T.variable = VARIABLE_UNIT_BOOT_TIMER;
    T.opcode = TLV_OPCODE_GET_RESPONSE;
    retval += writeReply64intTlv(&T, reply, len, boottimerIEEE64());

#ifdef HAVE_INSTANCE_SLEEP
#ifdef VARIABLE_SLEEP_DEFAULT_AWAKE_TIME
    oam_tx_activity(); // update activity before reading.
    memset(&T, 0, sizeof (T));
    T.variable = VARIABLE_SLEEP_DEFAULT_AWAKE_TIME;
    T.opcode = TLV_OPCODE_GET_RESPONSE;
    retval += writeReply32intTlv(&T, reply + retval, len - retval, get_awake_time());
#endif /* VARIABLE_SLEEP_DEFAULT_AWAKE_TIME */
#endif /* HAVE_INSTANCE_SLEEP */

#ifdef VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX
    memset(&T, 0, sizeof (T));
    T.variable = VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX;
    T.opcode = TLV_OPCODE_GET_RESPONSE;
    T.elementSize = 4;
    retval += writeReply32intTlv(&T, &reply[retval], len - retval,
                                 oam_get_time_since_last_good_rx_from_uc());
#endif /* VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX */

    for (i = 1; i < INSTANCES; i++) {
        if (IC[i].eventArray[3] & EVENTSTATE_OFFSET0_TRIGGED_MASK) {
            T.instance = i;
            /* First add the instance' own event TLVs */
            if (IC[i].poll) {
              retval += IC[i].poll(i, reply + retval, len - retval, OAM_POLL_TYPE_EVENT);
            }
            /* prepare and add the event TLV */
            memset(&T, 0, sizeof (T));
            T.length = 8;
            T.variable = VARIABLE_EVENT_ARRAY;
            T.opcode = TLV_OPCODE_EVENT_RESPONSE;
            T.elementSize = 4;
            T.instance = i;
            retval += tlv_to_bytes(&T, reply + retval, len - retval);
        }
    }       
    return retval;
}

/**
 * Update event state with action.
 * Return TRUE if a trigged event was acknowledged.
 */
uint8_t
doEventAction(uint8_t *state, uint8_t action) {
    uint8_t newState = *state;
    uint8_t retval = FALSE;

    switch (*state) {
    case EVENTSTATE_EVENT_DISARMED:
        if (action == EVENTACTION_EVENT_ARM) {
            newState = EVENTSTATE_EVENT_ARMED;
        } else if (action == EVENTACTION_EVENT_REARM) {
          newState = EVENTSTATE_EVENT_ARMED;
        }
        break;
    case EVENTSTATE_EVENT_ARMED:
        if (action == EVENTACTION_EVENT_DISARM) {
            newState = EVENTSTATE_EVENT_DISARMED;
        }
        break;
    case EVENTSTATE_EVENT_TRIGGED:
        if (action == EVENTACTION_EVENT_REARM) {
            newState = EVENTSTATE_EVENT_ARMED;
            retval = TRUE;
        } else if (action == EVENTACTION_EVENT_DISARM) {
            newState = EVENTSTATE_EVENT_DISARMED;
            retval = TRUE;
        }
    break;
    }

    *state = newState;
    return retval;
}

/*
 * Common processing of discovery variables for sub-instances.
 */
size_t
processDiscoveryRequest(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing, instanceControl *localIC) {
    if ((request->opcode == TLV_OPCODE_SET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_SET_REQUEST)) {
        if (request->variable == VARIABLE_EVENT_ARRAY) {
            uint8_t cancelEvent = FALSE;
            if (request->opcode & TLV_OPCODE_VECTOR_MASK) {
                if (request->offset == 0) {
                    // only LSB in first offset, enable/disable bit, is used.
                    if ((request->data[3] & 0x01) != (localIC[request->instance].eventArray[3] & 0x01)) {
                        localIC[request->instance].eventArray[3] ^= 0x01;
                    }
                    if (request->elements > 1) {
                        cancelEvent = doEventAction(&localIC[request->instance].eventArray[7], request->data[7]);
                    }
                } else {
                    // offset must be 1, since we would have been returning error otherwise.
                    cancelEvent = doEventAction(&localIC[request->instance].eventArray[7], request->data[3]);
                }
                if (cancelEvent) {
                    update_event_arrays(localIC);
                }
                return writeReplyVectorTlv(request, reply, len, localIC[request->instance].eventArray);
            } else {
                if ((request->data[3] & 0x01) != (localIC[request->instance].eventArray[3] & 0x01)) {
                    localIC[request->instance].eventArray[3] ^= 0x01;
                }
                return writeReply32Tlv(request, reply, len, localIC[request->instance].eventArray);
            }
        } /* VARIABLE_EVENT_ARRAY */
        
        return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

    } else if ((request->opcode == TLV_OPCODE_GET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_GET_REQUEST)) {
        if (request->variable == VARIABLE_PRODUCT_TYPE) {
            return writeReply64intTlv(request, reply, len, localIC[request->instance].productType);
        }
        if (request->variable == VARIABLE_PRODUCT_ID) {
            return writeReply128Tlv(request, reply, len, localIC[request->instance].productID);
        }
        if (request->variable == VARIABLE_PRODUCT_LABEL) {
            return writeReply256Tlv(request, reply, len, localIC[request->instance].label);
        }
        if (request->variable == VARIABLE_NUMBER_OF_INSTANCES) {
            uint32_t i = localIC[request->instance].instances;
            return writeReply32intTlv(request, reply, len, i);
        }
        if (request->variable == VARIABLE_PRODUCT_SUB_TYPE) {
            return writeReply32Tlv(request, reply, len, localIC[request->instance].productSubType);
        }
        if (request->variable == VARIABLE_EVENT_ARRAY) {
            if (request->opcode & TLV_OPCODE_VECTOR_MASK) {
                return writeReplyVectorTlv(request, reply, len, localIC[request->instance].eventArray);
            } else {
                return writeReply32Tlv(request, reply, len, localIC[request->instance].eventArray);
            }
        }

        return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

    }
    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}


/*
 * Check if this TLV contains a discovery variable request or
 * response.
 */
uint8_t
is_discovery_variable(TLV *t) {
    if (t->variable < 0xc0) {
        return TRUE;
    }
    return FALSE;
}

/*
 * Check if this TLV contains a public encryption variable request or
 * response.
 */
uint8_t
is_special_variable(TLV *t) {
    //Special variables are Location, Owner and Vendor Key
    if (t->variable == 0xd2 || t->variable == 0xd3 || t->variable == 0xd4) {
        return TRUE;
    }
    return FALSE;
}
