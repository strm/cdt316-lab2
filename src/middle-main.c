#include "communication.h"
#include "global.h"
#include "msg_queue.h"

int main(void) {
	int nBytes;
	int sock, csock;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	long stuff;
	int n = 0;
	command rcv;
	message_t newMsg;
	pthread_t lThread;
	/**
	 * SETUP
	 */
	globalMsg(MSG_SETUP, MSG_NO_ARG);
	sock = StartMiddleware("MIDDLEWARE");
	pthread_create(&lThread, NULL, Listener, (void *)sock);
	//csock = accept(sock, (struct sockaddr *)&addr, &addr_len);
	while(n) {
#if 0
		nBytes = read(csock, &stuff, sizeof(stuff));
		if(nBytes == sizeof(stuff)){
			globalMsg(MSG_LOCK, MSG_NO_ARG);
			printf("stuff: %d\n", stuff);
			for(n = 0; n < stuff; n++){
				nBytes = read(csock, &rcv, sizeof(rcv));
				if(nBytes == sizeof(rcv)){
					newMsg = createMessage(rcv.op, rcv.arg1, rcv.arg2, rcv.arg3, 0);
					globalMSG(MSG_PUSH, createNode(&newMsg));
				}
			}
		}
#endif
	}
		pthread_join(lThread, NULL);
		return 0;
	}
