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

#include "contiki.h"
#include "vargen.h"
#include "api.h"
#include "unitcontroller.h"
#include "oam.h"

#if !CAN_DO_UC_IPV4 && !CAN_DO_UC_IPV6
#error Unit controller need at least one of CAN_DO_UC_IPV6 or CAN_DO_UC_IPV4
#endif /* !CAN_DO_UC_IPV4 && !CAN_DO_UC_IPV6 */

static uint32_t uc_wd_lastSet = 0;
static uint32_t uc_wd;
static uint8_t uc_change_count = 0;
static uint16_t uc_port;
static uint8_t uc_address[16];
static uint8_t uc_address_type;

/*
 * Unit controller implementation.
 *
 * Rules:
 *
 * 1) Writing UC-Address sets mode to "Owned", increments revision
 *    field in UCStatus and watchdog initialized to n seconds. 100 < n
 *    < 1000. This implementation sets 313.
 *
 * 2) UC-Address can only be written when watchdog is zero.
 *
 * 3) When watchdog expires (goes from non-zero to zero), revision
 *    field in UC-Status is incremented and UC-Address, is set to
 *    zero.
 *
 * 4) Watchdog can only be written when it is non-zero.
 *
 * 5) When a value larger than max supported watchdog value is
 *    attempted to be written, watchdog should be set to the maximum
 *    supported value.
 *
 * Format of unit controller address:
 *
 *   3             2 2             1 1
 *   1             4 3             6 5             8 7             0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        type = 1 (IPv6)                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             port                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                               |
 *  |                          IPv6 Address                         |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                      Location identifier                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                     pad with 4 byte zero                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *   3             2               1
 *   1             4               6               8               0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        type = 2 (IPv4)                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             port                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                          IPv4 Address                         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                      Location identifier                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                               |
 *  |                    pad with 16 byte zero                      |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 */
/*----------------------------------------------------------------*/
/*
 * Set watchdog from a byte array representing an unsigned 32 bit
 * integer, in network byte order, of a local uint32_t.
 */
uint8_t
unitControllerSetWatchdog(uint8_t *p, uint32_t wd)
{
  uint32_t tmp;
  if(unitControllerGetWatchdog(NULL) == 0) {
    return TLV_ERROR_WRITE_ACCESS_DENIED;
  }

  if(p != NULL) {
    tmp = p[0] << 24;
    tmp |= p[1] << 16;
    tmp |= p[2] << 8;
    tmp |= p[3];
  } else {
    tmp = wd;
  }

  /* Emulate 22 bit counter, so we can have 32 bit milliseconds */
  if(tmp > 0x3fffffL) {
    uc_wd = 0xf9fffc18L; /* 0x3fffff seconds in milliseconds */
  } else {
    uc_wd = tmp * 1000;
  }
  uc_wd_lastSet = boottimer_read();
  return TLV_ERROR_NO_ERROR;
}
/*----------------------------------------------------------------*/
/*
 * Return watchdog and if "p" is non-NULL, write watchdog value in
 * network byte order to "p".
 */
uint32_t
unitControllerGetWatchdog(uint8_t *p)
{
  uint32_t elapsed;
  uint32_t retval;
  if(uc_wd == 0) {
    retval = 0;
  } else {
    elapsed = boottimer_elapsed(uc_wd_lastSet);
    if(elapsed < uc_wd) {
      retval = (uc_wd - elapsed) / 1000;
    } else {
      retval = 0;
    }

    if(retval == 0) {
      uc_wd = 0; /* protect from strangeness when time wraps. */
      uc_change_count++;
      watchdog_expired();
    }
  }
  if(p != NULL) {
    p[0] = (retval >> 24 & 0xff);
    p[1] = (retval >> 16 & 0xff);
    p[2] = (retval >> 8 & 0xff);
    p[3] = (retval & 0xff);
  }
  return retval;
}
/*----------------------------------------------------------------*/
/*
 * Set unit controller address.
 * Assumes "p" points to array of 32 bytes.
 *
 * Returns TLV_ERROR_WRITE_ACCESS_DENIED if watchdog is non-zero or type is unsupported.
 *
 */
