include ../../build/Makefile.config

LIBRGBLAMP_DIR := ../librgblamp
LIBRGBLAMP := $(LIBRGBLAMP_DIR)/librgblamp.a
CFLAGS := $(CFLAGS) -I$(LIBRGBLAMP_DIR)
SRCS := rgbm.c

RPI_WS281X_DIR := /home/pi/gitapps/rpi_ws281x

ifeq ($(PLATFORM),Windows)

WINAMPAPI_DIR := ../../pluginapi/winamp
WINAMP_CFLAGS := $(CFLAGS) -DRGBM_WINAMP -I$(WINAMPAPI_DIR)
SRCS := $(SRCS) rgbvis.c rgbm_rgblamp.c
TARGET := vis_rgb.dll
OBJS := $(SRCS:%.c=%.o)

all: $(TARGET)

$(TARGET): $(OBJS) $(LIBRGBLAMP)
	$(CC) $(CFLAGS) -mwindows -shared -static-libgcc \
	-Wl,--no-undefined -Wl,--exclude-libs,ALL $^ $(LIBS) -o $@

rgbm.o: rgbm.c greentab_winamp.h
	$(CC) $(WINAMP_CFLAGS) -c $< -o $@

rgbm_rgblamp.o: rgbm_rgblamp.c rgbm.h $(LIBRGBLAMP_DIR)/librgblamp.h Makefile
	$(CC) $(WINAMP_CFLAGS) -c $< -o $@

rgbvis.o: rgbvis.c $(WINAMPAPI_DIR)/vis.h rgbm.h Makefile
	$(CC) $(WINAMP_CFLAGS) -c $< -o $@

endif

.PHONY : install uninstall all dummy
STANDALONE_PREREQ := fftw3 portaudio-2.0
ifeq ($(shell $(PKG_CONFIG) --exists $(STANDALONE_PREREQ) || echo no),no)
$(warning WARNING: Not building RGB lamp standalone visualizer due to lack of prerequisites.)
else
STANDALONE_CFLAGS := $(CFLAGS) -DRGBM_FFTW -pthread $(shell $(PKG_CONFIG) --cflags $(STANDALONE_PREREQ))

STANDALONE_LIB := libvisrgb.a
STANDALONE_LIB_OBJS := rgbm-standalone.o portaudio.o soundbuf.o

STANDALONE := visrgb$(EXESUFFIX)
STANDALONE_OBJS := main.o
STANDALONE_LIBS := $(shell $(PKG_CONFIG) --libs $(STANDALONE_PREREQ)) -lm

ifeq ($(PLATFORM),Windows)
STANDALONE_CFLAGS += -static
endif

DRVDLL_FLAGS := $(STANDALONE_CFLAGS) -shared -fvisibility=default
ifneq ($(PLATFORM),Cygwin)
ifneq ($(PLATFORM),Windows)
DRVDLL_FLAGS := $(DRVDLL_FLAGS) -fPIC
endif
endif

ifneq ($(PLATFORM),Windows)
DRVDLL_DDP_LIBS :=
else
DRVDLL_DDP_LIBS := -lws2_32
endif
DRVDLL_DDP := $(DLPREFIX)rgbmddp$(DLSUFFIX)
all: $(DRVDLL_DDP)
$(DRVDLL_DDP): rgbm_ddp.c rgbm.h Makefile
	$(CC) -shared $(DRVDLL_FLAGS) $< -lm $(DRVDLL_DDP_LIBS) -o $@

ifneq ("$(wildcard $(RPI_WS281X_DIR)/ws2811.h)","")
DRVDLL_RPI := $(DLPREFIX)rgbmrpi$(DLSUFFIX)
all: $(DRVDLL_RPI)
$(DRVDLL_RPI): rpi_ws281x_rgb.c rgbm.h Makefile
	$(CC) -shared -I$(RPI_WS281X_DIR) $(DRVDLL_FLAGS) $< \
	-L$(RPI_WS281X_DIR) -lws2811 -lm -o $@
STANDALONE_CFLAGS += '-DMONITOR_NAME="alsa_monitor"' '-DDEFAULT_DRIVER="rpi"'
endif

STANDALONE_OBJS += rgbm_multi.o

all: $(STANDALONE)

DRVDLL_RGBLAMP := $(DLPREFIX)rgbmlamp$(DLSUFFIX)
all: $(DRVDLL_RGBLAMP)
$(DRVDLL_RGBLAMP): rgbm_rgblamp.c rgbm.h $(LIBRGBLAMP_DIR)/librgblamp.h $(LIBRGBLAMP) Makefile
	$(CC) -shared $(DRVDLL_FLAGS) $< $(LIBRGBLAMP) -lm -o $@

