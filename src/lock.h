/*
 * Text: header file for a variable locking system
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: 08/03/09
 */
#ifndef LOCK_H
#define LOCK_H

#include "soups.h"
#include "trans.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef TRUE
#define TRUE 	(1)
#endif

#ifndef FALSE
#define FALSE	(0)
#endif

#define NO_ARG	(-255)

typedef struct _lockNode{
	int id;
	char var[VAR_LEN];
	struct _lockNode * next;
} lockNode;

int checkLock(char var[VAR_LEN]);
int placeLock(char var[VAR_LEN], int id);
int removeLock(char var[VAR_LEN]);
int removeAll(int id);
int noLock(void);
int lockTransaction(transNode * trans);
#endif
