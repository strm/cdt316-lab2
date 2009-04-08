#include <time.h>
#include <string.h>
#include "../framework/cmd.h"
#include "../framework/middle-support.h"
#include "soups.h"
#include "middle_com.h"
#include "listen_thread.h"
#include "connections.h"
#include "work_thread.h"
#include "logging.h"

#define NS_LOOKUP_ATTEMPTS	(10)
#define NS_SLEEP_DELAY		(1)
#define NS_VERIFICATION_DELAY	(NS_SLEEP_DELAY * 2)
#define MAX_CONNECT_ATTEMPTS	(3)

#define MW_CONNECT_SUCCESS	(1)
#define MW_CONNECT_FAIL		(0)

static char myname[TABLENAMELEN] = "";
static int sin_port = -1;
char DB_GLOBAL[ARG_SIZE];
int DB_INSTANCE;

void ParseAddress(char *ns_entry, struct sockaddr_in *hostInfo) {
	char *conn_address;
	char entry_copy[ARG_SIZE];
	int conn_port;

	strncpy(entry_copy, ns_entry, ARG_SIZE);
	conn_address = strtok(entry_copy, ":");
	conn_port = atoi(strtok(NULL, ":"));
	initSocketAddress(hostInfo, conn_address, conn_port);
}

int ConnectMiddleware(char *ns_entry_data, int csock, struct sockaddr_in *hostInfo) {
	long db_instance = htonl(-DB_INSTANCE);
	ParseAddress(ns_entry_data, hostInfo);
	if(connect(csock, (struct sockaddr *)hostInfo, sizeof(struct sockaddr_in)) < 0) {
		return MW_CONNECT_FAIL;
	}
	else {
		send(csock, (void *)&db_instance, sizeof(long), 0);
		return MW_CONNECT_SUCCESS;
	}
}

