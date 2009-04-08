/*
 * Text: Functions for parsing a transaction locally.
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: Today
 */
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg_queue.h"
#include "work_thread.h"
/*
 * Get a list of all used variables in transaction
 */
int getUsedVariables(varList ** var, varList * trans){
	int ret = 0;
	command tmp;	
	debug_out(3, "getUSedVariables started\n");
	while ( trans != NULL ){
		//check all args
		debug_out(3, "data.op = %d arg1 = %s\n", trans->data.op, trans->data.arg1);
		if(trans->data.op <= 6){
			if(is_entry(trans->data.arg1))
				if(!varListFind(trans->data.arg1, (*var))){
					ret++;
					tmp.op = 1;
					strcpy(tmp.arg1, trans->data.arg1);
					varListPush(tmp, var);
					strcpy(tmp.arg1, "\0");
				}
			if(is_entry(trans->data.arg2))
				if(!varListFind(trans->data.arg2, (*var))){
					ret++;
					tmp.op = 1;
					strcpy(tmp.arg1, trans->data.arg2);
					varListPush(tmp, var);
					strcpy(tmp.arg1, "\0");
				}
			if(is_entry(trans->data.arg3))
				if(!varListFind(trans->data.arg3, (*var))){
					ret++;
					tmp.op = 1;
					strcpy(tmp.arg1, trans->data.arg3);
					varListPush(tmp, var);
					strcpy(tmp.arg1, "\0");
				}
		}
		trans = trans->next;
		printf("*");
	}
	
	return ret;	
}

/*
 * Get all variables current value from db
 */
int getFromDB(varList ** var){
	varList * iter = (*var);
	int nRet = 1;
	char ret[ARG_SIZE];

	while( iter != NULL ){
	//ret = (char *) malloc(sizeof(char)*ARG_SIZE);
		if(is_entry(iter->data.arg1)){
			debug_out(3, "%s %s\n", iter->data.arg1, ret);
			if(get_entry(ret, DB_GLOBAL, iter->data.arg1) == FALSE){
				debug_out(4, "No value retrived for %s\n", iter->data.arg1);
				strncpy(ret, " ", ARG_SIZE);
				strncpy(iter->data.arg2, ret, ARG_SIZE);
				debug_out(3,"\n\n%s\n",iter->data.arg2);
				nRet--;
			}
			else{
				//sucess
				debug_out(4, "Retrived { %s } for %s", ret, iter->data.arg1);
				strcpy(iter->data.arg2, "");
				strncpy(ret, "", ARG_SIZE);
			}
		}
		iter = iter->next;
	}
	return nRet;
}	

/**
 * Parses a varList locally but gets the values from the db
 */
int localParse(varList ** var, varList * trans){
	varList * iter = trans;
	command tmp;
	char * value = (char *) malloc(sizeof(char)*ARG_SIZE);
	int xValue, yValue, result;
	printf("\nBegin local parse\n");
	getFromDB(var);
	while( iter != NULL ){
		tmp = iter->data;
		switch (tmp.op) {
			case 1: //ASSIGN <OUT> = <IN>
				if( is_entry(tmp.arg1) && varListFind(tmp.arg1, (*var)) ){
					if( is_entry(tmp.arg2) && varListFind(tmp.arg2, (*var)) ){
						//assign value of another post to a post
						//arg1 = arg2
						value = varListGetValue((*var), tmp.arg2);
						if( value != NULL ){
							if(varListSetValue(var, tmp.arg1, value))
								printf("ASSIGN %s %s (DONE)\n", tmp.arg1, value);
							else
								printf("ASSIGN %s %s (FAIL)\n", tmp.arg1, value);
							value = NULL;
						}

					}
					else{
						//assume we have a actual value in arg2
						if(varListSetValue(var, tmp.arg1, tmp.arg2))
							printf("ASSIGN %s %s (DONE)\n", tmp.arg1, tmp.arg2);
						else
							printf("ASSIGN %s %s (FAIL)\n", tmp.arg1, tmp.arg2);
					}

				}
				else
					printf("ASSIGN %s ? (FAIL)\n", tmp.arg1);
				break;
			case 2: 
				debug_out(4, "//ADD <OUT> = <IN> + <IN>\n");
				if( is_entry(tmp.arg1) && varListFind(tmp.arg1, (*var)) ){
					//X value
					if( is_entry(tmp.arg2) && varListFind(tmp.arg2, (*var)) ){
						value = varListGetValue((*var), tmp.arg2);
						if(value != NULL){
							xValue = atoi(value);
						}
						else{
							printf("ADD %s %s ? (FAIL)", tmp.arg1, tmp.arg2);
							break;
						}
					}
					else{
						debug_out(4, "x = atoi\n");
						xValue = atoi(tmp.arg2);
					}
					//y value
					if( is_entry(tmp.arg3) && varListFind(tmp.arg3, (*var)) ){
						value = varListGetValue((*var), tmp.arg3);
						if( value != NULL){
							yValue = atoi(value);
						}
						else{
							printf("ADD %s %s %s (FAIL)", tmp.arg1, tmp.arg2, tmp.arg3);
							break;
						}
					}
					else{
						debug_out(4, "y = atoi\n");
						yValue = atoi(tmp.arg3);
					}

					debug_out(4, "do math\n");
					result = xValue + yValue;
					//do itoa(fake) and assing to tmp.arg1
					sprintf(value, "%d", result);
					//assign value
					debug_out(4, "ADD value to list\n");
					if(varListSetValue(var, tmp.arg1, value))
						printf("ADD %s %s %s (DONE)\n", tmp.arg1, tmp.arg2, tmp.arg3);
					else
						printf("ADD %s %s %s (FAIL)\n", tmp.arg1, tmp.arg2, tmp.arg3);

				}
				else
					printf("ADD %s ? ? (FAIL)\n", tmp.arg1);
				break;
			case PRINT:
				//nothing to see here move along
				break;
			case DELETE:
				if( is_entry(tmp.arg1) && varListFind(tmp.arg1, (*var)) ){
					//set string to  \0 to ask for a delete if value doesnt change
					//TODO is this a good idea
					if(varListSetValue(var, tmp.arg1, "DELETE"))
						printf("DELETE %s (DONE)", tmp.arg1);
					else
						printf("DELETE %s (FAIL)", tmp.arg1);
				}
				else
					printf("DELETE %s (FAIL)", tmp.arg1);
				break;
			case SLEEP:
				printf("Going to sleep for %s\n", tmp.arg1);
				if( is_entry(tmp.arg1) && varListFind(tmp.arg1, (*var)) ){
					value = varListGetValue((*var), tmp.arg1);
					sleep(atoi(value));
				}
				else
					sleep(atoi(tmp.arg1));
				break;
			case IGNORE:
				//TODO should we do this
				break;
			case MAGIC:
				//TODO should we do this
				break;
			case QUIT:
				/*
				 * Should not be parsed
				 */
				break;
			case NOCMD:
				printf("NO CMD\n");
				break;

		}
		iter = iter->next;
	}
	/*
	 * Flag deleted items
	 */
	iter = (*var);
	while ( iter != NULL ){
		if( strcmp(iter->data.arg2, "DELETE") == 0 ){
			iter->data.op = DELETE;
			strcpy(iter->data.arg2, "\0");
		}
		iter = iter->next;
	}
	return 1; //TODO WHAT THE ?
}

