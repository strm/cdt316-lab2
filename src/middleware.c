#include <string.h>
#include "../framework/cmd.h"
#include "../framework/middle-support.h"
#include "middle_com.h"
#include "listen_thread.h"

#define NS_LOOKUP_ATTEMPTS	(10)

static char myname[TABLENAMELEN] = "";

int start_middleware(char *database) {
	int sock;
	char address[ARG_SIZE];
	int reallen;
	struct sockaddr_in real;
	char db[TABLENAMELEN];
	char *p;
	
	int mw_instance = -1;
	int ns_miss_count;
	int ns_iterator;
	char ns_entry[TABLENAMELEN];
	char ns_entry_data[ARG_SIZE];
	char *conn_address;
	int conn_port;
	int conn_sock;
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
			/*TODO: add the connection to the connection list */
			conn_address = strtok(ns_entry_data, ":");
			conn_port = atoi(strtok(NULL, ":"));
			conn_sock = socket(PF_INET, SOCK_STREAM, 0);
			initSocketAddress(&hostInfo, conn_address, conn_port);
			
			while(1) {
				if(connect(conn_sock, (struct sockaddr *)&hostInfo, sizeof(hostInfo)) < 0) {
					perror("connect");
					printf("Could not connect to %s... Retrying in 2 seconds\n", ns_entry);
					sleep(2);
				}
				else {
					printf("Connected to %s\n", ns_entry);
					break;
				}
			}
			
			ns_miss_count = 0;
		}
		else if(mw_instance == -1) {
			mw_instance = ns_iterator;
			sprintf(myname, "%s%d", database, mw_instance);
			sprintf(address, "%s:%d", get_host_name(), ntohs(real.sin_port));
			if(!replace_entry(address, "nameserver", myname)) {
				debug_out(2, "Failed to enter data into nameserver\n");
				exit(1);
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
		debug_out(2, "Not registered!\n");
		exit(1);
	}

	if (!delete_entry(NSDB, myname)) {
		/* TODO: Send DISCONNECT to all connected middlewares */
		debug_out(2, "Failed to delete data from nameserver\n");
		exit(1);
	}

	myname[0] = '\0';
	close(sock);
}

int main(void) {
	pthread_t listenThread;
	
	printf("Creating listening thread\n");
	fflush(stdout);
	sleep(1);
	pthread_create(&listenThread, NULL, ListeningThread, (void *)NULL);
	printf("Starting middleware\n");
	fflush(stdout);
	sleep(1);
	start_middleware("MIDDLEWARE");
	printf("Middleware started\n");
	fflush(stdout);
	sleep(1);
	pthread_join(listenThread, NULL);
	return 0;
}
