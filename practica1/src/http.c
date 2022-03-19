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



int process_petitions(int sockfd, char* server_signature, char* server_root){
  char buf[4096], *method, *path, aux_method[10];
  int pret, minor_version;
  struct phr_header headers[100];
  size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
  ssize_t rret;

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

  printf("request is %d bytes long\n", pret);
  printf("method is %.*s\n", (int)method_len, method);
  printf("path is %.*s\n", (int)path_len, path);
  printf("HTTP version is 1.%d\n", minor_version);
  printf("headers:\n");
  for (int i = 0; i != num_headers; ++i) {
      printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
            (int)headers[i].value_len, headers[i].value);
  }
  
  sprintf(aux_method, "%.*s", (int)method_len, method);
  sprintf(path, "%.*s", (int)path_len, path);
  if(strcmp(aux_method, "GET")==0){
    GET(sockfd, path, server_signature, server_root);
  }else if(strcmp(aux_method, "POST")==0){
    POST(sockfd, path, server_signature, server_root);
  }else if(strcmp(aux_method, "OPTIONS")==0){
    OPTIONS(sockfd, server_signature);
  }else {
    send(sockfd, "HTTP/1.1 400 Bad Request\r\n\r\n", sizeof("HTTP/1.1 400 Bad Request\r\n"), 0);
    return 1;
  }


  return 1;
}

void GET(int sockfd, char* path, char* server_signature, char* server_root){
  FILE* pf=NULL;
  char *fullpath=NULL, res[BUFSIZ], last_modified[128], *aux=NULL, args[BUF_SIZE]="", res_scr[BUF_SIZE], scr[BUF_SIZE];
  int size, ret;
  time_t t = time(NULL);
  struct stat s;

  if(strcmp(path, "/")==0){
    strcat(path, "index.html");
  }

  //Checks if the url has arguments
  if(strstr(path, "?")){

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


    if(!strstr(path, ".php") && !strstr(path, ".py")){
      syslog(LOG_ERR, "No es un script.\n");
      sprintf(res, "HTTP/1.1 400 Bad Request\r\n");
      send(sockfd, res, sizeof(res), 0);
      printf("Not a scripting file\n");
      return;
    }

    if(strstr(path, ".py")){
      sprintf(scr, "python %s %s", fullpath, args);
    }else if(strstr(path, ".php")){
      sprintf(scr, "php %s %s", fullpath, args);
    }


    //check if file exists
    if(access(fullpath, F_OK) != 0 ) {
      send(sockfd, "HTTP/1.1 404 Not Found\r\n\r\n", sizeof("HTTP/1.1 404 Not Found\r\n\r\n"), 0);
      syslog(LOG_ERR, "Error abriendo fichero. No encontrado\n");
      printf("Error abriendo fichero access, %s, %d\n", fullpath, errno);
      return;
    }
    
    pf = popen(scr, "r");
    if(!pf){
      send(sockfd, "HTTP/1.1 400 Bad Request\r\n\r\n", sizeof("HTTP/1.1 404 Not Found\r\n\r\n"), 0);
      syslog(LOG_ERR, "Error haciendo popen\n");
      printf("Error abriendo fichero popen\n");
      return;
    }
    size = fread(res_scr, 1024, 1, pf);
    pclose(pf);

    pf=fopen(fullpath, "r");
    if(!pf){
      send(sockfd, "HTTP/1.1 404 Not Found\r\n\r\n", sizeof("HTTP/1.1 404 Not Found\r\n\r\n"), 0);
      syslog(LOG_ERR, "Error abriendo fichero. No encontrado\n");
      printf("Error abriendo fichero.\n");
      return;
    }

    fseek(pf, 0L, SEEK_END);
    size = ftell(pf);
    fseek(pf, 0L, SEEK_SET);
    fclose(pf);

    strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));

    sprintf(res, "HTTP/1.1 200 OK\r\n");
    send(sockfd, res, strlen(res), 0);
    sprintf(res, "Date: %.*s\r\n", (int) strlen(ctime(&t))-1, ctime(&t));
    send(sockfd, res, strlen(res), 0);
    sprintf(res, "Server: %s\r\n", server_signature);
    send(sockfd, res, strlen(res), 0);
    
    sprintf(res, "Last-Modified: %s\r\n", last_modified);
    send(sockfd, res, strlen(res), 0);
    sprintf(res, "Content-Length: %d\r\n", size);
    send(sockfd, res, strlen(res), 0);
    sprintf(res, "Content-Type: text/html\r\n");
    send(sockfd, res, strlen(res), 0);
    sprintf(res, "\r\n");
    send(sockfd, res, strlen(res), 0);

    send(sockfd, res_scr, strlen(res_scr), 0);
    send(sockfd, res, strlen(res), 0);
    sprintf(res, "\r\n");
    send(sockfd, res, strlen(res), 0);

  }else{
    fullpath = (char*)malloc(sizeof(char)*(strlen(path)+strlen(server_root)));
    strcpy(fullpath, server_root);
    strcat(fullpath, path);
    pf = fopen(fullpath, "r");
    if(pf==NULL){
      send(sockfd, "HTTP/1.1 404 Not Found\r\n\r\n", sizeof("HTTP/1.1 404 Not Found\r\n\r\n"), 0);
      syslog(LOG_ERR, "Error abriendo fichero. No encontrado\n");
      printf("Error abriendo fichero, %s\n", fullpath);
      return;
    }
    
    
    stat(fullpath, &s);
    size = s.st_size;
    char* type = get_content_type(path);
    if(type==NULL){
      syslog(LOG_ERR, "Filetype no soportado.\n");
      sprintf(res, "HTTP/1.1 400 Bad Request\r\n");
      send(sockfd, res, sizeof(res), 0);
      return;
    }
    strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));


    sprintf(res, "HTTP/1.1 200 OK\r\n");
    send(sockfd, res, strlen(res), 0);
    sprintf(res, "Date: %.*s\r\n", (int) strlen(ctime(&t))-1, ctime(&t));
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

    int file = open(fullpath, O_RDONLY);
    while ((ret = read(file, res, 1024))> 0) {
      send(sockfd, res, ret, 0);
    }
    sprintf(res, "\r\n");
    send(sockfd, res, strlen(res), 0);
  }
  
  free(fullpath);
}

