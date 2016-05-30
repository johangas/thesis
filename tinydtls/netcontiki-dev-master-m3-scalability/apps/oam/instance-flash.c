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

/**
 * Flash instance. Implements product 0x0090DA0303010010.
 * Currently hard-coded for 2 instances, PRIMARY_FLASH_INSTANCE
 * and BACKUP_FLASH_INSTANCE that should be defined
 * somewhere in application, typically project-conf.h
 */

#include "oam.h"
#include "instance-flash.h"
#include "api.h"
#include "trailer.h"
#include "dev/watchdog.h"

static instanceControl *localIC = NULL;
extern uint8_t zeroes[16];
static uint8_t primaryWrite_enable = FALSE;
static uint8_t backupWrite_enable = FALSE;

#define FLASH_WRITE_CONTROL_ERASE 0x911
#define FLASH_WRITE_CONTROL_WRITE_ENABLE 0x23

/*===========================================================================*/
/*
 * Hard-coded image start and end addresses, for testing purpose
 */
#define FLASH_IMAGE_1_START_ADDR  0x00204000
#define FLASH_IMAGE_1_END_ADDR    0x00241fff
#define FLASH_IMAGE_2_START_ADDR  0x00242000
#define FLASH_IMAGE_2_END_ADDR    0x0027f7ff

int
running_image(void)
{
  uint32_t where = (uint32_t)running_image;
  if(where < FLASH_IMAGE_1_START_ADDR || where > FLASH_IMAGE_2_END_ADDR) {
    printf("instance-flash.c: err-wrong-area\n");
    return -1;
  }
  if(where < FLASH_IMAGE_2_START_ADDR) {
    return 1;
  } else {
    return 2;
  }
}
/*===========================================================================*/

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
size_t
flash_processRequest(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing)
{
  uint32_t local32;
  uint8_t error = TLV_ERROR_NO_ERROR;
  image_info *image = NULL;
  uint8_t *write_enable;

  if(request->instance == PRIMARY_FLASH_INSTANCE) {
    image = &images[0];
    write_enable = &primaryWrite_enable;
  } else if(request->instance == BACKUP_FLASH_INSTANCE) {
    image = &images[1];
    write_enable = &backupWrite_enable;
  } else {
    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_INSTANCE, reply, len);
  }

  if((request->opcode == TLV_OPCODE_SET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_SET_REQUEST)) {
    /*
     * Payload variables
     */
    if(request->variable == VARIABLE_FLASH) {
      if(*write_enable) {
        error = writeFlash(image, request->offset, request->elements, (uint32_t *)request->data);
      } else {
        error = TLV_ERROR_WRITE_ACCESS_DENIED;
      }

      if(error == TLV_ERROR_NO_ERROR) {
        return writeReplyVectorTlv(request, reply, len, request->data);
      } else {
        return writeErrorTlv(request, error, reply, len);
      }
    } else if(request->variable == VARIABLE_WRITE_CONTROL) {
      if(request->instance == running_image()) {
        return writeErrorTlv(request, TLV_ERROR_WRITE_ACCESS_DENIED, reply, len);
      }
      local32 = request->data[0] << 24;
      local32 |= request->data[1] << 16;
      local32 |= request->data[2] << 8;
      local32 |= request->data[3];

      if(local32 == FLASH_WRITE_CONTROL_ERASE) {
        if(eraseImage(image) != TRUE) {
          return writeErrorTlv(request, TLV_ERROR_HARDWARE_ERROR, reply, len);
        }
        return writeReply32Tlv(request, reply, len, zeroes);
      } else if(local32 == FLASH_WRITE_CONTROL_WRITE_ENABLE) {
        *write_enable = TRUE;
        return writeReply32Tlv(request, reply, len, zeroes);
      }
    }
    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  } else if((request->opcode == TLV_OPCODE_GET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_GET_REQUEST)) {
    /*
     * Payload variables
     */
    if(request->variable == VARIABLE_FLASH) {
      if(((request->offset + request->elements) * request->elementSize) > image->length) {
        return writeErrorTlv(request, TLV_ERROR_INVALID_VECTOR_OFFSET, reply, len);
      }
      return writeReplyVectorTlv(request, reply, len, (uint8_t *)image->startAddress);
    } else if(request->variable == VARIABLE_WRITE_CONTROL) {
      if(*write_enable) {
        local32 = htonl(FLASH_WRITE_CONTROL_WRITE_ENABLE);
      } else {
        local32 = 0;
      }
      return writeReply32intTlv(request, reply, len, local32);
    } else if(request->variable == VARIABLE_IMAGE_START_ADDRESS) {
      return writeReply32intTlv(request, reply, len, image->startAddress);
    } else if(request->variable == VARIABLE_IMAGE_MAX_LENGTH) {
      return writeReply32intTlv(request, reply, len, image->length);
    } else if(request->variable == VARIABLE_IMAGE_ACCEPTED_TYPE) {
      return writeReply64intTlv(request, reply, len, image->imageType);
    } else if(request->variable == VARIABLE_IMAGE_STATUS) {
      local32 = get_image_status(image);
      return writeReply32intTlv(request, reply, len, local32);
    } else if(request->variable == VARIABLE_IMAGE_VERSION) {
      uint64_t ver;
      ver = get_image_version(image);
      return writeReply64intTlv(request, reply, len, ver);
    } else if(request->variable == VARIABLE_IMAGE_LENGTH) {
      local32 = get_image_length(image);
      return writeReply32intTlv(request, reply, len, local32);
    }
    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }
  return writeErrorTlv(request, TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}
