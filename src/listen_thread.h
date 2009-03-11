#ifndef _LISTEN_THREAD_H_
#define _LISTEN_THREAD_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include "middle_com.h"

// Temporary define, these values will be determined elsewhere in the future
#define PORT			(12345)
#define VAR_LEN			(255)
#define MSG_MAX_DATA		(8)

// Initial maximum number of connections
#define CONN_DEFAULT_LIMIT	(10)
// How many connections to add to the connection list when expanding
#define CONN_GROW_FACTOR	(CONN_DEFAULT_LIMIT)
// How many emtpy slots before expanding the connection list
#define CONN_RESIZE_THRESHOLD	(2)

// Definitions for middleware communication
#define MW_DISCONNECT		(0)
#define MW_CONNECT		(1)
#define MW_TRANSACTION		(2)
#define MW_SYNCHRONIZE		(3)
#define MW_ACK			(4)
#define MW_NAK			(5)
#define MW_COMMIT		(6)

// Definitions for connection status
#define STATUS_DISCONNECTED	(0)
#define STATUS_CONNECTED	(1)
#define STATUS_PENDING_ACK	(2)
#define STATUS_ACKED		(3)
#define STATUS_NONE		(4)

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct {
	char variable[VAR_LEN];
	int value;
} data_t;

typedef struct {
	int msgType;
	int endOfMsg;
	int msgId;
	data_t data[MSG_MAX_DATA];
} message_t;

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

typedef int socketfd;

/* Adds a connection to the first available position */
int AddConnection(connections_t *list, int sock);

/* Sets a connection to DISCONNECTED status */
int RemoveConnection(connections_t *list, int sock);

/* Initialize connection list to default values */
int InitConnectionList(connections_t *list);

/* Reallocate memory for a connection list to hold CONN_GROW_FACTOR more connections */
int ResizeConnectionList(connections_t *list);

//int CreateSocket(unsigned short int port);

int ReadMessage(int sock, message_t *buf);

void HandleMessage(message_t *msg, socketfd from, fd_set *fdSet, connections_t *list);

void *ListeningThread(void *arg);

#endif /* _LISTEN_THREAD_H_ */
