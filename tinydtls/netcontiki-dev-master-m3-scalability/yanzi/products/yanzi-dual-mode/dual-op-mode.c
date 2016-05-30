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

#include "vargen.h"
#include "contiki.h"
#include "net/llsec/llsec802154.h"
#include "yanzi-webserver-simple.h"
#include "resources-coap.h"
#include "dev/button-sensor.h"
#include "dev/slide-switch.h"
#include "usb/usb-serial.h"
#include "serial-radio.h"
#if LLSEC802154_SECURITY_LEVEL > 0
#include "net/llsec/noncoresec/noncoresec.h"
#endif /* LLSEC802154_SECURITY_LEVEL > 0 */

#if WITH_IPSO
#include "ipso-objects.h"
#endif

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

PROCESS_NAME(serial_radio_process);
PROCESS(felicia_dual_mode, "felicia dual mode");
AUTOSTART_PROCESSES(&felicia_dual_mode);

/* Use different product name for USB on platform Felicia */
uint8_t *USB_SERIAL_RADIO_GET_PRODUCT_DESCRIPTION(void);

struct product {
  uint8_t size;
  uint8_t type;
  uint16_t string[25];
};
static const struct product product = {
  sizeof(product),
  3,
  {
    'Y','a','n','z','i',' ','I','o','T',' ','D','o','n','g','l','e',
    ',', ' ','I','o','T','-','U','1','0'
  }
};

uint8_t *
felicia_dual_mode_get_product_description(void)
{
  if(dual_mode_get_op_mode() == DUAL_MODE_OP_MODE_STANDARD) {
    return (uint8_t *)&product;
  }
  return USB_SERIAL_RADIO_GET_PRODUCT_DESCRIPTION();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(felicia_dual_mode, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  if(dual_mode_get_op_mode() == DUAL_MODE_OP_MODE_STANDARD) {
    /* Initialize the CoAP server and activate resources */
    rest_init_engine();
    rest_activate_resource(&resource_led, (char *)"led");
    rest_activate_resource(&resource_led0, (char *)"led/0");
    rest_activate_resource(&resource_led1, (char *)"led/1");
    rest_activate_resource(&resource_ipv6_neighbors, (char *)"ipv6/neighbors");
    rest_activate_resource(&resource_rpl_info, (char *)"rpl-info");
    rest_activate_resource(&resource_rpl_parent, (char *)"rpl-info/parent");
    rest_activate_resource(&resource_rpl_rank, (char *)"rpl-info/rank");
    rest_activate_resource(&resource_rpl_link_metric, (char *)"rpl-info/link-metric");

    SENSORS_ACTIVATE(button_sensor);
    SENSORS_ACTIVATE(slide_switch_sensor);
    rest_activate_resource(&resource_push_button_event, (char *)"push-button");
    rest_activate_resource(&resource_temperature, (char *)"temperature");

#if WITH_IPSO
    ipso_objects_init();
#endif

    PROCESS_PAUSE();
    process_start(&yanzi_webserver_simple_process, NULL);

    while(1) {
      etimer_set(&timer, CLOCK_SECOND * 5);
      PROCESS_WAIT_EVENT();

      if(ev == sensors_event && data == &button_sensor) {
        resource_push_button_event.trigger();
        PRINTF("Button pressed!\n");
      }

      if(ev == sensors_event && data == &slide_switch_sensor) {
        PRINTF("Sliding switch is %s\n",
               slide_switch_sensor.value(0) ? "on" : "off");
      }
    }
  } else {
    /* Serial radio mode */

#if LLSEC802154_SECURITY_LEVEL > 0
    /* Disable encryption in serial radio - controlled by border router */
    noncoresec_set_security_level(0);
#endif /* LLSEC802154_SECURITY_LEVEL > 0 */

#if SLIP_ARCH_CONF_USB || DBG_CONF_USB
    /* No USB write timeout when running as serial radio */
    usb_serial_set_write_timeout(0);
#endif

    process_start(&serial_radio_process, NULL);
  }
  PROCESS_END();
}
