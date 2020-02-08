#include "sockets.h"

int socket_server_ini()
{
  int sockval;
  struct sockaddr_in addr;

  syslog (LOG_INFO, "Creating socket");
  if ((sockval = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    syslog(LOG_ERR, "Error creating socket");
    exit(EXIT_FAILURE);
  }
  addr.sin_family = AF_INET;                /* TCP/IP family */
  addr.sin_port = htons(SERVER_PORT);   /* Asigning port */
  addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Accept all adresses */
  bzero((void *) &(addr.sin_zero), 8);

  syslog (LOG_INFO, "Binding socket");
  if (bind(sockval, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    syslog(LOG_ERR, "Error binding socket");
    exit(EXIT_FAILURE);
  }

  syslog (LOG_INFO, "Listening connections");
  if (listen(sockval, MAX_CONNECTIONS) < 0){
    syslog(LOG_ERR, "Error listenining");
    exit(EXIT_FAILURE);
  }
  return sockval;
}

void socket_accept(int sockval)
{
  int connval, len;
  struct sockaddr addr;

  len = sizeof(addr);
  if ((connval = accept(sockval, (struct sockaddr *)&addr, (socklen_t*)&len)) < 0){
    syslog(LOG_ERR, "Error accepting connection");
    exit(EXIT_FAILURE);
  }
  syslog (LOG_INFO, "Server accepted the client");
  //launch_service(connval);
  //wait_finished_services();
  close(sockval);
}
