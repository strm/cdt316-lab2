#include <time.h>
#include <string.h>
#include "../framework/cmd.h"
#include "../framework/middle-support.h"
#include "middle_com.h"
#include "listen_thread.h"
#include "connection_list.h"

#define NS_LOOKUP_ATTEMPTS	(10)
#define NS_SLEEP_DELAY		(2)
#define NS_VERIFICATION_DELAY	(NS_SLEEP_DELAY + 1)
#define MAX_CONNECT_ATTEMPTS	(3)

#define MW_CONNECT_SUCCESS	(1)
#define MW_CONNECT_FAIL		(0)

static char myname[TABLENAMELEN] = "";
static connections_t connList;
char DB_GLOBAL[ARG_SIZE];

void ParseAddress(char *ns_entry, struct sockaddr_in *hostInfo) {
	char *conn_address;
	int conn_port;

	conn_address = strtok(ns_entry, ":");
	conn_port = atoi(strtok(NULL, ":"));
	initSocketAddress(hostInfo, conn_address, conn_port);
}

int ConnectMiddleware(char *ns_entry_data, int csock, struct sockaddr_in *hostInfo) {
	ParseAddress(ns_entry_data, hostInfo);
	if(connect(csock, (struct sockaddr *)&hostInfo, sizeof(struct sockaddr_in)) < 0) {
		return MW_CONNECT_FAIL;
	}
	else {
		AddConnection(&connList, csock);
		return MW_CONNECT_SUCCESS;
	}
}

int start_middleware(char *database) {
	int sock;
	char address[ARG_SIZE];
	int reallen;
	struct sockaddr_in real;
//	char db[TABLENAMELEN];
//	char *p;
	
	int mw_instance = -1;
	int ns_miss_count;
	int ns_iterator;
	char ns_entry[TABLENAMELEN];
	char ns_entry_data[ARG_SIZE];
//	char *conn_address;
//	int conn_port;
	int conn_sock;
	int conn_retries;
	struct sockaddr_in hostInfo;

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
	
	for(ns_miss_count = 0, ns_iterator = 0; ns_miss_count < NS_LOOKUP_ATTEMPTS; ns_iterator++) {
		sprintf(ns_entry, "%s%d", database, ns_iterator);
		if(get_entry(ns_entry_data, "nameserver", ns_entry)) {
			conn_sock = socket(PF_INET, SOCK_STREAM, 0);
			
			for(conn_retries = 0; conn_retries < MAX_CONNECT_ATTEMPTS; conn_retries++) {
				if(get_entry(ns_entry_data, "nameserver", ns_entry)) {
					if(ConnectMiddleware(ns_entry_data, conn_sock, &hostInfo) == MW_CONNECT_SUCCESS) {
						printf("Connected to %s (%s)\n", ns_entry, ns_entry_data);
						AddConnection(&connList, conn_sock);
						break;
					}
					else {
						printf("Could not connect to %s (%s)... Retrying in %d\n", ns_entry, ns_entry_data, NS_SLEEP_DELAY);
						sleep(NS_SLEEP_DELAY);
					}

					/*ParseAddress(ns_entry_data, &hostInfo);
					if(connect(conn_sock, (struct sockaddr *)&hostInfo, sizeof(hostInfo)) < 0) {
						perror("connect");
						printf("Could not connect to %s... Retrying in 2 seconds\n", ns_entry);
						sleep(2);
					}
					else {
						printf("Connected to %s (%s)\n", ns_entry, ns_entry_data);
						AddConnection(&connList, conn_sock);
						break;
					}*/
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
					debug_out(5, "I THINK am %s (%d s) until verification\n", myname, NS_VERIFICATION_DELAY);
					sleep(NS_VERIFICATION_DELAY);
					if(get_entry(ns_entry_data, "nameserver", ns_entry)) {
						// Another middleware stole our nameserver entry
						if(strncmp(address, ns_entry_data, ARG_SIZE) != 0) {
							mw_instance = -1;
							if(ConnectMiddleware(ns_entry_data, conn_sock, &hostInfo) == MW_CONNECT_SUCCESS) {
								debug_out(5, "Verification failed - Connected to %s (%s)\n", ns_entry, ns_entry_data);
								AddConnection(&connList, conn_sock);
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
					if(!delete_entry("nameserver", ns_entry))
						debug_out(5, "Could not delete %s (%s) - entry does not exist\n", ns_entry, ns_entry_data);
					else
						debug_out(5, "Deleted %s (%s) from the nameserver\n", ns_entry, ns_entry_data);
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
			debug_out(5, "I THINK am %s (%d s) until verification\n", myname, NS_VERIFICATION_DELAY);
			sleep(NS_VERIFICATION_DELAY);
			if(get_entry(ns_entry_data, "nameserver", ns_entry)) {
				// Another middleware stole our nameserver entry
				if(strncmp(address, ns_entry_data, ARG_SIZE) != 0) {
					mw_instance = -1;
					if(ConnectMiddleware(ns_entry_data, conn_sock, &hostInfo) == MW_CONNECT_SUCCESS) {
						debug_out(5, "Verification failed - Connected to %s (%s)\n", ns_entry, ns_entry_data);
						AddConnection(&connList, conn_sock);
					}
					// Another middleware stole our entry and disappeared, ignore the slot and let
					// someone else take it later
					else {
						debug_out(5, "Verification failed - Could not connect to %s (%s)\n",ns_entry, ns_entry_data);
					}
				}
			}
			ns_miss_count++;
		}
		else {
			ns_miss_count++;
		}
	}

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

int main(void) {
	pthread_t listenThread;
	int mw_sock;

	srand(time(NULL));
	
	InitConnectionList(&connList);

	set_severity(4);
	
	printf("Creating listening thread\n");
	fflush(stdout);
	pthread_create(&listenThread, NULL, ListeningThread, (void *)NULL);
	while(1) {
		printf("Starting middleware\n");
		mw_sock = start_middleware("MIDDLEWARE");
		printf("Middleware started\n");
		sleep(rand() % 30);
		printf("Stopping middleware\n");
		stop_middleware(mw_sock);
		sleep(5);
	}
	pthread_join(listenThread, NULL);
	return 0;
}
