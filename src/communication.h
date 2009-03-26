#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_


#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "../framework/nameserver.h"
#include "../framework/cmd.h"
#include "../framework/middle-support.h"
#include "../framework/hidden/io.h"
#include "../framework/hidden/msg.h"

void Listener(void *sock);
int StartMiddleware(char *mw_name);

#endif /* _COMMUNICATION_H_ */

