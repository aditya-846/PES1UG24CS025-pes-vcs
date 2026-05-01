CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=c11 -D_POSIX_C_SOURCE=200809L -Wno-deprecated-declarations
LDFLAGS = -lssl -lcrypto

SRC     = object.c tree.c index.c commit.c
OBJ     = $(SRC:.c=.o)

.PHONY: all clean test-integration

all: pes test_objects test_tree

pes: pes.c $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_objects: test_objects.c object.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_tree: test_tree.c object.c tree.c index.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test-integration: pes
	bash test_sequence.sh

clean:
	rm -f pes test_objects test_tree *.o
	rm -rf .pes
