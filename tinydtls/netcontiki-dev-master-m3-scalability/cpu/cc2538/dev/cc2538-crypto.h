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
 * \addtogroup cc2538
 * @{
 *
 * \defgroup cc2538-crypto cc2538 AES/SHA cryptoprocessor
 *
 * Driver for the cc2538 AES/SHA cryptoprocessor
 * @{
 *
 * \file
 * Header file for the cc2538 AES/SHA cryptoprocessor driver
 */
#ifndef CC2538_CRYPTO_H_
#define CC2538_CRYPTO_H_

#include "contiki.h"
#include "lib/aes-128.h"
/*---------------------------------------------------------------------------*/
/** \name Crypto drivers return codes
 * @{
 */
#define CC2538_CRYPTO_SUCCESS                0
#define CC2538_CRYPTO_INVALID_PARAM          1
#define CC2538_CRYPTO_NULL_ERROR             2
#define CC2538_CRYPTO_RESOURCE_IN_USE        3
#define CC2538_CRYPTO_DMA_BUS_ERROR          4
/** @} */
/*---------------------------------------------------------------------------*/
/** \name Crypto functions
 * @{
 */

/** \brief Enables and resets the AES/SHA cryptoprocessor
 */
void cc2538_crypto_init(void);

/** \brief Enables the AES/SHA cryptoprocessor
 */
void cc2538_crypto_enable(void);

/** \brief Disables the AES/SHA cryptoprocessor
 * \note Call this function to save power when the cryptoprocessor is unused.
 */
void cc2538_crypto_disable(void);

/** \brief Registers a process to be notified of the completion of a crypto
 * operation
 * \param p Process to be polled upon IRQ
 * \note This function is only supposed to be called by the crypto drivers.
 */
void cc2538_crypto_register_process_notification(struct process *p);

/** @} */

extern const struct aes_128_driver cc2538_aes_128_driver;

#endif /* CC2538_CRYPTO_H_ */

/**
 * @}
 * @}
 */
