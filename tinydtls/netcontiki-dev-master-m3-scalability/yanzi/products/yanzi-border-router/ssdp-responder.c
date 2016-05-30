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

#include "contiki.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include "border-router.h"

#define YLOG_LEVEL YLOG_LEVEL_INFO
#define YLOG_NAME  "ssdp"
#include "lib/ylog.h"

#ifndef UPNP_MCAST_GROUP
#define UPNP_MCAST_GROUP "239.255.255.250"
#endif /* UPNP_MCAST_GROUP */

#ifndef UPNP_PORT
#define UPNP_PORT 1900
#endif /* UPNP_PORT */

#ifndef UPNP_ANNOUNCE_PERIOD
#define UPNP_ANNOUNCE_PERIOD 0
#endif /* UPNP_ANNOUNCE_PERIOD */

#ifndef WITH_UPNP_ANNOUNCE
#define WITH_UPNP_ANNOUNCE 0
#endif /* WITH_UPNP_ANNOUNCE */

#define PDU_MAX_LEN 1280

uint8_t *getDid(void);

static int ssdp_fd = -1;
static struct sockaddr_in server;
static struct sockaddr_in client;
static unsigned char buf[PDU_MAX_LEN];

#if WITH_UPNP_ANNOUNCE && UPNP_ANNOUNCE_PERIOD > 0
static struct ctimer periodic_timer;
#endif /* WITH_UPNP_ANNOUNCE && UPNP_ANNOUNCE_PERIOD > 0 */

static char search_response[] =
  "HTTP/1.1 200 OK\r\n"
  "CACHE-CONTROL: max-age = 1800\r\n"
  "EXT:\r\n"
  "SERVER: Fiona UPnP/1.0 Yanzi UPnP/1.0\r\n"
  "ST: upnp:rootdevice\r\n"
  "LOCATION: http://www.identifyobject.net/location/0090DA0301020010/desc.xml\r\n"
  "USN: uuid:EUI64-BBBBBBBBBBBBBBBB::upnp:rootdevice\r\n"
  "yanzi-se-product-type: 0090DA0301020010\r\n"
  "yanzi-se-available: @\r\n"
  "se-yanzi-service-address: tcp:PPPPP\r\n";
