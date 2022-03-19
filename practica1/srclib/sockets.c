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

/**
 * Inicializa el socket con los datos del fichero de configuracion, conprobando que se haga correctamente
 */ 
int init_socket(int port, int max_size) {
    int socket_descr;
    struct sockaddr_in localaddr;


    socket_descr = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_descr < 0) {
        syslog(LOG_ERR, "Error initializing socket");
        return -1;
    }

    // Queremos que el SO reutilice el socket para no esperar a que lo cierre
    if(setsockopt(socket_descr, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))!=0){
        printf("(%d)\nError setsockopt\n", errno);
        syslog(LOG_ERR, "Error socket opstions");
        close(socket_descr);
        return -1;
    }

    //Datos del socket a bindear
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero((void *) &(localaddr.sin_zero), 8);

    if (bind(socket_descr, (struct sockaddr*)&localaddr, sizeof(localaddr)) < 0) {
        printf("(%d)\n", errno);
        syslog(LOG_ERR, "Error binding");
        close(socket_descr);
        return -1;
    }
    
    //Socket en modo listen
    if (listen(socket_descr, max_size) < 0) {
        syslog(LOG_ERR, "Error listening");
        close(socket_descr);
        
        return -1;
    }

    return socket_descr;

}

/**
 * Accepts a connectiong through the socket
 */ 
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

  

