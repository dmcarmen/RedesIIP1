#include "pool.h"

#include "sockets.h"
#include "http.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>

/* Estructura de la pool. */
struct pool_thread {
  int num_threads;
  int stop;
  int sockval;
  pthread_t *threads;
  void *(*work_function)(void *);
  char *server_signature;
  char *server_root;
};

/*
* Funcion que corre cada hilo de la pool. Se encarga de aceptar la conexion
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
    /* Si connval es -1 se ha producido otro error mientras estaba en el accept. */
    if(connval == -1) continue;

    /* Se procesan las peticiones HTTP. */
    process_petitions(connval, p->server_signature, p->server_root, &p->stop);
    syslog(LOG_INFO, "Connection closed");
    close(connval);
  }
  pthread_exit(NULL);
}

/*
* Funcion que inicializa la pool (la estructura y los hilos).
*/
pool_thread * pool_create(int sockval, char* server_signature, char* server_root, int num_threads){
  pool_thread *pool;
  int i, j;

  /* Reserva memoria para la estructura pool e inicializa sus valores. */
  pool = (pool_thread *) malloc (sizeof(pool_thread));
  if(pool == NULL){
    syslog(LOG_ERR, "Error creating pool");
    return NULL;
  }
  pool->num_threads = num_threads;
  pool->stop = 0;
  pool->sockval = sockval;
  pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * pool->num_threads);
  if(pool->threads == NULL) {
    free(pool);
    syslog(LOG_ERR, "Error malloc thread");
    return NULL;
  }
  pool->work_function = thread_accept;
  pool->server_signature = server_signature;
  pool->server_root = server_root;

  /* Crea los hilos y cada uno llama a la work function. */
  for(i=0; i<pool->num_threads; i++){
    if(pthread_create(&(pool->threads[i]),NULL, pool->work_function,(void *) pool)!=0) {
      /* Si ocurre un error con alguno de los hilos, terminamos los hilos
      * ya creados. */
      /* Envia SIGUSR1 a todos los hilos para que terminen su ejecucion. */
      for(j=0; j < i; j++){
        pthread_kill(pool->threads[j], SIGUSR1);
      }
      /* Espera a que acaben los hilos. */
      for(j=0; j < i; j++){
        pthread_join(pool->threads[j], NULL);
      }
      free(pool->threads);
      free(pool);
      syslog(LOG_ERR, "Error creating thread");
      return NULL;
    }
  }
  return pool;
}

/*
* Funcion que libera los recursos usados para la pool.
*/
void pool_free(pool_thread * pool) {
  int i;

  syslog(LOG_INFO, "Cleaning");
  /* Flag stop a 1 para que los hilos salgan de los bucles. */
  pool->stop = 1;

  /* Envia SIGUSR1 a todos los hilos para que terminen su ejecucion. */
  for(i=0; i < pool->num_threads; i++){
    pthread_kill(pool->threads[i], SIGUSR1);
  }
  /* Espera a que acaben los hilos. */
  for(i=0; i < pool->num_threads; i++){
    pthread_join(pool->threads[i], NULL);
  }

  free(pool->threads);
  free(pool);
}
