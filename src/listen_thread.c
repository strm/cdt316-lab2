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
#include <time.h>

/*
** Name:	AddConnection
** Parameters:	list - connectionlist to add the connection to
**		sock - the socket that is used for communication
** Description:	Adds a new connection to a connection list. It sets the attributes
**		of the new connection to default values for connected sockets
*/
int AddConnection(connections_t *list, socketfd sock) {
	int i;
	
	for(i = 0; i < list->maxConnections; i++) {
		if(list->connection[i].connStatus == STATUS_DISCONNECTED) {
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

/*
** Name:	RemoveConnection
** Parameters:	list - connectionlist to remove the connection from
**		sock - the socket that is used for communication
** Description:	Removes an existing connection from a connection list. The attributes
**		for the entry is set to STATUS_DISCONNECTED
*/
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

/*
** Name:	InitConnectionList
** Parameters:	list		- connectionlist to initialize
** Description:	Initializes an empty list to the default values for a connection list
*/
int InitConnectionList(connections_t *list) {
	int i;
	
	list->nConnections = 0;
	list->maxConnections = CONN_DEFAULT_LIMIT;
	list->connection = (connection_t *)malloc(sizeof(connection_t) * CONN_DEFAULT_LIMIT);
	for(i = 0; i < CONN_DEFAULT_LIMIT; i++) {
		list->connection[i].socket = -1;
		list->connection[i].connStatus = STATUS_DISCONNECTED;
	}
	
	return 0;
}

/*
** Name:	ResizeConnectionList
** Parameters:	list		- connectionlist to resize
** Description:	Increases the size of a connectionlist with CONN_GROW_FACTOR elements.
**		The attributes of the new elements are set to default values.
*/
int ResizeConnectionList(connections_t *list) {
	int oldMax = list->maxConnections;
	list->maxConnections += CONN_GROW_FACTOR;
	list->connection = realloc(list->connection, sizeof(connection_t) * list->maxConnections);
	while(oldMax < list->maxConnections) {
		list->connection[oldMax].connStatus = STATUS_DISCONNECTED;
		list->connection[oldMax].socket =  -1;
		oldMax++;
	}
	return 0;
}

/*int CreateSocket(unsigned short int port) {
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
}*/

int ReadMessage(socketfd sock, message_t *buf) {
	int nBytes;
	nBytes = recv(sock, (void *)buf, sizeof(message_t), 0);
	return nBytes;
}

void HandleMessage(message_t *msg, socketfd from, fd_set *fdSet, connections_t *list) {
	int i, count;
	
	switch(msg->msgType) {
		case MW_TRANSACTION:
			/* TODO: Incoming transaction should be forwarded to worker thread through queue */
			break;
		case MW_SYNCHRONIZE:
			/* TODO: send to worker thread for processing */
			break;
		case MW_COMMIT:
			/* TODO: send to worker thread for processing */
			break;
		case MW_CONNECT:
			/* TODO: this is a reply from middleware we connected to, contains information about sequence number etc. */
			break;
		case MW_DISCONNECT:
			/* TODO: Add mutex to lock the connection list so no bad things might happen */
			RemoveConnection(list, from);
			if(fdSet != NULL)
				FD_CLR(from, fdSet);
			close(from);
			break;
		case MW_ACK:
			/* TODO: Wait for all connections to ACK, then flag worker thread to commit */
			// Keep track of which middlewares that have sent an ACK, and count number of ACK's received
			for(i = 0, count = 0; i < list->maxConnections; i++) {
				if(list->connection[i].socket == from) {
					list->connection[i].transStatus = STATUS_ACKED;
				}
				if(list->connection[i].transStatus == STATUS_ACKED) {
					count++;
				}
			}
			
			// If all other middlewares has sent an ACK it is time to commit changes
			if(count == list->nConnections) {
				/* TODO: Flag worker thread to commit changes and send commit to all connected middlewares */
			}
			break;
		case MW_NAK:
			/* TODO: Forward to worker thread for rollback */
			break;
	}
}

/*
Things needed by listening thread:
connection_list from worker thread so they can process communication properly (inited by either one)
mutex for the connections list to prevent problems
queue from worker thread so messages can be passed between the threads (inited by worker thread)
mutex for the queue to prevent problems
*/
void *ListeningThread(void *arg) {
	fd_set masterFdSet, readFdSet; /* Used by select */
	int i;
	socketfd connectionSocket;
	socketfd listenSocket = CreateSocket(PORT);
	message_t msg;
	connections_t middlewareConnections;
	connections_t clientConnections;
	
	InitConnectionList(&middlewareConnections);
	InitConnectionList(&clientConnections);
	
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
					AddConnection(&middlewareConnections, connectionSocket);
				}
				else {
					if(ReadMessage(i, &msg)) {
						HandleMessage(&msg, i, &masterFdSet, &middlewareConnections);
					}
					else {
						/* TODO: Add error handling */
					}
				}
			}
		}
	}
	return (void *)0;
}

/*void *ComThread(void *arg) {
	int i;
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

	for(i = rand()%50; i >= 0; i--) {
		msg.msgType = MW_ACK;
		send(sock, (void *)&msg, sizeof(msg), 0);
		sleep(2);
	}
	msg.msgType = MW_DISCONNECT;
	send(sock, (void *)&msg, sizeof(msg), 0);
	return (void*)0;
}*/

/*int main(int argc, char *argv[]) {
	pthread_t listenThread, comThread[200];
	message_t msg;
	socketfd sock;
	struct sockaddr_in hostInfo;
	int i;
	
	srand(time(NULL));
	pthread_create(&listenThread, NULL, ListeningThread, (void *)NULL);
	sock = socket(PF_INET, SOCK_STREAM, 0);
	initSocketAddress(&hostInfo, "localhost", 12345);
	sleep(5);
	
	if(connect(sock, (struct sockaddr *)&hostInfo, sizeof(hostInfo)) < 0) {
		perror("Could not connect to server\n");
		exit(EXIT_FAILURE);
	}
	
	for(i = 0; i < 200; i++) {
		pthread_create(&(comThread[i]), NULL, ComThread, (void *)NULL);
		if(i == 8) {
			sleep(2);
			msg.msgType = DISCONNECT;
			send(sock, (void *)&msg, sizeof(msg), 0);
		}
		sleep(2);
	}
	
	sock = socket(PF_INET, SOCK_STREAM, 0);
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
}*/
