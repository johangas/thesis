/*
 * Copyright (c) 2013-2015, Swedish Institute of Computer Science.
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
 *         Border Router Configuration
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef BR_CONFIG_H_
#define BR_CONFIG_H_

#include "net/ip/uip.h"
#include "net/rpl/rpl.h"

#define BR_MODE_RPL_ROUTER              0
#define BR_MODE_SMART_BRIDGE            1

#define BR_MODE_ATTR_MULTIPLE           1
#define BR_MODE_ATTR_AUTOCONF           2
#define BR_MODE_ATTR_MANUAL_DODAG       4
#define BR_MODE_ATTR_GLOBAL_DODAG       8
#define BR_MODE_ATTR_REWRITE_ADDR_MASK 16

/* Initialisation flags */
extern uint8_t wait_for_address;
extern uint8_t radio_mac_addr_ready;
extern uint8_t ethernet_ready;
extern uint8_t eth_mac_addr_ready;

/* WSN configuration */
extern uip_ipaddr_t wsn_rpl_dag_id;
extern uip_ipaddr_t wsn_net_prefix;
extern uint8_t wsn_net_prefix_len;
extern uint8_t wsn_rpl_instance_id;
extern rpl_dag_t *wsn_rpl_dag;
extern uip_lladdr_t wsn_mac_addr;
extern uip_ipaddr_t wsn_ip_addr;
extern uip_ipaddr_t wsn_ip_local_addr;

/* Ethernet configuration */
typedef uint8_t ethaddr_t[6];

extern ethaddr_t eth_mac_addr;
extern uip_lladdr_t eth_mac64_addr;
extern uip_ipaddr_t eth_ip_addr;
extern uip_ipaddr_t eth_net_prefix;
extern uip_ipaddr_t eth_ip_local_addr;
extern uip_ipaddr_t eth_dft_router;

/* Border router configuration */
extern uint8_t br_config_mode;
extern uint8_t br_config_attrs;

extern int slip_config_verbose;
extern const char *slip_config_ipaddr;
extern int slip_config_flowcontrol;
extern const char *slip_config_siodev;
extern const char *slip_config_host;
extern const char *slip_config_port;
extern const char *slip_config_run_command;
extern const char *ctrl_config_port;
extern const char *server_config_port;
extern char slip_config_tundev[];
extern uint16_t slip_config_basedelay;
extern uint16_t slip_config_unit_controller_port;
extern uint8_t slip_config_is_slave;

/* 6LBR */
extern uint8_t use_raw_ethernet;
extern uint8_t ethernet_has_fcs;
extern const char *slip_config_ifup_script;
extern const char *slip_config_ifdown_script;
extern char const *br_config_www_root;
extern char const *ip_config_file_name;
/* extern char const *node_config_file_name; */
/* extern int slip_config_dtr_rts_set; */

int br_config_handle_arguments(int argc, char **argv);

#endif /* BR_CONFIG_H_ */
