#include "sockets.h"
#include <netinet/in.h>
#include <syslog.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

/*
* Funcion que crea un socket TCP/IP, lo liga al puerto y lo deja escuchando.
* Recibe como parametros el puerto y numero maximo de clientes que pueden
* estar en la cola. Devuelve el descriptor del socket. En caso de error
* devuelve -1.
*/
int socket_server_ini(int listen_port, int max_clients)
{
  int sockval;
  struct sockaddr_in addr;
  int option=1;

  /* Se crea el socket. */
  syslog(LOG_INFO, "Creating socket");
  if ((sockval = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    syslog(LOG_ERR, "Error creating socket");
    return -1;
  }

  /* Se inicializa la estructura addr. */
  addr.sin_family = AF_INET;                /* Familia TCP/IP. */
  addr.sin_port = htons(listen_port);   /* Se asigna el puerto. */
  addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Se aceptan todas las direcciones. */
  bzero((void *) &(addr.sin_zero), 8);

  /* Se puede reusar el socket. */
  if(setsockopt(sockval,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR), (char*)&option,sizeof(option)) < 0) {
    syslog(LOG_ERR, "Error setsockopt failed");
    close(sockval);
    return -1;
  }

  /* Se liga el puerto al socket. */
  syslog (LOG_INFO, "Binding socket");
  if (bind(sockval, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    syslog(LOG_ERR, "Error binding socket");
    close(sockval);
    return -1;
  }

  /* Se marca el socket como pasivo, indicando el numero maximo de peticiones
  * pendientes. */
  syslog (LOG_INFO, "Listening connections");
  if (listen(sockval, max_clients) < 0){
    syslog(LOG_ERR, "Error listenining");
    close(sockval);
    return -1;
  }

  return sockval;
}

/*
* Funcion que acepta la conexion de un cliente.
*/
int socket_accept(int sockval)
{
  int connval, len;
  struct sockaddr addr;

  len = sizeof(addr);

  /* Se acepta una conexion. */
  connval = accept(sockval, (struct sockaddr *)&addr, (socklen_t*)&len);
  if(connval == -1){
      /* Si ha salido del accept por una interrupcion, se devuelve -2. */
      if(errno == EINTR){
        return -2;
      }
      /* Si se produce un error que no sea por una señal, se devuelve -1. */
      syslog(LOG_ERR, "Error accepting connection");
      return -1;
  }

  syslog (LOG_INFO, "Server accepted the client");
  return connval;
}
