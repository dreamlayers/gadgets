include ../../build/Makefile.config

LIBSIGNTCP_DIR := ../lsd
SRCS := gen_led.c
WINAMPAPI_DIR := ../../pluginapi/winamp
CFLAGS := $(CFLAGS) -I$(LIBSIGNTCP_DIR) -I$(WINAMPAPI_DIR)

TARGET := gen_led.dll

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(LIBSIGNTCP_DIR)/libsigntcp.a
	$(CC) $(CFLAGS) -shared -o $@ $^ -lwsock32

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) $(CLEAN_EXTRA) *~ *.bak

gen_led.o: gen_led.c $(WINAMPAPI_DIR)/gen.h \
           $(LIBSIGNTCP_DIR)/libsigntcp.h