int SyncLogs() {
	int result, last_id, next_id;
	result = LogHandler(LOG_LAST_ID, 0, NULL, &last_id);
	varList *list = NULL, *it;
	message_t tmp_msg;

	globalId(ID_CHANGE, last_id);

	if(result > 0) 
		debug_out(5, "Sync: Pre-commit log ahead, last id is %d\n", last_id);
	else 
		debug_out(5, "Sync: Commit is ahead or equal, last id is %d\n", last_id);


	if(result > 0) { /* There is some syncing to be done  */
		debug_out(5, "Sync: There are transactions that have not been commited\n");
		/* last_id - result = last id in post log */
		last_id -= result;
		LogHandler(LOG_GET_NEXT_PRE_ID, last_id, NULL, &next_id);
		while(next_id != LOG_NO_ID) {
			while(list) {
				varListPop(&list);
			}
			LogHandler(LOG_READ_PRE, next_id, &list, &result);
			last_id = next_id;

			tmp_msg.sizeOfData = 0;
			tmp_msg.msgType = MW_TRANSACTION;
			tmp_msg.endOfMsg = 0;
			tmp_msg.msgId = last_id;
			tmp_msg.owner = MSG_PRE_LOG;

			it = list;
			while(it != NULL) {
				for(tmp_msg.sizeOfData = 0; tmp_msg.sizeOfData < MSG_MAX_DATA; tmp_msg.sizeOfData++) {
					if(it != NULL) {
						tmp_msg.data[tmp_msg.sizeOfData] = it->data;
						it = it->next;
					}
					else {
						tmp_msg.endOfMsg = MW_EOF;
						globalMsg(MSG_LOCK, MSG_NO_ARG);
						globalMsg(MSG_PUSH, createNode(&tmp_msg));
						globalMsg(MSG_UNLOCK, MSG_NO_ARG);
						break;
					}
				}
				if(tmp_msg.endOfMsg != MW_EOF) {
					globalMsg(MSG_LOCK, MSG_NO_ARG);
					globalMsg(MSG_PUSH, createNode(&tmp_msg));
					globalMsg(MSG_UNLOCK, MSG_NO_ARG);
					for(tmp_msg.sizeOfData = 0; tmp_msg.sizeOfData < MSG_MAX_DATA; tmp_msg.sizeOfData++) {
						strncpy(tmp_msg.data[tmp_msg.sizeOfData].arg1, "\0", 1);
						strncpy(tmp_msg.data[tmp_msg.sizeOfData].arg2, "\0", 1);
						strncpy(tmp_msg.data[tmp_msg.sizeOfData].arg3, "\0", 1);
						tmp_msg.data[tmp_msg.sizeOfData].op = NOCMD;
					}
				}
			}

			tmp_msg.msgType = MW_COMMIT;
			tmp_msg.owner = MSG_PRE_LOG;
			tmp_msg.socket = -1;
			tmp_msg.msgId = last_id;
			globalMsg(MSG_LOCK, MSG_NO_ARG);
			globalMsg(MSG_PUSH, createNode(&tmp_msg));
			globalMsg(MSG_UNLOCK, MSG_NO_ARG);
			LogHandler(LOG_GET_NEXT_PRE_ID, next_id, NULL, &next_id);
		}

		/*while(1) {
			LogHandler(LOG_GET_NEXT_PRE_ID, last_id, NULL, &next_id);
			if(next_id == LOG_NO_ID)
			break;
			debug_out(5, "prev_id: %d\t next_id: %d\n", last_id, next_id);
		// Read next precommit entry
		LogHandler(LOG_READ_PRE, next_id, &list, &result);
		printf("Read transaction %d:\n", result);

		last_id = next_id;
		}
		tmp_msg.sizeOfData = 0;
		tmp_msg.msgType = MW_TRANSACTION;
		tmp_msg.endOfMsg = 0;
		tmp_msg.msgId = result;
		tmp_msg.owner = MSG_PRE_LOG;

		for(it = list; it != NULL; it = it->next) {
		tmp_msg.data[tmp_msg.sizeOfData] = it->data;
		printf("%d %s %s \n", it->data.op, it->data.arg1, it->data.arg2);
		if(tmp_msg.sizeOfData == MSG_MAX_DATA - 1) {
		if(it->next == NULL) {
		tmp_msg.endOfMsg = MW_EOF;
		}
		globalMsg(MSG_LOCK, MSG_NO_ARG);
		globalMsg(MSG_PUSH, createNode(&tmp_msg));
		globalMsg(MSG_UNLOCK, MSG_NO_ARG);

		tmp_msg.sizeOfData = -1;
		}
		tmp_msg.sizeOfData++;
		}
		if(tmp_msg.endOfMsg != MW_EOF) {
		tmp_msg.endOfMsg = MW_EOF;
		globalMsg(MSG_LOCK, MSG_NO_ARG);
		globalMsg(MSG_PUSH, createNode(&tmp_msg));
		globalMsg(MSG_UNLOCK, MSG_NO_ARG);
		}
		tmp_msg.msgType = MW_COMMIT;
		tmp_msg.owner = -1;
		tmp_msg.socket = -1;
		tmp_msg.msgId = result;
		globalMsg(MSG_LOCK, MSG_NO_ARG);
		globalMsg(MSG_PUSH, createNode(&tmp_msg));
		globalMsg(MSG_UNLOCK, MSG_NO_ARG);*/
	}
	return 0;
}

