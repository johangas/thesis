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

#ifndef FLASH_H_
#define FLASH_H_

#include "contiki-conf.h"

/* The embedded flash memory consists of up to 256 pages of 2048 bytes each. */
#define FLASH_PAGE_SIZE                2048
/* The start of the flash memory in CC2538 is on 0x200000. */
#define FLASH_START                    ((uint32_t) 0x00200000)
/* The length of the flash memory is 0x0007FFD4. */
//#define FLASH_SIZE                     0x0007FFD4
#define FLASH_SIZE                       0x00080000
/* End of flash is relative to start and length definitions. */
#define FLASH_END                      (FLASH_START + FLASH_SIZE)
/* The minimum length for a write operation is 4 bytes (word). */
#define FLASH_MIN_WRITE                ((uint32_t) 4)
/* The minimum erasable unit is a complete flash page. */
#define FLASH_MIN_ERASE                FLASH_PAGE_SIZE



/* The following is a test to check read/write flash operations.
 * We allocate the 6-th page from the end */
#define FLASH_USR_START_ADDR           ((FLASH_END) - 6*(FLASH_PAGE_SIZE))
#define FLASH_USR_SIZE                 (FLASH_PAGE_SIZE)

/*-------------------------------- Public API -------------------------------*/
void flash_lock(void);
void flash_unlock(void);
unsigned long flash_write(uint32_t *buf, unsigned long addr,
  unsigned long byte_count);
unsigned long flash_write_word(uint32_t *buf, unsigned long addr);
unsigned long flash_erase(uint32_t addr, uint32_t size);
unsigned long flash_get_flash_size(void);
void flash_read(uint8_t *buf, unsigned long addr, unsigned long byte_count);
/*---------------------------------------------------------------------------*/

#endif /* FLASH_H_ */
