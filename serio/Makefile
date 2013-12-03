include ../build/Makefile.config

SRCS := serio.c
TARGET := libserio.a

ifeq ($(PLATFORM),Windows)

SRCS := $(SRCS) serio_windows.c

else

CFLAGS := $(CFLAGS) $(PIC)
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
