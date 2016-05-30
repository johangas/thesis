/* Copyright (c) 2015, Yanzi Networks AB.
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

#include "resources-coap.h"
#include "contiki-net.h"
#include "rest-engine.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void res_get_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset);

/*---------------------------------------------------------------------------*/
static uint16_t
print_routes(char *msg, int size)
{
  uip_ds6_route_t *r = uip_ds6_route_head();
  int len = strlen(msg);
  while(r != NULL && len < size + 30) {
    /* output a route entry */
    len += sprint_addr6(&msg[len], &r->ipaddr);
    len += snprintf(&msg[len], size, "->");
    len += sprint_addr6(&msg[len], uip_ds6_route_nexthop(r));
    len += snprintf(&msg[len], size, "\n");
    r = uip_ds6_route_next(r);
  }
  return len;
}
/*---------------------------------------------------------------------------*/
/*
 * Declare the IPv6 neighbors resource
 */
RESOURCE(resource_ipv6_routes,
  "title=\"IPV6-Routes: ?len=0..\";rt=\"Text\"",
  res_get_handler,
  NULL,
  NULL,
  NULL);
/*---------------------------------------------------------------------------*/
static void
res_get_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset)
{
  const char *len = NULL;
  char message[2*REST_MAX_CHUNK_SIZE];
  memset(message, 0, 2*REST_MAX_CHUNK_SIZE);
  print_routes(&message[0], 2*REST_MAX_CHUNK_SIZE);

  int length = strlen(&message[0]);

  /* The query string can be retrieved by rest_get_query(),
   * or parsed for its key-value pairs.
   */
  if(REST.get_query_variable(request, "len", &len)) {
    length = atoi(len);
    if(length < 0) {
      length = 0;
    }
    if(length > REST_MAX_CHUNK_SIZE) {
      length = REST_MAX_CHUNK_SIZE;
    }
    memcpy(buffer, message, length);
  } else {
    memcpy(buffer, message, length);
  }
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *)&length, 1);
  REST.set_response_payload(response, buffer, length);
}
/*---------------------------------------------------------------------------*/
