/*
 * Text: Functions for parsing a transaction locally.
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: Today
 */
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DB_GLOBAL "DATABASE1"

/*
 * Get a list of all used variables in transaction
 */
int getUsedVariables(varList ** var, varList * trans){
	int ret = 0;
	command tmp;	
	printf("getUSedVariables started\n");
	while ( trans != NULL ){
		//check all args
		if((trans->data.op != 3 || trans->data.op >= 5)){
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
	char * ret;
	int nRet = 1;
	
	while( iter != NULL ){
		if(is_entry(iter->data.arg1)){
			if(!get_entry(ret, DB_GLOBAL, iter->data.arg1)){
				printf("No value retrived for %s\n", iter->data.arg1);
				nRet--;
			}
			else{
				//sucess
				printf("Retrived { %s } for %s", ret, iter->data.arg1);
				strcpy(iter->data.arg2, ret);
				ret = "\0";
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
	char * value;
	int xValue, yValue, result;
	printf("\nBegin local parse\n");
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
			case 2: //ADD <OUT> = <IN> + <IN>
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
						yValue = atoi(tmp.arg3);
					}

					//do math
					result = xValue + yValue;
					//do itoa(fake) and assing to tmp.arg1
					sprintf(value, "%d", result);
					//assign value
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
			strcpy(iter->data.arg2, "DELETE(1)");
		}
		iter = iter->next;
	}
	return 1; //TODO WHAT THE ?
}

/*
 * MSG_ME
 */
int commitParse(transNode trans){
	varList *iter = (*trans);
	while(iter != NULL){
		switch(iter->cmd.op){
			case ASSIGN:
				if(replace_entry(iter->cmd.arg2, db, iter,cmd.arg1));
				else
					debug_out(5, "replace_entry (failed)\n");
				break;
		}
	}


}
