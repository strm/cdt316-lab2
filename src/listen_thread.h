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
#include "connection_list.h"
#include "global.h"
#include "soups.h"

// Temporary define, these values will be determined elsewhere in the future
#define PORT			(12345)
#define VAR_LEN			(255)
#define MSG_MAX_DATA		(8)

int ReadMessage(int sock, message_t *buf);
int HandleMessage(message_t *msg, socketfd from, fd_set *fdSet, connections_t *list);
void *ListeningThread(void *arg);

#endif /* _LISTEN_THREAD_H_ */
