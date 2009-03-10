#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef FALSE
#define FALSE (0L)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

#define BIGSTRING 65536		/* Assumed to be big enough */

#define TCP 1
#define UDP 2

int setup_port(unsigned long portno, int protocol, int reuse);
char *stream_gets(char *buf, int buflen, int stream);
int stream_printf(int stream, char *fmt, ...);
int open_conn(char *hostname, int portno, int protocol);
int setup_socket(int protocol);
int setup_address(int portno, struct sockaddr_in *address, char *hostname);
int set_severity(int new);
void my_perror(int level, char *str);
int debug_out(int level, char *fmt, ...);
char *straddr(struct sockaddr_in *addr);
char *get_host_name();
