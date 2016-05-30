CONTIKI_PROJECT=serial-radio
all: $(CONTIKI_PROJECT)
APPS = slip-cmd encap oam
#CFLAGS += -Werror

ifdef WITH_SPI
CFLAGS += -DWITH_SPI=1
else ifdef WITH_UART
CFLAGS += -DWITH_UART=1
endif

PROJECT_SOURCEFILES += instance-radio.c
OAM_INSTANCES += flash

CONTIKI=../../..

#linker optimizations
SMALL=1

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += slip-net.c no-framer.c sniffer-rdc.c radio-scan.c

CONTIKI_WITH_RPL = 0
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
