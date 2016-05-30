#!/usr/bin/python

import socket
import struct
import sys

SSDP_PORT = 1900
SSDP_SEARCH_PORT = 1901
SSDP_GRP = '239.255.255.250'
timeout=5.0
show_all = False

if len(sys.argv) > 1:
  if sys.argv[1] == "-a":
    show_all = True
  else:
    print "Usage:", sys.argv[0],"[-a]"
    sys.exit(0)

discover = ("M-SEARCH * HTTP/1.1\r\n"
            "HOST: " + SSDP_GRP + ":" + str(SSDP_PORT) + "\r\n"
            'MAN: "ssdp:discover"\r\n'
            "MX: 5\r\n"
            "ST: ssdp:all\r\n"
            "\r\n")

if __name__ == '__main__':
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
  sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 4)
  sock.bind(('', SSDP_SEARCH_PORT))
  sock.settimeout(timeout)

  sock.sendto(discover, (SSDP_GRP, SSDP_PORT))

  count = 1
  while True:
    try:
      data, addr = sock.recvfrom(1024)
      body = str(data).strip()
      if show_all or 'yanzi-se-product-type:' in body:
        print ""
        print str(count) + ". Received data from", addr
        print "-----"
        print body
        print "-----"
        count += 1
    except IOError:
      break
