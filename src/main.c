/*
 *	Testing application for pthread communication.
 *	
 * */

#include "com.h"
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
	data_t data;
	message_t msg;
	msg 
	for (p = 0; p < n; p++){
		push(p*p, NULL);
		usleep(1);
	}
	push(-1, NULL);
	return (void *) 0;
}
/*
 * Recives messages
 */
void * rose (void * arg){
	node * temp;
	int _while = 0;
	do{
		temp = pop();
		if( temp == NULL ){
	//<	printf("*");
			_while = 1;
		}
		else {
			printf("\nRose: %d \n", temp->message_type);
			if(temp->message_type != -1)
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
	if(globalMsg(MSG_SETUP, MSG_NO_ARG) == NULL){
		pthread_create(&thread[0], NULL, rose, (void *) 0);
		sleep(0);
		pthread_create(&thread[1], NULL, buster, (void *) 10);
		pthread_join(thread[1], NULL);
		pthread_join(thread[0], NULL);
	}
	return 0;
	
}