uint8_t
unitControllerSetAddress(uint8_t *p, uint8_t justAddress)
{
  uint32_t old_lid;
  uint32_t new_lid;

  if(unitControllerGetWatchdog(NULL) != 0) {
    return TLV_ERROR_WRITE_ACCESS_DENIED;
  }
  old_lid = get_location_id();

  if(p[3] == UNIT_CONTROLLER_ADDRESS_TYPES_IPV4) {
    new_lid = get_int32_from_data(&p[12]);
  } else if(p[3] == UNIT_CONTROLLER_ADDRESS_TYPES_IPV6) {
    new_lid = get_int32_from_data(&p[24]);
  } else {
    new_lid = 0;
  }

  /*
   * Can not use unit controller address to reset location ID, so all
   * 1:s is equal to zero
   */
  if(new_lid == 0xffffffff) {
    new_lid = 0;
  }

  if(!justAddress) {
    /*
     * If trying to set a location id, and there is one configured,
     * and they do not match, it is a fail.
     */
    if((new_lid != 0) && (old_lid != 0) && (old_lid != new_lid)) {
      return TLV_ERROR_INVALID_ARGUMENT;
    }
  }

  if(p[3] == UNIT_CONTROLLER_ADDRESS_TYPES_IPV4) {
#if !CAN_DO_UC_IPV4
    return TLV_ERROR_INVALID_ARGUMENT;
#endif /* CAN_DO_UC_IPV4 */
    /* sanity checks */
    if((p[8] == 0x00) && (p[9] == 0x00) && (p[10] == 0x00) && (p[11] == 0x00)) {
      /* 0.0.0.0 */
      return TLV_ERROR_INVALID_ARGUMENT;
    }
    if((p[8] == 0xff) && (p[9] == 0xff) && (p[10] == 0xff) && (p[11] == 0xff)) {
      /* 255.255.255.255 */
      return TLV_ERROR_INVALID_ARGUMENT;
    }

    /* Ok, address seem ok, let's do it */
    uc_port = (p[6] << 8) | p[7];
    memcpy(uc_address, &p[8], 4);
    uc_address_type = p[3];
  } else if(p[3] == UNIT_CONTROLLER_ADDRESS_TYPES_IPV6) {
#if !CAN_DO_UC_IPV6
    return TLV_ERROR_INVALID_ARGUMENT;
#endif /* CAN_DO_UC_IPV6 */
    uc_port = (p[6] << 8) | p[7];
    memcpy(uc_address, &p[8], 16);
    uc_address_type = p[3];
  } else {
    return TLV_ERROR_INVALID_ARGUMENT;
  }
  if(justAddress) {
    return TLV_ERROR_NO_ERROR;
  }

  set_location_id(new_lid);
  uc_change_count++;
  uc_wd = 31000L;
  uc_wd_lastSet = boottimer_read();
  return TLV_ERROR_NO_ERROR;
}
/*----------------------------------------------------------------*/
uint8_t *
unitControllerGetAddress()
{
  static uint8_t reply[32];
  uint32_t lid;
  uint8_t lid_offset = 0;

  memset(reply, 0, sizeof(reply));

  if(unitControllerGetWatchdog(NULL) != 0) {
    reply[3] = uc_address_type;
    reply[6] = (uc_port >> 8) & 0xff;
    reply[7] = uc_port & 0xff;

    if(uc_address_type == UNIT_CONTROLLER_ADDRESS_TYPES_IPV6) {
      memcpy(&reply[8], uc_address, 16);
      lid_offset = 24;
    } else if(uc_address_type == UNIT_CONTROLLER_ADDRESS_TYPES_IPV4) {
      memcpy(&reply[8], uc_address, 4);
      lid_offset = 12;
    }
    if(lid_offset) {
      lid = get_location_id();
      reply[lid_offset++] = (lid >> 24) & 0xff;
      reply[lid_offset++] = (lid >> 16) & 0xff;
      reply[lid_offset++] = (lid >> 8) & 0xff;
      reply[lid_offset++] = lid & 0xff;
    }
  }

  return reply;
}
/*----------------------------------------------------------------*/
/*
 * Write 32 bit status into array pointed to by "p", if p is non-NULL.
 */
void
unitControllerGetStatus(uint8_t *p)
{
  /* read watchdog to update uc_change_count if needed. */
  (void)unitControllerGetWatchdog(NULL);
  p[0] = 0;
  p[1] = 0;
  p[2] = 0;
  p[3] = uc_change_count;
}
/*----------------------------------------------------------------*/
/*
 * Write 32 bit communication style into array pointed to by "p", if p is non-NULL.
 */
void
getCommunicationStyle(uint8_t *p)
{
  if(p) {
    p[0] = 0;
    p[1] = UNIT_CONTROLLER_IMPLEMENTED_WATCHDOG_BITS;
    p[2] = UNIT_CONTROLLER_IMPLEMENTED_CRYPTO_METHODS;
    p[3] = UNIT_CONTROLLER_IMPLEMENTED_ADDRESS_TYPES;
  }
}
/*----------------------------------------------------------------*/
/*
 * Compare supplied address to unit controller address and eturn TRUE
 * if unit controller address is valid and they are the same.
 */
uint8_t
is_uc_address(uint8_t *a, size_t address_len)
{
  /* no address is the same as an expired unit controlelr */
  if(unitControllerGetWatchdog(NULL) == 0) {
    return FALSE;
  }

  if(uc_address_type == UNIT_CONTROLLER_ADDRESS_TYPES_IPV6) {
    if(address_len != 16) {
      return FALSE;
    }
    if(memcmp(a, uc_address, 16) == 0) {
      return TRUE;
    }
  } else if(uc_address_type == UNIT_CONTROLLER_ADDRESS_TYPES_IPV4) {
    if(address_len != 4) {
      return FALSE;
    }
    if(memcmp(a, uc_address, 4) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}
/*----------------------------------------------------------------*/
