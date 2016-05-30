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

#include "string.h"
#include "oam.h"
#include "api.h"
#include "encap.h"
#include "vargen.h"

/* local prototypes */
size_t generatePSPrequest(uint8_t *reply, size_t len);

/* struct ip_addr uc_address; */
uint16_t uc_port;

extern uint8_t oamPeriodic;
extern uint8_t printRxReflections;
extern volatile uint8_t ssdp_announce_now;
extern uint8_t print_traffic;

uint16_t next_event_increment = 100;
uint8_t zeroes[16] = { 0, };

static uint8_t respond_to_uc = FALSE;

#ifdef VARIABLE_TLV_GROUPING
uint64_t tlv_grouping_time = 0;
uint32_t tlv_grouping_holdoff = 0;
#endif /* VARIABLE_TLV_GROUPING */
#ifdef MAX_INSTANCES
instanceControl instances[MAX_INSTANCES];
#else
instanceControl instances[INSTANCES];
#endif /* MAX_INSTANCES */

/* callback for portal status */
void (*portal_status_cb)(uint32_t) = NULL;

/*
 * oam modules init prototypes
 */
/*
 * Process Portal Selection Protocol response.
 * Do not reply on error, just silentrly drop.
 * Do not reply on success.
 */
size_t
processPSPRequest(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing)
{
  if((request->opcode == TLV_OPCODE_SET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_SET_REQUEST)) {
    /* write requests */
#ifdef VARIABLE_UNIT_CONTROLLER_ADDRESS
    uint8_t error;
    if(request->variable == VARIABLE_UNIT_CONTROLLER_ADDRESS) {
      error = unitControllerSetAddress(request->data, TRUE);
      if(error == TLV_ERROR_NO_ERROR) {
        return writeReply256Tlv(request, reply, len, 0);
      } else {
        return 0;
      }
    }
#endif /* #ifdef VARIABLE_UNIT_CONTROLLER_ADDRESS */
  } else if(request->opcode == TLV_OPCODE_GET_RESPONSE) {
#ifdef VARIABLE_PSP_PORTAL_STATUS
    if(request->variable == VARIABLE_PSP_PORTAL_STATUS) {
      if(portal_status_cb) {
        portal_status_cb(get_int32_from_data(request->data));
      }
      return 0;
    }
#endif /* VARIABLE_PSP_PORTAL_STATUS */
  }
  return 0;
}
/*----------------------------------------------------------------*/

/*
 * OAM framework implementatoin.
 */

/*
 * Process an OAM request placed on "request", no more than "len" bytes.
 * If needed, generate a reply into "reply", no more than "replymax" bytes.
 * Return number of bytes in "reply" to send or 0 on no reply to send.
 */
