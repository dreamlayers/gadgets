include ../../build/Makefile.config

SIGND_DIR := ../../signd/daemon
LIBVFD_DIR := ../libvfd
SRCS := vfdcmds.c
CFLAGS := $(CFLAGS) -I$(SIGND_DIR) -I$(LIBVFD_DIR)

TARGET := vfdd

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(SIGND_DIR)/signd.a $(LIBVFD_DIR)/libvfd.a
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

sockets.o: sockets.c signd.h Makefile
tray42.o: tray42.c signd.h tray42.h Makefile
