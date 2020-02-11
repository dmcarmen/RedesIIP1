#ifndef SOCKETS_H
#define SOCKETS_H

//gcc -Wextra -Wall -g -o bin/main -I includes/ tests/test_sockets.c srclib/sockets.c
//curl http://localhost:8080/
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <syslog.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define SERVER_PORT 8080
#define MAX_CONNECTIONS 2

int socket_server_ini();
void socket_accept();

#endif