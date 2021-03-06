#include "sockets.h"
#include "pool.h"
#include "confuse.h"
#include <signal.h>
#include <syslog.h>
#include <unistd.h>

#define MAX_BUF 1000
/*Manejador vacío para SIGINT y SIGUSR1.*/
void manejador(int sig){}

/*
* main
* Descripcion: Funcion principal del servidor web.
* Argumentos:
*   - int argc: numero de args
*   - char const *argv[]: argumentos
* Retorno: int 0 si todo es correct, 1 en caso de error
*/
int main(int argc, char const *argv[]) {
  int sockval;
  pool_thread * pool;
  struct sigaction act;
  sigset_t set, old_set;

  static char *server_root = NULL;
  static long int max_clients = 0;
  static long int listen_port = 0;
  static char *server_signature = NULL;
  static long int num_threads = 0;
  cfg_t *cfg;
  char cwd[MAX_BUF];
  char path_conf[MAX_BUF];
  char server_root_complete[MAX_BUF];

  /* Se halla el directorio de trabajo. */
  getcwd(cwd, sizeof(cwd));

  /* Lee el fichero de configuracion. */
  cfg_opt_t opts[] = {
    CFG_SIMPLE_STR("server_root", &server_root),
    CFG_SIMPLE_INT("max_clients", &max_clients),
    CFG_SIMPLE_INT("listen_port", &listen_port),
    CFG_SIMPLE_STR("server_signature", &server_signature),
    CFG_SIMPLE_INT("num_threads", &num_threads),
    CFG_END()
  };

  cfg = cfg_init(opts, 0);
  /* Se obtiene la ruta a server.conf. */
  sprintf(path_conf,"%s/server.conf",cwd);
  if(cfg_parse(cfg, path_conf) == CFG_PARSE_ERROR)
  {
    syslog(LOG_ERR, "Error parsing server.conf");
    return 1;
  }
  cfg_free(cfg);
  /* Se obtiene la ruta completa al server. */
  sprintf(server_root_complete, "%s%s", cwd, server_root);

  /*Configuramos de inicio syslog */
  openlog(server_signature, 0, 0);
  syslog(LOG_INFO, "Server_root: %s, server_signature: %s, max_clients: %ld, listen_ports: %ld, num_threads: %ld",
		  server_root_complete, server_signature, max_clients, listen_port, num_threads);

  /* Manejo de señales. Se bloquea SIGINT a todos los hilos y se atrapa SIGUSR1.*/
  /* Levanta el manejador vacio. */
  act.sa_handler = manejador;
  sigemptyset(&(act.sa_mask));
  act.sa_flags = 0;

  /* Inicializa el conjunto de señales al conjunto vacio. */
  sigemptyset(&set);
  /* Aniade SIGINT. */
  sigaddset(&set, SIGTERM);
  /* Bloqueo de las seniales del set. */
  pthread_sigmask(SIG_SETMASK, &set, &old_set);
  /* Asigna el manejador de SIGINT.*/
  if (sigaction(SIGTERM, &act, NULL) < 0) {
    syslog(LOG_ERR, "Error sigaction");
    return 1;
  }
  /* Asigna el manejador de SIGUSR1. */
  if (sigaction(SIGUSR1, &act, NULL) < 0) {
    syslog(LOG_ERR, "Error sigaction");
    return 1;
  }
  /* La senial SIGPIPE es ignorada. */
  signal(SIGPIPE, SIG_IGN);

  /* Se abre el socket. */
  sockval = socket_server_ini(listen_port, max_clients);
  if(sockval == -1){
    syslog(LOG_ERR, "Error socket ini");
    free(server_root);
    free(server_signature);
    return 1;
  }
  /* Se crean los hilos del pool estatico. */
  pool = pool_create(sockval, server_signature, server_root_complete, num_threads);
  if (pool == NULL) {
    syslog(LOG_ERR, "Error pool create");
    free(server_root);
    free(server_signature);
    close(sockval);
    return 1;
  }
  /* El servidor se suspende hasta que le llega SIGINT. */
  sigsuspend(&old_set);

  /* Se liberan los recursos. */
  pool_free(pool);
  free(server_root);
  free(server_signature);
  close(sockval);
  return 0;
}
