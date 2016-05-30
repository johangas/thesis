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
 *         A brief description of what this file is
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "contiki.h"
#include "oma-tlv.h"
#include "unit-test.h"

UNIT_TEST_REGISTER(tlv, "OMA TLV");

void
printtlv(const oma_tlv_t *tlv)
{
  int i;
  printf("TLV\n");
  printf("  type: %d\n", tlv->type);
  printf("  id: %d\n", tlv->id);
  printf("  len: %d\n", tlv->length);
  printf("  val:");
  for(i = 0; i < tlv->length; i++) {
    printf(" %02x", tlv->value[i]);
  }
  printf("\n");
}

void
printbuf(const uint8_t *buffer, int len)
{
  int i;
  printf("Len: %d\n", len);
  printf("0b");

  for(i = 7; i >=  0; i--) {
    printf("%c", (buffer[0] >> i) & 0x01 ? '1':'0');
  }

  for(i = 0; i < len; i++) {
    printf(" %02x", buffer[i]);
  }
  printf("\n");
}

UNIT_TEST(tlv)
{
  uint8_t buffer[256];
  uint8_t b1[32];
  uint8_t b2[32];
  size_t len;
  size_t len2;
  int i;
  oma_tlv_t tlv;
  oma_tlv_t tlv2;
  int32_t fix;
  char *oma = "Lightweight M2M Client";
  char *v = "1.0";

  UNIT_TEST_BEGIN();

  tlv.type = OMA_TLV_TYPE_RESOURCE;
  tlv.id = 1;
  tlv.length = strlen(oma);
  tlv.value = (uint8_t *) oma;

  printf("Before write\n");
  printtlv(&tlv);

  len = oma_tlv_write(&tlv, buffer, sizeof(buffer));
  printbuf(buffer, len);

  len = oma_tlv_read(&tlv2, buffer, sizeof(buffer));
  printf("Read: %zu\n", len);
  printtlv(&tlv2);

  UNIT_TEST_ASSERT(tlv.id == 1);
  UNIT_TEST_ASSERT(tlv.length == strlen(oma));

  tlv.id = 3;
  tlv.length = strlen(v);
  len = oma_tlv_write(&tlv, buffer, sizeof(buffer));
  printbuf(buffer, len);

  len = oma_tlv_read(&tlv, buffer, sizeof(buffer));
  printf("Read: %zu\n", len);
  printtlv(&tlv);

  /* Multi resource instances */
  tlv.type = OMA_TLV_TYPE_RESOURCE_INSTANCE;
  tlv.id = 1; /* ACL 1 */
  tlv.length = 1;
  tlv.value = (uint8_t *) "A";
  len = oma_tlv_write(&tlv, b1, sizeof(b1));

  tlv.id = 2; /* ACL 2 */
  tlv.length = 1;
  tlv.value = (uint8_t *) "B";
  len2 = oma_tlv_write(&tlv, &b1[len], sizeof(b1) - len);

  len = len + len2;

  tlv.type = OMA_TLV_TYPE_MULTI_RESOURCE;
  tlv.id = 2; /* ACL */
  tlv.length = len;
  tlv.value = b1;
  len2 = oma_tlv_write(&tlv, buffer, sizeof(buffer));
  printbuf(buffer, len2);


  tlv.type = OMA_TLV_TYPE_RESOURCE;
  tlv.length = 4;
  tlv.value = b2;

  fix = 32 * 1024 + 512;
  printf("Fix10: %x\n", fix);
  len2 = oma_tlv_write_float32(1, fix, 10, b2, sizeof(b2));
  printbuf(b2, len2);

  len = oma_tlv_read(&tlv, b2, sizeof(b2));
  printf("Read: %zu\n", len);
  printtlv(&tlv);

  oma_tlv_float32_to_fix(&tlv, &fix, 10);

  printf("Fix10: %x\n", fix);

  UNIT_TEST_ASSERT(fix == 32 * 1024 + 512);

  UNIT_TEST_END();
}

PROCESS(test_process, "Unit testing");
AUTOSTART_PROCESSES(&test_process);

PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  UNIT_TEST_RUN(tlv);

  PROCESS_END();
}

