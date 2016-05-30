/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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

#ifndef PROJECT_ROUTER_CONF_H_
#define PROJECT_ROUTER_CONF_H_

#ifndef WITH_CLOCK_GETTIME
#ifdef linux
#define WITH_CLOCK_GETTIME 1
#else /* linux */
#define WITH_CLOCK_GETTIME 0
#endif /* linux */
#endif /* WITH_CLOCK_GETTIME */

#if WITH_CLOCK_GETTIME
/* Use clock_gettime() under Linux for monotonic time */
#define CLOCK_USE_CLOCK_GETTIME 1
#define CLOCK_USE_RELATIVE_TIME 0
#else
#define CLOCK_USE_CLOCK_GETTIME 0
#define CLOCK_USE_RELATIVE_TIME 1
#endif

#define SELECT_CONF_MAX 16

#ifndef RF_CHANNEL
#define RF_CHANNEL 26
#endif

#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM         32

#undef SICSLOWPAN_CONF_FRAGMENT_BUFFERS
#define SICSLOWPAN_CONF_FRAGMENT_BUFFERS 30

#undef SICSLOWPAN_CONF_REASS_CONTEXTS
#define SICSLOWPAN_CONF_REASS_CONTEXTS 5

/* CoAP */
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE      512

#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    1280

#undef UIP_CONF_RECEIVE_WINDOW
#define UIP_CONF_RECEIVE_WINDOW  60

#define SLIP_DEV_CONF_SEND_DELAY (CLOCK_SECOND / 128)

#undef WEBSERVER_CONF_CFS_CONNS
#define WEBSERVER_CONF_CFS_CONNS 2

#define SERIALIZE_ATTRIBUTES 1

#define CMD_CONF_OUTPUT border_router_cmd_output
#define CMD_CONF_ERROR  border_router_cmd_error

#undef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO border_router_radio_driver

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC border_router_rdc_driver

#define RPL_CONF_DIO_INTERVAL_MIN 9
#define RPL_CONF_DIO_INTERVAL_DOUBLINGS 11

/* Increase the active scan time if netscan is used because it might
   take too long to receive the beacons from the serial radio. */
#define HANDLER_802154_CONF_ACTIVE_SCAN_TIME CLOCK_SECOND

#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 250

#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 250

#undef UIP_CONF_DS6_ROUTE_NBU
#define UIP_CONF_DS6_ROUTE_NBU 250

#define SELECT_CALLBACK 1

/* OAM definitions */
#define PRODUCT_TYPE_INT64 0x0090DA030201001FULL
#define INSTANCES 2
#define INSTANCE_NSTATS 1

#define OAM_PORT 49111
#define PRODUCT_LABEL "native debug node"
#define SUPPORTED_RADIO_TYPE 0x0090DA0301010482ULL



#define BORDER_ROUTER_CONTROL_API_VERSION 2

/* Set the radio watchdog in seconds as long as Fiona is running but
   not peered */
#define BORDER_ROUTER_CONF_SET_RADIO_WATCHDOG (10 * 60)

#define BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_OK      (7)
#define BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING (0)

#define PLATFORM_POWER_CONTROL(index, value) printf("Power: %d = %d\n", index, value)

/* The border router will never push network statistics */
#define INSTANCE_NSTATS_WITH_PUSH 0

#define RPL_DEBUG_DIO_INPUT(from, dio) debug_node_dio_input(from, dio);

#define CONTIKI_WAIT_FOR_MAC contiki_wait_for_mac

/* Network statistics */
#define RPL_CONF_STATS 1
#define NETSELECT_CONF_STATS 1
/* #define HANDLER_802154_CONF_STATS 1 */

#endif /* PROJECT_ROUTER_CONF_H_ */
