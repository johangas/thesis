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

#include "dev/watchdog.h"
#include "dev/rom-util.h"
#include "flash.h"
#include "string.h" /* For memcpy() */
#include "cpu.h"

/*---------------------------------------------------------------------------*/
/*
 * Flash lock and unlock functions are empty in CC2538.
 * TODO Consider implementing soft lock/unlock functions instead.
 */
void
flash_lock(void)
{
}
/*---------------------------------------------------------------------------*/
void
flash_unlock(void)
{
}
/*---------------------------------------------------------------------------*/
/*
 * Write an array of bytes to flash.
 */
unsigned long
flash_write(uint32_t *buf, unsigned long addr, unsigned long byte_count)
{
  watchdog_periodic();
  return rom_util_program_flash(buf, addr, byte_count);
}
/*---------------------------------------------------------------------------*/
unsigned long
flash_write_word(uint32_t *buf, unsigned long addr)
{
  return flash_write(buf, addr, sizeof(uint32_t));
}
/*---------------------------------------------------------------------------*/
/*
 * Clear a flash page.
 */
unsigned long
flash_erase(uint32_t addr, uint32_t size)
{
  watchdog_periodic();
  return rom_util_page_erase(addr, size);
}
/*---------------------------------------------------------------------------*/
/*
 * Return the size of flash in bytes.
 */
unsigned long
flash_get_flash_size(void)
{
  return rom_util_get_flash_size();
}
/*---------------------------------------------------------------------------*/
void
flash_read(uint8_t *buf, unsigned long addr, unsigned long byte_count)
{
  memcpy(buf, (unsigned long *)addr, byte_count);
}
/*---------------------------------------------------------------------------*/
