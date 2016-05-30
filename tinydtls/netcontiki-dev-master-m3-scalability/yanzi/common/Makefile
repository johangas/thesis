
TARGETLIB=libyanzi-common.a

all: check_target $(TARGETLIB)

SRCS = crc.c trailer.c
DEPS=$(SRCS:.c=.d)
OBJS=$(SRCS:.c=.o)

LAST_TARGET = ${shell cat .last_target 2> /dev/null}

$(TARGETLIB): $(OBJS)
	$(AR) rcf $(TARGETLIB) $(OBJS)

clean:
	rm -f $(OBJS) $(SRCS:.c=.d) $(TARGETLIB) .last_target

-include $(DEPS)

.phony: check_target
check_target:
	@if [ ! "$(TARGET)" = "$(LAST_TARGET)" ] ; then echo "New target, cleaning." ; ${MAKE} clean ; fi
	@echo $(TARGET) > .last_target
