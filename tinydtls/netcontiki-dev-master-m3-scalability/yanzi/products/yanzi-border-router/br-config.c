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
 *         Border Router Configuration
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */
#include "contiki.h"
#include "br-config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <err.h>

#ifdef __APPLE__
#ifndef B230400
#define B230400 230400
#endif
#ifndef B460800
#define B460800 460800
#endif
#ifndef B921600
#define B921600 921600
#endif
#endif /* __APPLE__ */

/* Initialisation flags */
uint8_t wait_for_address = 0;
uint8_t radio_mac_addr_ready = 0;
uint8_t ethernet_ready = 0;
uint8_t eth_mac_addr_ready = 0;

/* WSN configuration */
uint8_t wsn_rpl_instance_id = RPL_DEFAULT_INSTANCE;
rpl_dag_t *wsn_rpl_dag;
uip_ipaddr_t wsn_rpl_dag_id;
uip_ipaddr_t wsn_net_prefix;
uint8_t wsn_net_prefix_len;

uip_lladdr_t wsn_mac_addr;
uip_ipaddr_t wsn_ip_addr;
uip_ipaddr_t wsn_ip_local_addr;

/* Ethernet configuration */
ethaddr_t eth_mac_addr;
uip_lladdr_t eth_mac64_addr;
uip_ipaddr_t eth_ip_addr;
uip_ipaddr_t eth_net_prefix;
uip_ipaddr_t eth_ip_local_addr;
uip_ipaddr_t eth_dft_router;

/* Border router configuration */
uint8_t br_config_mode = BR_MODE_RPL_ROUTER;
uint8_t br_config_attrs = BR_MODE_ATTR_MULTIPLE | BR_MODE_ATTR_GLOBAL_DODAG | BR_MODE_ATTR_AUTOCONF;

int slip_config_verbose = 0;
const char *slip_config_ipaddr;
int slip_config_flowcontrol = 0;
const char *slip_config_siodev = NULL;
const char *slip_config_host = NULL;
const char *slip_config_port = NULL;
const char *slip_config_run_command = NULL;
const char *ctrl_config_port = NULL;
const char *server_config_port = NULL;
char slip_config_tundev[1024] = { "" };
uint16_t slip_config_basedelay = 0;
uint16_t slip_config_unit_controller_port = 4444;
uint8_t slip_config_is_slave = 0;

/* 6LBR */
uint8_t use_raw_ethernet = 0;
uint8_t ethernet_has_fcs = 0;
const char *slip_config_ifup_script = "6lbr/6lbr-ifup";
const char *slip_config_ifdown_script = "6lbr/6lbr-ifdown";
char const *br_config_www_root = "www";
char const *ip_config_file_name = NULL;
/* char const *node_config_file_name = NULL; */
/* int slip_config_dtr_rts_set = 1; */

#ifndef BAUDRATE
#define BAUDRATE B115200
#endif
speed_t slip_config_b_rate = BAUDRATE;

