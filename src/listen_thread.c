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

int HandleMessage(message_t *msg, int from) {
	int ret = 0;
	node *newNode;
	int i;
	int next_id, result;
	varList *list = NULL, *it;
	message_t tmp;

	//if(globalId(ID_CHECK, msg->msgId)) {
	if(msg->endOfMsg) ret = MW_EOF;

	switch(msg->msgType) {
		case MW_TRANSACTION:
		case MW_COMMIT:
		case MW_ACK:
		case MW_NAK:
			// TODO: Check if we need to synchronize:
			debug_out(4, "msgType: %d\n", msg->msgType);
			for(i = 0; i < 8; i++) {
				debug_out(4, "Got %d %s %s %s\n",
						msg->data[i].op,
						msg->data[i].arg1,
						msg->data[i].arg2,
						msg->data[i].arg3);
			}
			debug_out(4, "---\n");
			msg->socket = from;
			newNode = createNode(msg);
			globalMsg(MSG_LOCK, MSG_NO_ARG);
			globalMsg(MSG_PUSH, newNode);
			globalMsg(MSG_UNLOCK, MSG_NO_ARG);
			break;
		case MW_SYNCHRONIZE:
			debug_out(5, "Got SYNCHRONIZE request from other middleware\n");
			/*if(globalId(ID_CHECK, msg->msgId)) { // everything seems to be in order
				debug_out(5, "According to globalId everything is fine\n");
			}
			else { // Sender seems to have missed something earlier*/
				debug_out(5, "According to globalId something needs to sync\n");
				LogHandler(LOG_GET_NEXT_POST_ID, msg->msgId, NULL, &next_id);
				while(next_id != LOG_NO_ID) {
					debug_out(5, "Reading a helluvalot of log posts: %d\n", next_id);
					LogHandler(LOG_READ_POST, next_id, &list, &result);
					debug_out(5, "Read a log post: %d\n", next_id);
					LogHandler(LOG_GET_NEXT_POST_ID, next_id, NULL, &next_id);
				}
				debug_out(5, "Got out of while loop before seg fault\n");

				tmp.sizeOfData = 0;
				tmp.msgType = MW_TRANSACTION;
				tmp.endOfMsg = 0;
				tmp.msgId = result;
				tmp.owner = 0;

				for(it = list; it != NULL; it = it->next) {
					debug_out(5, "Looping varlist\n");
					tmp.data[tmp.sizeOfData] = it->data;
					if(tmp.sizeOfData == MSG_MAX_DATA) {
						debug_out(5, "Max data reached for tmp\n");
						if(it->next == NULL) {
							debug_out(5, "There are no more messages\n");
							tmp.endOfMsg = MW_EOF;
						}
						else
							debug_out(5, "There are more messages\n");
						mw_send(from, (void *)&tmp, sizeof(tmp));
						tmp.sizeOfData = 0;
					}
					tmp.sizeOfData++;
				}
				debug_out(5, "Got out of forloop\n");
				if(tmp.endOfMsg != MW_EOF) {
					tmp.endOfMsg = MW_EOF;
					mw_send(from, (void *)&tmp, sizeof(tmp));
				}

				tmp.msgType = MW_COMMIT;
				tmp.owner = -1;
				tmp.socket = -1;
				tmp.msgId = result;
				mw_send(from, (void *)&tmp, sizeof(tmp));
			//}
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
			close(from);
			break;
	}
	//}
	/*else { // The message id was lower then we expected
		debug_out(3, "undefined stuff happening\n");
		tmp.msgId = globalId(ID_GET, 0);
		tmp.msgType = MW_NAK;
		tmp.endOfMsg = TRUE;
		tmp.owner = msg->msgId;

		if(send(from, (void *)&tmp, sizeof(message_t), 0) < 0) {
		perror("send: ");
		}
		}*/

	return ret;
}

