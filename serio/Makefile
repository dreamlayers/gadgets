include ../build/Makefile.config

SRCS := serio.c
TARGET := libserio.a

ifeq ($(PLATFORM),Windows)

SRCS := $(SRCS) serio_windows.c
OBJS := $(SRCS:%.c=%.o)

else

CFLAGS := $(CFLAGS) $(PIC)
SRCS := $(SRCS) serio_unix.c serio_tcp.c
OBJS := $(SRCS:%.c=%.o) ../common/tcp.o

endif


$(TARGET): $(OBJS)
	ar rcs $@ $^

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

serio.o: serio.c serio.h Makefile
serio_unix.o: serio_unix.c serio.h Makefile
serio_windows.o: serio_windows.c serio.h Makefile
