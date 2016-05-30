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

#ifndef TRAILER_H_
#define TRAILER_H_

#include <stdint.h>
#include "byteorder.h"

#define TRAILER_MAGIC 0x66745951 /* host byte order */

/* image_info must be n * 8 bytes total to avoid padding. */
typedef struct {
  uint64_t imageType;
  uint32_t startAddress;
  uint32_t length;
  uint8_t numberOfSectors;
  uint8_t sectors[15];
} image_info;

#define BOOT_MAGIC_0 0x9f
#define BOOT_MAGIC_1 0xd2
#define BOOT_MAGIC_2 0x44
#define BOOT_MAGIC_3 0xe1
#define BOOT_MAGIC_4 0x95
#define BOOT_MAGIC_5 0x2a
#define BOOT_MAGIC_6 0xca
#define BOOT_MAGIC_NO_SELECTED_IMAGE 0

typedef struct boot_data {
  uint8_t magic0;
  uint8_t magic1;
  uint8_t magic2;
  uint8_t magic3;
  uint8_t magic4;
  uint8_t magic5;
  uint8_t magic6;
  uint8_t next_boot_image;
  uint32_t isr_vector;
  uint8_t extended_reset_cause;
  uint8_t unused_0;
  uint8_t unused_1;
  uint8_t unused_2;
} boot_data;

#define BOOT_FRONT_LED_CONFIG_DO_INDICATE  0x01
#define BOOT_FRONT_LED_CONFIG_DO_CONFIG    0x02
#define BOOT_FRONT_BUTTON_READ_BUTTON      0x01
#define BOOT_FRONT_BUTTON_CONFIG_DO_CONFIG 0x02

typedef struct boot_frontpanel {
  uint8_t led_config;
  uint8_t pushbutton_config;
  uint8_t reserved0;
  uint8_t reserved1;
  uint32_t led_config_address;
  uint32_t led_config_value;
  uint32_t led_data_address;
  uint32_t led_data_value_on;
  uint32_t led_data_value_off;
  uint32_t pushbutton_config_address;
  uint32_t pushbutton_config_value;
  uint32_t pushbutton_data_address;
  uint32_t pushbutton_data_mask;
  uint32_t pushbutton_data_pushed_after_mask;
} boot_frontpanel;

#define MFG_CAPABILITY_CAN_SLEEP          0x01
#define MFG_CAPABILITY_BARBIE_RTS         0x02
#define MFG_CAPABILITY_BARBIE_MISSING_FAN 0x04

#define MFG_MAGIC0 0x01
#define MFG_MAGIC1 0x59
#define MFG_MAGIC2 0x5d
#define MFG_MAGIC3 0xe1

#define MFG_EXTRA_INFO_FRONTPANEL 0x01

typedef struct {
  uint8_t revision;
  uint8_t magic1;
  uint8_t magic2;
  uint8_t magic3;
  uint8_t numberOfImages;
  uint8_t extra_info;
  uint8_t reserved1;
  uint8_t reserved2;
  uint64_t capabilities;
  uint8_t mfgkey[16];
  image_info images[];
} mfg_area;

typedef struct {
  uint8_t imageType[8];
  uint8_t imageVersion[8];
  uint32_t imageStart;
  uint8_t reserved;
  uint8_t salt;
  uint8_t algorithm;
  uint8_t length;
  uint32_t magic;
  uint32_t digest;
} trailer;

typedef enum {
  DIGEST_reserved = 0x00,
  DIGEST_MD5 = 0x01,
  DIGEST_SHA1 = 0x02,
  DIGEST_none = 0x03,       /* no digest present, just trailer */
  DIGEST_CRC32 = 0x04,
} digest_algorithm;

#define IMAGE_STATUS_OK            0x00
#define IMAGE_STATUS_BAD_CHECKSUM  0x01
#define IMAGE_STATUS_BAD_TYPE      0x02
#define IMAGE_STATUS_BAD_SIZE      0x04
#define IMAGE_STATUS_NEXT_BOOT     0x08
#define IMAGE_STATUS_WRITABLE      0x10
#define IMAGE_STATUS_ACTIVE        0x20
#define IMAGE_STATUS_ERASED        0x40

trailer *find_trailer(uint8_t *start, uint32_t len, uint64_t type);
uint8_t verify_checksum(trailer *T);
uint32_t get_image_status(image_info *image);
uint64_t get_image_version(image_info *image);
uint32_t get_image_length(image_info *image);

extern image_info *images;

#endif /* TRAILER_H_ */
