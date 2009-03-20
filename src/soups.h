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
#include "../framework/cmd.h"
#include "../framework/middle-support.h"

#define VAR_LEN		(255)
#define VALUE_LEN	(768)
#define MSG_MAX_DATA	(7)
#define MSG_ME		(-42)

typedef struct {
	int msgType;
	int endOfMsg;
	int msgId;
	int sizeOfData;
	int owner;
	int nMiddlewares;
	command data[MSG_MAX_DATA];
} message_t;

#endif
