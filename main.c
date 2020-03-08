#include "sockets.h"
#include "pool.h"
#include <signal.h>
#include "confuse.h"

int flag=1;

void manejador(int sig) {
  flag=0;
}

int main(int argc, char const *argv[]) {
  int sockval;
  pool_thread * pool;
  struct sigaction act;

  static char *server_root = NULL;
  static long int max_clients = 0;
  static long int listen_port = 0;
  static char *server_signature = NULL;
  cfg_t *cfg;

  /* Leemos el fichero de configuracion. */
  cfg_opt_t opts[] = {
    CFG_SIMPLE_STR("server_root", &server_root),
    CFG_SIMPLE_INT("max_clients", &max_clients),
    CFG_SIMPLE_INT("listen_port", &listen_port),
    CFG_SIMPLE_STR("server_signature", &server_signature),
    CFG_END()
  };

  cfg = cfg_init(opts, 0);
  if(cfg_parse(cfg, "server.conf") == CFG_PARSE_ERROR) //cuidado dond corres donde esta
  {
    syslog(LOG_ERR, "Error parsing server.conf");
    return 1;
  }
  cfg_free(cfg);
  syslog(LOG_INFO, "Server_root: %s, server_signature: %s, max_clients: %ld, listen_ports: %ld",
		  server_root, server_signature, max_clients, listen_port); //

  /* Levantamos el manejador para poder cerrar el servidor con SIGINT. */
  act.sa_handler = manejador;
  sigemptyset(&(act.sa_mask));
  act.sa_flags = 0;

  if (sigaction(SIGINT, &act, NULL) < 0) {
    syslog(LOG_ERR, "Error sigaction");
    return 0;
  }

  /* Abrimos el socket y creamos los hilos del pool estatico. */
  sockval = socket_server_ini(listen_port, max_clients);
  pool = pool_create(sockval, server_signature, server_root);

  /* El servidor corre hasta que le llega SIGINT. */
  while(flag);

  /* Liberamos los recursos. */
  free(server_root);
  free(server_signature);
  pool_free(pool);
  close(sockval);
  return 0;
}