int start_middleware(char *database) {
	int sock;
	int first_connect = 1, last_trans = -1;
	char address[ARG_SIZE];
	socklen_t reallen;
	struct sockaddr_in real;
	int mw_instance = -1;
	int ns_miss_count;
	int ns_iterator;
	char ns_entry[TABLENAMELEN];
	char ns_entry_data[ARG_SIZE];
	char entry_data_copy[ARG_SIZE];
	int conn_sock;
	int conn_retries;
	struct sockaddr_in hostInfo;
	connection conn;
	message_t tmp;

	if (myname[0] != '\0') {
		debug_out(2, "Already registered!\n");
		exit(1);
	}

	sock = setup_port(0, TCP, FALSE);
	if (sock < 0) {
		debug_out(2,"Error setting up port\n");
		exit(1);
	}

	SyncLogs();

	reallen = sizeof(struct sockaddr_in);
	if (getsockname(sock, (struct sockaddr *) &real, &reallen) < 0) {
		my_perror(2,"getsockname");
		exit(1);
	}

	if (reallen != sizeof(struct sockaddr_in)) {
		debug_out(2, "Length mismatch in getsockname()\n");
		exit(1);
	}

	sin_port = ntohs(real.sin_port);

	for(ns_miss_count = 0, ns_iterator = 0; ns_miss_count < NS_LOOKUP_ATTEMPTS; ns_iterator++) {
		sprintf(ns_entry, "%s%d", database, ns_iterator);
		if(get_entry(ns_entry_data, "nameserver", ns_entry)) {
			conn_sock = socket(AF_INET, SOCK_STREAM, 0);

			for(conn_retries = 0; conn_retries < MAX_CONNECT_ATTEMPTS; conn_retries++) {
				if(get_entry(ns_entry_data, "nameserver", ns_entry)) {
					strncpy(entry_data_copy, ns_entry_data, ARG_SIZE);
					debug_out(3, "Trying to connect to '%s'\n", ns_entry_data);
					if(ConnectMiddleware(entry_data_copy, conn_sock, &hostInfo) == MW_CONNECT_SUCCESS) {
						debug_out(5, "Connected to %s (%s, %d)\n", ns_entry, ns_entry_data, conn_sock);
						if(first_connect) {
							LogHandler(LOG_LAST_ID, 0, NULL, &last_trans);
							first_connect = 0;
							tmp.msgType = MW_SYNCHRONIZE;
							tmp.msgId = last_trans;
							tmp.owner = 0;
							tmp.socket = conn_sock;
						}
						CreateConnectionInfo(
								&conn,
								conn_sock,
								ns_entry_data,
								TYPE_MIDDLEWARE,
								0);
						ConnectionHandler(
								ADD_TO_LIST,
								&conn,
								NULL,
								NULL,
								0);
						break;
					}
					else {
						debug_out(5, "Could not connect to %s (%s)... Retrying in %d\n", ns_entry, ns_entry_data, NS_SLEEP_DELAY);
						sleep(NS_SLEEP_DELAY);
					}
				}
			}

			// Remove the entry in the nameserver if it was unreachable
			if(conn_retries == MAX_CONNECT_ATTEMPTS) {
				if(mw_instance == -1) {
					mw_instance = ns_iterator;
					sprintf(myname, "%s%d", database, mw_instance);
					sprintf(address, "%s:%d", get_host_name(), ntohs(real.sin_port));
					if(!replace_entry(address, "nameserver", myname)) {
						debug_out(5, "Failed to enter data into nameserver\n");
						exit(1);
					}
					debug_out(5, "I THINK am I %s (%d s until verification)\n", myname, NS_VERIFICATION_DELAY);
					sleep(NS_VERIFICATION_DELAY);
					if(get_entry(ns_entry_data, "nameserver", ns_entry)) {
						// Another middleware stole our nameserver entry
						if(strncmp(address, ns_entry_data, ARG_SIZE) != 0) {
							mw_instance = -1;
							strncpy(entry_data_copy, ns_entry_data, ARG_SIZE);
							if(ConnectMiddleware(entry_data_copy, conn_sock, &hostInfo) == MW_CONNECT_SUCCESS) {
								debug_out(5, "Verification failed - Connected to %s (%s)\n", ns_entry, ns_entry_data);
						CreateConnectionInfo(
								&conn,
								conn_sock,
								ns_entry_data,
								TYPE_MIDDLEWARE,
								0);
						ConnectionHandler(
								ADD_TO_LIST,
								&conn,
								NULL,
								NULL,
								0);
							}
							// Another middleware stole our entry and disappeared, ignore the slot and let
							// someone else take it later
							else {
								debug_out(5, "Verification failed - Could not connect to %s (%s)\n",ns_entry, ns_entry_data);
							}
						}
					}
				}
				else {
					debug_out(5, "Invalid nameserver entry found, waiting for delete confirmation\n");
					sleep(NS_VERIFICATION_DELAY);
					if(get_entry(ns_entry_data, "nameserver", ns_entry)) {
						strncpy(entry_data_copy, ns_entry_data, ARG_SIZE);
						if(ConnectMiddleware(entry_data_copy, conn_sock, &hostInfo) == MW_CONNECT_SUCCESS) {
							debug_out(5, "Entry rectified - Connected to %s (%s)\n", ns_entry, ns_entry_data);
						CreateConnectionInfo(
								&conn,
								conn_sock,
								ns_entry_data,
								TYPE_MIDDLEWARE,
								0);
						ConnectionHandler(
								ADD_TO_LIST,
								&conn,
								NULL,
								NULL,
								0);
						}
						else {
							if(!delete_entry("nameserver", ns_entry))
								debug_out(5, "Could not delete %s (%s) - entry does not exist\n", ns_entry, ns_entry_data);
							else
								debug_out(5, "Deleted %s (%s) from the nameserver\n", ns_entry, ns_entry_data);
						}
					}
				}
			}

			ns_miss_count = 0;
		}
		else if(mw_instance == -1) {
			mw_instance = ns_iterator;
			sprintf(myname, "%s%d", database, mw_instance);
			sprintf(address, "%s:%d", get_host_name(), ntohs(real.sin_port));
			if(!replace_entry(address, "nameserver", myname)) {
				debug_out(5, "Failed to enter data into nameserver\n");
				exit(1);
			}
			debug_out(5, "I THINK am I %s (%d s) until verification\n", myname, NS_VERIFICATION_DELAY);
			sleep(NS_VERIFICATION_DELAY);
			if(get_entry(ns_entry_data, "nameserver", ns_entry)) {
				// Another middleware stole our nameserver entry
				if(strncmp(address, ns_entry_data, ARG_SIZE) != 0) {
					mw_instance = -1;
					strncpy(entry_data_copy, ns_entry_data, ARG_SIZE);
					if(ConnectMiddleware(entry_data_copy, conn_sock, &hostInfo) == MW_CONNECT_SUCCESS) {
						debug_out(5, "Verification failed - Connected to %s (%s)\n", ns_entry, ns_entry_data);
						CreateConnectionInfo(
								&conn,
								conn_sock,
								ns_entry_data,
								TYPE_MIDDLEWARE,
								0);
						ConnectionHandler(
								ADD_TO_LIST,
								&conn,
								NULL,
								NULL,
								0);
					}
					// Another middleware stole our entry and disappeared, ignore the slot and let
					// someone else take it later
					else {
						debug_out(5, "Verification failed - Could not connect to %s (%s)\n",ns_entry, ns_entry_data);
					}
				}
			}
			// Another middleware deleted our entry, ignore the entry and let someone else take it later
			else {
				debug_out(5, "Verification failed - Entry was deleted\n");
			}
			ns_miss_count++;
		}
		else {
			ns_miss_count++;
		}
	}
	if(!first_connect)
		mw_send(conn_sock, (void *)&tmp, sizeof(tmp));
	debug_out(3, "Returning from start_middleware\n");
	return sock;
}

