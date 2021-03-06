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

typedef enum {
    RADIO_EXT_RESET_CAUSE_NONE = 0,
    RADIO_EXT_RESET_CAUSE_NORMAL = 1,
    RADIO_EXT_RESET_CAUSE_WATCHDOG = 2,
} radio_ext_reset_cause;

typedef enum {
    RADIO_RESET_CAUSE_NONE = 0,
    RADIO_RESET_CAUSE_LOCKUP = 1,
    RADIO_RESET_CAUSE_DEEP_SLEEP = 2,
    RADIO_RESET_CAUSE_SOFTWARE = 3,
    RADIO_RESET_CAUSE_WATCHDOG = 4,
    RADIO_RESET_CAUSE_HARDWARE = 5,
    RADIO_RESET_CAUSE_POWER_FAIL = 6,
    RADIO_RESET_CAUSE_COLD_START = 7,
} radio_reset_cause;

typedef enum {
    RADIO_FRONTPANEL_INFO_OFF = 0,
    RADIO_FRONTPANEL_INFO_YELLOW = 1,
    RADIO_FRONTPANEL_INFO_YELLOW_05HZ = 2,
    RADIO_FRONTPANEL_INFO_YELLOW_1HZ = 3,
    RADIO_FRONTPANEL_INFO_YELLOW_2HZ = 4,
    RADIO_FRONTPANEL_INFO_YELLOW_4HZ = 5,
    RADIO_FRONTPANEL_INFO_GREEN = 6,
    RADIO_FRONTPANEL_INFO_GREEN_05HZ = 7,
    RADIO_FRONTPANEL_INFO_GREEN_1HZ = 8,
    RADIO_FRONTPANEL_INFO_GREEN_2HZ = 9,
    RADIO_FRONTPANEL_INFO_GREEN_4HZ = 10,
} radio_frontpanel_info;

typedef enum {
    RADIO_SUPPLY_STATUS_UNKNOWN = 0,
    RADIO_SUPPLY_STATUS_USING_GRID = 1,
    RADIO_SUPPLY_STATUS_USING_GRID_AND_CHARGING_UPS = 2,
    RADIO_SUPPLY_STATUS_USING_UPS = 3,
    RADIO_SUPPLY_STATUS_USING_BATTERY = 4,
} radio_supply_status;

typedef enum {
    RADIO_SUPPLY_TYPE_UNKNOWN = 0,
    RADIO_SUPPLY_TYPE_GRID = 1,
    RADIO_SUPPLY_TYPE_UPS = 2,
    RADIO_SUPPLY_TYPE_BATTERY = 3,
} radio_supply_type;

typedef enum {
    RADIO_MODE_IDLE = 0,
    RADIO_MODE_NORMAL = 1,
    RADIO_MODE_SNIFFER = 2,
    RADIO_MODE_SCAN = 3,
} radio_mode;

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
#define VARIABLE_CONFIGURATION_STATUS 0x0c8
#define VARIABLE_UNIT_BOOT_TIMER 0x0c9
#define VARIABLE_HARDWARE_RESET 0x0ca
#define VARIABLE_READ_THIS_COUNTER 0x0cb
#define VARIABLE_SW_REVISION 0x0cc
#define VARIABLE_TLV_GROUPING 0x0cd
#define VARIABLE_LOCATION_ID 0x0ce
#define VARIABLE_OWNER_ID 0x0cf
#define VARIABLE_SLEEP_DEFAULT_AWAKE_TIME 0x0d0
#define VARIABLE_CHASSIS_CAPABILITIES 0x0e0
#define VARIABLE_CHASSIS_BOOTLOADER_VERSION 0x0e1
#define VARIABLE_CHASSIS_FACTORY_DEFAULT 0x0e2
#define VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX 0x0e3
#define VARIABLE_IDENTIFY_CHASSIS 0x0e4
#define VARIABLE_PSP_PORTAL_STATUS 0x10d
#define VARIABLE_OLD_UNIT_CONTROLLER_WATCHDOG 0x104
#define VARIABLE_OLD_UNIT_BOOT_TIMER 0x105
#define VARIABLE_OLD_SW_REVISION 0x10c
#define VARIABLE_FLASH 0x100
#define VARIABLE_WRITE_CONTROL 0x101
#define VARIABLE_IMAGE_START_ADDRESS 0x102
#define VARIABLE_IMAGE_MAX_LENGTH 0x103
#define VARIABLE_IMAGE_ACCEPTED_TYPE 0x104
#define VARIABLE_IMAGE_STATUS 0x105
#define VARIABLE_IMAGE_VERSION 0x106
#define VARIABLE_IMAGE_LENGTH 0x107
#define VARIABLE_RADIO_CHANNEL 0x100
#define VARIABLE_RADIO_PAN_ID 0x101
#define VARIABLE_RADIO_BEACON_RESPONSE 0x102
#define VARIABLE_RADIO_MODE 0x103
#define VARIABLE_RADIO_SCAN_MODE 0x104
#define VARIABLE_RADIO_SERIAL_MODE 0x105
#define VARIABLE_RADIO_STAT_LENGTH 0x106
#define VARIABLE_RADIO_STAT_DATA 0x107
#define VARIABLE_RADIO_CONTROL_API_VERSION 0x108
#define VARIABLE_RADIO_SUPPLY_STATUS 0x109
#define VARIABLE_RADIO_BATTERY_SUPPLY_TIME 0x10a
#define VARIABLE_RADIO_SUPPLY_VOLTAGE 0x10b
#define VARIABLE_RADIO_FRONTPANEL_INFO 0x10c
#define VARIABLE_RADIO_WATCHDOG 0x10d
#define VARIABLE_RADIO_RESET_CAUSE 0x10e
#define VARIABLE_RADIO_ERROR_CODE 0x10f

