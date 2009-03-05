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

typedef struct {
	char variable[VAR_LEN];
	int value;
} data_t;

typedef struct {
	int msgType;
	int endOfMsg;
	int msgId;
	data_t data[MSG_MAX_DATA];
} message_t;

typedef struct {
	int socket;
	int connStatus;
	int transStatus
} connection_t;

typedef struct {
	int nConnections;
	int maxConnections;
	connection_t *connection;
} connections_t;

/* Adds a connection to the first available position */
int AddConnection(connections_t *list, int sock) {
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
}

/* Sets a connection to DISCONNECTED status */
int RemoveConnection(connections_t *list, int sock) {
	int i;
	
	for(i = 0; i < list->nConnections; i++) {
		if(list->connection[i].socket == sock) {
			list->connection[i].connStatus = STATUS_DISCONNECTED;
			break;
		}
	}
}

/* Initialize connection list to default values */
int InitConnectionList(connections_t *list) {
	int i;
	list->nConnections = 0;
	list->maxConnections = CONN_DEFAULT_LIMIT;
	list->connection = (connection_t *)malloc(sizeof(connection_t) * CONN_DEFAULT_LIMIT);
	for(i = 0; i < CONN_DEFAULT_LIMIT; i++)
		list->connection[i].socket = -1;
	return 0;
}

/* Reallocate memory for a connection list to hold CONN_GROW_FACTOR more connections */
int ResizeConnectionList(connections_t *list) {
	list->maxConnections += CONN_GROW_FACTOR;
	realloc(list->connection, sizeof(connection_t) * list->maxConnections);
	return 0;
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

int ReadMessage(int sock, data_t *buf) {
	int nBytes;
	nBytes = recv(sock, (void *)buf, sizeof(data_t), 0);
	return nBytes;
}

void HandleMessage(data_t *msg, int from) {
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
		case DISCONNECT:
			/* TODO: Remove connection from list */
			RemoveConnection(&list, from);
			break;
		case ACK:
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
	int connectionSocket;
	int listenSocket = CreateSocket(PORT);
	data_t msg;
	
	if(listen(listenSocket, 1) < 0) {
		perror("Bad stuff: ");
		//TODO: Add error handling here
	}
	FD_ZERO(&masterFdSet);
	FD_SET(listenSocket, &masterFdSet);
	
	// The thread should wait for new messages when not processing something
	while(1) {
		readFdSet = masterFdSet;
		if(select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0) {
			perror("Bad stuff: ");
			//TODO: Add error handling here
		}
		for(i = 0; i < FD_SETSIZE; i++) {
			if(FD_ISSET(i, &readFdSet)) {
				// New connection incoming
				if(i == listenSocket) {
					connectionSocket = accept(listenSocket, NULL, NULL);
					if(connectionSocket < 0) {
						perror("Bad stuff: ");
						//TODO: Add error handling here
					}
					FD_SET(connectionSocket, &masterFdSet);
					fprintf(stdout, "New connection established to unknown thingy\n");
				}
				else {
					if(ReadMessage(i, &msg)) {
						
					}
					//TODO: Some request from connected middleware... DANGER!!!!
				}
			}
		}
	}
	return (void *)0;
}

int main(int argc, char *argv[]) {
	pthread_t listenThread;
	
	pthread_create(&listenThread, NULL, ListeningThread, (void *)NULL);
	pthread_join(listenThread, NULL);
	return 0;
}
