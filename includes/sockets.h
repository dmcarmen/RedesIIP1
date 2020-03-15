#ifndef SOCKETS_H
#define SOCKETS_H

#include <netinet/in.h>
#include <syslog.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int socket_server_ini(int listen_port, int max_clients);
int socket_accept(int sockval);

#endif
