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
   force_read() is wrapper around the read() call that ensures that count bytes
   are always read before returning control, unless EOF or an error is
   detected.
   Arguments and return values are as for the read() call.

   We *should* use recv() call with MSG_WAITALL flag, but it is not always supported.

 */
ssize_t force_read(int fd, void *buf, size_t count) {
	size_t so_far = 0;

	while (so_far < count) {
		size_t res;
		res = read(fd, buf + so_far, count - so_far);
		if (res <= 0) return res;
		so_far += res;
	}
	return so_far;
}

int ReadMessage(socketfd sock, void *buf, size_t bufsize) {
	int nBytes;
	nBytes = recv(sock, buf, bufsize, 0);
	return nBytes;
}

int HandleMessage(message_t *msg, socketfd from, fd_set *fdSet, connections_t *list) {
	int i, count;
	int ret = 0;
	node_t *newNode;

	
	/*TODO: Check message id to verify that the message is somewhat expected */
	if(msg->endOfMsg) ret = MSG_EOF;

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
			ret = MW_DISCONNECT;
			RemoveConnection(list, from);
			if(fdSet != NULL)
				FD_CLR(from, fdSet);
			close(from);
			break;
	}

	return ret;
}

void *ListeningThread(void *arg) {
	fd_set masterFdSet, readFdSet; /* Used by select */
	fd_set clientSet, mwSet;
	int i, j, nBytes;
	void * recvBuf;
	size_t bufSize;
	socketfd connectionSocket;
	socketfd listenSocket = CreateSocket(PORT);
	message_t msg;
	connections_t connections;
	struct timeval selectTimeout;
	
	InitConnectionList(&connections);
	
	if(listen(listenSocket, 1) < 0) {
		perror("listen: ");
		//TODO: Add error handling here
	}
	FD_ZERO(&masterFdSet);
	FD_ZERO(&clientSet);
	FD_ZERO(&mwSet);
	FD_SET(listenSocket, &masterFdSet);
	
	selectTimeout.tv_sec = 0;
	selectTimeout.tv_usec = 0;

	// The thread should wait for new messages when not processing something
	while(1) {
		readFdSet = masterFdSet;
		if(select(FD_SETSIZE, &readFdSet, NULL, NULL, &selectTimeout) < 0) {
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
					AddConnection(&connections, connectionSocket);
				}
				// Existing connection wishes to send something
				else {
					recvBuf = malloc(sizeof(long));
					if((nBytes = force_read(i, recvBuf, sizeof(long))) > 0) {
						// We are about to receive stuff from a middleware
						if((int)(&recvBuf) == -1) {
							debug_out(5, "Received pre send request from MW\n");
							FD_SET(i, &mwSet)
							FD_CLR(i, &masterFdSet);
						}
						// We are about to receive stuff from a client
						else {
							debug_out(5, "Received pre send request from client\n");
							for(j = 0; j < connections.maxConnections; j++) {
								if(connections.connection[j].socket == i) {
									connections.connection[j].numCmds = (int)(&recvBuf);
								}
							}
							FD_SET(i, &clientSet);
							FD_CLR(i, &masterFdSet);
						}
					}
					else {
						/* TODO: Add error handling */
					}
				}
			}
		}

		readFdSet = mwSet;
		if(select(FD_SETSIZE, &readFdSet, NULL, NULL, &selectTimeout) < 0) {
			perror("select: ");
			//TODO: Add error handling here
		}
		// Iterate over all incoming middleware messages
		for(i = 0; i < FD_SETSIZE; i++) {
			if(FDISSET(i, &readFdSet)) {
				recvBuf = malloc(sizeof(message_t));
				if((nBytes = force_read(i, recvBuf, sizeof(message_t))) > 0) {
					if(HandleMessage((message_t *)msg, i, &mwSet, &connections) == MSG_EOF) {
						//TODO: Add socket to master_set and clear from mw_set
						FD_SET(i, &masterFdSet);
						FD_CLR(i, &mwSet);
					}
				}
			}
		}

		readFdSet = clientSet;
		if(select(FD_SETSIZE, &readFdSet, NULL, NULL, &selectTimeout) < 0) {
			perror("select: ");
			//TODO: Add error handling here
		}
		for(i = 0; i < FD_SETSIZE; i++) {
			if(FDISSET(i, &readFdSet)) {
				recvBuf = malloc(sizeof(command));
				if((nBytes = force_read(i, recvBuf, sizeof(command))) > 0) {
					//TODO: Add client stuff here
				}
			}
		}
	}
	return (void *)0;
}
