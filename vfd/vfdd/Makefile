include ../../build/Makefile.config

SIGND_DIR := ../../signd
LIBVFD_DIR := ../libvfd
CFLAGS := $(CFLAGS) -I$(SIGND_DIR)/daemon -I$(LIBVFD_DIR)

TARGET := vfdd
OBJS := vfdcmds.o

all: vfdd
# libsigntcp.a

ifeq ($(PLATFORM),Windows)

LIBS := -lwsock32 -lgdi32
CFLAGS := $(CFLAGS) -mwindows

vfdd-res.o: vfdd.rc
# LEDsign.ico
	windres -I$(SIGND_DIR)/daemon $< $@
OBJS := $(OBJS) vfdd-res.o

else
LIBS := -lpthread
endif

vfdd: $(OBJS) $(SIGND_DIR)/daemon/signd.a $(LIBVFD_DIR)/libvfd.a
	$(CC) $(CFLAGS) -Wl,-u,_WinMain@16 -o $@ $^ $(LIBS)

#libsigntcp.a: libsigntcp.o $(SIGND_DIR)/client/libsdclient.a
#	cp $(SIGND_DIR)/client/libsdclient.a $@
#	ar rs $@ $(OBJS)

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

vfdcmds.o: vfdcmds.c ../libvfd/vfd.h ../../signd/daemon/signd.h \
           ../../signd/daemon/../sdprotocol.h Makefile
