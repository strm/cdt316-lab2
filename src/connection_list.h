#ifndef _CONNECTION_LIST_H_
#define _CONNECTION_LIST_H_

// Initial maximum number of connections
#define CONN_DEFAULT_LIMIT	(10)
// How many connections to add to the connection list when expanding
#define CONN_GROW_FACTOR	(CONN_DEFAULT_LIMIT)
// How many emtpy slots before expanding the connection list
#define CONN_RESIZE_THRESHOLD	(2)

// Definitions for connection status
#define STATUS_DISCONNECTED	(0)
#define STATUS_CONNECTED	(1)
#define STATUS_PENDING_ACK	(2)
#define STATUS_ACKED		(3)
#define STATUS_NONE		(4)

typedef struct {
	int socket;
	int connStatus;
	int transStatus;
} connection_t;

typedef struct {
	int nConnections;
	int maxConnections;
	connection_t *connection;
} connections_t;

/* Adds a connection to the first available position */
int AddConnection(connections_t *list, int sock);

/* Sets a connection to DISCONNECTED status */
int RemoveConnection(connections_t *list, int sock);

/* Initialize connection list to default values */
int InitConnectionList(connections_t *list);

/* Reallocate memory for a connection list to hold CONN_GROW_FACTOR more connections */
int ResizeConnectionList(connections_t *list);

#endif