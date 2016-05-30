/*
 * Copyright (c) 2015, Yanzi Networks AB.
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

#if defined CONTIKI_TARGET_NATIVE && (CONTIKI_TARGET_NATIVE != 0)
#include <stdlib.h>
#endif /* CONTIKI_TARGET_NATIVE */
#ifdef CONTIKI
#include "sys/clock.h"
#include "net/ip/uip.h"
#endif /* CONTIKI */
#include "api.h"
#include "lib/random.h"
#include "swrevision.h"
#ifdef CORTEXM3_STM32W108
#include "dev/stm32w-radio.h"
#include "lib/random.h"
#include "flash.h"
#endif /* CORTEXM3_STM32W108  */
#include "dev/watchdog.h"
#include "dev/uart1.h"
#if HAVE_EEPROM_STORE
#include "eeprom-store.h"
#endif /* HAVE_EEPROM_STORE */

#define NODE_KEY_SIZE 32

#if defined CONTIKI_TARGET_NATIVE && CONTIKI_TARGET_NATIVE == 1
#define STORE_LOCATIONID_LOCALLY 1
#endif /* CONTIKI_TARGET_NATIVE && CONTIKI_TARGET_NATIVE == 1 */

#ifndef STORE_LOCATIONID_LOCALLY
#define STORE_LOCATIONID_LOCALLY 0
#endif /* STORE_LOCATIONID_LOCALLY */

#if STORE_LOCATIONID_LOCALLY == 1
uint32_t local_location_id = 0;
uint8_t local_location_key[NODE_KEY_SIZE] = { 0, };
#endif /* STORE_LOCATIONID_LOCALLY */
const int8_t always_ff[NODE_KEY_SIZE] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

uint64_t
get_product_type(void)
{
#ifdef PRODUCT_TYPE_INT64
#if (CONTIKI_TARGET_FELICIA) && (FELICIA_PLATFORM_WITH_DUAL_MODE == 1)
#ifdef PRODUCTTYPE_INT64
  /* Run-time selected product type, based on the operation mode */
  return PRODUCTTYPE_INT64;
#endif /* PRODUCTTYPE_INT64 */
#endif /* CONTIKI_TARGET_FELICIA && FELICIA_PLATFORM_WITH_DUAL_MODE == 1 */
  /* Compile-time selected product type, present also in trailer */
  return PRODUCT_TYPE_INT64;
#else /* PRODUCT_TYPE_INT64 */
  return 0x0090DA0301010000ULL;   /* Generic TLV device */
#endif /* PRODUCT_TYPE_INT64 */
}
/*
 * Get 128bit Product ID formatted for variable response.
 */
uint8_t *
getDid(void)
{
  static uint8_t done = 0;
  static uint8_t did[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x90, 0xda, 0xff, 0xfe, 0x00, 0x4f, 0x00 };

  if(!done) {
    int i;
    for(i = 8; i < 16; i++) {
      did[i] = uip_lladdr.addr[i - 8];
    }
    done = TRUE;
  }
  return did;
}
/*
 * IEEE 64 bit time format.
 * 32 bit seconds + 1 bit sign + 31 bits * nanoseconds.
 */
uint64_t
boottimerIEEE64(void)
{
  uint64_t now;
  now = boottimer_read();
  now = ((now / 1000) << 32) + ((now % 1000) * 1000000L);
#ifdef CONTIKI_TARGET_NATIVE
  {
    static uint64_t last = 0;
    if(now <= last) {
      now = last + 1;
    }
    last = now;
  }
#endif /* ! CONTIKI_TARGET_NATIVE */
  return now;
}
/*
 * IEEE 64 bit time format.
 * 32 bit seconds only
 */
uint32_t
boottimerIEEE64in32(void)
{
  uint64_t now;
  now = boottimer_read();
  return now / 1000;
}
/*
 * Get string representation of software version
 */
const uint8_t *
getswrevision(void)
{
  return (const uint8_t *)swrevision;
}
/*
 * Get string representation of compile time.
 */
