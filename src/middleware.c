#include <time.h>
#include <string.h>
#include "../framework/cmd.h"
#include "../framework/middle-support.h"
#include "soups.h"
#include "middle_com.h"
#include "listen_thread.h"
#include "connections.h"
#include "work_thread.h"

#define NS_LOOKUP_ATTEMPTS	(10)
#define NS_SLEEP_DELAY		(1)
#define NS_VERIFICATION_DELAY	(NS_SLEEP_DELAY * 2)
#define MAX_CONNECT_ATTEMPTS	(3)

#define MW_CONNECT_SUCCESS	(1)
#define MW_CONNECT_FAIL		(0)

static char myname[TABLENAMELEN] = "";
static int sin_port = -1;
char DB_GLOBAL[ARG_SIZE];

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
	long REMOVETHIS = -10;
	ParseAddress(ns_entry_data, hostInfo);
	if(connect(csock, (struct sockaddr *)hostInfo, sizeof(struct sockaddr_in)) < 0) {
		return MW_CONNECT_FAIL;
	}
	else {
		send(csock, (void *)&REMOVETHIS, sizeof(long), 0);
		return MW_CONNECT_SUCCESS;
	}
}

int start_middleware(char *database) {
	int sock;
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

	if (myname[0] != '\0') {
		debug_out(2, "Already registered!\n");
		exit(1);
	}

	sock = setup_port(0, TCP, FALSE);
	if (sock < 0) {
		debug_out(2,"Error setting up port\n");
		exit(1);
	}

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
	pthread_t alive;

	if(argc < 2) {
		fprintf(stderr, "Insufficient parameters for %s\n", argv[0]);
		fprintf(stderr, "Usage: %s <database name>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	strncpy(DB_GLOBAL, argv[1], ARG_SIZE);

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
	return 0;
}

