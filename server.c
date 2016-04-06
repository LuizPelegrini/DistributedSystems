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

#define MAX 1000    // Size of buffer, where the HTTP body will be stored

#define MAX_DATASIZE 2000 // Size of my request buffer

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
int create(unsigned long id, char *valor);
int inserir(Dado *dado);
int get(unsigned long id);
int put(unsigned long id, char *newValue);
int del(unsigned long id);
int processRequest(char *cmd);
void printData();
char *setResponse(int status);
int getContentLength(char* command);
char *encodeString(char* valor);

unsigned long chave;
int contentLength;
char res[MAX_DATASIZE];
char ip[INET6_ADDRSTRLEN];
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
	int bytesReceived, bytesSent;

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

        void *addr;

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

        bytesReceived = recv(new_fd, command, MAX_DATASIZE, 0);

        contentLength = getContentLength(command);

        if(contentLength < 1000)
        {
            command[bytesReceived] = '\0';
            //printf("comando inteiro: %s\n\n", command);
            status = processRequest(command);
            //printData();
            //printf("strlen(res): %d\n", (int)strlen(res));
            //printf("RES: %s\n", res);

        }
        else{
            sprintf(res, "HTTP/1.1 400 BAD REQUEST\n\n");
        }

        if(bytesSent = send(new_fd, res, strlen(res), 0) == -1){
            fprintf(stderr, "Erro no send\n");
            return -1;
        }
        //printf("numBytes >> %d\n", numbytes);


        //printf("STRLEN RES: %d\n", (int)strlen(res));
        //strcpy(res, "");
        //printf("RES: %s\n", res);
        //printf("STRLEN RES: %d\n", (int)strlen(res));

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

    contentLength = 0;
}


int create(unsigned long id, char *valor)
{
    Dado *dado;
    Dado *aux;

    aux = D->first;


    while(aux!=NULL){
        //printf("aux: %s\n", aux->bytes);
        if(aux->chave == id){
            sprintf(res, "HTTP/1.1 409 CONFLICT\n\n");
            return -1;
        }
        aux= aux->next;
    }

    if((dado = (Dado*)malloc(sizeof(Dado))) == NULL)
    {
        printf("erro");
        return -1;
    }

    dado->bytes = (char*)malloc(strlen(valor)+1*sizeof(char));

    encodeString(valor);

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


    sprintf(res, "HTTP/1.1 200 OK\n\n");

    return 1;
}

int processRequest(char *cmd)
{
    unsigned long id;
    char** req;
    int w, i = 0, j=0;
    char idstr[30];
    char *data;

    // size of 2, because it is the method and header+body
    req = (char**)malloc(2 * sizeof(char*));
    data = (char*)malloc(MAX * sizeof(char*));

    // Separates the entire command into an array of strings
    // First Position ->> Method "GET" or "POST" or "PUT" or "DELETE"
    // Second Postion ->> rest of the header and the entire body
    req[0] = (char*)malloc(10*sizeof(char));
    req[1] = (char*)malloc((strlen(cmd) - strlen(req[0])) * sizeof(char));



    // Getting first token (the method)
    while(cmd[i]!=' '){
        i++;
    }

    while(j < i)
    {
        req[0][j] = cmd[j];
        j++;
    }

    req[0][j] = '\0';

    // Getting key
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
    id = atol(idstr);

    //printf("chave: %ld\n", id);

    // Getting the rest of the request (ignoring HTTP/1.1)
    while(cmd[j]!='\n')
    {
        j++;
    }

    //ignoring the '\n' at the end of the line
    j++;
    i=0;
    while(j <= strlen(cmd))
    {
        req[1][i] = cmd[j];
        i++;
        j++;
    }

    req[1][i] = '\0';

    //printf("Method: %s\n", req[0]);
    //printf("Restante: %s\n", req[1]);

    if(!strcmp(req[0], "POST"))
    {
        i=0;

        // The body comes after \r\n\r\n
        while(req[1][i]!='\n' || req[1][i+2]!='\n')
        {
            i++;
        }
        i+=3;
        j=0;

        // Now at the 'i' position, the body of the request starts =]
        // Copy the data now on
        while(req[1][i]!='\0')
        {
            data[j] = req[1][i];
            i++;
            j++;
        }

        data[j] = '\0';
        //printf("DATA: %s\n", data);

        create(id, data);
    }
    else if(!strcmp(req[0], "GET"))
    {
        get(id);
    }
    else if(!strcmp(req[0], "PUT"))
    {
        i=0;
        while(req[1][i]!='\n' || req[1][i+2]!='\n')
        {
            i++;
        }
        i+=3;
        j=0;

        while(req[1][i]!='\0')
        {
            data[j] = req[1][i];
            i++;
            j++;
        }

        data[j] = '\0';
        put(id, data);
    }
    else if(!strcmp(req[0], "DELETE"))
    {
        del(id);
    }


    // free stuff
    for(w=0;w<2;w++){
        free(req[w]);
    }
    free(data);
    free(req);
}

