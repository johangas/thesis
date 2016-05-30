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
#include "webserver.h"

#include "cetic-6lbr.h"
#include "log-6lbr.h"

HTTPD_CGI_CALL_NAME(webserver_main)
HTTPD_CGI_CALL_NAME(webserver_network)
HTTPD_CGI_CALL_NAME(webserver_network_routes)
HTTPD_CGI_CMD_NAME(webserver_network_route_rm_cmd)
HTTPD_CGI_CMD_NAME(webserver_network_nbr_rm_cmd)
HTTPD_CGI_CALL_NAME(webserver_rpl)
HTTPD_CGI_CMD_NAME(webserver_rpl_gr_cmd)
HTTPD_CGI_CMD_NAME(webserver_rpl_reset_cmd)
HTTPD_CGI_CMD_NAME(webserver_rpl_child_cmd)
#if CETIC_NODE_INFO
HTTPD_CGI_CALL_NAME(webserver_sensors)
#ifdef WITH_WEBSERVER_SENSORS_ACTIONS
HTTPD_CGI_CMD_NAME(webserver_sensors_reset_stats_all_cmd)
#endif /* WITH_WEBSERVER_SENSORS_ACTIONS */
#ifdef WITH_WEBSERVER_SENSOR
HTTPD_CGI_CALL_NAME(webserver_sensor)
HTTPD_CGI_CMD_NAME(webserver_sensor_reset_stats_cmd)
HTTPD_CGI_CMD_NAME(webserver_sensor_delete_node_cmd)
#endif /* WITH_WEBSERVER_SENSOR */
#endif /* CETIC_NODE_INFO */
HTTPD_CGI_CALL_NAME(webserver_statistics)
#if WITH_WEBSERVER_ADMIN
HTTPD_CGI_CALL_NAME(webserver_admin)
HTTPD_CGI_CMD_NAME(webserver_admin_restart_cmd)
#if CONTIKI_TARGET_NATIVE
#if !CETIC_6LBR_ONE_ITF
HTTPD_CGI_CMD_NAME(webserver_admin_reset_slip_radio_cmd);
#endif
HTTPD_CGI_CMD_NAME(webserver_admin_reboot_cmd)
HTTPD_CGI_CMD_NAME(webserver_admin_halt_cmd)
#endif /* WITH_WEBSERVER_ADMIN */
HTTPD_CGI_CALL_NAME(webserver_log_send_log)
HTTPD_CGI_CALL_NAME(webserver_log_send_err)
HTTPD_CGI_CMD_NAME(webserver_log_clear_log_cmd)
#endif

void
webserver_init(void)
{
  httpd_init();

  httpd_cgi_add(&webserver_main);
#if CETIC_NODE_INFO
  httpd_cgi_add(&webserver_sensors);
#ifdef WITH_WEBSERVER_SENSORS_ACTIONS
  httpd_cgi_command_add(&webserver_sensors_reset_stats_all_cmd);
#endif /* WITH_WEBSERVER_SENSORS_ACTIONS */
#ifdef WITH_WEBSERVER_SENSOR
  httpd_cgi_add(&webserver_sensor);
  httpd_cgi_command_add(&webserver_sensor_reset_stats_cmd);
  httpd_cgi_command_add(&webserver_sensor_delete_node_cmd);
#endif /* WITH_WEBSERVER_SENSOR */
#endif /* CETIC_NODE_INFO */
#if UIP_CONF_IPV6_RPL
  httpd_cgi_add(&webserver_rpl);
#endif
  httpd_cgi_add(&webserver_network);
  httpd_cgi_add(&webserver_network_routes);
  httpd_cgi_add(&webserver_statistics);
}
