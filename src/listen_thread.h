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
#include <fcntl.h>
#include "middle_com.h"
#include "msg_queue.h"
#include "connections.h"
#include "global.h"
#include "soups.h"

// Temporary define, these values will be determined elsewhere in the future
#define PORT			(12345)

ssize_t force_read(int fd, void *buf, size_t count);
int HandleMessage(message_t *msg, int from);
void *ListeningThread(void *arg);

#endif /* _LISTEN_THREAD_H_ */
