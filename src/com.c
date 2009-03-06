/*
 * 	Functions for communctions between pthreads
 *
 */

#include "com.h"

/*
 * Gets the next element in list.
 * Returns the node or null (check for null!!!)
 * TODO Add to h file
 */  
node * getNext(int last, node * start){
	return start;
}

/*
 * Check if node is empty
 */
int isEmpty(node * start){
	if(start == NULL)
		return 1;
	else
		return 0;
}

/*
 * Finds a node by id or arg
 * TODO add to h file  
 */
node * searchNode(int id, char * arg, node * start){
	return start;	
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
message_t createMessage(char var[VAR_LEN], char value[VALUE_LEN], int cmd,int eof,){
	static message_t msg = newMsg();
	message_t ret;
	
	msg.data[(msg.sizeOfData++)].variable = strdup(var);
	msg.data[msg.sizeOfData].value = strdup(value);
	msg.data[msg.sizeOfData].cmd = cmd;
	
	ret = msg;
	//reset cases
	if ( msg.sizeOfData == 7 || eof = TRUE)
		msg = newMsg();
	return ret;
}


/*
 * Creates a new node with a uniqe id( uniqe in current list)
 */
node * createNode(message_t msg){
	node * N = (node *) malloc(sizeof(node));
	//set values in struct
	N->msg = msg;
	N->next = NULL;
	return N;
}

/*
 * Setup?
 */
int setup(node * node, pthread_mutex_t * mutex){
	pthread_mutex_init(mutex, NULL);
	node = NULL;
	
	return 0;
}

/*
 * Adds a new node to the queue.
 */
int push(node * start, node * new_node){
	new_node->next = NULL;
	node * current;
	int ret = 0;
	//first node
	if(isEmpty(start)){
		start = new_node;
		ret = SUCCESS;
	}
	//last node
	else{
		current = start;
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
node * pop(node * start){
	node * temp;
	if(isEmpty(start))
		temp = NULL;
	else if(start->next == NULL){
		temp = start;
		start = NULL;
	}
	else{
		temp = start;
		start = start->next;
	}
	return temp;
}
