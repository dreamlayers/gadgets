SRCS := serio.c
CFLAGS := -O -Wall -g -Wmissing-prototypes
TARGET := libserio.a

PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Cygwin)

CC := i686-w64-mingw32-gcc
SRCS := $(SRCS) serio_windows.c

else

SRCS := $(SRCS) serio_unix.c

endif

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS)
	ar rcs $@ $^

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

.PHONY : depend
depend:
	echo '# Automatically generated dependencies' > Makefile.dep
	gccmakedep -s '# Automatically generated dependencies' -fMakefile.dep -- $(CFLAGS) -- $(SRCS)

-include Makefile.dep
