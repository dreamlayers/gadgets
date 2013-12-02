include ../../build/Makefile.config

LIBRGBLAMP_DIR := ../librgblamp
WINAMPAPI_DIR := ../../pluginapi/winamp
SRCS := rgbvis.c
CFLAGS := $(CFLAGS) -I$(LIBRGBLAMP_DIR) -I$(WINAMPAPI_DIR)

TARGET := vis_rgb.dll
CFLAGS := $(CFLAGS)

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(LIBRGBLAMP_DIR)/librgblamp.a
	$(CC) $(CFLAGS) -shared -o $@ $^

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

rgbvis.o: rgbvis.c $(WINAMPAPI_DIR)/vis.h \
          $(LIBRGBLAMP_DIR)/librgblamp.h Makefile
