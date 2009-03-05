/*
 * Text: Soups file.
 * Authors: Niklas Pettersson, Lars Cederholm
 * Course: MDH CDT316
 * Date: March 05 2009
 */

#ifndef SOUPS_H
#define SOUPS_H

typedef struct {
	char variable[VAR_LEN];
	int value;
} data_t;

typedef struct {
	int msgType;
	int endOfMsg;
	int msgId;
	data_t data[MSG_MAX_DATA];
} message_t;

#endif
