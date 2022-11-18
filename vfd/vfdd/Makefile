include ../../build/Makefile.config

SIGND_DIR := ../../signd
LIBVFD_DIR := ../libvfd
CFLAGS := $(CFLAGS) -I$(SIGND_DIR)/daemon -I$(LIBVFD_DIR)

TARGET := vfdd$(EXESUFFIX)
OBJS := vfdcmds.o

all: vfdd
# libsigntcp.a

ifeq ($(PLATFORM),Windows)

LIBS := -lwsock32 -lgdi32
CFLAGS := $(CFLAGS) -mwindows

vfdd-res.o: vfdd.rc
# LEDsign.ico
	$(WINDRES) -I$(SIGND_DIR)/daemon $< $@
OBJS := $(OBJS) vfdd-res.o

else
LIBS := -lpthread
endif

vfdd: $(OBJS) $(SIGND_DIR)/daemon/signd.a $(LIBVFD_DIR)/libvfd.a
	$(CC) $(CFLAGS) -o $@ $(OBJS) -Wl,--whole-archive $(SIGND_DIR)/daemon/signd.a -Wl,--no-whole-archive $(LIBVFD_DIR)/libvfd.a $(LIBS)

#libsigntcp.a: libsigntcp.o $(SIGND_DIR)/client/libsdclient.a
#	cp $(SIGND_DIR)/client/libsdclient.a $@
#	ar rs $@ $(OBJS)

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

vfdcmds.o: vfdcmds.c ../libvfd/vfd.h ../../signd/daemon/signd.h \
           ../../signd/daemon/../sdprotocol.h Makefile
