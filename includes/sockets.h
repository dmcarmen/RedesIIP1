#ifndef SOCKETS_H
#define SOCKETS_H

/*
* Funcion que crea un socket TCP/IP, lo liga al puerto y lo deja escuchando.
*/
int socket_server_ini(int listen_port, int max_clients);

/*
* Funcion que acepta la conexion de un cliente.
*/
int socket_accept(int sockval);

#endif
