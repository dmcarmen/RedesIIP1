#include "http.h"
#include <stdio.h>
#include <stdlib.h>

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

int procesarPeticiones(int connval){
  char buf[4096], *method, *path;
  int pret, minor_version;
  struct phr_header headers[100];
  size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
  ssize_t rret;

  while(1){
    rret = recv(connval,buf + buflen, sizeof(buf) - buflen, 0); //Esto esta distinto
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

    //comprobar si path existe si no 404

    //ver si la extension existe

    pestructura = phr_parse_response();
    send(connval,pestructura);
  }
}
