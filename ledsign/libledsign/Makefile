LIBNAME := ledsign
LIBINCLUDE := ledsign.h
SRCS := ledsign.c
LIBS :=
LIBRUBY := ledsign.rb
MAKEINCLUDE := ../../build/Makefile.devlib

include $(MAKEINCLUDE)

ledsign.o: ledsign.c $(SERIO_DIR)/serio.h $(LIBINCLUDE) Makefile $(MAKEINCLUDE)
