/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once

char *concatenate(char *method, char *value);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char bufS[MAXDATASIZE];
	char bufR[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc <= 4 && argc >= 5) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure


    //if(argc == 4)
        strcpy(bufS, concatenate(argv[2], argv[3]));
    //else if(argc == 5)
      //  strcpy(bufS, concatenate(argv[2], argv[3], argv[4]));

    printf("cmd: %s\n", bufS);

	send(sockfd, bufS, strlen(bufS), 0);

    if ((numbytes = recv(sockfd, bufR, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	bufR[numbytes] = '\0';

	printf("client: received '%s'\n",bufR);

	close(sockfd);

	return 0;
}

char *concatenate(char* method, char *value){
    char* str;
    int size;

    size = strlen(method) + strlen(value);
    // ';' + ';' + ';' + \0'
    size += 3;

    str = (char*)malloc(size*sizeof(char));

    memset(str, '\0', sizeof(str));

    strcpy(str, method);

    str[strlen(method)] = ' ';

    strncat(str, value, strlen(value));

   // str[strlen(method) + strlen(key) + 1] = ';';

    //strncat(str, value, strlen(value));

    //printf("str: %s\n", str);

    return str;
}