/*----------------------------------------------------------------*/

/*
 * On new PDU; clear all write and erase enable states.
 */
static uint8_t
new_tlv_pdu(uint8_t operation)
{
  if(operation == NEW_TLV_PDU_OPERATION_NEW) {
    primaryWrite_enable = FALSE;
    backupWrite_enable = FALSE;
  }
  return FALSE;
}
/*----------------------------------------------------------------*/

/*
 * Action from OAM, typically new PDU or unitcontroller watchdog expire.
 */
static uint8_t
flash_oam_do_action(oam_action action, uint8_t parameter)
{
  switch(action) {
  case OAM_ACTION_NEW_PDU:
    return new_tlv_pdu(parameter);
    break;
  case OAM_ACTION_UC_WD_EXPIRE:
    return 0;
    break;
  default:
    return 0;
    break;
  }
}
/*----------------------------------------------------------------*/

/*
 * Init called from oam_init.
 */
void
instance_flash_init(instanceControl IC[])
{
  localIC = IC;
  /* Setup primary, then copy to backup */
  IC[PRIMARY_FLASH_INSTANCE].eventArray[0] = 0;
  IC[PRIMARY_FLASH_INSTANCE].eventArray[1] = 1; /* Number of elements in the event array */
  IC[PRIMARY_FLASH_INSTANCE].eventArray[2] = 0;
  IC[PRIMARY_FLASH_INSTANCE].eventArray[3] = 0;
  IC[PRIMARY_FLASH_INSTANCE].eventArray[4] = 0;
  IC[PRIMARY_FLASH_INSTANCE].eventArray[5] = 0;
  IC[PRIMARY_FLASH_INSTANCE].eventArray[6] = 0;
  IC[PRIMARY_FLASH_INSTANCE].eventArray[7] = 0;

  IC[PRIMARY_FLASH_INSTANCE].processRequest = flash_processRequest;
  IC[PRIMARY_FLASH_INSTANCE].oam_do_action = flash_oam_do_action;

  IC[PRIMARY_FLASH_INSTANCE].productType = 0x0090DA0303010010ULL; /* Simplified flash */
  /* Copy to backup flash instance */
  memcpy(&IC[BACKUP_FLASH_INSTANCE], &IC[PRIMARY_FLASH_INSTANCE], sizeof(instanceControl));

  strncat((char *)IC[PRIMARY_FLASH_INSTANCE].label, "Primary firmware", 32 - 1);
  strncat((char *)IC[BACKUP_FLASH_INSTANCE].label, "Backup firmware", 32 - 1);
}
/*----------------------------------------------------------------*/

/*
 * Erase image.
 * Return TRUE on success.
 */
uint8_t
eraseImage(image_info *image)
{
  static uint8_t success = TRUE;
  static uint32_t p;

  FLASH_Unlock();
  if(FLASH_WaitForLastOperation() != FLASH_COMPLETE) {
    DEBUG_PRINT_("eraseImage: Failed to unlock flash\n");
    return FALSE;
  }

  if(image->numberOfSectors == 0) {
    for(p = image->startAddress; p < image->startAddress + image->length; p += 4) {
      watchdog_periodic();
      if(*((uint32_t *)p) != 0xffffffff) {
        if(FLASH_EraseSector(p) != FLASH_COMPLETE) {
          success = FALSE;
          break;
        }
      }
    }
  }

  FLASH_Lock();
  if(FLASH_WaitForLastOperation() != FLASH_COMPLETE) {
    DEBUG_PRINT_("eraseImage: Failed to lock flash\n");
    success = FALSE;
  }

  return success;
}
/*----------------------------------------------------------------*/

/*
 * Write flash.
 *
 * @param image pointer to the image definition to write to.
 * @param offset start writing this number of 32-bit words into the image.
 * @param length number of 32-bit words to write.
 * @param data pointer to the data to write.
 *
 * Return TLV error
 */
uint8_t
writeFlash(image_info *image, uint32_t offset, uint32_t length, uint32_t *data)
{
  int i;
  uint8_t retval = TLV_ERROR_NO_ERROR;
  FLASH_Status status;

  if(offset * 4 >= image->length) {
    return TLV_ERROR_INVALID_VECTOR_OFFSET;
  }

  if(((offset * 4) + (length * 4)) >= image->length) {
    return TLV_ERROR_BAD_NUMBER_OF_ELEMENTS;
  }

  FLASH_Unlock();
  if(FLASH_WaitForLastOperation() != FLASH_COMPLETE) {
    DEBUG_PRINT_("writeFlash: Failed to unlock flash\n");
    return TLV_ERROR_HARDWARE_ERROR;
  }

  for(i = 0; i < length; i++) {
    uint32_t present = *((uint32_t *)(image->startAddress + (offset + i) * 4));
    if(present == 0xffffffff) {
      status = FLASH_ProgramWord(image->startAddress + (offset + i) * 4, data[i]);
      if(status != FLASH_COMPLETE) {
        retval = TLV_ERROR_HARDWARE_ERROR;
        break;
      }
    } else if(present == data[i]) {
      /* same data, do nothing */
    } else {
      DEBUG_PRINT_I("flash content not erase and not same as write request at address 0x%08lx\n",
                    image->startAddress + (offset + i) * 4);
    }
  }

  FLASH_Lock();
  if(FLASH_WaitForLastOperation() != FLASH_COMPLETE) {
    DEBUG_PRINT_("writeFlash: Failed to lock flash\n");
    retval = TLV_ERROR_HARDWARE_ERROR;
  }

  return retval;
}
/*----------------------------------------------------------------*/
