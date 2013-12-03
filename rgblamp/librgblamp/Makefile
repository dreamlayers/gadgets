LIBNAME := rgblamp
LIBINCLUDE := librgblamp.h
SRCS := librgblamp.c
LIBS := -lm
LIBRUBY := rgblamp.rb
MAKEINCLUDE := ../../build/Makefile.devlib

include $(MAKEINCLUDE)

librgblamp.o: librgblamp.c $(SERIO_DIR)/serio.h $(LIBINCLUDE) \
              Makefile $(MAKEINCLUDE)
