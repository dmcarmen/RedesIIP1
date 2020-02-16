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
  struct sigaction act;

  act.sa_handler = manejador;
  sigemptyset(&(act.sa_mask));
  act.sa_flags = 0;

  if (sigaction(SIGINT, &act, NULL) < 0) {
    syslog(LOG_ERR, "Error sigaction");
    return 0;
  }

  sockval = socket_server_ini();

  pool = pool_create(sockval);

  while(flag);

  pool_free(pool);
  close(sockval);
  return 0;
}
