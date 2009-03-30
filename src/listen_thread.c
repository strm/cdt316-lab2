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

int HandleMessage(message_t *msg, int from, fd_set *fdSet) {
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
				ConnectionHandler(
						REMOVE_BY_SOCKET,
						NULL,
						NULL,
						NULL,
						from);
				//ConnectionHandler(LIST_REMOVE, from, NULL, NULL, NULL);
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
	int i, nBytes, count, ret;
	void * recvBuf;
	connection *connections;
	connection conn, *it;
	int connectionSocket;
	struct sockaddr connectInfo;
	socklen_t connectInfoLength = sizeof(connectInfo);
	int listenSocket = (int)((int *)arg);
	struct timeval selectTimeout;

	debug_out(5, "Listenthread has started\n");
	fcntl(listenSocket, F_SETFL, O_NONBLOCK);

	if(listen(listenSocket, 1) < 0) {
		perror("listen: ");
		//TODO: Add error handling here
	}
	FD_ZERO(&masterFdSet);
	FD_ZERO(&clientSet);
	FD_ZERO(&mwSet);
	

	// The thread should wait for new messages when not processing something
	while(1) {
		count = 0;
		selectTimeout.tv_sec = 0;
		selectTimeout.tv_usec = 50000;
		FD_ZERO(&readFdSet);
		FD_SET(listenSocket, &readFdSet);

		if(ConnectionHandler(
					COPY_LIST,
					NULL,
					&connections,
					NULL,
					0) != 0) 
			debug_out(5, "Could not create copy of connection list\n");
		for(it = connections; it != NULL; it = it->next) {
			FD_SET(it->socket, &readFdSet);
			debug_out(5, "Listen: %d %s\n", it->socket, it->address);
			count++;
		}
		DeleteConnectionList(&connections);

		ret = select(FD_SETSIZE, &readFdSet, NULL, NULL, &selectTimeout);
		if(ret < 0) {
			perror("select: ");
			//TODO: Add error handling here
		}
		else if(ret == 0) {
			sleep(1);
		}
		else {
			for(i = 0; i < FD_SETSIZE; i++) {
				if(FD_ISSET(i, &readFdSet)) {
					debug_out(5, "Listen: fd_set(%d) is set\n", i);	
					// New connection incoming
					if(i == listenSocket) {
						debug_out(5, "Listen: new connection incoming\n");
						connectionSocket = accept(listenSocket, &connectInfo, &connectInfoLength);

						if(connectionSocket < 0) {
							perror("accept: ");
							//TODO: Add error handling here
						}
						else {
							FD_SET(connectionSocket, &masterFdSet);
							printf("Listen: Adding new connection (%d) to list... ", connectionSocket);
							CreateConnectionInfo(
									&conn,
									connectionSocket,
									"Unknown",
									TYPE_MIDDLEWARE,
									0);
							ConnectionHandler(
									ADD_TO_LIST,
									&conn,
									NULL,
									NULL,
									0);
							//ConnectionHandler(LIST_ADD_TO_LIST, connectionSocket, NULL, NULL, &connectInfo);
							printf("connection added\n"); 
						}
					}
					// Existing connection wishes to send something
					else {
						debug_out(5, "Listen: New data from existing connections\n");
						recvBuf = malloc(sizeof(long));
						if((nBytes = force_read(i, recvBuf, sizeof(long))) > 0) {
							debug_out(5, "Listen: force_read returned successfully\n");
							// We are about to receive stuff from a middleware
							if(*((long *)recvBuf) == -1) {
								debug_out(5, "Received send request from MW\n");
								FD_SET(i, &mwSet);
								FD_CLR(i, &masterFdSet);
							}
							// We are about to receive stuff from a client
							else {
								debug_out(5, "Listen: Received send request from a client\n");
								if(ConnectionHandler(GET_BY_SOCKET, &conn, NULL, NULL, i) == 0) {
									conn.numCmds = *((long *)recvBuf);
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
							debug_out(5, "Listen: Unknown error occured\n");
							/* TODO: Add error handling */
						}
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

