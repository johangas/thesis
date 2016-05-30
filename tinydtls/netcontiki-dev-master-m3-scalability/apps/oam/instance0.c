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
#include "string.h"
#include "oam.h"
#include "api.h"
#include "vargen.h"
#include "net/rpl/rpl-private.h"
#include "net/ipv6/uip-ds6-route.h"

#ifdef HAVE_FRONTPANEL
#include "frontpanel.h"
#endif /* HAVE_FRONTPANEL */
#ifdef INSTANCE_LAMP
#include "instance-lamp.h"
#endif /* INSTANCE_LAMP */

#if OAM_MIN_DIS_INTERVAL_MS < 1000
#error Too small min dis interval. Please set OAM_CONF_MIN_DIS_INTERVAL_MS.
#endif /* OAM_MIN_DIS_INTERVAL_MS < 1000 */

static uint32_t (*get_awake_time_from_sleep)(void) = NULL;
static void (*watchdog_address_set)(void) = NULL;
static void instance0_route_callback(int event, uip_ipaddr_t *route, uip_ipaddr_t *ipaddr, int num);

static struct uip_ds6_notification route_notification;
static uip_ipaddr_t defrt;

/*
 * set get_awake_time function to use.
 */
void
set_get_awake_time_function(uint32_t (*func)(void))
{
  get_awake_time_from_sleep = func;
}

/*
 * set watchdog_address_set function to use.
 */
void
set_watchdog_address_set_function(void (*func)(void))
{
  watchdog_address_set = func;
}

/* local prototypes */
int process_request(uint8_t *request, size_t len, uint8_t *reply, size_t replymax, uint8_t PSP);
size_t generatePSPrequest(uint8_t *reply, size_t len);

extern uint8_t oamPeriodic;
extern uint8_t printRxReflections;
extern volatile uint8_t ssdp_announce_now;
extern uint8_t print_traffic;
#ifdef VARIABLE_TLV_GROUPING
extern uint64_t tlv_grouping_time;
extern uint32_t tlv_grouping_holdoff;
#endif /* VARIABLE_TLV_GROUPING */

static oam_do_action_handle action_handle;
#define FACTORY_DEFAULT_STATE_0 0
#define FACTORY_DEFAULT_STATE_1 1
#define FACTORY_DEFAULT_KEY_0 0x567190
#define FACTORY_DEFAULT_KEY_1 0x5

static uint8_t factory_default_state = FACTORY_DEFAULT_STATE_0;

static uint64_t last_wd_expire = 0;

/**
 * How to use the OAM module:
 *
 * 1: In your applications project-conf.h add
 *
 * #define OAM_PORT 49111
 *
 * #define PRODUCT_TYPE_INT64 0x0090DA0301010470ULL
 * #define PRODUCT_LABEL "The product label that will be show in UI"
 * #define INSTANCES 4 // number of instances including instance 0
 * #define PRIMARY_FLASH_INSTANCE 1
 * #define BACKUP_FLASH_INSTANCE 2
 * ... any other instances defined by application.
 *
 *
 * 
 * 2: When adding a new instance, Implement initialzation function.
 *    call it anything.  The init function takes one argument, an
 *    array if instanceControl. Like So:
 *
 *    void anything_init(instanceControl[]);
 * 
 *    The init function should fill in the instanceControl struct in
 *    the array supplied as argument. Index is instance number. If the
 *    new module handles more than one instance, fill in all that is
 *    handled.
 *
 *    Add the init function to an array of init functions that is
 *    called just before starting oam in main application. The array
 *    MUST end with a NULL element. Supply the array as argumen to
 *    oam_init(). Example:
 *
 *     static void (*initfuncs[3])(instanceControl[]);
 *
 *     initfuncs[0] = gpio_init;
 *     initfuncs[1] = temp_init;
 *     initfuncs[2] = NULL;
 *
 *     
 *   #ifdef HAVE_OAM
 *     oam_init(initfuncs);
 *     process_start(&oam, NULL);
 *   #endif
 *

 * 
 *
 * 4: Implement a processRequest function, see prototype in oam.h in
 *    struct instanceControl. This function is called for all TLVs
 *    that match the instance, and should write a reply TLV to "reply".
 *
 * 5: Implement a poll function, see prototype in oam.h in struct
 *    instanceControl. This function is called for periodic update and
 *    on event. It is not necessary to write a reply to a poll.
 */

