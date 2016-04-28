CONTIKI_PROJECT = tinydtls
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
NRF52_SDK_ROOT=/home/johan/Downloads/nrf5_iot_sdk_3288530

CONTIKI_WITH_RPL=0
NRF52_WITHOUT_SOFTDEVICE=0
WITH_DTLS=1
PLATFORM_INDICATE_BLE_STATE=0

ifeq ($(ADDR),)
$(error Please define ADDR=<full ipv6 addr, full ipv6 addr, full ipv6 addr, full ipv6 addr>)
else
CFLAGS += -DSERVER_IPV6_ADDR=\"$(ADDR)\"
CFLAGS += -DDEVICE_NAME=\"Testapp1\"
endif

# automatically build RESTful resources
REST_RESOURCES_DIR = ./resources
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c' ! -name 'res-plugtest*'))

PROJECTDIRS += $(REST_RESOURCES_DIR)
PROJECT_SOURCEFILES += $(REST_RESOURCES_FILES)

# linker optimizations
SMALL=1

# REST Engine shall use Erbium CoAP implementation
APPS += er-coap
APPS += rest-engine
APPS += tinydtls

all: $(CONTIKI_PROJECT) 

CONTIKI = ../..
include $(CONTIKI)/Makefile.include
