#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_PORT	6543
#define SERVER_ADDRESS	"127.0.0.1"
#define MAXLINE 477
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
    char mensaje[MAXLINE];
    char *temp;

    temp = "Ingrese nombre:";

    do
    {   
        memset(mensaje, 0, MAXLINE);

        if((recv(sock, mensaje, MAXLINE, 0)) < 0){
            printf("Recepcion fallida\n");
        }
            
        mensaje[MAXLINE] = '\0';
        printf("%s", mensaje);

        fgets(mensaje, MAXLINE, stdin);
        if(send(sock, mensaje, MAXLINE, 0) < 0){
            printf("Error al enviar mensaje\n");
        }
    } while (strncmp(mensaje, temp, strlen(temp)) == 0 || mensaje[0] == '!');
}

void *recibir_msj(void* p){
    int *socket, sock;
    char mensaje[MAXLINE];
	socket = (int* ) p;
	sock = *socket;

    memset(mensaje, 0, MAXLINE);

    while (TRUE){
        if((recv(sock, mensaje, MAXLINE, 0)) < 0)
        printf("Recepcion fallida\n");
        
        mensaje[MAXLINE] = '\0';
        printf("%s", mensaje);
    }
}

int main()
{
    int sock, status;
    char mensaje[MAXLINE];

    pthread_t hilo;
    
    sock = crear_socket();
    guardar_nombre(sock);

    if((recv(sock, mensaje, MAXLINE, 0)) < 0)
        printf("Recepcion fallida\n");
        
    mensaje[MAXLINE] = '\0';
    printf("%s", mensaje);
    
    while(TRUE){   
        memset(mensaje, 0, MAXLINE);
        printf("Ingrese mensaje a enviar: "); fgets(mensaje, MAXLINE, stdin);
        if(send(sock, mensaje, MAXLINE, 0) < 0){
            printf("Error al enviar mensaje\n");
            exit(-1);
        }
        
        if (status = pthread_create(&hilo, NULL, recibir_msj, (void *)&sock)){
            fprintf(stderr, "Error al crear el hilo\n");
            break;
        }
    }
    close(sock);
    return 0;
}
