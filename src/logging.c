#include "logging.h"

int LogHandler(char cmd, int id_in, varList **commands, int *id_out) {
	int ret = 0;
	FILE *post_file = NULL;
	FILE *pre_file = NULL;
	char *post_log = NULL;
	char *pre_log = NULL;
	int post_log_len = strlen(LOG_PATH) + strlen(LOG_POST_NAME) + strlen(DB_GLOBAL) + 1;
	int pre_log_len = strlen(LOG_PATH) + strlen(LOG_PRE_NAME) + strlen(DB_GLOBAL) + 1;

	post_log = (char *)malloc(sizeof(char *) * post_log_len);
	pre_log = (char *)malloc(sizeof(char *) * pre_log_len);

	strncpy(post_log, LOG_PATH, post_log_len);
	strncat(post_log, DB_GLOBAL, post_log_len);
	strncat(post_log, LOG_POST_NAME, post_log_len);

	strncpy(pre_log, LOG_PATH, pre_log_len);
	strncat(pre_log, DB_GLOBAL, pre_log_len);
	strncat(pre_log, LOG_PRE_NAME, pre_log_len);

	switch (cmd){
		case LOG_WRITE_PRE:
			debug_out(4, "log: In LOG_WRITE_PRE\n");
			pre_file = fopen(pre_log, "a");
			if(pre_file != NULL)
				debug_out(4, "log: Opened file '%s'\n", pre_log);
			else
			{
				debug_out(4, "log: Could not open file '%s'\n", pre_log);
				ret = -1;
				break;
			}
			WriteLogEntry(pre_file, id_in, *commands);
			debug_out(4, "log: logentry written to file\n");
			break;
		case LOG_WRITE_POST:
			debug_out(4, "log: In LOG_WRITE_POST\n");
			post_file = fopen(post_log, "a");
			if(post_file != NULL)
				debug_out(4, "log: Opened file '%s'\n", post_log);
			else {
				debug_out(4, "log: Could not open file '%s'\n", post_log);
				ret = -1;
				break;
			}
			WriteLogEntry(post_file, id_in, *commands);
			debug_out(4, "log: logentry written to file\n");
			break;
		case LOG_READ_POST:
			post_file = fopen(post_log, "r");
			if(post_file != NULL) {
				if(ReadLogEntry(&post_file, &id_in, commands) == -1) {
					*id_out = LOG_NO_ID;
					ret = -1;
				}
			}
			break;
		case LOG_READ_PRE:
			pre_file = fopen(pre_log, "r");
			if(pre_file != NULL) {
				if(ReadLogEntry(&pre_file, &id_in, commands) == -1) {
					*id_out = LOG_NO_ID;
					ret = -1;
				}
			}
			break;
		case LOG_LAST_ID:
			if(id_out != NULL) {
				ret = GetLastId(pre_log, post_log, id_out);
			}
			else
				ret = -1;
			break;
		case LOG_GET_NEXT_PRE_ID:
			if(id_out != NULL)
				*id_out = GetNextId(pre_log, id_in);
			else
				ret = -1;
			break;
		case LOG_GET_NEXT_POST_ID:
			if(id_out != NULL)
				*id_out = GetNextId(post_log, id_in);
			else
				ret = -1;
			break;
		case LOG_CHECK_ID:
			break;
		default:
			ret = -1;
			break;
	}
	if(pre_file) {
		fclose(pre_file);
	}
	if(post_file) {
		fclose(post_file);
	}

	return ret;
}

int JumpToNextTrans(FILE **logfile) {
	int ret = 0;
	char line[LINE_LEN];

	while(1) {
		if(fscanf(*logfile, "%s", line) == EOF) {
			ret = -1;
			break;
		}
		else if(strncmp(line, LOG_TRANS_START, LINE_LEN) == 0) {
			ret = 0;
			break;
		}
	}
	return ret;
}

