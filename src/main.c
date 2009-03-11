/*
 *	Testing application for pthread communication.
 *	
 * */
#include "global.h"
#include "lock.h"
#include "trans.h"
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
		msg = createMessage(0,"hej", "hälsning", "hej", 1);
		if(p == n-1)
			msg->msgType = -1;
		else
			msg->msgType = p;
		printf("%s\n", msg->data[0].arg1);
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
		printf("\nRose: %d %s \n", temp->msg.msgType, temp->msg.data[temp->msg.sizeOfData].arg1);
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
	varList * vList = NULL;
	command dt;
	command tmp;
	dt.op = 0;
	transNode * tList = NULL;
	pthread_t thread[2];
	if(globalMsg(MSG_SETUP, MSG_NO_ARG) == NULL){
		pthread_create(&thread[0], NULL, rose, (void *) 0);
		pthread_create(&thread[1], NULL, buster, (void *) 10);
		pthread_join(thread[1], NULL);
		pthread_join(thread[0], NULL);
	}

	/**
	 * Testing of lock.c
	 */
	placeLock("hej", 2);
	placeLock("morgon", 1);
	placeLock("girth", 1);
	printf("lock hej? %d\n", placeLock("hej", 1));
	printf("is locked ? %d\n", checkLock("hej"));
	removeLock("girth");
		removeLock("hej");
	printf("%d\n",removeLock("hej"));
	printf("is locked ? %d\n", checkLock("hej"));
	printf("remove all %d\n", removeAll(1));
	removeLock("morgon");
	removeLock("girth");
	
	printf("hej: %d\n", checkLock("hej"));
	printf("morgon: %d\n", checkLock("morgon"));
	printf("girth: %d\n", checkLock("girth"));
	printf("remove all %d\n", removeAll(1));
	printf("no locks? %d\n",noLock());

 	/**
	 * Testing of trans
	 */
	dt.op++;
	printf("varListPust = %d\n", varListPush(dt, &vList));
	dt.op++;
	printf("varListPust = %d\n", varListPush(dt, &vList));
	dt.op++;
	printf("varListPust = %d\n", varListPush(dt, &vList));
	dt.op++;
	printf("varListPust = %d\n", varListPush(dt, &vList));
	printf("varList = {");
	do{
		tmp = varListPop(&vList);
		printf("%d,", tmp.op);
	} while( tmp.op != NO_ARG );
	printf("}\n");

	/*
 	 * testing trans
 	 */ 
	printf("trans: %d\n",addTransaction(&tList, createTransaction(1)));
	printf("trans: %d\n",addTransaction(&tList, createTransaction(2)));
	printf("trans: %d\n",addTransaction(&tList, createTransaction(3)));
	
	printf("rm trans: %d\n", removeTransaction(&tList, 3));
	printf("rm trans: %d\n", removeTransaction(&tList, 1));
	printf("rm trans: %d\n", removeTransaction(&tList, 2));
	printf("rm trans: %d\n", removeTransaction(&tList, 1));
	
	printf("rm trans: %d\n", removeTransaction(&tList, 5));
	
	return 0;
	
}
