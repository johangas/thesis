/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#ifdef CONTIKI_TARGET_FELICIA

#ifndef WITH_SPI
#define WITH_SPI 0
#endif

#ifndef WITH_UART
#define WITH_UART 0
#endif

#if WITH_SPI
#define DBG_CONF_USB             0 /* command output uses debug output */
#define SLIP_ARCH_CONF_SPI       1
#define SSI0_CONF_ENABLE         1

#define SPI_CLK_PORT             GPIO_B_NUM
#define SPI_CLK_PIN              2
#define SPI_MOSI_PORT            GPIO_B_NUM
#define SPI_MOSI_PIN             4
#define SPI_MISO_PORT            GPIO_B_NUM
#define SPI_MISO_PIN             3
#define SPI_CS_PORT              GPIO_B_NUM
#define SPI_CS_PIN               1
#define SPI_INT_PORT             GPIO_B_NUM
#define SPI_INT_PIN              5          /* Conflict with UART0_RTS_PIN */
#define SPI_SEL_PORT             GPIO_C_NUM /* Currently not used */
#define SPI_SEL_PIN              7

#elif WITH_UART

#define DBG_CONF_USB             0 /* command output uses debug output */
#define SLIP_ARCH_CONF_USB       0

#else /* WITH_SPI */

#define DBG_CONF_USB             1 /* command output uses debug output */
#define SLIP_ARCH_CONF_USB       1

#endif /* WITH_SPI */

#define LPM_CONF_ENABLE          0 /* serial radio never in low-power mode */
#define USB_CONF_WRITE_TIMEOUT   0 /* no USB write timeout in serial radio */

#define CONF_RINGBUF_PTR_SIZE   32

/* Use different product description for USB */
#define USB_SERIAL_GET_PRODUCT_DESCRIPTION serial_radio_get_product_description
#endif /* CONTIKI_TARGET_FELICIA */

#define STORE_LOCATIONID_LOCALLY 1

#define FRAMER_802154_HANDLER handler_802154_frame_received

#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM          4

#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    1280

#undef UIP_CONF_ROUTER
#define UIP_CONF_ROUTER                 0

#ifdef CONTIKI_TARGET_ANANAS

#define UART1_CONF_TX_TIMEOUT (CLOCK_SECOND / 2)

/* Number of buffers for incoming frames on STM32W108. Set to a number
   greater than 1 to decrease packet loss probability at high rates
   (e.g, with fragmented packets)
 */
#undef RADIO_RXBUFS
#define RADIO_RXBUFS                    8

#define ST_CONF_RADIO_AUTOACK            1
#define NULLRDC_CONF_802154_AUTOACK_HW  ST_CONF_RADIO_AUTOACK

#undef SERIAL_BAUDRATE
#if (ANANAS_VARIANT == ANANAS_VARIANT_barbie)
#define SERIAL_BAUDRATE 460800
#else /* (ANANAS_VARIANT == barbie) */
#define SERIAL_BAUDRATE 460800
#endif /* (ANANAS_VARIANT == barbie) */

#elif defined CONTIKI_TARGET_SKY

#undef UART1_CONF_RX_WITH_DMA
#define UART1_CONF_RX_WITH_DMA           1

#endif /* defined CONTIKI_TARGET_SKY */

#define CMD_CONF_OUTPUT serial_radio_cmd_output
#define CMD_CONF_ERROR  serial_radio_cmd_error

/* configuration for the serial radio/network driver */
#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC     nullmac_driver

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     sniffer_rdc_driver

#define NULLRDC_CONF_ENABLE_RETRANSMISSIONS 1
#define NULLRDC_CONF_MAX_RETRANSMISSIONS 3
#define NULLRDC_CONF_ENABLE_RETRANSMISSIONS_BCAST 1

#undef NETSTACK_CONF_NETWORK
#define NETSTACK_CONF_NETWORK slipnet_driver

#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER no_framer

/* Default serial radio watchdog for radio frontpanel in seconds */
#define RADIO_FRONTPANEL_WATCHDOG_RESET_VALUE (60 * 60)

#include "instance-radio.h"

#define VARGEN_ADDITIONAL_INSTANCES INSTANCE_RADIO_VARIABLES

/*
 * OAM definitions
 */
#define INSTANCES 4
#define PRODUCT_TYPE_INT64 0x0090DA0301010482ULL
#define PRIMARY_FLASH_INSTANCE 1
#define BACKUP_FLASH_INSTANCE  2
#define INSTANCE_RADIO         3

#define PRODUCT_LABEL "Serial Radio"

#define SERIAL_RADIO_CONTROL_API_VERSION 2L

#define OAM_INIT_DECLARATIONS void instance_radio_init(instanceControl IC[]);
#define OAM_INIT_FUNCS instance_radio_init

#endif /* PROJECT_CONF_H_ */
