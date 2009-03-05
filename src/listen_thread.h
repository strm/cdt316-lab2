#ifndef _LISTEN_THREAD_H_
#define _LISTEN_THREAD_H_

#define PORT			(12345)
#define VAR_LEN			(255)
#define MSG_MAX_DATA		(8)

#define CONN_DEFAULT_LIMIT	(10)
#define CONN_GROW_FACTOR	(CONN_DEFAULT_LIMIT)
#define CONN_RESIZE_THRESHOLD	(2)

#define DISCONNECT		(0)
#define CONNECT			(1)
#define TRANSACTION		(2)
#define SYNCHRONIZE		(3)
#define ACK			(4)
#define NAK			(5)

#define STATUS_DISCONNECTED	(0)
#define STATUS_CONNECTED	(1)
#define STATUS_PENDING_ACK	(2)

/* Adds a connection to the first available position */
int AddConnection(connections_t *list, int sock);

/* Sets a connection to DISCONNECTED status */
int RemoveConnection(connections_t *list, int sock);

/* Initialize connection list to default values */
int InitConnectionList(connections_t *list);

/* Reallocate memory for a connection list to hold CONN_GROW_FACTOR more connections */
int ResizeConnectionList(connections_t *list);

int CreateSocket(unsigned short int port);

int ReadMessage(int sock, data_t *buf);

void HandleMessage(data_t *msg, int from);

void *ListeningThread(void *arg);

#endif /* _LISTEN_THREAD_H_ */