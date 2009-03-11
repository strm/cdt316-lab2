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
#include "../framework/io.h"
#include "../framework/msg.h"

void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port);
int CreateSocket(unsigned short int port);

#endif /* _MIDDLE_COM_H_ */
