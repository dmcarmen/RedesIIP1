#ifndef HTTP_H_
#define HTTP_H_

#include "picohttpparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

/* Constantes codigos de error. */
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define INTERNAL_SERVER 500
#define NOT_IMPLEMENTED 501

/* Constantes posibles scripts. */
#define PY "python3"
#define PHP "php -f"

/* Tamanio maximo buffers de entrada y salida. */
#define MAX_BUF 1000

/* Extension de archivo y su tipo. */
typedef struct extension extension;

struct extension {
  char * ext;
  char * tipo;
};

/* Metodo HTTP y la funcion que lo procesa. */
typedef struct method method;

struct method {
	char *name;
	void (*funcion)(int , char*, char*, extension*, char*);
};

void process_petitions(int connval, char *server_signature, char* server_root, int * stop);

#endif
