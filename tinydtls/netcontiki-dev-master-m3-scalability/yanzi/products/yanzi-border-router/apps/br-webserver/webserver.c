/*
 * Copyright (c) 2011-2015, Swedish Institute of Computer Science.
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
 *         A simple webserver showing route information
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

#include "httpd-simple.h"
#include "webserver.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include <stdio.h>

#define YLOG_LEVEL YLOG_LEVEL_DEBUG
#define YLOG_NAME  "webserver"
#include "lib/ylog.h"

PROCESS(webserver_nogui_process, "Web server");
/*---------------------------------------------------------------------------*/
static const char *TOP = "<html><head><title>NetContiki RPL</title></head><body>\n";
static const char *BOTTOM = "</body></html>\n";
static char buf[8192];
static int blen;
#define ADD(...) do {                                                   \
    if(sizeof(buf) - 1 > blen) {                                        \
      blen += snprintf(&buf[blen], sizeof(buf) - blen - 1, __VA_ARGS__); \
    }                                                                   \
  } while(0)

#define SEND_BUF(sout) do {                                             \
    if(prepare_buf()) {                                                 \
      SEND_STRING(sout, buf);                                           \
    }                                                                   \
  } while(0)
/*---------------------------------------------------------------------------*/
static int
prepare_buf(void)
{
  static uint8_t has_warned = 0;
  if(blen == 0) {
    return 0;
  }

  if(blen >= sizeof(buf)) {
    blen = sizeof(buf) - 1;
    buf[blen - 3] = '.';
    buf[blen - 2] = '.';
    buf[blen - 1] = '.';

    if(has_warned < 50) {
      has_warned++;
      YLOG_ERROR("Too large web page requested - truncating\n");
    }
  }
  /* Make sure the string is terminated */
  buf[blen] = '\0';

  /* Clear position to prepare for next additions */
  blen = 0;

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
ipaddr_add(const uip_ipaddr_t *addr)
{
  uint16_t a;
  int i, f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0 && sizeof(buf) - blen > 2) {
        buf[blen++] = ':';
        buf[blen++] = ':';
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0 && blen < sizeof(buf) - 1) {
        buf[blen++] = ':';
      }
      ADD("%x", a);
    }
  }
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(generate_routes(struct httpd_state *s))
{
  static int full;
  uip_ds6_route_t *r;
  uip_ds6_nbr_t *nbr;

  PSOCK_BEGIN(&s->sout);

  /* if filename start with /r, just print routes */
  if(s->filename[1] == 'r') {
    full = 0;
  } else {
    full = 1;
  }

  blen = 0;
  if(full) {
    SEND_STRING(&s->sout, TOP);
    ADD("Neighbors<pre>\n");
    for(nbr = nbr_table_head(ds6_neighbors);
        nbr != NULL;
        nbr = nbr_table_next(ds6_neighbors, nbr)) {
      ipaddr_add(&nbr->ipaddr);
      ADD("\n");
    }
    SEND_BUF(&s->sout);
    ADD("</pre>\nRoutes<pre>\n");
  }

  for(r = uip_ds6_route_head();
      r != NULL;
      r = uip_ds6_route_next(r)) {
    ipaddr_add(&r->ipaddr);
    if(full) {
      ADD("/%u (via ", r->length);
      ipaddr_add(uip_ds6_route_nexthop(r));
      if(r->state.lifetime < 6000) {
        ADD(") %lu:%02lus\n", (unsigned long)r->state.lifetime / 60,
	    (unsigned long)r->state.lifetime % 60);
      } else {
        ADD(")\n");
      }
    } else {
      ADD("\n");
    }
  }
  SEND_BUF(&s->sout);

  if(full) {
    ADD("</pre>\n");
    SEND_BUF(&s->sout);

    SEND_STRING(&s->sout, BOTTOM);
  }
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
httpd_simple_script_t
httpd_simple_get_script(const char *name)
{
  return generate_routes;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(webserver_nogui_process, ev, data)
{
  PROCESS_BEGIN();

  httpd_init();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    httpd_appcall(data);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
webserver_init(void)
{
  process_start(&webserver_nogui_process, NULL);
}
/*---------------------------------------------------------------------------*/
