include ../../build/Makefile.config

SIGND_DIR := ../../signd
LIBLEDSIGN_DIR := ../libledsign
CFLAGS := $(CFLAGS) -I$(SIGND_DIR)/daemon -I$(SIGND_DIR)/client \
          -I$(LIBLEDSIGN_DIR)

all: lsd libsigntcp.a

ifeq ($(PLATFORM),Cygwin)

LIBS := ../libledsign/gditext.c -lwsock32 -lgdi32
CFLAGS := $(CFLAGS) -mwindows -I../../serio

lsd-res.o: lsd.rc LEDsign.ico
	windres -I$(SIGND_DIR)/daemon $< $@
lsd: lsd-res.o

else
LIBS := -lpthread
endif

lsd: signcmds.o $(SIGND_DIR)/daemon/signd.a $(LIBLEDSIGN_DIR)/libledsign.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

libsigntcp.a: libsigntcp.o $(SIGND_DIR)/client/libsdclient.a
	cp $(SIGND_DIR)/client/libsdclient.a $@
	ar rs $@ $(OBJS)

.PHONY : clean
clean:
	rm -f $(OBJS) libsigntcp.o signcmds.o $(TARGET) *~ *.bak

libsigntcp.o: libsigntcp.c libsigntcp.h $(SIGND_DIR)/client/sdclient.h \
			  $(SIGND_DIR)/sdprotocol.h Makefile

signcmds.o: signcmds.c $(LIBLEDSIGN_DIR)/ledsign.h $(SIGND_DIR)/daemon/signd.h \
            $(SIGND_DIR)/sdprotocol.h Makefile
