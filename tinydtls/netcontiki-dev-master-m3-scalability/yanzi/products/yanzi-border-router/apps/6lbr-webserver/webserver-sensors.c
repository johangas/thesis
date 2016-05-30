/*
 * Copyright (c) 2013, CETIC.
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
 *         6LBR Web Server
 * \author
 *         6LBR Team <6lbr@cetic.be>
 */

#define LOG6LBR_MODULE "WEB"

#include "contiki.h"
#include "httpd.h"
#include "httpd-cgi.h"
#include "webserver-utils.h"

#include "cetic-6lbr.h"
#include "node-info.h"
#include "log-6lbr.h"

static
PT_THREAD(generate_sensors(struct httpd_state *s))
{
  static int i;

  PSOCK_BEGIN(&s->sout);
  add("<br /><h2>Device list</h2>");
  add
    ("<table>"
     "<theader><tr class=\"row_first\"><td>Node</td><td>Type</td><td>Web</td><td>Coap</td><td>Last seen</td><td>Status</td></tr></theader>"
     "<tbody>");
  SEND_STRING(&s->sout, buf);
  reset_buf();

  for(i = 0; i < UIP_DS6_ROUTE_NB; i++) {
    if(node_info_table[i].isused) {
      add("<tr><td>");
#ifdef WITH_WEBSERVER_SENSOR
      add("<a href=\"sensor?ip=");
      ipaddr_add(&node_info_table[i].ipaddr);
      {
        add("\">");
        ipaddr_add(&node_info_table[i].ipaddr);
        add("</a></td>");
      }
#else /* WITH_WEBSERVER_SENSOR */
      ipaddr_add(&node_info_table[i].ipaddr);
      add("</td>");
#endif /* WITH_WEBSERVER_SENSOR */

      if(0) {
      } else if(node_info_table[i].ipaddr.u8[8] == 0x02
         && node_info_table[i].ipaddr.u8[9] == 0x12
         && (node_info_table[i].ipaddr.u8[10] == 0x74 ||
             node_info_table[i].ipaddr.u8[10] == 0x75)) {
        add("<td>Moteiv Telos</td>");
      } else if(node_info_table[i].ipaddr.u8[8] == 0x02
         && node_info_table[i].ipaddr.u8[9] == 0x1A
         && node_info_table[i].ipaddr.u8[10] == 0x4C) {
        add("<td>Crossbow Sky</td>");
      } else if(node_info_table[i].ipaddr.u8[8] == 0xC3
         && node_info_table[i].ipaddr.u8[9] == 0x0C
         && node_info_table[i].ipaddr.u8[10] == 0x00) {
        add("<td>Zolertia Z1</td>");
      } else if(node_info_table[i].ipaddr.u8[8] == 0x02
         && node_info_table[i].ipaddr.u8[9] == 0x80
         && node_info_table[i].ipaddr.u8[10] == 0xE1) {
        add("<td>STMicro</td>");
      } else if(node_info_table[i].ipaddr.u8[8] == 0x02
         && node_info_table[i].ipaddr.u8[9] == 0x12
         && node_info_table[i].ipaddr.u8[10] == 0x4B) {
        add("<td>TI</td>");
      } else if(node_info_table[i].ipaddr.u8[8] == 0x02
                && node_info_table[i].ipaddr.u8[9] == 0x50
                && node_info_table[i].ipaddr.u8[10] == 0xC2
                && node_info_table[i].ipaddr.u8[11] == 0xA8
                && (node_info_table[i].ipaddr.u8[12] & 0XF0) == 0xC0) {
        add("<td>Redwire Econotag I</td>");
      } else if(node_info_table[i].ipaddr.u8[8] == 0x02
                && node_info_table[i].ipaddr.u8[9] == 0x05
                && node_info_table[i].ipaddr.u8[10] == 0x0C
                && node_info_table[i].ipaddr.u8[11] == 0x2A
                && node_info_table[i].ipaddr.u8[12] == 0x8C) {
        add("<td>Redwire Econotag I</td>");
      } else if(node_info_table[i].ipaddr.u8[8] == 0xEE
                && node_info_table[i].ipaddr.u8[9] == 0x47
                && node_info_table[i].ipaddr.u8[10] == 0x3C) {
        if(node_info_table[i].ipaddr.u8[11] == 0x4D
           && node_info_table[i].ipaddr.u8[12] == 0x12) {
          add("<td>Redwire M12</td>");
        } else {
          add("<td>Redwire Unknown</td>");
        }
      } else if((node_info_table[i].ipaddr.u8[8] & 0x02) == 0) {
        add("<td>User defined</td>");
      } else {
        add("<td>Unknown</td>");
      }
      SEND_STRING(&s->sout, buf);
      reset_buf();
      add("<td><a href=\"http://[");
      ipaddr_add(&node_info_table[i].ipaddr);
      add("]/\">web</a></td>");
      add("<td><a href=\"coap://[");
      ipaddr_add(&node_info_table[i].ipaddr);
      add("]:5683/\">coap</a></td>");
      add("<td>%d</td>",
          (clock_time() - node_info_table[i].last_seen) / CLOCK_SECOND);
      add("<td>%s</td>", node_info_table[i].has_route ? "OK" : "NR");
      add("</tr>");
      SEND_STRING(&s->sout, buf);
      reset_buf();
    }
  }
  add("</tbody></table><br />");

#ifdef WITH_WEBSERVER_SENSORS_ACTIONS
  add("<br /><h2>Actions</h2>");
  add("<form action=\"reset-stats-all\" method=\"get\">");
  add("<input type=\"submit\" value=\"Reset all statistics\"/></form><br />");
#endif /* WITH_WEBSERVER_SENSORS_ACTIONS */
  SEND_STRING(&s->sout, buf);
  reset_buf();

  PSOCK_END(&s->sout);
}

#ifdef WITH_WEBSERVER_SENSORS_ACTIONS
static httpd_cgi_call_t *
webserver_sensors_reset_stats_all(struct httpd_state *s)
{
  node_info_reset_statistics_all();
  webserver_result_title = "Reset all statistics";
  webserver_result_text = "Statistics reset";
  return &webserver_result_page;
}
HTTPD_CGI_CMD(webserver_sensors_reset_stats_all_cmd, "reset-stats-all", webserver_sensors_reset_stats_all, 0);
#endif /* WITH_WEBSERVER_SENSORS_ACTIONS */

HTTPD_CGI_CALL(webserver_sensors, "sensors.html", "Sensors", generate_sensors,
	       0);
