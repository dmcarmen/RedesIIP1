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
#include <sys/stat.h>
#include <time.h>

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

void GETProcesa(int connval, char *path, extension *ext, char *cuerpo);
void POSTProcesa(int connval, char *path, extension *ext, char *cuerpo);
void OPTIONSProcesa(int connval, char *path, extension *ext, char *cuerpo);
char* cleanVars(char *cuerpo);
char* FechaActual();

metodo metodos[] = {
  {"GET", GETProcesa},
  {"POST", POSTProcesa},
  {"OPTIONS", OPTIONSProcesa}
};

void enviarError(int connval, int error) {
  char *res, *date, *server;

  date = FechaActual();
  if ( date == NULL ) {
    return;
  }

  //TODO server??
  server="";

  switch(error){
    case BAD_REQUEST:
    sprintf(res,"HTTP/1.1 400 Bad Request\nDate: %s\nServer: %s\nConnection: keep-alive\r\n\r\n",date,server);
      break;
    case NOT_FOUND:
      sprintf(res,"HTTP/1.1 404 Not Found\nDate: %s\nServer: %s\nConnection: keep-alive\r\n\r\n",date,server);
      break;
    case INTERNAL_SERVER:
      sprintf(res,"HTTP/1.1 500 Internal Server Error\nDate: %s\nServer: %s\nConnection: keep-alive\r\n\r\n",date,server);
      break;
    case NOT_IMPLEMENTED:
      printf(res,"HTTP/1.1 501 Not Implemented\nDate: %s\nServer: %s\nConnection: keep-alive\r\n\r\n",date,server);
      break;
    default:
      break;
  }

  send(connval, res, strlen(res), 0);
  free(date);
}

/*TODO cuando parar de procesar peticiones, cuerpo*/
void procesarPeticiones(int connval){
  char buf[4096], *method, *path;
  int pret, minor_version;
  struct phr_header headers[100];
  size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
  ssize_t rret;
  int n_ext, n_met;
  char *cadena, *res, *cuerpo;

  while(1){
    //TODO Esto esta distinto, cuidao
    rret = recv(connval,buf + buflen, sizeof(buf) - buflen, 0);
    if(rret == -1) {
      syslog(LOG_ERR, "Error recv");
      return;
    }

    prevbuflen = buflen;
    buflen += rret;

    num_headers = sizeof(headers) / sizeof(headers[0]);
    //TODO bytes leidos == pret, para sacar el cuerpo buf + pret
    pret = phr_parse_request((const char *)buf, buflen, (const char **)&method, &method_len,(const char **) &path, &path_len,
                             &minor_version, headers, &num_headers, prevbuflen);

    if (pret < 0) {
      syslog(LOG_ERR, "Error parse request");
      enviarError(connval, INTERNAL_SERVER);
      continue;
    }
    //Guardamos el cuerpo de la petición, que se encuetra en el buffer más los bytes leidos por phr_parse_request
    if (pret > 1)
      cuerpo = buf - pret;

    //Comprobamos que se da soporte el método
    for(n_met=0; n_met<NUM_METODOS; n_met++){
      if(strcmp(metodos[n_met].name, method) == 0) {
        break;
      }
    }
    //Método no soportado
    if(n_met == NUM_METODOS) {
      enviarError(connval, NOT_IMPLEMENTED);
      continue;
    }

    if(strcmp(path,"/") == 0 || path == NULL) {
      path="/index.html";
      path_len=strlen(path);
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

    funcion_procesa(connval, metodos[n_met].funcion, extensiones[n_ext], cuerpo);
  }

}

char * FechaActual(){
  time_t now;
  char s[100];
  char *date;

  now = time(NULL);
  strftime(s, sizeof(s)-1, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&now));

  date = (char *) malloc (sizeof(s));
  if(date==NULL) return NULL;

  strcpy(date, s);
  return date;
}

char * FechaModificado(char * path){
  struct stat buf;
  char s[100];
  char *date;

  if(stat(path,&buf) == -1) return NULL;
  strftime(s, sizeof(s)-1, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&buf.st_mtime));

  date = (char *) malloc (sizeof(s));
  if(date==NULL) return NULL;

  strcpy(date, s);
  return date;
}

void GETProcesa(int connval, char *path, extension *ext, char *cuerpo) {
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
    enviarError(connval,BAD_REQUEST); //TODO check si badrequest o internalerror
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
  sprintf(res,"HTTP/1.1 200 OK\nDate: %s\nServer: %s\nLast-Modified: %s\nContent-Length: %d\nContent-Type: %s\nConnection: keep-alive\r\n\r\n", date, server, modified, len, ext->tipo);
  send(connval, res, strlen(res), 0);

  //Enviamos los datos del fichero
  while((count = read(fd,res,sizeof(res))) > 0 ){
    send(connval,res,count,0);
  }

  free(date);
  free(modified);
  close(fd);
}

void POSTProcesa(int connval, char *path, extension *ext, char *cuerpo){ //buf+num
  char command[100], answer[100]; //TODO 100?
  char *vars;
  FILE *fp;
  char py[] = "python3"; //TODO i guess constants
  char php[] = "php -f";

  vars = cleanVars(cuerpo); //TODO remember free vars
  if(!vars){
    enviarError(connval, BAD_REQUEST);
    return;
  }

  if(strcmp(ext->ext,"py") ==0)
    sprintf(command, "echo \"%s\" | %s %s", vars, py, path);
  else if(strcmp(ext->ext,"php") ==0)
    sprintf(command, "echo \"%s\" | %s %s", vars, php, path);
  else{
    free(vars);
    enviarError(connval, BAD_REQUEST);
    return;
  }

  fp = popen(command, "r");
  if(!fp){
    free(vars);
    enviarError(connval, INTERNAL_SERVER);
    return;
  }
  fgets(answer, 100, fp);
  if (pclose(fp) != 0){
    free(vars);
    enviarError(connval, INTERNAL_SERVER);
    return;
  }
  free(vars);
}

char* cleanVars(char *cuerpo)
{
  char *limpio;
  char *ret;
  int flag = 1;

  limpio = (char *)malloc(sizeof(char) * (strlen(cuerpo) + 1));
  if(!limpio)
    return NULL;
  ret = cuerpo;
//var1=manolo&var2=paco&var3=men
  while(flag)
  {
    //busca primer =
    if(!(ret = strchr(ret, '='))){
      free(limpio);
      return NULL;
    }
    ret++;
    //guarda hasta encontrar & o fin de cadena
    while (*ret != '&' || *ret != 0)
    {
      *limpio = *ret;
      limpio++;
      ret++;
    }
    limpio++;
    //si es fin de cadena se va del bucle
    if(*ret == 0)
    {
      flag = 0;
      break;
    }
    *limpio = ' ';
    limpio++;
  }
  *limpio = 0;
  return limpio;
}

void OPTIONSProcesa(int connval, char *path, extension *ext, char *cuerpo){
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
