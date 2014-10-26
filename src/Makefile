CC=gcc
CFLAGS=-Wall -Werror -O3
LDFLAGS=-lz
OBJ = $(wildcard *.o)

# First executable
SEND=sender
SEND_SRC=rtp.c sender.c
SEND_OBJ= $(SEND_SRC:.c=.o)

# Second executable
RECV=receiver
RECV_SRC=rtp.c receiver.c
RECV_OBJ= $(RECV_SRC:.c=.o)

#RECV=serverMod
#RECV_SRC=rtp.c serverMod.c
#RECV_OBJ= $(RECV_SRC:.c=.o)

all: $(SEND) $(RECV)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(SEND): $(SEND_OBJ)
	$(CC)  $(LDFLAGS) -o $@ $^

$(RECV): $(RECV_OBJ)
	$(CC)  $(LDFLAGS) -o $@ $^

clean:
	rm $(OBJ)
