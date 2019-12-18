include ../../build/Makefile.config

LIBSIGNTCP_DIR := ../lsd
SRCS := lsp.c
CFLAGS := $(CFLAGS) -I$(LIBSIGNTCP_DIR) -fvisibility=default
LIBS := -lcgic

ifeq ($(PLATFORM),Windows)
# Allow cgic in subfolder
CFLAGS := $(CFLAGS) -Icgic205 -Lcgic205
LIBS := $(LIBS) -lwsock32
endif

TARGET := lsp

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(LIBSIGNTCP_DIR)/libsigntcp.a
	-$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) $(CLEAN_EXTRA) *~ *.bak

lsp.o: lsp.c $(LIBSIGNTCP_DIR)/libsigntcp.h Makefile
	-$(CC) -c $(CFLAGS) -o $@ $<
