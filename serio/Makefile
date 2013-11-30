SRCS := serio.c
CFLAGS := -fvisibility=hidden -O -Wall -g -Wmissing-prototypes
TARGET := libserio.a

PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Cygwin)

CC := i686-w64-mingw32-gcc
SRCS := $(SRCS) serio_windows.c

else

CFLAGS := $(CFLAGS) -fPIC
SRCS := $(SRCS) serio_unix.c

endif

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS)
	ar rcs $@ $^

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

serio.o: serio.c serio.h Makefile
serio_unix.o: serio_unix.c serio.h Makefile
serio_windows.o: serio_windows.c serio.h Makefile
