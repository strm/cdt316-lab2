#ifndef _CONNECTION_LIST_H_
#define _CONNECTION_LIST_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../framework/cmd.h"
#include <netinet/in.h>
#include <sys/socket.h>

// Initial maximum number of connections
#define CONN_DEFAULT_LIMIT		(10)
// How many connections to add to the connection list when expanding
#define CONN_GROW_FACTOR			(CONN_DEFAULT_LIMIT)
// How many emtpy slots before expanding the connection list
#define CONN_RESIZE_THRESHOLD	(2)

#define LIST_ADD 							(0)
#define LIST_REMOVE						(1)
#define LIST_INIT							(-1)
#define LIST_CONNECTION_COUNT	(2)
#define LIST_GET_ENTRY				(3)
#define LIST_COPY							(4)
#define LIST_REPLACE_ENTRY		(5)
#define LIST_SET_CLIENT				(6)
#define LIST_SET_MIDDLEWARE		(7)
#define LIST_PRINT						(8)

// Definitions for connection status
#define STATUS_DISCONNECTED		(0)
#define STATUS_CONNECTED			(1)
#define STATUS_PENDING_ACK		(2)
#define STATUS_ACKED					(3)
#define STATUS_NONE						(4)

#define TYPE_CLIENT						(2)
#define TYPE_MIDDLEWARE				(1)
#define TYPE_UNKNOWN					(0)

typedef struct {
	int socket;
	int connStatus;
	int transStatus;
	int type;
	int numCmds;
	char addr[ARG_SIZE];
} connection_t;

typedef struct {
	int nConnections;
	int maxConnections;
	connection_t *connection;
} connections_t;

typedef int socketfd;

int ConnectionHandler(int cmd, int sock, connection_t *buf, connections_t *list_buf, struct sockaddr *cInfo);

/* Sets the connection type (MW or client) in a connection */
int SetConnectionType(connections_t *list, socketfd sock, int type);

/* Adds a connection to the first available position */
int AddConnection(connections_t *list, int sock, char *addr);

/* Replaces an entry with the information supplied */
int ReplaceConnection(connections_t *list, connection_t *src);

/* Sets a connection to DISCONNECTED status */
int RemoveConnection(connections_t *list, int sock);

/* Search for a socket in the connection list */
int SearchConnection(connections_t *list, int sock, connection_t *buf);

/* Initialize connection list to default values */
int InitConnectionList(connections_t *list);

/* Reallocate memory for a connection list to hold CONN_GROW_FACTOR more connections */
int ResizeConnectionList(connections_t *list);

#endif

