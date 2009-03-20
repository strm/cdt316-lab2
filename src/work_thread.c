/*
 * Text: Worker thread (parses and commits transactions)
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: 19 March 2009
 */

#include <time.h>
#include <string.h>
#include "../framework/cmd.h"
#include "../framework/middle-support.h"
#include "middle_com.h"
#include "listen_thread.h"
#include "connection_list.h"
#include "global.h"

#define LOCALPARSE_SUCCESS	(1)
#define LOCALPARSE_NO_LOCK	(2)
#define LOCALPARSE_FAILED		(0)

int localParseBackup(int sock){
	/*
	 * Testing local parser
	 */
	printf("Welcome to local parser\n");
	//socket settings
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);
	int psock;
	int res;
	long num_cmds;
	//transaction
	transNode * trans = createTransaction(0); //TODO add uniq id here
	//open socked to client
	psock = accept(sock, (struct sockaddr *)&addr, &addrlen);
	if(!psock){
		debug_out(2, "Failed to accept incoming connection.\n");
		exit(1);
	}
	printf("Accepted client connection maybe\n");
	//read number of commands
	res = force_read(psock, &num_cmds, sizeof(num_cmds));
	if( res < sizeof(num_cmds) ){
		if( res < 0) 
			my_perror(2, "force_read()");
		else
			debug_out(2, "Failed to read number of lines (%d)\n");
	}
	num_cmds = ntohl(num_cmds);
	num_rsp = 0;
	//Read all commands
	for (; num_cmds; num_cmds--){
		command cmd;
		response rsp;
		memset(&rsp, 0, sizeof(rsp));
		//read the command
		res = force_read(psock, &cmd, sizeof(command));
		if(res < sizeof(command)){
			if( res  < 0 )
				my_perror(2, "force_read");
			else
				debug_out(2, "Failed to read command (%d)\n");
			exit(1);
		}

		cmd.op = ntohl(cmd.op);
		rsp.seq = cmd.seq;
		/*
		 * Add to list of unparsed commands here
		 */
		if(!varListPush(cmd, &(trans->unparsed))){
			debug_out(5, "Couldn't add command struct to varList\n");
			exit(2);
		}
		//TODO DEBUG EXTRA TODO
		if(!varListFind(cmd.arg1, (trans->unparsed))){
			debug_out(5, "Command struct not found in list\n");
			exit(2);
		}

	}
	//TODO CLOSE PSOCK FOR TESTING ONLY TODO
	close(psock);
	//parse the list of commands
	printf("GET LIST OF USED VARIABLES\n");
	if( getUsedVariables(&(trans->parsed), trans->unparsed) == 0){
		debug_out(5, "Failed to create a list of variables from the unparsed list\n");
		exit(2);
	}
	//Do local parsing
	printf("LOCALPARSE\n");
	if( !localParse(&(trans->parsed), trans->unparsed) ){
		debug_out(5, "localParse Failed\n");
		exit(2);
	}
	//print the list
	printf("PRINY LOCALLY PARSED VALUES\n");
	varList * iter = trans->parsed;
	while ( iter != NULL ){
		printf("{%s: %s }", iter->data.arg1, iter->data.arg2);
		iter = iter->next;
	}
	printf("\nThats it\n");
	/*
	 * End parser tests.
	 */
#endif
	pthread_join(listenThread, NULL);
	return 0;
}
/**
 * Parse a transaction locally
 */
int ParseTransaction(transNode ** trans){
	//get a list of all used variables
	if( getUsedVariables((trans->parsed), (*trans)->unparsed) == 0 ){
		debug_out(5, "getUsedVariables (failed)\n");
		return LOCALPARSE_FAILED;
	}
	//lock transaction
	if(lockTransaction(*trans) == 0){
		debug_out(6, "lockTransaction (failed)\n");
		return LOCALPARSE_NO_LOCK;
	}
	//do localparse
	if(localParse(trans->parsed, (*trans)->unparsed) == 0 ){
		debug_out(5, "localParse (failed)\n");
		return LOCALPARSE_FAILED;
	}

	return LOCALPARSE_SUCCESS;

}

