/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAX 1024

#define MAX_DATASIZE 1024

typedef struct dado{
    unsigned long   chave;
    unsigned char   *bytes;
    struct dado     *next;
}Dado;

typedef struct dados{
    Dado *first;
    unsigned int numElems;
}Dados;

int init();
int create(int id, char *valor);
int inserir(Dado *dado);
int get(int id);
int put(int id, char *newValue);
int del(int id);
int processRequest(char *cmd);
void printData();
char *setResponse(int status);

unsigned long chave;
char res[MAX_DATASIZE];
Dados *D;

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char *command;
	int rv;
	int status;
	int numbytes;

    if(argc!=2){
        fprintf(stderr,"usage: port number\n");
	    exit(1);
    }

    init();

    command = (char*)malloc(MAX_DATASIZE*sizeof(char));

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		// this is the child process
        //close(sockfd); // child doesn't need the listener
        /*if (send(new_fd, "Hello, world!", 13, 0) == -1)
            perror("send");*/
        numbytes = recv(new_fd, command, MAX_DATASIZE, 0);
        command[numbytes] = '\0';
        status = processRequest(command);
        //printData();
        if(send(new_fd, res, MAX_DATASIZE, 0) == -1){
            fprintf(stderr, "Erro no send\n");
            return -1;
        }
        close(new_fd);
			//exit(0);
		//close(new_fd);  // parent doesn't need this
		//command[0] = '\0';
	}

	return 0;
}


int init(){
    D = (Dados*)malloc(sizeof(Dados));

    D->first = NULL;
    D->numElems = 0;

    //chave = 0;
}


int create(int id, char *valor)
{
    Dado *dado;
    Dado *aux;

    aux = D->first;

    //printf("valor passado: %s\n", valor);

    if(strlen(valor) > MAX)
    {
        //erro
        sprintf(res, "HTTP/1.1 400 Bad Request\n\nLimite ultrapassado\n");
        return -1;
    }

    while(aux!=NULL){
        //printf("aux: %s\n", aux->bytes);
        if(!strcmp(aux->bytes, valor)){
            sprintf(res, "HTTP/1.1 400 Bad Request\n\nValor ja inserido\n");
            return -1;
        }
        aux= aux->next;
    }

    if((dado = (Dado*)malloc(sizeof(Dado))) == NULL)
    {
        //erro
        printf("erro");
        return -1;
    }

    dado->bytes = (char*)malloc(strlen(valor)*sizeof(char));

    dado->chave = id;
    strcpy(dado->bytes, valor);
    dado->next = NULL;

    return inserir(dado);
}

int inserir(Dado *dado)
{
    Dado *aux;
    aux = D->first;

    if(D->numElems == 0)
    {
        D->first = dado;
    }
    // Insert on the last position
    else
    {
        while(aux->next != NULL)
            aux = aux->next;

        aux->next = dado;
    }

    D->numElems++;


    sprintf(res, "HTTP/1.1 200 OK\n\nValor inserido com sucesso\nValor: %s, Chave: %ld\n", dado->bytes, dado->chave);

    //chave++;

    return 1;
}

