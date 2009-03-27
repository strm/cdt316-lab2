#include "soups.h"
#include "connection_list.h"
#include "global.h"
#include "listen_thread.h"
/*
 * Thread to check if all connected sockets are alive.
 */
void * aliveThread(void * arg){
	connections_t list;
	int n, msg = MW_ALIVE;
	message_t * nodeMsg;
	node * nNode;
	while(1){
		sleep(20); //place holder
		/*
		 * Get the connection list
		 */
		if(!ConnectionHandler(LIST_COPY, 0, NULL, &list)){
			for(n = 0; n < list.nConnections; n++){
				nodeMsg = createMessage(-1, "", "", "", 1);
				nNode = createNode(nodeMsg);
				if(send(list.connection->socket, msg, sizeof(msg), 0) == -1){
					//need to be removed from the list
					debug_out(4, "No connection from socket: %d", list.connection->socket);
					if(ConnectionHandler(LIST_REMOVE, list.connection->socket, NULL, NULL) == -1)
						debug_out(5, "Failed LIST_REMOVE");
					/*
					 * Create message for work_thread
					 */
					nNode->msg.msgType = MW_ALIVE;
					nNode->msg.endOfMsg = 1;
					nNode->msg.socket = list.connection->socket;

					globalMsg(MSG_LOCK, MSG_NO_ARG);
					globalMsg(MSG_PUSH, nNode);
					globalMsg(MSG_UNLOCK, MSG_NO_ARG);
				}	
			}
		}
		else
			debug_out(5, "Failed to copy connection list");
	}	
}
