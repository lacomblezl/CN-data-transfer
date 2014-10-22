CC=gcc
CFLAGS=-Wall -Werror -O3
LDFLAGS=
OBJ = $(wildcard *.o)

# First executable
SEND=clientMod
SEND_SRC=rtp.c clientMod.c
SEND_OBJ= $(SEND_SRC:.c=.o)

# Second executable
RECV=serverMod
RECV_SRC=rtp.c serverMod.c
RECV_OBJ= $(RECV_SRC:.c=.o)

all: $(SEND) $(RECV)

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS)  -c -o $@ $<

$(SEND): $(SEND_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(RECV): $(RECV_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm $(OBJ)
