include ../build/Makefile.config

SRCS := tcp.c

OBJS := $(SRCS:%.c=%.o)

.PNONY : all
all: $(OBJS)

.PHONY : clean
clean:
	rm -f $(OBJS) *~ *.bak

tcp.o: tcp.c tcp.h Makefile
