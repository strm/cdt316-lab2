/*
 * Text: Worker thread header file
 * Authors: Niklas Pettersson, Lars Cederholm
 * Data: 26 March 2009
 */
#ifndef WORK_H
#define WORK_H


#include "soups.h"
#include <time.h>
#include <string.h>
#include "../framework/cmd.h"
#include "../framework/middle-support.h"
#include "middle_com.h"
#include "listen_thread.h"
#include "connections.h"
#include "global.h"
#include "parser.h"
#include "trans.h"
#include "lock.h"

#define LOCALPARSE_SUCCESS	(1)
#define LOCALPARSE_NO_LOCK	(2)
#define LOCALPARSE_FAILED		(0)

int ParseTransaction(transNode ** trans);
void * worker_thread ( void * arg );

#endif

