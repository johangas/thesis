/*
 * Copyright (c) 2014-2015, Yanzi Networks AB.
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
 *         Border Router Management Instance
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef INSTANCE_BRM_H_
#define INSTANCE_BRM_H_

const char *instance_brm_get_radio_sw_revision(void);
void instance_brm_set_radio_sw_revision(const char *revision);

uint32_t instance_brm_get_radio_bootloader_version(void);
void instance_brm_set_radio_bootloader_version(uint32_t version);

uint64_t instance_brm_get_radio_capabilities(void);
void instance_brm_set_radio_capabilities(uint64_t capabilities);

#define VARIABLE_BRM_STAT_DEBUG_LENGTH 0x200
#define VARIABLE_BRM_STAT_DEBUG_DATA   0x201

#define INSTANCE_BRM_VARIABLES \
    { 0x0090DA0303010022ULL, VARIABLE_BRM_STAT_DEBUG_LENGTH,  4, WRITABILITY_RO, FORMAT_INTEGER,  0 }, \
    { 0x0090DA0303010022ULL, VARIABLE_BRM_STAT_DEBUG_DATA,    4, WRITABILITY_RO, FORMAT_ARRAY  , VECTOR_DEPTH_DONT_CHECK }, \

#endif /* INSTANCE_BRM_H_ */
