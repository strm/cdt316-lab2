/*
 * Text: Soups file.
 * Authors: Niklas Pettersson, Lars Cederholm
 * Course: MDH CDT316
 * Date: March 05 2009
 */

#ifndef SOUPS_H
#define SOUPS_H

#include <string.h>

#define VAR_LEN		(255)
#define VALUE_LEN	(768)
#define MSG_MAX_DATA	(8)

typedef struct {
	int cmd;
	char variable[VAR_LEN];
	char value[VALUE_LEN];
} data_t;

typedef struct {
	int msgType;
	int endOfMsg;
	int msgId;
	int sizeOfData;
	data_t data[MSG_MAX_DATA];
} message_t;

#endif
