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

#ifndef API_H_
#define API_H_

#include <stddef.h>
#include "vargen.h"

/*
 * Common macros
 */
#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

uint8_t *getDid(void);
uint64_t get_product_type(void);
uint64_t boottimerIEEE64(void);
uint32_t boottimerIEEE64in32(void);
const uint8_t *getswrevision(void);
const uint8_t *getcompiletime(void);
uint64_t boottimer_elapsed(uint64_t when);
uint64_t boottimer_read(void);
void usleep(uint32_t usec);
void random_fill(uint8_t *dst, size_t len);
int32_t milliseconds_until(uint64_t future);
uint32_t get_location_id(void);
uint16_t get_mfg_test_progress(void);
uint8_t set_mfg_test_progress(uint16_t progress);
uint8_t set_factory_default(void);

uint8_t is_location_set(void);

#define swap64(a) (((a >> 56) & 0x00000000000000ffULL) | \
                   ((a >> 40) & 0x000000000000ff00ULL) | \
                   ((a >> 24) & 0x0000000000ff0000ULL) | \
                   ((a >>  8) & 0x00000000ff000000ULL) | \
                   ((a <<  8) & 0x000000ff00000000ULL) | \
                   ((a << 24) & 0x0000ff0000000000ULL) | \
                   ((a << 40) & 0x00ff000000000000ULL) | \
                   ((a << 56) & 0xff00000000000000ULL))

#ifndef __LITTLE_ENDIAN__

 #ifdef __i386__
  #define __LITTLE_ENDIAN__
 #endif /* __i386__ */
 #ifdef __ARM_EABI__
  #define __LITTLE_ENDIAN__
 #endif /* __ARM_EABI__ */

 #ifdef __BYTE_ORDER__
  #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define __LITTLE_ENDIAN__
  #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define __BIG_ENDIAN__
  #elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
    #error Seriously?  a PDP?
  #else
    #error Unknown endian
  #endif
 #endif /* __BYTE_ORDER__ */

#endif /* __LITTLE_ENDIAN__ */

#ifdef __LITTLE_ENDIAN__

#ifndef ntohs
#define ntohs(x) (((x >> 8) & 0xff) | ((x << 8) & 0xff00))
#endif
#ifndef htons
#define htons(x) (((x >> 8) & 0xff) | ((x << 8) & 0xff00))
#endif
#ifndef ntohl
#define ntohl(x) (((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | ((x << 24) & 0xff000000))
#endif
#ifndef htonl
#define htonl(x) (((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | ((x << 24) & 0xff000000))
#endif
#ifndef ntoh64
#define ntoh64(a) swap64(a)
#endif
#else /* __LITTLE_ENDIAN__ */
#ifndef ntohs
#define ntohs(x) x
#endif
#ifndef htons
#define htons(x) x
#endif
#ifndef ntohl
#define ntohl(x) x
#endif
#ifndef htonl
#define htonl(x) x
#endif
#ifndef ntoh64
#define ntoh64(a) a
#endif
#error Remove this if you are really compiling for a big endian target.
#endif /* __LITTLE_ENDIAN__ */

#endif /* API_H_ */
