#include "communication.h"

void Listener(void *sock) {
	int connection_socket;
	int accept_socket = (int *)sock;
	int i;
	long recvBuf, nBytes;
	fd_set master_set;
	fd_set read_set;

	FD_ZERO(&master_set);
	FD_SET(accept_socket, &master_set);

	while(1) {
		read_set = master_set;
		if(select(FD_SETSIZE; &read_set, NULL, NULL, NULL) < 0) {
			perror("select");
		}
		for(i = 0; i < FD_SETSIZE; i++) {
			if(FD_ISSET(i, &read_set)) {
				if(i == accept_socket) {
					connection_socket = accecpt(accept_socket, NULL, NULL);
					if(connection_socket < 0) {
						perror("accept");
					}
					else
						FD_SET(connection_socket, &master_set);
				}
			}
			else {
				nBytes = read(i, &recvBuf, sizeof(recvBuf));
				if(nbytes < 0) {
					perror("read");
				}
				else {
					debug_out(2, "Received %ld\n", recvBuf);
				}
			}
		}
	}
}

int StartMiddleware(char *mw_name) {
	int sock;
	char address[ARG_SIZE];
	char mw_entry[TABLENAMELEN];
	int mw_instance = 0;
	struct sockaddr_in real;
	int reallen;

	if(strncmp(mw_name, MWPREFIX, strlen(MWPREFIX)) != 0) {
		debug_out(2, "Bogus database '%s'\n", mw_name);
		return FALSE;
	}
	
	sprintf(mw_entry, "%s%d", mw_name, mw_instance);
	sock = setup_port(0, TCP, FALSE);
	if(sock < 0) {
		debug_out(2, "StartMiddleware: Error setting up port\n");
		exit(1);
	}

	reallen = sizeof(struct sockaddr_in);
	if(getsockname(sock, (struct sockaddr *)&real, &reallen) < 0) {
		my_perror(2, "getsockname");
		exit(1);
	}

	sprintf(address, "%s:%d", get_host_name(), ntohs(real.sin_port));

	if(!replace_entry(address, "nameserver", mw_entry)) {
		debug_out(2, "--- communication.c:33 ---\n");
		debug_out(2, "Failed to enter data into nameserver\n");
		exit(1);
	}

	return sock;
}


