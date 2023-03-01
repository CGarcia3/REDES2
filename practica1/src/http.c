#include "../includes/http.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#define BUF_SIZE 1000
#define MAX_ARGS 10


/**
 * Procesa la petición recibida en el servidor,
 * la parsea para leer los contenidos de la cabecera,
 * y llama a la función correspondiente para realizar la
 * funcionalidad necesaria.
*/
int process_petitions(int sockfd, char* server_signature, char* server_root){
  char buf[4096], *method, *path, aux_method[10], res[BUF_SIZE];
  int pret, minor_version;
  struct phr_header headers[100];
  size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
  ssize_t rret;
  time_t t = time(NULL);

  while (1) {
      /* read the request */
      while ((rret = read(sockfd, buf + buflen, sizeof(buf) - buflen)) == -1 && errno == EINTR)
          ;
      if (rret <= 0)
          return -1;
      prevbuflen = buflen;
      buflen += rret;
      /* parse the request */
      num_headers = sizeof(headers) / sizeof(headers[0]);
      pret = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
                              &minor_version, headers, &num_headers, prevbuflen);
      if (pret > 0)
          break; /* successfully parsed the request */
      else if (pret == -1)
          return -1;
      /* request is incomplete, continue the loop */
      assert(pret == -2);
      if (buflen == sizeof(buf))
          return -1;
  }

  // Print requests info for debugging
  /*printf("request is %d bytes long\n", pret);
  printf("method is %.*s\n", (int)method_len, method);
  printf("path is %.*s\n", (int)path_len, path);
  printf("HTTP version is 1.%d\n", minor_version);
  printf("headers:\n");
  for (int i = 0; i != num_headers; ++i) {
      printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
            (int)headers[i].value_len, headers[i].value);
  }*/
  
  // Guardamos de forma correcta el método y el path
  sprintf(aux_method, "%.*s", (int)method_len, method);
  sprintf(path, "%.*s", (int)path_len, path);

  // Dependiendo del método, llamamos a una función o a otra
  if(strcmp(aux_method, "GET")==0){
    GET(sockfd, path, server_signature, server_root);
  }else if(strcmp(aux_method, "POST")==0){
    POST(sockfd, path, server_signature, server_root);
  }else if(strcmp(aux_method, "OPTIONS")==0){
    OPTIONS(sockfd, server_signature);
  }else {
    sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
    strcpy(buf, "<html><p>400 Bad Request</p></html>");
    enviar_cabecera_http_400(sockfd, server_signature, strlen(buf), res);
    send(sockfd, buf, sizeof(buf), 0);
    return 1;
  }


  return 1;
}


/**
 * Funcionalidad del método GET. Si no tiene parámetros, 
 * lee el fichero situado en el path y lo envía al cliente.
 * Si tiene, ejecuta el script correspondiente con dichos parámetros.
 */
