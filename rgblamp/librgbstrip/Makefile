LIBNAME := rgbstrip
LIBRGBLAMP_DIR := ../librgblamp
LIBINCLUDE := $(LIBRGBLAMP_DIR)/librgblamp.h
SRCS := librgbstrip.c
LIBS := -lm
MAKEINCLUDE := ../../build/Makefile.devlib

include $(MAKEINCLUDE)

CFLAGS := $(CFLAGS) -I$(LIBRGBLAMP_DIR)

librgbstrip.o: librgbstrip.c $(SERIO_DIR)/serio.h $(LIBINCLUDE) \
              Makefile $(MAKEINCLUDE)
