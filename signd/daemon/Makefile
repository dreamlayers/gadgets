SRCS := sockets.c gencmds.c
CFLAGS := -O -Wall -Wmissing-prototypes -g

TARGET := signd.a

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS)
	ar rcs $@ $^

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

sockets.o: sockets.c signd.h Makefile
tray42.o: tray42.c signd.h tray42.h Makefile
