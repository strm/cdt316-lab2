/*
 * 	Functions for communctions between pthreads
 *
 */
#include <stdio.h>
#include "msg_queue.h"

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
 * Creates a new message_t struct that can then be filled
 */
message_t newMsg(void){
	message_t msg;
	msg.msgType = -1;
	msg.endOfMsg = 0;
	msg.msgId = -99;
	msg.owner = -1;
	msg.nMiddlewares = -1;
	msg.sizeOfData = -1;

	return msg;
}

/*
 * Creates a new message_t struct and starts filling it up
 * User needs to check returned message_t for sizeOfData = 7 if eof is FALSE
 */
message_t * createMessage(int cmd, char arg1[ARG_SIZE], char arg2[ARG_SIZE], char arg3[ARG_SIZE], int eof){
	static message_t msg;
	message_t * ret = (message_t *) malloc(sizeof(message_t));
	
	if(msg.msgType != -1) 
		msg = newMsg();
	msg.sizeOfData++;
	strcpy(msg.data[(msg.sizeOfData)].arg1, arg1);
	strcpy(msg.data[(msg.sizeOfData)].arg2, arg2);
	strcpy(msg.data[(msg.sizeOfData)].arg3, arg3);
	msg.data[msg.sizeOfData].op = cmd;
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
