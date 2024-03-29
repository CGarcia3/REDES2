#include "../includes/sockets.h"
#include "../includes/http.h"
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "confuse.h"

#define MAX 80
#define SA struct sockaddr

int sockfd, connection_fd;
char* server_root, *server_signature;

/** 
 * En caso de SIGINT, queremos que cierre los descriptores de forma correcta
*/
void termination_handler (int signum){
    close(connection_fd);
    close(sockfd);
    free(server_root);
    free(server_signature);
    exit(0);
}

int main()
{
    int pid, i;
    char conf[1024], *tok;
    unsigned long listen_port=1, max_clients=1;
    FILE *pf=NULL;

    struct sigaction new_action, old_action;

    chroot("/www/");

    // Demonizamos proceso
    pid_t pid_daemon;
    pid_daemon = fork();
    
    if (pid_daemon > 0) exit(EXIT_SUCCESS);
    printf("PID del proceso: %d.\n", getpid());
    if(setsid() < 0){
        exit(EXIT_FAILURE);
    }
    close(STDIN_FILENO); 
    close(STDOUT_FILENO); 
    close(STDERR_FILENO);

    // Leemos fichero de configuraciones y guardamos los datos
    pf = fopen("server.conf", "r");
    if(pf == NULL){
        syslog(LOG_ERR, "Error abriendo fichero server.conf");
        return -1;
    }
    while (fgets(conf, 1024, pf)) {
        if (strstr(conf, "listen_port") != NULL) {
            tok = strtok(conf, "=");
            tok = strtok(NULL, "\n");
            tok += 1;
            listen_port = atol(tok);
        }else if (strstr(conf, "max_clients") != NULL) {
            tok = strtok(conf, "=");
            tok = strtok(NULL, "\n");
            tok += 1;
            max_clients = atol(tok);
        }else if (strstr(conf, "server_root") != NULL) {
            tok = strtok(conf, "=");
            tok = strtok(NULL, "\n");
            server_root = strdup(tok);
            for(i=1; server_root[i]!='\0'; i++){
                server_root[i-1]=server_root[i];
            }
            server_root[i-1]='\0';
        }else if (strstr(conf, "server_signature") != NULL) {
            tok = strtok(conf, "=");
            tok = strtok(NULL, "\n");
            server_signature = strdup(tok);
            for(i=1; server_signature[i]!='\0'; i++){
                server_signature[i-1]=server_signature[i];
            }
            server_signature[i-1]='\0';
        }
    }
    fclose(pf);

    // Configuramos la respuesta al SIGINT
    new_action.sa_handler = termination_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    sigaction (SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction (SIGINT, &new_action, NULL);

   
    // Inicializamos el socket
    sockfd = init_socket(listen_port, max_clients);
    if(sockfd == -1){
        syslog(LOG_ERR, "Error abriendo socket.");
        close(sockfd);
        return -1;
    }

    // Loop principal, cada vez que recibamos una peticion se crea un proceso
    // con fork para que la atienda.
    while (1){
        
        connection_fd = accept_socket(sockfd);
        if(connection_fd == -1){
            continue;
        }

        if ((pid = fork())==0){
            process_petitions(connection_fd, server_signature, server_root);
            exit(EXIT_SUCCESS);
        }
        close(connection_fd);
    }    
    return 1;
}