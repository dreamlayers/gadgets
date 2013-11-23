SRCS := serio.c vfd.c vfdm.c

PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Cygwin)

CC := i686-pc-mingw32-gcc
CFLAGS := -O -Wall -g -shared
SRCS := $(SRCS) vfdm_windows.c sysmon_windows.c
LIBS := -lpowrprof
TARGETS := vis_vfd.dll dsp_vfd.dll

all: $(TARGETS)

vfd_winamp_vis.o : vfd_winamp.c vis.h lrint.h vfdm.h
	$(CC) -c $(CFLAGS) -DVISPLUG -o $@ $<

vfd_winamp_dsp.o : vfd_winamp.c dsp.h lrint.h vfdm.h
	$(CC) -c $(CFLAGS) -DDSPPLUG -o $@ $<

OBJS := $(SRCS:%.c=%.o)

vis_vfd.dll: $(OBJS) vfd_winamp_vis.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

dsp_vfd.dll: $(OBJS) vfd_winamp_dsp.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

CLEAN_EXTRA := vfd_winamp_vis.o vfd_winamp_dsp.o

else

CC := gcc
CFLAGS := $(shell pkg-config --cflags audacious glib-2.0 dbus-glib-1 dbus-1) -O -Wall -g -fPIC -shared
#LIBS = -lm
SRCS := $(SRCS) sysmon_linux.c vfdm_unix.c pm_upower.c aosd.c aosd_trigger.c

TARGET := aud_vfd.so
TARGETS := $(TARGET)

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $@

install: $(TARGET)
	cp $(TARGET) ~/.local/share/audacious/Plugins/

uninstall:
	rm ~/.local/share/audacious/Plugins/$(TARGET)

endif

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGETS) $(CLEAN_EXTRA) *~ *.bak

.PHONY : depend
depend:
	echo '# Automatically generated dependencies' > Makefile.dep
	gccmakedep -s '# Automatically generated dependencies' -fMakefile.dep -- $(CFLAGS) -- $(SRCS)

include Makefile.dep
