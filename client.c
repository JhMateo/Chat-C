#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define SERVER_PORT	6543
#define SERVER_ADDRESS	"127.0.0.1"
#define MAXLINE 477
#define MAXNAME 32
#define TRUE 1

int crear_socket(){
    int sock; 
    struct sockaddr_in server;

    printf("Creando socket...\n");
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        printf("Error creando socket\n");
        exit(-1);
    }
    printf("Socket creado\n");

    server.sin_family = PF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
     
    if(connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1){
        printf("Error de conexion\n");
        exit(-1);
    }
    printf("Conexion exitosa\n\n");
    return sock;
}

void guardar_nombre(int sock){
    char nombre[MAXNAME];
    char mensaje[MAXLINE];
    char *temp;

    temp = "Ingrese nombre:";

    memset(mensaje, 0, MAXLINE);

    if((recv(sock, mensaje, MAXLINE, 0)) < 0){
        printf("Recepcion fallida\n");
        exit(EXIT_FAILURE);
    }

    while (strncmp(mensaje, temp, strlen(temp)) == 0 || mensaje[0] == '!'){

        printf("%s", mensaje);
        fflush(stdout);

        memset(nombre, 0, MAXNAME);

        fgets(nombre, MAXNAME, stdin);
        if(send(sock, nombre, MAXNAME, 0) < 0){
            printf("Error al enviar mensaje\n");
            exit(EXIT_FAILURE);
        }

        memset(mensaje, 0, MAXLINE);

        if((recv(sock, mensaje, MAXLINE, 0)) < 0){
            printf("Recepcion fallida\n");
            exit(EXIT_FAILURE);
        }

        printf("%s", mensaje);
        fflush(stdout);

    }
}

void cifrar_msj(char *mensaje){
    int msj_len = strlen(mensaje);
    for (int i = 0; i < msj_len; i++){
        mensaje[i] = mensaje[i] + 1;
    }
}

void descifrar_msj(char *mensaje){
    int msj_len = strlen(mensaje);
    for (int i = 0; i < msj_len; i++){
        mensaje[i] = mensaje[i] - 1;
    }
}

void *recibir_msj(void* p){
    int *socket, sock;
    char mensaje[MAXLINE];
	socket = (int* ) p;
	sock = *socket;

    memset(mensaje, 0, MAXLINE);

    while (TRUE){
        int bytes = recv(sock, mensaje, MAXLINE, 0);
        if(bytes < 0){
            printf("Recepcion fallida\n");
            exit(EXIT_FAILURE);
        } else if (bytes == 0) {
            printf("Conexión cerrada\n");
            exit(EXIT_SUCCESS);
        }

        if(mensaje[0] == '_'){
            char *receptor = strtok(mensaje, " ");
            char *msj_descifrado = strtok(NULL, "");
            descifrar_msj(msj_descifrado);
            sprintf(mensaje, "%s %s", receptor, msj_descifrado);
            printf("%s", mensaje);
            fflush(stdout);
        }else{
            printf("%s", mensaje);
            fflush(stdout);
        }

    }
}

int main(){
    int sock, status;
    char mensaje[MAXLINE];

    pthread_t hilo;
    
    sock = crear_socket();
    guardar_nombre(sock);

    signal(SIGPIPE, SIG_IGN);
            
    if (status = pthread_create(&hilo, NULL, recibir_msj, (void *)&sock)){
            fprintf(stderr, "Error al crear el hilo\n");
        }

    char buffer[MAXLINE];

    while(TRUE){
        memset(mensaje, 0, MAXLINE);
        memset(buffer, 0, MAXLINE);
        fgets(mensaje, MAXLINE, stdin);

        strcpy(buffer, mensaje);

        char *receptor = strtok(buffer, " ");
        if(receptor[0] == '_' && receptor[strlen(receptor)-1] == '_'){
            char *msj_cifrado = strtok(NULL, "");
            cifrar_msj(msj_cifrado);
            sprintf(buffer, "%s %s", receptor, msj_cifrado);
            if(send(sock, buffer, MAXLINE, 0) < 0){
                printf("Error al enviar mensaje\n");
                exit(EXIT_FAILURE);
            }
        }else if(send(sock, mensaje, MAXLINE, 0) < 0){
            printf("Error al enviar mensaje\n");
            exit(EXIT_FAILURE);
        }
    }
    close(sock);
    return 0;
}
