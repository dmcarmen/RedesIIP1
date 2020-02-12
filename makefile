CC = gcc
CFLAGS = -g -Wall -pedantic -I includes/
SRC = src/
SRCLIB = srclib/
LIBS = srclib/sockets.c srclib/pool.c
EJS =  main

all: $(EJS)

clean:
	rm -rf *.o *.dot $(EJS)

main: $(LIBS)
	$(CC) $(CFLAGS) main.c $(LIBS) -o bin/main -lpthread