int processRequest(char *cmd)
{
    //printf("%s\n", cmd);
    unsigned long id;
    char** req;
    int w, i = 0, j=0;
    char idstr[30];

    // size of 2, because it is the method and (value or key)
    req = (char**)malloc(2 * sizeof(char*));

    // Olhar isso aqui depois <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    for(w=0;w<2;w++){
        req[w] = (char*)malloc(1000 * sizeof(char));
       // memset(req[w], '\0', sizeof(req[w]));
    }

    // Separates the entire command into an array of strings
    // First Position ->> Method "GET" or "POST" or "PUT" or "DELETE"
    // Second Postion ->> Key
    // Third Position (optional) ->> Value

    // Getting first token
    while(cmd[i]!=' '){
        i++;
    }

    while(j < i)
    {
        req[0][j] = cmd[j];
        j++;
    }

    req[0][j] = '\0';

    // Getting second token
    /*i++;
    j = i;
    w = 0;
    while(cmd[i]!=';'){
        i++;
    }

    while(j < i)
    {
        req[1][w] = cmd[j];
        w++;
        j++;
    }*/

    // Getting third token
    i=0;
    j++;
    j++;

    while(cmd[j]!=' ')
    {
        idstr[i] = cmd[j];
        i++;
        j++;
    }

    idstr[i] = '\0';
    id = atoi(idstr);
 printf("chave: %ld\n", id);

    while(cmd[j]!='\n')
    {
        j++;
    }

    i=0;
    while(j <= strlen(cmd))
    {
        req[1][i] = cmd[j];
        i++;
        j++;
    }

    req[1][i] = '\0';

    printf("Method: %s\n", req[0]);
    printf("Restante: %s\n", req[1]);
    //printf("Value: %s\n", req[2]);


    if(!strcmp(req[0], "POST"))
    {
        create(id, req[1]);
    }
    else if(!strcmp(req[0], "GET"))
    {
        //id = atoi(req[1]);
        get((int)id);
    }
    else if(!strcmp(req[0], "PUT"))
    {
        //id = atoi(req[1]);
        put((int)id, req[2]);
    }
    else if(!strcmp(req[0], "DELETE"))
    {
        //id = atoi(req[1]);
        del((int)id);
    }

    for(w=0;w<2;w++){
        free(req[w]);
    }

    free(req);
}

int get(int id){
    Dado *aux;

    aux = D->first;

    while(aux!=NULL)
    {
        if(aux->chave == id)
        {
            sprintf(res, "HTTP/1.1 200 OK\n\nChave: %ld, Valor: %s", aux->chave, aux->bytes);
            return 1;
        }
        aux = aux->next;
    }

    sprintf(res, "HTTP/1.1 404 NOT FOUND\n\nRegistro nao encontrado\n");
    return -1;
}

int put(int id, char *newValue)
{
    Dado *aux;

    aux = D->first;

    while(aux!=NULL)
    {
        if(aux->chave == id)
        {
            strcpy(aux->bytes, newValue);
            sprintf(res, "HTTP/1.1 200 OK\n\nValor Atualizado C: %ld V: %s\n", aux->chave, aux->bytes);
            return 1;
        }
        aux = aux->next;
    }

    sprintf(res, "HTTP/1.1 404 NOTFOUND\n\nRegistro nao encontrado\n");
    return -1;
}

int del(int id)
{
    Dado *aux;
    Dado *aux2;

    aux = D->first;
    aux2 = aux->next;

    if(D->first == NULL)
    {
        sprintf(res, "HTTP/1.1 400 Bad Request\n\nLista esta vazia\n");
        return -1;
    }


    // In case, it is the first element of my list
    if(aux->chave == id)
    {
        D->first = aux->next;
        free(aux);
        sprintf(res, "HTTP/1.1 200 OK\n\nRegistro Removido com Sucesso!\n");
        D->numElems--;
        return 1;
    }

    while(aux2!=NULL){
        if(aux2->chave == id)
        {
            aux->next = aux2->next;
            sprintf(res, "HTTP/1.1 200 OK\n\nRegistro Removido com Sucesso!\n");
            D->numElems--;
            free(aux2);
            return 1;
        }
        aux = aux->next;
        aux2 = aux->next;
    }

    sprintf(res, "HTTP/1.1 404 NOT FOUND\n\nRegistro nao encontrado!");
    return -1;
}


void printData(){
    Dado *aux;
    aux = D->first;

    while(aux!=NULL){
        printf("Chave: %ld\n", aux->chave);
        printf("Valor: %s\n", aux->bytes);
        printf("NumElems: %d\n\n", D->numElems);
        aux = aux->next;
    }
}





























