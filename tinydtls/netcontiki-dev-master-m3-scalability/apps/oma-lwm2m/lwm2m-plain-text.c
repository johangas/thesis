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
 *         LWM2M plain text reader / writer
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "lwm2m-object.h"
#include "lwm2m-plain-text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
size_t
lwm2m_plain_text_read_int(const uint8_t *inbuf, size_t len, int32_t *value)
{
  int i;
  *value = 0;
  for(i = 0; i < len; i++) {
    if(inbuf[i] >= '0' && inbuf[i] <= '9') {
      *value = *value * 10 + (inbuf[i] - '0');
    } else {
      break;
    }
  }
  return i;
}
/*---------------------------------------------------------------------------*/
static size_t
write_boolean(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen, int value)
{
  if(outlen > 0) {
    if(value) {
      *outbuf = '1';
    } else {
      *outbuf = '0';
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static size_t
write_int(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen, int32_t value)
{
  int n = snprintf((char *)outbuf, outlen, "%ld", (long)value);
  if(n < 0 || n >= outlen) {
    return 0;
  }
  return n;
}
/*---------------------------------------------------------------------------*/
static size_t
write_float32fix(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen, int32_t value, int bits)
{
  int64_t v;
  long integer_part;
  unsigned long frac_part;
  int n;

  v = value;
  v = (v * 100) >> bits;
  integer_part = (long)(value >> bits);
  if(v < 0) {
    v = -v;
  }
  frac_part = (unsigned long)v;
  frac_part = v % 100;

  n = snprintf((char *)outbuf, outlen, "%ld.%lu", integer_part, frac_part);
  if(n < 0 || n >= outlen) {
    return 0;
  }
  return n;
}
/*---------------------------------------------------------------------------*/
static size_t
write_string(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen, const char *value, size_t strlen)
{
  int n = snprintf((char *)outbuf, outlen, "%.*s", (int) strlen, value);
  if(n < 0 || n >= outlen) {
    return 0;
  }
  return n;
}
/*---------------------------------------------------------------------------*/
const lwm2m_writer_t lwm2m_plain_text_writer =
{
  write_int,
  write_string,
  write_float32fix,
  write_boolean
};
/*---------------------------------------------------------------------------*/
static size_t
read_int(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int32_t *value)
{
  return lwm2m_plain_text_read_int(inbuf, len, value);
}
/*---------------------------------------------------------------------------*/
static size_t
read_string(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, uint8_t *value, size_t strlen)
{
  if(strlen <= len) {
    /* The outbuffer can not contain the full string including ending zero */
    return 0;
  }
  memcpy(value, inbuf, len);
  value[len] = '\0';
  return len;
}
/*---------------------------------------------------------------------------*/
static size_t
read_float32fix(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int32_t *value, int bits)
{
  /* TODO Add support for parsing of floats */
  return 0;
}
/*---------------------------------------------------------------------------*/
static size_t
read_boolean(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int *value)
{
  if(len > 0) {
    if(*inbuf == '1' || *inbuf == '0') {
      *value = *inbuf == '1' ? 1 : 0;
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
const lwm2m_reader_t lwm2m_plain_text_reader =
{
  read_int,
  read_string,
  read_float32fix,
  read_boolean
};
/*---------------------------------------------------------------------------*/