#ifdef MAX_INSTANCES
instanceControl instances[MAX_INSTANCES];
#else
instanceControl instances[INSTANCES];
#endif /* MAX_INSTANCES */
/*
 * oam modules init prototypes
 */
void instance0_init(instanceControl[]);
void instance_flash_init(instanceControl[]);

/*
 * Initialize all oam modules.
 */
void
oam_init(void (*initfunc[])(instanceControl[]))
{
  static uint8_t init_done = FALSE;
  int i;

  if(init_done) {
    return;
  }

  memset(instances, 0, sizeof (instances));
  instance0_init(instances);
#ifdef HAVE_INSTANCE_flash
  instance_flash_init(instances);
#endif /* HAVE_INSTANCE_flash */
  if(initfunc) {
    for(i = 0; initfunc[i] != NULL; i++) {
      initfunc[i](instances);
    }
  }
  init_done = TRUE;
}
/* ----------------------------------------------------------------*/

instanceControl *
get_instances() {
  return instances;
}
/* ----------------------------------------------------------------*/

uint64_t
get_last_wd_expire() {
  return last_wd_expire;
}
/* ----------------------------------------------------------------*/

/*
 * Instance 0 implementation
 */
size_t
static process0Request(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing) {
  uint8_t error;
  uint8_t r[4];
#ifdef VARIABLE_READ_THIS_COUNTER
  static uint32_t read_this_counter = 0;
#endif /* VARIABLE_READ_THIS_COUNTER */

  if ((request->opcode == TLV_OPCODE_SET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_SET_REQUEST)) {
    /*
     *   #    #  #####      #     #####  ######
     *   #    #  #    #     #       #    #
     *   #    #  #    #     #       #    #####
     *   # ## #  #####      #       #    #
     *   ##  ##  #   #      #       #    #
     *   #    #  #    #     #       #    ######
     */
#ifdef VARIABLE_UNIT_CONTROLLER_ADDRESS
    if (request->variable == VARIABLE_UNIT_CONTROLLER_ADDRESS) {
      error = unitControllerSetAddress(request->data, FALSE);
      if (error == TLV_ERROR_NO_ERROR) {
        if(watchdog_address_set) {
          watchdog_address_set();
        }
        return writeReply256Tlv(request, reply, len, 0);
      } else {
        return writeErrorTlv(request, error, reply, len);
      }
    }
#endif /* VARIABLE_UNIT_CONTROLLER_ADDRESS */
#ifdef VARIABLE_UNIT_CONTROLLER_WATCHDOG
    if (request->variable == VARIABLE_UNIT_CONTROLLER_WATCHDOG) {
      error = unitControllerSetWatchdog(request->data, 0);
      if (error == TLV_ERROR_NO_ERROR) {
        return writeReply32Tlv(request, reply, len, 0);
      } else {
        return writeErrorTlv(request, error, reply, len);
      }
    }
#endif /* VARIABLE_UNIT_CONTROLLER_WATCHDOG */
#ifdef VARIABLE_OLD_UNIT_CONTROLLER_WATCHDOG
    if (request->variable == VARIABLE_OLD_UNIT_CONTROLLER_WATCHDOG) {
      error = unitControllerSetWatchdog(request->data, 0);
      if (error == TLV_ERROR_NO_ERROR) {
        return writeReply32Tlv(request, reply, len, 0);
      } else {
        return writeErrorTlv(request, error, reply, len);
      }
    }
#endif /* VARIABLE_OLD_UNIT_CONTROLLER_WATCHDOG */
#ifdef VARIABLE_HARDWARE_RESET
    if (request->variable == VARIABLE_HARDWARE_RESET) {
      if ((request->data[0] * 256 + request->data[1])  == 911) {
        platform_reboot_to_selected_image(request->data[3]);
        return writeReply32Tlv(request, reply, len, 0);
      } else if (request->data[3] != 0) {
        platform_reboot();
        /* Probably not reached */
        return writeReply32Tlv(request, reply, len, 0);
      } else {
        return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
    }
#endif /* VARIABLE_HARDWARE_RESET */
#ifdef VARIABLE_TLV_GROUPING
    /*
     * Data format: 
     *
     *   3               2               1
     *   1               3               5               7             0
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |   reply flag  |    reserved   |          holdoff ms           |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    if (request->variable == VARIABLE_TLV_GROUPING) {
      /* first clear hold-off time and call stackBegin to clear state */
      tlv_grouping_holdoff = 0;
      stackBegin();
      /* Then set new hold-off time */
      tlv_grouping_holdoff = (request->data[2] << 8) + request->data[3];
      tlv_grouping_time = boottimer_read();

      if (request->data[0] == 0) {
        return 0;
      }
      return writeReply32Tlv(request, reply, len, 0);
    }
#endif /* VARIABLE_TLV_GROUPING */
#ifdef VARIABLE_LOCATION_ID
    if (request->variable == VARIABLE_LOCATION_ID) {
      uint32_t local32 = get_int32_from_data(request->data);
      if (local32 == 0) {
        return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      if (set_location_id(local32)) {
        return writeReply32Tlv(request, reply, len, 0);
      } else {
        return writeErrorTlv(request, TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }
#endif /* VARIABLE_LOCATION_ID */
#ifdef VARIABLE_OWNER_ID
    if (request->variable == VARIABLE_OWNER_ID) {
      uint32_t local32 = get_int32_from_data(request->data);
      if (local32 == 0) {
        return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      if (set_owner_id(local32)) {
        return writeReply32Tlv(request, reply, len, 0);
      } else {
        return writeErrorTlv(request, TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }
#endif /* VARIABLE_OWNER_ID */
#ifdef VARIABLE_CHASSIS_FACTORY_DEFAULT
    if (request->variable == VARIABLE_CHASSIS_FACTORY_DEFAULT) {
      switch (factory_default_state) {
      case FACTORY_DEFAULT_STATE_0:
        if (get_int32_from_data(request->data) == FACTORY_DEFAULT_KEY_0) {
          factory_default_state = FACTORY_DEFAULT_STATE_1;
        } else {
          return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
        }
        ;;
      case FACTORY_DEFAULT_STATE_1:
        if (get_int32_from_data(request->data) == FACTORY_DEFAULT_KEY_1) {
          //erase and rewind
          set_factory_default();
          platform_reboot();
          /* Probably not reached */
          return writeReply32Tlv(request, reply, len, 0);
        } else {
          return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
        }
        ;;
      default:
        return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
        ;;
      } // switch state
    }
#endif /* VARIABLE_CHASSIS_FACTORY_DEFAULT */
#ifdef VARIABLE_IDENTIFY_CHASSIS
    if (request->variable == VARIABLE_IDENTIFY_CHASSIS) {
#ifdef HAVE_FRONTPANEL
      frontpanel_set_identify_pattern(get_int32_from_data(request->data));
#endif /* HAVE_FRONTPANEL */
#ifdef INSTANCE_LAMP
      instance_lamp_identify_pattern(get_int32_from_data(request->data));
#endif /*  INSTANCE_LAMP */
      return writeReply32Tlv(request, reply, len, 0);
    }
#endif /* VARIABLE_IDENTIFY_CHASSIS  */

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

  } else if ((request->opcode == TLV_OPCODE_GET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_GET_REQUEST)) {
    /* 
     *   #####   ######    ##    #####
     *   #    #  #        #  #   #    #
     *   #    #  #####   #    #  #    #
     *   #####   #       ######  #    #
     *   #   #   #       #    #  #    #
     *   #    #  ######  #    #  #####
     */
#ifdef VARIABLE_UNIT_BOOT_TIMER
    if (request->variable == VARIABLE_UNIT_BOOT_TIMER) {
      if (request->elementSize == 8) {
        return writeReply64intTlv(request, reply, len, boottimerIEEE64());
      }
      if (request->elementSize == 4) {
        return writeReply32intTlv(request, reply, len, boottimerIEEE64in32());
      }
      return writeErrorTlv(request, TLV_ERROR_UNKNOWN_ELEMENT_SIZE, reply, len);
    }
#endif /* VARIABLE_UNIT_BOOT_TIMER */
#ifdef VARIABLE_OLD_UNIT_BOOT_TIMER
    if (request->variable == VARIABLE_OLD_UNIT_BOOT_TIMER) {
      if (request->elementSize == 8) {
        return writeReply64intTlv(request, reply, len, boottimerIEEE64());
      }
      if (request->elementSize == 4) {
        return writeReply32intTlv(request, reply, len, boottimerIEEE64in32());
      }
      return writeErrorTlv(request, TLV_ERROR_UNKNOWN_ELEMENT_SIZE, reply, len);
    }
#endif /* VARIABLE_OLD_UNIT_BOOT_TIMER */
#ifdef VARIABLE_SW_REVISION
    if (request->variable == VARIABLE_SW_REVISION) {
      return writeReply128Tlv(request, reply, len,  (uint8_t *)getswrevision());
    }
#endif /* VARIABLE_SW_REVISION */
#ifdef VARIABLE_OLD_SW_REVISION
    if (request->variable == VARIABLE_OLD_SW_REVISION) {
      return writeReply128Tlv(request, reply, len,  (uint8_t *)getswrevision());
    }
#endif /* VARIABLE_OLD_SW_REVISION */
#ifdef VARIABLE_REVISION
    if (request->variable == VARIABLE_REVISION) {
      return writeReply128Tlv(request, reply, len,  (uint8_t *)getswrevision());
    }
#endif /* VARIABLE_REVISION */
#ifdef VARIABLE_READ_THIS_COUNTER
    if (request->variable == VARIABLE_READ_THIS_COUNTER) {
      return writeReply32intTlv(request, reply, len,  read_this_counter++);
    }
#endif /* VARIABLE_READ_THIS_COUNTER */
#ifdef VARIABLE_UNIT_CONTROLLER_ADDRESS
    if (request->variable == VARIABLE_UNIT_CONTROLLER_ADDRESS) {
      return writeReply256Tlv(request, reply, len, unitControllerGetAddress());
    }
#endif /* VARIABLE_UNIT_CONTROLLER_ADDRESS */
#ifdef VARIABLE_UNIT_CONTROLLER_WATCHDOG
    if (request->variable == VARIABLE_UNIT_CONTROLLER_WATCHDOG) {
      unitControllerGetWatchdog(r);
      return writeReply32Tlv(request, reply, len, r);
    }
#endif /* VARIABLE_UNIT_CONTROLLER_WATCHDOG */
#ifdef VARIABLE_OLD_UNIT_CONTROLLER_WATCHDOG
    if (request->variable == VARIABLE_OLD_UNIT_CONTROLLER_WATCHDOG) {
      //request->variable = VARIABLE_UNIT_CONTROLLER_WATCHDOG;
      unitControllerGetWatchdog(r);
      return writeReply32Tlv(request, reply, len, r);
    }
#endif /* VARIABLE_OLD_UNIT_CONTROLLER_WATCHDOG */
#ifdef VARIABLE_UNIT_CONTROLLER_STATUS
    if (request->variable == VARIABLE_UNIT_CONTROLLER_STATUS) {
      unitControllerGetStatus(r);
      return writeReply32Tlv(request, reply, len, r);
    }
#endif /* VARIABLE_UNIT_CONTROLLER_STATUS */
#ifdef VARIABLE_COMMUNICATION_STYLE
    if (request->variable == VARIABLE_COMMUNICATION_STYLE) {
      getCommunicationStyle(r);
      return writeReply32Tlv(request, reply, len, r);
    }
#endif /* VARIABLE_COMMUNICATION_STYLE */
#ifdef VARIABLE_TLV_GROUPING
    if (request->variable == VARIABLE_TLV_GROUPING) {
      uint32_t local32 = boottimer_elapsed(tlv_grouping_time);
      if (tlv_grouping_holdoff > local32) {
        return writeReply32intTlv(request, reply, len, tlv_grouping_holdoff - local32);
      } else {
        return writeReply32intTlv(request, reply, len, 0);
      }
    }
#endif /* VARIABLE_TLV_GROUPING */
#ifdef VARIABLE_LOCATION_ID
    if (request->variable == VARIABLE_LOCATION_ID) {
      uint32_t local32 = get_location_id();
      if (local32 == 0) {
        return writeErrorTlv(request, TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
      return writeReply32intTlv(request, reply, len, local32);
    }
#endif /* VARIABLE_LOCATION_ID */
#ifdef VARIABLE_OWNER_ID
    if (request->variable == VARIABLE_OWNER_ID) {
      uint32_t local32 = get_owner_id();
      if (local32 == 0) {
        return writeErrorTlv(request, TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
      return writeReply32intTlv(request, reply, len, local32);
    }
#endif /* VARIABLE_OWNER_ID */
#ifdef HAVE_INSTANCE_SLEEP
#ifdef VARIABLE_SLEEP_DEFAULT_AWAKE_TIME
    if (request->variable == VARIABLE_SLEEP_DEFAULT_AWAKE_TIME) {
      return writeReply32intTlv(request, reply, len, get_awake_time());
    }
#endif /* VARIABLE_SLEEP_DEFAULT_AWAKE_TIME */
#endif /* HAVE_INSTANCE_SLEEP */
#ifdef VARIABLE_CHASSIS_CAPABILITIES
    if (request->variable == VARIABLE_CHASSIS_CAPABILITIES) {
      uint64_t local64;
      if(platform_get_capabilities(&local64)) {
        return writeReply64intTlv(request, reply, len, local64);
      } else {
        return writeErrorTlv(request, TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }
#endif /* VARIABLE_CHASSIS_CAPABILITIES */
#ifdef VARIABLE_CHASSIS_BOOTLOADER_VERSION
    if (request->variable == VARIABLE_CHASSIS_BOOTLOADER_VERSION) {
      return writeReply32intTlv(request, reply, len, platform_get_bootloader_version());
    }
#endif /* VARIABLE_CHASSIS_BOOTLOADER_VERSION */

#ifdef VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX
    if (request->variable == VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX) {
      return writeReply32intTlv(request, reply, len, oam_get_time_since_last_good_rx_from_uc());
    }
#endif /* VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX */

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }
  return writeErrorTlv(request, TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}

size_t
static poll0(uint8_t instance, uint8_t *reply, size_t len, oam_poll_type poll_type) {
  int replyLen = 0;
  TLV T;
  oam_end_processing endProcessing;

  T.version = TLV_VERSION;
  T.instance = instance;
  T.opcode = TLV_OPCODE_GET_REQUEST;
  T.error = TLV_ERROR_NO_ERROR;
  T.variable = VARIABLE_PRODUCT_TYPE;
  T.elementSize = 8;

  replyLen += processDiscoveryRequest(&T, reply, len, &endProcessing, instances);

  T.variable = VARIABLE_PRODUCT_ID;
  T.opcode = TLV_OPCODE_GET_RESPONSE;
  T.elementSize = 16;
  replyLen += writeReply128Tlv(&T, &reply[replyLen], len - replyLen, getDid());

  T.variable = VARIABLE_UNIT_BOOT_TIMER;
  T.elementSize = 8;
  replyLen += writeReply64intTlv(&T, &reply[replyLen], len - replyLen, boottimerIEEE64());

  T.variable = VARIABLE_UNIT_CONTROLLER_WATCHDOG;
  T.elementSize = 4;
  replyLen += writeReply32intTlv(&T, &reply[replyLen], len - replyLen, unitControllerGetWatchdog(NULL));

#ifdef VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX
  T.variable = VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX;
  T.elementSize = 4;
  replyLen += writeReply32intTlv(&T, &reply[replyLen], len - replyLen,
                                 oam_get_time_since_last_good_rx_from_uc());
#endif /* VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX */

  return replyLen;
}
/*----------------------------------------------------------------*/

/*
 * Add "new_handle" to list of actions that will be performed.
 */
void
instance0_oam_do_action_add(oam_do_action_handle *new_handle) {
  oam_do_action_handle *h;
  if (new_handle == NULL) {
    return;
  }

  for (h = &action_handle; h->next; h = h->next) {
  }
  h->next = new_handle;
  new_handle->next = NULL;
}
/*----------------------------------------------------------------*/

/*
 * Remove "old_handle" from list of handles.
 */
void
instance0_oam_do_action_remove(oam_do_action_handle *old_handle) {
  oam_do_action_handle *h;
  if (old_handle == NULL) {
    return;
  }
  
  for (h = &action_handle; h->next; h = h->next) {
    if (h->next == old_handle) {
      h->next = h->next->next;
      break;
    }
  }
}
/*----------------------------------------------------------------*/

uint8_t
instance0_oam_do_action(oam_action action, uint8_t operation)
{
  oam_do_action_handle *h;

  if ((action == OAM_ACTION_NEW_PDU) && (operation == NEW_TLV_PDU_OPERATION_NEW)) {
    factory_default_state = FACTORY_DEFAULT_STATE_0;
  }

  if(action == OAM_ACTION_UC_WD_EXPIRE) {
    last_wd_expire = boottimer_read();
  }

  /* Now do all */
  h = &action_handle;

  // disabled for now
  return 0;

  do {
    if (h->oam_do_action != NULL) {
      h->oam_do_action(action, operation);
    }
    h = h->next;
  } while (h != NULL);
  return 0;
}
/*----------------------------------------------------------------*/

void
instance0_init(instanceControl IC[]) {
  uint8_t me = 0;

  IC[me].productType = get_product_type();//PRODUCT_TYPE_INT64;
  IC[me].processRequest = process0Request;
  IC[me].poll = poll0;
  IC[me].oam_do_action = instance0_oam_do_action;
  strncat((char *)IC[me].label, PRODUCT_LABEL, sizeof (IC[me].label) - 1);  

  memcpy(IC[me].productID, getDid(), 16);
  IC[me].eventArray[0] = 0;
  IC[me].eventArray[1] = 1;
  IC[me].eventArray[2] = 0;
  IC[me].eventArray[3] = 0;
  IC[me].eventArray[4] = 0;
  IC[me].eventArray[5] = 0;
  IC[me].eventArray[6] = 0;
  IC[me].eventArray[7] = 0;

  IC[me].productSubType[0] = 0x00;
  IC[me].productSubType[1] = 0x00;
  IC[me].productSubType[2] = 0x00;
  IC[me].productSubType[3] = 0x00;

  IC[me].instances = INSTANCES -1;

  action_handle.next = NULL;
  action_handle.oam_do_action = NULL;

  uip_ds6_notification_add(&route_notification, instance0_route_callback);

#if ! defined(HAVE_INSTANCE_SLEEP) && UIP_CONF_IPV6_RPL
  process_start(&periodic_response_process, NULL);
#endif /* ! defined(HAVE_INSTANCE_SLEEP) && UIP_CONF_IPV6_RPL */
}
/*----------------------------------------------------------------*/
uint32_t
get_awake_time()
{
  if (get_awake_time_from_sleep) {
    return get_awake_time_from_sleep();
  }
  return 0;
}
/*----------------------------------------------------------------*/

/*
 * Try to repair network. Called from instance-sleep and
 * periodic_response_process when unit controller has not been heard
 * from for some time.
 * 
 */
int
oam_network_repair(uint8_t prefer_unicast) {
  static uint64_t last_DIS_tx = 0;

  /* If netselect is enabled, it is started when unit controller
   * watchdog expires. Do nothing.
   */
#ifdef HAVE_NETSELECT
  if (unitControllerGetWatchdog(NULL) == 0) {
    return 0;
  }
#endif /* HAVE_NETSELECT */

  /* XXX: look at more stuff than just last tx? */ 
  if (boottimer_elapsed(last_DIS_tx) < OAM_MIN_DIS_INTERVAL_MS) {
    return 0;
  }

  if (!uip_ipaddr_cmp(&defrt, &uip_all_zeroes_addr) && prefer_unicast) {
    dis_output(&defrt);
  } else {
    dis_output(NULL);
  }
  last_DIS_tx = boottimer_read();
  return 1;
}
/*----------------------------------------------------------------*/

/*
 * Process and functions to handle periodic sending of response to
 * unit controller when sleep instance is not present
 */

/*
 * Store default gateway when set.
 */
static void
instance0_route_callback(int event, uip_ipaddr_t *route, uip_ipaddr_t *ipaddr, int num)
{
  if (event == UIP_DS6_NOTIFICATION_DEFRT_ADD) {
    uip_ipaddr_copy(&defrt, route);
  } else if (event == UIP_DS6_NOTIFICATION_DEFRT_RM) {
    memset(&defrt, 0, sizeof(uip_ipaddr_t));
  }
}
/*----------------------------------------------------------------*/
#if ! defined(HAVE_INSTANCE_SLEEP) && UIP_CONF_IPV6_RPL

PROCESS(periodic_response_process, "periodic_response");

PROCESS_THREAD(periodic_response_process, ev, data) {
  static struct etimer sleep_timer;
  static uint32_t this_sleep_time;
  static uint8_t unicast_tx_times = 0;
  rpl_dag_t *dag;
  uint64_t elapsed;
  uint8_t prefer_unicast;

  PROCESS_BEGIN();

  this_sleep_time = CLOCK_CONF_SECOND * 10;

  for (;;) {
    etimer_set(&sleep_timer, this_sleep_time);
    PROCESS_WAIT_EVENT();

    /* No periodic transmissions or network repairs until we have a
       unit controller. */
    if(unitControllerGetWatchdog(NULL) == 0) {
      continue;
    }

    dag = rpl_get_any_dag();
    if(dag != NULL && dag->instance != NULL && dag->rank == ROOT_RANK(dag->instance)) {
      /* We are the root so no need to repair the network */
      continue;
    }

    /* First check for time to send periodic */
    elapsed = boottimer_elapsed(oam_last_activity(OAM_ACTIVITY_TYPE_TX));
    if (elapsed > (OAM_NON_SLEEP_PERIODIC_INTERVAL)) {
      schedule_wakeup_notification();
    }

    /* Check if it might time to update route */
    elapsed = boottimer_elapsed(oam_last_activity(OAM_ACTIVITY_TYPE_RX));
    if (elapsed > (OAM_NON_SLEEP_REPAIR_DELAY)) {
      if (unicast_tx_times < 4) {
	prefer_unicast = TRUE;
      } else {
	prefer_unicast = FALSE;
      }
      if (oam_network_repair(prefer_unicast)) {
        if (unicast_tx_times < 255) {
          unicast_tx_times++;
        }
      }
    } else {
      unicast_tx_times = 0;
    }
  } /* for (;;) */

  PROCESS_END();
}
#endif /* !defined(HAVE_INSTANCE_SLEEP) && UIP_CONF_IPV6_RPL */
/*----------------------------------------------------------------*/
