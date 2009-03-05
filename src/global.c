/*
 * Text: Functions that replace global variables
 * Auhtors: Niklas Pettersson, Lars Cederholm
 * Course: MDH CDT316
 * Date: March 05 2009
 */

#include "global.h"
#include "com.h"

pthread_mutex_t _msgMutex; //mutex for globalMSG

/*
 * A global ID number used to order messages
 */
unsigned uint64_t globalId ( int cmd, int arg ) {
	static unsigned uint64_t _id = 0;
	unsigned uint64_t ret;
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
			
	}
}

node * globalMSG ( int cmd, node * arg){
	static node * _msg;
	node * ret;
	//lock mutex
	if( cmd != MSG_SETUP )	
		pthread_mutex_lock(&_msgMutex);
	switch (cmd){
		case MSG_GET:
			ret = _msg;
			break;
		case MSG_SETUP:
			setup(_msg, &_msgMutex);
			ret = MSG_NO_ARG;
			break;
		case MSG_POP:
			ret = pop(_msg);
			break;
		case MSG_PUSH:
			if (push(_msg, arg))
				ret = arg;
			else
				ret = MSG_NO_ARG;
			break;
			
	}
	//unlock
	pthread_mutex_unlock(&_msgMutex);
	return ret;
}
