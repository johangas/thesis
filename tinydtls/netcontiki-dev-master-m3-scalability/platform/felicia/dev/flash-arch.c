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

#include "flash.h"
#include <stdio.h>
#if HAVE_OAM
#include "instance-flash.h"

/* Felicia flash contains pages of 2048 bytes length. */
#define MFB_PAGE_SIZE_B     2048
#define MFB_PAGE_MASK_B     0xFFFFF800

void
FLASH_Unlock(void)
{
  flash_unlock();
}
/*---------------------------------------------------------------------------*/
void
FLASH_Lock(void)
{
  flash_lock();
}
/*---------------------------------------------------------------------------*/
FLASH_Status
FLASH_WaitForLastOperation(void)
{
  return FLASH_COMPLETE;
}
/*---------------------------------------------------------------------------*/
FLASH_Status
FLASH_EraseSector(uint32_t address)
{
  FLASH_Status status = FLASH_COMPLETE;
  FLASH_Unlock();

  /* Actual erase operation is performed here. */
  unsigned long erase_result =
    flash_erase(address & MFB_PAGE_MASK_B, MFB_PAGE_SIZE_B);

  FLASH_Lock();

  if(erase_result) {
    /* TODO map the function output to FLASH_Status return values. For now
     * w simply use the Write protection error
     */
    status = FLASH_ERROR_WRP;
  }
  return status;
} 
/*---------------------------------------------------------------------------*/
FLASH_Status
FLASH_ProgramWord(uint32_t address, uint32_t data)
{
  FLASH_Status status = FLASH_COMPLETE;
  FLASH_Unlock();
  uint8_t write_count = 0;

  while(write_count < 2) {
    unsigned long write_result = flash_write_word(&data, address);
    if(write_result) {
      /* TODO map the function output to FLASH_Status return values. For now
       * we simply use the PG error flag. 
       */
      printf("flash-arch: write-err\n");
      return FLASH_ERROR_PG;
    }
    /* Read flash back to verify the write operation */
    uint32_t read_data;
    flash_read((uint8_t *)&read_data, address, sizeof(read_data));
    if(read_data == data) {
      return status;
    } else {
      printf("flash-arch: mismatch %lx %lx %lx %lx\n",
        data, read_data, address, (*(uint32_t *)address));
      write_count++;
    }
  }
  printf("flash-arch: verify-err\n");
  /* We couldn't verify the read for 2 consecutive times, so report an error */
  return FLASH_ERROR_PG;
}
/*---------------------------------------------------------------------------*/
#endif
