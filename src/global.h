/*
 * Text: Header for global.c (functions instead of global variables)
 * Authors: Niklas Pettersson, Lars Cederholm
 * Course: MDH CDT316
 * Date: March 05 2009
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include "msg_queue.h"

#define ID_GET 		(1)
#define ID_CHECK 	(2)
#define ID_CHANGE 	(3)
#define ID_TEST 	(4)

#define MSG_GET		(1)
#define MSG_SETUP	(2)
#define MSG_POP		(3)
#define MSG_PUSH	(4)
#define MSG_CLEAN	(5)
#define MSG_LOCK	(6)
#define MSG_UNLOCK	(-6)
#define MSG_NO_ARG	(NULL)

//functions 
node * globalMsg ( int cmd, node * arg );
uint64_t globalId ( int cmd, int arg );
#endif
