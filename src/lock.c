/*
 * Text: Functions for a variable locking system
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: 08/03/09
 */
#include "lock.h"
lockNode * lockList = NULL;
/*
 * Check if the list is empty
 */
int noLock(void){
	if(lockList == NULL)
		return TRUE;
	else
		return FALSE;
}
/*
 * Pushes a new node to the list.
 */
int lockPush(char var[VAR_LEN], int id){
	lockNode * nNode = (lockNode *) malloc(sizeof(lockNode));
	lockNode * current = lockList;
	//setup the new node
	nNode->id = id;
	strcpy(nNode->var, var);
	nNode->next = NULL;
	
	if(noLock()){
		lockList = nNode;
		return TRUE;		
	}
	else{
		//find last node
		while (current->next != NULL)
			current = current->next;
		current->next = nNode;
		return TRUE;
	}
	return FALSE;
}
/*
 * Pops an existing node from the list.
 * Since only one with the same var should exist we remove the first one found that.
 * Also if id is specified we remove the first matched id
 */
int lockPop(char var[VAR_LEN], int id){
	lockNode * temp, * prev;
	if(noLock())
		return FALSE;
	else{
		temp = lockList;
		prev = lockList;
		do{
			//prev = temp;
			if(temp->id == id || strcmp(temp->var, var) == 0){
				//match found remove node from list
				prev->next = temp->next;
				free(temp);
				return TRUE;
			}
			else{
				prev = temp;
				temp = temp->next;
			}
		} while(temp != NULL);
	}
	return FALSE;
}
/*
 * Checks if a node with the argument exists.
 */
int lockFind(char var[VAR_LEN]){
	lockNode * temp = lockList;
	if(noLock());
	else{
		do{
			if(strcmp(temp->var, var) == 0)
				return TRUE;
			else
				temp = temp->next;

		} while ( temp != NULL ); 
	}
	return FALSE;
}
/*
 * Pops all nodes with specified id
 */
int lockBatchPop(int id){
	int ret = FALSE;
	while( lockPop("\n", id) )
		ret = TRUE;
	return ret;
}
/*
 * Returns TRUE if locked FALSE if not.
 */
int checkLock(char var[VAR_LEN]){
	return lockFind(var);
}
/*
 * Places a new lock. After checkLock have been used.
 * TRUE if lock is sucessful FALSE if already locked
 */
int placeLock(char var[VAR_LEN], int id){
	if(!checkLock(var))
		return lockPush(var, id);
	else
		return FALSE;
}
/*
 * Unlocked the specified variable.
 * Returns FALSE if not locked.
 */
int removeLock(char var[VAR_LEN]){
	return lockPop(var, NO_ARG);	
}
/*
 * Remove all locks associated with the transaction id supplied.
 * Returns FALSE if no locks was found.
 */
int removeAll(int id){
	return lockBatchPop(id);
}

