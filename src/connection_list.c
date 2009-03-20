#include "connection_list.h"

int ConnectionHandler(int cmd, int sock, connection_t *buf, connections_t *list_buf) {
	static connections_t list;
	static char first_run = TRUE;
	int ret = 0, i;

	if(first_run) {
		InitConnectionList(&list);
		first_run = FALSE;
	}

	switch(cmd) {
		case LIST_ADD:
			AddConnection(&list, sock);
			break;
		case LIST_REMOVE:
			RemoveConnection(&list, sock);
			break;
		case LIST_CONNECTION_COUNT:
			ret = list.nConnections;
			break;
		case LIST_GET_ENTRY:
			SearchConnection(&list, sock, buf);
			break;
		case LIST_COPY:
			list_buf->nConnections = list.nConnections;
			list_buf->maxConnections = list.maxConnections;
			list_buf->connection = (connections_t *)malloc(sizeof(connection_t) * list.maxConnections);
			for(i = 0; i < list_buf->maxConnections; i++)
				list_buf->connection[i] = list.connection[i];
			break;
		case LIST_REPLACE_ENTRY:
			ReplaceConnection(&list, buf);
			break;
	}
	return ret;
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

int ReplaceConnection(connections_t *list, connection_t *src) {
	int i;
	int ret = 0;

	for(i = 0; i < list->maxConnections; i++) {
		if(list->connection[i].socket == src->socket) {
			list->connection[i] = *src;
		}
		else
			ret = -1;
	}
	return ret;
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
			return 0;
		}
	}
	return -1;
}

int SearchConnection(connections_t *list, int sock, connection_t *buf) {
	int i, res;
	
	for(i = 0; i < list->maxConnections; i++) {
		if(list->connection[i].socket == sock) {
			*buf = *list->connection[i];
			res = 0;
		}
		else {
			buf = NULL;
			res = -1;
		}
	}
	return res;
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
