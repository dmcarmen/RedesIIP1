#ifndef SOCKETS_H
#define SOCKETS_H

int socket_server_ini(int listen_port, int max_clients);
int socket_accept(int sockval);

#endif
