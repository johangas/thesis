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
#include <string.h>
#include "bootloader.h"
#include "trailer.h"

#define SEARCH_MFG_START 0x200000
#define SEARCH_MFG_END   0x203000
#define IMAGES 2

mfg_area *our_mfg_area = NULL;
boot_frontpanel *fpinfo = NULL;
void _exit(int status);
void find_mfg_area();
void boot_image(image_info *img);
void flash_led(uint8_t n);
int is_button_pressed();
void busywait(uint32_t ms);

void
boot_failed0()
{
  for(;;) {
  }
}
void
boot_failed1()
{
  for(;;) {
    flash_led(1);
    busywait(800);
  }
}
void
boot_failed2()
{
  for(;;) {
    flash_led(2);
    busywait(800);
  }
}
void
boot_failed3()
{
  for(;;) {
    flash_led(3);
    busywait(800);
  }
}
void
boot_failed4()
{
  for(;;) {
    flash_led(4);
    busywait(800);
  }
}
void
boot_failed5()
{
  for(;;) {
    flash_led(5);
    busywait(800);
  }
}
void
boot_failed6()
{
  for(;;) {
    flash_led(6);
    busywait(800);
  }
}
void
boot_failed7()
{
  for(;;) {
    flash_led(7);
    busywait(800);
  }
}
void
boot_failed8()
{
  for(;;) {
    flash_led(8);
    busywait(800);
  }
}
/*----------------------------------------------------------------*/

/*
 * The main loader. Search for manufacturing area, images, select the "best"
 * one, verify checksum and go.
 */
int
main(void)
{
  uint8_t search_images = IMAGES;
  int i;
  int versions;
  image_info *img;
  image_info *images;
  static trailer *found[IMAGES];

  memset(found, 0, sizeof(found));

  if(our_mfg_area == NULL) {
    find_mfg_area();
  }
  if(our_mfg_area == NULL) {
    boot_failed7();
  }

  images = (image_info *)our_mfg_area->images;
  if(our_mfg_area->extra_info == MFG_EXTRA_INFO_FRONTPANEL) {
    fpinfo = (boot_frontpanel *)((void *)&our_mfg_area[1] + (sizeof(image_info) * our_mfg_area->numberOfImages));
  }
  if(fpinfo != NULL) {
    /* Config LED port if enabled onf should be configured */
    if((fpinfo->led_config & BOOT_FRONT_LED_CONFIG_DO_INDICATE) == BOOT_FRONT_LED_CONFIG_DO_INDICATE) {
      if((fpinfo->led_config & BOOT_FRONT_LED_CONFIG_DO_CONFIG) == BOOT_FRONT_LED_CONFIG_DO_CONFIG) {
        *((uint32_t *)fpinfo->led_config_address) = fpinfo->led_config_value;
        *((uint32_t *)fpinfo->led_data_address) = fpinfo->led_data_value_off;
      }
    }
    /* Config pushbutton port if enabled onf should be configured */
    if((fpinfo->pushbutton_config & BOOT_FRONT_BUTTON_READ_BUTTON) == BOOT_FRONT_BUTTON_READ_BUTTON) {
      if((fpinfo->pushbutton_config & BOOT_FRONT_BUTTON_CONFIG_DO_CONFIG) == BOOT_FRONT_BUTTON_CONFIG_DO_CONFIG) {
        *((uint32_t *)fpinfo->pushbutton_config_address) = fpinfo->pushbutton_config_value;
      }
    }
  }

  if(search_images > our_mfg_area->numberOfImages) {
    search_images = our_mfg_area->numberOfImages;
  }

  for(i = 0; i < search_images; i++) {
    img = &images[i];
    found[i] = find_trailer((uint8_t *)img->startAddress, img->length, img->imageType);
  }

  if(found[1] == NULL) {
    if(found[0] == NULL) {
      boot_failed8();
    } else {
      if(verify_checksum(found[0])) {
        flash_led(1);
        boot_image(&images[0]);
      } else {
        boot_failed1();
      }
    }
  } else if(found[0] == NULL) {
    if(found[1] == NULL) {
      boot_failed2();
    } else {
      if(verify_checksum(found[1])) {
        flash_led(2);
        boot_image(&images[1]);
      } else {
        boot_failed3();
      }
    }
  } else {
    /* Both images found */
#if HAVE_BOOT_DATA
    extern uint32_t _boot_data;
    struct boot_data *boot_data = (struct boot_data *)&_boot_data;
#endif /* HAVE_BOOT_DATA */
    versions = memcmp(found[0]->imageVersion, found[1]->imageVersion, 8);

    if(is_button_pressed()) {
      if(versions < 0) {
        versions = 1;
      } else {
        versions = -1;
      }
    }

#if HAVE_BOOT_DATA
    if((boot_data->magic0 == BOOT_MAGIC_0) &&
       (boot_data->magic1 == BOOT_MAGIC_1) &&
       (boot_data->magic2 == BOOT_MAGIC_2) &&
       (boot_data->magic3 == BOOT_MAGIC_3) &&
       (boot_data->magic4 == BOOT_MAGIC_4) &&
       (boot_data->magic5 == BOOT_MAGIC_5) &&
       (boot_data->magic6 == BOOT_MAGIC_6)) {
      if(boot_data->next_boot_image == 1) {
        versions = 1;
      } else if(boot_data->next_boot_image == 2) {
        versions = -1;
      }
    }
#endif /* HAVE_BOOT_DATA */

    if(versions >= 0) {
      if(verify_checksum(found[0])) {
        flash_led(1);
        boot_image(&images[0]);
      } else if(verify_checksum(found[1])) {
        flash_led(2);
        boot_image(&images[1]);
      } else {
        boot_failed4();
      }
    } else {
      if(verify_checksum(found[1])) {
        flash_led(2);
        boot_image(&images[1]);
      } else if(verify_checksum(found[0])) {
        flash_led(1);
        boot_image(&images[0]);
      } else {
        boot_failed5();
      }
    }
  }
  /* NOTREACHED */
  boot_failed6();
  return 0;
}
/*----------------------------------------------------------------*/

