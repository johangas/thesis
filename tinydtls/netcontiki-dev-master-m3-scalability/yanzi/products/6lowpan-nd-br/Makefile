CONTIKI=../../..
CONTIKI_PROJECT=border-router

all: $(CONTIKI_PROJECT)

TARGET ?= native

APPDIRS = ../yanzi-border-router/apps
APPS += slip-cmd encap oam
APPS += oam/instance-nstats

CURSES_LIBS=

CFLAGS += -DUIP_CONF_ND6_RA_PERIODIC=0 -DTCPIP_CONF_ANNOTATE_TRANSMISSIONS=1 
CONF_6LOWPAN_ND=1
UIP_CONF_6LBR=1

#linker optimizations
SMALL=1

#CFLAGS += -Werror
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

PROJECTDIRS += ../yanzi-border-router/
PROJECTDIRS += ../yanzi-border-router/lib

CFLAGS += -DHAVE_BORDER_ROUTER_CTRL=1
CFLAGS += -DHAVE_BORDER_ROUTER_SERVER=1
CFLAGS += -DHAVE_BORDER_ROUTER_SSDP=1

CONTIKI_SOURCEFILES += udp-cmd.c instance-brm.c instance-rtable.c

ifneq ($(WITH_CLOCK_GETTIME),)
  CFLAGS += -DWITH_CLOCK_GETTIME=$(WITH_CLOCK_GETTIME)
endif

WITH_WEBSERVER=1
ifeq ($(WITH_WEBSERVER),1)
APPS += br-webserver
endif

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

connect-high:	border-router.native
	sudo $(RUN_GDB) ./border-router.native $(PORTARGS) -B 921600 aaaa::1/64

connect-server:	border-router.native
	sudo $(RUN_GDB) ./border-router.native $(PORTARGS) -B 460800
