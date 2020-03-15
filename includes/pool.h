#ifndef POOL_H_
#define POOL_H_

#include "sockets.h"
#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>

#define NUM_THREADS 10

typedef struct pool_thread pool_thread;

struct pool_thread {
  int num_threads;
  int stop;
  int sockval;
  pthread_t *threads;
  void *(*work_function)(void *);
  char *server_signature;
  char *server_root;
};

pool_thread * pool_create(int sockval, char* server_signature, char* server_root);
void * thread_accept(void * pool);
void pool_free(pool_thread * pool);

#endif
