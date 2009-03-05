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
