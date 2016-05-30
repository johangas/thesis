CONTIKI=../../..
CONTIKI_PROJECT = iot-u10plus

TARGET=felicia
TARGET_VARIANT=iot-u10plus
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

all: $(CONTIKI_PROJECT)

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

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