const uint8_t *
getcompiletime(void)
{
  return (const uint8_t *)compiletime;
}
/*
 * Give number of milliseconds that have elapsed since "when".
 */
uint64_t
boottimer_elapsed(uint64_t when)
{
  /* Really simple when time can not wrap */
  return boottimer_read() - when;
}
/*
 * milliseconds since boot
 */
uint64_t
boottimer_read(void)
{
#ifdef CONTIKI
#if (CLOCK_CONF_SECOND == 1000)
  return clock_time();
#else /* (CLOCK_CONF_SECOND == 1000) */
  return clock_time() * 1000 / CLOCK_CONF_SECOND;
#endif /* (CLOCK_CONF_SECOND == 1000) */
#elif defined FreeRTOS
  return (uint32_t)xTaskGetTickCount() * portTICK_RATE_MS;
#endif /* OS */
}
int32_t
milliseconds_until(uint64_t future)
{
  uint64_t now = boottimer_read();
  uint64_t n;
  if(future <= now) {
    return 0;
  }
  n = future - now;
  if(n > 0x7fffffff) {
    return 0x7fffffff;
  }
  return (int32_t)(n & 0x7fffffff);
}
/*
 * Busy wait some short time.
 */
void
usleep(uint32_t usec)
{
  rtimer_clock_t when;
  uint32_t delay;
  when = RTIMER_NOW();

  delay = (RTIMER_SECOND * usec) / 1000000;
  if(delay < 2) {
    delay = 2;
  }
  when += delay;

  while(RTIMER_NOW() < when) ;
}
/*
 * Write "len" bytes of random to "dst"
 */
void
random_fill(uint8_t *dst, size_t len)
{
  int i;
  uint16_t r;
#ifdef CONTIKI
  for(i = 0; i < len; i += 2) {
#ifdef CORTEXM3_STM32W108
    ST_RadioGetRandomNumbers(&r, 1);
#else /* CORTEXM3_STM32W108  */
#if TARGET == felicia
    r = random_rand();
#else
    r = rand();
#endif /* TARGET == felicia */
#endif /* CORTEXM3_STM32W108  */
    dst[i] = (r >> 8) & 0xff;
    if((i + 1) < len) {
      dst[i + 1] = r & 0xff;
    }
  }
#endif /* CONTIKI */
}
/*----------------------------------------------------------------*/

/*
 * Read location ID from NVMemory.
 * Return 0 on failure;
 */
uint32_t
get_location_id(void)
{
#if STORE_LOCATIONID_LOCALLY == 1
  return local_location_id;
#elif CONTIKI_TARGET_ANANAS == 1
  StStatus success;
  uint32_t buf;
  success = halCommonReadFromNvm(&buf, NVM_LOCATION_ID_OFFSET, NVM_LOCATION_ID_SIZE_B);
  if(success == ST_SUCCESS) {
    if(buf == 0xffffffff) {
      buf = 0;
    }
    /* location id is part of peer fingerprint, so it is stored in network byte order */
    return ntohl(buf);
  }
#else
#error Location ID storage undefined.
#endif /* PLATFORM == ANANAS */
  return 0;
}
/*----------------------------------------------------------------*/

/*
 * Write location ID to NVMemory.
 * zero is not a valid id.
 *
 * Return FALSE on failure.
 */
uint8_t
set_location_id(uint32_t id)
{
#if STORE_LOCATIONID_LOCALLY == 1
  local_location_id = id;
  id = 0xffffffff;
#endif /* STORE_LOCATIONID_LOCALLY == 1 */

#if CONTIKI_TARGET_ANANAS == 1
  StStatus success;
  uint32_t buf;

  if(id == 0) {
    return FALSE;
  }

  /* If location id already is configured to the same value, do not re-write it. */
  success = halCommonReadFromNvm(&buf, NVM_LOCATION_ID_OFFSET, NVM_LOCATION_ID_SIZE_B);
  if(success == ST_SUCCESS) {
    if(buf == htonl(id)) {
      return TRUE;
    }
  }

  /* location id is part of peer fingerprint, so it is stored in network byte order */
  id = htonl(id);
  success = halCommonWriteToNvm(&id, NVM_LOCATION_ID_OFFSET, NVM_LOCATION_ID_SIZE_B);
  if(success == ST_SUCCESS) {
    return TRUE;
  }
#else /* CONTIKI_TARGET_ANANAS == 1 */

  /* If it is handled, return true */
#if STORE_LOCATIONID_LOCALLY == 1
  return TRUE;
#endif /* STORE_LOCATIONID_LOCALLY == 1 */

#endif /* CONTIKI_TARGET_ANANAS == 1 */
  return FALSE;
}
/*----------------------------------------------------------------*/

