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

int HandleMessage(message_t *msg, socketfd from, fd_set *fdSet) {
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
				// Check if we need to synchronize:
				// if (our ID = their ID - 1): Someone else is doing a transaction, increase our ID to compensate
				// if (our ID < their ID - 1): We have missed some transaction, lock down and send a sync message to them
				if(msg->owner == msg->msgId - 1) {
					if(globalId(ID_CHECK, msg->msgId)) {
						globalId(ID_CHANGE, msg->owner);
						newNode = createNode(msg);
						globalMsg(MSG_LOCK, MSG_NO_ARG);
						globalMsg(MSG_PUSH, newNode);
						globalMsg(MSG_UNLOCK, MSG_NO_ARG);
					}
				}
				else if (msg->owner < msg->msgId - 1) {
					if(globalId(ID_CHECK, msg->msgId)) {
						tmp.msgType = MW_SYNCHRONIZE;
						tmp.endOfMsg = TRUE;
						// TODO: THIS REQUIRES LOGGING FEATURES!!!
					}
					// THIS IS THE DANGEROUS THING WHERE THINGS REALLY WENT TOTALLY WRONG AND WE HAVE TO SYNC THINGS!!!
				}
				else {
					newNode = createNode(msg);
					globalMsg(MSG_LOCK, MSG_NO_ARG);
					globalMsg(MSG_PUSH, newNode);
					globalMsg(MSG_UNLOCK, MSG_NO_ARG);
				}
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
				ConnectionHandler(LIST_REMOVE, from, NULL, NULL, NULL);
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
	int i, nBytes;
	void * recvBuf;
	connections_t connections;
	connection_t tmp_conn;
	socketfd connectionSocket;
	struct sockaddr connectInfo;
	socklen_t connectInfoLength = sizeof(connectInfo);
	//socketfd listenSocket = CreateSocket(PORT);
	socketfd listenSocket = (int *)arg;
	struct timeval selectTimeout;

	debug_out(5, "Listenthread has started\n");

	if(listen(listenSocket, 1) < 0) {
		perror("listen: ");
		//TODO: Add error handling here
	}
	FD_ZERO(&masterFdSet);
	FD_ZERO(&clientSet);
	FD_ZERO(&mwSet);
	FD_ZERO(&readFdSet);
	FD_SET(listenSocket, &masterFdSet);
	
	fcntl(listenSocket, F_SETFL, O_NONBLOCK);

	// The thread should wait for new messages when not processing something
	while(1) {
		//readFdSet = masterFdSet;
		selectTimeout.tv_sec = 0;
		selectTimeout.tv_usec = 200;
		FD_ZERO(&readFdSet);
		FD_SET(listenSocket, &readFdSet);

		debug_out(5, "Listenthread: copying connection list\n");
		if(ConnectionHandler(LIST_COPY, 0, NULL, &connections, NULL) != 0) 
			debug_out(5, "Could not create copy of connection list\n");
		for(i = 0; i < connections.maxConnections; i++) {
			if(connections.connection[i].connStatus == STATUS_CONNECTED) {
				FD_SET(connections.connection[i].socket, &readFdSet);
			}
		}
		debug_out(5, "Listenthread: going into select\n");

		if(select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0) {
			perror("select: ");
			//TODO: Add error handling here
		}
		for(i = 0; i < FD_SETSIZE; i++) {
			if(FD_ISSET(i, &readFdSet)) {  
				// New connection incoming
				if(i == listenSocket) {
					connectionSocket = accept(listenSocket, &connectInfo, &connectInfoLength);

					if(connectionSocket < 0) {
						perror("accept: ");
						//TODO: Add error handling here
					}
					else {
						FD_SET(connectionSocket, &masterFdSet);
						printf("Adding new connection (%d) to list... ", connectionSocket);
						ConnectionHandler(LIST_ADD, connectionSocket, NULL, NULL, &connectInfo);
						printf("connection added\n"); 
					}
				}
				// Existing connection wishes to send something
				else {
					recvBuf = malloc(sizeof(long));
					if((nBytes = force_read(i, recvBuf, sizeof(long))) > 0) {
						// We are about to receive stuff from a middleware
						if(*((long *)recvBuf) == -1) {
							debug_out(5, "Received send request from MW\n");
							FD_SET(i, &mwSet);
							FD_CLR(i, &masterFdSet);
						}
						// We are about to receive stuff from a client
						else {
							ConnectionHandler(LIST_SET_CLIENT, i, NULL, NULL, NULL);
							if(ConnectionHandler(LIST_GET_ENTRY, i, &tmp_conn, NULL, NULL) == 0) {
								tmp_conn.numCmds = *((long *)recvBuf);
								debug_out(3, "Received '%d' from '%d'\n", *((long *)recvBuf), i);
							}
							else {
								debug_out(5, "Received message from unknown client\n");
							}
							//ConnectionHandler(LIST_REPLACE_ENTRY, 0, &tmp_conn, NULL);
							//FD_SET(i, &clientSet);
							//FD_CLR(i, &masterFdSet);
						}
					}
					else {
						/* TODO: Add error handling */
					}
				}
			}
		}

/*		readFdSet = mwSet;
		if(select(FD_SETSIZE, &readFdSet, NULL, NULL, &selectTimeout) < 0) {
			perror("select: ");
			//TODO: Add error handling here
		}
		// Iterate over all incoming middleware messages
		for(i = 0; i < FD_SETSIZE; i++)	{
			if(FD_ISSET(i, &readFdSet)) {
				recvBuf = malloc(sizeof(message_t));
				if((nBytes = force_read(i, recvBuf, sizeof(message_t))) > 0) {
					switch(HandleMessage((message_t *)recvBuf, i, &mwSet)) {
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
					printf("CMD: %s ", ((command *)recvBuf)->arg1);
				}
			}
		}*/
	}
	return (void *)0;
}

