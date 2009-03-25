#include "logging.h"

int LogHandler(char cmd, int id, varList **commands) {
	int ret = 0;
	FILE *logfile = NULL;
	char *filename = NULL;
	int filename_len;

	switch cmd {
		case LOG_WRITE_PRE:
			break;
		case LOG_WRITE_POST:
			break;
		case LOG_READ:
			break;
		case LOG_READ_NEXT:
			break;
		case LOG_LAST_ID:
			break;
		case LOG_CHECK_ID:
			break;
		default:
			ret = -1;
			break;
	}

	return ret;
}

log_type LogEntryType(char *type) {
	log_type op = NOCMD;

	if(strncmp(type, LOG_TRANS_START, strlen(LOG_TRANS_START)) == 0)
		op = TRANS_START;
	else if(strncmp(type, LOG_TRANS_END, strlen(LOG_TRANS_END)) == 0)
		op = TRANS_END;
	else if(strncmp(type, LOG_ASSIGN, strlen(LOG_ASSIGN)) == 0)
		op = ASSIGN;
	else if(strncmp(type, LOG_ADD, strlen(LOG_ADD)) == 0)
		op = ADD;
	else if(strncmp(type, LOG_PRINT, strlen(LOG_PRINT)) == 0)
		op = PRINT;
	else if(strncmp(type, LOG_DELETE, strlen(LOG_DELETE)) == 0)
		op = DELETE;
	else if(strncmp(type, LOG_SLEEP, strlen(LOG_SLEEP)) == 0)
		op = SLEEP;
	else if(strncmp(type, LOG_IGNORE, strlen(LOG_IGNORE)) == 0)
		op = IGNORE;
	else if(strncmp(type, LOG_MAGIC, strlen(LOG_MAGIC)) == 0)
		op = MAGIC;
	else if(strncmp(type, LOG_QUIT, strlen(LOG_QUIT)) == 0)
		op = QUIT;

	return op;
}

int WriteLogEntry(FILE *logfile, int id, const varList *cmd) {
	int ret;
	varList *it;

	fprintf(logfile, "%s\n%d\n", LOG_TRANS_START, id);

	for(it = cmd, ret = 0; it != NULL; it = it->next, ret++) {
		switch(it->data.op) {
			case  ASSIGN:
				fprintf(logfile, "%s %s %s\n", LOG_ASSIGN, it->data.arg1, it->data.arg2);
				break;
			case ADD:
				fprintf(logfile, "%s %s %s %s\n", LOG_ADD, it->data.arg1, it->data.arg2, it->data.arg3);
				break;
			case PRINT:
				fprintf(logfile, "%s %s\n", LOG_PRINT, it->data.arg1);
				break;
			case DELETE:
				fprintf(logfile, "%s %s\n", LOG_DELETE, it->data.arg1);
				break;
			case SLEEP:
				fprint(logfile, "%s %s\n", LOG_SLEEP, it->data.arg1);
				break;
			case IGNORE:
				fprintf(logfile, "%s\n", LOG_IGNORE);
				break;
			case MAGIC:
				fprintf(logfile, "%s %s %s %s\n", LOG_MAGIC, it->data.arg1, it->data.arg2, it->data.arg3);
				break;
			case QUIT:
				fprintf(logfile, "%s\n", LOG_QUIT);
				break;
		}
	}

	fprintf(logfile, "%s\n", LOG_TRANS_END);
}

int ReadLogEntry(FILE *logfile, int *id, varList **cmd) {
	int ret = 0, transaction = 0;
	char tmp[ARG_SIZE];
	command data;

	while(fscanf(logfile, "%s", tmp) != -1) {
		if(LogEntryType(tmp) == TRANS_START) {
			transaction = 1;
			break;
		}
	}

	if(transaction) {
		data.op = NOCMD;
		while(fscanf(logfile, "%s", tmp) != -1 && transaction) {
			switch(LogEntryType(tmp)) {
				case ASSIGN:
					data.op = ASSIGN;
					fscanf(logfile, "%s %s", data.arg1, data.arg2);
					break;
				case ADD:
					data.op = ADD;
					fscanf(logfile, "%s %s %s", data.arg1, data.arg2, data.arg3);
					break;
				case PRINT:
					data.op = PRINT;
					fscanf(logfile, "%s", data.arg1);
					break;
				case DELETE:
					data.op = DELETE;
					fscanf(logfile, "%s", data.arg1);
					break;
				case SLEEP:
					data.op = SLEEP;
					fscanf(logfile, "%s", data.arg1);
					break;
				case QUIT:
					data.op = QUIT;
					break;
				case IGNORE:
					data.op = IGNORE;
					break;
				case MAGIC:
					data.op = MAGIC;
					fscanf(logfile, "%s %s %s", data.arg1, data.arg2, data.arg3);
					break;
				case TRANS_END:
					data.op = NOCMD;
					transaction = 0;
					break;
				default:
					break;
			}
			if(transaction && data.op != NOCMD)
				ret++;
				varListPush(data, cmd);
		}
	}

	return ret;
}

