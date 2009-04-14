/*
 * Text: Transaction handling system
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: Today
 */

#include "trans.h"
#include "lock.h"
#include <stdio.h>

transNode * popTransaction(transNode ** list, int id, int socket){
	transNode * current;
	transNode * prev;

	if((*list) == NULL){
		debug_out(5, "list empty\n");
		return FALSE;
	}
	else{
		for(current = (*list), prev = (*list); current != NULL; prev = current, current = current->next){
			if(current->id == id && current->socket == socket){
				//match
			debug_out(5, "transList %d\n", current->id);
				if(current == (*list))
					(*list) = current->next;
				else
					prev->next = current->next;
				if((*list) == NULL)
					debug_out(5, "list should be empty\n");
				return current;
			}
		}
	}
	return NULL;
}

int isTransaction(transNode * list, int id){
	if(id < 0 || list == NULL)
		return FALSE;
	debug_out(3, "trying to match transaction\n");
	while( list != NULL ){
		debug_out(3, "not null\n");
		if(list->id == id){
			debug_out(3, "isTransaction match found %d = %d", list->id, id );
			return TRUE;
		}
		else
			list = list->next;
	}
	debug_out(5, "not a transaction %d\n", id);
	return FALSE;
}
/*
 * Returns a transaction pointer
 */
transNode * getTransaction(transNode * list, int id){
	while ( list != NULL ) {
		if(list->id == id)
			return list;
		else
			list = list->next;
	}
	return NULL;

}
int removeTransaction(transNode ** list, int id){
	transNode * current;
	transNode * prev;

	if((*list) == NULL){
		debug_out(5, "list empty\n");
		return FALSE;
	}
	else{
		for(current = (*list), prev = (*list); current != NULL; prev = current, current = current->next){
			if(current->id == id){
				//match
			debug_out(5, "transList %d\n", current->id);
				if(current == (*list))
					(*list) = current->next;
				else
					prev->next = current->next;
				free(current);
				if((*list) == NULL)
					debug_out(5, "list should be empty\n");
				return TRUE;
			}
		}
	}
	return FALSE;
}


/*
 * Add a transaction to a transaction list
 */
int addTransaction(transNode ** list, transNode * arg){
	transNode * tmp;
	if((*list) == NULL){
		(*list) = arg;
		return TRUE;
	}
	else{
		tmp = (*list);
		while(tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = arg;
		arg->next = NULL;
		return TRUE;
	}
	return FALSE;
}

/*
 * Creates a new transaction
 */
transNode * createTransaction(int id){
	transNode * tmp = (transNode *) malloc(sizeof(transNode));
	tmp->id = id;
	tmp->next = NULL;
	tmp->parsed = NULL;
	tmp->unparsed = NULL;
	tmp->conList = NULL;
	return tmp;
}

/*
 * Pops the first element of argument varList
 * TODO will this be used?
 */
command varListPop(varList ** arg){
	varList * tmp;
	command ret;
	ret.op = NO_ARG;
	if((*arg) == NULL)
		ret.op = MAGIC;
	else if((*arg)->next == NULL){
		tmp = (*arg);
		(*arg) = NULL;
		ret = tmp->data;
		free(tmp);
	}
	else{
		tmp = (*arg);
		(*arg) = (*arg)->next;
		ret = tmp->data;
		free(tmp);
	}
	return ret;
}
/*
 * Adds a new item to the list.
 * Placed last for reasons.
 */
int varListPush(command data, varList ** arg){
	varList * tmp = (varList *) malloc(sizeof(varList));
	varList * cur;
	tmp->data = data;
	tmp->next = NULL;
	printf("push(%d %s)\n", data.op, data.arg1);
	if(*arg == NULL){
		printf("First\n");
		(*arg) = tmp;
		return 1;
	}
	else{
		printf("not first\n");
		cur = (*arg);
		while( (cur)->next != NULL ){
			cur = cur->next;
		}
		cur->next = tmp;
		return 1;
	}
	return 0;
}

/*
 * Returns TRUE if variable exists in list or FALSE if not
 */
int varListFind(char var[ARG_SIZE], varList * list){
	while ( list != NULL){
		if(strcmp(list->data.arg1, var) == 0)
			return TRUE;
		else
			list = list->next;
		if(list != NULL)
			printf("%s\n", list->data.arg1);
	}
	return FALSE;
}
//get a value from the list
char * varListGetValue(varList * list, char var[ARG_SIZE]){
	while( list != NULL){
		if(strcmp(list->data.arg1, var) == 0)
			return list->data.arg2;
		else
			list = list->next;
	}
	return NULL;
}
/* 
 * Changes a value
 */
int varListSetValue(varList ** List, char var[ARG_SIZE], char val[ARG_SIZE]){
	varList * list = (*List);
	while( list != NULL ){
		if(!strcmp(list->data.arg1, var)){
			strcpy(list->data.arg2, val);
			return TRUE;
		}
		else
			list = list->next;
	}
	return FALSE;
}
