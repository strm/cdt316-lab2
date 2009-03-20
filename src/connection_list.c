#include "connection_list.h"


int ConnectionHandler(int cmd, int csock, connection_t *buf) {
	static connections_t list;
	int i;

	switch(cmd) {
	case LIST_ADD:
		return AddConnection(&list, csock);
	case LIST_REMOVE:
		return RemoveConnection(&list, csock);
	case LIST_INIT:
		return InitConnectionList(&list);
	case LIST_CONNECTION_COUNT:
		return list.nConnections;
	case LIST_GET_ENTRY:
		if(buf != NULL) {
			for(i = 0; i < list.maxConnections; i++) {
				if(list.connection[i].socket == csock) {
					buf->socket = list.connection[i].socket;
					buf->connStatus = list.connection[i].connStatus;
					buf->transStatus = list.connection[i].transStatus;
					return 0;
				}
			}
		}
		break;
	}
	return -1;
}

/*
** Name:	AddConnection
** Parameters:	list - connectionlist to add the connection to
**		sock - the socket that is used for communication
** Description:	Adds a new connection to a connection list. It sets the attributes
**		of the new connection to default values for connected sockets
*/
int AddConnection(connections_t *list, socketfd sock) {
	int i;
	
	for(i = 0; i < list->maxConnections; i++) {
		if(list->connection[i].connStatus == STATUS_DISCONNECTED) {
			list->connection[i].connStatus = STATUS_CONNECTED;
			list->connection[i].socket = sock;
			list->connection[i].numCmds = 0;
			list->nConnections++;
			
			if(list->maxConnections - list->nConnections <= CONN_RESIZE_THRESHOLD)
				ResizeConnectionList(list);
			printf("Added connection from %d in connection list\n", sock);
			return 0;
		}
	}
	return -1;
}

/*
** Name:	RemoveConnection
** Parameters:	list - connectionlist to remove the connection from
**		sock - the socket that is used for communication
** Description:	Removes an existing connection from a connection list. The attributes
**		for the entry is set to STATUS_DISCONNECTED
*/
int RemoveConnection(connections_t *list, socketfd sock) {
	int i;
	
	for(i = 0; i < list->maxConnections; i++) {
		if(list->connection[i].socket == sock) {
			list->connection[i].connStatus = STATUS_DISCONNECTED;
			list->nConnections--;
			printf("Removed connection from %d in connection list\n", sock);
			return 0;
		}
	}
	return -1;
}

/*
** Name:	InitConnectionList
** Parameters:	list		- connectionlist to initialize
** Description:	Initializes an empty list to the default values for a connection list
*/
int InitConnectionList(connections_t *list) {
	int i;
	
	list->nConnections = 0;
	list->maxConnections = CONN_DEFAULT_LIMIT;
	list->connection = (connection_t *)malloc(sizeof(connection_t) * CONN_DEFAULT_LIMIT);
	for(i = 0; i < CONN_DEFAULT_LIMIT; i++) {
		list->connection[i].socket = -1;
		list->connection[i].connStatus = STATUS_DISCONNECTED;
	}
	
	return 0;
}

/*
** Name:	ResizeConnectionList
** Parameters:	list		- connectionlist to resize
** Description:	Increases the size of a connectionlist with CONN_GROW_FACTOR elements.
**		The attributes of the new elements are set to default values.
*/
int ResizeConnectionList(connections_t *list) {
	int oldMax = list->maxConnections;
	list->maxConnections += CONN_GROW_FACTOR;
	list->connection = realloc(list->connection, sizeof(connection_t) * list->maxConnections);
	while(oldMax < list->maxConnections) {
		list->connection[oldMax].connStatus = STATUS_DISCONNECTED;
		list->connection[oldMax].socket =  -1;
		oldMax++;
	}
	return 0;
}
