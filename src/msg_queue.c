/*
 * 	Functions for communctions between pthreads
 *
 */
#include <stdio.h>
#include "msg_queue.h"

/*
 * Gets the next element in list.
 * Returns the node or null (check for null!!!)
 * TODO DO WE NEED THIS??`????????
 */  
node * getNext(int last, node * start){
	node * ret;
	node * current;
	if(isEmpty(&start))
		ret = NULL;
	else{
		current = start->next;
		while (current != NULL){
			if(current->id == (last+1))
				break;
			current = current->next;
		}
		ret = current;
	}
	return ret;
}

/*
 * Check if node is empty
 */
int isEmpty(node ** start){
	if((*start) == NULL)
		return TRUE;
	else
		return FALSE;
}

/*
 * TODO NEEDED?
 * Finds a node by id or ar
 * Returns the first! match
 * If id isnt used set to NO_ARG (0)
 */
node * searchNode(int id, char arg[VAR_LEN], node * start){
	int n;
	node * current;
	node * ret;
	if(isEmpty(&start))
		return NULL;
	else{
		current = start->next;
		while (current != NULL){
			for(n = 0; n < current->msg.sizeOfData; n++){
				if(id == NO_ARG){
					if(strcmp(current->msg.data[n].variable, arg) == 0){
						ret = current;
						break;
					}
				}
				else
					if(current->msg.msgId == id){
						ret = current;
						break;
					}
			}
			current = current->next;
		}
		ret = current; //works?
		return ret; 
	}
}
/*
 * Creates a new message_t struct that can then be filled
 */
message_t newMsg(void){
	message_t msg;
	msg.msgType = -1;
	msg.endOfMsg = 0;
	msg.msgId = -1;
	msg.sizeOfData = -1;

	return msg;
}

/*
 * Creates a new message_t struct and starts filling it up
 * User needs to check returned message_t for sizeOfData = 7 if eof is FALSE
 */
message_t * createMessage(char var[VAR_LEN], char value[VALUE_LEN], int cmd,int eof){
	static message_t msg;
	message_t * ret = (message_t *) malloc(sizeof(message_t));
	
	if(msg.msgType != -1) 
		msg = newMsg();
	msg.sizeOfData++;
	strncpy(msg.data[(msg.sizeOfData)].variable, var, VAR_LEN-1);
	strcpy(msg.data[msg.sizeOfData].value, value);
	msg.data[msg.sizeOfData].cmd = cmd;
	*ret = msg;
	//reset cases
	if ( msg.sizeOfData == 7 || eof == TRUE)
		msg = newMsg();
	return ret;
}


/*
 * Creates a new node with a uniqe id( uniqe in current list)
 */
node * createNode(message_t * msg){
	static int _id = 0;
	node * N = (node *) malloc(sizeof(node));
	//set values in struct
	N->msg = *msg;
	N->id = (_id++);
	N->next = NULL;
	return N;
}

/*
 * Setup?
 */
int setup(node ** node, pthread_mutex_t * mutex){
	pthread_mutex_init(mutex, NULL);
	(*node) = NULL;
	
	return 0;
}

/*
 * Adds a new node to the queue.
 */
int push(node ** start, node * new_node){
	node * current;
	int ret = 0;
	new_node->next = NULL;
	//first node
	if((*start) == NULL){
		(*start) = new_node;
		ret = SUCCESS;
	}
	//last node
	else{
		current = (*start);
		while(current->next != NULL)
			current = current->next;
		current->next = new_node;
		ret = SUCCESS;
	}
	return ret;
}

/*
 * Removes the first element in the queue.
 */
node * pop(node ** start){
	node * temp;
	if(isEmpty(start))
		temp = NULL;
	else if((*start)->next == NULL){
		temp = (*start);
		(*start) = NULL;
	}
	else{
		temp = (*start);
		(*start) = (*start)->next;
	}
	return temp;
}
