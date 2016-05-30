/*
** ===================================================================
** Project  :
** Author   : $Author: flag/svn@YANZI.SE $
** File     : $URL: https://svn.yanzi.se/fiona/fiona/trunk/src/se/yanzi/progs/vargen/VargenOutC.java $
** Revision : $Revision: 15194 $
** Date     : $Date: 2014-09-03 17:09:59 +0200 (Wed, 03 Sep 2014) $
**
** Description : Vargen C header for TLV and embedded.
**
**               DO NOT EDIT, generated by VargenGen on
**               Sat Apr 18 18:05:58 CEST 2015
**
** Copyright (c) 2012-2014 Yanzi Networks. All Rights reserved. This file
** is the confidential and proprietary property of Yanzi Networks and
** the possession, use, duplication or transfer in any way of this
** file requires a written agreement from Yanzi Networks.
**
**===================================================================
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
/* ECC Communication key pair 2 x 256 bit. Offset 22 through 85 */
#define NVM_ENCRYPTION_COMMUNICATION_PUBLIC_KEY         (NVM_START+22)
#define NVM_ENCRYPTION_COMMUNICATION_PUBLIC_KEY_SIZE_B  32
#define NVM_ENCRYPTION_COMMUNICATION_PRIVATE_KEY        (NVM_START+54)
#define NVM_ENCRYPTION_COMMUNICATION_PRIVATE_KEY_SIZE_B 32
/* ECC Location public key 256 bit. Offset 86 through 117 */
#define NVM_ENCRYPTION_LOCATION_PUBLIC_KEY              (NVM_START+86)
#define NVM_ENCRYPTION_LOCATION_PUBLIC_KEY_SIZE_B       32
/* ECC Owner public key 256 bit. Offset 118 through 149 */
#define NVM_ENCRYPTION_OWNER_PUBLIC_KEY                 (NVM_START+118)
#define NVM_ENCRYPTION_OWNER_PUBLIC_KEY_SIZE_B          32
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

typedef enum {
    BORDER_ROUTER_STATUS_CONFIGURATION_READY = 1,
    BORDER_ROUTER_STATUS_CONFIGURATION_HAS_STATUS_BITS = 2,
    BORDER_ROUTER_STATUS_CONFIGURATION_PAN_ID = 4,
    BORDER_ROUTER_STATUS_CONFIGURATION_BEACON = 8,
    BORDER_ROUTER_STATUS_CONFIGURATION_MODE = 16,
} border_router_status_configuration;

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
    /* Product 0x0090DA030201001F "native virtual chassis" */
    { 0x0090DA030201001FULL, 0x000,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_TYPE */
    { 0x0090DA030201001FULL, 0x001, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_ID */
    { 0x0090DA030201001FULL, 0x002, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_PRODUCT_LABEL */
    { 0x0090DA030201001FULL, 0x003,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NUMBER_OF_INSTANCES */
    { 0x0090DA030201001FULL, 0x004,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_SUB_TYPE */
    { 0x0090DA030201001FULL, 0x005,  4, WRITABILITY_RW, FORMAT_INTEGER,  2 }, /* VARIABLE_EVENT_ARRAY */
    { 0x0090DA030201001FULL, 0x0c0,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_CONTROLLER_WATCHDOG */
    { 0x0090DA030201001FULL, 0x0c1,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_CONTROLLER_STATUS */
    { 0x0090DA030201001FULL, 0x0c2, 32, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_CONTROLLER_ADDRESS */
    { 0x0090DA030201001FULL, 0x0c3,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_COMMUNICATION_STYLE */
    { 0x0090DA030201001FULL, 0x0c9,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_BOOT_TIMER */
    { 0x0090DA030201001FULL, 0x0cc, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_SW_REVISION */
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