#define VECTOR_DEPTH_DONT_CHECK 0xff

#ifdef __VARGEN_CODE__
const variable allVariables[] = {
    /* Product 0x0090DA0301010482 "Serial radio" */
    { 0x0090DA0301010482ULL, 0x000,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_TYPE */ 
    { 0x0090DA0301010482ULL, 0x001, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_ID */ 
    { 0x0090DA0301010482ULL, 0x002, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_PRODUCT_LABEL */ 
    { 0x0090DA0301010482ULL, 0x003,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NUMBER_OF_INSTANCES */ 
    { 0x0090DA0301010482ULL, 0x004,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_SUB_TYPE */ 
    { 0x0090DA0301010482ULL, 0x005,  4, WRITABILITY_RW, FORMAT_INTEGER,  2 }, /* VARIABLE_EVENT_ARRAY */ 
    { 0x0090DA0301010482ULL, 0x0c0,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_CONTROLLER_WATCHDOG */ 
    { 0x0090DA0301010482ULL, 0x0c1,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_CONTROLLER_STATUS */ 
    { 0x0090DA0301010482ULL, 0x0c2, 32, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_CONTROLLER_ADDRESS */ 
    { 0x0090DA0301010482ULL, 0x0c3,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_COMMUNICATION_STYLE */ 
    { 0x0090DA0301010482ULL, 0x0c8,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_CONFIGURATION_STATUS */ 
    { 0x0090DA0301010482ULL, 0x0c9,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_UNIT_BOOT_TIMER */ 
    { 0x0090DA0301010482ULL, 0x0ca,  4, WRITABILITY_WO, FORMAT_INTEGER,  0 }, /* VARIABLE_HARDWARE_RESET */ 
    { 0x0090DA0301010482ULL, 0x0cb,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_READ_THIS_COUNTER */ 
    { 0x0090DA0301010482ULL, 0x0cc, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_SW_REVISION */ 
    { 0x0090DA0301010482ULL, 0x0cd,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_TLV_GROUPING */ 
    { 0x0090DA0301010482ULL, 0x0ce,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_LOCATION_ID */ 
    { 0x0090DA0301010482ULL, 0x0cf,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_OWNER_ID */ 
    { 0x0090DA0301010482ULL, 0x0d0,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_SLEEP_DEFAULT_AWAKE_TIME */ 
    { 0x0090DA0301010482ULL, 0x0e0,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_CHASSIS_CAPABILITIES */ 
    { 0x0090DA0301010482ULL, 0x0e1,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_CHASSIS_BOOTLOADER_VERSION */ 
    { 0x0090DA0301010482ULL, 0x0e2,  4, WRITABILITY_WO, FORMAT_INTEGER,  0 }, /* VARIABLE_CHASSIS_FACTORY_DEFAULT */ 
    { 0x0090DA0301010482ULL, 0x0e3,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX */ 
    { 0x0090DA0301010482ULL, 0x0e4,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_IDENTIFY_CHASSIS */ 
    { 0x0090DA0301010482ULL, 0x10d,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PSP_PORTAL_STATUS */ 
    { 0x0090DA0301010482ULL, 0x104,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_OLD_UNIT_CONTROLLER_WATCHDOG */ 
    { 0x0090DA0301010482ULL, 0x105,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_OLD_UNIT_BOOT_TIMER */ 
    { 0x0090DA0301010482ULL, 0x10c, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_OLD_SW_REVISION */ 
    /* Product 0x0090DA0303010010 "Flash" */
    { 0x0090DA0303010010ULL, 0x000,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_TYPE */ 
    { 0x0090DA0303010010ULL, 0x001, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_ID */ 
    { 0x0090DA0303010010ULL, 0x002, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_PRODUCT_LABEL */ 
    { 0x0090DA0303010010ULL, 0x003,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NUMBER_OF_INSTANCES */ 
    { 0x0090DA0303010010ULL, 0x004,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_SUB_TYPE */ 
    { 0x0090DA0303010010ULL, 0x005,  4, WRITABILITY_RW, FORMAT_INTEGER,  1 }, /* VARIABLE_EVENT_ARRAY */ 
    { 0x0090DA0303010010ULL, 0x100,  4, WRITABILITY_RW, FORMAT_INTEGER, VECTOR_DEPTH_DONT_CHECK }, /* VARIABLE_FLASH */ 
    { 0x0090DA0303010010ULL, 0x101,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_WRITE_CONTROL */ 
    { 0x0090DA0303010010ULL, 0x102,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_IMAGE_START_ADDRESS */ 
    { 0x0090DA0303010010ULL, 0x103,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_IMAGE_MAX_LENGTH */ 
    { 0x0090DA0303010010ULL, 0x104,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_IMAGE_ACCEPTED_TYPE */ 
    { 0x0090DA0303010010ULL, 0x105,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_IMAGE_STATUS */ 
    { 0x0090DA0303010010ULL, 0x106,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_IMAGE_VERSION */ 
    { 0x0090DA0303010010ULL, 0x107,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_IMAGE_LENGTH */ 
    /* Product 0x0090DA0303010014 "802.15.4 radio" */
    { 0x0090DA0303010014ULL, 0x000,  8, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_TYPE */ 
    { 0x0090DA0303010014ULL, 0x001, 16, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_ID */ 
    { 0x0090DA0303010014ULL, 0x002, 32, WRITABILITY_RO, FORMAT_STRING ,  0 }, /* VARIABLE_PRODUCT_LABEL */ 
    { 0x0090DA0303010014ULL, 0x003,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_NUMBER_OF_INSTANCES */ 
    { 0x0090DA0303010014ULL, 0x004,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_PRODUCT_SUB_TYPE */ 
    { 0x0090DA0303010014ULL, 0x005,  4, WRITABILITY_RW, FORMAT_INTEGER,  1 }, /* VARIABLE_EVENT_ARRAY */ 
    { 0x0090DA0303010014ULL, 0x100,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_CHANNEL */ 
    { 0x0090DA0303010014ULL, 0x101,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_PAN_ID */ 
    { 0x0090DA0303010014ULL, 0x102,  4, WRITABILITY_RW, FORMAT_ARRAY  , VECTOR_DEPTH_DONT_CHECK }, /* VARIABLE_RADIO_BEACON_RESPONSE */ 
    { 0x0090DA0303010014ULL, 0x103,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_MODE */ 
    { 0x0090DA0303010014ULL, 0x104,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_SCAN_MODE */ 
    { 0x0090DA0303010014ULL, 0x105,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_SERIAL_MODE */ 
    { 0x0090DA0303010014ULL, 0x106,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_STAT_LENGTH */ 
    { 0x0090DA0303010014ULL, 0x107,  4, WRITABILITY_RO, FORMAT_ARRAY  , VECTOR_DEPTH_DONT_CHECK }, /* VARIABLE_RADIO_STAT_DATA */ 
    { 0x0090DA0303010014ULL, 0x108,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_CONTROL_API_VERSION */ 
    { 0x0090DA0303010014ULL, 0x109,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_SUPPLY_STATUS */ 
    { 0x0090DA0303010014ULL, 0x10a,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_BATTERY_SUPPLY_TIME */ 
    { 0x0090DA0303010014ULL, 0x10b,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_SUPPLY_VOLTAGE */ 
    { 0x0090DA0303010014ULL, 0x10c,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_FRONTPANEL_INFO */ 
    { 0x0090DA0303010014ULL, 0x10d,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_WATCHDOG */ 
    { 0x0090DA0303010014ULL, 0x10e,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_RESET_CAUSE */ 
    { 0x0090DA0303010014ULL, 0x10f,  4, WRITABILITY_RW, FORMAT_INTEGER,  0 }, /* VARIABLE_RADIO_ERROR_CODE */ 
#ifdef VARGEN_ADDITIONAL_INSTANCES
    VARGEN_ADDITIONAL_INSTANCES
#endif /* VARGEN_ADDITIONAL_INSTANCES */
};
#endif /* __VARGEN_CODE__ */

uint8_t withData(TLVopcode o);
#endif /* VARGEN_H_ */
