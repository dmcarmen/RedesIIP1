#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_EXTENSIONES 11
#define NUM_METODOS 3

extension extensiones[] = {
  {"txt", "text/plain"},
  {"html","text/html"},
  {"htm","text/html"},
  {"gif","image/gif"},
  {"jpeg","image/jpeg"},
  {"jpg","image/jpg"},
  {"mpeg","video/mpeg"},
  {"mpg","video/mpeg"},
  {"doc","application/msword"},
  {"docx","application/msword"},
  {"pdf","application/pdf"}
};

//TODO cambiar para que esten asociadas a metodos
char metodos[] = {"GET", "POST", "OPTIONS"};

/*TODO errores, comprobar si while tiene sentido, respuestas, metodos
Comprobar si tiene sentido lo de antes de llamar a esta funcion*/
int procesarPeticiones(int connval){
  char buf[4096], *method, *path;
  int pret, minor_version;
  struct phr_header headers[100];
  size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
  ssize_t rret;
  int n_ext, n_met;
  char *cadena, *res;

  while(1){
    //TODO Esto esta distinto, cuidao
    rret = recv(connval,buf + buflen, sizeof(buf) - buflen, 0);
    if(rret == -1) {
      syslog(LOG_ERR, "Error recv");
      return -1;
    }

    prevbuflen = buflen;
    buflen += rret;

    num_headers = sizeof(headers) / sizeof(headers[0]);
    pret = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
                             &minor_version, headers, &num_headers, prevbuflen);

    if (pret < 0) {
      syslog(LOG_ERR, "Error parse request");
      return -1;
    }

    //Comprobamos si el recurso existe
    if(access( fname, F_OK ) == -1) {
      res = parse_response();
      send(connval,res,len(res),0);
      continue;
    }

    //Comprobamos si se da soporte a la extension
    for(n_ext=0;n_ext<NUM_EXTENSIONES;n_ext++){
      if(strcmp(extensiones[n_ext].ext,&path[pathlen-strlen(extensiones[n_ext].ext)]) != 0) {
        break;
      }
    }

    //Extensión incorrecta
    if(n_ext == NUM_EXTENSIONES) {
      res = parse_response();
      send(connval,res,len(res),0);
      continue;
    }

    //Comprobamos que se da soporte el método
    for(n_met=0;n_met<NUM_METODOS;n_met++){
      if(strcmp(extensiones[n_met].ext,&path[pathlen-strlen(extensiones[n_met].ext)]) != 0) {
        break;
      }
    }

    //Método no soportado
    if(n_met == NUM_METODOS) {
      res = parse_response();
      send(connval,res,len(res),0);
      continue;
    }

    //Parseamos la respuesta
    res = parse_response(const char *buf, const char *buf_end, int *minor_version, int *status, const char **msg,
                                  size_t *msg_len, struct phr_header *headers, size_t *num_headers, size_t max_headers, int *ret);

    //Enviamos la respuesta
    send(connval, res, len(res), 0);
  }
}