void stop_middleware(int sock) {

	if (myname[0] == '\0') {
		debug_out(5, "Not registered!\n");
		exit(1);
	}

	if (!delete_entry(NSDB, myname)) {
		/* TODO: Send DISCONNECT to all connected middlewares */
		debug_out(5, "Failed to delete data from nameserver\n");
		exit(1);
	}

	myname[0] = '\0';
	close(sock);
}

int main(int argc, char *argv[]) {
	//int i;
	//long msg;
	int mw_sock;
	//connection c;
	pthread_t listenThread;
	pthread_t work;
	//pthread_t alive;

	if(argc < 2) {
		fprintf(stderr, "Insufficient parameters for %s\n", argv[0]);
		fprintf(stderr, "Usage: %s <database name>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	sscanf(argv[1], "%d", &DB_INSTANCE);
	sprintf(DB_GLOBAL, "DATABASE%s", argv[1]);

	printf("DB_GLOBAL: %s\nDB_INSTANCE: %d\n", DB_GLOBAL, DB_INSTANCE);

	srand(time(NULL));
	
	set_severity(3);
	globalMsg(MSG_SETUP, MSG_NO_ARG);
	
	mw_sock = start_middleware("MIDDLEWARE");
	debug_out(2, "Middleware initialized, starting listening thread\n");
	pthread_create(&listenThread, NULL, ListeningThread, (void *)&mw_sock);
	pthread_create(&work, NULL, worker_thread, ( void * ) &mw_sock);
/*	while(1) {
		for(i = 0; i < 20; i++) {
			if(ConnectionHandler(
						GET_BY_SOCKET,
						&c,
						NULL,
						NULL,
						i) == 0) {
				msg = (rand() % 100) + 1;
				msg = htonl(msg);
				if(send(c.socket, &msg, sizeof(msg), 0) < 0) {
					perror("send");
				}
				else {
					debug_out(3, "[S] %ld to '%s' (%d)\n", msg, c.address, c.socket);
				}
				sleep(2);
			}
		}
	}	
*/
	pthread_join(work, NULL);
	pthread_join(listenThread, NULL);
	return 0;
}

