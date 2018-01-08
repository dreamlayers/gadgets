include ../../build/Makefile.config

LIBRGBLAMP_DIR := ../librgblamp
CFLAGS := $(CFLAGS)
CC := gcc
LIBS := -lm
EXTRA_DEPS :=

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
CFLAGS += -I/home/pi/gitapps/rpi_ws281x
PIXCNT ?= 60
LIBS += -lws2811 -lwiringPi

endif

ifneq ($(PIXCNT),)
CFLAGS += -DPIXCNT=$(PIXCNT)
endif

#.PHONY : install uninstall all dummy

SRCS := coloranim.c parser.c drv_$(DRIVER).c
TARGET := coloranim$(EXESUFFIX)
OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(EXTRA_DEPS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET)

coloranim.o: coloranim.c coloranim.h Makefile
parser.o: parser.c coloranim.h Makefile
drv_rgblamp.o: $(LIBRGBLAMP_DIR)/librgblamp.h \
               coloranim.h Makefile
drv_$(DRIVER).o: drv_$(DRIVER).c coloranim.h Makefile
