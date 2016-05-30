/*
 * Copyright (c) 2015, Yanzi Networks AB.
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
 */

#include "contiki.h"
#include "platform-felicia.h"
#include "dev/rom-util.h"
#include "dev/sys-ctrl.h"
#include "reg.h"
#include "trailer.h"

extern mfg_area *flash_mfg_area;

void board_init(void);

/*---------------------------------------------------------------------------*/
void
platform_init(void)
{
  board_init();
}
/*---------------------------------------------------------------------------*/
uint8_t
platform_get_reset_cause(void)
{
  switch((REG(SYS_CTRL_CLOCK_STA) & SYS_CTRL_CLOCK_STA_RST) >> 22) {
  case 0:
    return PLATFORM_RESET_CAUSE_POWER_FAIL;
  case 1:
    return PLATFORM_RESET_CAUSE_HARDWARE;
  case 2:
    return PLATFORM_RESET_CAUSE_WATCHDOG;
  case 3:
    return PLATFORM_RESET_CAUSE_SOFTWARE;
  default:
    return PLATFORM_RESET_CAUSE_NONE;
  }
}
/*---------------------------------------------------------------------------*/
const char *
platform_reset_cause(void)
{
  switch((REG(SYS_CTRL_CLOCK_STA) & SYS_CTRL_CLOCK_STA_RST) >> 22) {
  case 0:
    return "Power fail";
  case 1:
    return "External Reset";
  case 2:
    return "Watchdog Reset";
  case 3:
    return "Software Reset";
  default:
    return "Unknown";
  }
}
/*---------------------------------------------------------------------------*/
int
platform_get_capabilities(uint64_t *capabilities)
{
  if(flash_mfg_area != NULL && capabilities != NULL) {
    *capabilities = flash_mfg_area->capabilities;
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
uint32_t
platform_get_bootloader_version(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
void
platform_reboot(void)
{
  rom_util_reset_device();
}
/*---------------------------------------------------------------------------*/
void
platform_reboot_to_selected_image(uint8_t image)
{
  /* TODO add support for other image */
  platform_reboot();
}
/*---------------------------------------------------------------------------*/
