/*
 * Text: Header file for handling already parsed transactions.i
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: 090309
 */
#ifndef TRANS_H
#define TRANS_H

#include "log.h"

typedef struct _trans{
	struct _trans * next;
} transNode;

int addTransaction(void);
int removeTransaction(void);
int logTransaction(void);
int commitTransaction(void);

#endif
