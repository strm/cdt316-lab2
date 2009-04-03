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
		op = L_ASSIGN;
	else if(strncmp(type, LOG_ADD, strlen(LOG_ADD)) == 0)
		op = L_ADD;
	else if(strncmp(type, LOG_PRINT, strlen(LOG_PRINT)) == 0)
		op = L_PRINT;
	else if(strncmp(type, LOG_DELETE, strlen(LOG_DELETE)) == 0)
		op = L_DELETE;
	else if(strncmp(type, LOG_SLEEP, strlen(LOG_SLEEP)) == 0)
		op = L_SLEEP;
	else if(strncmp(type, LOG_IGNORE, strlen(LOG_IGNORE)) == 0)
		op = L_IGNORE;
	else if(strncmp(type, LOG_MAGIC, strlen(LOG_MAGIC)) == 0)
		op = L_MAGIC;
	else if(strncmp(type, LOG_QUIT, strlen(LOG_QUIT)) == 0)
		op = L_QUIT;

	return op;
}

int WriteLogEntry(FILE *logfile, int id, const varList *cmd) {
	int ret;
	varList *it;

	fprintf(logfile, "%s\n%d\n", LOG_TRANS_START, id);

	for(it = cmd, ret = 0; it != NULL; it = it->next, ret++) {
		switch(it->data.op) {
			case  L_ASSIGN:
				fprintf(logfile, "%s %s %s\n", LOG_ASSIGN, it->data.arg1, it->data.arg2);
				break;
			case L_ADD:
				fprintf(logfile, "%s %s %s %s\n", LOG_ADD, it->data.arg1, it->data.arg2, it->data.arg3);
				break;
			case L_PRINT:
				fprintf(logfile, "%s %s\n", LOG_PRINT, it->data.arg1);
				break;
			case L_DELETE:
				fprintf(logfile, "%s %s\n", LOG_DELETE, it->data.arg1);
				break;
			case L_SLEEP:
				fprint(logfile, "%s %s\n", LOG_SLEEP, it->data.arg1);
				break;
			case L_IGNORE:
				fprintf(logfile, "%s\n", LOG_IGNORE);
				break;
			case L_MAGIC:
				fprintf(logfile, "%s %s %s %s\n", LOG_MAGIC, it->data.arg1, it->data.arg2, it->data.arg3);
				break;
			case L_QUIT:
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
				case L_ASSIGN:
					data.op = L_ASSIGN;
					fscanf(logfile, "%s %s", data.arg1, data.arg2);
					break;
				case L_ADD:
					data.op = L_ADD;
					fscanf(logfile, "%s %s %s", data.arg1, data.arg2, data.arg3);
					break;
				case L_PRINT:
					data.op = L_PRINT;
					fscanf(logfile, "%s", data.arg1);
					break;
				case L_DELETE:
					data.op = L_DELETE;
					fscanf(logfile, "%s", data.arg1);
					break;
				case L_SLEEP:
					data.op = L_SLEEP;
					fscanf(logfile, "%s", data.arg1);
					break;
				case L_QUIT:
					data.op = L_QUIT;
					break;
				case L_IGNORE:
					data.op = L_IGNORE;
					break;
				case L_MAGIC:
					data.op = L_MAGIC;
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