static const char *hex = "0123456789ABCDEF";
static char *available_indicator;
/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset)
{
  if(ssdp_fd > 0) {
    FD_SET(ssdp_fd, rset);	/* Read from udp ASAP! */
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
handle_fd(fd_set *rset, fd_set *wset)
{
  int l;
  unsigned int a = sizeof(client);

  if(ssdp_fd > 0 && FD_ISSET(ssdp_fd, rset)) {
    l = recvfrom(ssdp_fd, buf, sizeof(buf), 0, (struct sockaddr *)&client, &a);
    if(l < 0) {
      if(errno == EAGAIN) {
        return;
      }
      YLOG_ERROR("failed to receive UPnP\n");
      err(1, "ssdp: recv");
      return;
    }

    if(l == 0) {
      /* Nothing received */
      YLOG_ERROR("server socket shutdown?\n");
      return;
    }

    /* Only respond to search */
    if(strncmp("M-SEARCH", (char *)buf, 8) == 0) {
      YLOG_DEBUG("responding to search from %s:%d\n",
                 inet_ntoa(client.sin_addr),
                 ntohs(client.sin_port));
      *available_indicator = border_router_is_slave_active() ? '0' : '1';
      sendto(ssdp_fd, search_response, sizeof(search_response), 0, (struct sockaddr *)&client, a);
    } else {
      YLOG_DEBUG("not a search, not responding to %s:%d\n",
                 inet_ntoa(client.sin_addr),
                 ntohs(client.sin_port));
    }

#if 0
    /* Dump full packet as debug output */
    buf[l - 1] = '\0';
    YLOG_DEBUG("UPnP Data:\n----------\n%s\n----------\n", buf);
#endif /* 0 */
  }
}
/*---------------------------------------------------------------------------*/
static const struct select_callback udp_callback = { set_fd, handle_fd };
/*---------------------------------------------------------------------------*/
#if WITH_UPNP_ANNOUNCE
int
ssdp_responder_announce(void)
{
  struct ifaddrs *ifaddr, *ifa;
  struct in_addr interface;
  struct sockaddr_in mcast_group;
  char host[NI_MAXHOST];
  int optval, fd;

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(fd < 0) {
    YLOG_ERROR("failed to open mcast socket\n");
    return -1;
  }

  optval = 1;
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    YLOG_ERROR("failed to set SO_REUSEADDR on mcast socket\n");
    close(fd);
    return -1;
  }

  optval = 5;
  setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &optval, sizeof(optval));

  memset(&mcast_group, 0, sizeof(mcast_group));
  mcast_group.sin_family = AF_INET;
  mcast_group.sin_addr.s_addr = inet_addr(UPNP_MCAST_GROUP);
  mcast_group.sin_port = htons(UPNP_PORT);

  memset(&interface, 0, sizeof(interface));

  if(getifaddrs(&ifaddr) == -1) {
    YLOG_ERROR("failed to list interfaces\n");
    close(fd);
    return -1;
  }

  YLOG_DEBUG("------------\n");

  for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if(ifa->ifa_addr == NULL) {
      continue;
    }

    if(ifa->ifa_addr->sa_family != AF_INET) {
      continue;
    }

    if(getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) != 0) {
      YLOG_INFO("failed to get name info for interface %s\n", ifa->ifa_name);
      continue;
    }

    YLOG_DEBUG("Announcing UPnP to interface <%s>, <%s>\n",
              ifa->ifa_name, host);

    interface.s_addr = inet_addr(host);
    if(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&interface, sizeof(interface)) < 0) {
      YLOG_ERROR("failed to configure interface %s\n", ifa->ifa_name);
      continue;
    }

    if(sendto(fd, discover_msg, sizeof(discover_msg), 0, (struct sockaddr*)&mcast_group, sizeof(mcast_group)) < 0) {
      YLOG_ERROR("failed to announce on interface %s\n", ifa->ifa_name);
    }
  }
  YLOG_DEBUG("------------\n");

  freeifaddrs(ifaddr);

  close(fd);

  return 0;
}
#endif /* WITH_UPNP_ANNOUNCE */
/*---------------------------------------------------------------------------*/
#if WITH_UPNP_ANNOUNCE && UPNP_ANNOUNCE_PERIOD > 0
static void
periodic_task(void *ptr)
{
  ctimer_restart(&periodic_timer);
  ssdp_responder_announce();
}
#endif /* WITH_UPNP_ANNOUNCE && UPNP_ANNOUNCE_PERIOD > 0 */
/*---------------------------------------------------------------------------*/
void
ssdp_responder_init(uint16_t port)
{
  struct ifaddrs *ifaddr, *ifa;
  struct ip_mreq upnp_group;
  char host[NI_MAXHOST];
  char *B;
  char *P;
  int i, optval, fd;

  if(ssdp_fd > 0) {
    /* Already initialized */
    return;
  }

  available_indicator = strstr(search_response, "@");
  B = strstr(search_response, "BBBBBBBBBBBBBBBB");
  P = strstr(search_response, "PPPPP");
  if(B == NULL || available_indicator == NULL) {
    YLOG_ERROR("Malformed search_response message, ssdp responder exiting");
    return;
  }

  for(i = 0; i < 8; i++) {
    B[i *2] = hex[(getDid()[8+i] >> 4) & 0xf];
    B[(i * 2) + 1] = hex[getDid()[8+i] & 0xf];
  }
  *available_indicator = '1';

  if (P != NULL) {
    sprintf(P, "%-5u", port);
  }

  YLOG_DEBUG("ssdp setup:\n----\n%s\n----\n", search_response);

  /* create a UDP socket */
  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(fd == -1) {
    YLOG_ERROR("failed to create socket, ssd responder exiting\n");
    return;
  }

  optval = 1;
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    YLOG_ERROR("failed to set SO_REUSEADDR\n");
    close(fd);
    return;
  }

  optval = 5;
  setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &optval, sizeof(optval));

  memset((void *) &server, 0, sizeof(server));

  server.sin_family = AF_INET;
  server.sin_port = htons(UPNP_PORT);
  server.sin_addr.s_addr = INADDR_ANY;

  /* bind socket to port */
  if(bind(fd, (struct sockaddr *)&server, sizeof(server) ) == -1) {
    YLOG_ERROR("Error binding UPnP port\n");
    close(fd);
    return;
  }

  upnp_group.imr_multiaddr.s_addr = inet_addr(UPNP_MCAST_GROUP);

  if(getifaddrs(&ifaddr) == -1) {
    YLOG_ERROR("failed to list interfaces\n");
    close(fd);
    return;
  }

  YLOG_DEBUG("------------\n");
  for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if(ifa->ifa_addr == NULL) {
      continue;
    }

    if(ifa->ifa_addr->sa_family != AF_INET) {
      continue;
    }

    if(getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) != 0) {
      YLOG_INFO("failed to get name info for interface\n");
      continue;
    }

    YLOG_INFO("Joining UPnP mcast group at interface <%s>, <%s>\n",
              ifa->ifa_name, host);

    upnp_group.imr_interface.s_addr = inet_addr(host);
    if(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                  (char *)&upnp_group, sizeof(upnp_group)) < 0) {
      YLOG_ERROR("failed to join UPnP mcast group\n");
      close(fd);
      freeifaddrs(ifaddr);
      return;
    }
  }
  YLOG_DEBUG("------------\n");

  freeifaddrs(ifaddr);

  ssdp_fd = fd;

  YLOG_INFO("Started SSDP on port %d\n", UPNP_PORT);
  select_set_callback(ssdp_fd, &udp_callback);


#if WITH_UPNP_ANNOUNCE && UPNP_ANNOUNCE_PERIOD > 0
  ctimer_set(&periodic_timer, UPNP_ANNOUNCE_PERIOD * CLOCK_SECOND,
             periodic_task, NULL);
#endif /* WITH_UPNP_ANNOUNCE && UPNP_ANNOUNCE_PERIOD > 0 */
}
/*---------------------------------------------------------------------------*/