int get(unsigned long id){
    Dado *aux;
    char *response;
    aux = D->first;

    while(aux!=NULL)
    {
        if(aux->chave == id)
        {
            response = (char*)malloc(strlen(aux->bytes)+100*sizeof(char));

            sprintf(response, "chave=%ld&valor=%s", aux->chave, aux->bytes);
            sprintf(res, "HTTP/1.1 200 OK\nContent-Length: %d\n\n%s\n\n", (int)strlen(response)+1, response);
            free(response);
            return 1;
        }
        aux = aux->next;
    }

    sprintf(res, "HTTP/1.1 404 NOT FOUND\n\n");
    return -1;
}

int put(unsigned long id, char *newValue)
{
    Dado *aux;

    aux = D->first;

    while(aux!=NULL)
    {
        if(aux->chave == id)
        {
            aux->bytes = (char*)realloc(aux->bytes, strlen(newValue)+1);
            strcpy(aux->bytes, newValue);
            sprintf(res, "HTTP/1.1 200 OK\n\n");
            return 1;
        }
        aux = aux->next;
    }

    sprintf(res, "HTTP/1.1 404 NOT FOUND\n\n");
    return -1;
}

int del(unsigned long id)
{
    Dado *aux;
    Dado *aux2;

    aux = D->first;

    // Checks whether the list is empty
    if(aux == NULL)
    {
        sprintf(res, "HTTP/1.1 400 BAD REQUEST\n\n");
        return -1;
    }

    aux2 = aux->next;

    // In case, it is the first element of my list
    if(aux->chave == id)
    {
        D->first = aux->next;
        free(aux);
        sprintf(res, "HTTP/1.1 200 OK\n\n");
        D->numElems--;
        return 1;
    }

    while(aux2!=NULL){
        if(aux2->chave == id)
        {
            aux->next = aux2->next;
            sprintf(res, "HTTP/1.1 200 OK\n\n");
            D->numElems--;
            free(aux2);
            return 1;
        }
        aux = aux->next;
        aux2 = aux->next;
    }

    sprintf(res, "HTTP/1.1 404 NOT FOUND\n\n");
    return -1;
}

int getContentLength(char *command)
{
    char **tokens;
    char *commandAux;
    int i, contentLength;

    // 100 tokens
    tokens = (char**)malloc(100*sizeof(char*));
    commandAux = (char*)malloc(strlen(command)+1*sizeof(char));
    strcpy(commandAux, command);

    // each token has a size of 100 at maximum
    for(i=0;i<100;i++)
    {
        tokens[i] = (char*)malloc(100*sizeof(char));
    }

    // Separate the command into tokens by " " and '\n'
    i=0;
    tokens[i] = strtok(commandAux, " \n");

    // Keep doing it, if it is not a GET HTTP method
    // because the GET request method does not contain the Content-Length header
    if(strcmp(tokens[0], "GET") != 0)
    {
        while(tokens[i] != NULL)
        {
            //printf("token %d: %s\n", i, tokens[i]);
            i++;
            tokens[i] = strtok(NULL, " \n");
            if(!(strcmp(tokens[i], "Content-Length:")))
            {
                i++;
                tokens[i] = strtok(NULL, " \n");
                break;
            }
        }

        contentLength = atoi(tokens[i]);
    }

    // In GET requests, just return 0
    else
    {
        contentLength = -1;
    }


    /*
    for(i=0;i<100;i++)
    {
        printf("token %d: %s\n", i, tokens[i]);
        free(tokens[i]);
        printf("Here!\n");
    }
    free(tokens);
    free(commandAux);*/

    return contentLength;
}

char *encodeString(char* valor){}


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





























