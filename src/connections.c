#include "connections.h"

void CreateConnectionInfo(connection *dest, int csock, char *addr, char conn_type, long cmds) {
	
	dest->socket = csock;
	dest->connection_type = conn_type;
	dest->numCmds = cmds;
	strncpy(dest->address, addr, ARG_SIZE);
	dest->next = NULL;
}

int ConnectionHandler(char cmd, connection *c, connection **clist, char *addr, int sock) {
	static connection *list = NULL;
	int ret = 0;
	connection *it;
	
	switch(cmd) {
		case ADD_TO_LIST:
			ret = AddConnection(&list, c);
			break;
		case REMOVE_BY_ADDR:
			ret = RemoveConnectionByAddress(&list, addr);
			break;
		case REMOVE_BY_SOCKET:
			ret = RemoveConnectionBySocket(&list, sock);
			break;
		case GET_BY_SOCKET:
			ret = GetConnectionBySocket(&list, c, sock);
			break;
		case GET_BY_ADDR:
			GetConnectionByAddress(&list, c, addr);
			break;
		case COPY_LIST:
			ret = CopyList(list, clist);
			break;
		case PRINT_LIST:
			for(it = list; it != NULL; it = it->next) {
				printf("socket: %d\naddr: %s\n---\n", it->socket, it->address);
			}
		default:
			for(it = list; it != NULL; it = it->next) {
				printf("socket: %d\naddr: %s\n---\n", it->socket, it->address);
			}
			ret = -1;
			break;
	}

	return ret;
}

int AddConnection(connection **list, connection *c) {
	connection *tmp;
	int ret = -1;

	if(c == NULL) {
		return EBADSRC;
	}

	tmp = (connection *)malloc(sizeof(connection));

	ret = CopyConnection(c, tmp);
	if(ret == 0) {
		tmp->next = *list;
		*list = tmp;
	}
	return ret;
}

int GetConnectionBySocket(connection **list, connection *c, int sock) {
	connection *it = NULL;
	int ret = -1;

	for(it = *list; it != NULL; it = it->next) {
		if(it->socket == sock) {
			ret = CopyConnection(it, c);
			break;
		}
	}

	return ret;
}

int GetConnectionByAddress(connection **list, connection *c, char *addr) {
	connection *it = NULL;
	int ret = -1;

	for(it = *list; it != NULL; it = it->next) {
		if(strncmp(it->address, addr, strlen(addr)) == 0) {
			ret = CopyConnection(it, c);
			break;
		}
	}

	return ret;
}

int CopyConnection(connection *src, connection *dest) {
	int name_len, ret = 0;

	if(src == NULL)
		return EBADSRC;
	else if(dest == NULL)
		return EBADDST;

	dest->socket = src->socket;
	dest->connection_type = src->connection_type;
	dest->numCmds = src->numCmds;
	dest->next = NULL;
	name_len = strlen(src->address) + 1;
	strncpy(dest->address, src->address, name_len);
	return ret;
}

int RemoveConnectionBySocket(connection **list, int sock) {
	connection *it, *prev;
	int ret = -1;

	if(list == NULL)
		return ret;

	for(it = *list, prev = *list; it != NULL; prev = it, it = it->next) {
		if(it->socket == sock) {
			if(it == *list) // First node in list, relink to second node
				*list = it->next;
			else
				prev->next = it->next;
			free(it);
			ret = 0;
			break;
		}
	}

	return ret;
}

int RemoveConnectionByAddress(connection **list, char *addr) {
	connection *it, *prev;
	int ret = -1;

	if(list == NULL)
		return ret;

	for(it = *list, prev = *list; it != NULL; prev = it, it = it->next) {
		if(strncmp(it->address, addr, ARG_SIZE) == 0) {
			if(it == *list) // First node in list, relink to second node
				*list = it->next;
			else
				prev->next = it->next;
			free(it);
			ret = 0;
			break;
		}
	}

	return ret;
}

int CopyList(connection *src, connection **dest) {
	connection *it;
	int ret = 0;

	for(it = src; it != NULL; it = it->next) {
		ret = AddConnection(dest, it);
		if(ret != 0) {
			printf("CopyList: AddConnection failed\n");
			break;
		}
	}

	return ret;
}

int DeleteConnectionList(connection **list) {
	connection *it, *prev;
	int count = 0;

	for(it = *list, prev = *list; it != NULL; prev = it) {
		it = it->next;
		free(prev);
		count++;
	}
	*list = NULL;
	return count;
}

