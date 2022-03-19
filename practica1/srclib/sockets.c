/*
* Fichero:  sockets.c
* Autores:  Carlos García Toledano
            Samai García González
*/
#include "../includes/sockets.h"

#include <netinet/in.h>
#include <syslog.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>


int init_socket(int port, int max_size) {
    int socket_descr;
    struct sockaddr_in localaddr;

    socket_descr = socket(AF_INET, SOCK_STREAM, 0);
    

    if (socket_descr < 0) {
        syslog(LOG_ERR, "Error initializing socket");
        return -1;
    }
    if(setsockopt(socket_descr, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))!=0){
        printf("(%d)\nError setsockopt\n", errno);
        syslog(LOG_ERR, "Error socket opstions");
        close(socket_descr);
        return -1;
    }

    localaddr.sin_family = AF_INET;                /* Familia TCP/IP. */
    localaddr.sin_port = htons(port);       /* Se asigna el puerto. */
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Se aceptan todas las direcciones. */
    bzero((void *) &(localaddr.sin_zero), 8);

    if (bind(socket_descr, (struct sockaddr*)&localaddr, sizeof(localaddr)) < 0) {
        printf("(%d)\n", errno);
        syslog(LOG_ERR, "Error binding");
        close(socket_descr);
        return -1;
    }
    
    if (listen(socket_descr, max_size) < 0) {
        syslog(LOG_ERR, "Error listening");
        close(socket_descr);
        
        return -1;
    }

    return socket_descr;

}

int accept_socket(int socket_descr){

    int val, len;
    struct sockaddr localaddr;

    len = sizeof(localaddr);

    val = accept(socket_descr, (struct sockaddr*)&localaddr, (socklen_t*)&len);
    if(val == -1){
        printf("%dAA\n", errno);
        if(errno == EINTR){
            return -1;
        }

        syslog(LOG_ERR, "Error accepting connection");
        return -1;
    }

    syslog(LOG_INFO, "Server accepted client");

    return val;

}

  

