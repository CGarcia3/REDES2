#ifndef SOCKETS_H
#define SOCKETS_H

/**
 * Sockets.h se encarga de abrir los sockets y escuchar las peticiones.
 * @author Carlos Garcia
 * @author Samai Garcia
 * */


/*
* Funcion que crea un socket TCP/IP, lo liga al puerto y lo deja escuchando.
*/
int init_socket(int port, int max_size);

/*
* Funcion que acepta la conexion de un cliente.
*/
int accept_socket(int socket_descr);

#endif