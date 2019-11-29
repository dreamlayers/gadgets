include ../../build/Makefile.config

SIGND_DIR := ../../signd
LIBRGBLAMP_DIR := ../librgblamp
RPI_WS281X_DIR := /home/pi/gitapps/rpi_ws281x
CFLAGS := $(CFLAGS) -g -I$(SIGND_DIR)/daemon
CC := gcc
LIBS := -lm -lmosquitto -ljsmn
EXTRA_DEPS :=

ifeq ($(PLATFORM),Windows)
FIXME
else
LIBS := $(LIBS) -lpthread
endif

ifeq ($(DRIVER),)
ifeq ($(PLATFORM),Windows)
DRIVER := rgblamp
else ifneq ("$(wildcard /usr/include/wiringPi.h)","")
DRIVER := rpi_ws281x
endif
endif

ifeq ($(DRIVER),rgblamp)
CFLAGS += -I$(LIBRGBLAMP_DIR)
PIXCNT := 1
EXTRA_DEPS += $(LIBRGBLAMP_DIR)/librgblamp.a
LIBS += $(LIBRGBLAMP_DIR)/librgblamp.a

else ifeq ($(DRIVER),rpi_ws281x)
CFLAGS += -I$(RPI_WS281X_DIR)
PIXCNT ?= 60
LIBS += -L$(RPI_WS281X_DIR) -lws2811 -lwiringPi

endif

ifneq ($(PIXCNT),)
CFLAGS += -DPIXCNT=$(PIXCNT)
endif

#.PHONY : install uninstall all dummy

SRCS := coloranim.c parser.c drv_$(DRIVER).c mqtt.c json.c rgbcmds.c
TARGET := coloranim$(EXESUFFIX)
OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(EXTRA_DEPS) $(SIGND_DIR)/daemon/signd.a
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET)

coloranim.o: coloranim.c coloranim.h Makefile
parser.o: parser.c coloranim.h Makefile
drv_rgblamp.o: $(LIBRGBLAMP_DIR)/librgblamp.h \
               coloranim.h Makefile
drv_$(DRIVER).o: drv_$(DRIVER).c coloranim.h Makefile
mqtt.o: mqtt.c coloranim.h Makefile
json.o: json.c coloranim.h Makefile
rgbcmds.o: rgbcmds.c coloranim.h \
           $(SIGND_DIR)/daemon/signd.h $(SIGND_DIR)/sdprotocol.h Makefile
