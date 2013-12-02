LIBNAME := libvfd
LIBINCLUDE := vfd.h
SRCS := vfd.c
LIBS :=
LIBRUBY := vfd.rb
MAKEINCLUDE := ../../build/Makefile.devlib

include $(MAKEINCLUDE)

vfd_test: $(SRCS) $(SERIO_DIR)/libserio.a Makefile
	$(CC) -DTESTING -I$(SERIO_DIR) -g -Wall $(SRCS) $(SERIO_DIR)/libserio.a -o $@

vfd.o: vfd.c $(SERIO_DIR)/serio.h $(LIBINCLUDE) 14.h Makefile $(MAKEINCLUDE)
