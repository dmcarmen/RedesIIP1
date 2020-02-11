#include "sockets.h"

int main()
{
  int sockval;

  sockval = socket_server_ini();
  socket_accept(sockval);
  close(sockval);
  return 0;
}
