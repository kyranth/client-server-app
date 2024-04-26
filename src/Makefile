# Compiler gcc for C program
CC=gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS=-g -Wall -std=c99

all: client server

client: client.c
	$(CC) $(CFLAGS) -o ../bin/client client.c

server: server.c
	$(CC) $(CFLAGS) -o ../bin/server server.c

clean:
	rm -f ../bin/client ../bin/server