#include "soups.h"
#include "connections.h"
#include "global.h"
#include "listen_thread.h"
/*
 * Thread to check if all connected sockets are alive.
 */
void * aliveThread(void * arg){
	connection *list;
	connection *it;
	int msg = MW_ALIVE;
	message_t * nodeMsg;
	node * nNode;
	while(1){
		sleep(20); //place holder
		/*
		 * Get the connection list
		 */
		if(!ConnectionHandler(COPY_LIST, NULL, &list, NULL, 0)){
			for(it = list; it != NULL; it = it->next) {
			//for(n = 0; n < list.nConnections; n++){
				nodeMsg = createMessage(-1, "", "", "", 1);
				nNode = createNode(nodeMsg);
				if(send(it->socket, &msg, sizeof(msg), 0) == -1){
					//need to be removed from the list
					debug_out(4, "No connection from socket: %d", it->socket);
					if(ConnectionHandler(REMOVE_BY_SOCKET, NULL, NULL, NULL, it->socket) == -1)
						debug_out(5, "Failed LIST_REMOVE");
					/*
					 * Create message for work_thread
					 */
					nNode->msg.msgType = MW_ALIVE;
					nNode->msg.endOfMsg = 1;
					nNode->msg.socket = it->socket;

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
