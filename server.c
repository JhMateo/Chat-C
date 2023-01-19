#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define SERVER_PORT	6543
#define SERVER_ADDRESS "127.0.0.1"
#define MAXLINE	520
#define MAXMSJ 477
#define MAXNAME 32
#define MAXCLIENTS 10
#define TRUE 1

pthread_mutex_t semaforo_clientes;

struct vector{
	int socket;
	char name[MAXNAME];
	int sign_in;
};

struct vector *vectorClientes[MAXCLIENTS];
int clientes_online = 0;

int crear_socket(){
    int sockfd;
    struct sockaddr_in server_adr;
    int longitud;

    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        perror("Error: Imposible crear socket\n");
        exit(2);
    }

    bzero((char *)&server_adr, sizeof(server_adr));
    server_adr.sin_port = htons(SERVER_PORT);
    server_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_adr.sin_family = PF_INET;

    if(bind(sockfd, (struct sockaddr *)&server_adr, sizeof(server_adr)) == -1){
        perror("Error: bind\n");
        exit(3);
    }

    longitud = sizeof(server_adr);

    if (getsockname(sockfd, (struct sockaddr *) &server_adr, &longitud)) {
        perror("Error: Obtencion del nombre del sock\n");
        exit (4) ;
    }

    return(sockfd);
}

void mostrar_comandos(int id){
    char *comandos = "\n[LISTA COMANDOS]\n_usuarios -> Da una lista de los usuarios conectados.\n_nombre_ msj -> Envia mensaje privado a quien le pertenezca el nombre\n_salir -> Termina la sesión en el chat.\n\nSi quiere enviar un mensaje, escribelo y dale enter\n\n";
    if(send(vectorClientes[id]->socket, comandos, strlen(comandos), 0) < 0){
            printf("Error al enviar comandos\n");
            fflush(stdout);
    }
}

void mostrar_usuarios(int id){
    char usuarios[100];
    memset(usuarios,0,100);
    for (int i = 0; i < clientes_online; i++){
        sprintf(usuarios, "  - %s\n", vectorClientes[i]->name);
        if(i != id && vectorClientes[i]->sign_in == 1 && send(vectorClientes[id]->socket, usuarios, 100, 0) < 0){
            printf("Error al enviar los usuarios\n");
            break;
        }
    }
}

void limpiar_cadena(char *cadena, int tamanio) {
       for(int i = 0; i < tamanio; i++) {
        if(cadena[i] == '\r' || cadena[i] == '\n') {
            cadena[i] = '\0';
        }
    }
}

int comprobar_nombre(int sock, char *nombre){
    pthread_mutex_lock(&semaforo_clientes);
    for (int i = 0; i < clientes_online; i++){
        int resultado = strcmp(vectorClientes[i]->name, nombre);
        if (resultado == 0) {
            char *error = "!!: Un cliente con este nombre está online, prueba con otro nombre.\n\n";
            if (send(sock, error, strlen(error), 0) < 0) {
                fprintf(stderr, "Error al enviar respuesta\n");
            }
            pthread_mutex_unlock(&semaforo_clientes);
            return -1;
        }
    }
    pthread_mutex_unlock(&semaforo_clientes);
    return 0;
}

void salir(int id){
    pthread_mutex_lock(&semaforo_clientes);
    close(vectorClientes[id]->socket);
    vectorClientes[id] = NULL;
    clientes_online--;
    printf("Clientes conectados: %d\n", clientes_online);
    pthread_mutex_unlock(&semaforo_clientes);
}

void msj_privado(int id, char *mensaje){
    char buffer[MAXLINE];
    memset(buffer, 0, MAXLINE);
    char *tok_name = strtok(mensaje, "_");
    char *msj_priv = strtok(NULL, "");

    limpiar_cadena(tok_name, strlen(tok_name));
    
    pthread_mutex_lock(&semaforo_clientes);
    for (int i = 0; i < clientes_online; i++){
        sprintf(buffer, "_%s:%s", vectorClientes[id]->name, msj_priv);
        if(i != id && strncmp(vectorClientes[i]->name, tok_name, strlen(tok_name)) == 0 && send(vectorClientes[i]->socket, buffer, MAXLINE, 0) < 0){
            printf("Error al enviar el mensaje\n");
            break;
        }
        buffer[MAXLINE] = '\0';
        pthread_mutex_unlock(&semaforo_clientes);
    }
    printf("[%s] msj: %s\n", vectorClientes[id]->name, msj_priv);                     
    fflush(stdout);	
    pthread_mutex_unlock(&semaforo_clientes);
}

