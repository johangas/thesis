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

#ifndef PLATFORM_FELICIA_H_
#define PLATFORM_FELICIA_H_

/* Include macros for enabling/disabling interrupts */
#include "cpu.h"

#define PLATFORM_RESET_CAUSE_NONE       0
#define PLATFORM_RESET_CAUSE_LOCKUP     1
#define PLATFORM_RESET_CAUSE_DEEP_SLEEP 2
#define PLATFORM_RESET_CAUSE_SOFTWARE   3
#define PLATFORM_RESET_CAUSE_WATCHDOG   4
#define PLATFORM_RESET_CAUSE_HARDWARE   5
#define PLATFORM_RESET_CAUSE_POWER_FAIL 6
#define PLATFORM_RESET_CAUSE_COLD_START 7

#define CAN_DO_UC_IPV6 1
#define CAN_DO_UC_IPV4 0

uint8_t platform_get_reset_cause(void);
const char *platform_reset_cause(void);

int platform_get_capabilities(uint64_t *capabilities);
uint32_t platform_get_bootloader_version(void);

void platform_reboot(void);
void platform_reboot_to_selected_image(uint8_t image);

void platform_init(void);

#define PLATFORM_REBOOT() platform_reboot()

#endif /* PLATFORM_FELICIA_H_ */
