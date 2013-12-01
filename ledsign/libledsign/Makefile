SERIO_DIR := ../../serio
LIBNAME := libledsign
LIBINCLUDE := ledsign.h
CFLAGS := -I$(SERIO_DIR) -O -Wall -g -Wmissing-prototypes
SRCS = ledsign.c

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
	install $(LIBINCLUDE) /usr/local/include

.PHONY : uninstall
uninstall:
	rm -f /usr/local/lib/$(LIBNAME).so
	rm -f /usr/local/include/$(LIBINCLUDE)

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

ledsign.o: ledsign.c $(SERIO_DIR)/serio.h ledsign.h Makefile
