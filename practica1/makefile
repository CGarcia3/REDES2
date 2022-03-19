CC = gcc
CFLAGS = -g -Wall -pedantic -I includes/

EJS =  server

SRC = ./src
SRCLIB = ./srclib
INCL = ./includes
OBJ = ./obj
LIB = ./lib

all: clean dirs $(EJS)

clean:
	rm -rf $(EJS)
	rm -rf $(LIB)
	rm -rf $(OBJ)

server: sockets.a picohttpparser.a $(SRC)/http.c
	$(CC) $(CFLAGS) $(SRC)/server.c $(SRC)/http.c -o server $(LIB)/sockets.a $(LIB)/picohttpparser.a -lpthread -lconfuse

dirs:
	mkdir -p $(LIB)
	mkdir -p $(OBJ)

sockets.o: $(SRCLIB)/sockets.c $(INCL)/sockets.h
	$(CC) $(CFLAGS) -c -o $(OBJ)/sockets.o $(SRCLIB)/sockets.c

picohttpparser.o: $(SRCLIB)/picohttpparser.c $(INCL)/picohttpparser.h
	$(CC) $(CFLAGS) -c -o $(OBJ)/picohttpparser.o $(SRCLIB)/picohttpparser.c

sockets.a: sockets.o
	ar rcs $(LIB)/sockets.a $(OBJ)/sockets.o

picohttpparser.a: picohttpparser.o
	ar rcs $(LIB)/picohttpparser.a $(OBJ)/picohttpparser.o