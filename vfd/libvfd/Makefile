SERIO_DIR := ../../serio
CFLAGS := -I$(SERIO_DIR) -O -Wall -g -Wmissing-prototypes
SRCS = vfd.c

PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Cygwin)

CC := i686-w64-mingw32-gcc

else

CFLAGS := $(CFLAGS) -fPIC

libvfd.so: libvfd.a Makefile
	$(CC) $(CFLAGS) -shared -Wl,--no-undefined -Wl,--whole-archive $< -Wl,--no-whole-archive -o $@

.PHONY : install
install: $(TARGET) vfd.h
	install libvfd.so /usr/local/lib
	install vfd.h /usr/local/include

.PHONY : uninstall
uninstall:
	rm -f /usr/local/lib/libvfd.so
	rm -f /usr/local/include/vfd.h

endif

OBJS := $(SRCS:%.c=%.o)

libvfd.a: $(OBJS) $(SERIO_DIR)/libserio.a Makefile
	cp $(SERIO_DIR)/libserio.a $@
	ar rs $@ $(OBJS)

vfd_test: $(SRCS) $(SERIO_DIR)/libserio.a Makefile
	$(CC) -DTESTING -I$(SERIO_DIR) -g -Wall $(SRCS) $(SERIO_DIR)/libserio.a -o $@

.PHONY : clean
clean:
	rm -f $(OBJS) libvfd.a libvfd.so libvfd.dll *~ *.bak

vfd.o: vfd.c $(SERIO_DIR)/serio.h vfd.h 14.h Makefile
