# Compiler gcc for C program
CC=gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS=-g -Wall -std=c99

TARGET=app
OBJECTS=cJSON.o

lib=lib
src=src

$(TARGET): $(OBJECTS) client server standalone

cJSON.o: $(lib)/cJSON.c $(lib)/cJSON.h
	$(CC) $(CFLAGS) -c $(lib)/cJSON.c

client: $(src)/client.c $(OBJECTS)
	$(CC) $(CFLAGS) -o client $(src)/client.c $(OBJECTS)

server: $(src)/server.c $(OBJECTS)
	$(CC) $(CFLAGS) -o server $(src)/server.c $(OBJECTS)

standalone: $(src)/standalone.c $(OBJECTS)
	$(CC) $(CFLAGS) -o standalone $(src)/standalone.c $(OBJECTS)

clean:
	rm -f *.o *.so client server standalone recv_config.json

run:
	./client config.json
