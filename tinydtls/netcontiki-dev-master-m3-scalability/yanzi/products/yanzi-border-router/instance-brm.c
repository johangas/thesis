/*
 * Copyright (c) 2014-2015, Yanzi Networks AB.
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
 *         Border Router Management Instance
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "oam.h"
#include "instance-brm.h"
#include "brm-stats.h"
#include "border-router.h"
#include "br-config.h"
#include "cmd.h"
#include "net/rpl/rpl.h"
#include <string.h>
#include <stdlib.h>

#define YLOG_LEVEL YLOG_LEVEL_INFO
#define YLOG_NAME  "brm"
#include "lib/ylog.h"

static instanceControl *localIC = NULL;
static uint32_t radio_bootloader_version;
static uint64_t radio_capabilities;
static char radio_sw_revision[16];

uint32_t brm_stats[BRM_STATS_MAX];
uint32_t brm_stats_debug[BRM_STATS_DEBUG_MAX];

/*---------------------------------------------------------------------------*/
uint32_t
instance_brm_get_radio_bootloader_version(void)
{
  return radio_bootloader_version;
}
/*---------------------------------------------------------------------------*/
void
instance_brm_set_radio_bootloader_version(uint32_t version)
{
  radio_bootloader_version = version;
}
/*---------------------------------------------------------------------------*/
uint64_t
instance_brm_get_radio_capabilities(void)
{
  return radio_capabilities;
}
/*---------------------------------------------------------------------------*/
void
instance_brm_set_radio_capabilities(uint64_t capabilities)
{
  radio_capabilities = capabilities;
}
/*---------------------------------------------------------------------------*/
const char *
instance_brm_get_radio_sw_revision(void)
{
  return radio_sw_revision;
}
/*---------------------------------------------------------------------------*/
void
instance_brm_set_radio_sw_revision(const char *revision)
{
  strncpy(radio_sw_revision, revision, sizeof(radio_sw_revision) - 1);
}
/*---------------------------------------------------------------------------*/
/**
 * Process a request TLV.
 *
 * Writes a response TLV starting at "reply", no more than "len"
 * bytes.
 *
 * if the "request" is of type vectorAbortIfEqualRequest or
 * abortIfEqualRequest, and processing should be aborted,
 * "endProcessing" is set to OAM_END_PROCESSING_ABORT_TLV_STACK.
 *
 * Returns number of bytes written to "reply".
 */
