include ../../build/Makefile.config

SIGND_DIR := ../../signd
LIBLEDSIGN_DIR := ../libledsign
CFLAGS := $(CFLAGS) -I$(SIGND_DIR)/daemon -I$(SIGND_DIR)/client \
          -I$(LIBLEDSIGN_DIR)
OBJS := signcmds.o
TARGET := lsd$(EXESUFFIX)

all: $(TARGET) libsigntcp.a

ifeq ($(PLATFORM),Windows)

LIBS := ../libledsign/gditext.c -lwsock32 -lgdi32
CFLAGS := $(CFLAGS) -mwindows -I../../serio

lsd-res.o: lsd.rc LEDsign.ico
	$(WINDRES) -I$(SIGND_DIR)/daemon $< $@
OBJS := $(OBJS) lsd-res.o

else
LIBS := -lpthread
endif

$(TARGET): $(OBJS) $(SIGND_DIR)/daemon/signd.a $(LIBLEDSIGN_DIR)/libledsign.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

libsigntcp.a: libsigntcp.o $(SIGND_DIR)/client/libsdclient.a
	cp $(SIGND_DIR)/client/libsdclient.a $@
	ar rs $@ $<

.PHONY : clean
clean:
	rm -f $(OBJS) libsigntcp.o libsigntcp.a $(TARGET) font2c$(EXESUFFIX) 6x7font.h *~ *.bak

font2c$(EXESUFFIX): font2c.c $(LIBLEDSIGN_DIR)/ledsign.h Makefile
	$(CC) $(CFLAGS) -o $@ $< $(LIBLEDSIGN_DIR)/libledsign.a

6x7font.h: 6x7.rawfont font2c$(EXESUFFIX)
	./font2c$(EXESUFFIX) $< > $@.tmp
	mv -f $@.tmp $@

libsigntcp.o: libsigntcp.c libsigntcp.h $(SIGND_DIR)/client/sdclient.h \
			  $(SIGND_DIR)/sdprotocol.h Makefile

signcmds.o: signcmds.c 6x7font.h $(LIBLEDSIGN_DIR)/ledsign.h \
	    $(SIGND_DIR)/daemon/signd.h $(SIGND_DIR)/sdprotocol.h Makefile