DRVDLL_RGBSTRIP := $(DLPREFIX)rgbmstrip$(DLSUFFIX)
LIBRGBSTRIP := ../librgbstrip/librgbstrip.a
all: $(DRVDLL_RGBSTRIP)
$(DRVDLL_RGBSTRIP): rgbm_rgblamp.c rgbm.h $(LIBRGBLAMP_DIR)/librgblamp.h \
                    $(LIBRGBSTRIP) Makefile
	$(CC) -shared $(DRVDLL_FLAGS) $< $(LIBRGBSTRIP) -lm -o $@

$(STANDALONE_LIB): $(STANDALONE_LIB_OBJS)
	ar rcs $@ $^

$(STANDALONE): $(STANDALONE_OBJS) $(STANDALONE_LIB)
	$(CC) $(STANDALONE_CFLAGS) $^ -o $@ $(STANDALONE_LIBS)

rgbm-standalone.o: rgbm.c rgbm.h $(LIBRGBLAMP_DIR)/librgblamp.h  \
                   greentab_audacious.h freqadj_audacious.h Makefile
	$(CC) $(STANDALONE_CFLAGS) -c $< -o $@

portaudio.o: portaudio.c rgbm.h Makefile
	$(CC) $(STANDALONE_CFLAGS) -c $< -o $@

soundbuf.o: soundbuf.c rgbm.h soundbuf.h Makefile
	$(CC) $(STANDALONE_CFLAGS) -c $< -o $@

main.o: main.c rgbm.h Makefile
	$(CC) $(STANDALONE_CFLAGS) -c $< -o $@

SDLVIS_PREREQ := sdl2
ifeq ($(shell $(PKG_CONFIG) --exists $(SDLVIS_PREREQ) || echo no),no)
$(warning WARNING: Not building SDL visualizer output due to lack of prerequisites.)
else

DRVDLL_SDL := $(DLPREFIX)rgbmsdl$(DLSUFFIX)
all: $(DRVDLL_SDL)
$(DRVDLL_SDL): sdl_rgb.c rgbm.h Makefile
	$(CC) -shared $(DRVDLL_FLAGS) $< \
	$(shell $(PKG_CONFIG) --cflags --libs $(SDLVIS_PREREQ)) -lm -o $@

endif

endif

ifneq ($(PLATFORM),Windows)

PKG_PREREQ := audacious glib-2.0 dbus-glib-1 dbus-1
ifeq ($(shell $(PKG_CONFIG) --exists $(PKG_PREREQ) || echo no),no)
$(warning WARNING: Not building RGB lamp plugin due to lack of prerequisites.)
else
CFLAGS := $(CFLAGS) -DRGBM_AUDACIOUS \
		  $(shell $(PKG_CONFIG) --cflags $(PKG_PREREQ)) $(PIC)
LIBS = $(shell $(PKG_CONFIG) --libs $(PKG_PREREQ)) -lm
SRCS := $(SRCS) aud_rgb.c rgbm_rgblamp.c
TARGET := aud_rgb.so
CXXFLAGS := $(filter-out -Wmissing-prototypes,$(CFLAGS)) -std=c++11
OBJS := $(SRCS:%.c=%.o)
OBJS := $(OBJS:%.cc=%.o)
all: $(TARGET)

$(TARGET): $(OBJS) $(LIBRGBLAMP)
	$(CXX) $(CXXFLAGS) -shared -Wl,--no-undefined \
	-Wl,--exclude-libs,ALL $^ $(LIBS) -o $@

rgbm.o: greentab_audacious.h freqadj_audacious.h

install: $(TARGET)
	cp $(TARGET) ~/.local/share/audacious/Plugins/

endif

dummy:

uninstall:
	rm ~/.local/share/audacious/Plugins/$(TARGET)

endif

.PHONY : clean veryclean
clean:
	rm -f $(OBJS) $(STANDALONE_OBJS) $(STANDALONE_LIB) \
        $(STANDALONE_LIB_OBJS) sdl_rgb.o $(TARGET) visrgb$(EXESUFFIX) \
        *.so *.dll *~ *.bak greentab_audacious.h greentab_winamp.h

aud_rgb.o: aud_rgb.cc rgbm.h Makefile

rgbm.o: rgbm.c rgbm.h $(LIBRGBLAMP_DIR)/librgblamp.h Makefile

rpi_ws281x_rgb.o: rpi_ws281x_rgb.c $(LIBRGBLAMP_DIR)/librgblamp.h Makefile
	$(CC) $(STANDALONE_CFLAGS) -c $< -o $@

rgbm_rgblamp.o: rgbm_rgblamp.c rgbm.h $(LIBRGBLAMP_DIR)/librgblamp.h Makefile

freqadj_audacious.h: makefreqadj.m
	octave -q $^

greentab_audacious.h: makeramps.py
	python3 makeramps.py 144 1 512 > $@.tmp
	mv -f $@.tmp $@

greentab_winamp.h: makeramps.py
	python3 makeramps.py 291 -1 1024 > $@.tmp
	mv -f $@.tmp $@
