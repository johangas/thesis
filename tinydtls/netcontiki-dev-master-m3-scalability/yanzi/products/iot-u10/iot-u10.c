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

#include "contiki.h"
#include "yanzi-webserver-simple.h"
#include "resources-coap.h"
#include "dev/button-sensor.h"
#include "dev/slide-switch.h"

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

PROCESS(felicia_main, "felicia main");
AUTOSTART_PROCESSES(&felicia_main);

PROCESS_THREAD(felicia_main, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

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

  PROCESS_END();
}
