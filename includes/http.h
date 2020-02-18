#ifndef HTTP_H_
#define HTTP_H_

#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404

typedef struct extension extension;

struct extension {
  char * ext;
  char * tipo;
};

void procesarPeticiones(int connval);

#endif
