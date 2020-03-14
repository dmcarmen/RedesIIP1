#include "pool.h"
#include <syslog.h>
#include <unistd.h>

void * thread_accept(void * pool){
  pool_thread *p = (pool_thread *)pool;
  int connval;

  while(p->stop == 0) {
    connval = socket_accept(p->sockval);
    /* Si connval es -2 no se ha realizado el accept por la interrupcion de una
    * senial. Es decir, el hilo tiene que terminar. */
    if(connval == -2) pthread_exit(NULL);
    /* Si connval es -1 se ha producido un error mientras estaba en el accept.*/
    if(connval == -1) continue;
    if(procesarPeticiones(connval, p->server_signature, p->server_root, &p->stop)==-1) {
      close(connval);
      pthread_exit(NULL);
    }
    syslog(LOG_INFO, "Conexion cerrada");
    close(connval);
  }
  pthread_exit(NULL);
}

pool_thread * pool_create(int sockval, char* server_signature, char* server_root){
  pool_thread *pool;
  int i;

  pool = (pool_thread *) malloc (sizeof(pool_thread));
  if(pool == NULL){
    syslog(LOG_ERR, "Error creating pool");
    exit(EXIT_FAILURE);
  }

  pool->num_threads = NUM_THREADS;
  pool->stop = 0;
  pool->sockval = sockval;
  //TODO se liberan fuera, no deberia drama creooo
  pool->server_signature = server_signature;
  pool->server_root = server_root;
  if(pthread_mutex_init(&pool->shared_mutex, NULL)!=0){
    free(pool);
    syslog(LOG_ERR, "Error creating mutex");
    exit(EXIT_FAILURE);
  }

  pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * pool->num_threads);
  if(pool->threads == NULL) {
    free(pool);
    syslog(LOG_ERR, "Error creating thread");
    exit(EXIT_FAILURE);
  }
  pool->work_function = thread_accept;

  //TODO hay que hacer algo con los hilos creados si hay error
  for(i=0; i<pool->num_threads; i++){
    if(pthread_create(&(pool->threads[i]),NULL, pool->work_function,(void *) pool)!=0){
      free(pool->threads);
      free(pool);
      syslog(LOG_ERR, "Error creating thread");
      exit(EXIT_FAILURE);
    }
  }
  return pool;
}

void pool_free(pool_thread * pool) {
  int i;

  syslog(LOG_INFO, "Cleaning");

  pool->stop = 1;

  for(i=0; i<pool->num_threads; i++){
    pthread_kill(pool->threads[i], SIGUSR1);
  }

  for(i=0; i<pool->num_threads; i++){
    pthread_join(pool->threads[i], NULL);
  }

  free(pool->threads);
  free(pool);
}
