LIBNAME := libvfd
SERIO_DIR := ../../serio
CFLAGS := -I$(SERIO_DIR) -O -Wall -g -Wmissing-prototypes
SRCS = vfd.c

PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Cygwin)

CC := i686-w64-mingw32-gcc

else

CFLAGS := $(CFLAGS) -fPIC

$(LIBNAME).so: $(LIBNAME).a Makefile
	$(CC) $(CFLAGS) -shared -Wl,--no-undefined -Wl,--whole-archive $< -Wl,--no-whole-archive -o $@

.PHONY : install
install: $(TARGET) vfd.h
	install $(LIBNAME).so /usr/local/lib
	install vfd.h /usr/local/include
	install vfd.rb /usr/local/lib/site_ruby

.PHONY : uninstall
uninstall:
	rm -f /usr/local/lib/$(LIBNAME).so
	rm -f /usr/local/include/vfd.h
	rm -f /usr/local/lib/site_ruby/vfd.rb

endif

OBJS := $(SRCS:%.c=%.o)

$(LIBNAME).a: $(OBJS) $(SERIO_DIR)/libserio.a Makefile
	cp $(SERIO_DIR)/libserio.a $@
	ar rs $@ $(OBJS)

vfd_test: $(SRCS) $(SERIO_DIR)/libserio.a Makefile
	$(CC) -DTESTING -I$(SERIO_DIR) -g -Wall $(SRCS) $(SERIO_DIR)/libserio.a -o $@

.PHONY : clean
clean:
	rm -f $(OBJS) $(LIBNAME).a $(LIBNAME).so $(LIBNAME).dll *~ *.bak

vfd.o: vfd.c $(SERIO_DIR)/serio.h vfd.h 14.h Makefile
