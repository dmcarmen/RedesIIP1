//gcc -Wextra -Wall -g -I../includes -c sockets.c
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <syslog.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define SERVER_PORT 8080
#define MAX_CONNECTIONS 2

int socket_server_ini();
void socket_accept();
int socket_send();
int socket_recv();