void *ListeningThread(void *arg) {
	fd_set readFdSet; /* Used by select */
	int i, nBytes, count, ret;
	void * recvBuf;
	long first_recv;
	connection *connections;
	connection conn, *it;
	int connectionSocket;
	struct sockaddr connectInfo;
	socklen_t connectInfoLength = sizeof(connectInfo);
	int listenSocket = *((int *)arg);
	struct timeval selectTimeout;
	node *tmp_msg = NULL;
	message_t msg_data;
	debug_out(5, "DB_GLOBAL: %s\n", DB_GLOBAL);

	debug_out(5, "Listenthread has started\n");

	// The thread should wait for new messages when not processing something
	while(1) {
		count = 0;
		selectTimeout.tv_sec = 0;
		selectTimeout.tv_usec = 50000;
		FD_ZERO(&readFdSet);
		FD_SET(listenSocket, &readFdSet);

		ConnectionHandler(
					COPY_LIST,
					NULL,
					&connections,
					NULL,
					0);
		for(it = connections; it != NULL; it = it->next) {
			FD_SET(it->socket, &readFdSet);
			count++;
		}
		DeleteConnectionList(&connections);

		//debug_out(3, "Entering selective state with %d connections\n", count);
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
					// New connection incoming
					if(i == listenSocket) {
						debug_out(5, "Listen: new connection incoming\n");
						connectionSocket = accept(listenSocket, &connectInfo, &connectInfoLength);
						if(connectionSocket < 0) {
							perror("accept");
						}
						else {
							recvBuf = malloc(sizeof(long));
							debug_out(3, "Waiting for connection identification\n");
							nBytes = force_read(connectionSocket, recvBuf, sizeof(long));
							if(nBytes > 0) {
								first_recv = ntohl(*((long *)recvBuf));
								if(first_recv > 0) { // This is a client
									debug_out(3, "Client connection established\n");
									msg_data = newMsg();
									msg_data.msgType = MW_TRANSACTION;
									msg_data.owner = MSG_ME;
									msg_data.socket = connectionSocket;
									msg_data.sizeOfData = first_recv;
									tmp_msg = createNode(&msg_data);
									globalMsg(MSG_PUSH, tmp_msg);
									debug_out(3, "Pushed shit on the queue\n");
								}
								else {
									debug_out(3, "Middleware connection established\n");
									CreateConnectionInfo(
											&conn,
											connectionSocket,
											"Unknown",
											TYPE_MIDDLEWARE,
											0);
									if(ConnectionHandler(
											ADD_TO_LIST,
											&conn,
											NULL,
											NULL,
											0) != 0) {
										debug_out(3, "Could not add connection to list\n");
									}
									// Middleware connection here
								}
							}
							else {
								debug_out(3, "This reading thing is hard\n");
								debug_out(3, "Read returned '%d' '%ld'\n", nBytes, (*((long *)recvBuf) = *((long *)recvBuf)));
								// Error reading, endpoint probably crashed or hanged up
							}
						}
					}
					// Connected middleware wants to send stuff
					else {
						recvBuf = malloc(sizeof(message_t));
						debug_out(3, "Reading from another middleware\n");
						nBytes = force_read(i, recvBuf, sizeof(message_t));
						if(nBytes > 0) {
							HandleMessage((message_t *)recvBuf, i);
						}
						else if (nBytes == -1) {
							perror("force_read");
							if(ConnectionHandler(REMOVE_BY_SOCKET, NULL, NULL, 0, i) == 0){
								debug_out(5, "Removed socket %d from connection list\n", i);
								msg_data.msgType = MW_ALIVE;
								msg_data.socket = i;
								msg_data.endOfMsg = 1;
								msg_data.msgId = -75;
								globalMsg(MSG_LOCK, MSG_NO_ARG);
								globalMsg(MSG_PUSH, createNode(&msg_data));
								globalMsg(MSG_UNLOCK, MSG_NO_ARG);
							}
							else
								debug_out(5, "Failed to remove interrupted connection\n");
							/* TODO: Add error handling */
							FD_CLR(i, &readFdSet);
						}
						else {
							debug_out(5, "Listen: force_read returned EOF, terminating connection\n");
							if(ConnectionHandler(REMOVE_BY_SOCKET, NULL, NULL, 0, i) == 0) {
								debug_out(5, "Removed socket %d from connection list\n", i);
								msg_data.msgType = MW_ALIVE;
								msg_data.socket = i;
								msg_data.endOfMsg = 1;
								msg_data.msgId = -75;
								globalMsg(MSG_LOCK, MSG_NO_ARG);
								globalMsg(MSG_PUSH, createNode(&msg_data));
								globalMsg(MSG_UNLOCK, MSG_NO_ARG);
							}
							else {
								debug_out(5, "Failed to remove interrupted connection\n");
							}
							FD_CLR(i, &readFdSet);
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

