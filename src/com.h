#ifndef COM_H
#define COM_H

#include <pthread.h>
#include <stdlib.h>
#include "soups.h"

#define TRUE 1
#define FALSE 0
	
#define SUCCESS 1

#define NO_ELEMENTS 0

typedef struct _node {
	message_t msg;
	struct _node * next;
} node;


/*
 * Function list
 */
int isEmpty(node * start);
int setup(node * node, pthread_mutex_t * mutex);
int push(node * start, node * new_node);
node * pop (node * start);
node * createNode(message_t msg);
message_t createMessage(char var[VAR_LEN], char value[VALUE_LEN], int cmd, int eof);
#endif
