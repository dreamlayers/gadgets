include ../../build/Makefile.config

LIBRGBLAMP_DIR := ../librgblamp
SRCS := aud_rgb.c
CFLAGS := $(CFLAGS) -I$(LIBRGBLAMP_DIR)
PKG_PREREQ := audacious glib-2.0 dbus-glib-1 dbus-1
CFLAGS := $(CFLAGS) $(shell pkg-config --cflags $(PKG_PREREQ)) $(PIC)
LIBS = $(shell pkg-config --libs $(PKG_PREREQ)) -lpthread -lm

TARGET := aud_rgb.so

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(LIBRGBLAMP_DIR)/librgblamp.a
	$(CC) $(CFLAGS) -shared -Wl,--no-undefined \
	-Wl,--exclude-libs,ALL $^ $(LIBS) -o $@

install: $(TARGET)
	cp $(TARGET) ~/.local/share/audacious/Plugins/

uninstall:
	rm ~/.local/share/audacious/Plugins/$(TARGET)

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

aud_rgb.o: aud_rgb.c ../librgblamp/librgblamp.h Makefile
