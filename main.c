#include "sockets.h"
#include "pool.h"

int main(int argc, char const *argv[]) {
  int sockval;
  pool_thread * pool;

  sockval = socket_server_ini();

  pool = pool_create(sockval);

  pool_free(pool);
  return 0;
}