void GET(int sockfd, char* path, char* server_signature, char* server_root){
  FILE* pf=NULL;
  char *fullpath=NULL, res[BUFSIZ], last_modified[128], *aux=NULL, args[BUF_SIZE]="", res_scr[BUF_SIZE], scr[BUF_SIZE],buf[4096];
  int size, ret;
  time_t t = time(NULL);
  struct stat s;

  // En caso de path vacío, lee el index.html
  if(strcmp(path, "/")==0){
    strcat(path, "index.html");
  }

  // Comprobamos si tiene argumentos
  if(strstr(path, "?")){

    // Separamos el path de los parámetros
    fullpath = (char*)malloc(sizeof(char)*(strlen(path)+strlen(server_root)));
    strcpy(fullpath, server_root);
    aux = strtok(path, "?");
    strcat(fullpath, aux);

    aux=strtok(NULL, "&");
    while(aux != NULL){
      strcat(args, strchr(aux, '=')+1);
      strcat(args, " ");
      aux=strtok(NULL, "&");
    }

    // Comprobamos si el fichero del path es un .py o .php para ejecutarlos
    if(!strstr(path, ".php") && !strstr(path, ".py")){
      syslog(LOG_ERR, "No es un script.\n");
      sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
      strcpy(buf, "<html><p>400 Bad Request</p></html>");
      enviar_cabecera_http_400(sockfd, server_signature, strlen(buf), res);
      send(sockfd, buf, sizeof(buf), 0);
      return;
    }

    // Dependiendo del filetype, el comando es distinto
    if(strstr(path, ".py")){
      sprintf(scr, "python %s %s", fullpath, args);
    }else if(strstr(path, ".php")){
      sprintf(scr, "php %s %s", fullpath, args);
    }

    // Comprobamos si el fichero existe
    if(access(fullpath, F_OK) != 0 ) {
      sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
      strcpy(buf, "<html><p>404 Not Found</p></html>");
      enviar_cabecera_http_404(sockfd, server_signature, strlen(buf), res);
      send(sockfd, buf, sizeof(buf), 0);
      syslog(LOG_ERR, "Error abriendo fichero. No encontrado\n");
      return;
    }
    
    // Ejecutamos el script, el resultado se guarda en pf, y lo leemos para guardarlo en res_scr
    pf = popen(scr, "r");
    if(!pf){
      sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
      strcpy(buf, "<html><p>400 Bad Request</p></html>");
      enviar_cabecera_http_400(sockfd, server_signature, strlen(buf), res);
      send(sockfd, buf, sizeof(buf), 0);
      syslog(LOG_ERR, "Error haciendo popen\n");
      return;
    }
    size = fread(res_scr, 1024, 1, pf);
    pclose(pf);

    // Abrimos el fichero con fread para ver el tamaño
    pf=fopen(fullpath, "r");
    if(!pf){
      sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
      strcpy(buf, "<html><p>404 Not Found</p></html>");
      enviar_cabecera_http_404(sockfd, server_signature, strlen(buf), res);
      send(sockfd, buf, sizeof(buf), 0);
      syslog(LOG_ERR, "Error abriendo fichero. No encontrado\n");
      return;
    }

    fseek(pf, 0L, SEEK_END);
    size = ftell(pf);
    fseek(pf, 0L, SEEK_SET);
    fclose(pf);

    //Construimos la cabecera y cuerpo del mensaje, y se envía
    strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));
    sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));

    enviar_cabecera_http(sockfd, server_signature, last_modified, size, "text/html", res);

    send(sockfd, res_scr, strlen(res_scr), 0);
    send(sockfd, res, strlen(res), 0);
    sprintf(res, "\r\n");
    send(sockfd, res, strlen(res), 0);

  // Caso sin parámetros, solo leemos fichero y enviamos contenidos
  }else{

    //Guardamos el path
    fullpath = (char*)malloc(sizeof(char)*(strlen(path)+strlen(server_root) + 1));
    strcpy(fullpath, server_root);
    strcat(fullpath, path);
    pf = fopen(fullpath, "r");
    if(pf==NULL){
      sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
      strcpy(buf, "<html><p>404 Not Found</p></html>");
      enviar_cabecera_http_404(sockfd, server_signature, strlen(buf), res);
      send(sockfd, buf, sizeof(buf), 0);
      syslog(LOG_ERR, "Error abriendo fichero. No encontrado\n");
      free(fullpath);
      return;
    }
    
    
    stat(fullpath, &s);
    size = s.st_size;
    char* type = get_content_type(path);
    if(type==NULL){
      syslog(LOG_ERR, "Filetype no soportado.\n");
      sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
      strcpy(buf, "<html><p>400 Bad Request</p></html>");
      enviar_cabecera_http_400(sockfd, server_signature, strlen(buf), res);
      send(sockfd, buf, sizeof(buf), 0);
      free(fullpath);
      fclose(pf);
      return;
    }

    //Construimos cabecera
    strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));
    sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));

    enviar_cabecera_http(sockfd, server_signature, last_modified, size, type, res);

    // Enviamos contenido en el cuerpo del mensaje
    int file = open(fullpath, O_RDONLY);
    while ((ret = read(file, res, 1024))> 0) {
      send(sockfd, res, ret, 0);
    }
    sprintf(res, "\r\n");
    send(sockfd, res, strlen(res), 0);
  }
  
  // Liberamos recursos usados
  fclose(pf);
  free(fullpath);
}

