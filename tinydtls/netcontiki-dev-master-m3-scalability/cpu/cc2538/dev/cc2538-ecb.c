/*
 * Original file:
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Port to Contiki:
 * Copyright (c) 2013, ADVANSEE - http://www.advansee.com/
 * All rights reserved.
 *
 * Modified to implement only AES-ECB mode.
 * Copyright (c) 2015, Yanzi Networks AB
 * Oriol Piñol Piñol <oriol@yanzi.se>
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \addtogroup cc2538-ecb
 * @{
 *
 * \file
 * Implementation of the cc2538 AES-ECB driver
 */
#include "contiki.h"
#include "sys/cc.h"
#include "dev/rom-util.h"
#include "dev/nvic.h"
#include "dev/cc2538-aes.h"
#include "dev/cc2538-ecb.h"
#include "reg.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
uint8_t
cc2538_ecb_encrypt_start(uint8_t key_area, void *pdata, uint16_t pdata_len,
                       struct process *process)
{
  if(REG(AES_CTRL_ALG_SEL) != 0x00000000) {
    return CC2538_CRYPTO_RESOURCE_IN_USE;
  }

  /* Workaround for AES registers not retained after PM2 */
  REG(AES_CTRL_INT_CFG) = AES_CTRL_INT_CFG_LEVEL;
  REG(AES_CTRL_INT_EN) = AES_CTRL_INT_EN_DMA_IN_DONE |
                         AES_CTRL_INT_EN_RESULT_AV;

  REG(AES_CTRL_ALG_SEL) = AES_CTRL_ALG_SEL_AES;
  REG(AES_CTRL_INT_CLR) = AES_CTRL_INT_CLR_DMA_IN_DONE |
                          AES_CTRL_INT_CLR_RESULT_AV;

  REG(AES_KEY_STORE_READ_AREA) = key_area;

  /* Wait until key is loaded to the AES module */
  while(REG(AES_KEY_STORE_READ_AREA) & AES_KEY_STORE_READ_AREA_BUSY);

  /* Check for Key Store read error */
  if(REG(AES_CTRL_INT_STAT) & AES_CTRL_INT_STAT_KEY_ST_RD_ERR) {
    /* Clear the Keystore Read error bit */
    REG(AES_CTRL_INT_CLR) = AES_CTRL_INT_CLR_KEY_ST_RD_ERR;
    /* Disable the master control / DMA clock */
    REG(AES_CTRL_ALG_SEL) = 0x00000000;
    return AES_KEYSTORE_READ_ERROR;
  }

  /* Program AES-ECB encryption */
  REG(AES_AES_CTRL) = AES_AES_CTRL_DIRECTION_ENCRYPT; /* Encryption */

  /* Write the length of the crypto block (lo) */
  REG(AES_AES_C_LENGTH_0) = pdata_len;
  /* Write the length of the crypto block (hi) */
  REG(AES_AES_C_LENGTH_1) = 0;

  /* Clear interrupt status */
  REG(AES_CTRL_INT_CLR) = AES_CTRL_INT_CLR_DMA_IN_DONE |
                          AES_CTRL_INT_CLR_RESULT_AV;

  if(process != NULL) {
    cc2538_crypto_register_process_notification(process);
    nvic_interrupt_unpend(NVIC_INT_AES);
    nvic_interrupt_enable(NVIC_INT_AES);
  }

  /* Enable result available bit in interrupt enable */
  REG(AES_CTRL_INT_EN) = AES_CTRL_INT_EN_RESULT_AV;

  if(pdata_len != 0) {
    /* Configure DMAC
     * Enable DMA channel 0 */
    REG(AES_DMAC_CH0_CTRL) = AES_DMAC_CH_CTRL_EN;
    /* Base address of the payload data in ext. memory */
    REG(AES_DMAC_CH0_EXTADDR) = (uint32_t)pdata;
    /* Payload data length in bytes */
    REG(AES_DMAC_CH0_DMALENGTH) = pdata_len;

    /* Enable DMA channel 1 */
    REG(AES_DMAC_CH1_CTRL) = AES_DMAC_CH_CTRL_EN;
    /* Base address of the output data buffer */
    REG(AES_DMAC_CH1_EXTADDR) = (uint32_t)pdata;
    /* Output data length in bytes */
    REG(AES_DMAC_CH1_DMALENGTH) = pdata_len;
  }

  return CC2538_CRYPTO_SUCCESS;
}

/*---------------------------------------------------------------------------*/
uint8_t
cc2538_ecb_encrypt_check_status(void)
{
  return !!(REG(AES_CTRL_INT_STAT) &
            (AES_CTRL_INT_STAT_DMA_BUS_ERR | AES_CTRL_INT_STAT_KEY_ST_WR_ERR |
             AES_CTRL_INT_STAT_KEY_ST_RD_ERR | AES_CTRL_INT_STAT_RESULT_AV));
}
/*---------------------------------------------------------------------------*/
uint8_t
cc2538_ecb_encrypt_get_result(void)
{
  uint32_t aes_ctrl_int_stat;

  aes_ctrl_int_stat = REG(AES_CTRL_INT_STAT);
  /* Clear the error bits */
  REG(AES_CTRL_INT_CLR) = AES_CTRL_INT_CLR_DMA_BUS_ERR |
                          AES_CTRL_INT_CLR_KEY_ST_WR_ERR |
                          AES_CTRL_INT_CLR_KEY_ST_RD_ERR;

  nvic_interrupt_disable(NVIC_INT_AES);
  cc2538_crypto_register_process_notification(NULL);

  /* Disable the master control / DMA clock */
  REG(AES_CTRL_ALG_SEL) = 0x00000000;

  if(aes_ctrl_int_stat & AES_CTRL_INT_STAT_DMA_BUS_ERR) {
    return CC2538_CRYPTO_DMA_BUS_ERROR;
  }
  if(aes_ctrl_int_stat & AES_CTRL_INT_STAT_KEY_ST_WR_ERR) {
    return AES_KEYSTORE_WRITE_ERROR;
  }
  if(aes_ctrl_int_stat & AES_CTRL_INT_STAT_KEY_ST_RD_ERR) {
    return AES_KEYSTORE_READ_ERROR;
  }

  return CC2538_CRYPTO_SUCCESS;
}

/** @} */
