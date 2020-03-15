#include "http.h"

void send_error(int connval, int error, char *server);
void GET_process(int connval, char* server, char *path, extension *ext, char *vars);
void POST_process(int connval, char* server, char *path, extension *ext, char *vars);
void OPTIONS_process(int connval, char* server, char *path, extension *ext, char *vars);
char* get_date();
char* get_mod_date(char * path);
char* run_script(int connval, char* server, char *path, extension *ext, char *vars);
char* clean_vars(char *body);

/* Extensiones implementadas. Es necesario mantener acorde la constante NUM_EXTENSIONS */
#define NUM_EXTENSIONS 13
extension extensions[] = {
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

/* Metodos implementados. Es necesario mantener acorde la constante NUM_METHODS */
#define NUM_METHODS 3
method methods[] = {
  {"GET", GET_process},
  {"POST", POST_process},
  {"OPTIONS", OPTIONS_process}
};

/*
* Función que procesa todas las peticiones que van llegando a una misma conexion.
* Para solo si hay un error grave o si recibe la senial para terminar (en este
* caso termina lo que estuviera haciendo antes de irse).
*/
int process_petitions(int connval, char *server, char* server_root, int * stop){
  char buf[4096], *method = NULL, *path = NULL, *body = NULL;
  char total_path[100], *mini_path = NULL, *aux_path=NULL;
  char *vars = NULL, *q_path = NULL, *aux_q = NULL;
  int pret, minor_version;
  int n_ext, n_met;
  struct phr_header headers[100];
  size_t method_len, path_len, num_headers;
  ssize_t rret;
  void (*funcion_procesa)(int , char*, char*, extension*, char*);

  /* Corre hasta recibir la señal para parar o un break por error. */
  while(*stop == 0){
    /* Vaciamos el buffer y guardamos lo que nos mande el cliente ahi. */
    bzero(buf,sizeof(char)*4096);
    rret = recv(connval,buf, sizeof(buf), 0);
    if(rret == 0) return 0;

    /* Si recibimos una senial, terminamos la ejecucion. */
    if(rret == -1) {
      if(errno == EINTR){
        syslog(LOG_INFO, "Signal received, closing connection.");
        return rret;
      }
      syslog(LOG_ERR, "Error recv.");
      return 0;
    }
    syslog(LOG_INFO, "REQUEST: %s %d", buf, (int)rret);

    /* Usando las funciones de picohttpparser parseamos la peticion. */
    num_headers = sizeof(headers) / sizeof(headers[0]);

    pret = phr_parse_request((const char *)buf, rret, (const char **)&method, &method_len,(const char **) &path, &path_len,
                             &minor_version, headers, &num_headers, 0);

    if (pret < 0) {
      syslog(LOG_ERR, "Error parse request");
      send_error(connval, INTERNAL_SERVER, server);
      continue;
    }

    /*Guardamos el cuerpo de la petición, que se encuetra en el buffer más los bytes leidos por phr_parse_request. */
    if (pret > 1)
      body = buf + pret;
    syslog(LOG_INFO, "Body: %s", body);

    /*Comprobamos que se da soporte al método. */
    for(n_met=0; n_met<NUM_METHODS; n_met++){
      if(strncmp(methods[n_met].name, method, method_len) == 0) {
        funcion_procesa = methods[n_met].funcion;
        syslog(LOG_INFO, "Method: %s", methods[n_met].name);
        break;
      }
    }
    if(n_met == NUM_METHODS) { // Metodo no soportado
      send_error(connval, NOT_IMPLEMENTED, server);
      continue;
    }

    aux_path = strndup(path, path_len);
    /* Buscamos en el path si hay interogacion (para las peticiones GET).*/
    q_path = strchr(aux_path, '?');
    /* Si no encuentra ? si es POST cogera el cuerpo y lo limpiara,
      si no, vars sera NULL. */
    if (q_path == NULL){
      vars = clean_vars(body);
      /* Guardamos en mini_path el path relativo que nos han mandado. */
      mini_path = strdup(aux_path);
    }
    /* Si hay ? cogemos las variables de despues de la ? si es GET, y si es POST
       del cuerpo. Despues guardamos en mini_path el path relativo de antes de la ? */
    else{
      if(!(*body)){ //GET
        syslog(LOG_INFO, "q_path: %s", q_path);
        aux_q = strdup(q_path);
        vars = clean_vars(aux_q);
        free(aux_q);
      }
      else //POST
        vars = clean_vars(body);
      mini_path = strndup(aux_path, (int)((q_path - aux_path) * sizeof(char)));
    }
    free(aux_path);
    syslog(LOG_INFO, "vars: %s", vars);
    syslog(LOG_INFO, "mini_path: %s", mini_path);

    /* Si el path relativo es / se refiere al index. */
    if(strcmp(mini_path,"/") == 0 || path == NULL) {
      free(mini_path);
      mini_path = strdup("/index.html");
      path_len += strlen(mini_path) - 1;
    }

    /* Guardamos el path completo y comprobamos si el recuros existe. */
    sprintf(total_path, "%s%s", server_root, mini_path);
    syslog(LOG_INFO, "Path: %s", total_path);
    if(access(total_path, F_OK) == -1) {
      send_error(connval, NOT_FOUND, server);
      free(vars);
      free(mini_path);
      continue;
    }

    /* Comprobamos si se da soporte a la extension. */
    for(n_ext=0; n_ext<NUM_EXTENSIONS; n_ext++){
      if(strcmp(extensions[n_ext].ext, strchr(mini_path, '.') + 1) == 0) {
        syslog(LOG_INFO, "Extension: %s", extensions[n_ext].ext);
        break;
      }
    }
    if(n_ext == NUM_EXTENSIONS) { //Extensión incorrecta
      syslog(LOG_INFO, "Incorrect extension.");
      send_error(connval, BAD_REQUEST, server);
      free(vars);
      free(mini_path);
      continue;
    }

    /* Procesamos con la funcion del metodo correspondiente la peticion. */
    funcion_procesa(connval, server,total_path, &extensions[n_ext], vars);
    free(vars);
    free(mini_path);
  }
  return 0;
}

/*
* Función que se encarga de enviar respuestas de error al cliente.
*/
void send_error(int connval, int error, char *server) {
  char res[MAX_BUF], *date;
  char *mensaje = "HTTP/1.1 %s\r\nDate: %s\r\nServer: %s\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n<!DOCTYPE HTML PUBLIC '-//IETF//DTD HTML 2.0//EN'><html><head><title>%s</title></head><body><h1>%s</h1><p>%s</p></body></html>";

  date = get_date();
  if (date == NULL) {
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

/*
* Función que procesa las peticiones de tipo GET.
*/
void GET_process(int connval, char* server, char *path, extension *ext, char *vars) {
  int fd;
  char *date, *modified, res[MAX_BUF], *answer;
  int len, count;
  struct stat file_stat;

  /* Calculamos la fecha actual. */
  date = get_date();
  if (date == NULL) {
    send_error(connval,INTERNAL_SERVER, server);
    return;
  }

  /* Calculamos la última fecha en la que fue modificado. */
  modified = get_mod_date(path);
  if (modified == NULL) {
    send_error(connval, INTERNAL_ERROR, server); //TODO check si badrequest o internalerror
    free(date);
    return;
  }

  /*Si tenemos variables y es un script lo corremos. */
  if (vars != NULL && (strcmp(ext->ext, "py") == 0 || strcmp(ext->ext, "php") == 0)){
    /* Corremos el script. */
    answer = run_script(connval, server, path, ext, vars);
    if(!answer){
      syslog(LOG_ERR, "Error running the script.");
      send_error(connval, INTERNAL_SERVER, server);
      free(date);
      free(modified);
      return;
    }
    /* Enviamos la cabecera y la respuesta. */
    len = strlen(answer);
    sprintf(res,"HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: keep-alive\r\n\r\n", date, server, modified, len, ext->tipo);
    send(connval, res, strlen(res), 0);
    send(connval, answer, len, 0);

    free(date);
    free(modified);
    free(answer);
    return;
  }
  /* Si no es un script devolvemos el archivo */
  else{
    /* Abrimos el fichero. */
    fd = open(path, O_RDONLY);
    if(fd==-1){
      send_error(connval,INTERNAL_SERVER, server);
      free(date);
      free(modified);
      return;
    }

    /* Calculamos su tam. */
    if(fstat(fd, &file_stat) == -1) {
      syslog(LOG_ERR, "Error fstat");
      free(date);
      free(modified);
      return;
    }
    len = file_stat.st_size;

    /* Enviamos el mensaje con el tamaño del fichero. */
    sprintf(res,"HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: keep-alive\r\n\r\n", date, server, modified, len, ext->tipo);
    send(connval, res, strlen(res), 0);

    /* Enviamos los datos del fichero. */
    while((count = read(fd,res,sizeof(res))) > 0 ){
      send(connval,res,count,0);
    }
    free(date);
    free(modified);
    close(fd);
  }
}

/*
* Función que procesa las peticiones de tipo POST.
*/
void POST_process(int connval, char* server, char *path, extension *ext, char *vars){ //buf+num
  char *answer, *date, res[MAX_BUF], *modified;
  int len;

  /* Calculamos la fecha actual. */
  date = get_date();
  if (date == NULL) {
    send_error(connval,INTERNAL_SERVER, server);
    return;
  }

  /* Calculamos la última fecha en la que fue modificado. */
  modified = get_mod_date(path);
  if (modified == NULL) {
    send_error(connval,INTERNAL_ERROR, server); //TODO check si badrequest o internalerror
    free(date);
    return;
  }

  /* Corremos el script. */
  answer = run_script(connval, server, path, ext, vars);
  if(!answer){
    syslog(LOG_ERR, "Error running the script.");
    send_error(connval, INTERNAL_SERVER, server);
    free(date);
    free(modified);
    return;
  }
  /* Enviamos la cabecera y la respuesta. */
  len = strlen(answer);
  sprintf(res,"HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: keep-alive\r\n\r\n", date, server, modified, len, ext->tipo);
  send(connval, res, strlen(res), 0);
  send(connval, answer, len, 0);

  free(date);
  free(modified);
  free(answer);
}

/*
* Función que procesa las peticiones de tipo OPTIONS.
*/
void OPTIONS_process(int connval, char* server, char *path, extension *ext, char *vars){
  char *date, res[MAX_BUF];

  /* Calculamos la fecha actual. */
  date = get_date();
  if ( date == NULL ) {
    send_error(connval, INTERNAL_SERVER, server);
    return;
  }

  /* Enviamos el mensaje con los metodos implementados. */
  sprintf(res,"HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nAllow: GET, POST, OPTIONS\r\nConnection: keep-alive\r\n\r\n",date,server);
  send(connval, res, strlen(res), 0);
  free(date);
}

/*
* Función que corre un script especificado y devuelve su respuesta.
*/
char* run_script(int connval, char* server, char *path, extension *ext, char *vars){
  FILE *fp;
  char command[MAX_BUF]; //TODO 1000?
  char *ret;
  int len;

  /* Según la extension construimos el comando, en ambos casos enviando las variables
     con un pipe para que lleguen por stdin. */
  if(strcmp(ext->ext,"py") == 0)
    sprintf(command, "echo \"%s\" | %s %s", vars, PY, path);
  else if(strcmp(ext->ext,"php") == 0)
    sprintf(command, "echo \"%s\" | %s %s", vars, PHP, path);
  else{
    send_error(connval, BAD_REQUEST, server);
    return NULL;
  }

  /* Abrimos una pipe para leer el resultado del comando con popen. */
  fp = popen(command, "r");
  if(!fp){
    syslog(LOG_ERR, "Error creating the pipe.");
    send_error(connval, INTERNAL_SERVER, server);
    return NULL;
  }

  /* Reservamos espacio para la respuesta y la leemos. */
  ret = (void*)calloc((MAX_BUF + 1), sizeof(char));
  if(!ret){
    syslog(LOG_ERR, "Error allocating ret.");
    send_error(connval, INTERNAL_SERVER, server);
    pclose(fp);
    return NULL;
  }
  len = fread(ret, 1, MAX_BUF, fp);
  if (len < 0){
    syslog(LOG_ERR, "Error reading the pipe.");
    send_error(connval, INTERNAL_SERVER, server);
    free(ret);
    pclose(fp);
    return NULL;
  }

  /* Cerramos la pipe y devolvemos el resultado. */
  if (pclose(fp) == -1){
    syslog(LOG_ERR, "Error closing the pipe.");
    free(ret);
    send_error(connval, INTERNAL_SERVER, server);
    return NULL;
  }
  return (char*)ret;
}

/*
* Función que devuelve la fecha actual en el formato para las cabeceras HTTP.
*/
char * get_date(){
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

/*
* Función que devuelve la fecha de ultima modificacion de un fichero
* en el formato para las cabeceras HTTP.
*/
char * get_mod_date(char * path){
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

/*
* Función que coge las variables en el formato var1=valor1&var2=valor2... y las
* devuelve valor1 valor2... separadas por espacios para usarlas en los scripts.
*/
char* clean_vars(char *body)
{
  char *clean_str, *ret;
  int flag = 1, i, j;

  /* Si body está vacío devolvemos NULL. */
  if(!body || *body == 0)
    return NULL;

  /* Reservamos memoria para devolver la cadena limpia. */
  clean_str = strdup(body);
  ret = body;
  i = 0;
  while(flag)
  {
    /* Buscamos el primer = */
    if(!(ret = strchr(ret, '='))){
      /* Si no hay = esta mal formateado. */
      free(clean_str);
      return NULL;
    }
    /* Guarda la variable  (hasta encontrar & o fin de cadena). */
    for (j = 1; ret[j] != '&' && ret[j] != 0; j++)
    {
      clean_str[i] = ret[j];
      i++;
    }
    /* Si es fin de cadena dejamos el bucle. */
    if(ret[j] == 0)
    {
      flag = 0;
      break;
    }
    /* Si no es el final ponemos un espacio entre esta y la siguiente variable y seguimos. */
    ret = ret + j;
    clean_str[i] = ' ';
    i++;
  }
  /* Cerramos la string limpia. */
  clean_str[i] = 0;
  return clean_str;
}