/*
 * Read owner ID from NVMemory.
 * Return 0 on failure;
 */
uint32_t
get_owner_id(void)
{
#if CONTIKI_TARGET_ANANAS == 1
  StStatus success;
  uint32_t buf;
  success = halCommonReadFromNvm(&buf, NVM_OWNER_ID_OFFSET, NVM_OWNER_ID_SIZE_B);
  if(success == ST_SUCCESS) {
    if(buf == 0xffffffff) {
      buf = 0;
    }
    return buf;
  }
#endif /* PLATFORM == ANANAS */
  return 0;
}
/*----------------------------------------------------------------*/

/*
 * Write owner ID to NVMemory.
 * zero is not a valid id.
 *
 * Return FALSE on failure.
 */
uint8_t
set_owner_id(uint32_t id)
{
#if CONTIKI_TARGET_ANANAS == 1
  StStatus success;

  if(id == 0) {
    return FALSE;
  }
  success = halCommonWriteToNvm(&id, NVM_OWNER_ID_OFFSET, NVM_OWNER_ID_SIZE_B);
  if(success == ST_SUCCESS) {
    return TRUE;
  }
#endif /* PLATFORM == ANANAS */
  return FALSE;
}
/*----------------------------------------------------------------*/

/*
 * Read mfg test progress from NVMemory.
 * Return 0 on failure;
 */
uint16_t
get_mfg_test_progress(void)
{
#if defined NVM_MFG_TEST_PROGRESS && CONTIKI_TARGET_ANANAS == 1
  StStatus success;
  uint16_t buf;
  success = halCommonReadFromNvm(&buf, NVM_MFG_TEST_PROGRESS, NVM_MFG_TEST_PROGRESS_SIZE_B);
  if(success == ST_SUCCESS) {
    if(buf == 0xffff) {
      buf = 0;
    }
    return buf;
  }
#endif /* defined NVM_MFG_TEST_PROGRESS && PLATFORM == ANANAS */
  return 0;
}
/*----------------------------------------------------------------*/

/*
 * Write mfg test progress to NVMemory.
 * zero is not a valid value.
 *
 * Return FALSE on failure.
 */
uint8_t
set_mfg_test_progress(uint16_t progress)
{
#if defined NVM_MFG_TEST_PROGRESS && CONTIKI_TARGET_ANANAS == 1
  StStatus success;

  if(progress == 0) {
    return FALSE;
  }
  success = halCommonWriteToNvm(&progress, NVM_MFG_TEST_PROGRESS, NVM_MFG_TEST_PROGRESS_SIZE_B);
  if(success == ST_SUCCESS) {
    return TRUE;
  }
#endif /* defined NVM_MFG_TEST_PROGRESS && PLATFORM == ANANAS */
  return FALSE;
}
/*----------------------------------------------------------------*/
/* Sets to 0 the location_id, the location_public_key and the      / */
/* communication keys                                              / */
/*----------------------------------------------------------------*/
uint8_t
set_factory_default(void)
{
  uint8_t success;
  success = set_location_id(0xffffffff);
  if(success == FALSE) {
    return FALSE;
  }

#if HAVE_EEPROM_STORE
  eeprom_store_clear_config();
#endif /* HAVE_EEPROM_STORE */
  return success;
}
/*----------------------------------------------------------------*/
uint8_t
is_location_set(void)
{
  if(get_location_id() == 0) {
    return FALSE;
  }
  return TRUE;
}
/*----------------------------------------------------------------*/
