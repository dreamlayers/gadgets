LIBVFD_DIR := ../libvfd
SRCS := vfdm.c
CFLAGS := -I$(LIBVFD_DIR) -fvisibility=hidden -O -Wall -Wmissing-prototypes -g

PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Cygwin)

CC := i686-pc-mingw32-gcc
SRCS := $(SRCS) vfdm_windows.c sysmon_windows.c
LIBS := -lpowrprof
TARGETS := vis_vfd.dll dsp_vfd.dll

all: $(TARGETS)

vfd_winamp_vis.o : vfdm_winamp.c vis.h lrint.h vfdm.h
	$(CC) -c $(CFLAGS) -DVISPLUG -o $@ $<

vfd_winamp_dsp.o : vfdm_winamp.c dsp.h lrint.h vfdm.h
	$(CC) -c $(CFLAGS) -DDSPPLUG -o $@ $<

OBJS := $(SRCS:%.c=%.o)

vis_vfd.dll: $(OBJS) vfd_winamp_vis.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

dsp_vfd.dll: $(OBJS) vfd_winamp_dsp.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

CLEAN_EXTRA := vfd_winamp_vis.o vfd_winamp_dsp.o

else

PKG_PREREQ := audacious glib-2.0 dbus-glib-1 dbus-1
CC := gcc
CFLAGS := $(shell pkg-config --cflags $(PKG_PREREQ)) $(CFLAGS) -fPIC
LIBS = $(shell pkg-config --libs $(PKG_PREREQ)) -lpthread -lm
SRCS := $(SRCS) sysmon_linux.c vfdm_unix.c pm_upower.c aosd.c aosd_trigger.c

TARGET := aud_vfd.so
TARGETS := $(TARGET)

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(LIBVFD_DIR)/libvfd.a
	$(CC) $(CFLAGS) -shared -Wl,--no-undefined \
	-Wl,--exclude-libs,ALL $^ $(LIBS) -o $@

install: $(TARGET)
	cp $(TARGET) ~/.local/share/audacious/Plugins/

uninstall:
	rm ~/.local/share/audacious/Plugins/$(TARGET)

endif

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGETS) $(CLEAN_EXTRA) *~ *.bak

analoglog.o: analoglog.c Makefile
aosd.o: aosd.c aosd_trigger.h vfdm.h Makefile
aosd_trigger.o: aosd_trigger.c aosd_trigger.h aosd_trigger_private.h \
 vfdm.h Makefile
pm_upower.o: pm_upower.c vfdm.h sysmon.h Makefile
sysmon_linux.o: sysmon_linux.c sysmon.h Makefile
sysmon_windows.o: sysmon_windows.c sysmon.h Makefile
vfdm.o: vfdm.c $(LIBVFD_DIR)/vfd.h sysmon.h vfdm.h Makefile
vfdm_unix.o: vfdm_unix.c vfdm.h Makefile
vfdm_winamp.o: vfdm_winamp.c vis.h vfdm.h Makefile
vfdm_windows.o: vfdm_windows.c vfdm.h Makefile