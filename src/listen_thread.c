#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include "listen_thread.h"

connections_t list;

/* Adds a connection to the first available position */
int AddConnection(connections_t *list, socketfd sock) {
	int i;
	
	for(i = 0; i < list->maxConnections; i++) {
		if(list->connection[i].connStatus == STATUS_DISCONNECTED) {
			printf("Adding socket from connection list\n");
			list->connection[i].connStatus = STATUS_CONNECTED;
			list->connection[i].socket = sock;
			list->nConnections++;
			
			if(list->maxConnections - list->nConnections <= CONN_RESIZE_THRESHOLD)
				ResizeConnectionList(list);
			
			break;
		}
	}
	return 0;
}

/* Sets a connection to DISCONNECTED status */
int RemoveConnection(connections_t *list, socketfd sock) {
	int i;
	
	for(i = 0; i < list->maxConnections; i++) {
		if(list->connection[i].socket == sock) {
			list->connection[i].connStatus = STATUS_DISCONNECTED;
			list->nConnections--;
			break;
		}
	}
	return 0;
}

/* Initialize connection list to default values */
int InitConnectionList(connections_t *list, int forceInit) {
	static int init = 0;
	int i;
	
	if(!init || forceInit) {
		init = 1;
		list->nConnections = 0;
		list->maxConnections = CONN_DEFAULT_LIMIT;
		list->connection = (connection_t *)malloc(sizeof(connection_t) * CONN_DEFAULT_LIMIT);
		for(i = 0; i < CONN_DEFAULT_LIMIT; i++)
			list->connection[i].socket = -1;
	}
	return 0;
}

/* Reallocate memory for a connection list to hold CONN_GROW_FACTOR more connections */
int ResizeConnectionList(connections_t *list) {
	list->maxConnections += CONN_GROW_FACTOR;
	list->connection = realloc(list->connection, sizeof(connection_t) * list->maxConnections);
	return 0;
}

int CreateSocket(unsigned short int port) {
	struct sockaddr_in sockName;
	socketfd sock;
	
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

int ReadMessage(socketfd sock, message_t *buf) {
	int nBytes;
	nBytes = recv(sock, (void *)buf, sizeof(message_t), 0);
	return nBytes;
}

void HandleMessage(message_t *msg, socketfd from, fd_set *fdSet) {
	switch(msg->msgType) {
		case TRANSACTION:
			/* TODO: Incoming transaction should be forwarded to worker thread through queue */
			break;
		case SYNCHRONIZE:
			/* TODO: if incoming request, flag worker thread to send transaction list, otherwise sync with incoming list */
			break;
		case CONNECT:
			/* TODO: Obsolete */
			break;
		case COMMIT:
			/* TODO: Handle a commit request */
			break;
		case DISCONNECT:
			/* TODO: Add mutex to lock the connection list so no bad things might happen */
			printf("Received a DISCONNECT request\n");
			printf("%d/%d connections\n", list.nConnections, list.maxConnections);
			RemoveConnection(&list, from);
			if(fdSet != NULL)
				FD_CLR(from, fdSet);
			close(from);
			break;
		case ACK:
			printf("Received ACK message\n");
			printf("%d/%d connections\n", list.nConnections, list.maxConnections);
			/* TODO: Wait for all connections to ACK, then flag worker thread to commit */
			break;
		case NAK:
			/* TODO: DO STUFF!!! Depends on situation, still on a TODO TODO list */
			break;
	}
}

void *ListeningThread(void *arg) {
	fd_set masterFdSet, readFdSet; /* Used by select */
	int i;
	socketfd connectionSocket;
	socketfd listenSocket = CreateSocket(PORT);
	message_t msg;
	
	InitConnectionList(&list, 0);
	
	if(listen(listenSocket, 1) < 0) {
		perror("listen: ");
		//TODO: Add error handling here
	}
	FD_ZERO(&masterFdSet);
	FD_SET(listenSocket, &masterFdSet);
	
	// The thread should wait for new messages when not processing something
	while(1) {
		readFdSet = masterFdSet;
		if(select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0) {
			perror("select: ");
			//TODO: Add error handling here
		}
		for(i = 0; i < FD_SETSIZE; i++) {
			if(FD_ISSET(i, &readFdSet)) {
				// New connection incoming
				if(i == listenSocket) {
					connectionSocket = accept(listenSocket, NULL, NULL);
					if(connectionSocket < 0) {
						perror("accept: ");
						//TODO: Add error handling here
					}
					FD_SET(connectionSocket, &masterFdSet);
					AddConnection(&list, connectionSocket);
					fprintf(stdout, "New connection established to unknown thingy\n");
				}
				else {
					if(ReadMessage(i, &msg)) {
						HandleMessage(&msg, i, &masterFdSet);
						printf("Read a message successfully\n");
					}
					else {
						printf("Read a message UNsuccessfully\n");
					}
					//TODO: Some request from connected middleware... DANGER!!!!
				}
			}
		}
	}
	return (void *)0;
}

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

void *ComThread(void *arg) {
	message_t msg;
	socketfd sock;
	struct sockaddr_in hostInfo;
	
	sock = socket(PF_INET, SOCK_STREAM, 0);
	initSocketAddress(&hostInfo, "localhost", 12345);
	while(1) {
		if(connect(sock, (struct sockaddr *)&hostInfo, sizeof(hostInfo)) < 0) {
			perror("Thread could not connect to server\n");
			sleep(2);
		}
		else {
			printf("Thread connected to server\n");
			break;
		}
	}

	while(1) {
		msg.msgType = ACK;
		send(sock, (void *)&msg, sizeof(msg), 0);
		sleep(2);
	}
}

int main(int argc, char *argv[]) {
	pthread_t listenThread, comThread[18];
	message_t msg;
	socketfd sock;
	struct sockaddr_in hostInfo;
	int i;
	
	pthread_create(&listenThread, NULL, ListeningThread, (void *)NULL);
	sock = socket(PF_INET, SOCK_STREAM, 0);
	initSocketAddress(&hostInfo, "localhost", 12345);
	sleep(5);
	
	for(i = 0; i < 18; i++) {
		pthread_create(&(comThread[i]), NULL, ComThread, (void *)NULL);
		sleep(2);
	}
	
	if(connect(sock, (struct sockaddr *)&hostInfo, sizeof(hostInfo)) < 0) {
		perror("Could not connect to server\n");
		exit(EXIT_FAILURE);
	}
	
	msg.msgType = ACK;
	send(sock, (void *)&msg, sizeof(msg), 0);
	msg.msgType = DISCONNECT;
	send(sock, (void *)&msg, sizeof(msg), 0);
	msg.msgType = ACK;
	send(sock, (void *)&msg, sizeof(msg), 0);
	close(sock);
	
	sleep(2);
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(connect(sock, (struct sockaddr *)&hostInfo, sizeof(hostInfo)) < 0) {
		perror("Could not connect to server\n");
		exit(EXIT_FAILURE);
	}
	sleep(3);
	msg.msgType = DISCONNECT;
	send(sock, (void *)&msg, sizeof(msg), 0);
	pthread_join(listenThread, NULL);
	return 0;
}
