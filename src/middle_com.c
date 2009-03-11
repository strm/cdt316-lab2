#include "middle_com.h"

void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port) {
	struct hostent *hostInfo; /* Contains info about the host */
	/* Socket address format set to AF_INET for internet use. */
	name->sin_family = AF_INET;     
	/* Set port number. The function htons converts from host byte order to network byte order.*/
	name->sin_port = htons(port);
	/* Get info about host. */
	hostInfo = gethostbyname(hostName); 
	if(hostInfo == NULL) {
		fprintf(stderr, "initSocketAddress - Unknown host %s\n",hostName);
		exit(EXIT_FAILURE);
	}
	/* Fill in the host name into the sockaddr_in struct. */
	name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
}

int CreateSocket(unsigned short int port) {
	struct sockaddr_in sockName;
	int sock;
	
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		//TODO: Add error handling here
	}
	sockName.sin_family = AF_INET;
	sockName.sin_port = htons(port);
	sockName.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock, (struct sockaddr*)&sockName, sizeof(sockName)) < 0) {
		//TODO: Add error handling here
	}
	return sock;
}
