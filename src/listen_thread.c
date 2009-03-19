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

int ReadMessage(socketfd sock, void *buf, size_t bufsize) {
	int nBytes;
	nBytes = recv(sock, buf, bufsize, 0);
	return nBytes;
}

void HandleMessage(message_t *msg, socketfd from, fd_set *fdSet, connections_t *list) {
	int i, count;
	node_t *newNode;
	
	/*TODO: Check message id to verify that the message is somewhat expected */
	switch(msg->msgType) {
		case MW_TRANSACTION:
		case MW_SYNCHRONIZE:
		case MW_COMMIT:
		case MW_ACK:
		case MW_NAK:
			newNode = createNode(msg);
			globalMsg(MSG_LOCK, MSG_NO_ARG);
			globalMsg(MSG_PUSH, newNode);
			globalMsg(MSG_UNLOCK, MSG_NO_ARG);
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
	}
}

void *ListeningThread(void *arg) {
	fd_set masterFdSet, readFdSet; /* Used by select */
	fd_set clientSet, mwSet;
	fd_set clientReadSet, mwReadSet;
	int i, nBytes;
	void * recvBuf;
	size_t bufSize;
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
	FD_ZERO(&clientSet);
	FD_ZERO(&mwSet);
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
					recvBuf = malloc(sizeof(long));
					if((nBytes = recv(i, recvBuf, sizeof(long))) > 0) {
						// We are about to receive stuff from a middleware
						if((int)(&recvBuf) == -1) {
							FD_SET(i, &mwSet)
							FD_CLEAR(i, &masterFdSet);
						}
						// We are about to receive stuff from a client
						else {
							FD_SET(i, &clientSet);
							FD_CLEAR(i, &masterFdSet);
						}
					}
					else {
						/* TODO: Add error handling */
					}
				}
			}
		}
		readFdSet = masterFdSet;
		if(select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0) {
			perror("select: ");
			//TODO: Add error handling here
		}
		// Iterate over all incoming middleware messages
		for(i = 0; i < FD_SETSIZE; i++) {
		}
	}
	return (void *)0;
}
