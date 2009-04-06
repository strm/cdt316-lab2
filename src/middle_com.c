#include "middle_com.h"
#include "soups.h"
void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port) {
	struct hostent *hostInfo; /* Contains info about the host */
	/* Socket address format set to AF_INET for internet use. */
	name->sin_family = AF_INET;     
	/* Set port number. The function htons converts from host byte order to network byte order.*/
	name->sin_port = htons(port);
	/* Get info about host. */
	hostInfo = gethostbyname(hostName); 
	if(hostInfo == NULL) {
		fprintf(stderr, "initSocketAddress - Unknown host %s\n",hostName);
		exit(EXIT_FAILURE);
	}
	/* Fill in the host name into the sockaddr_in struct. */
	name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
}

int CreateSocket(unsigned short int port) {
	struct sockaddr_in sockName;
	int sock;
	
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		//TODO: Add error handling here
	}
	sockName.sin_family = AF_INET;
	sockName.sin_port = htons(port);
	sockName.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock, (struct sockaddr*)&sockName, sizeof(sockName)) < 0) {
		debug_out(10, "Bind failed in CreateSocket\n");
		//TODO: Add error handling here
	}
	return sock;
}

ssize_t mw_send(int fd, void *buf, size_t len) {
	ssize_t nBytes;

	// Send -1 to inform receiver that the message is from a middleware
	debug_out(5, "mw_send: fd=%d\n", ((message_t* )buf)->msgId);
	if((nBytes = send(fd, buf, len, MSG_NOSIGNAL)) < 0) {
		perror("mw_send:send - ");
	}
	return nBytes;
}

/* 
   force_read() is wrapper around the read() call that ensures that count bytes
   are always read before returning control, unless EOF or an error is
   detected.
   Arguments and return values are as for the read() call.

   We *should* use recv() call with MSG_WAITALL flag, but it is not always supported.

 */
ssize_t force_read(int fd, void *buf, size_t count) {
  size_t so_far = 0;

  while (so_far < count) {
		size_t res;
		res = read(fd, buf + so_far, count - so_far);
		if (res <= 0) return res;
		so_far += res;
	}
	return so_far;
}