void POST(int sockfd, char* path, char* server_signature, char* server_root){
  char* fullpath, res[BUF_SIZE], scr[BUF_SIZE], res_scr[BUF_SIZE], *aux=NULL, args[BUF_SIZE]="", last_modified[128];
  FILE* pf=NULL;
  long size;
  time_t t = time(NULL);
  struct stat s;
  if(!strstr(path, ".php") && !strstr(path, ".py")){
    syslog(LOG_ERR, "No es un script.\n");
    sprintf(res, "HTTP/1.1 400 Bad Request\r\n");
    send(sockfd, res, sizeof(res), 0);
    printf("Not a scripting file\n");
    return;
  }
  

  if(strstr(path, "?")){

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

    if(strstr(path, ".py")){
      sprintf(scr, "python %s %s", fullpath, args);
    }else if(strstr(path, ".php")){
      sprintf(scr, "php %s %s", fullpath, args);
    }
  }else{

    fullpath = (char*)malloc(sizeof(char)*(strlen(path)+strlen(server_root)));
    strcpy(fullpath, server_root);
    strcat(fullpath, path);
    if(strstr(path, ".py")){
      sprintf(scr, "python %s", fullpath);
    }else if(strstr(path, ".php")){
      sprintf(scr, "php %s", fullpath);
    }
  }

  //check if file exists
  if(access(fullpath, F_OK) != 0 ) {
    send(sockfd, "HTTP/1.1 404 Not Found\r\n\r\n", sizeof("HTTP/1.1 404 Not Found\r\n\r\n"), 0);
    syslog(LOG_ERR, "Error abriendo fichero. No encontrado\n");
    printf("Error abriendo fichero access, %s, %d\n", fullpath, errno);
    return;
  }
  
  pf = popen(scr, "r");
  if(!pf){
    send(sockfd, "HTTP/1.1 400 Bad Request\r\n\r\n", sizeof("HTTP/1.1 404 Not Found\r\n\r\n"), 0);
    syslog(LOG_ERR, "Error haciendo popen\n");
    printf("Error abriendo fichero popen\n");
    return;
  }
  size = fread(res_scr, 1024, 1, pf);
  pclose(pf);

  pf=fopen(fullpath, "r");
  if(!pf){
    send(sockfd, "HTTP/1.1 404 Not Found\r\n\r\n", sizeof("HTTP/1.1 404 Not Found\r\n\r\n"), 0);
    syslog(LOG_ERR, "Error abriendo fichero. No encontrado\n");
    printf("Error abriendo fichero.\n");
    return;
  }
  fseek(pf, 0L, SEEK_END);
  size = ftell(pf);
  fseek(pf, 0L, SEEK_SET);
  fclose(pf);


  strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));

  sprintf(res, "HTTP/1.1 200 OK\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Date: %.*s\r\n", (int) strlen(ctime(&t))-1, ctime(&t));
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Server: %s\r\n", server_signature);
  send(sockfd, res, strlen(res), 0);
  
  sprintf(res, "Last-Modified: %s\r\n", last_modified);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Content-Length: %ld\r\n", size);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "Content-Type: text/html\r\n");
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "\r\n");
  send(sockfd, res, strlen(res), 0);

  send(sockfd, res_scr, strlen(res_scr), 0);
  send(sockfd, res, strlen(res), 0);
  sprintf(res, "\r\n");
  send(sockfd, res, strlen(res), 0);

  
  free(fullpath);
  
}

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