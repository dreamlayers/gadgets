include ../../build/Makefile.config

# Local configuration

SIGND_DIR := ../../signd
LIBRGBLAMP_DIR := ../librgblamp
RPI_WS281X_DIR := /home/pi/gitapps/rpi_ws281x

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
PIXCNT := 1
EXTRA_DEPS += $(LIBRGBLAMP_DIR)/librgblamp.a
COMMON_LIBS += $(LIBRGBLAMP_DIR)/librgblamp.a
else ifeq ($(DRIVER),rpi_ws281x)
CFLAGS += -I$(RPI_WS281X_DIR)
PIXCNT ?= 60
COMMON_LIBS += -L$(RPI_WS281X_DIR) -lws2811 -lwiringPi
endif

CFLAGS += -DPIXCNT=$(PIXCNT)

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
DAEMON_OBJS := $(DAEMON_SRCS:%.c=%.o)
DAEMON_LIBS := $(COMMON_LIBS) -lmosquitto -ljsmn

# Platform configuration for specific targets

ifeq ($(PLATFORM),Windows)
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
# FIXME Add Windows files for this program in particular
lsd-res.o: ../../ledsign/lsd/lsd.rc ../../ledsign/lsd/LEDsign.ico
	windres -I$(SIGND_DIR)/daemon $< $@
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
