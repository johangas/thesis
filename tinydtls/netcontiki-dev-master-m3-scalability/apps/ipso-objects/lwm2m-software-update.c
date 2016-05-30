/*
 * Copyright (c) 2015, SICS, Swedish ICT AB.
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

/**
 * \file
 *         OMA LWM2M device
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include <stdint.h>
#include "lwm2m-object.h"
#include "lwm2m-engine.h"

#define MAX_COUNT 2

static int32_t state_arr[MAX_COUNT];

/* hoping that we do not get more than 64 bytes... */
static char name[MAX_COUNT][64] = {"test1", "test2"};
/* static char uri[MAX_COUNT][64] = {"test", "test"}; */

static int
get_name(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outsize)
{
  int id = ctx->object_instance_id;
  int index = ctx->object_instance_index;
  printf("Got a get on name: ID: %d  index: %d\n", id, index);
  if(index > 1) return 0;
  return ctx->writer->write_string(ctx, outbuf, outsize,
			      name[index], strlen((const char *)name[index]));
}

static int
write_uri(lwm2m_context_t *ctx,
          const uint8_t *buffer, size_t len,
	  uint8_t *outbuf, size_t outlen)
{
  printf("Got a write on URI: %*.s", (int) len, buffer);
  return 0;
}



LWM2M_RESOURCES(software_resources,
                LWM2M_RESOURCE_CALLBACK(0, { get_name, NULL, NULL }),
                LWM2M_RESOURCE_CALLBACK(3, { NULL, write_uri, NULL }),
		LWM2M_RESOURCE_INTEGER_VAR_ARR(9, MAX_COUNT, state_arr)
                );

/* Quick hack for dynamic instances - allocate for two instances */
LWM2M_INSTANCES(software_instances,
		LWM2M_INSTANCE(0, software_resources),
		LWM2M_INSTANCE_UNUSED(1, software_resources));

LWM2M_OBJECT(software, 9, software_instances);
/*---------------------------------------------------------------------------*/
void
lwm2m_software_init(void)
{
  /* register this device and its handlers - the handlers automatically
     sends in the object to handle */
  printf("** ** Init lwm2m-software\n");
  lwm2m_engine_register_object(&software);
}
/*---------------------------------------------------------------------------*/
