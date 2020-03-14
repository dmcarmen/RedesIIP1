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

#define NUM_EXTENSIONES 13
#define NUM_METODOS 3
#define MAX_BUF 1000

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
  {"pdf","application/pdf"},
  {"py", "text/plain"},
  {"php", "text/plain"}
};

void GETProcesa(int connval, char* server, char *path, extension *ext, char *vars);
void POSTProcesa(int connval, char* server, char *path, extension *ext, char *vars);
void OPTIONSProcesa(int connval, char* server, char *path, extension *ext, char *vars);
char* clean_vars(char *body);
char* FechaActual();
char* FechaModificado(char * path);
char* run_script(int connval, char* server, char *path, extension *ext, char *vars);
void enviarError(int connval, int error, char *server);

metodo metodos[] = {
  {"GET", GETProcesa},
  {"POST", POSTProcesa},
  {"OPTIONS", OPTIONSProcesa}
};

void enviarError(int connval, int error, char *server) {
  char res[MAX_BUF], *date;
  char *mensaje = "HTTP/1.1 %s\r\nDate: %s\r\nServer: %s\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n<!DOCTYPE HTML PUBLIC '-//IETF//DTD HTML 2.0//EN'><html><head><title>%s</title></head><body><h1>%s</h1><p>%s</p></body></html>";

  date = FechaActual();
  if ( date == NULL ) {
    return;
  }
  syslog(LOG_INFO, "Sending error %d to the client.", error);
  switch(error){
    case BAD_REQUEST:
      sprintf(res, mensaje,"400 Bad Request",date,server,196,"400 Bad Request","Bad Request","The request could not be understood by the server.");
      break;
    case NOT_FOUND:
      sprintf(res, mensaje,"404 Not Found",date,server,189,"404 Not Found","Not Found","The requested URL was not found on this server.");
      break;
    case INTERNAL_SERVER:
      sprintf(res, mensaje,"500 Internal Server Error",date,server,206,"500 Internal Server Error","Internal Server Error","An unexpected condition was encountered.");
      break;
    case NOT_IMPLEMENTED:
      sprintf(res, mensaje,"501 Not Implemented",date,server,246,"501 Method Not Implemented","Method Not Implemented","The server does not support the functionality required to fulfill the request.");
      break;
    default:
      break;
  }
  send(connval, res, strlen(res), 0);
  free(date);
}

