CONTIKI=../../..
CONTIKI_PROJECT = dual-op-mode

TARGET=felicia
TARGET_VARIANT=iot-u10

all: $(CONTIKI_PROJECT)

APPS += slip-cmd encap
APPS += oam
APPS += oam/instance-leds
APPS += oam/instance-button
APPS += oam/instance-nstats
APPS += oam/stts751
APPS += er-coap
APPS += rest-engine
APPS += yanzi/resources-coap
APPS += yanzi/resources-web
APPS += yanzi/resources-common

# instances to add to OAM
OAM_INSTANCES = flash

ifdef WITH_NETSCAN
APPS += netscan
endif

ifdef WITH_IPSO
APPS += ipso-objects
endif

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# Include sourcefiles from serial radio
PROJECT_SOURCEFILES += instance-radio.c
PROJECT_SOURCEFILES += slip-net.c no-framer.c sniffer-rdc.c radio-scan.c
PROJECT_SOURCEFILES += serial-radio.c
# Include proxy modules
PROJECT_SOURCEFILES += dual-mode-net.c dual-mode-rdc.c

CONTIKIDIRS += ../yanzi-serial-radio

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