#define GET_OPT_OPTIONS "_?hLB:H::s:m:t:v::d::a:p:SP:C:c:X:" "r::f::U:D:W:I:"
/*---------------------------------------------------------------------------*/
int
br_config_handle_arguments(int argc, char **argv)
{
  const char *prog;
  int c;
  int baudrate = 115200;
  int tmp;

  slip_config_verbose = 0;

  /* Default configuration */
  uip_ip6addr(&wsn_net_prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x0);
  wsn_net_prefix_len = 64;
  uip_ip6addr(&wsn_ip_addr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x100);
  uip_ip6addr(&eth_net_prefix, 0xbbbb, 0, 0, 0, 0, 0, 0, 0x0);
  uip_ip6addr(&eth_ip_addr, 0xbbbb, 0, 0, 0, 0, 0, 0, 0x100);

  prog = argv[0];
  while((c = getopt(argc, argv, GET_OPT_OPTIONS)) != -1) {
    switch(c) {
    case '_':
      fprintf(stderr, "GETOPT:%s\n", GET_OPT_OPTIONS);
      exit(EXIT_SUCCESS);
      break;
    case 'B':
      baudrate = atoi(optarg);
      break;

    case 'H':
      if(optarg != NULL) {
        slip_config_flowcontrol = *optarg == '1';
      } else {
        slip_config_flowcontrol = 1;
      }
      break;

    case 's':
      if(strncmp("/dev/", optarg, 5) == 0) {
	slip_config_siodev = optarg + 5;
      } else {
	slip_config_siodev = optarg;
      }
      break;

    case 't':
      if(strncmp("/dev/", optarg, 5) == 0) {
	strncpy(slip_config_tundev, optarg + 5, sizeof(slip_config_tundev) - 1);
      } else {
	strncpy(slip_config_tundev, optarg, sizeof(slip_config_tundev) - 1);
      }
      break;

    case 'a':
      slip_config_host = optarg;
      break;

    case 'm':
      /* check if we target smart-bridge mode */
      if(strncmp("smartbridge", optarg, 11) == 0) {
	br_config_mode = BR_MODE_SMART_BRIDGE;
	printf("*** SmartBridge Mode ****\n");
      } else {
        fprintf(stderr, "Unsupported router mode: %s\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case 'p':
      slip_config_port = optarg;
      break;

    case 'C':
      tmp = atoi(optarg);
      if(tmp <= 0 || tmp > 0xffff) {
        fprintf(stderr, "illegal unit controller port: %s\n", optarg);
        exit(EXIT_FAILURE);
      } else {
        slip_config_unit_controller_port = tmp;
      }
      break;

    case 'c':
      ctrl_config_port = optarg;
      break;

    case 'P':
      server_config_port = optarg;
      break;

    case 'S':
      /* Start as slave */
      slip_config_is_slave = 1;
      break;

    case 'd':
      slip_config_basedelay = 10;
      if(optarg) slip_config_basedelay = atoi(optarg);
      break;

    case 'v':
      slip_config_verbose = 2;
      if(optarg) slip_config_verbose = atoi(optarg);
      break;

    case 'X':
      slip_config_run_command = optarg;
      break;

    case 'r':
      if(optarg != NULL) {
        use_raw_ethernet = *optarg == '1';
      } else {
        use_raw_ethernet = 1;
      }
      break;

    case 'f':
      if(optarg != NULL) {
        ethernet_has_fcs = *optarg == '1';
      } else {
        ethernet_has_fcs = 1;
      }
      break;

    case 'U':
      slip_config_ifup_script = optarg;
      break;

    case 'D':
      slip_config_ifdown_script = optarg;
      break;

    case 'W':
      br_config_www_root = optarg;
      break;

    case 'I':
      ip_config_file_name = optarg;
      break;

    /* case 'n': */
    /*   node_config_file_name = optarg; */
    /*   break; */

    /* case 'y': */
    /*   if(optarg != NULL) { */
    /*     slip_config_dtr_rts_set = *optarg == '1'; */
    /*   } else { */
    /*     slip_config_dtr_rts_set = 1; */
    /*   } */
    /*   break; */

    case 'L':
      fprintf(stderr, "Warning, the 'L' option has been removed (log output have time as default)\n");
      break;
    case '?':
    case 'h':
    default:
fprintf(stderr,"usage:  %s [options] [ipaddress]\n", prog);
fprintf(stderr,"example: border-router.native -v2 -s ttyUSB1 aaaa::1/64\n");
fprintf(stderr,"Options are:\n");
fprintf(stderr," -B baudrate    9600,19200,38400,57600,115200,230400,460800,921600 (default 115200)\n");
fprintf(stderr," -H             Hardware CTS/RTS flow control (default disabled)\n");
fprintf(stderr," -s siodev      Serial device (default /dev/ttyUSB0)\n");
fprintf(stderr," -S             Run in slave mode.\n");
fprintf(stderr," -a host        Connect via TCP to server at <host>\n");
fprintf(stderr," -p port        Connect via TCP to server at <host>:<port>\n");
fprintf(stderr," -c port        Open UDP control at localhost:<port>\n");
fprintf(stderr," -P port        Open TCP control at localhost:<port>\n");
fprintf(stderr," -t tundev      Name of interface (default tun0)\n");
fprintf(stderr," -X cmd         Run the command and then exit\n");
fprintf(stderr," -m smartbridge Run in smart bridge mode\n");
fprintf(stderr," -v[level]      Verbosity level\n");
fprintf(stderr,"    -v0         No messages\n");
fprintf(stderr,"    -v1         Encapsulated SLIP debug messages (default)\n");
fprintf(stderr,"    -v2         Printable strings after they are received\n");
fprintf(stderr,"    -v3         Printable strings and SLIP packet notifications\n");
fprintf(stderr,"    -v4         All printable characters as they are received\n");
fprintf(stderr,"    -v5         All SLIP packets in hex\n");
fprintf(stderr,"    -v          Equivalent to -v3\n");
fprintf(stderr," -d[basedelay]  Minimum delay between outgoing SLIP packets.\n");
fprintf(stderr,"                Actual delay is basedelay*(#6LowPAN fragments) milliseconds.\n");
fprintf(stderr,"                -d is equivalent to -d10.\n");
exit(EXIT_FAILURE);
      break;
    }
  }
  argc -= optind;
  argv += optind;

  if(br_config_mode == BR_MODE_SMART_BRIDGE) {
    /* No need to wait for address in smart bridge mode */
    wait_for_address = 0;
  } else if(argc == 0) {
    /* If no address is supplied, wait for it to be set via OAM */
    wait_for_address = 1;
  } else if(argc == 1) {
    wait_for_address = 0;
    slip_config_ipaddr = argv[0];
  } else {
    fprintf(stderr, "*** too many arguments\n");
    err(1, "usage: %s [-B baudrate] [-H] [-S] [-s siodev] [-t tundev] [-v verbosity] [-d delay] [-a serveraddress] [-p serverport] [ipaddress]", prog);
  }

  switch(baudrate) {
  case -2:
    break;			/* Use default. */
  case 9600:
    slip_config_b_rate = B9600;
    break;
  case 19200:
    slip_config_b_rate = B19200;
    break;
  case 38400:
    slip_config_b_rate = B38400;
    break;
  case 57600:
    slip_config_b_rate = B57600;
    break;
  case 115200:
    slip_config_b_rate = B115200;
    break;
#ifdef B230400
  case 230400:
    slip_config_b_rate = B230400;
    break;
#endif
#ifdef B460800
  case 460800:
    slip_config_b_rate = B460800;
    break;
#endif
#ifdef B921600
  case 921600:
    slip_config_b_rate = B921600;
    break;
#endif
  default:
    err(1, "unknown baudrate %d (all baudrates are not supported on all systems)", baudrate);
    break;
  }

  if(*slip_config_tundev == '\0') {
    /* Use default. */
    strcpy(slip_config_tundev, "tun0");
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
