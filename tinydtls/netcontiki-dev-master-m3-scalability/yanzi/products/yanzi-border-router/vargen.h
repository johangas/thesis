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
 * Vargen C header for TLV and embedded.
 *
 * DO NOT EDIT, generated by VargenGen on
 * Thu Jan 15 17:38:46 CET 2015
 *
 */

#ifndef VARGEN_H_
#define VARGEN_H_
#include <stdint.h>

#define TRUE 1
#define FALSE 0

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#ifndef NULL
#define NULL 0
#endif /* NULL */

/* Variables defined in Non-Volatile Memory area */
#define NVM_START                            (NVM_MGMT_SIZE_B+0x0040)
/* owner and peering information (location) */
#define NVM_OWNER_ID_OFFSET                             (NVM_START)
#define NVM_OWNER_ID_SIZE_B                              4
#define NVM_RESERVED1                                   (NVM_START+4)
#define NVM_RESERVED1_SIZE_B                             8
#define NVM_LOCATION_ID_64_OFFSET                       (NVM_START+12)
#define NVM_LOCATION_ID_64_SIZE_B                        8
/* Short location ID is last 4 bytes of long location ID. */
#define NVM_LOCATION_ID_OFFSET                          (NVM_START+16)
#define NVM_LOCATION_ID_SIZE_B                           4
/* Netselect channel bitmask */
#define NVM_CHANNEL_BITMASK                             (NVM_START+20)
#define NVM_CHANNEL_BITMASK_SIZE_B                       2
#define NVM_MFG_TEST_PROGRESS                           (NVM_START+150)
#define NVM_MFG_TEST_PROGRESS_SIZE_B                    2


#define TLV_VERSION 0
#define TLV_OPCODE_VECTOR_MASK 0x80
#define TLV_OPCODE_REPLY_MASK 0x01

typedef enum {
    TLV_OPCODE_GET_REQUEST = 0x00,
    TLV_OPCODE_GET_RESPONSE = 0x01,
    TLV_OPCODE_SET_REQUEST = 0x02,
    TLV_OPCODE_SET_RESPONSE = 0x03,
    TLV_OPCODE_REFLECT_REQUEST = 0x04,
    TLV_OPCODE_REFLECT_RESPONSE = 0x05,
    TLV_OPCODE_EVENT_REQUEST = 0x06,
    TLV_OPCODE_EVENT_RESPONSE = 0x07,
    TLV_OPCODE_MASKED_SET_REQUEST = 0x08,
    TLV_OPCODE_MASKED_SET_RESPONSE = 0x09,
    TLV_OPCODE_ABORT_IF_EQUAL_REQUEST = 0x0a,
    TLV_OPCODE_ABORT_IF_EQUAL_RESPONSE = 0x0b,
    TLV_OPCODE_BLOB_REQUEST = 0x0c,
    TLV_OPCODE_BLOB_RESPONSE = 0x0d,
    TLV_OPCODE_VECTOR_GET_REQUEST = 0x80,
    TLV_OPCODE_VECTOR_GET_RESPONSE = 0x81,
    TLV_OPCODE_VECTOR_SET_REQUEST = 0x82,
    TLV_OPCODE_VECTOR_SET_RESPONSE = 0x83,
    TLV_OPCODE_VECTOR_REFLECT_REQUEST = 0x84,
    TLV_OPCODE_VECTOR_REFLECT_RESPONSE = 0x85,
    TLV_OPCODE_VECTOR_EVENT_REQUEST = 0x86,
    TLV_OPCODE_VECTOR_EVENT_RESPONSE = 0x87,
    TLV_OPCODE_VECTOR_MASKED_SET_REQUEST = 0x88,
    TLV_OPCODE_VECTOR_MASKED_SET_RESPONSE = 0x89,
    TLV_OPCODE_VECTOR_ABORT_IF_EQUAL_REQUEST = 0x8a,
    TLV_OPCODE_VECTOR_ABORT_IF_EQUAL_RESPONSE = 0x8b,
    TLV_OPCODE_VECTOR_BLOB_REQUEST = 0x8c,
    TLV_OPCODE_VECTOR_BLOB_RESPONSE = 0x8d,
} TLVopcode;

