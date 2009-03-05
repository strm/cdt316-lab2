#ifndef COM_H
#define COM_H

#include <pthread.h>
#include <stdlib.h>

	#define TRUE 1
	#define FALSE 0
	
	#define SUCCESS 1

	#define NO_ELEMENTS 0
typedef struct _node {
	int id; //node only id for next, search...
	int message_type;
	void * data;
	struct _node * next;
} node;


/*
 * Function list
 */
int isEmpty(node * start);
int setup(node * node, pthread_mutex_t * mutex);
int push(node * start, node * new_node);
node * pop (node * start);

#endif
