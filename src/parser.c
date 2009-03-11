/*
 * Text: Functions for parsing a transaction locally.
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: Today
 */
#include "parser.h"

/*
 * Get a list of all used variables in transaction
 */
int getUsedVariables(varList ** var, varList * trans){
	int ret = 0;
	command tmp;	
	while ( trans != NULL ){
		//check all args
		if(is_entry(trans->data.arg1))
			if(!varListFind(trans->data.arg1, (*var))){
				ret++;
				tmp.op = 1;
				strcpy(tmp.arg1, trans->data.arg1);
				varListPush(tmp, var);
			}
		if(is_entry(trans->data.arg2))
			if(!varListFind(trans->data.arg2, (*var)))
				ret++;
		if(is_entry(trans->data.arg3))
			if(!varListFind(trans->data.arg3, (*var)))
				ret++;
		trans = trans->next;
	}
	
	return ret;		
}

