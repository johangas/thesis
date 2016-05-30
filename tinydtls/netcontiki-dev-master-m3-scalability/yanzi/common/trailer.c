/*
 * Copyright (c) 2012-2015, Yanzi Networks AB.
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

/*
 *
 * Description : Common trailer functions.
 *
 */

#include <string.h>
#include "crc.h"
#include "lib/crc32.h"
#include "trailer.h"

/*----------------------------------------------------------------*/
/*
 * Search for a trailer and between "start" + "len" and "start".
 * Return pointer to found trailer ot NULL on fail.
 */
trailer *
find_trailer(uint8_t *start, uint32_t len, uint64_t type)
{
  trailer *T;
  int i;
  uint32_t ll;
  uint8_t type_bytes[8];

  type_bytes[0] = (type >> 56) & 0xff;
  type_bytes[1] = (type >> 48) & 0xff;
  type_bytes[2] = (type >> 40) & 0xff;
  type_bytes[3] = (type >> 32) & 0xff;
  type_bytes[4] = (type >> 24) & 0xff;
  type_bytes[5] = (type >> 16) & 0xff;
  type_bytes[6] = (type >> 8) & 0xff;
  type_bytes[7] = type & 0xff;

  for(ll = len; ll > sizeof(trailer); ll -= 4) {
    T = (trailer *)(start + ll - sizeof(trailer));
    if(T->magic != TRAILER_MAGIC) {
      continue;
    }

    /*
     * Have trailer.
     * We only allow one traier, so if fields don't match
     * return NULL from here on.
     */
    if(T->length < (sizeof(trailer) >> 2)) {       /* length in 32bit words */
      return NULL;
    }
    if(T->algorithm != 4) {
      return NULL;
    }

    for(i = 0; i < 8; i++) {
      if(T->imageType[i] != type_bytes[i]) {
        return NULL;
      }
    }

    if((ntohl(T->imageStart) != (uint32_t)start)) {
      return NULL;
    }

    /* Hey, found one! */
    return T;
  }
  return NULL;
}
/*----------------------------------------------------------------*/
unsigned char
checkCRC32(unsigned char *buff, int len)
{

  unsigned char status = 0;

  if(MAGIC_REMAINDER == crc32(buff, len)) {
    status = 1;
  }

  return status;
}
/*----------------------------------------------------------------*/
/*
 * Verify image checksum.
 *
 * Return 0 on fail, non-zero on success.
 */
uint8_t
verify_checksum(trailer *T)
{
  uint32_t len;
  uint8_t *start;
  start = (uint8_t *)ntohl(T->imageStart);
  len = (uint32_t)T + sizeof(trailer) - (uint32_t)start;
  return checkCRC32(start, len);
}
/*----------------------------------------------------------------*/
/*
 * Get image status.
 */
uint32_t
get_image_status(image_info *image)
{
  trailer *T;
  uint32_t status;
  uint32_t *p;
  uint32_t erased = TRUE;
  uint8_t compare_type[8];

  compare_type[0] = (image->imageType >> 56) & 0xff;
  compare_type[1] = (image->imageType >> 48) & 0xff;
  compare_type[2] = (image->imageType >> 40) & 0xff;
  compare_type[3] = (image->imageType >> 32) & 0xff;
  compare_type[4] = (image->imageType >> 24) & 0xff;
  compare_type[5] = (image->imageType >> 16) & 0xff;
  compare_type[6] = (image->imageType >> 8) & 0xff;
  compare_type[7] = (image->imageType) & 0xff;

  for(p = (uint32_t *)image->startAddress;
      (uint8_t *)p < (uint8_t *)(image->startAddress + image->length);
      p++) {
    if(*p != 0xffffffff) {
      erased = FALSE;
      break;
    }
  }
  if(erased) {
    return IMAGE_STATUS_ERASED;
  }

  T = find_trailer((uint8_t *)image->startAddress, image->length, image->imageType);
  if(T == NULL) {
    return IMAGE_STATUS_BAD_SIZE;
  }

  if(verify_checksum(T)) {
    status = IMAGE_STATUS_OK;
    if(memcmp(T->imageType, compare_type, 8) != 0) {
      status |= IMAGE_STATUS_BAD_TYPE;
    }
    if(((uint32_t)get_image_status > image->startAddress) &&
       ((uint32_t)get_image_status < (image->startAddress + image->length))) {
      status |= IMAGE_STATUS_ACTIVE;
    } else {
      status |= IMAGE_STATUS_WRITABLE;
    }
  } else {
    return IMAGE_STATUS_BAD_CHECKSUM;
  }
  return status;
}
/*----------------------------------------------------------------*/
/*
 * Get image version.
 *
 * Return 0 on failure.
 */
uint64_t
get_image_version(image_info *image)
{
  trailer *T;
  uint64_t version = 0;

  T = find_trailer((uint8_t *)image->startAddress, image->length, image->imageType);
  if(T == NULL) {
    return 0;
  }
  version |= (uint64_t)T->imageVersion[0] << 56;
  version |= (uint64_t)T->imageVersion[1] << 48;
  version |= (uint64_t)T->imageVersion[2] << 40;
  version |= (uint64_t)T->imageVersion[3] << 32;
  version |= (uint64_t)T->imageVersion[4] << 24;
  version |= (uint64_t)T->imageVersion[5] << 16;
  version |= (uint64_t)T->imageVersion[6] << 8;
  version |= (uint64_t)T->imageVersion[7];

  return version;
}
/*----------------------------------------------------------------*/
/*
 * Get image length.
 *
 * Return maximum image length on failure.
 */
uint32_t
get_image_length(image_info *image)
{
  trailer *T;

  T = find_trailer((uint8_t *)image->startAddress, image->length, image->imageType);
  if(T == NULL) {
    return image->length;
  }

  return (uint32_t)((uint8_t *)T - image->startAddress + sizeof(trailer));
}
/*----------------------------------------------------------------*/