typedef enum {
    TLV_ERROR_NO_ERROR = 0,
    TLV_ERROR_UNKNOWN_VERSION = 1,
    TLV_ERROR_UNKNOWN_VARIABLE = 2,
    TLV_ERROR_UNKNOWN_INSTANCE = 3,
    TLV_ERROR_UNKNOWN_OP_CODE = 4,
    TLV_ERROR_UNKNOWN_ELEMENT_SIZE = 5,
    TLV_ERROR_BAD_NUMBER_OF_ELEMENTS = 6,
    TLV_ERROR_TIMEOUT = 7,
    TLV_ERROR_DEVICE_BUSY = 8,
    TLV_ERROR_HARDWARE_ERROR = 9,
    TLV_ERROR_BAD_LENGTH = 10,
    TLV_ERROR_WRITE_ACCESS_DENIED = 11,
    TLV_ERROR_UNKNOWN_BLOB_COMMAND = 12,
    TLV_ERROR_NO_VECTOR_ACCESS = 13,
    TLV_ERROR_UNEXPECTED_RESPONSE = 14,
    TLV_ERROR_INVALID_VECTOR_OFFSET = 15,
    TLV_ERROR_INVALID_ARGUMENT = 16,
    TLV_ERROR_READ_ACCESS_DENIED = 17,
    TLV_ERROR_UNPROCESSED_TLV = 18,
} TLVerror;

#ifdef __VARGEN_CODE__
/*
 * Return true if this opcode is carrying data.
 */
uint8_t
withData(TLVopcode o) {

    uint8_t retval;
    switch (o) {
    case TLV_OPCODE_GET_RESPONSE:
    case TLV_OPCODE_SET_REQUEST:
    case TLV_OPCODE_REFLECT_REQUEST:
    case TLV_OPCODE_REFLECT_RESPONSE:
    case TLV_OPCODE_MASKED_SET_REQUEST:
    case TLV_OPCODE_ABORT_IF_EQUAL_REQUEST:
    case TLV_OPCODE_BLOB_REQUEST:
    case TLV_OPCODE_BLOB_RESPONSE:
    case TLV_OPCODE_VECTOR_GET_RESPONSE:
    case TLV_OPCODE_VECTOR_SET_REQUEST:
    case TLV_OPCODE_VECTOR_REFLECT_REQUEST:
    case TLV_OPCODE_VECTOR_REFLECT_RESPONSE:
    case TLV_OPCODE_VECTOR_EVENT_RESPONSE:
    case TLV_OPCODE_VECTOR_MASKED_SET_REQUEST:
    case TLV_OPCODE_VECTOR_ABORT_IF_EQUAL_REQUEST:
    case TLV_OPCODE_VECTOR_BLOB_REQUEST:
    case TLV_OPCODE_VECTOR_BLOB_RESPONSE:
        retval = TRUE;
        break;

    case TLV_OPCODE_GET_REQUEST:
    case TLV_OPCODE_SET_RESPONSE:
    case TLV_OPCODE_EVENT_REQUEST:
    case TLV_OPCODE_EVENT_RESPONSE:
    case TLV_OPCODE_MASKED_SET_RESPONSE:
    case TLV_OPCODE_ABORT_IF_EQUAL_RESPONSE:
    case TLV_OPCODE_VECTOR_GET_REQUEST:
    case TLV_OPCODE_VECTOR_SET_RESPONSE:
    case TLV_OPCODE_VECTOR_EVENT_REQUEST:
    case TLV_OPCODE_VECTOR_MASKED_SET_RESPONSE:
    case TLV_OPCODE_VECTOR_ABORT_IF_EQUAL_RESPONSE:
        retval = FALSE;
        break;

    default:
        retval = FALSE;
        break;
    }
    return retval;
}
#endif /* __VARGEN_CODE__ */

typedef struct {
    uint32_t offset;
    uint32_t elements;
    uint8_t *data;
    uint16_t length;
    uint16_t variable;
    uint8_t version;
    uint8_t instance;
    uint8_t opcode;
    uint8_t elementSize;
    uint8_t error;
} TLV;

typedef enum {
    WRITABILITY_RO,
    WRITABILITY_RW,
    WRITABILITY_WO,
} writabilty;

