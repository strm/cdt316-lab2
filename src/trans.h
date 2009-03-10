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

#define NO_ARG	(-255)

typedef struct _var{
	data_t data;
	struct _var * next;
} varList;

typedef struct _trans{
	varList * parsed;
	varList * unparsed;
	int id;
	struct _trans * next;
} transNode;

int addTransaction(transNode ** list, transNode * arg);
int removeTransaction(transNode ** list, int id);
int logTransaction(transNode ** list, int id);
int commitTransaction(transNode ** list, int id);
transNode * createTransaction(int id);

//functions for handling a varList
data_t varListPop(varList ** arg); //TODO used?
int varListPush(data_t data, varList ** arg);



#endif
