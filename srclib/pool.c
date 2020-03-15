#include "pool.h"

/*
* Función que corre cada hilo de la pool. Se encarga de aceptar la conexion
* procesar las peticiones HTTP que le lleguen y al terminar cerrar la conexion.
*/
void * thread_accept(void * pool){
  pool_thread *p = (pool_thread *)pool;
  int connval;

  while(p->stop == 0) {
    connval = socket_accept(p->sockval);
    /* Si connval es -2 no se ha realizado el accept por la interrupcion de una
    * senial. Es decir, el hilo tiene que terminar. */
    if(connval == -2) pthread_exit(NULL);
    /* Si connval es -1 se ha producido un error mientras estaba en el accept. */
    if(connval == -1) continue;

    /* Procesamos las peticiones HTTP y si devolvemos error cerramos la conexion. */
    if(process_petitions(connval, p->server_signature, p->server_root, &p->stop)==-1) {
      close(connval);
      pthread_exit(NULL);
    }
    syslog(LOG_INFO, "Conexion cerrada");
    close(connval);
  }
  pthread_exit(NULL);
}

/*
* Función que inicializa la pool (la estrcutura y los hilos).
*/
pool_thread * pool_create(int sockval, char* server_signature, char* server_root){
  pool_thread *pool;
  int i;

  /* Reservamos memoria para la estructura pool e inicializamos sus valores. */
  pool = (pool_thread *) malloc (sizeof(pool_thread));
  if(pool == NULL){
    syslog(LOG_ERR, "Error creating pool");
    return NULL;
  }
  pool->num_threads = NUM_THREADS;
  pool->stop = 0;
  pool->sockval = sockval;
  pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * pool->num_threads);
  if(pool->threads == NULL) {
    free(pool);
    syslog(LOG_ERR, "Error creating thread");
    return NULL;
  }
  pool->work_function = thread_accept;
  pool->server_signature = server_signature;
  pool->server_root = server_root;

  //TODO hay que hacer algo con los hilos creados si hay error, tal vez pool free comprobando hilos
  /* Creamos los hilos y cada uno llama a la work function. */
  for(i=0; i<pool->num_threads; i++){
    if(pthread_create(&(pool->threads[i]),NULL, pool->work_function,(void *) pool)!=0){
      free(pool->threads);
      free(pool);
      syslog(LOG_ERR, "Error creating thread");
      return NULL;
    }
  }
  return pool;
}

/*
* Función que libera los recursos usados para la pool.
*/
void pool_free(pool_thread * pool) {
  int i;

  syslog(LOG_INFO, "Cleaning");
  pool->stop = 1;

  /* Enviamos SIGUSR1 a todos los hilos para que terminen su ejecucion. */
  for(i=0; i < pool->num_threads; i++){
    pthread_kill(pool->threads[i], SIGUSR1);
  }
  for(i=0; i < pool->num_threads; i++){
    pthread_join(pool->threads[i], NULL);
  }

  free(pool->threads);
  free(pool);
}
