#ifndef SOCKETS_H
#define SOCKETS_H

/*
* Funcion que crea un socket TCP/IP, lo liga al puerto y lo deja escuchando.
* Recibe como parametros el puerto y numero maximo de clientes que pueden
* estar en la cola. Devuelve el descriptor del socket. En caso de error
* devuelve -1.
*/
int socket_server_ini(int listen_port, int max_clients);

/*
* Funcion que acepta la conexion de un cliente.
*/
int socket_accept(int sockval);

#endif
