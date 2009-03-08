/*
 *	Testing application for pthread communication.
 *	
 * */
#include "global.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
/*
 * Creates messages and delivers them
 */
void * buster (void * arg){
	int n = (int) arg;
	int p = 0;
	message_t * msg;
	node * nMsg;
	for (p = 0; p < n; p++){
		msg = createMessage("hej", "hälsning", 0, 1);
		if(p == n-1)
			msg->msgType = -1;
		else
			msg->msgType = p;
		printf("%s\n", msg->data[0].variable);
		msg->msgId = p;
		nMsg = createNode(msg);
		globalMsg(MSG_PUSH, nMsg);
		usleep(1);
	}
	return (void *) 0;
}
/*
 * Recives messages
 */
void * rose (void * arg){
	node * temp;
	int _while = 0;
	do{
		temp = (node *)globalMsg(MSG_POP, NULL);
		if( temp == NULL ){
	//<	printf("*");
			_while = 1;
		}
		else {
		printf("\nRose: %d %s \n", temp->msg.msgType, temp->msg.data[temp->msg.sizeOfData].variable);
			if(temp->msg.msgType != -1)
				_while = 1;
			else
				_while = 0;
		}
	} while( _while );

	return (void *) 0;
}
/*
 * Main function:
 * sets up 2 threads and uses communction between them.
 */
int main(void){
	pthread_t thread[2];
	if(globalMsg(MSG_SETUP, MSG_NO_ARG) == NULL){
		pthread_create(&thread[0], NULL, rose, (void *) 0);
		pthread_create(&thread[1], NULL, buster, (void *) 10);
		pthread_join(thread[1], NULL);
		pthread_join(thread[0], NULL);
	}
	return 0;
	
}