static size_t
brm_processRequest(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing) {
  uint32_t local32;

  if((request->opcode == TLV_OPCODE_SET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_SET_REQUEST)) {

    if(request->variable == VARIABLE_RADIO_SERIAL_MODE) {
      local32 = get_int32_from_data(request->data);
      serial_set_mode(local32);
      return writeReply32Tlv(request, reply, len, 0);
    }

    if(request->variable == VARIABLE_RADIO_SERIAL_SEND_DELAY) {
      serial_set_send_delay(get_int32_from_data(request->data));
      return writeReply32Tlv(request, reply, len, 0);
    }

    if(request->variable == VARIABLE_BRM_COMMAND) {
      local32 = get_int32_from_data(request->data);
      switch(local32) {
      case BRM_COMMAND_NOP:
        return writeReply32Tlv(request, reply, len, 0);
      case BRM_COMMAND_RPL_GLOBAL_REPAIR:
        YLOG_INFO("RPL global repair\n");
        rpl_repair_root(wsn_rpl_instance_id);
        return writeReply32Tlv(request, reply, len, 0);
      default:
        return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
    }

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

  } else if((request->opcode == TLV_OPCODE_GET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_GET_REQUEST)) {

    if (request->variable == VARIABLE_INTERFACE_NAME) {
      extern char slip_config_tundev[32];
      return writeReply256Tlv(request, reply, len, (uint8_t *)slip_config_tundev);
    }

    if(request->variable == VARIABLE_SERIAL_INTERFACE_NAME) {
      extern const char *slip_config_siodev;
      extern const char *slip_config_host;
      extern const char *slip_config_port;
      char tmp[32];
      memset(tmp, 0, sizeof(tmp));
      if(slip_config_host != NULL) {
        snprintf(tmp, sizeof(tmp) - 1, "tcp:%s:%s", slip_config_host,
                 slip_config_port == NULL ? "*" : slip_config_port);
      } else if(slip_config_siodev != NULL) {
        strncpy(tmp, slip_config_siodev, sizeof(tmp) - 1);
      }

      return writeReply256Tlv(request, reply, len, (uint8_t *)tmp);
    }

    if(request->variable == VARIABLE_RADIO_CONTROL_API_VERSION) {
      local32 = BORDER_ROUTER_CONTROL_API_VERSION;
      return writeReply32intTlv(request, reply, len, local32);
    }

    if(request->variable == VARIABLE_RADIO_SERIAL_MODE) {
      local32 = serial_get_mode();
      return writeReply32intTlv(request, reply, len, local32);
    }

    if(request->variable == VARIABLE_RADIO_SERIAL_SEND_DELAY) {
      local32 = serial_get_send_delay();
      return writeReply32intTlv(request, reply, len, local32);
    }

    if (request->variable == VARIABLE_RADIO_SW_REVISION) {
      return writeReply128Tlv(request, reply, len,  (uint8_t *)radio_sw_revision);
    }

    if (request->variable == VARIABLE_RADIO_BOOTLOADER_VERSION) {
      return writeReply32intTlv(request, reply, len, radio_bootloader_version);
    }

    if (request->variable == VARIABLE_RADIO_CHASSIS_CAPABILITIES) {
      return writeReply64intTlv(request, reply, len, radio_capabilities);
    }

    if (request->variable == VARIABLE_BRM_STAT_LENGTH) {
      local32 = BRM_STATS_MAX;
      return writeReply32intTlv(request, reply, len, local32);
    }

    if (request->variable == VARIABLE_BRM_STAT_DATA) {
      if (request->opcode == TLV_OPCODE_VECTOR_GET_REQUEST) {
        if (request->offset >= BRM_STATS_MAX ) {
          request->elements = 0;
        } else {
          request->elements = MIN(request->elements, (BRM_STATS_MAX - request->offset));
        }
        return writeReplyVectorTlv(request, reply, len, ((uint8_t *)brm_stats));

      } else {
        return writeErrorTlv(request, TLV_ERROR_NO_VECTOR_ACCESS, reply, len);
      }
    }

    if (request->variable == VARIABLE_BRM_RPL_DAG_VERSION) {
      rpl_instance_t *instance;

      instance = rpl_get_instance(wsn_rpl_instance_id);
      if(instance != NULL && instance->current_dag != NULL) {
        local32 = instance->current_dag->version;
        YLOG_DEBUG("RPL DAG version: %lu\n", (unsigned long)local32);
      } else {
        local32 = 0xffffffffUL;
      }
      return writeReply32intTlv(request, reply, len,  local32);
    }

#ifdef VARIABLE_BRM_STAT_DEBUG_LENGTH
    if (request->variable == VARIABLE_BRM_STAT_DEBUG_LENGTH) {
      local32 = BRM_STATS_DEBUG_MAX;
      return writeReply32intTlv(request, reply, len, local32);
    }
#endif /* VARIABLE_BRM_STAT_DEBUG_LENGTH */

#ifdef VARIABLE_BRM_STAT_DEBUG_DATA
    if (request->variable == VARIABLE_BRM_STAT_DEBUG_DATA) {
      if (request->opcode == TLV_OPCODE_VECTOR_GET_REQUEST) {
        if (request->offset >= BRM_STATS_DEBUG_MAX ) {
          request->elements = 0;
        } else {
          request->elements = MIN(request->elements, (BRM_STATS_DEBUG_MAX - request->offset));
        }
        return writeReplyVectorTlv(request, reply, len, ((uint8_t *)brm_stats_debug));

      } else {
        return writeErrorTlv(request, TLV_ERROR_NO_VECTOR_ACCESS, reply, len);
      }
    }
#endif /* VARIABLE_BRM_STAT_DEBUG_DATA */

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }
  return writeErrorTlv(request, TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}
/*---------------------------------------------------------------------------*/
/*
 * Init called from oam_init.
 */
void
instance_brm_init(instanceControl IC[])
{
  localIC = IC;

  IC[INSTANCE_BRM].processRequest = brm_processRequest;
  IC[INSTANCE_BRM].productType = 0x0090DA0303010022ULL;

  strncat((char *)IC[INSTANCE_BRM].label, "Border router management",
          sizeof(IC[INSTANCE_BRM].label) - 1);
}
/*---------------------------------------------------------------------------*/
