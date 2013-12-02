include build/Makefile.config

SUBDIRS := serio \
           signd/client signd/daemon \
           ledsign/lsd ledsign/libledsign \
           vfd/libvfd vfd/vfd_plugin vfd/vfdd \
           rgblamp/librgblamp

ifeq ($(PLATFORM),Cygwin)
else
SUBDIRS := $(SUBDIRS) rgblamp/aud_rgb
endif

CLEANDIRS := $(SUBDIRS:%=clean-%)
INSTALLDIRS := $(SUBDIRS:%=install-%)

.PHONY : all $(SUBDIRS) $(CLEANDIRS) $(INSTALLDIRS)

all: $(SUBDIRS)

# Dependencies between targets

ledsign/libledsign: serio
ledsign/lsd: ledsign/libledsign \
             signd/daemon signd/client

vfd/libvfd: serio
vfd/vfdd: vfd/libvfd signd/daemon signd/client
vfd/vfd_plugin: vfd/libvfd

rgblamp/librgblamp: serio
rgblamp/aud_rgb: rgblamp/librgblamp

# Rules

$(SUBDIRS):
	$(MAKE) -C $@

install: $(INSTALLDIRS) all
$(INSTALLDIRS):
	$(MAKE) -C $(@:install-%=%) install

clean: $(CLEANDIRS)
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean
