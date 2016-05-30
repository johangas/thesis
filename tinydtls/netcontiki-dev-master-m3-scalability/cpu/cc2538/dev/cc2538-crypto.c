/*
 * Copyright (c) 2013, ADVANSEE - http://www.advansee.com/
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
 * \addtogroup cc2538-crypto
 * @{
 *
 * \file
 * Implementation of the cc2538 AES/SHA cryptoprocessor driver
 */
#include "contiki.h"
#include "sys/energest.h"
#include "dev/sys-ctrl.h"
#include "dev/nvic.h"
#include "dev/cc2538-crypto.h"
#include "dev/cc2538-aes.h"
#include "dev/cc2538-ecb.h"
#include "reg.h"
#include "lpm.h"

#include <stdbool.h>
/*---------------------------------------------------------------------------*/
static volatile struct process *notification_process = NULL;
/*---------------------------------------------------------------------------*/
/** \brief The AES/SHA cryptoprocessor ISR
 *
 *        This is the interrupt service routine for the AES/SHA
 *        cryptoprocessor.
 *
 *        This ISR is called at worst from PM0, so lpm_exit() does not need
 *        to be called.
 */
void
cc2538_crypto_isr(void)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  nvic_interrupt_unpend(NVIC_INT_AES);
  nvic_interrupt_disable(NVIC_INT_AES);

  if(notification_process != NULL) {
    process_poll((struct process *)notification_process);
    notification_process = NULL;
  }

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
/*---------------------------------------------------------------------------*/
#if LPM_CONF_ENABLE != 0
static bool
permit_pm1(void)
{
  return REG(AES_CTRL_ALG_SEL) == 0;
}
#endif /* LPM_CONF_ENABLE != 0 */
/*---------------------------------------------------------------------------*/
void
cc2538_crypto_init(void)
{
  volatile int i;

  lpm_register_peripheral(permit_pm1);

  cc2538_crypto_enable();

  /* Reset the AES/SHA cryptoprocessor */
  REG(SYS_CTRL_SRSEC) |= SYS_CTRL_SRSEC_AES;
  for(i = 0; i < 16; i++);
  REG(SYS_CTRL_SRSEC) &= ~SYS_CTRL_SRSEC_AES;
}
/*---------------------------------------------------------------------------*/
void
cc2538_crypto_enable(void)
{
  /* Enable the clock for the AES/SHA cryptoprocessor */
  REG(SYS_CTRL_RCGCSEC) |= SYS_CTRL_RCGCSEC_AES;
  REG(SYS_CTRL_SCGCSEC) |= SYS_CTRL_SCGCSEC_AES;
  REG(SYS_CTRL_DCGCSEC) |= SYS_CTRL_DCGCSEC_AES;
}
/*---------------------------------------------------------------------------*/
void
cc2538_crypto_disable(void)
{
  /* Gate the clock for the AES/SHA cryptoprocessor */
  REG(SYS_CTRL_RCGCSEC) &= ~SYS_CTRL_RCGCSEC_AES;
  REG(SYS_CTRL_SCGCSEC) &= ~SYS_CTRL_SCGCSEC_AES;
  REG(SYS_CTRL_DCGCSEC) &= ~SYS_CTRL_DCGCSEC_AES;
}
/*---------------------------------------------------------------------------*/
void
cc2538_crypto_register_process_notification(struct process *p)
{
  notification_process = p;
}

/*---------------------------------------------------------------------------*/
static void
set_key(const uint8_t *key)
{
  cc2538_crypto_init();
  cc2538_aes_load_keys(key, AES_KEY_STORE_SIZE_KEY_SIZE_128, 1, 0);
}
/*---------------------------------------------------------------------------*/
static void
encrypt(uint8_t *plaintext_and_result)
{
  cc2538_ecb_encrypt_start(0, plaintext_and_result, 16, NULL);
  /* Wait for operation to complete */
  while(!cc2538_ecb_encrypt_check_status());
  /* Finish operation and clean-up */
  cc2538_ecb_encrypt_get_result();
}
/*---------------------------------------------------------------------------*/
const struct aes_128_driver cc2538_aes_128_driver = {
  set_key,
  encrypt
};

/** @} */