void * worker_thread ( void * arg ){
	int sock = (int) (*arg);
	node * tmp;
	int n;
	transNode * transList;
	transNode * trans; //TODO does stuff get lost?
	message_t newMsg;
	while(1){
		/*
		 * Read message queue
		 */
		if ( globalMsg( MSG_LOCK, MSG_NO_ARG ) == NULL ){
			//locked
			tmp = globalMsg( MSG_POP, MSG_NO_ARG );
			if( tmp == NULL )
				continue;
			else if(isTransaction(transList, tmp->msg.msgId))
				trans = getTransaction(transList, tmp->msg.msgId);
			else if(tmp->msg.msgType == MW_TRANSACTION){
				//create new transaction
				trans = createTransaction( globalId(ID_GET, 0 ));
				if(!addTransaction( &transList, trans))
					debug_out(5, "Failed to add transaction to list\n");
				else{
					trans->owner = tmp->msg.owner;
					//TODO COPY FROM LIST TODO trans->acks = tmp->msg.nMiddlewares;
					trans->id = tmp->msg.msgId;
					trans->socket = tmp->msg.socket;
				}
			}
			else //do nothing
				continue;
	
			switch(tmp->msg.msgType){
				case MW_TRANSACTION:
					switch(tmp->owner){
						case MSG_ME:
							//from client
							for( n = 0; n < tmp->msg.sizeOfData; n++ )
								varListPush(tmp->msg.data[n],(&trans->unparsed));
							if(tmp->msg.endOfMsg == MW_EOF){
								switch(ParseTransaction(&trans)){
									case LOCALPARSE_SUCCESS:
										//TODO send to all clients!
										break;
									case LOCALPARSE_FAILED:
										//unrecoverable error
										debug_out(5, "Failed to parse transaction from client\n");
										if(removeAll(tmp->msg.msgId));
										else
											debug_out(5, "removeAll (failed)\n");
										if(removeTransaction(&transList, tmp->msg.msgId));
										else
											debug_out(5, "removeTransaction (failed)\m");
										break;
									case LOCALPARSE_NO_LOCK:
										//add message again to queue
										if(globalMsg( MSG_PUSH, tmp ) == NULL)
											debug_out(5, "globalMsg (failed)\n");
										break;
								}
							}
							break;
						default:
							//TODO THIS IS FOR MW_EOF ONLY TODO
							//from middleware;
							for ( n = 0; n < tmp->msg.sizeofData; n++ )
								varListPush(tmp->msg.data[n], (&trans->parsed));
							if(tmp->msg.endOfMsg == MW_EOF){
							//Try to lockvariables
								if(lockTransaction(*trans) == 0){
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
										debug_out(5, "removeTransaction (failed)\m");
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
								soupSend(tmp->msg.socket, &newMsg, sizeof(message_t), 0);
							}
					}
					break;
				case MW_ACK:
					//TODO CHANGE TODO
					trans->acks--;
					if(trans->acks <= 0){
						//TODO send commit message to everyone TODO

					}
					break;
				case MW_NAK:
					switch(trans->owner){
						case MSG_ME:
							//unlock
							if(removeAll(trans->id)){
								//need to do this one again
								/* TODO send nak to everyone */
								trans->id = globalId ( ID_GET, 0 ); //get a new id
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
								debug_out(5, "removeTransaction (failed)\m");
							break;
					}
					break;
				case MW_COMMIT:
					/*
					 * Update transaction to db
					 */

					break;
			} //msgtype switch
		} //unlock after here
		globalMsg( MSG_UNLOCK, MSG_NO_ARG )
	} /* end of while(1) */
	return (void *) 0;
}

