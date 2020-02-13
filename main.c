#include "sockets.h"
#include "pool.h"
#include <signal.h>

int flag=1;

void manejador(int sig) {
  flag=0;
}

int main(int argc, char const *argv[]) {
  int sockval;
  pool_thread * pool;

  signal(SIGINT, manejador);

  sockval = socket_server_ini();

  pool = pool_create(sockval);

  while(flag);
  
  pool_free(pool);
  return 0;
}
