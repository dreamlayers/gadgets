include ../../build/Makefile.config

LIBRGBLAMP_DIR := ../librgblamp
CFLAGS := $(CFLAGS) -I$(LIBRGBLAMP_DIR)
SRCS :=

LIBRGBLAMP_DIR := ../librgblamp

ifeq ($(PLATFORM),Windows)

WINAMPAPI_DIR := ../../pluginapi/winamp
CFLAGS := $(CFLAGS) -I$(WINAMPAPI_DIR)
SRCS := $(SRCS) rgbvis.c
TARGET := vis_rgb.dll

else

PKG_PREREQ := audacious glib-2.0 dbus-glib-1 dbus-1
CFLAGS := $(CFLAGS) -DRGBM_AUDACIOUS \
		  $(shell pkg-config --cflags $(PKG_PREREQ)) $(PIC)
LIBS = $(shell pkg-config --libs $(PKG_PREREQ)) -lpthread -lm
SRCS := $(SRCS) aud_rgb.c rgbm.c
TARGET := aud_rgb.so

install: $(TARGET)
	cp $(TARGET) ~/.local/share/audacious/Plugins/

uninstall:
	rm ~/.local/share/audacious/Plugins/$(TARGET)

endif

CFLAGS := $(CFLAGS)

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(LIBRGBLAMP_DIR)/librgblamp.a
	$(CC) $(CFLAGS) -shared -Wl,--no-undefined \
	-Wl,--exclude-libs,ALL $^ $(LIBS) -o $@

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

rgbvis.o: rgbvis.c $(WINAMPAPI_DIR)/vis.h \
          $(LIBRGBLAMP_DIR)/librgblamp.h Makefile

aud_rgb.o: aud_rgb.c rgbm.h Makefile

rgbm.o: rgbm.c rgbm.h $(LIBRGBLAMP_DIR)/librgblamp.h Makefile