/*
 * Commits a parsed transaction to the database and sends any print responses to the client
 */
int commitParse(transNode * trans){
	varList *iter = (trans->parsed);
	debug_out(5, "COMMIT TRANSACTION %d\n", trans->id);
	for(;iter != NULL; iter = iter->next){
		switch(iter->data.op){
			case ASSIGN:
				if(replace_entry(iter->data.arg2, DB_GLOBAL, iter->data.arg1))
					debug_out(4, "ASSIGN: %s to %s\n", iter->data.arg2, iter->data.arg1);
				else
					debug_out(5, "replace_entry (failed)\n");
				break;
			case DELETE:
				if(delete_entry(DB_GLOBAL, iter->data.arg1));
				else
					debug_out(5, "delete_entry (failed)\n");
			case ADD:
				break;
			case PRINT:
				//no support here
				break;
			case SLEEP:
				break;
			case IGNORE:
				break;
			case MAGIC:
				break;
			case QUIT:
				break;
			case NOCMD:
				break;
		}
	}
	return 1;
}
/*
	* Finds all print commands and send them to the client if owner of the transaction
	* 0 on error
	* 1 on suceess?
	* -1 on nothing done
	*/
int sendResponse(transNode * trans){
	varList * iter = trans->unparsed;
	response rsp;
	int nRsp = 0;
	int kPn = 0;
	if(trans->owner == MSG_ME && iter != NULL){
		//get number of responses needed
		debug_out(5, "Sending all prints to client %d \n", trans->owner);
		while(iter != NULL){
			if(iter->data.op == PRINT)
				nRsp++;
			iter = iter->next; 
		}
		if(nRsp >= 0){
			/**
			 * Send amount of responses to client
			 */
			nRsp = htonl(nRsp);
			if(send(trans->socket, &nRsp, sizeof(int), MSG_NOSIGNAL) == -1){
				debug_out(5, "send to client (failed)\n");
				return 0;
			}
			else if(nRsp == 0) {
				return 0;
			}
			else{
				for(iter = trans->unparsed; iter != NULL; iter = iter->next){
				debug_out(5, "Trying to send response %s\n", iter->data.arg1);
					if(iter->data.op == PRINT){
						debug_out(3, "Sending response %d", kPn);
						rsp.seq = htonl(kPn);
						kPn++;
						rsp.is_message = 0;
						rsp.is_error = 0;
						//check what type of print
						debug_out(3, "Trying to check what type of data to be sent\n");
						if(is_entry(iter->data.arg1))
							strcpy(rsp.result, varListGetValue(trans->parsed, iter->data.arg1));
						else if(is_literal(iter->data.arg1))
							strcpy(rsp.result, iter->data.arg1);
						else{
							debug_out(5, "response (faileD) not a value or an entry\n");
							return 0;
						}
						if(send(trans->socket, &rsp, sizeof(response), MSG_NOSIGNAL) == -1)
							debug_out(5, "Send to clint (failed)\n");
					}
				}
			}
		}
	}
	else
		return -1;
	return 1;
}