int procesarPeticiones(int connval, char *server, char* server_root, int * stop){
  char buf[4096], *method = NULL, *path = NULL, total_path[100], *q_path = NULL, *mini_path = NULL, *body = NULL, *aux_path=NULL;
  char *vars = NULL, *aux_q = NULL;
  int pret, minor_version;
  struct phr_header headers[100];
  size_t method_len, path_len, num_headers;
  ssize_t rret;
  int n_ext, n_met;
  void (*funcion_procesa)(int , char*, char*, extension*, char*);

  while(*stop == 0){
    bzero(buf,sizeof(char)*4096);
    rret = recv(connval,buf, sizeof(buf), 0);
    if(rret == 0) return 0;

    if(rret == -1) {
      if(errno == EINTR){
        syslog(LOG_INFO, "Signal received, closing connection.");
        return rret;
      }
      syslog(LOG_ERR, "Error recv.");
      return 0;
    }

    syslog(LOG_INFO, "REQUEST: %s %d", buf, (int)rret);

    num_headers = sizeof(headers) / sizeof(headers[0]);

    pret = phr_parse_request((const char *)buf, rret, (const char **)&method, &method_len,(const char **) &path, &path_len,
                             &minor_version, headers, &num_headers, 0);

    if (pret < 0) {
      syslog(LOG_ERR, "Error parse request");
      enviarError(connval, INTERNAL_SERVER, server);
      continue;
    }
    //Guardamos el cuerpo de la petición, que se encuetra en el buffer más los bytes leidos por phr_parse_request
    if (pret > 1)
      body = buf + pret;
    syslog(LOG_INFO, "Body: %s", body);
    //Comprobamos que se da soporte el método
    for(n_met=0; n_met<NUM_METODOS; n_met++){
      if(strncmp(metodos[n_met].name, method, method_len) == 0) {
        funcion_procesa = metodos[n_met].funcion;
        syslog(LOG_INFO, "Method: %s", metodos[n_met].name);
        break;
      }
    }
    //Método no soportado
    if(n_met == NUM_METODOS) {
      enviarError(connval, NOT_IMPLEMENTED, server);
      continue;
    }

    aux_path = strndup(path, path_len);
    // Buscamos en el path si hay interogacion (solo deberia aparecer en GET si eso)
    q_path = strchr(aux_path, '?');
    // Si no encuentra ? si es POST cogera el cuerpo y lo limpiara, vars sera NULL
    if (q_path == NULL){
      vars = clean_vars(body);
      mini_path = strdup(aux_path);
    }
    // Si hay ? cogemos las variables del cuespo si es POST y de despues
    // de la ? si es GET y guardamos el path antes de la ?
    else{
      if(!(*body)){ //GET
        syslog(LOG_INFO, "q_path: %s", q_path);
        aux_q = strdup(q_path);
        vars = clean_vars(aux_q);
        free(aux_q);
      }
      else //POST
        vars = clean_vars(body);
      mini_path = strndup(aux_path, (int)((q_path - aux_path) *sizeof(char))); //TODO no se usar cadenas comprobar bien calculo:), si no token maybe, miedo a que no nul terminen
    }
    free(aux_path);
    syslog(LOG_INFO, "vars: %s", vars);
    syslog(LOG_INFO, "mini_path: %s", mini_path);

    if(strcmp(mini_path,"/") == 0 || path == NULL) {
      free(mini_path);
      mini_path = strdup("/index.html");
      path_len += strlen(mini_path) - 1;
    }

    sprintf(total_path, "%s%s", server_root, mini_path);
    syslog(LOG_INFO, "Path:%s", total_path);
    //Comprobamos si el recurso existe
    if(access(total_path, F_OK) == -1) {
      enviarError(connval, NOT_FOUND, server);
      free(vars);
      free(mini_path);
      continue;
    }

    //Comprobamos si se da soporte a la extension
    for(n_ext=0; n_ext<NUM_EXTENSIONES; n_ext++){
      if(strcmp(extensiones[n_ext].ext, strchr(mini_path, '.') + 1) == 0) {
        syslog(LOG_INFO, "Extension: %s %s %i", extensiones[n_ext].ext, strchr(mini_path, '.') + 1, n_ext); //quitar
        break;
      }
    }
    //Extensión incorrecta
    if(n_ext == NUM_EXTENSIONES) {
      syslog(LOG_INFO, "Extension incorrecta.");
      enviarError(connval, BAD_REQUEST, server);
      free(vars);
      free(mini_path);
      continue;
    }

    funcion_procesa(connval, server,total_path, &extensiones[n_ext], vars);
    free(vars);
    free(mini_path);
  }

  return 0;
}

