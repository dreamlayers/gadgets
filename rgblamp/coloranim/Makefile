include ../../build/Makefile.config

# Local configuration

SIGND_DIR := ../../signd
LIBRGBLAMP_DIR := ../librgblamp
RPI_WS281X_DIR := /home/pi/gitapps/rpi_ws281x

COLORANIMDCONF = /usr/local/etc/coloranimd.conf
ifneq ("$(wildcard $(COLORANIMDCONF))","")
include $(COLORANIMDCONF)
endif

# Driver configuration

ifeq ($(DRIVER),)
ifneq ("$(wildcard $(RPI_WS281X_DIR)/ws2811.h)","")
DRIVER := rpi_ws281x
else
DRIVER := rgblamp
endif
endif

ifeq ($(DRIVER),rgblamp)
CFLAGS += -I$(LIBRGBLAMP_DIR)
MQTT_ID ?= behind_pc
MQTT_NAME ?= Behind PC
PIXCNT := 1
EXTRA_DEPS += $(LIBRGBLAMP_DIR)/librgblamp.a
COMMON_LIBS += $(LIBRGBLAMP_DIR)/librgblamp.a
else ifeq ($(DRIVER),rpi_ws281x)
CFLAGS += -I$(RPI_WS281X_DIR) -DPWR_TMOUT=5
MQTT_ID ?= behind_tv
MQTT_NAME ?= Behind TV
PIXCNT ?= 60
COMMON_LIBS += -L$(RPI_WS281X_DIR) -lws2811 -lwiringPi
endif
CFLAGS += '-DMQTT_ID="$(MQTT_ID)"' '-DMQTT_NAME="$(MQTT_NAME)"' -DPIXCNT=$(PIXCNT)

# Defining that two targets

CFLAGS += -I$(SIGND_DIR)/daemon

COMMON_SRCS := coloranim.c parser.c drv_$(DRIVER).c
COMMON_LIBS += -lm

STANDALONE_TARGET := coloranim$(EXESUFFIX)
STANDALONE_SRCS := $(COMMON_SRCS) standalone.c
STANDALONE_OBJS := $(STANDALONE_SRCS:%.c=%.o)
STANDALONE_LIBS := $(COMMON_LIBS)

DAEMON_TARGET := coloranimd$(EXESUFFIX)
DAEMON_SRCS := $(COMMON_SRCS) mqtt.c json.c rgbcmds.c presets.c
DAEMON_LIBS := $(COMMON_LIBS) -lmosquitto

RGBM_PREREQ := fftw3 portaudio-2.0
ifeq ($(shell $(PKG_CONFIG) --exists $(RGBM_PREREQ) || echo no),no)
$(warning WARNING: Not building RGB lamp visualizer due to lack of prerequisites.)
else
DAEMON_LIBS := $(DAEMON_LIBS) ../vis_rgb/libvisrgb.a -pthread $(shell $(PKG_CONFIG) --libs $(RGBM_PREREQ))
DAEMON_SRCS := $(DAEMON_SRCS) rgbm_glue.c
CFLAGS += -I../../rgblamp/librgblamp -I../vis_rgb
endif

DAEMON_OBJS := $(DAEMON_SRCS:%.c=%.o)

# Platform configuration for specific targets

ifeq ($(PLATFORM),Windows)
DAEMON_OBJS += coloranimd-res.o winrgbchoose.o
DAEMON_LIBS := -L/usr/local/cross-tools/i686-w64-mingw32/lib \
               $(DAEMON_LIBS) -lws2_32 -mwindows
CFLAGS += -I/usr/local/cross-tools/i686-w64-mingw32/include
else
DAEMON_LIBS += -lpthread
endif

# Target rules

.PHONY: all
all: $(STANDALONE_TARGET) $(DAEMON_TARGET)

$(DAEMON_TARGET): $(DAEMON_OBJS) $(EXTRA_DEPS) $(SIGND_DIR)/daemon/signd.a
	$(CC) $(CFLAGS) $^ $(DAEMON_LIBS) -o $@

$(STANDALONE_TARGET): $(STANDALONE_OBJS) $(EXTRA_DEPS)
	$(CC) $(CFLAGS) $^ $(STANDALONE_LIBS) -o $@

.PHONY: clean
clean:
	rm -f $(DAEMON_OBJS) $(STANDALONE_OBJS) \
	      $(DAEMON_TARGET) $(STANDALONE_TARGET)

# Dependencies

ifeq ($(PLATFORM),Windows)
coloranimd-res.o: coloranimd.rc coloranimd.ico
	$(WINDRES) -I$(SIGND_DIR)/daemon $< $@
winrgbchoose.o: ../winrgbchoose/winrgbchoose.c
	$(CC) -c $(CFLAGS) -I. -DCOLORANIMD_WRGBC -o $@ $<
endif

coloranim.o: coloranim.c coloranim.h Makefile
parser.o: parser.c coloranim.h Makefile
drv_rgblamp.o: $(LIBRGBLAMP_DIR)/librgblamp.h \
               coloranim.h Makefile
drv_$(DRIVER).o: drv_$(DRIVER).c coloranim.h Makefile
mqtt.o: mqtt.c coloranim.h Makefile
json.o: json.c coloranim.h Makefile
rgbcmds.o: rgbcmds.c coloranim.h \
           $(SIGND_DIR)/daemon/signd.h $(SIGND_DIR)/sdprotocol.h Makefile
standalone.o: standalone.c coloranim.h Makefile
