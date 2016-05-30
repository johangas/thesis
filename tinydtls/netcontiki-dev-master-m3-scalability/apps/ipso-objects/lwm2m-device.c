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

#include "lwm2m-object.h"
#include "lwm2m-engine.h"

#ifndef PRODUCT_MODEL_NUMBER
#ifdef BOARD_STRING
#define PRODUCT_MODEL_NUMBER BOARD_STRING
#else /* BOARD_STRING */
#define PRODUCT_MODEL_NUMBER "-"
#endif /* BOARD_STRING */
#endif /* PRODUCT_MODEL_NUMBER */

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static int32_t time_offset = 0;

static int
battery(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outsize)
{
  /*TODO  Always return 100% battery for now */
  return ctx->writer->write_int(ctx, outbuf, outsize, 100);
}

static int
read_lwtime(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outsize)
{
  return ctx->writer->write_int(ctx, outbuf, outsize, time_offset + clock_seconds());
}

static int
set_lwtime(lwm2m_context_t *ctx,
           const uint8_t *inbuf, size_t insize,
           uint8_t *outbuf, size_t outsize)
{
  /* assume that this only read one TLV value */
  int32_t lw_time;
  size_t len = ctx->reader->read_int(ctx, inbuf, insize, &lw_time);
  if(len == 0) {
    PRINTF("FAIL: could not read time '%*.s'\n", (int)insize, inbuf);
  } else {
    PRINTF("Got: time: %*.s => %ld\n", (int)insize, inbuf, (long)lw_time);

    time_offset = lw_time - clock_seconds();
    PRINTF("Write time...%ld => offset = %ld\n",
           (long)lw_time, (long)time_offset);
  }
  /* return the number of bytes read */
  return len;
}

static int
reboot(lwm2m_context_t *ctx, const uint8_t *arg, size_t argsize, uint8_t *outbuf, size_t outsize)
{
#ifdef PLATFORM_REBOOT
  PRINTF("Device will reboot!\n");
  PLATFORM_REBOOT();
#else /* PLATFORM_REBOOT */
  PRINTF("Device should reboot (reboot not supported)\n");
#endif /* PLATFORM_REBOOT */
  return 0;
}
static int
factory_reset(lwm2m_context_t *ctx, const uint8_t *arg, size_t arg_size, uint8_t *outbuf, size_t outsize)
{
  PRINTF("Device should do factory default!\n");
  return 0;
}

LWM2M_RESOURCES(device_resources,
                LWM2M_RESOURCE_STRING(0, "Yanzi Networks AB"),			/* Manufacturer */
                LWM2M_RESOURCE_STRING(1, PRODUCT_MODEL_NUMBER),			/* Model number */
                LWM2M_RESOURCE_STRING(2, "0001"),				/* Serial number */
                LWM2M_RESOURCE_STRING(3, CONTIKI_VERSION_STRING),		/* Firmware Version */
                LWM2M_RESOURCE_CALLBACK(4, { NULL, NULL, reboot }),		/* Reboot */
                LWM2M_RESOURCE_CALLBACK(5, { NULL, NULL, factory_reset }),	/* Factory default */
                LWM2M_RESOURCE_INTEGER(6, 1),					/* Available Power Sources */
                LWM2M_RESOURCE_INTEGER(7, 2800),				/* Power source voltage */
                LWM2M_RESOURCE_INTEGER(8, 120),					/* Power source current */
                LWM2M_RESOURCE_CALLBACK(9, { battery, NULL, NULL }),		/* Battery level (%) */
                LWM2M_RESOURCE_INTEGER(10, 422),
/* Memory Free */
                LWM2M_RESOURCE_INTEGER(11, 0),
/* Error Code */
                LWM2M_RESOURCE_CALLBACK(13, { read_lwtime, set_lwtime, NULL}),
/* Current Time */
                );

LWM2M_INSTANCES(device_instances, LWM2M_INSTANCE(0, device_resources));
LWM2M_OBJECT(device, 3, device_instances);
/*---------------------------------------------------------------------------*/
void
lwm2m_device_init(void)
{
  /* register this device and its handlers - the handlers automatically
     sends in the object to handle */
  PRINTF("*** Init lwm2m-device\n");
  lwm2m_engine_register_object(&device);
}
/*---------------------------------------------------------------------------*/
