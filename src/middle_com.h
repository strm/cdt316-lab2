#ifndef _MIDDLE_COM_H_
#define _MIDDLE_COM_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include "../framework/hidden/io.h"
#include "../framework/hidden/msg.h"

#define MW_IDENT	(-1)
#define MW_WHOIS	(-2)

void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port);
int CreateSocket(unsigned short int port);
ssize_t mw_send(int fd, void *buf, size_t len);
ssize_t force_read(int fd, void *buf, size_t count);

#endif /* _MIDDLE_COM_H_ */
