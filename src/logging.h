#ifndef _LOGGING_H_
#define _LOGGING_H_

#include "trans.h"
#include "../framework/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define LOG_WRITE_PRE		(1)
#define LOG_WRITE_POST	(2)
#define LOG_READ				(3)
#define LOG_LAST_ID			(4)
#define LOG_CHECK_ID		(5)
#define LOG_FIRST				(-100)
#define LOG_LAST				(-101)

#define LOG_TRANS_START	("<transaction>")
#define LOG_TRANS_END		("</transaction>")

#define LOG_ASSIGN			("assign")
#define LOG_ADD					("add")
#define LOG_PRINT				("print")
#define LOG_DELETE			("delete")
#define LOG_SLEEP				("sleep")
#define LOG_IGNORE			("ignore")
#define LOG_MAGIC				("magic")
#define LOG_QUIT				("quit")

#define LOG_PATH				("./logs/")
#define LOG_POST_NAME		("-commit.log")
#define LOG_PRE_NAME		("-precommit.log")

typedef enum {
	TRANS_START, TRANS_END, L_NOCMD, L_ASSIGN, L_ADD, L_PRINT, L_DELETE, L_SLEEP, L_IGNORE, L_MAGIC, L_QUIT
} log_type;

int LogHandler(char cmd, int id, varList **cmds);
int WriteLogEntry(FILE **log, int id, varList *cmd);
int ReadLogEntry(FILE *logfile, int *id, varList **cmd);

#endif

