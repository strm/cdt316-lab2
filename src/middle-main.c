#include "communication.h"

int main(void) {
	int nBytes;
	int sock, csock;
	struct sockaddr_in addr;
	int addr_len = sizeof(addr);
	int stuff;


	sock = StartMiddleware("MIDDLEWARE");
	csock = accept(sock, (struct sockaddr *)&addr, addr_len);

	while(1) {
		nBytes = force_read(csock, &stuff, sizeof(stuff));
		if(nBytes == sizeof(stuff)) 
			printf("stuff: %d\n", stuff);
	}
	
	return 0;
}
