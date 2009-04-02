/*
 * Text: Worker thread (parses and commits transactions)
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: 19 March 2009
 */

#include "work_thread.h"


/**
 * Parse a transaction locally
 */
int ParseTransaction(transNode ** trans){
	//get a list of all used variables
	if( getUsedVariables(&((*trans)->parsed), (*trans)->unparsed) == 0 ){
		debug_out(5, "getUsedVariables (failed)\n");
		return LOCALPARSE_FAILED;
	}
	//lock transaction
	if(lockTransaction(*trans) == 0){
		debug_out(6, "lockTransaction (failed)\n");
		return LOCALPARSE_NO_LOCK;
	}
	//do localparse
	if(localParse(&((*trans)->parsed), (*trans)->unparsed) == 0 ){
		debug_out(5, "localParse (failed)\n");
		return LOCALPARSE_FAILED;
	}

	return LOCALPARSE_SUCCESS;

}

void * worker_thread ( void * arg ){
	int sock;
	sock = (int ) (arg);
	node * tmp;
	connection *it;
	int n, counter, res;
	transNode * transList = NULL;
	transNode * trans; //TODO does stuff get lost?
	message_t newMsg;
	varList *iter;
	command cmd;
	debug_out(5,"Welcome to worker_thread\n");
	while(1){
		/*
		 * Read message queue
		 */
		if ( globalMsg( MSG_LOCK, MSG_NO_ARG ) == NULL ){
			//lockedi
			tmp = globalMsg( MSG_POP, MSG_NO_ARG );
			if( tmp == NULL ){
				globalMsg( MSG_UNLOCK, MSG_NO_ARG );
				continue;
			}
			else{
				debug_out(5, "Message recived %d\n", tmp->msg.msgType);
				if(isTransaction(transList, tmp->msg.msgId)){
					trans = getTransaction(transList, tmp->msg.msgId);
					debug_out(3, "getTransaction(%d)\n", trans->id);
				}
				else if(tmp->msg.msgType == MW_TRANSACTION){
					//create new transaction
					if(tmp->msg.owner == MSG_ME)
						trans = createTransaction( globalId(ID_GET, 0 ));
					else{
						trans = createTransaction(tmp->msg.msgId);
						globalId(ID_CHANGE, tmp->msg.msgId);
					}
					debug_out(5, "Creating new transaction(%d)\n", trans->id);
					if(!addTransaction( &transList, trans))
						debug_out(5, "Failed to add transaction to list\n");
					else{
						debug_out(3, "Setting up new transaction\n");
						trans->owner = tmp->msg.owner;
						if(ConnectionHandler(COPY_LIST, NULL, &(trans->conList), NULL, 0) != 0)
							debug_out(5, "Error %d\n", trans->conList);
						else
							debug_out(3, "We have a list\n");
						trans->socket = tmp->msg.socket;
						if(trans->owner == MSG_ME){
						/**
						 * READ FROM CLIENT HERE
						 * TODO TODO TODO TODO
						 */
						counter = 0;
						printf("read from client %d\n", tmp->msg.sizeOfData);
						for(n = tmp->msg.sizeOfData; n; n--){
							res = read(tmp->msg.socket, &cmd, sizeof(command));
							if (res < sizeof(command)){
								my_perror(2, "force_read");
							}
							else{
								cmd.op = ntohl(cmd.op);
								varListPush(cmd,(&trans->unparsed));
								counter++;
							}
						}
						if(counter == tmp->msg.sizeOfData)
							tmp->msg.endOfMsg = MW_EOF;
						}
					}
				}
			}
			switch(tmp->msg.msgType){
				case MW_TRANSACTION:
					switch(trans->owner){
						case MSG_ME:
							//from client
							if(tmp->msg.endOfMsg == MW_EOF){
								switch(ParseTransaction(&trans)){
									case LOCALPARSE_SUCCESS:
										//send to all clients!
										counter = -1;
										newMsg.msgType = MW_TRANSACTION;
										newMsg.endOfMsg = 0;
										newMsg.msgId = tmp->id;
										newMsg.sizeOfData = -1;
										newMsg.owner = -1;

										iter = trans->parsed;
										if(iter != NULL){
											for(;iter != NULL; iter = iter->next){
												counter++;
												newMsg.sizeOfData++;
												newMsg.data[newMsg.sizeOfData] = iter->data;
												if(counter % 7){
													/*
													 * Send Message here
													 */
													if(iter->next == NULL)
														newMsg.endOfMsg = MW_EOF;
													for(it = trans->conList; it != NULL; it = it->next) {
													//for(n = 0; n < trans->conList.nConnections; n++){

												debug_out(3, "Gonna send to socket %d\n", it->socket);
														mw_send(it->socket, &newMsg, sizeof(message_t));
													}
													//clean up
													newMsg.sizeOfData = -1;
												}
											}
											if(newMsg.endOfMsg != 1){
												newMsg.endOfMsg = MW_EOF;
												for(it = trans->conList; it != NULL; it = it->next) {
												//for(n = 0; n < trans->conList.nConnections; n++){
													mw_send(it->socket, &newMsg, sizeof(message_t));
												}
											}
										}
										debug_out(5, "Local Parse(DONE) send to client(DONE)\n");
										break;
									case LOCALPARSE_FAILED:
										//unrecoverable error
										debug_out(5, "Failed to parse transaction from client\n");
										if(removeAll(tmp->msg.msgId));
										else
											debug_out(5, "removeAll (failed)\n");
										if(removeTransaction(&transList, tmp->msg.msgId));
										else
											debug_out(5, "removeTransaction (failed)\n");
										/* TODO
										 * Send Response to client
										 */
										break;
									case LOCALPARSE_NO_LOCK:
										//empty parsed list
										for(cmd = varListPop(&(trans->parsed)); cmd.op != MAGIC; cmd = varListPop(&(trans->parsed)));
										//add message again to queue
										if(globalMsg( MSG_PUSH, tmp ) == NULL)
											debug_out(5, "globalMsg (failed)\n");
										break;
								}
							}
							break;
						default:
							debug_out(4, "Transaction recived from mw\n");
							//from middleware;
							for ( n = 0; n < tmp->msg.sizeOfData; n++ )
								varListPush(tmp->msg.data[n], (&trans->parsed));
							if(tmp->msg.endOfMsg == MW_EOF){
								//Try to lockvariables
								if(lockTransaction(trans) == 0){
									debug_out(6, "lockTransaction (failed), sending nak frame\n");
									/*
									 * Create a nak frame
									 */
									newMsg.msgType = MW_NAK;
									newMsg.msgId = tmp->msg.msgId;
									newMsg.endOfMsg = MW_EOF;
									newMsg.sizeOfData = 0;
									newMsg.owner = MSG_ME;
									newMsg.nMiddlewares = 0;
									//remove transaction
									if(removeAll(tmp->msg.msgId));
									else
										debug_out(5, "removeAll (failed)\n");
									if(removeTransaction(&transList, tmp->msg.msgId));
									else
										debug_out(5, "removeTransaction (failed)\n");
								}
								else{
									//transaction locked send ack
									/*
									 * Create a ACK frame
									 */
									newMsg.msgType = MW_ACK;
									newMsg.msgId = tmp->msg.msgId;
									newMsg.endOfMsg = MW_EOF;
									newMsg.sizeOfData = 0;
									newMsg.owner = MSG_ME;
									newMsg.nMiddlewares = 0;
								}
								/*
								 * Send Frame
								 */
								mw_send(tmp->msg.socket, &newMsg, sizeof(message_t));
							}
					}
					break;
				case MW_ACK:
					if(GetConnectionBySocket(&(trans->conList), it, tmp->msg.socket) == 0){
						it->ack = 1;
						if(RemoveConnectionBySocket(&(trans->conList), tmp->msg.socket) == 0)
							if(AddConnection(&(trans->conList), it) == 0);
							else
								debug_out(5, "AddConnection != 0\n");
						else
							debug_out(5, "RemoveConnectionBySocketFailed\n");
						for(it = trans->conList; it != NULL; it = it->next){
							if(it->ack == 0)
								break;
								
								
						}
					}
					else
						debug_out(5, "GetConnectionBySocket != 0\n");
					break;
				case MW_NAK:
					switch(trans->owner){
						case MSG_ME:
							//unlock
							if(removeAll(trans->id)){

								newMsg.msgType = MW_NAK;
								newMsg.msgId = tmp->msg.msgId;
								newMsg.endOfMsg = MW_EOF;
								newMsg.sizeOfData = 0;
								newMsg.owner = MSG_ME;

								for(it = trans->conList; it != NULL; it = it->next) {
								//for(n = 0; n < trans->conList.nConnections; n++){
									mw_send(it->socket, &newMsg, sizeof(newMsg));
								}
								//need to do this one again
								for(cmd = varListPop(&(trans->parsed)); cmd.op != MAGIC; cmd = varListPop(&(trans->parsed)));
								newMsg.msgType = MW_TRANSACTION;
								newMsg.endOfMsg = MW_EOF;
								newMsg.msgId = trans->id;

								globalMsg(MSG_PUSH, createNode(&newMsg));
							}
							else
								debug_out(5, "removeAll (failed)\n");
							break;
						default:
							//throw out transaction
							if(removeAll(tmp->msg.msgId));
							else
								debug_out(5, "removeAll (failed)\n");
							if(removeTransaction(&transList, tmp->msg.msgId));
							else
								debug_out(5, "removeTransaction (failed)\n");
							break;
					}
					break;
				case MW_COMMIT:
					/*
					 * Update transaction to db
					 */
					if(commitParse(trans))
						n = sendResponse(trans);
							if(n)
								debug_out(4, "Response sent to client\n");
							else if(!n)
								debug_out(4, "Failed to send response to client\n");
							else;
								//we are not the owner
					break;
				case MW_ALIVE:
					/**
					 * Update connection list here
					 */
					break;
			} //msgtype switch
		} //unlock after here
		globalMsg( MSG_UNLOCK, MSG_NO_ARG );
	} /* end of while(1) */
	return (void *) 0;
}
