#ifndef POOL_H_
#define POOL_H_

#include <stdlib.h>

/* Estructura de la pool. */
typedef struct pool_thread pool_thread;

/*
* Funcion que inicializa la pool (la estructura y los hilos).
*/
pool_thread * pool_create(int sockval, char* server_signature, char* server_root, int num_threads);

/*
* Funcion que corre cada hilo de la pool. Se encarga de aceptar la conexion
* procesar las peticiones HTTP que le lleguen y al terminar cerrar la conexion.
*/
void * thread_accept(void * pool);

/*
* Funcion que libera los recursos usados para la pool.
*/
void pool_free(pool_thread * pool);

#endif
