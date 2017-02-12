include ../../build/Makefile.config

LIBRGBLAMP_DIR := ../librgblamp
CFLAGS := $(CFLAGS) -I$(LIBRGBLAMP_DIR)
SRCS := rgbm.c

LIBRGBLAMP_DIR := ../librgblamp

ifeq ($(PLATFORM),Windows)

WINAMPAPI_DIR := ../../pluginapi/winamp
CFLAGS := $(CFLAGS) -DRGBM_WINAMP -I$(WINAMPAPI_DIR)
SRCS := $(SRCS) rgbvis.c
TARGET := vis_rgb.dll

all: $(TARGET)

rgbm.o: greentab_winamp.h

else

PKG_PREREQ := audacious glib-2.0 dbus-glib-1 dbus-1
CFLAGS := $(CFLAGS) -DRGBM_AUDACIOUS \
		  $(shell pkg-config --cflags $(PKG_PREREQ)) $(PIC)
LIBS = $(shell pkg-config --libs $(PKG_PREREQ)) -lpthread -lm
SRCS := $(SRCS) aud_rgb.c
TARGET := aud_rgb.so

.PHONY : install uninstall all

all: $(TARGET)

install: $(TARGET)
	cp $(TARGET) ~/.local/share/audacious/Plugins/

uninstall:
	rm ~/.local/share/audacious/Plugins/$(TARGET)

rgbm.o: greentab_audacious.h freqadj_audacious.h

endif

CXXFLAGS := $(filter-out -Wmissing-prototypes,$(CFLAGS)) -std=c++11

OBJS := $(SRCS:%.c=%.o)
OBJS := $(OBJS:%.cc=%.o)

$(TARGET): $(OBJS) $(LIBRGBLAMP_DIR)/librgblamp.a
	$(CXX) $(CXXFLAGS) -shared -Wl,--no-undefined \
	-Wl,--exclude-libs,ALL $^ $(LIBS) -o $@

.PHONY : clean veryclean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

veryclean: clean
	rm -f freqadj_audacious.h greentab_audacious.h greentab_winamp.h

rgbvis.o: rgbvis.c $(WINAMPAPI_DIR)/vis.h rgbm.h Makefile

aud_rgb.o: aud_rgb.cc rgbm.h Makefile

rgbm.o: rgbm.c rgbm.h $(LIBRGBLAMP_DIR)/librgblamp.h Makefile \

freqadj_audacious.h: makefreqadj.m
	octave -q $^

greentab_audacious.h: makeramps.rb
	ruby makeramps.rb 144 1 512 > $@

greentab_winamp.h: makeramps.rb
	ruby makeramps.rb 291 -1 1024 > $@
