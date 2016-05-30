CONTIKI=../../..
CONTIKI_PROJECT=border-router

all: $(CONTIKI_PROJECT)

TARGET ?= native

APPDIRS = apps
APPS += slip-cmd encap oam
APPS += oam/instance-nstats

CURSES_LIBS=

#linker optimizations
SMALL=1

CFLAGS += -Werror
# OSX (g)cc does not like  -Wunused-but-set-variable
MACHINE := $(shell gcc -dumpmachine)
ifeq (darwin, $(findstring darwin, $(MACHINE)))
CFLAGS += -Wunused
else
CFLAGS += -Wunused-but-set-variable
endif
CFLAGS += -Wunused
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

PROJECTDIRS += lib
CONTIKI_SOURCEFILES += border-router-cmds.c tun-bridge.c border-router-rdc.c \
border-router-radio.c br-config.c enc-dev.c border-router-ctrl.c ylog.c \
border-router-server.c dataqueue.c ssdp-responder.c

CFLAGS += -DHAVE_BORDER_ROUTER_CTRL=1
CFLAGS += -DHAVE_BORDER_ROUTER_SERVER=1
CFLAGS += -DHAVE_BORDER_ROUTER_SSDP=1

CONTIKI_SOURCEFILES += udp-cmd.c instance-brm.c instance-rtable.c

ifneq ($(WITH_CLOCK_GETTIME),)
  CFLAGS += -DWITH_CLOCK_GETTIME=$(WITH_CLOCK_GETTIME)
endif

ifeq ($(WITH_SPI),1)
CFLAGS += -DWITH_SPI=1
endif

WITH_WEBSERVER ?= 6lbr
ifeq ($(WITH_WEBSERVER),br)
APPS += br-webserver
endif

ifeq ($(WITH_WEBSERVER),6lbr)
APPS += 6lbr-webserver node-info
endif

ifneq ($(WITH_COAPSERVER),0)
COAP_NO_SENSORS = 1
CFLAGS += -DWITH_COAPSERVER=1
APPS += er-coap
APPS += rest-engine
APPS += yanzi/resources-coap
APPS += yanzi/resources-common
endif

# 6LBR
CONTIKI_SOURCEFILES += packet-filter.c raw-tap-dev.c raw-tap-eth-drv.c sicslow-ethernet.c log-6lbr.c cetic-6lbr.c

CC=$(CROSS_COMPILE)gcc
LD_OVERRIDE=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
ifneq ($(HOST_OS),Darwin)
  LDFLAGS += -pthread
endif
ifeq ($(HOST_OS),Linux)
  LDFLAGS += -lrt
endif

ifdef PORT
  PORTARGS = -s $(PORT)
else ifeq ($(HOST_OS),Darwin)
  PORT = ${firstword ${wildcard /dev/tty.usbserial-*}}
  ifneq ($(PORT),)
    PORTARGS = -s $(PORT)
  endif
endif

ifdef WITH_GDB
  RUN_GDB=gdb -ex run --args
endif

connect-router:	connect-normal

connect-normal: border-router.native
	sudo $(RUN_GDB) ./border-router.native $(PORTARGS) aaaa::1/64

connect-full:	border-router.native
	sudo $(RUN_GDB) ./border-router.native $(PORTARGS) -B 460800 aaaa::1/64

connect-6lbr:	border-router.native
	sudo $(RUN_GDB) ./border-router.native -t tap0 $(PORTARGS) -m smartbridge -B 460800

connect-high:	border-router.native
	sudo $(RUN_GDB) ./border-router.native $(PORTARGS) -B 921600 aaaa::1/64

connect-server:	border-router.native
	sudo $(RUN_GDB) ./border-router.native $(PORTARGS) -B 460800
