include ../../build/Makefile.config

LIBRGBLAMP_DIR := ../librgblamp
LIBRGBLAMP := $(LIBRGBLAMP_DIR)/librgblamp.a
CFLAGS := $(CFLAGS) -I$(LIBRGBLAMP_DIR) -DSTANDALONE_WRGBC -mwindows
SRCS := winrgbchoose.c

TARGET := winrgbchoose.exe

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(LIBRGBLAMP)
	$(CC) $(CFLAGS) -o $@ $^ -lcomdlg32

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) $(CLEAN_EXTRA) *~ *.bak

winrgbchoose.o: winrgbchoose.c $(LIBRGBLAMP_DIR)/librgblamp.h Makefile
