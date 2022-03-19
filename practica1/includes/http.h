/** 
 * @brief Defines the functionality associated with the requests made to the server.
 * 
 * @file request_response.h
 * @author Pair number 15 Redes II Group 2361
 * @version 1.0
 * @date 11-02-2020 
 * @copyright GNU Public License
 */

#include <stdlib.h>
#include <syslog.h>
#include "picohttpparser.h"

/** @struct HTTPRequest
 * 
 *  @brief Holds the fields of a parsed HTTP request.
 */
typedef struct {
  const char *method; /*Method of the request: POST, GET, OPTIONS*/
  size_t method_len; /*Length of method*/
  const char *path; /*Everything after the first '/' character, path to the requested object*/
  size_t path_len; /*Length of path*/
  int minor_version; /*HTTP 1.0 or 1.1*/
  struct phr_header headers[100]; /*Headers of the request*/
  size_t num_headers; /*Number of headers*/
  char *body; /*Body of the request*/
} HTTPRequest;



/** 
 * @brief Processes the request of the client associated to the given socket, parsing its fields and 
 * executing a specific functionality regarding the method of the request.
 * @param connDescr int - descriptor of the socket associated to a connection
 * @param request HTTPRequest - structure to store the fields of the parsed request
 * @return int - error code
 *	
 * @author: Pair number 15 Redes II Group 2361
 */

int process_petitions(int connDescr, char* server_signature, char* server_root);
int send_file(int connDescr, char* path);
void GET(int sockfd, char* path, char* server_signature, char* server_root);
void POST(int sockfd, char* path, char* server_signature, char* server_root);
void OPTIONS(int sockfd, char* server_signature);
char* get_content_type(char* path);