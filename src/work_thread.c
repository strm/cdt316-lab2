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
	varList * test = (*trans)->parsed;
	while(test != NULL){
		debug_out(3, "test->data.arg1 = %s\n", test->data.arg1);
		test = test->next;
	}
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
						if(ConnectionHandler(COPY_LIST,NULL, &(trans->conList), NULL, 0) != 0)
							debug_out(5, "Error %d\n", trans->conList);
						else{
							debug_out(3, "We have a list\n");
							if(trans->conList == NULL)
								debug_out(3, "List broken\n");
						}
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
										counter = 0;
										newMsg.msgType = MW_TRANSACTION;
										newMsg.endOfMsg = 0;
										newMsg.msgId = tmp->id;
										newMsg.sizeOfData = 0;
										newMsg.owner = -1;

										iter = trans->parsed;
										if(iter != NULL){
											for(;iter != NULL; iter = iter->next){
												debug_out(3, "newMsg = %s %s %s\n", iter->data.arg1 , iter->data.arg2, iter->data.arg3);
												newMsg.data[newMsg.sizeOfData] = iter->data;
												if(counter == 7){
													/*
													 * Send Message here
													 */
													if(iter->next == NULL)
														newMsg.endOfMsg = MW_EOF;
													for(it = trans->conList; it != NULL; it = it->next) {
														debug_out(3, "Gonna send to socket %d\n", it->socket);
														mw_send(it->socket, &newMsg, sizeof(message_t));
													}
													//clean up
													newMsg.sizeOfData = 0;
													counter = 0;
												}
												counter++;
												newMsg.sizeOfData++;
											}
											if(newMsg.endOfMsg != MW_EOF){
												newMsg.endOfMsg = MW_EOF;
												for(it = trans->conList; it != NULL; it = it->next) {
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
										cmd.op = -1;
										for(; cmd.op != MAGIC; cmd = varListPop(&(trans->parsed)));
										printf("MESSAGE ID SWITHC = %d %d\n", tmp->msg.msgId, trans->id);
										tmp->msg.msgId = trans->id;
										//add message again to queue
										if(globalMsg( MSG_PUSH, tmp ) == NULL)
											debug_out(5, "globalMsg (failed)\n");
										break;
								}
							}
							break;
						default:
							debug_out(4, "Transaction recived from mw %d\n", tmp->msg.sizeOfData);
							//from middleware;
							for ( n = 0; n < tmp->msg.sizeOfData; n++ ){
								varListPush(tmp->msg.data[n], (&trans->parsed));
								debug_out(2, "Added %s to list", tmp->msg.data[n]);
							}
							if(tmp->msg.endOfMsg == MW_EOF){
								//Try to lockvariables
								if(lockTransaction(trans) == 0){
									debug_out(6, "lockTransaction (failed), sending nak frame\n");
									/*
									 * Create a nak frame
									 */
									debug_out(5, "NAK frame for %d", trans->id);
									newMsg.msgType = MW_NAK;
									newMsg.msgId = tmp->msg.msgId;
									newMsg.endOfMsg = MW_EOF;
									newMsg.sizeOfData = 0;
									newMsg.owner = -1;
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
									debug_out(5, "ACK frame for %d", trans->id);
									newMsg.msgType = MW_ACK;
									newMsg.msgId = tmp->msg.msgId;
									newMsg.endOfMsg = MW_EOF;
									newMsg.sizeOfData = 0;
									newMsg.owner = -1;
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
					debug_out(3, "Ack recived from %d\n", tmp->msg.socket);
					it = trans->conList;
					while(it != NULL){
						debug_out(3, "list socket: %d", it->socket);
						it = it->next;
					}
					it = (connection *)malloc(sizeof(connection));
					if(GetConnectionBySocket(&(trans->conList), it, tmp->msg.socket) == 0){
						it->ack = 1;
						if(RemoveConnectionBySocket(&(trans->conList), tmp->msg.socket) == 0)
							if(AddConnection(&(trans->conList), it) == 0);
							else
								debug_out(5, "AddConnection != 0\n");
						else
							debug_out(5, "RemoveConnectionBySocketFailed\n");
						counter = 1;
						for(it = trans->conList; it != NULL; it = it->next){
							if(it->ack == 0)
								counter--;
						}
						if(counter == 1){
							newMsg.msgType = MW_COMMIT;
							newMsg.msgId = trans->id;
							//send commit to everyone
							debug_out(5, "Sending commit\n");
							globalMsg(MSG_PUSH, createNode(&newMsg));
							for(it = trans->conList; it != NULL; it = it->next){
								mw_send(it->socket, &newMsg, sizeof(newMsg));
							}
						}
					}
					else {
						if(it == NULL) {
							debug_out(3, "'it' is NULL\n");
						}
						debug_out(5, "GetConnectionBySocket != 0 (%d)\n", tmp->msg.socket);
					}
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
								cmd.op = -1;
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
							debug_out(5, "We are commiting trans %d\n", trans->id);
							if(commitParse(trans)){

								if(sendResponse(trans))
									debug_out(4, "Response sent to client\n");
								else
									debug_out(4, "Failed to send response to client\n");
							}
							else;
								debug_out(5, "commitParse(failed)\n");
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
