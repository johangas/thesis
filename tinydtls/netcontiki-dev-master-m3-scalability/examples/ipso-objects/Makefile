CONTIKI=../..
CONTIKI_PROJECT = example-ipso-objects

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

all: $(CONTIKI_PROJECT)

APPS += ipso-objects

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
