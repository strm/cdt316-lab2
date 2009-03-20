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

int HandleMessage(message_t *msg, socketfd from, fd_set *fdSet, connections_t *list) {
	int i, count;
	int ret = 0;
	node *newNode;
	message_t tmp;

	if(globalId(ID_CHECK, msg->msgId)) {
		if(msg->endOfMsg) ret = MW_EOF;
	
		switch(msg->msgType) {
			case MW_TRANSACTION:
			case MW_COMMIT:
			case MW_ACK:
			case MW_NAK:
				//TODO: Do we need to synchronize?
				if((msg->owner > 0) && (msg->owner < msg->msgId)) {

				}
				newNode = createNode(msg);
				globalMsg(MSG_LOCK, MSG_NO_ARG);
				globalMsg(MSG_PUSH, newNode);
				globalMsg(MSG_UNLOCK, MSG_NO_ARG);
				break;
			case MW_SYNCHRONIZE:
				if(globalId(ID_CHECK, msg->msgId)) { // everything seems to be in order

				}
				else { // Sender seems to have missed something earlier
					//TODO: We need to send all transactions that the sender has missed
				}
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
	}
	else { // The message id was lower then we expected
		tmp.msgId = globalId(ID_GET, 0);
		tmp.msgType = MW_NAK;
		tmp.endOfMsg = TRUE;
		tmp.owner = msg->msgId;

		if(send(from, (void *)&tmp, sizeof(message_t), 0) < 0) {
			perror("send: ");
		}
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
							debug_out(5, "Received send request from MW\n");
							FD_SET(i, &mwSet);
							FD_CLR(i, &masterFdSet);
						}
						// We are about to receive stuff from a client
						else {
							debug_out(5, "Received send request from client\n");
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
		for(i = 0; i < FD_SETSIZE; i++)	{
			if(FD_ISSET(i, &readFdSet)) {
				recvBuf = malloc(sizeof(message_t));
				if((nBytes = force_read(i, recvBuf, sizeof(message_t))) > 0) {
					switch(HandleMessage((message_t *)recvBuf, i, &mwSet, &connections)) {
						case MW_EOF:
							FD_SET(i, &masterFdSet);
							FD_CLR(i, &mwSet);
							break;
					}
				}
			}
		}

		// Iterate over all incoming client messages
		readFdSet = clientSet;
		if(select(FD_SETSIZE, &readFdSet, NULL, NULL, &selectTimeout) < 0) {
			perror("select: ");
			//TODO: Add error handling here
		}
		for(i = 0; i < FD_SETSIZE; i++) {
			if(FD_ISSET(i, &readFdSet)) {
				recvBuf = malloc(sizeof(command));
				if((nBytes = force_read(i, recvBuf, sizeof(command))) > 0) {
					//TODO: Add client stuff here
				}
			}
		}
	}
	return (void *)0;
}
