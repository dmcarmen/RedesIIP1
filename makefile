CC = gcc
CFLAGS = -g -Wall -pedantic -I includes/
SRC = src/
SRCLIB = srclib/
LIBS = srclib/sockets.c srclib/pool.c srclib/picohttpparser.c
SRCS = src/http.c
EJS =  main

all: $(EJS)

clean:
	rm -rf *.o *.dot $(EJS)

main: $(LIBS)
	$(CC) $(CFLAGS) main.c $(LIBS) $(SRCS) -o bin/main -lpthread