/*
 * SystemInit dummy for compatibility with start code that calls it.
 */
void
SystemInit(void)
{
}
/*----------------------------------------------------------------*/

void __set_MSP(uint32_t topOfMainStack) __attribute__((naked));
void
__set_MSP(uint32_t topOfMainStack)
{
  __asm volatile ("MSR msp, %0\n\t"
                  "BX  lr     \n\t" : : "r" (topOfMainStack));
}
/*----------------------------------------------------------------*/

/*
 * Start the payload. will not return.
 */
void
boot_image(image_info *img)
{
  typedef void (*EntryPoint)(void);

  EntryPoint entryPoint = (EntryPoint)(*(uint32_t *)(img->startAddress + 4));
  uint32_t stack = *(uint32_t *)(img->startAddress);
  *((volatile uint32_t *)(0xe000ed08)) = img->startAddress;
  __set_MSP(stack);

  entryPoint();
  for(;;) {
    flash_led(20);
    busywait(800);
  }
}
/*----------------------------------------------------------------*/

/*
 * Search memory for a manufacturing area, and sets "our_mfg_area" if found.
 */
void
find_mfg_area()
{
  uint32_t p;
  uint32_t compare;

  compare = MFG_MAGIC0 + (MFG_MAGIC1 << 8) + (MFG_MAGIC2 << 16) + (MFG_MAGIC3 << 24);

  for(p = SEARCH_MFG_START; p < SEARCH_MFG_END; p += 4) {
    if(*((uint32_t *)p) == compare) {
      our_mfg_area = (mfg_area *)p;
      /* images = our_mfg_area->images; */
      break;
    }
  }
}
/*----------------------------------------------------------------*/

/*
 * Simple non-accurate busy wait delay.
 */
void
busywait(uint32_t ms)
{
  uint32_t i;
  /*  uint32_t n; */

  while(ms--) {
    for(i = 0; i < 3037; i++) {
      asm ("nop");
    }
  }
}
/*----------------------------------------------------------------*/

/**
 * Flash led, if configured, n times.
 */
void
flash_led(uint8_t n)
{
  int i;
  for(i = 0; i < n; i++) {
    if(fpinfo != NULL) {
      if((fpinfo->led_config & BOOT_FRONT_LED_CONFIG_DO_INDICATE) == BOOT_FRONT_LED_CONFIG_DO_INDICATE) {
        *((uint32_t *)fpinfo->led_data_address) = fpinfo->led_data_value_on;
        busywait(100);
        *((uint32_t *)fpinfo->led_data_address) = fpinfo->led_data_value_off;
        busywait(300);
      }
    }
  }
}
/*----------------------------------------------------------------*/

/**
 * Return non-zero if button is congured and pressed.
 */
int
is_button_pressed()
{
  volatile uint32_t data;
  uint32_t mask;

  if(fpinfo != NULL) {
    if((fpinfo->pushbutton_config & BOOT_FRONT_BUTTON_READ_BUTTON) == BOOT_FRONT_BUTTON_READ_BUTTON) {
      data = *((volatile uint32_t *)(fpinfo->pushbutton_data_address + ((fpinfo->pushbutton_data_mask & 0xff) << 2)));
      mask = fpinfo->pushbutton_data_mask;
      if((data & mask) == fpinfo->pushbutton_data_pushed_after_mask) {
        return 1;
      } else {
        return 0;
      }
    }
  }
  return 0;
}
/*----------------------------------------------------------------*/
