SERIO_DIR := ../../serio
CFLAGS := -I$(SERIO_DIR) -O -Wall -g -Wmissing-prototypes
SRCS := librgblamp.c
LIBS := -lm

PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Cygwin)

CC := i686-w64-mingw32-gcc

else

CFLAGS := $(CFLAGS) -fPIC

librgblamp.so: librgblamp.a Makefile
	$(CC) $(CFLAGS) -shared -Wl,--no-undefined \
	-Wl,--whole-archive $< -Wl,--no-whole-archive -lm -o $@

.PHONY : install
install: librgblamp.a vfd.h
	install librgblamp.so /usr/local/lib
	install librgblamp.h /usr/local/include

.PHONY : uninstall
uninstall:
	rm -f /usr/local/lib/librgblamp.so
	rm -f /usr/local/include/librgblamp.h

endif

OBJS := $(SRCS:%.c=%.o)

librgblamp.a: $(OBJS) $(SERIO_DIR)/libserio.a Makefile
	cp $(SERIO_DIR)/libserio.a $@
	ar rs $@ $(OBJS)

.PHONY : clean
clean:
	rm -f $(OBJS) librgblamp.a librgblamp.so librgblamp.dll *~ *.bak

librgblamp.o: librgblamp.c ../../serio/serio.h librgblamp.h
