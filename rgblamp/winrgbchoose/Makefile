include ../../build/Makefile.config

LIBRGBLAMP_DIR := ../librgblamp
LIBRGBLAMP := $(LIBRGBLAMP_DIR)/librgblamp.a
LIBRGBSTRIP := ../librgbstrip/librgbstrip.a

CFLAGS := $(CFLAGS) -I$(LIBRGBLAMP_DIR) -DSTANDALONE_WRGBC -mwindows
SRCS := winrgbchoose.c
OBJS := $(SRCS:%.c=%.o)

TARGET := winrgbchoose.exe
all: $(TARGET)
$(TARGET): $(OBJS) $(LIBRGBLAMP)
	$(CC) $(CFLAGS) -o $@ $^ -lcomdlg32

TARGET_STRIP := winrgbchoose-strip.exe
all: $(TARGET_STRIP)
$(TARGET_STRIP): $(OBJS) $(LIBRGBSTRIP)
	$(CC) $(CFLAGS) -o $@ $^ -lcomdlg32

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) $(CLEAN_EXTRA) *~ *.bak

winrgbchoose.o: winrgbchoose.c $(LIBRGBLAMP_DIR)/librgblamp.h Makefile
