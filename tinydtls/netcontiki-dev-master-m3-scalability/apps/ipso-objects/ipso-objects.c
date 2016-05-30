/*
 * Copyright (c) 2015, SICS Swedish ICT AB.
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
 * Author: Joakim Eriksson, joakime@sics.se
 *         Niclas Finne, nfi@sics.se
 *
 */


#include "contiki.h"
#include "lwm2m-engine.h"

void lwm2m_device_init(void);
void ipso_temperature_init(void);
void ipso_button_init(void);
void ipso_leds_control_init(void);
void lwm2m_device_init(void);
void lwm2m_security_init(void);
void lwm2m_server_init(void);
void lwm2m_software_init(void);
void ipso_temperature_init(void);
void ipso_button_init(void);
void ipso_leds_control_init(void);
void ipso_power_control_init(void);


void
ipso_objects_init()
{
  /* initialize any relevant object for the IPSO Objects */
  lwm2m_engine_init();
  lwm2m_security_init();
  lwm2m_server_init();
  /* lwm2m_software_init(); */
  lwm2m_device_init();
  ipso_temperature_init();
#if PLATFORM_HAS_BUTTON
  ipso_button_init();
#endif
#if PLATFORM_HAS_LEDS
  ipso_leds_control_init();
#endif
#ifdef PLATFORM_POWER_CONTROL
  ipso_power_control_init();
#endif

}
