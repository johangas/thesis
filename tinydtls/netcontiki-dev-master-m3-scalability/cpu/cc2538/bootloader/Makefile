#
#
# $Id: Makefile 198 2013-03-13 01:25:10Z flag/svn@YANZI.SE $
#
# makefile for M3 bootloader.
#
#
# Define tools used
TOOL=arm-none-eabi-
CC=$(TOOL)gcc
AS=$(TOOL)gcc
AR=$(TOOL)ar
LD=$(TOOL)ld
OBJCOPY=$(TOOL)objcopy
SIZE=$(TOOL)size
SRCS=
ASRCS=
CFLAGS=
OBJECTDIR=.

#IMAGE=0

all: cc2538.ld bootloader

#
# Contiki source where we can find trailer.h
CONTIKI=../../..
CONTIKI_CPU=$(CONTIKI)/cpu/cc2538
COMMON=$(CONTIKI)/yanzi/common
include $(CONTIKI_CPU)/Makefile.cc2538


# The target application name.
BOOTLOADER=bootloader.elf

CPPFLAGS=-MD
CFLAGS+=-mthumb -mcpu=cortex-m3
CFLAGS+=-I.
CFLAGS+=-I$(CONTIKI)/core
CFLAGS+=-I$(CONTIKI_CPU)
CFLAGS+=-I$(CONTIKI_CPU)/dev
CFLAGS+=-I$(COMMON)
CFLAGS+=-g -Wall -Werror -Wundef -Os -O2

SRCS+=bootloader.c
SRCS+=startup-gcc.c
SRCS+=$(COMMON)/crc.c
SRCS+=$(COMMON)/trailer.c

# Convert source name to object and dependencies
OBJS=$(SRCS:.c=.o) $(ASRCS:.s79=.o)
DEPS=$(SRCS:.c=.d) $(ASRCS:.s79=.d)

# Linker options
LINKEROPTS=-mthumb
LINKEROPTS+=-mcpu=cortex-m3
LINKEROPTS+=-T cc2538.ld
#LINKEROPTS+=-L $(CONTIKI)/cpu/stm32w108 -T image0.ld
LINKEROPTS+=-static
LINKEROPTS+=-Wl,-cref,-u,Reset_Handler
LINKEROPTS+=-Wl,--gc-sections

# The actual rules to build it all

%.o: %.s79
	$(AS) $(ASFLAGS) -o $@ $<

bootloader: $(BOOTLOADER)
	$(OBJCOPY) -O binary $(BOOTLOADER) bootloader.odd
	dd if=/dev/zero of=pad bs=1 count=`ls -l bootloader.odd | awk ' { pad = 0; size = $$5 ; rest = size % 16 ; if (rest > 0) { pad = 16 - rest;}  print pad }'` > /dev/null
	cat bootloader.odd pad > bootloader
	rm bootloader.odd pad


$(BOOTLOADER): $(OBJS)
	$(CC) -o $@ $(LINKEROPTS) -Wl,-Map=$(@:.elf=.map) $(OBJS)
	$(SIZE) -A -x $@
	$(SIZE) -B -x $@


clean:
	rm -f $(BOOTLOADER) $(BOOTLOADER:.elf=.map) $(OBJS) $(OBJS:.o=.d)
	rm -f cc2538.ld bootloader mfg-felicia.bin cca.bin

# include all dependencies (softly; no error if missing)
-include $(DEPS)