int JumpToTrans(FILE **logfile, int id) {
	int ret = LOG_NO_ID;
	char line[LINE_LEN];

	if(fscanf(*logfile, "%s", line) != EOF) {
		if(strncmp(line, LOG_TRANS_START, strlen(LOG_TRANS_START)) == 0) {
			fscanf(*logfile, "%d", &ret);
			if(ret == id) {
				return ret;
			}
			else
				ret = -1;
		}
	}

	while(1) {
		if(JumpToNextTrans(logfile) == 0) {
			fscanf(*logfile, "%d", &ret);
			if(ret == id)
				break;
			else
				ret = -1;
		}
	}
	return ret;
}

int GetNextId(char *logfile, int current_id) {
	int ret = LOG_NO_ID;
	int tmp;
	FILE *log = fopen(logfile, "r");
	
	if(!log)
		return ret;

	while(1) {
		if(JumpToNextTrans(&log) == -1) {
			break;
		}
		if(fscanf(log, "%d", &tmp) == EOF) {
			break;
		}
		else if(tmp > current_id) {
			// We have the next id, no reason to continue
			break;
		}
	}

	printf("GetNextId: tmp: %d, current_id %d\n", tmp, current_id);

	if(tmp > current_id)
		ret = tmp;
	else
		ret = LOG_NO_ID;

	if(log)
		fclose(log);
	return ret;
}

int GetLastId(char *pre_log, char *post_log, int *result) {
	int post_id = -1, pre_id = -1;
	FILE *pre_file = fopen(pre_log, "r");
	FILE *post_file = fopen(post_log, "r");
	char line[LINE_LEN];

	if(pre_file != NULL) {
		while(fscanf(pre_file, "%s", line) != EOF) {
			if(strncmp(line, LOG_TRANS_START, strlen(LOG_TRANS_START) + 1) == 0) {
				fscanf(pre_file, "%d", &pre_id);
			}
		}
		fclose(pre_file);
	}
	if(post_file != NULL) {
		while(fscanf(post_file, "%s", line) != EOF) {
			if(strncmp(line, LOG_TRANS_START, strlen(LOG_TRANS_START) + 1) == 0) {
				fscanf(post_file, "%d", &post_id);
			}
		}
		fclose(post_file);
	}

	if(pre_id > post_id) {
		*result = pre_id;
	}
	else {
		*result = post_id;
	}
	return pre_id - post_id;
}

log_type LogEntryType(char *type) {
	log_type op = L_NOCMD;

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

int WriteLogEntry(FILE *logfile, int id, varList *cmd) {
	int ret;
	varList *it;
	//FILE *logfile = *log;

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
				fprintf(logfile, "%s %s\n", LOG_SLEEP, it->data.arg1);
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
			default:
				break;
		}
	}

	fprintf(logfile, "%s\n", LOG_TRANS_END);
	return 0;
}

int ReadLogEntry(FILE **logfile, int *id, varList **cmd) {
	int ret = 0;
	char tmp[ARG_SIZE];
	command data;

	if(JumpToTrans(logfile, *id) == -1) {
		return -1;
	}

	data.op = L_NOCMD;
	while((fscanf(*logfile, "%s", tmp) != -1)) {
		switch(LogEntryType(tmp)) {
			case L_ASSIGN:
				data.op = ASSIGN;
				fscanf(*logfile, "%s %s", data.arg1, data.arg2);
				break;
			case L_ADD:
				data.op = ADD;
				fscanf(*logfile, "%s %s %s", data.arg1, data.arg2, data.arg3);
				break;
			case L_PRINT:
				data.op = PRINT;
				fscanf(*logfile, "%s", data.arg1);
				break;
			case L_DELETE:
				data.op = DELETE;
				fscanf(*logfile, "%s", data.arg1);
				break;
			case L_SLEEP:
				data.op = SLEEP;
				fscanf(*logfile, "%s", data.arg1);
				break;
			case L_QUIT:
				data.op = QUIT;
				break;
			case L_IGNORE:
				data.op = IGNORE;
				break;
			case L_MAGIC:
				data.op = MAGIC;
				fscanf(*logfile, "%s %s %s", data.arg1, data.arg2, data.arg3);
				break;
			case TRANS_END:
				data.op = NOCMD;
				break;
			default:
				break;
		}
		if(data.op != NOCMD) {
			ret++;
		}
		else {
			break;
		}
		varListPush(data, cmd);
	}

	return ret;
}

