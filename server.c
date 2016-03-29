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
int create();
int inserir(Dado *dado);
int processRequest(char *cmd);

Dados *D;
int chave = 0;

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
	char s[INET6_ADDRSTRLEN];
	char *command;
	int rv;

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
        /*
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);
        */
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			/*if (send(new_fd, "Hello, world!", 13, 0) == -1)
				perror("send");*/
            recv(new_fd, command, MAX_DATASIZE-1, 0);
            processRequest(command);
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}


int init(){
    D = (Dados*)malloc(sizeof(Dados));

    D->first = NULL;
    D->numElems = 0;
}


int create(char *valor)
{
    Dado *dado;

    if(strlen(valor) > MAX)
    {
        //erro
        return -1;
    }

    if((dado = (Dado*)malloc(sizeof(Dado))) == NULL)
    {
        //erro
        return -1;
    }



    dado->chave = chave++;
    dado->bytes = valor;
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

    return 1;
}

int processRequest(char *cmd)
{
    char** req;
    int w, i = 0, j=0;

    // size of 2, because it is the method and (value or key)
    req = (char**)malloc(2 * sizeof(char*));

    // Olhar isso aqui depois <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    for(w=0;w<2;w++){
        req[w] = (char*)malloc(100 * sizeof(char));
    }

    // Separates the entire command into an array of strings
    // First Position ->> Method "GET" or "POST" or "PUT" or "DELETE"
    // Second Postion ->> Value or Key
    while(cmd[i]!=';'){
        i++;
    }


    while(j < i)
    {
        req[0][j] = cmd[j];
        j++;
    }

    req[0][j] = '\0';
    i=0;
    j++;
    while(j <= strlen(cmd))
    {
        req[1][i] = cmd[j];
        i++;
        j++;
    }
    req[0][i] = '\0';
    printf("Method: %s\n", req[0]);
    printf("Data: %s\n", req[1]);

    if(!strcmp(req[0], "POST"))
    {
        create(req[1]);
    }
    else if(!strcmp(req[0], "GET"))
    {
    }
    else if(!strcmp(req[0], "PUT"))
    {
    }
    else if(!strcmp(req[0], "DELETE"))
    {
    }
}





























