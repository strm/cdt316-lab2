/*
 * Text: Soups file.
 * Authors: Niklas Pettersson, Lars Cederholm
 * Course: MDH CDT316
 * Date: March 05 2009
 */

#ifndef SOUPS_H
#define SOUPS_H

#include <string.h>
#include "trans.h"
#include "msg_queue.h"
#include "../framework/cmd.h"
#include "../framework/middle-support.h"

#define VAR_LEN		(255)
#define VALUE_LEN	(768)
#define MSG_MAX_DATA	(7)
#define MSG_ME		(-42)
#define MSG_ALIVE	(-255)

// Definitions for middleware communication
#define MW_DISCONNECT			(0)
#define MW_CONNECT				(1)
#define MW_TRANSACTION		(2)
#define MW_SYNCHRONIZE		(3)
#define MW_ACK						(4)
#define MW_NAK						(5)
#define MW_COMMIT					(6)
#define MW_EOF						(7)
#define MW_ALIVE					(-888)

typedef struct {
	int msgType;
	int endOfMsg;
	int msgId;
	int sizeOfData;
	int owner;
	int nMiddlewares;
	int socket;
	command data[MSG_MAX_DATA];
} message_t;

#endif