void GETProcesa(int connval, char* server, char *path, extension *ext, char *vars) {
  int fd;
  char *date, *modified, res[MAX_BUF], *answer;
  int len, count;
  struct stat file_stat;

  //Calculamos la fecha actual
  date = FechaActual();
  if ( date == NULL ) {
    enviarError(connval,INTERNAL_SERVER, server);
    return;
  }

  //Calculamos la última fecha en la que fue modificado
  modified = FechaModificado(path);
  if (modified == NULL) {
    enviarError(connval, BAD_REQUEST, server); //TODO check si badrequest o internalerror
    free(date);
    return;
  }

  //Si tenemos variables y es un script lo corremos
  if (vars != NULL && (strcmp(ext->ext, "py") == 0 || strcmp(ext->ext, "php") == 0)){
    //Corremos el script
    answer = run_script(connval, server, path, ext, vars);
    if(!answer){
      syslog(LOG_ERR, "Error running the script.");
      enviarError(connval, INTERNAL_SERVER, server);
      free(date);
      free(modified);
      return;
    }
    //Enviamos la cabecera y la respuesta
    len = strlen(answer);
    sprintf(res,"HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: keep-alive\r\n\r\n", date, server, modified, len, ext->tipo);
    send(connval, res, strlen(res), 0);
    send(connval, answer, len, 0);

    free(date);
    free(modified);
    free(answer);
    return;
  } //si no devolvemos el archivo
  else{
    //Abrimos fichero
    fd = open(path, O_RDONLY);
    if(fd==-1){
      enviarError(connval,INTERNAL_SERVER, server);
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
    len = file_stat.st_size;

    //Enviamos el mensaje con el tamaño del fichero
    sprintf(res,"HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: keep-alive\r\n\r\n", date, server, modified, len, ext->tipo);
    send(connval, res, strlen(res), 0);

    //Enviamos los datos del fichero
    while((count = read(fd,res,sizeof(res))) > 0 ){
      send(connval,res,count,0);
    }
    free(date);
    free(modified);
    close(fd);
  }
}

void POSTProcesa(int connval, char* server, char *path, extension *ext, char *vars){ //buf+num
  char *answer, *date, res[MAX_BUF], *modified;
  int len;

  //Calculamos la fecha actual
  date = FechaActual();
  if (date == NULL) {
    enviarError(connval,INTERNAL_SERVER, server);
    return;
  }

  //Calculamos la última fecha en la que fue modificado
  modified = FechaModificado(path);
  if (modified == NULL) {
    enviarError(connval,BAD_REQUEST, server); //TODO check si badrequest o internalerror
    free(date);
    return;
  }

  //Corremos el script
  answer = run_script(connval, server, path, ext, vars);
  if(!answer){
    syslog(LOG_ERR, "Error running the script.");
    enviarError(connval, INTERNAL_SERVER, server);
    free(date);
    free(modified);
    return;
  }
  //Enviamos la cabecera y la respuesta
  len = strlen(answer);
  sprintf(res,"HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: keep-alive\r\n\r\n", date, server, modified, len, ext->tipo);
  send(connval, res, strlen(res), 0);
  send(connval, answer, len, 0);

  free(date);
  free(modified);
  free(answer);
}

void OPTIONSProcesa(int connval, char* server, char *path, extension *ext, char *vars){
  char *date, res[MAX_BUF];

  //Calculamos la fecha actual
  date = FechaActual();
  if ( date == NULL ) {
    enviarError(connval, INTERNAL_SERVER, server);
    return;
  }

  //Enviamos el mensaje con el tamaño del fichero
  sprintf(res,"HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nAllow: GET, POST, OPTIONS\r\nConnection: keep-alive\r\n\r\n",date,server);
  send(connval, res, strlen(res), 0);
  free(date);
}

char* run_script(int connval, char* server, char *path, extension *ext, char *vars){
  FILE *fp;
  char command[MAX_BUF]; //TODO 100?
  char *ret;
  int len;

  if(strcmp(ext->ext,"py") ==0)
    sprintf(command, "echo \"%s\" | %s %s", vars, PY, path);
  else if(strcmp(ext->ext,"php") ==0)
    sprintf(command, "echo \"%s\" | %s %s", vars, PHP, path);
  else{
    enviarError(connval, BAD_REQUEST, server);
    return NULL;
  }

  fp = popen(command, "r");
  if(!fp){
    syslog(LOG_ERR, "Error creating the pipe.");
    enviarError(connval, INTERNAL_SERVER, server);
    return NULL;
  }

  ret = (void*)calloc((MAX_BUF + 1), sizeof(char));
  if(!ret){
    syslog(LOG_ERR, "Error allocating ret.");
    enviarError(connval, INTERNAL_SERVER, server);
    return NULL;
  }

  len = fread(ret, 1, MAX_BUF, fp);
  if (len < 0){
    syslog(LOG_ERR, "Error reading the pipe.");
    enviarError(connval, INTERNAL_SERVER, server);
    free(ret);
    pclose(fp);
    return NULL;
  }

  if (pclose(fp) == -1){
    syslog(LOG_ERR, "Error closing the pipe.");
    free(ret);
    enviarError(connval, INTERNAL_SERVER, server);
    return NULL;
  }
  return (char*)ret;
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

char* clean_vars(char *body)
{
  char *clean_str, *ret;
  int flag = 1, i, j;

  if(!body || *body == 0)
    return NULL;
  clean_str = strdup(body);
  ret = body;
  //?var1=manolo&var2=paco&var3=men
  i = 0;
  while(flag)
  {
    //busca primer =
    if(!(ret = strchr(ret, '='))){
      free(clean_str);
      return NULL;
    }
    //guarda hasta encontrar & o fin de cadena
    for (j = 1; ret[j] != '&' && ret[j] != 0; j++)
    {
      clean_str[i] = ret[j];
      i++;
    }
    //si es fin de cadena se va del bucle
    if(ret[j] == 0)
    {
      flag = 0;
      break;
    }
    ret = ret + j;
    clean_str[i] = ' ';
    i++;
  }
  clean_str[i] = 0;
  return clean_str;
}
