#ifndef HTTP_H_
#define HTTP_H_

#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define INTERNAL_SERVER 500
#define NOT_IMPLEMENTED 501

#define PY "python3"
#define PHP "php -f"

typedef struct extension extension;

struct extension {
  char * ext;
  char * tipo;
};

typedef struct metodo metodo;

struct metodo {
	char *name;
	void (*funcion)(int , char*, char*, extension*, char*); //meter parametros
};

int procesarPeticiones(int connval, char *server_signature, char* server_root);

#endif
