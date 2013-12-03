include ../../build/Makefile.config

LIBSIGNTCP_DIR := ../lsd
SRCS := lsp.c
CFLAGS := $(CFLAGS) -I$(LIBSIGNTCP_DIR)

ifeq ($(PLATFORM),Cygwin)
# Allow cgic in subfolder
CFLAGS := $(CFLAGS) -Icgic205 -Lcgic205
endif

TARGET := lsp

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(LIBSIGNTCP_DIR)/libsigntcp.a
	$(CC) $(CFLAGS) -o $@ $^ -lcgic -lwsock32

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) $(CLEAN_EXTRA) *~ *.bak

gen_led.o: gen_led.c $(LIBSIGNTCP_DIR)/libsigntcp.h Makefile
