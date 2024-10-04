include ../../build/Makefile.config

LIBVFD_DIR := ../libvfd
SRCS := vfdm.c
CFLAGS := $(CFLAGS) -I$(LIBVFD_DIR)

ifeq ($(PLATFORM),Windows)

SRCS := $(SRCS) vfdm_windows.c sysmon_windows.c
LIBS := -lpowrprof
TARGETS := vis_vfd.dll dsp_vfd.dll
WINAMPAPI := ../../pluginapi/winamp
CFLAGS := $(CFLAGS) -I$(WINAMPAPI)

all: $(TARGETS)

vfd_winamp_vis.o : vfdm_winamp.c $(WINAMPAPI)/vis.h lrint.h vfdm.h
	$(CC) -c $(CFLAGS) -DVISPLUG -o $@ $<

vfd_winamp_dsp.o : vfdm_winamp.c $(WINAMPAPI)/dsp.h lrint.h vfdm.h
	$(CC) -c $(CFLAGS) -DDSPPLUG -o $@ $<

OBJS := $(SRCS:%.c=%.o)

vis_vfd.dll: $(OBJS) vfd_winamp_vis.o $(LIBVFD_DIR)/libvfd.a
	$(CC) $(CFLAGS) -shared -static-libgcc -o $@ $^ $(LIBS)

dsp_vfd.dll: $(OBJS) vfd_winamp_dsp.o $(LIBVFD_DIR)/libvfd.a
	$(CC) $(CFLAGS) -shared -static-libgcc -o $@ $^ $(LIBS)

CLEAN_EXTRA := vfd_winamp_vis.o vfd_winamp_dsp.o

else

PKG_PREREQ := audacious glib-2.0 dbus-glib-1 dbus-1
ifeq ($(shell pkg-config --exists $(PKG_PREREQ) || echo no),no)
fail:
	$(warning WARNING: Not building vfd plugin due to lack of prerequisites.)
endif
CFLAGS := $(shell pkg-config --cflags $(PKG_PREREQ)) $(CFLAGS) $(PIC)
CXXFLAGS := $(filter-out -Wmissing-prototypes,$(CFLAGS)) -std=c++11
LIBS := $(shell pkg-config --libs $(PKG_PREREQ)) -lpthread -lm
SRCS := $(SRCS) sysmon_linux.c vfdm_unix.c pm_systemd.c avfd.cc avfd_trigger.cc

TARGET := aud_vfd.so
TARGETS := $(TARGET)

OBJS := $(SRCS:%.c=%.o)
OBJS := $(OBJS:%.cc=%.o)

$(TARGET): $(OBJS) $(LIBVFD_DIR)/libvfd.a
	$(CXX) $(CXXFLAGS) -shared -Wl,--no-undefined \
	-Wl,--exclude-libs,ALL $^ $(LIBS) -o $@

.PHONY : install uninstall

install: $(TARGET)
	cp $(TARGET) ~/.local/share/audacious/Plugins/

uninstall:
	rm ~/.local/share/audacious/Plugins/$(TARGET)

endif

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGETS) $(CLEAN_EXTRA) *~ *.bak

analoglog.o: analoglog.c Makefile
avfd.o: avfd.cc avfd_trigger.h vfdm.h Makefile
avfd_trigger.o: avfd_trigger.cc avfd_trigger.h avfd_trigger_private.h \
 vfdm.h Makefile
pm_systemd.o: pm_systemd.c vfdm.h sysmon.h Makefile
sysmon_linux.o: sysmon_linux.c sysmon.h Makefile
sysmon_windows.o: sysmon_windows.c sysmon.h Makefile
vfdm.o: vfdm.c $(LIBVFD_DIR)/vfd.h sysmon.h vfdm.h Makefile
vfdm_unix.o: vfdm_unix.c vfdm.h Makefile
vfdm_windows.o: vfdm_windows.c vfdm.h Makefile
