#ifndef _CONNECTIONS_H_
#define _CONNECTIONS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../framework/cmd.h"

#define ADD_TO_LIST				(1)
#define REMOVE_BY_ADDR		(2)
#define REMOVE_BY_SOCKET	(3)
#define GET_BY_SOCKET			(4)
#define GET_BY_ADDR				(5)
#define COPY_LIST					(6)
#define PRINT_LIST				(7)

#define EBADSRC						(-10)
#define EBADDST						(-11)

#define TYPE_MIDDLEWARE		(10)
#define TYPE_CLIENT				(11)

typedef struct _connection {
	int socket;								// Socket!
	char address[ARG_SIZE];		// Address to the connectee
	char connection_type;			// Type of connection (middleware/client)
	long numCmds;							// Number of commands a client wishes to send
	struct _connection *next;
} connection;

void CreateConnectionInfo(connection *dest, int csock, char *addr, char conn_type, long cmds);
 
int ConnectionHandler(char cmd, connection *c, connection **clist, char *addr, int sock);
int AddConnection(connection **list, connection *c);
int GetConnectionBySocket(connection **list, connection *c, int sock);
int GetConnectionByAddress(connection **list, connection *c, char *addr);
int RemoveConnectionBySocket(connection **list, int sock);
int RemoveConnectionByAddress(connection **list, char *addr);
int CopyConnection(connection *src, connection *dest);
int CopyList(connection *src, connection **dest);

int DeleteConnectionList(connection **list);

#endif /* _CONNECTIONS_H_ */