int
process_TLV_request(const uint8_t *request, size_t len, uint8_t *reply, size_t replymax, pdu_info *pinfo)
{
  int reqpos;
  int replyLen = 0;
  int thislen = 0;
  TLV T;
  TLV *t = &T;
  oam_end_processing endProcessing = OAM_END_PROCESSING_NEW;
  uint8_t error;
  uint8_t PSP = FALSE;

  if(replymax < 8) {
    return 0;
  }

  if(len <= 0) {
    return 0;
  }
  if(pinfo != NULL) {
    PSP = (pinfo->payload_type == ENC_PAYLOAD_PORTAL_SELECTION_PROTO) ? TRUE : FALSE;
  }
  reqpos = 0;
  stackBegin();

  /* Process the TLV stack */
  for(reqpos = 0; (thislen = tlv_from_bytes(t, request + reqpos)) > 0; reqpos += thislen) {

    /* first check end processing from last turn */
    if(endProcessing & OAM_END_PROCESSING_DO_NOT_REPLY) {
      pinfo->dont_reply = TRUE;
    }
    if(endProcessing & OAM_END_PROCESSING_ABORT_TLV_STACK) {
      break;
    }

    /*
     * If parsing this TLV would read outside buffer, abort stack parsing.
     */
    if((thislen + reqpos) > len) {
      DEBUG_PRINT_I("TLV @ %d parsing beyond buffer, aborting\n", reqpos);
      return 0;
    }

    /*
     * If any encryption key is set, only allow read request to
     * discovery variables in instance 0.
     */
    if((is_keys_set() != 0) && (pinfo->encrypt == FALSE)) {
      if(!is_discovery_variable(t) && t->instance != 0) {
        printf("Dropping unencrypted non-discovery variable to instance %d\n", t->instance);
        continue;
      }
      if(is_discovery_variable(t)) {
        if((t->opcode == TLV_OPCODE_GET_REQUEST) || (t->opcode == TLV_OPCODE_VECTOR_GET_REQUEST)) {
          replyLen += processDiscoveryRequest(t, reply + replyLen, replymax - replyLen, &endProcessing, instances);
          continue;
        }
        printf("Dropping unencrypted write request discovery variable\n");
        continue;
      } else if(t->instance == 0) { /* is not discovery but instance is 0 */
        if((t->opcode == TLV_OPCODE_GET_REQUEST) || (t->opcode == TLV_OPCODE_VECTOR_GET_REQUEST)) {
          replyLen += instances[t->instance].processRequest(t, reply + replyLen, replymax - replyLen, &endProcessing);
          continue;
        }
        printf("Dropping unencrypted write request instance 0 variable\n");
        continue;
      }
    }

    /* No need to check if have_keys, if I hadn't I wouldn't reach here. */
    if(pinfo->encrypt == TRUE) {
      /* Check if it is a special variable. If it is, let the ecc_instance decide on the processing */
      if(is_special_variable(t)) {
        if(!get_authorized_action(t->variable, pinfo)) {
          printf("Dropping unauthorized change\n");
          continue;
        }
      } else {
        if(pinfo->key_class != 0) {
          printf("Dropping standard encrypted access with key %d\n", pinfo->key_class);
          continue;
        }
      }
    }

    /*
     * If this instance is handled elsewhere, give it there, without
     * any checks
     */
    if(instances[t->instance].process == INSTANCE_PROCESS_EXTERNAL) {
      replyLen += instances[t->instance].processRequest(t, reply + replyLen, replymax - replyLen, &endProcessing);
      continue;
    }

    /*
     * Reflector for any instance is handeled here.
     */
    if((t->opcode == TLV_OPCODE_REFLECT_REQUEST) || (t->opcode == TLV_OPCODE_VECTOR_REFLECT_REQUEST)) {
      if(printRxReflections) {
        /* DEBUG_PRINT_H("Reflection payload: ", t->data, 0, t->elementSize * t->elements, " ", 0); */
      }
      replyify(t);
      replyLen += tlv_to_bytes(t, reply + replyLen, replymax - replyLen);
      continue;
    }

    /*
     * Process Event request
     */
    if((t->opcode == TLV_OPCODE_EVENT_REQUEST) || (t->opcode == TLV_OPCODE_VECTOR_EVENT_REQUEST)) {
      replyify(t);
      replyLen += tlv_to_bytes(t, reply + replyLen, replymax - replyLen);
      respond_to_uc = TRUE;
      continue;
    }

    error = checkTLV(t, INSTANCES, instances, PSP);
    /* DEBUG_PRINT_II("@%04d: Checking TLV error: %d\n", reqpos, error); */
    if(error == TLV_ERROR_UNPROCESSED_TLV) {
      /* Silently skip it. */
      continue;
    }
    /* Silently skip unknown PSP variables */
    if(PSP && (error == TLV_ERROR_UNKNOWN_VARIABLE)) {
      continue;
    }
    if(error != TLV_ERROR_NO_ERROR) {
      t->error = error;
      replyLen += tlv_to_bytes(t, reply + replyLen, replymax - replyLen);
      continue;
    }
    /*
     * Process request
     */
    /* DEBUG_PRINT_II("@%04d: replyLen before: %d\n", reqpos, replyLen); */
    if(PSP) {
      replyLen += processPSPRequest(t, reply + replyLen, replymax - replyLen, &endProcessing);
    } else {
      if(is_discovery_variable(t)) {
        replyLen += processDiscoveryRequest(t, reply + replyLen, replymax - replyLen, &endProcessing, instances);
      } else {
        replyLen += instances[t->instance].processRequest(t, reply + replyLen, replymax - replyLen, &endProcessing);
      }
    }
    /* DEBUG_PRINT_II("@%04d: replyLen after : %d\n", reqpos, replyLen); */

    if((endProcessing & OAM_END_PROCESSING_ABORT_TLV_STACK) || (endProcessing & OAM_END_PROCESSING_DO_NOT_REPLY)) {
      continue;
    }
  } /* for(reqpos = 0; (thislen = tlv_from_bytes(t, request + reqpos)) > 0; reqpos += thislen) */

  stackEnd();

  return replyLen;
}
/*----------------------------------------------------------------*/

/*
 * TLV stacks
 */

/*
 * Stuff to do before starting processing of a new TLV stack.
 */
