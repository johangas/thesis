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
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         RPL control API
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef RPL_CONTROL_H_
#define RPL_CONTROL_H_

#include "contiki-conf.h"

typedef uint8_t rpl_control_key_t;

#define RPL_CONTROL_KEY_NONE 0

/**
 * Allocate a new RPL control key.
 *
 * \retval A RPL control key or RPL_CONTROL_KEY_NONE if no more keys
 * can be allocated.
 */
rpl_control_key_t rpl_control_alloc_key(void);

/**
 * Get the DIO suppression state
 *
 * \retval The DIO suppression state
 */
int rpl_control_is_dio_suppressed(void);

/**
 * Set the DIO suppression state for the specified RPL control
 * key. All DIOs are suppressed as long as suppress state for at least
 * one RPL control key has been set.
 *
 * \param key The RPL control key to use to suppress DIO
 * \param state Non-zero if DIO should be suppressed
 */
void rpl_control_set_suppress_dio(rpl_control_key_t key, int state);

#endif /* RPL_CONTROL_H_ */
