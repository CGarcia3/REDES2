/**
 * HTTP.h se encarga de procesar las peticiones http que 
 * llegan al servidos, parseándolas y ejecutando la funcionalidad
 * correspondiente dependiendo del método usadoy contestando al cliente
 * con otro mensaje http.
 * @author Carlos Garcia
 * @author Samai Garcia
 * */

#include <stdlib.h>
#include <syslog.h>
#include "picohttpparser.h"

int process_petitions(int connDescr, char* server_signature, char* server_root);
void GET(int sockfd, char* path, char* server_signature, char* server_root);
void POST(int sockfd, char* path, char* server_signature, char* server_root);
void OPTIONS(int sockfd, char* server_signature);
char* get_content_type(char* path);
void enviar_cabecera_http(int sockfd, char* server_signature, char* last_modified, int size, char* type, char* date);
void enviar_cabecera_http_400(int sockfd, char* server_signature, int size, char* date);
void enviar_cabecera_http_404(int sockfd, char* server_signature, int size, char* date);