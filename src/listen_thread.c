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
