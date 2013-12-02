include ../../build/Makefile.config

SRCS := sockets.c gencmds.c

ifeq ($(PLATFORM),Cygwin)
CFLAGS := $(CFLAGS) -mwindows
SRCS := $(SRCS) tray42.c
endif

TARGET := signd.a

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS)
	ar rcs $@ $^

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

sockets.o: sockets.c signd.h ../sdprotocol.h Makefile
tray42.o: tray42.c signd.h tray42.h Makefile