/** 
 * Funcionalidad del método POST. Ejecuta scripts .py o .php
 * y devuelve el resultado
*/
void POST(int sockfd, char* path, char* server_signature, char* server_root){
  char* fullpath, res[BUF_SIZE], scr[BUF_SIZE], res_scr[BUF_SIZE], *aux=NULL, args[BUF_SIZE]="", last_modified[128], buf[4096];
  FILE* pf=NULL;
  long size;
  time_t t = time(NULL);
  struct stat s;

  //Comprobamos si es un script py o php
  if(!strstr(path, ".php") && !strstr(path, ".py")){
    syslog(LOG_ERR, "No es un script.\n");
    sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
    strcpy(buf, "<html><p>400 Bad Request</p></html>");
    enviar_cabecera_http_400(sockfd, server_signature, strlen(buf), res);
    send(sockfd, buf, sizeof(buf), 0);
    return;
  }
  
  //Comprobamos si tiene argumentos
  if(strstr(path, "?")){

    //Separamos los argumentos del path
    fullpath = (char*)malloc(sizeof(char)*(strlen(path)+strlen(server_root)));
    strcpy(fullpath, server_root);
    aux = strtok(path, "?");
    strcat(fullpath, aux);

    aux=strtok(NULL, "&");
    while(aux != NULL){
      strcat(args, strchr(aux, '=')+1);
      strcat(args, " ");
      aux=strtok(NULL, "&");
    }

    // Creamos el comando a ejecutar dependiendo de la extension
    if(strstr(path, ".py")){
      sprintf(scr, "python %s %s", fullpath, args);
    }else if(strstr(path, ".php")){
      sprintf(scr, "php %s %s", fullpath, args);
    }

  //Caso sin argumentos
  }else{

    //Guardamos el path
    fullpath = (char*)malloc(sizeof(char)*(strlen(path)+strlen(server_root)));
    strcpy(fullpath, server_root);
    strcat(fullpath, path);
    if(strstr(path, ".py")){
      sprintf(scr, "python %s", fullpath);
    }else if(strstr(path, ".php")){
      sprintf(scr, "php %s", fullpath);
    }
  }

  //Comprobamos si el fichero existe
  if(access(fullpath, F_OK) != 0 ) {
    sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
    strcpy(buf, "<html><p>404 Not Found</p></html>");
    enviar_cabecera_http_404(sockfd, server_signature, strlen(buf), res);
    send(sockfd, buf, sizeof(buf), 0);
    syslog(LOG_ERR, "Error abriendo fichero. No encontrado\n");
    return;
  }
  
  // Ejecutamos el script y guardamos el resultado en pf y lo leemos
  pf = popen(scr, "r");
  if(!pf){
    sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
    strcpy(buf, "<html><p>400 Bad Request</p></html>");
    enviar_cabecera_http_400(sockfd, server_signature, strlen(buf), res);
    send(sockfd, buf, sizeof(buf), 0);
    syslog(LOG_ERR, "Error haciendo popen\n");
    return;
  }
  size = fread(res_scr, 1024, 1, pf);
  pclose(pf);

  //Abrimos el fichero para ver el tamaño
  pf=fopen(fullpath, "r");
  if(!pf){
    sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));
    strcpy(buf, "<html><p>404 Not Found</p></html>");
    enviar_cabecera_http_404(sockfd, server_signature, strlen(buf), res);
    send(sockfd, buf, sizeof(buf), 0);
    syslog(LOG_ERR, "Error abriendo fichero. No encontrado\n");
    return;
  }
  fseek(pf, 0L, SEEK_END);
  size = ftell(pf);
  fseek(pf, 0L, SEEK_SET);
  fclose(pf);

  //Construimos cabecera y cuerpo del mensaje
  strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));
  sprintf(res, "%.*s", (int) strlen(ctime(&t))-1, ctime(&t));

  enviar_cabecera_http(sockfd, server_signature, last_modified, size, "text/html", res);

  send(sockfd, res_scr, strlen(res_scr), 0);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "\r\n");
  send(sockfd, res, strlen(res), 0);

  //Liberamos recursos
  free(fullpath);
  
}

