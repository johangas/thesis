/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \addtogroup platform
 * @{
 *
 * \defgroup cc2538 The felicia module
 *
 * The felicia is a module from Yanzi Networks AB, based on the
 * cc2530 SoC with an ARM Cortex-M3 core.
 * @{
 *
 * \file
 *   Main module for the felicia platform
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/sys-ctrl.h"
#include "dev/scb.h"
#include "dev/nvic.h"
#include "dev/uart.h"
#include "dev/watchdog.h"
#include "dev/ioc.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/cc2538-rf.h"
#include "dev/udma.h"
#include "dev/leds.h"
#include "usb/usb-serial.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/queuebuf.h"
#include "net/ip/tcpip.h"
#include "net/ip/uip.h"
#include "net/mac/frame802154.h"
#include "cpu.h"
#include "reg.h"
#include "ieee-addr.h"
#include "lpm.h"
#if PLATFORM_HAS_SENSORS
#include "lib/sensors.h"
#endif /* PLATFORM_HAS_SENSORS */
#ifdef HAVE_NETSCAN
#include "netscan.h"
#endif /* HAVE_NETSCAN */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
#ifdef MY_APP_PROCESS
PROCESS_NAME(MY_APP_PROCESS);
#endif /* MY_APP_PROCESS */
#ifdef MY_APP_INIT_FUNCTION
void MY_APP_INIT_FUNCTION(void);
#endif /* MY_APP_INIT_FUNCTION */
/*---------------------------------------------------------------------------*/
#include "trailer.h"
#if HAVE_OAM
#include "oam.h"
#ifdef OAM_INIT_DECLARATIONS
OAM_INIT_DECLARATIONS
#endif /* OAM_INIT_DECLARATIONS */
#ifdef OAM_INIT_FUNCS
static void (*oam_init_funcs[])(instanceControl[]) = { OAM_INIT_FUNCS, NULL };
#else /* OAM_INIT_FUNCS */
#define oam_init_funcs NULL
#endif /* OAM_INIT_FUNCS */
#endif /* HAVE_OAM */
/*---------------------------------------------------------------------------*/
#if STARTUP_CONF_VERBOSE
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if UART_CONF_ENABLE
#define PUTS(s) puts(s)
#else
#define PUTS(s)
#endif

#define IMAGES 2
#define SEARCH_MFG_START 0x200000
#define SEARCH_MFG_END   0x203000

mfg_area *flash_mfg_area = NULL;
image_info *images = NULL;

#if FELICIA_PLATFORM_WITH_DUAL_MODE == 1
/* The state of the operation mode is stored in contiki-main */
static dual_mode_op_mode_t op_mode = DUAL_MODE_OP_MODE_STANDARD;
/*---------------------------------------------------------------------------*/
/*
 * Return the current operation mode of the mote.
 */
dual_mode_op_mode_t
dual_mode_get_op_mode(void)
{
  return op_mode;
}
/*---------------------------------------------------------------------------*/
#endif /* FELICIA_PLATFORM_WITH_DUAL_MODE */

#if PLATFORM_HAS_SLIDE_SWITCH
#include "dev/slide-switch.h"
#endif /* PLATFORM_HAS_SLIDE_SWITCH */
/*---------------------------------------------------------------------------*/
/*
 * Search memory for a manufacturing area, and sets "our_mfg_area" if found.
 */
void
find_mfg_area(void)
{
  uint32_t p;
  uint32_t compare;

  compare = MFG_MAGIC0 + (MFG_MAGIC1 << 8) + (MFG_MAGIC2 << 16) + (MFG_MAGIC3 << 24);

  for(p = SEARCH_MFG_START; p < SEARCH_MFG_END; p += 4) {
    if(*((uint32_t *)p) == compare) {
      flash_mfg_area = (mfg_area *)p;
      /* images = our_mfg_area->images; */
      break;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
set_rf_params(void)
{
  uint16_t short_addr;
  uint8_t ext_addr[8];

  ieee_addr_cpy_to(ext_addr, 8);

  short_addr = ext_addr[7];
  short_addr |= ext_addr[6] << 8;

  /* Populate linkaddr_node_addr. Maintain endianness */
  memcpy(&linkaddr_node_addr, &ext_addr[8 - LINKADDR_SIZE], LINKADDR_SIZE);

#if STARTUP_CONF_VERBOSE
  {
    int i;
    printf("Rime configured with address ");
    for(i = 0; i < LINKADDR_SIZE - 1; i++) {
      printf("%02x:", linkaddr_node_addr.u8[i]);
    }
    printf("%02x\n", linkaddr_node_addr.u8[i]);
  }
#endif

  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, CC2538_RF_CHANNEL);
  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, 8);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Start the network stack
 */
static void
start_network(void)
{
#if NETSTACK_CONF_WITH_IPV6
#if FELICIA_PLATFORM_WITH_DUAL_MODE == 1
  if(op_mode == DUAL_MODE_OP_MODE_STANDARD) {
    /* tcpip_process does not start when in serial-radio mode */
    process_start(&tcpip_process, NULL);
  }
#else /* FELICIA_PLATFORM_WITH_DUAL_MODE == 1 */
  process_start(&tcpip_process, NULL);
#endif /* FELICIA_PLATFORM_WITH_DUAL_MODE == 1 */
#endif /* NETSTACK_CONF_WITH_IPV6 */

#ifdef HAVE_OAM_MFG_INIT
  oam_mfg_init();
#endif

#if HAVE_OAM
#if FELICIA_PLATFORM_WITH_DUAL_MODE == 1
  if(op_mode == DUAL_MODE_OP_MODE_SERIAL_RADIO) {
    void (*oam_init_functions[])(instanceControl[]) = { OAM_INIT_FUNCS_SERIAL_RADIO, NULL };
    oam_init(oam_init_functions);
  } else {
    void (*oam_init_functions[])(instanceControl[]) = { OAM_INIT_FUNCS_STANDARD_MODE, NULL };
    oam_init(oam_init_functions);

#ifdef HAVE_NETSCAN
    netscan_init();
#endif /* HAVE_NETSCAN */
  }
#else /* FELICIA_PLATFORM_WITH_DUAL_MODE */
  oam_init(oam_init_funcs);
#ifdef HAVE_NETSCAN
  netscan_init();
#endif /* HAVE_NETSCAN */
#endif /* FELICIA_PLATFORM_WITH_DUAL_MODE */
  process_start(&oam, NULL);
#endif

  autostart_start(autostart_processes);

#ifdef MY_APP_INIT_FUNCTION
  MY_APP_INIT_FUNCTION();
#endif /* MY_APP_INIT_FUNCTION */

#ifdef MY_APP_PROCESS
  process_start(&MY_APP_PROCESS, NULL);
#endif /* MY_APP_PROCESS */
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Main routine for the felicia platform
 */
int
main(void)
{
  nvic_init();
  ioc_init();
  sys_ctrl_init();
  clock_init();
  lpm_init();
  rtimer_init();
  gpio_init();

#if PLATFORM_HAS_LEDS
  leds_init();
#endif /* PLATFORM_HAS_LEDS */

  process_init();

  platform_init();

  watchdog_init();

  /*
   * Character I/O Initialisation.
   * When the UART receives a character it will call serial_line_input_byte to
   * notify the core. The same applies for the USB driver.
   *
   * If slip-arch is also linked in afterwards (e.g. if we are a border router)
   * it will overwrite one of the two peripheral input callbacks. Characters
   * received over the relevant peripheral will be handled by
   * slip_input_byte instead
   */
#if UART_CONF_ENABLE
  uart_init(0);
  uart_init(1);
  uart_set_input(SERIAL_LINE_CONF_UART, serial_line_input_byte);
#endif

#if FELICIA_PLATFORM_WITH_DUAL_MODE == 1
#if PLATFORM_HAS_SLIDE_SWITCH
  slide_switch_sensor.configure(SENSORS_HW_INIT, 0);
  /* Determine running mode according to the position of the slide switch */
  if(slide_switch_sensor.value(0) == 0) {
    /* Start as SERIAL_RADIO */
    op_mode = DUAL_MODE_OP_MODE_SERIAL_RADIO;
  } else {
    /* Start as standard mote */
    op_mode = DUAL_MODE_OP_MODE_STANDARD;
  }
#else
#error "Slide-switch is required to enable run-time mode selection"
#endif /* PLATFORM_HAS_SLIDE_SWITCH */
#endif /* FELICIA_PLATFORM_WITH_DUAL_MODE */

#if USB_SERIAL_CONF_ENABLE
  usb_serial_init();
  usb_serial_set_input(serial_line_input_byte);
#endif

  serial_line_init();

  INTERRUPTS_ENABLE();

  find_mfg_area();
  if(flash_mfg_area != NULL) {
    images = flash_mfg_area->images;
  }

  PRINTF("Reset by %s\n", platform_reset_cause());

  PUTS(CONTIKI_VERSION_STRING);
  PUTS(BOARD_STRING);

  PRINTF(" Net: ");
  PRINTF("%s\n", NETSTACK_NETWORK.name);
  PRINTF(" MAC: ");
  PRINTF("%s\n", NETSTACK_MAC.name);
  PRINTF(" RDC: ");
  PRINTF("%s\n", NETSTACK_RDC.name);
  PRINTF("PAN-ID: 0x%04x\n", IEEE802154_PANID);
  PRINTF("RF Channel: %u\n", CC2538_RF_CONF_CHANNEL);
  PRINTF("Compiled @ %s %s\n", __DATE__, __TIME__);

  /* Initialise the H/W RNG engine. */
  random_init(0);

  udma_init();

  process_start(&etimer_process, NULL);
  ctimer_init();

  set_rf_params();

#if PLATFORM_HAS_SENSORS
  process_start(&sensors_process, NULL);
#endif /* PLATFORM_HAS_SENSORS */

  netstack_init();
#if NETSTACK_CONF_WITH_IPV6
  memcpy(&uip_lladdr.addr, &linkaddr_node_addr, sizeof(uip_lladdr.addr));
  queuebuf_init();
#endif /* NETSTACK_CONF_WITH_IPV6 */

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  NETSTACK_LLSEC.bootstrap(start_network);

  watchdog_start();

  while(1) {
    uint8_t r;
    do {
      /* Reset watchdog and handle polls and events */
      watchdog_periodic();

      r = process_run();
    } while(r > 0);

    /* We have serviced all pending events. Enter a Low-Power mode. */
    //lpm_enter();
  }
}
/*---------------------------------------------------------------------------*/

/**
 * @}
 * @}
 */
