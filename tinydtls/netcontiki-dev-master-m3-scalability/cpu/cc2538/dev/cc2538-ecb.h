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
 * \addtogroup cc2538-crypto
 * @{
 *
 * \defgroup cc2538-ccm cc2538 AES-ECB
 *
 * Driver for the cc2538 AES-ECB mode of the security core
 * @{
 *
 * \file
 * Header file for the cc2538 AES-ECB driver
 */
#ifndef ECB_H_
#define ECB_H_

#include "contiki.h"
#include "dev/cc2538-crypto.h"

#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/** \name AES-ECB functions
 * @{
 */

/** \brief Starts the ECB authentication and encryption operation
 * \param len_len Number of octets in length field (2, 4 or 8)
 * \param key_area Area in Key RAM where the key is stored (0 to 7)
 * \param pdata Pointer to message to authenticate and encrypt, or \c NULL
 * \param pdata_len Length of message to authenticate and encrypt in octets, or \c 0
 * \param process Process to be polled upon completion of the operation, or \c NULL
 * \return \c CC2538_AES_SUCCESS if successful, or AES / ECB error code
 */
uint8_t cc2538_ecb_encrypt_start(uint8_t key_area, void *pdata,
                               uint16_t pdata_len, struct process *process);

/** \brief Checks the status of the ECB authentication and encryption operation
 * \retval false Result not yet available, and no error occurred
 * \retval true Result available, or error occurred
 */
uint8_t cc2538_ecb_encrypt_check_status(void);

/** \brief Gets the result of the ECB authentication and encryption operation
 * \return \c CC2538_AES_SUCCESS if successful, or AES / ECB error code
 * \note This function must be called only after \c cc2538_ecb_encrypt_start().
 */
uint8_t cc2538_ecb_encrypt_get_result(void);

/** @} */

#endif /* ECB_H_ */

/**
 * @}
 * @}
 */