void
stackBegin()
{
  int i;

#ifdef VARIABLE_TLV_GROUPING
  if(boottimer_elapsed(tlv_grouping_time) < tlv_grouping_holdoff) {
    return;
  }
#endif /* VARIABLE_TLV_GROUPING */
  /* Tell instances that has registered interest */
  for(i = 0; i < INSTANCES; i++) {
    if(instances[i].oam_do_action) {
      instances[i].oam_do_action(OAM_ACTION_NEW_PDU, NEW_TLV_PDU_OPERATION_NEW);
    }
  }
}
/*----------------------------------------------------------------*/

/*
 * Stuff to do after processing of a TLV stack.
 */
void
stackEnd()
{

  int i;
  for(i = 0; i < INSTANCES; i++) {
    if(instances[i].oam_do_action) {
      instances[i].oam_do_action(OAM_ACTION_END_PDU, 0);
    }
  }
}
/*----------------------------------------------------------------*/

/*
 * Stuff to do after sending reply.
 * sent_or_skipped should be non-zero if reply was sent.
 */
void
reply_sent(uint8_t sent_or_skipped)
{
  int i;
  /* Tell instances that has registered interest */
  for(i = 0; i < INSTANCES; i++) {
    if(instances[i].oam_do_action) {
      instances[i].oam_do_action(OAM_ACTION_REPLY_SENT, sent_or_skipped);
    }
  }
}
/*----------------------------------------------------------------*/

/*
 * Called from unit controller when watchdog expired.
 * Tell anyone that might be interested.
 */
void
watchdog_expired()
{
  int i;
  for(i = 0; i < INSTANCES; i++) {
    if(instances[i].oam_do_action) {
      instances[i].oam_do_action(OAM_ACTION_UC_WD_EXPIRE, 0);
    }
  }
}
/*----------------------------------------------------------------*/

/*
 * Write configuration status to "p".
 */
void
getConfigurationStatus(uint8_t *p)
{
  p[0] = 0;
  p[1] = 0;
  p[2] = 0;
  p[3] = 1;
}
/*----------------------------------------------------------------*/

/*
 * Generate Portal Selection Protocol request
 * Write request into "reply", no more than "len" bytes.
 * Return number of bytes written.
 */
size_t
generatePSPrequest(uint8_t *reply, size_t len)
{
  int replyLen = 0;
  TLV T;
  uint8_t tmp[4];

  tmp[0] = 0;   /* prevent warning: unused variable 'tmp' */

  memset(&T, 0, sizeof(T));
  T.version = TLV_VERSION;
  T.opcode = TLV_OPCODE_GET_RESPONSE;
  T.error = TLV_ERROR_NO_ERROR;

  T.variable = VARIABLE_PRODUCT_TYPE;
  T.elementSize = 8;
  replyLen += writeReply64intTlv(&T, &reply[replyLen], len - replyLen, get_product_type());

  T.variable = VARIABLE_PRODUCT_ID;
  T.elementSize = 8;
  replyLen += writeReply128Tlv(&T, &reply[replyLen], len - replyLen, getDid());

  memset(&T, 0, sizeof(T));
  T.version = TLV_VERSION;
  T.opcode = TLV_OPCODE_GET_RESPONSE;
  T.error = TLV_ERROR_NO_ERROR;

#ifdef VARIABLE_SW_REVISION
  T.variable = VARIABLE_SW_REVISION;
  T.elementSize = 16;
  replyLen += writeReply128Tlv(&T, &reply[replyLen], len - replyLen, (uint8_t *)getswrevision());
#endif /* VARIABLE_SW_REVISION */

#ifdef VARIABLE_CONFIGURATION_STATUS
  T.variable = VARIABLE_CONFIGURATION_STATUS;
  T.elementSize = 4;
  getConfigurationStatus(tmp);
  replyLen += writeReply32Tlv(&T, &reply[replyLen], len - replyLen, tmp);
#endif /* VARIABLE_CONFIGURATION_STATUS */

#ifdef VARIABLE_COMMUNICATION_STYLE
  T.variable = VARIABLE_COMMUNICATION_STYLE;
  T.elementSize = 4;
  getCommunicationStyle(tmp);
  replyLen += writeReply32Tlv(&T, &reply[replyLen], len - replyLen, tmp);
#endif /* VARIABLE_COMMUNICATION_STYLE */
  return replyLen;
}
/*----------------------------------------------------------------*/
/*
 * set portal_status_cb function to use.
 */
void
set_portal_status_cb(void (*func)(uint32_t))
{
  portal_status_cb = func;
}
/*----------------------------------------------------------------*/
