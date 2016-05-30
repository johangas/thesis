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
 *
 */

#ifndef SERIAL_RADIO_H_
#define SERIAL_RADIO_H_

#include "contiki-conf.h"
#include "vargen.h"

#ifndef SERIAL_RADIO_CONF_NO_PUTCHAR
#ifdef SLIP_RADIO_CONF_NO_PUTCHAR
#define SERIAL_RADIO_CONF_NO_PUTCHAR SLIP_RADIO_CONF_NO_PUTCHAR
#else
#define SERIAL_RADIO_CONF_NO_PUTCHAR 0
#endif /* SLIP_RADIO_CONF_NO_PUTCHAR */
#endif /* SERIAL_RADIO_CONF_NO_PUTCHAR */

typedef radio_mode radio_mode_t;

extern radio_mode_t serial_radio_mode;

uint32_t radio_get_reset_cause(void);

void serial_radio_send_watchdog_warning(uint16_t seconds_left);

radio_mode_t radio_get_mode(void);
void radio_set_mode(radio_mode_t mode);

unsigned serial_radio_get_channel(void);
void serial_radio_set_channel(unsigned channel);

uint32_t serial_get_mode(void);
void serial_set_mode(uint32_t mode);

void slip_set_fragment_size(uint16_t size);
void slip_set_fragment_delay(uint16_t delay);

#endif /* SERIAL_RADIO_H_ */