/** 
 * Con el método de options solo necesitamos devolver en la cabecera los 
 * métodos que se aceptan en el servidor
*/
void OPTIONS(int sockfd, char* server_signature){
  char res[BUF_SIZE];
  time_t t = time(NULL);

  sprintf(res, "HTTP/1.1 200 OK\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Allow: OPTIONS, GET, POST\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Date: %.*s\r\n", (int) strlen(ctime(&t))-1, ctime(&t));
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Server: %s\r\n", server_signature);
  send(sockfd, res, strlen(res), 0);
  
  sprintf(res, "Content-Length: 0\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Content-Type: text/html\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "\r\n");
  send(sockfd, res, strlen(res), 0);
}

/**
 * Devuelve una string con el tipo de contenido dependiendo de 
 * la extension del fichero
 * */
char* get_content_type(char* path) {


    if (strstr(path, ".txt")) {
        return "text/plain";
    } else if (strstr(path, ".html") || strstr(path, ".htm")) {
        return "text/html";
    } else if (strstr(path, ".gif")) {
        return "image/gif";
    } else if (strstr(path, ".jpeg") || strstr(path, ".jpg")) {
        return "image/jpeg";
    } else if (strstr(path, ".mpeg") || strstr(path, ".mpg")) {
        return "video/mpeg";
    } else if (strstr(path, ".doc") || strstr(path, ".docx")) {
        return "application/msword";
    } else if (strstr(path, ".pdf")) {
        return "application/pdf";
    } else {
        return NULL;
    }
}

/** 
 * Construye y envía la cabecera http estandar de 200OK
 */ 
void enviar_cabecera_http(int sockfd, char* server_signature, char* last_modified, int size, char* type, char* date){
  char res[BUF_SIZE];
  sprintf(res, "HTTP/1.1 200 OK\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Date: %s\r\n", date);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Server: %s\r\n", server_signature);
  send(sockfd, res, strlen(res), 0);
  
  sprintf(res, "Last-Modified: %s\r\n", last_modified);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Content-Length: %d\r\n", size);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Content-Type: %s\r\n", type);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "\r\n");
  send(sockfd, res, strlen(res), 0);
}

/** 
 * Construye y envía la cabecera http estandar de 400OK
 */ 
void enviar_cabecera_http_400(int sockfd, char* server_signature, int size, char* date){
  char res[BUF_SIZE];
  sprintf(res, "HTTP/1.1 400 Bad Request\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Date: %s\r\n", date);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Server: %s\r\n", server_signature);
  send(sockfd, res, strlen(res), 0);
  
  sprintf(res, "Content-Length: %d\r\n", size);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Content-Type: text/html\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "\r\n");
  send(sockfd, res, strlen(res), 0);
}

/** 
 * Construye y envía la cabecera http estandar de 404OK
 */ 
void enviar_cabecera_http_404(int sockfd, char* server_signature, int size, char* date){
  char res[BUF_SIZE];
  sprintf(res, "HTTP/1.1 404 Not Found\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Date: %s\r\n", date);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Server: %s\r\n", server_signature);
  send(sockfd, res, strlen(res), 0);
  
  sprintf(res, "Content-Length: %d\r\n", size);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Content-Type: text/html\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "\r\n");
  send(sockfd, res, strlen(res), 0);
}