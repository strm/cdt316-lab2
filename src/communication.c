#include "communication.h"

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


