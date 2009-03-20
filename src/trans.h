/*
 * Text: Header file for handling already parsed transactions.i
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: 090309
 */
#ifndef TRANS_H
#define TRANS_H

#include <stdlib.h>
#include "log.h"
#include "soups.h"
#include "../framework/cmd.h"
#include "../framework/middle-support.h"
#include "connection_list.h"

#define NO_ARG	(-255)

typedef struct _var{
	command data;
	struct _var * next;
} varList;

typedef struct _trans{
	varList * parsed;
	varList * unparsed;
	int id;
	int owner;
	connections_t conList;
	struct _trans * next;
} transNode;

int addTransaction(transNode ** list, transNode * arg);
int removeTransaction(transNode ** list, int id);
int logTransaction(transNode ** list, int id);
int commitTransaction(transNode ** list, int id);
int isTransaction(transNode * list, int id);
transNode * getTransaction(transNode * list, int id);
transNode * createTransaction(int id);

//functions for handling a varList
command varListPop(varList ** arg); //TODO used?
int varListPush(command data, varList ** arg);
int varListFind(char var[ARG_SIZE], varList * list);

char * varListGetValue(varList * list, char var[ARG_SIZE]);
int varListSetValue(varList ** List, char var[ARG_SIZE], char val[ARG_SIZE]);

#endif
