#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <syslog.h>
#include "picohttpparser.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>


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

void GETProcesa(int connval, char *path, extension *ext);
void POSTProcesa(int connval);
void OPTIONSProcesa(int connval);

metodo metodos[] = {
  {"GET", GETProcesa},
  {"POST", POSTProcesa},
  {"OPTIONS", OPTIONSProcesa}
};

void enviarError(int connval, int error) {
  char *res, *date;

  //TODO comprobar esto, date y server??
  switch(error){
    case BAD_REQUEST:
    sprintf(res,"HTTP/1.1 400 Bad Request\nDate: %s\nServer: %s\nConnection: keep-alive\r\n\r\n",date,server);
      break;
    case NOT_FOUND:
      sprintf(res,"HTTP/1.1 400 Not Found\nDate: %s\nServer: %s\nConnection: keep-alive\r\n\r\n",date,server);
      break;
    case INTERNAL_SERVER:
      sprintf(res,"HTTP/1.1 400 Internal Server Error\nDate: %s\nServer: %s\nConnection: keep-alive\r\n\r\n",date,server);
      break;
    case NOT_IMPLEMENTED:
      printf(res,"HTTP/1.1 400 Not Implemented\nDate: %s\nServer: %s\nConnection: keep-alive\r\n\r\n",date,server);
      break;
    default:
      break; 
  }
  send(connval,res,0);
}

/*TODO metodos, cuando parar de procesar peticiones*/
int procesarPeticiones(int connval){
  char buf[4096], *method, *path;
  int pret, minor_version;
  struct phr_header headers[100];
  size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
  ssize_t rret;
  int n_ext, n_met, i;
  char *cadena, *res;

  while(1){
    //TODO Esto esta distinto, cuidao
    rret = recv(connval,buf + buflen, sizeof(buf) - buflen, 0);
    if(rret == -1) {
      if(errno == EINTR){
        return rret;
      }
      syslog(LOG_ERR, "Error recv");
      exit(EXIT_FAILURE);
    }

    prevbuflen = buflen;
    buflen += rret;

    num_headers = sizeof(headers) / sizeof(headers[0]);
    pret = phr_parse_request((const char *)buf, buflen, (const char **)&method, &method_len,(const char **) &path, &path_len,
                             &minor_version, headers, &num_headers, prevbuflen);

    if (pret < 0) {
      syslog(LOG_ERR, "Error parse request");
      enviarError(connval, INTERNAL_SERVER);
      return -1;
    }

    /*printf("request is %d bytes long\n", pret);
    printf("method is %.*s\n", (int)method_len, method);
    printf("path is %.*s\n", (int)path_len, path);
    printf("HTTP version is 1.%d\n", minor_version);
    printf("headers:\n");
    for (i = 0; i != num_headers; ++i) {
        printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
               (int)headers[i].value_len, headers[i].value);
    }*/


    //Comprobamos que se da soporte el método
    for(n_met=0;n_met<NUM_METODOS;n_met++){
      if(strcmp(metodos[n_met].name,method) != 0) {
        break;
      }
    }
    //Método no soportado
    if(n_met == NUM_METODOS) {
      enviarError(connval, NOT_IMPLEMENTED);
      continue;
    }

    //Comprobamos si el recurso existe
    if(access(path, F_OK ) == -1) {
      enviarError(connval, NOT_FOUND);
      continue;
    }

    //Comprobamos si se da soporte a la extension
    for(n_ext=0;n_ext<NUM_EXTENSIONES;n_ext++){
      if(strcmp(extensiones[n_ext].ext,&path[path_len-strlen(extensiones[n_ext].ext)]) != 0) {
        break;
      }
    }

    //Extensión incorrecta
    if(n_ext == NUM_EXTENSIONES) {
      enviarError(connval, BAD_REQUEST);
      continue;
    }
  }

}

char * FechaActual(){
  time_t now;
  char s[100];
  char *date;

  now = time(NULL);
  strftime(s, sizeof(s)-1, "%a, %d %b %Y %H:%M:%S %Z", &gmtime(&now));

  date = (char *) malloc (sizeof(s));
  if(date==NULL) return NULL;

  strcpy(date, s);
  return date;
}

char * FechaModificado(char * path){
  struct stat buf;
  char s[100];
  char *date;

  if( stat(path,&buf) == -1) return NULL;
  strftime(s, sizeof(s)-1, "%a, %d %b %Y %H:%M:%S %Z", &gmtime(&buf.st_mtime));

  date = (char *) malloc (sizeof(s));
  if(date==NULL) return NULL;

  strcpy(date, s);
  return date;
}

void GETProcesa(int connval, char *path, extension *ext) {
  int fd;
  char *date, *server, *modified, *res;
  int len, count;
  struct stat file_stat;

  //Calculamos la fecha actual
  date = FechaActual();
  if ( date == NULL ) {
    enviarError(connval,INTERNAL_SERVER);
    return;
  }

  //Calculamos la última fecha en la que fue modificado
  modified = FechaModificado(path);
  if ( modified == NULL) {
    enviarError(connval,INTERNAL_SERVER);
    free(date);
    return;
  }

  //TODO server
  server="";

  //Abrimos fichero
  fd = open(path, O_RDONLY);
  if(fd==-1){
    enviarError(connval,INTERNAL_SERVER);
    free(date);
    free(modified);
    return;
  }

  //Calculamos su tam
  if(fstat(fd, &file_stat) == -1) {
    //Error obteniendo info
    free(date);
    free(modified);
    return;
  }

  len=file_stat.st_size;

  //Enviamos el mensaje con el tamaño del fichero
  sprintf(res,"HTTP/1.1 %d %s\nDate: %s\nServer: %s\nLast-Modified: %s\nContent-Length: %d\nContent-Type: %s\nConnection: keep-alive\r\n\r\n",date,server,modified,ext->tipo,len);
  send(connval, res, strlen(res), 0);

  //Enviamos los datos del fichero
  while( count = read(fd,res,sizeof(res)) > 0 ){
    send(connval,res,count,0);
  }

  free(date);
  free(modified);
  close(fd);
}

void POSTProcesa(int connval, char *path, extension *ext){

  if(strcmp(ext->ext,"py") !=0 ){
    enviarError(connval, BAD_REQUEST);
    return;
  }


}

void OPTIONSProcesa(int connval){
   char *date, *server, *res;

  //Calculamos la fecha actual
  date = FechaActual();
  if ( date == NULL ) {
    enviarError(connval,INTERNAL_SERVER);
    return;
  }

  //TODO server
  server="";

  //Enviamos el mensaje con el tamaño del fichero
  sprintf(res,"HTTP/1.1 200 OK\nDate: %s\nServer: %s\nAllow: GET, POST, OPTIONS\nConnection: keep-alive\r\n\r\n",date,server);
  send(connval, res, strlen(res), 0);
  free(date);
}