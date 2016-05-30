/*
 * Copyright (c) 2013 Joakim Eriksson, Niclas Finne
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef UDP_CMD_H_
#define UDP_CMD_H_

/* #include <sys/types.h> */
/* #include <sys/socket.h> */
#include <netinet/in.h>

#define PDU_MAX_LEN 1280

typedef struct {
  uint8_t rxbuf[PDU_MAX_LEN];
  uint8_t txbuf[PDU_MAX_LEN];
  int txlen;
  int payload_len;
  int fd;
  struct sockaddr_in sin;
} tlv_in_progress;

void udp_cmd_init(void);
void udp_cmd_start(void);
void send_poll_response(void);
void get_radio_info();
void process_tlv_from_radio(uint8_t *stack, int stacklen);
void handle_tlv_request(tlv_in_progress *tip);

#endif /* UDP_CMD_H_ */
