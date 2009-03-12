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

  set_severity(2);

  while (myargv[0] && myargv[0][0] == '-') {
    if (strcmp(myargv[0], "-d") == 0) {
      set_severity(0);
      debug_out(0, "Found -d\n");
      myargv++;
      continue;
    }
    client_error("Unrecognized flag '%s'\n", myargv[0]);
    exit(1);
  }

  debug_out(0, "End of flag parsing.\n");

  argc -= (myargv - argv);
  argv = myargv;

  debug_out(0, "Args left: %d\n", argc);

  if (argc < 2) {
    client_error( "%s <middleware> <filename>\n", prgname);
    exit (1);
  }

  middleware = argv[0];
  filename = argv[1];

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
      sprintf(db, "%d%s%s", getuid(), MWPREFIX, middleware);
    } else {
      sprintf(db, "%s%s", MWPREFIX, middleware);
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

  if ((fh = fopen(filename, "r")) == NULL) {
    debug_out(2, "Failed to open file '%s'\n", filename);
    exit(1);
  }

#define MAXSTR 80

  while (!feof(fh)) {
    char str[MAXSTR];
    char op[ARG_SIZE];
    struct cmdlist *cmd;
    int matches;

    if (!fgets(str, MAXSTR, fh))
      continue;

    cmd = (struct cmdlist *) calloc(1, sizeof(struct cmdlist));

    if (!cmd) {
      debug_out(2, "calloc() failed\n");
      exit(1);
    }

    matches = sscanf(str, "%s %s %s %s", op, cmd->cmd.arg1, cmd->cmd.arg2, cmd->cmd.arg3);

    if (matches < 1) {		/* No matches */
      continue;			/* skip */
    } else if (strncasecmp("NOCMD", op, MAXSTR) == 0) {
      if (matches == 1) {
	cmd->cmd.op = NOCMD;
      } else {
	debug_out(2, "Wrong number of arguments for command 'NOCMD'\n");
	exit(1);
      }
    } else if (strncasecmp("ASSIGN", op, MAXSTR) == 0) {
      if (matches == 3) {
	cmd->cmd.op = ASSIGN;
      } else {
	debug_out(2, "Wrong number of arguments for command 'ASSIGN'\n");
	exit(1);
      }
    } else if (strncasecmp("ADD", op, MAXSTR) == 0) {
      if (matches == 4) {
	cmd->cmd.op = ADD;
      } else {
	debug_out(2, "Wrong number of arguments for command 'ADD'\n");
	exit(1);
      }
    } else if (strncasecmp("PRINT", op, MAXSTR) == 0) {
      if (matches == 2) {
	cmd->cmd.op = PRINT;
      } else {
	debug_out(2, "Wrong number of arguments for command 'PRINT'\n");
	exit(1);
      }
    } else if (strncasecmp("DELETE", op, MAXSTR) == 0) {
      if (matches == 2) {
	cmd->cmd.op = DELETE;
      } else {
	debug_out(2, "Wrong number of arguments for command 'DELETE'\n");
	exit(1);
      }
    } else if (strncasecmp("SLEEP", op, MAXSTR) == 0) {
      if (matches == 2) {
	cmd->cmd.op = SLEEP;
      } else {
	debug_out(2, "Wrong number of arguments for command 'SLEEP'\n");
	exit(1);
      }
    } else if (strncasecmp("IGNORE", op, MAXSTR) == 0) {
      if (matches == 1) {
	cmd->cmd.op = IGNORE;
      } else {
	debug_out(2, "Wrong number of arguments for command 'IGNORE'\n");
	exit(1);
      }
    } else if (strncasecmp("MAGIC", op, MAXSTR) == 0) {
      if (matches == 4) {
	cmd->cmd.op = MAGIC;
      } else {
	debug_out(2, "Wrong number of arguments for command 'MAGIC'\n");
	exit(1);
      }
    } else if (strncasecmp("QUIT", op, MAXSTR) == 0) {
      if (matches == 1) {
	cmd->cmd.op = QUIT;
      } else {
	debug_out(2, "Wrong number of arguments for command 'QUIT'\n");
	exit(1);
      }
    } else {
      debug_out(2, "Unknown command '%s'\n", op);
      exit(1);
    }

    cmd->cmd.op = htonl(cmd->cmd.op);
    cmd->cmd.seq = htonl(num_lines);

    if (!cmdbase) {
      cmdbase = cmd;
    } else {
      struct cmdlist *ptr = cmdbase;
      
      while (ptr->next) ptr = ptr->next;
      ptr->next = cmd;
    }
    num_lines++;
  }
  fclose(fh);

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

  res = write(sock, &num_lines, sizeof(num_lines));

  if (res < sizeof(num_lines)) {
    debug_out(2, "Failed to write number of lines.\n");
    exit(1);
  }

  {
    struct cmdlist *ptr;

    for (ptr = cmdbase; ptr; ptr = ptr->next) {
      {
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);

	if (getpeername(sock, (struct sockaddr *)&addr, &addrlen)) {
	  my_perror(2, "getpeername()");
	  exit(1);
	}
	if (addrlen != sizeof(struct sockaddr_in)) {
	  debug_out(2, "Bogus size of address %d != %d (send 2)\n", addrlen, sizeof(struct sockaddr_in));
	  exit(1);
	}
	debug_out(1, "Sending %s(%d): %s\n", straddr(&addr), ntohs(addr.sin_port), print_cmd(&(ptr->cmd)));
      }

      res = write(sock, &(ptr->cmd), sizeof(command));
      if (res < sizeof(command)) {
	debug_out(2, "Failed to write a command.\n");
	exit(1);
      }
    }
  }

  {
    long num_lines;
    int res;

    res = force_read(sock, &num_lines, sizeof(num_lines));

    if (res < sizeof(num_lines)) {
      if (res == 0) my_perror(2, "force_read(num_lines)");
      else debug_out(2, "Failed to read number of lines (%d)\n", res);
      exit(1);
    }
  
    {
      struct sockaddr_in addr;
      int addrlen = sizeof(addr);
      
      if (getpeername(sock, (struct sockaddr *)&addr, &addrlen)) {
	my_perror(2, "getpeername()");
	exit(1);
      }
      if (addrlen != sizeof(struct sockaddr_in)) {
	debug_out(2, "Bogus size of address %d != %d (read)\n", addrlen, sizeof(struct sockaddr_in));
	exit(1);
      }
      debug_out(1, "Reading %s(%d): %ld\n", straddr(&addr), ntohs(addr.sin_port), ntohl(num_lines));
    }

    num_lines = ntohl(num_lines);

    for (; num_lines; num_lines--) {
      response rsp;

      res = force_read(sock, &rsp, sizeof(response));

      if (res < sizeof(response)) {
	if (res == 0) my_perror(2, "force_read(response)");
	else debug_out(2, "Failed to read response (%d)\n", res);
	exit(1);
      }

      {
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);

	if (getpeername(sock, (struct sockaddr *)&addr, &addrlen)) {
	  my_perror(2, "getpeername()");
	  exit(1);
	}
	if (addrlen != sizeof(struct sockaddr_in)) {
	  debug_out(2, "Bogus size of address %d != %d (read 2)\n", addrlen, sizeof(struct sockaddr_in));
	  exit(1);
	}
	debug_out(1, "Reading %s(%d): %s\n", straddr(&addr), ntohs(addr.sin_port), print_rsp(&rsp));
      }

      if (rsp.is_message) {
	if (rsp.is_error) {
	  printf("ERROR in line %ld: %s\n", (long int)ntohl(rsp.seq) + 1, rsp.result);
	} else printf("%s\n", rsp.result);
      }
    }
  }
  close(sock);
  return 0;
}