typedef enum {
    FORMAT_ARRAY,
    FORMAT_ENUM,
    FORMAT_INTEGER,
    FORMAT_STRING,
    FORMAT_BLOB,
    FORMAT_TIME,
} format;
#define HAVE_FORMAT_TIME

#define GLOBAL_CHASSIS_CAPABILITY_MASK          0x8000000000000000ULL
#define GLOBAL_CHASSIS_CAPABILITY_CAN_SLEEP     0x01ULL
#define GLOBAL_CHASSIS_CAPABILITY_CTS_RTS       0x02ULL
#define GLOBAL_CHASSIS_CAPABILITY_MISSING_FAN   0x04ULL

typedef enum {
    EVENTSTATE_EVENT_DISARMED = 0,
    EVENTSTATE_EVENT_ARMED = 1,
    EVENTSTATE_EVENT_TRIGGED = 3,
} eventState;
#define EVENTSTATE_OFFSET0_ENABLE_MASK 1
#define EVENTSTATE_OFFSET0_TRIGGED_MASK 2

typedef enum {
    EVENTACTION_EVENT_ARM = 1,
    EVENTACTION_EVENT_DISARM = 0,
    EVENTACTION_EVENT_REARM = 2,
} eventAction;

typedef struct {
    uint64_t productType;
    uint16_t number;
    uint8_t  elementSize;
    uint8_t  writability;
    uint8_t  format;
    uint8_t  vectorDepth;
} variable;

typedef struct {
    uint8_t address[16];
    uint8_t next_hop[16];
    uint8_t reserved[2];
    uint8_t metric;
    uint8_t is_router;
    uint8_t padding[28];
} route_table_entry;

typedef enum {
    BORDER_ROUTER_STATUS_CONFIGURATION_READY = 1,
    BORDER_ROUTER_STATUS_CONFIGURATION_HAS_STATUS_BITS = 2,
    BORDER_ROUTER_STATUS_CONFIGURATION_PAN_ID = 4,
    BORDER_ROUTER_STATUS_CONFIGURATION_BEACON = 8,
    BORDER_ROUTER_STATUS_CONFIGURATION_MODE = 16,
} border_router_status_configuration;

typedef enum {
    BRM_COMMAND_NOP = 0,
    BRM_COMMAND_RPL_GLOBAL_REPAIR = 1,
} brm_command;

/* Variable names */
#define VARIABLE_PRODUCT_TYPE 0x000
#define VARIABLE_PRODUCT_ID 0x001
#define VARIABLE_PRODUCT_LABEL 0x002
#define VARIABLE_NUMBER_OF_INSTANCES 0x003
#define VARIABLE_PRODUCT_SUB_TYPE 0x004
#define VARIABLE_EVENT_ARRAY 0x005
#define VARIABLE_UNIT_CONTROLLER_WATCHDOG 0x0c0
#define VARIABLE_UNIT_CONTROLLER_STATUS 0x0c1
#define VARIABLE_UNIT_CONTROLLER_ADDRESS 0x0c2
#define VARIABLE_COMMUNICATION_STYLE 0x0c3
#define VARIABLE_UNIT_BOOT_TIMER 0x0c9
#define VARIABLE_SW_REVISION 0x0cc
#define VARIABLE_INTERFACE_NAME 0x100
#define VARIABLE_ROUTER_ADDRESS 0x101
#define VARIABLE_ROUTER_ADDRESS_PREFIX_LEN 0x102
#define VARIABLE_OLD_UNIT_BOOT_TIMER 0x105
#define VARIABLE_ROUTER_READY 0x106
#define VARIABLE_OLD_SW_REVISION 0x10c
#define VARIABLE_TABLE_LENGTH 0x100
#define VARIABLE_TABLE_REVISION 0x101
#define VARIABLE_TABLE 0x102
#define VARIABLE_NETWORK_ADDRESS 0x103
#define VARIABLE_SERIAL_INTERFACE_NAME 0x101
#define VARIABLE_RADIO_CONTROL_API_VERSION 0x102
#define VARIABLE_RADIO_SERIAL_MODE 0x103
#define VARIABLE_RADIO_SERIAL_SEND_DELAY 0x104
#define VARIABLE_RADIO_SW_REVISION 0x105
#define VARIABLE_RADIO_BOOTLOADER_VERSION 0x106
#define VARIABLE_RADIO_CHASSIS_CAPABILITIES 0x107
#define VARIABLE_BRM_STAT_LENGTH 0x108
#define VARIABLE_BRM_STAT_DATA 0x109
#define VARIABLE_BRM_COMMAND 0x10a
#define VARIABLE_BRM_RPL_DAG_VERSION 0x10b
#define VARIABLE_NSTATS_VERSION 0x100
#define VARIABLE_NSTATS_CAPABILITIES 0x101
#define VARIABLE_NSTATS_PUSH_PERIOD 0x102
#define VARIABLE_NSTATS_PUSH_TIME 0x103
#define VARIABLE_NSTATS_PUSH_PORT 0x104
#define VARIABLE_NSTATS_RECOMMENDED_PARENT 0x105
#define VARIABLE_NSTATS_DATA 0x106

