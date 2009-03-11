/*
 * Text: Functions for parsing a transaction locally.
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: Today
 */
#include "parser.h"
#include <stdio.h>

#define DB_GLOBAL "DATABASE1"

/*
 * Get a list of all used variables in transaction
 */
int getUsedVariables(varList ** var, varList * trans){
	int ret = 0;
	command tmp;	
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
			trans = trans->next;
		}
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
	varList * writer = (*var);
	command tmp;
	char * value;
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
					if( is_entry(tmp.arg1) && varListFind(tmp.arg1, (*var)) ){
						if( is_entry(tmp.arg1) && varListFind(tmp.arg1, (*var)) ){
				}
				else
					printf("ADD %s ? ? (FAIL)\n", tmp.arg1);
				
		}
	}

}
