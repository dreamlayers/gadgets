CC=g++
CFLAGS=-fPIC -Wall -c
LDFLAGS=-shared -Wl,--no-as-needed -Wl,--no-undefined
OUTLIB=rgblamp.vis
PREFIX=/usr

all: ${OUTLIB}

$(OUTLIB): addon.o ../rgbm.o ../../ws281xproxy/ws281x_rgb.o
	$(CC) $(LDFLAGS) -o $@ $^ -lrt

addon.o: addon.cpp
	$(CC) $(CFLAGS) addon.cpp

clean:
	rm -rf *.o $(OUTLIB)

install: all
	mkdir -p $(PREFIX)/share/kodi/addons/visualization.rgblamp
	install -m 644 $(OUTLIB) $(PREFIX)/share/kodi/addons/visualization.rgblamp
	install -m 644 icon.png $(PREFIX)/share/kodi/addons/visualization.rgblamp
	install -m 644 addon.xml $(PREFIX)/share/kodi/addons/visualization.rgblamp
