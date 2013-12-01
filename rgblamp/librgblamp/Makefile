LIBNAME := librgblamp
SERIO_DIR := ../../serio
CFLAGS := -I$(SERIO_DIR) -O -Wall -g -Wmissing-prototypes
SRCS := librgblamp.c
LIBS := -lm

PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Cygwin)

CC := i686-w64-mingw32-gcc

else

CFLAGS := $(CFLAGS) -fPIC

$(LIBNAME).so: $(LIBNAME).a Makefile
	$(CC) $(CFLAGS) -shared -Wl,--no-undefined \
	-Wl,--whole-archive $< -Wl,--no-whole-archive -lm -o $@

.PHONY : install
install: $(LIBNAME).a librgblamp.h
	install $(LIBNAME).so /usr/local/lib
	install librgblamp.h /usr/local/include
	install rgblamp.rb /usr/local/lib/site_ruby

.PHONY : uninstall
uninstall:
	rm -f /usr/local/lib/$(LIBNAME).so
	rm -f /usr/local/include/librgblamp.h
	rm -f /usr/local/lib/site_ruby/rgblamp.rb

endif

OBJS := $(SRCS:%.c=%.o)

$(LIBNAME).a: $(OBJS) $(SERIO_DIR)/libserio.a Makefile
	cp $(SERIO_DIR)/libserio.a $@
	ar rs $@ $(OBJS)

.PHONY : clean
clean:
	rm -f $(OBJS) $(LIBNAME).a $(LIBNAME).so $(LIBNAME).dll *~ *.bak

librgblamp.o: librgblamp.c ../../serio/serio.h librgblamp.h