void* manejo_clientes(void* p){
    int *ide, id;
	ide = (int* ) p;
	id = *ide;
	char nombre[MAXNAME];
	int i;
    char *login = "Ingrese nombre: ";

    do{
        memset(nombre, 0, MAXNAME);

        if(send(vectorClientes[id]->socket, login, strlen(login), 0) < 0){
            printf("Error al pedir el nombre\n");
            break;
        }
    
	    recv(vectorClientes[id]->socket, nombre, MAXNAME, 0);
        
        limpiar_cadena(nombre, MAXNAME);

    }while (comprobar_nombre(vectorClientes[id]->socket, nombre) != 0);
    
    strcpy(vectorClientes[id]->name, nombre);
    vectorClientes[id]->sign_in = 1;

    mostrar_comandos(id);
    
    char mensaje[MAXMSJ] = "", buffer[MAXLINE] = "";
    char *salir_chat = "_salir";
    char *usuarios = "_usuarios";
    
	while(TRUE){
        memset(mensaje, 0, MAXMSJ);
        memset(buffer, 0, MAXLINE);

        int bytes = recv(vectorClientes[id]->socket, mensaje, MAXMSJ, 0);

        if (bytes < 0){
            fprintf(stderr, "Error al recibir mensaje\n");
            salir(id);
            break;
        } else if (bytes == 0){
            salir(id);
            break;
        }
        
        if (mensaje[0] == '_'){
            if (strncmp(mensaje, salir_chat, strlen(salir_chat)) == 0){
                salir(id);
                break;
            }else if(strncmp(mensaje, usuarios, strlen(usuarios)) == 0){
                mostrar_usuarios(id);
            }else{
                msj_privado(id, mensaje);
            }
        }else{
            sprintf(buffer, "> %s: %s", vectorClientes[id]->name, mensaje);
        
            for (i = 0; i < clientes_online; i++){
                if(i != id && vectorClientes[i]->sign_in == 1 && send(vectorClientes[i]->socket, buffer, MAXLINE, 0) < 0){
                    printf("Error al enviar el mensaje\n");
                    break;
                }
            }
            
            printf("[%s] msj: %s", vectorClientes[id]->name, mensaje);                     
            fflush(stdout);		
        }
    }
}

int main(){
    int sock_escucha, sock_servicio, status, id;
    struct sockaddr_in client_adr;
    int lg_client = sizeof(client_adr);
    char buffer[MAXLINE];

    pthread_t hilos[MAXCLIENTS];

    if((sock_escucha = crear_socket()) == -1){
        fprintf(stderr, "Error:  No se pudo crear/conectar socket\n");
        exit(2);
    }

    listen(sock_escucha, 10);

    fprintf(stdout, "Servidor en el puerto %d\n", SERVER_PORT);

    signal(SIGPIPE, SIG_IGN);

    while(TRUE){
        lg_client = sizeof(client_adr);
        if((sock_servicio = accept(sock_escucha, (struct sockaddr *) &client_adr, &lg_client)) < 0){
            fprintf(stderr, "Error aceptando peticones\n");
        }
        if(clientes_online == MAXCLIENTS){
            sprintf(buffer, "Error: No se pueden conectar más clientes\n");
            if (send(sock_servicio, buffer, MAXLINE, 0) < 0) {
                fprintf(stderr, "Error al enviar respuesta");
            }
            close(sock_servicio);
        }else{
            fprintf(stdout, "Servicio aceptado\n");

            id = clientes_online;

            vectorClientes[id] = (struct vector*)malloc(sizeof(struct vector));
            vectorClientes[id]->socket = sock_servicio;
		    vectorClientes[id]->sign_in = 0;	
            fflush(stdout);			
		    clientes_online++;

            printf("Clientes conectados: %d\n", clientes_online);

            status = pthread_create(&hilos[id], NULL, manejo_clientes, (void *) &id);
            if (status){
                fprintf(stderr, "Error al crear el hilo\n");
            }
        }
    }
    close(sock_escucha);
    return 0;
}
