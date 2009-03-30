#include "connection_list.h"
#include <sys/socket.h>

int ConnectionHandler(int cmd, int sock, connection_t *buf, connections_t *list_buf, struct sockaddr *cInfo) {
	static connections_t list;
	static char first_run = 1;
	int ret = 0, i;
	char addr[ARG_SIZE] = "";

	if(first_run) {
		printf("Connection handler: First run (init)\n");
		InitConnectionList(&list);
		first_run = 0;
	}

	switch(cmd) {
		case LIST_SET_MIDDLEWARE:
			SetConnectionType(&list, sock, TYPE_MIDDLEWARE);
			break;
		case LIST_SET_CLIENT:
			ret = SetConnectionType(&list, sock, TYPE_CLIENT);
			break;
		case LIST_ADD:
			sprintf(addr, "%s", cInfo->sa_data);
			ret = AddConnection(&list, sock, addr);
			break;
		case LIST_REMOVE:
			ret = RemoveConnection(&list, sock);
			break;
		case LIST_CONNECTION_COUNT:
			ret = list.nConnections;
			break;
		case LIST_GET_ENTRY:
			ret = SearchConnection(&list, sock, buf);
			break;
		case LIST_COPY:
			list_buf->nConnections = list.nConnections;
			list_buf->maxConnections = list.maxConnections;
			list_buf->connection = (connection_t *)malloc(sizeof(connection_t) * list.maxConnections);
			for(i = 0; i < list_buf->maxConnections; i++) {
				list_buf->connection[i].socket = list.connection[i].socket;
				list_buf->connection[i].connStatus = list.connection[i].connStatus;
				list_buf->connection[i].transStatus = list.connection[i].transStatus;
				list_buf->connection[i].type = list.connection[i].type;
				list_buf->connection[i].numCmds = list.connection[i].numCmds;
			}
			break;
		case LIST_REPLACE_ENTRY:
			ReplaceConnection(&list, buf);
			break;
		case LIST_PRINT:
			for(i = 0; i < list.maxConnections; i++) {
				if(list.connection[i].connStatus == STATUS_CONNECTED) {
					printf("socket: %d, status: %d\n", list.connection[i].socket, list.connection[i].connStatus);
				}
			}
	}
	return ret;
}

int SetConnectionType(connections_t *list, socketfd sock, int type) {
	int i, ret = -1;

	for(i = 0; i < list->maxConnections; i++) {
		if(list->connection[i].socket == sock) {
			list->connection[i].type = type;
			ret = 0;
		}
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
int AddConnection(connections_t *list, socketfd sock, char *addr) {
	int i;
	struct sockaddr_in saddr;
	socklen_t saddr_len = sizeof(saddr);

	if(getpeername(sock, &saddr, &saddr_len) == -1) {
		perror("getsockname");
		//return -1;
	}

//	printf("sa_data: %c\n", saddr.sa_data[0]);
	printf("sa_data (int): %d\n", ntohs(saddr.sin_port));



	for(i = 0; i < list->maxConnections; i++) {
		if(list->connection[i].connStatus == STATUS_DISCONNECTED) {
			list->connection[i].connStatus = STATUS_CONNECTED;
			list->connection[i].socket = sock;
			list->connection[i].numCmds = 0;
			list->connection[i].type = TYPE_MIDDLEWARE;
			//strncpy(list->connection[i].addr, saddr.sa_data, strlen(saddr.sa_data));
			(list->nConnections)++;

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
			list->connection[i].type = TYPE_MIDDLEWARE;
			(list->nConnections)--;
			return 0;
		}
	}
	return -1;
}

int SearchConnection(connections_t *list, int sock, connection_t *buf) {
	int i, res;
	
	for(i = 0; i < list->maxConnections; i++) {
		if(list->connection[i].socket == sock) {
			*buf = list->connection[i];
			res = 0;
			break;
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
		list->connection[i].type = TYPE_MIDDLEWARE;
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
		list->connection[oldMax].type = TYPE_MIDDLEWARE;
		list->connection[oldMax].socket =  -1;
		oldMax++;
	}
	return 0;
}
