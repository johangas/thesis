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
 * Author: Joakim Eriksson, joakime@sics.se
 *         Niclas Finne, nfi@sics.se
 *
 */

#ifndef LWM2M_ENGINE_H
#define LWM2M_ENGINE_H

#include "contiki-net.h"
#include "lwm2m-object.h"
#include "rest-engine.h"


/* LWM2M / CoAP Content-Formats */
typedef enum {
  LWM2M_TEXT_PLAIN = 1541,
  LWM2M_TLV = 1542,
  LWM2M_JSON = 1543,
  LWM2M_OPAQUE = 1544
} lwm2m_content_format_t;

void lwm2m_engine_init(void);
void lwm2m_register_with(uip_ipaddr_t *server);

const lwm2m_object_t *lwm2m_engine_get_object(uint16_t id);
int lwm2m_engine_register_object(const lwm2m_object_t *object);

void lwm2m_engine_handler(const lwm2m_object_t *object, void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

void lwm2m_engine_delete_handler(const lwm2m_object_t *object, void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

#endif /* LWM2M_ENGINE_H */
