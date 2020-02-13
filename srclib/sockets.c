#include "sockets.h"

/*
* Funcion que crea un socket TCP/IP, lo liga al puerto SERVER_PORT y lo
* deja escuchando.
*/
int socket_server_ini()
{
  int sockval;
  struct sockaddr_in addr;

  /* Se crea el socket*/
  syslog (LOG_INFO, "Creating socket");
  if ((sockval = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    syslog(LOG_ERR, "Error creating socket");
    exit(EXIT_FAILURE);
  }

  /* Se inicializa la estructura addr*/
  addr.sin_family = AF_INET;                /* TCP/IP family */
  addr.sin_port = htons(SERVER_PORT);   /* Asigning port */
  addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Accept all adresses */
  bzero((void *) &(addr.sin_zero), 8);

  /*int yes = 1;
  if (setsockopt(sockval, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  if (setsockopt(sockval, SOL_SOCKET, SO_REUSEPORT, (const char*)&yes, sizeof(yes)) < 0){
    perror("setsockopt(SO_REUSEPORT) failed");
    exit(EXIT_FAILURE);
  }*/

  /* Se liga el puerto al socket */
  syslog (LOG_INFO, "Binding socket");
  if (bind(sockval, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    syslog(LOG_ERR, "Error binding socket");
    exit(EXIT_FAILURE);
  }

  /* Se comienza a escuchar */
  syslog (LOG_INFO, "Listening connections");
  if (listen(sockval, MAX_CONNECTIONS) < 0){
    syslog(LOG_ERR, "Error listenining");
    exit(EXIT_FAILURE);
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
  connval = accept(sockval, (struct sockaddr *)&addr, (socklen_t*)&len);
  if(connval == -1){
      if(errno == EINTR){
        return connval;
      }
      syslog(LOG_ERR, "Error accepting connection");
      exit(EXIT_FAILURE);
  }
  syslog (LOG_INFO, "Server accepted the client");
  return connval;
}
