# Compiler gcc for C program
CC=gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS=-g -Wall -std=c99

lib=lib
src=src

all: cJSON.o client

cJSON.o: $(lib)/cJSON.c $(lib)/cJSON.h
	$(CC) $(CFLAGS) -c $(lib)/cJSON.c

client: $(src)/client.c
	$(CC) $(CFLAGS) -o client $(src)/client.c cJSON.o

server: $(src)/server.c
	$(CC) $(CFLAGS) -o server $(src)/server.c

clean:
	rm -f *.o *.so client server

run:
	./client config.json
