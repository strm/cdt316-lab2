/*
 * Text: Functions that replace global variables
 * Auhtors: Niklas Pettersson, Lars Cederholm
 * Course: MDH CDT316
 * Date: March 05 2009
 */

#include "global.h"
#include <stdint.h>
pthread_mutex_t _msgMutex; //mutex for globalMSG

/*
 * A global ID number used to order messages
 */
uint64_t globalId ( int cmd, int arg ) {
	static uint64_t _id = 0;
	uint64_t ret;
	switch (cmd){
		case ID_GET:
			ret = (_id++);
			break;
		case ID_CHECK:
			if ( arg > _id )
				ret = 1;
			else
				ret = 0;
			break;
		//changes id and returns old value
		case ID_CHANGE:
			ret = _id;
			_id = arg;
			break;
		case 4:
			break;
			
	}
	return ret;
}
/*
 * Wrapper for a global message queue.
 * Arguemnts: Command to execute, Argument to that command.
 * int cmd: MSG_GET, MSG_SETUP, MSG_POP, MSG_PUSH
 * node * arg: node * node or MSG_NO_ARG
 * MSG_GET: Returns a pointer to the queue
 * MSG_SETUP: Sets up mutex and start node (must be called first and never again)
 * MSG_PUSH/POP: Normal push pop behavior.
 * MSG_CLEAN: Cleans the queue(exterminate!)
 * MSG_LOCK
 * MSG_UNLOCK
 * */
node * globalMsg ( int cmd, node * arg){
	static node * _msg;
	static int _setup = FALSE;
	node * ret;
	switch (cmd){
		case MSG_GET:
			ret = _msg;
			break;
		case MSG_SETUP:
			if(!_setup){
				setup(&_msg, &_msgMutex);
				ret = MSG_NO_ARG;
				_setup = TRUE;
			}
			break;
		case MSG_POP:
			ret = pop(&_msg);
			break;
		case MSG_PUSH:
			if (push(&_msg, arg))
				ret = arg;
			else
				ret = MSG_NO_ARG;
			break;
		case MSG_CLEAN:
			while(pop(&_msg) != NULL );
			_setup = FALSE;
			ret = MSG_NO_ARG;
			break;
		case MSG_UNLOCK:
			ret = MSG_NO_ARG;
			pthread_mutex_unlock(&_msgMutex);
			break;
		case MSG_LOCK:
			ret = MSG_NO_ARG;
			pthread_mutex_lock(&_msgMutex);
			break;
	}
	return ret;
}
