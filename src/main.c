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
	message_t msg;
	node * nMsg;
	char var[VAR_LEN];
	char val[VALUE_LEN];

	for (p = 0; p < n; p++){
		strcat(var, "hej");
		strcat(val, "hälsning");
		msg = createMessage(var, val, 0, 1);
		msg.msgType = 0;
		msg.msgId = p;
		nMsg = createNode(msg);
		globalMSG(MSG_PUSH, nMsg);
		usleep(1000);
	}
	msg = createMessage("Good bye", "Hej då", 1, 1);
	msg.msgType = -1;
	msg.msgId = (p++);
	return (void *) 0;
}
/*
 * Recives messages
 */
void * rose (void * arg){
	node * temp;
	int _while = 0;
	do{
		temp = globalMSG(MSG_POP, MSG_NO_ARG);
		if( temp == NULL ){
	//<	printf("*");
			_while = 1;
		}
		else {
			printf("\nRose: %d \n", temp->msg.msgId);
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
	//node * temp;
	if(globalMSG(MSG_SETUP, MSG_NO_ARG) == NULL){
		pthread_create(&thread[0], NULL, rose, (void *) 0);
		sleep(0);
		pthread_create(&thread[1], NULL, buster, (void *) 10);
		pthread_join(thread[1], NULL);
		pthread_join(thread[0], NULL);
	}
	return 0;
	
}
