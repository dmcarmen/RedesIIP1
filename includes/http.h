#ifndef HTTP_H_
#define HTTP_H_

/* Extension de archivo y su tipo. */
typedef struct extension extension;

/* Metodo HTTP y la funcion que lo procesa. */
typedef struct method method;

/*
* Funcion que procesa todas las peticiones que van llegando a una misma conexion.
* Solo para si hay un error grave o si recibe la senial para terminar (en este
* caso termina lo que estuviera haciendo antes de irse).
*/
void process_petitions(int connval, char *server_signature, char* server_root, int * stop);

#endif
