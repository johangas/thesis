CONTIKI=../../..
CONTIKI_PROJECT=debug-node

TARGET=native

all: $(CONTIKI_PROJECT)

APPS += slip-cmd encap
APPS += oam
APPS += oam/instance-nstats

ifdef WITH_NETSCAN
APPS += netscan
endif

ifdef WITH_IPSO
APPS += ipso-objects
endif

ifdef WITH_6LOWPAN_ND
CONF_6LOWPAN_ND=1
UIP_CONF_6LR=1
CFLAGS += -DUIP_CONF_ND6_RA_PERIODIC=0 -DTCPIP_CONF_ANNOTATE_TRANSMISSIONS=1
endif

CURSES_LIBS=

#linker optimizations
SMALL=1

# OSX (g)cc does not like  -Wunused-but-set-variable
MACHINE := $(shell gcc -dumpmachine)
ifeq (darwin, $(findstring darwin, $(MACHINE)))
CFLAGS += -Wunused
else
CFLAGS += -Wunused-but-set-variable
endif
CFLAGS += -Wunused
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CONTIKI_SOURCEFILES += debug-node.c debug-cmds.c border-router-rdc.c \
border-router-radio.c br-config.c enc-dev.c border-router-cmds.c ylog.c

PROJECTDIRS += ../yanzi-border-router/lib
PROJECTDIRS += ../yanzi-border-router


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

connect-normal: debug-node.native
	./debug-node.native $(PORTARGS)

connect-full:	debug-node.native
	./debug-node.native $(PORTARGS) -B 460800

connect-high:	debug-node.native
	./debug-node.native $(PORTARGS) -B 921600
