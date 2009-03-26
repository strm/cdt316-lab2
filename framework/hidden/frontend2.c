#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
/* #include <setjmp.h> */
#include "io.h"
#include "msg.h"
#include "cmd.h"
#include "middle-support.h"

struct cmdlist {
  struct cmdlist *next;
  command cmd;
} *cmdbase = NULL;

char *
print_cmd(command *cmd) {
  static char buf[4096];

  cmd->arg1[ARG_SIZE-1] = '\0';
  cmd->arg2[ARG_SIZE-1] = '\0';
  cmd->arg2[ARG_SIZE-1] = '\0';

  sprintf(buf, "%ld, %ld, \"%s\", \"%s\", \"%s\"",(long int)ntohl(cmd->op),(long int)ntohl(cmd->seq), cmd->arg1, cmd->arg2, cmd->arg3);

  return buf;
}

char *
print_rsp(response *rsp) {
  static char buf[4096];

  rsp->result[ARG_SIZE-1] = '\0';

  sprintf(buf, "%ld, %d, %d, \"%s\"",(long int)ntohl(rsp->seq), rsp->is_message, rsp->is_error, rsp->result);

  return buf;
}

/* 
   force_read() is wrapper around the read() call that ensures that count bytes
   are always read before returning control, unless EOF or an error is
   detected.
   Arguments and return values are as for the read() call.
 */

size_t force_read(int fd, void *buf, size_t count) {
  size_t so_far = 0;

  while (so_far < count) {
    size_t res;
    res = read(fd, buf + so_far, count - so_far);
    if (res <= 0) return res;
    so_far += res;
  }
  return so_far;
}

int main(int argc, char **argv) {
  int sock;			/* Socket */
  char *prgname = argv[0];
  char *middleware, *filename;
  /*int myargc = argc;*/
  char **myargv = argv + 1;
  char host[DATALEN];
  int port;
  FILE *fh;
  long num_lines = 0;
  int res;

  set_severity(0);

  /* Setup our communication port */
  sock = setup_port(0, UDP, FALSE);
  if (sock < 0) {
    client_error( "Failed to open port.\n");
    exit(1);
  }

  {
    char data[DATALEN];
    int status;
    char db[TABLENAMELEN];

    if (strncmp(middleware, MWPREFIX, strlen(MWPREFIX)) != 0) {
      sprintf(db, "%s0", MWPREFIX, middleware);
    } else {
      sprintf(db, "%s0", MWPREFIX, middleware);
    }
    data[0] = '\0';		/* Ensure that data is cleared */
    status = do_get(sock, "nameserver", db, data, NULL);
    if (status != EXISTS) {
      debug_out(2, "Failed to lookup middleware %s\n", middleware);
      exit(1);
    }
    if (!extract_host_address(data, host, &port)) {
      debug_out(2, "No sensible data from nameserver: '%s'.\n", data);
      return FALSE;
    }
  }
  close(sock);

  sock = open_conn(host, port, TCP);

  if (sock <= 0) {
    debug_out(2, "Failed to open connection to middleware.\n");
    exit(1);
  }

  num_lines = htonl(num_lines);

  {
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);

    if (getpeername(sock, (struct sockaddr *)&addr, &addrlen)) {
      my_perror(2, "getpeername()");
      exit(1);
    }
    if (addrlen != sizeof(struct sockaddr_in)) {
      debug_out(2, "Bogus size of address %d != %d (send)\n", addrlen, sizeof(struct sockaddr_in));
      exit(1);
    }
    debug_out(1, "Sending %s(%d): %ld\n", straddr(&addr), ntohs(addr.sin_port), ntohl(num_lines));
  }
	num_lines = 42;
	while(1){
					res = write(sock, &num_lines, sizeof(num_lines));
					if (res < sizeof(num_lines)) {
									debug_out(2, "Failed to write number of lines.\n");
									exit(1);
					}
					debug_out(2, "Send a message");
					sleep(1);
	}

  close(sock);
  return 0;
}
