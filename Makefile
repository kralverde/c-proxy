CC=gcc
CFLAGS=-Wall -Iincludes -Wextra -std=c99 -ggdb
LDLIBS=-L/usr/lib
VPATH=src

all: client server

client: client.o argparse.o sha256.o
server: server.o argparse.o sha256.o

clean:
	rm -rf *~ *.o client server client-test server-test
