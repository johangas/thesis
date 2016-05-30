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
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define USB_SERIAL_CONF_ENABLE 1
#define DBG_CONF_USB 1

#define STORE_LOCATIONID_LOCALLY 1

#define PRODUCT_TYPE_INT64 0x0090DA0301010501ULL
#define PRODUCT_LABEL "IoT-U10"
#define INSTANCES 7                     /* Including instance 0 */
#define PRIMARY_FLASH_INSTANCE 1
#define BACKUP_FLASH_INSTANCE 2
#define INSTANCE_STTS751 3
#define INSTANCE_LEDS 4
#define INSTANCE_BUTTON 5
#define INSTANCE_NSTATS 6

#define OAM_INIT_DECLARATIONS                           \
  void stts751_init(instanceControl IC[]);              \
  void instance_leds_init(instanceControl IC[]);        \
  void instance_button_init(instanceControl IC[]);      \
  void instance_nstats_init(instanceControl IC[]);

#define OAM_INIT_FUNCS stts751_init,          \
    instance_leds_init, instance_button_init, \
    instance_nstats_init

/* Network statistics */
#define RPL_CONF_STATS 1

/* CoAP */
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE 256

#define WEBSERVER_CONF_CFS_PATHLEN 24

#define WITH_LLSEC 0

#if WITH_LLSEC

/* Link layer encryption configuration */
#undef NETSTACK_CONF_LLSEC
#define NETSTACK_CONF_LLSEC               noncoresec_driver

/* LEVEL 6 = Encryption and 64 bits MIC */
#undef LLSEC802154_CONF_SECURITY_LEVEL
#define LLSEC802154_CONF_SECURITY_LEVEL   6

#define NONCORESEC_CONF_KEY { 0x00 , 0x01 , 0x02 , 0x03 , \
                              0x04 , 0x05 , 0x06 , 0x07 , \
                              0x08 , 0x09 , 0x0A , 0x0B , \
                              0x0C , 0x0D , 0x0E , 0x0F }

/* Software based AES-128 encryption */
#undef AES_128_CONF
#define AES_128_CONF aes_128_driver

#endif /* WITH_LLSEC */

#endif /* PROJECT_CONF_H_ */
