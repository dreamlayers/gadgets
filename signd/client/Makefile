SRCS := sdclient.c
CFLAGS := -O -Wall -Wmissing-prototypes -g

TARGET := libsdclient.a

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS)
	ar rcs $@ $^

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

sdclient.o: sdclient.c sdclient.h ../sdprotocol.h Makefile