#define VECTOR_DEPTH_DONT_CHECK 0xff

#ifdef __VARGEN_CODE__
const variable allVariables[] = {
    /* Product 0x0090DA0302010014 "Border router" */
    { 0x0090DA0302010014ULL, 0x000,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_TYPE */ 
    { 0x0090DA0302010014ULL, 0x001, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_ID */ 
    { 0x0090DA0302010014ULL, 0x002, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_PRODUCT_LABEL */ 
    { 0x0090DA0302010014ULL, 0x003,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NUMBER_OF_INSTANCES */ 
    { 0x0090DA0302010014ULL, 0x004,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_SUB_TYPE */ 
    { 0x0090DA0302010014ULL, 0x005,  4, WRITABILITY_RW, FORMAT_INTEGER,  2 }, /* VARIABLE_EVENT_ARRAY */ 
    { 0x0090DA0302010014ULL, 0x0c0,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_CONTROLLER_WATCHDOG */ 
    { 0x0090DA0302010014ULL, 0x0c1,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_CONTROLLER_STATUS */ 
    { 0x0090DA0302010014ULL, 0x0c2, 32, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_CONTROLLER_ADDRESS */ 
    { 0x0090DA0302010014ULL, 0x0c3,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_COMMUNICATION_STYLE */ 
    { 0x0090DA0302010014ULL, 0x0c9,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_BOOT_TIMER */ 
    { 0x0090DA0302010014ULL, 0x0cc, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_SW_REVISION */ 
    { 0x0090DA0302010014ULL, 0x100, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_INTERFACE_NAME */ 
    { 0x0090DA0302010014ULL, 0x101, 16, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_ROUTER_ADDRESS */ 
    { 0x0090DA0302010014ULL, 0x102,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_ROUTER_ADDRESS_PREFIX_LEN */ 
    { 0x0090DA0302010014ULL, 0x105,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_OLD_UNIT_BOOT_TIMER */ 
    { 0x0090DA0302010014ULL, 0x106,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_ROUTER_READY */ 
    { 0x0090DA0302010014ULL, 0x10c, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_OLD_SW_REVISION */ 
    /* Product 0x0090DA0302010015 "Routing table" */
    { 0x0090DA0302010015ULL, 0x000,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_TYPE */ 
    { 0x0090DA0302010015ULL, 0x001, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_ID */ 
    { 0x0090DA0302010015ULL, 0x002, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_PRODUCT_LABEL */ 
    { 0x0090DA0302010015ULL, 0x003,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NUMBER_OF_INSTANCES */ 
    { 0x0090DA0302010015ULL, 0x004,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_SUB_TYPE */ 
    { 0x0090DA0302010015ULL, 0x005,  4, WRITABILITY_RW, FORMAT_INTEGER,  2 }, /* VARIABLE_EVENT_ARRAY */ 
    { 0x0090DA0302010015ULL, 0x100,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_TABLE_LENGTH */ 
    { 0x0090DA0302010015ULL, 0x101,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_TABLE_REVISION */ 
    { 0x0090DA0302010015ULL, 0x102, 64, WRITABILITY_RO, FORMAT_ARRAY  , VECTOR_DEPTH_DONT_CHECK }, /* VARIABLE_TABLE */ 
    { 0x0090DA0302010015ULL, 0x103, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NETWORK_ADDRESS */ 
    /* Product 0x0090DA0303010022 "Border Router Management" */
    { 0x0090DA0303010022ULL, 0x000,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_TYPE */ 
    { 0x0090DA0303010022ULL, 0x001, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_ID */ 
    { 0x0090DA0303010022ULL, 0x002, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_PRODUCT_LABEL */ 
    { 0x0090DA0303010022ULL, 0x003,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NUMBER_OF_INSTANCES */ 
    { 0x0090DA0303010022ULL, 0x004,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_SUB_TYPE */ 
    { 0x0090DA0303010022ULL, 0x005,  4, WRITABILITY_RW, FORMAT_INTEGER,  1 }, /* VARIABLE_EVENT_ARRAY */ 
    { 0x0090DA0303010022ULL, 0x100, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_INTERFACE_NAME */ 
    { 0x0090DA0303010022ULL, 0x101, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_SERIAL_INTERFACE_NAME */ 
    { 0x0090DA0303010022ULL, 0x102,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_CONTROL_API_VERSION */ 
    { 0x0090DA0303010022ULL, 0x103,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_SERIAL_MODE */ 
    { 0x0090DA0303010022ULL, 0x104,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_SERIAL_SEND_DELAY */ 
    { 0x0090DA0303010022ULL, 0x105, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_SW_REVISION */ 
    { 0x0090DA0303010022ULL, 0x106,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_BOOTLOADER_VERSION */ 
    { 0x0090DA0303010022ULL, 0x107,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_CHASSIS_CAPABILITIES */ 
    { 0x0090DA0303010022ULL, 0x108,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_BRM_STAT_LENGTH */ 
    { 0x0090DA0303010022ULL, 0x109,  4, WRITABILITY_RO, FORMAT_ARRAY  , VECTOR_DEPTH_DONT_CHECK }, /* VARIABLE_BRM_STAT_DATA */ 
    { 0x0090DA0303010022ULL, 0x10a,  4, WRITABILITY_WO, FORMAT_INTEGER,  0 }, /* VARIABLE_BRM_COMMAND */ 
    { 0x0090DA0303010022ULL, 0x10b,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_BRM_RPL_DAG_VERSION */ 
    /* Product 0x0090DA0303010023 "Network Statistics" */
    { 0x0090DA0303010023ULL, 0x000,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_TYPE */ 
    { 0x0090DA0303010023ULL, 0x001, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_ID */ 
    { 0x0090DA0303010023ULL, 0x002, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_PRODUCT_LABEL */ 
    { 0x0090DA0303010023ULL, 0x003,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NUMBER_OF_INSTANCES */ 
    { 0x0090DA0303010023ULL, 0x004,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_SUB_TYPE */ 
    { 0x0090DA0303010023ULL, 0x005,  4, WRITABILITY_RW, FORMAT_INTEGER,  1 }, /* VARIABLE_EVENT_ARRAY */ 
    { 0x0090DA0303010023ULL, 0x100,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NSTATS_VERSION */ 
    { 0x0090DA0303010023ULL, 0x101,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NSTATS_CAPABILITIES */ 
    { 0x0090DA0303010023ULL, 0x102,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_NSTATS_PUSH_PERIOD */ 
    { 0x0090DA0303010023ULL, 0x103,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_NSTATS_PUSH_TIME */ 
    { 0x0090DA0303010023ULL, 0x104,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_NSTATS_PUSH_PORT */ 
    { 0x0090DA0303010023ULL, 0x105, 16, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_NSTATS_RECOMMENDED_PARENT */ 
    { 0x0090DA0303010023ULL, 0x106,  4, WRITABILITY_RW, FORMAT_ARRAY  , VECTOR_DEPTH_DONT_CHECK }, /* VARIABLE_NSTATS_DATA */ 
#ifdef VARGEN_ADDITIONAL_INSTANCES
    VARGEN_ADDITIONAL_INSTANCES
#endif /* VARGEN_ADDITIONAL_INSTANCES */
};
#endif /* __VARGEN_CODE__ */

uint8_t withData(TLVopcode o);
#endif /* VARGEN_H_ */