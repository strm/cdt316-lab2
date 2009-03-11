#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

#include <pthread.h>
#include <stdlib.h>
#include "soups.h"
#include "../framework/cmd.h"

#define TRUE			(1)
#define FALSE			(0)
	
#define SUCCESS			(1)

#define NO_ELEMENTS		(0)
#define NO_ARG			(-255)

//queue nood struct.
typedef struct _node {
	message_t msg;
	int id;
	struct _node * next;
} node;


/*
 * Function list
 */
int isEmpty(node ** start);
int setup(node ** node, pthread_mutex_t * mutex);
int push(node ** start, node * new_node);
node * pop (node ** start);
node * createNode(message_t * msg);
message_t * createMessage(int cmd, char arg1[ARG_SIZE], char arg2[ARG_SIZE], char arg3[ARG_SIZE], int eof);
#endif
