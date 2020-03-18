CC = gcc
CFLAGS = -g -Wall -pedantic -I includes/

EJS =  main

SRC = src
SRCLIB = ./srclib
INCL = ./includes
OBJ = ./obj
LIB = ./lib

all: dirs $(EJS)

clean:
	rm -rf $(EJS)
	rm -r $(LIB)
	rm -r $(OBJ)

main: sockets.a pool.a picohttpparser.a $(SRC)/http.c
	$(CC) $(CFLAGS) main.c $(SRC)/http.c -o main $(LIB)/sockets.a $(LIB)/pool.a $(LIB)/picohttpparser.a -lpthread -lconfuse

dirs:
	mkdir -p $(LIB)
	mkdir -p $(OBJ)

sockets.o: $(SRCLIB)/sockets.c $(INCL)/sockets.h
	$(CC) $(CFLAGS) -c -o $(OBJ)/sockets.o $(SRCLIB)/sockets.c

pool.o: $(SRCLIB)/pool.c $(INCL)/pool.h
	$(CC) $(CFLAGS) -c -o $(OBJ)/pool.o $(SRCLIB)/pool.c

picohttpparser.o: $(SRCLIB)/picohttpparser.c $(INCL)/picohttpparser.h
	$(CC) $(CFLAGS) -c -o $(OBJ)/picohttpparser.o $(SRCLIB)/picohttpparser.c

sockets.a: sockets.o
	ar rcs $(LIB)/sockets.a $(OBJ)/sockets.o

pool.a: pool.o
	ar rcs $(LIB)/pool.a $(OBJ)/pool.o

picohttpparser.a: picohttpparser.o
	ar rcs $(LIB)/picohttpparser.a $(OBJ)/picohttpparser.o
