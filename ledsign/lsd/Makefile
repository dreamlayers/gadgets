SIGND_DIR := ../../signd/daemon
LIBLEDSIGN_DIR := ../libledsign
SRCS := signcmds.c
CFLAGS := -I$(SIGND_DIR) -I$(LIBLEDSIGN_DIR) -O -Wall -Wmissing-prototypes -g

TARGET := lsd

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS) $(SIGND_DIR)/signd.a $(LIBLEDSIGN_DIR)/libledsign.a
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

