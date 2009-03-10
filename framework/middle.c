/*

 Receive block of requests from clients.
 Handle each request in a block, one at a time.
 Collect responses, if any, and return them.

 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "cmd.h"
#include "middle-support.h"

#define TRUE 1
#define FALSE 0

#define MANAGEMENT_DATABASE "MANAGEMENT"

#define HARD_CODED_DATABASE "DATABASE1"
#define HARD_CODED_MIDDLEWARE "MIDDLEWARE1"

#define VERSION_STRING "Vanilla 1.2" /* Set to your own version string */

int ignore_next_error = 0;	/* Used by IGNORE request */
int terminate_program = FALSE;	/* Indicate that the program should be terminated */


/* A simple list structure to keep track of answers */

struct rsplist {
  struct rsplist *next;
  response rsp;
} *rspbase = NULL;		/* A global base for the list */

/* 
   get_next_response() extracts the next response in the response-list. It returns a pointer to
   a response, or NULL if no more responses are available.
   Note! The pointer is only valid until the next call to get_next_response().
 */

response *get_next_response() {
  static response rsp;

  if (rspbase) {
    struct rsplist *tmp = rspbase;

    rspbase = tmp->next;
    memcpy(&rsp, &(tmp->rsp), sizeof(response));	/* Make a copy */
    free(tmp);
    return &rsp;
  }
  return NULL;
}

/* 
   insert_response appends its argument to the list of responses.
 */

void insert_response(response *rsp) {
  struct rsplist *tmprsp;

  /* Allocate a response structure (can't use the one on the stack) */
  tmprsp = (struct rsplist *)calloc(1, sizeof(struct rsplist));

  if (!tmprsp) {
    debug_out(2, "Failed to allocate memory.\n");
    exit(1);
  }

  memcpy(&(tmprsp->rsp), rsp, sizeof(response));	/* Make a copy */

  /* Insert it into the list of responses. */
  if (!rspbase) {		/* Empty list */
    rspbase = tmprsp;
  } else {		/* Go to end of list, and append it */
    struct rsplist *ptr = rspbase;
	    
    while (ptr->next) ptr = ptr->next;
    ptr->next = tmprsp;
  }
}


/* 
   force_read() is wrapper around the read() call that ensures that count bytes
   are always read before returning control, unless EOF or an error is
   detected.
   Arguments and return values are as for the read() call.

   We *should* use recv() call with MSG_WAITALL flag, but it is not always supported.

 */

ssize_t force_read(int fd, void *buf, size_t count) {
  size_t so_far = 0;

  while (so_far < count) {
    size_t res;
    res = read(fd, buf + so_far, count - so_far);
    if (res <= 0) return res;
    so_far += res;
  }
  return so_far;
}

/* 
   Create a response containing the specified error message. The message is a
   concaternation of the message and suffix. Appropriate flags in the response
   are set to indicate an error. The error message is truncated at the end of
   the result string if it is too long, and the return value indicates if the
   error message fit.

 */

int gen_error(char *msg, char *suffix, response *rsp) {
  rsp->result[ARG_SIZE - 1] = '\0'; /* Ensure proper termination */

  rsp->is_message = 1;		/* We want to report error */
  rsp->is_error = 1;		/* This is an error */

  /* Set error message and suffix */
  strncpy(rsp->result, msg, ARG_SIZE);
  strncat(rsp->result, suffix, (ARG_SIZE - strlen(rsp->result)));

  if (rsp->result[ARG_SIZE - 1]) { /* Did we stomp over end of string? */
    rsp->result[ARG_SIZE - 1] = '\0'; /* Ensure proper termination */
    return FALSE;
  }
  return TRUE;
}

/* 
   fetch_entry tries to fetch the specified entry from the database.
   Return value is the value of the entry, unless the function failed. Failure
   is indicated by the return value 0 AND is_error in the response structure is
   set. I.e. for return value 0, the response structure must be queried to
   determine if this was the real value, or an error indication.

   err_suffix is a string that will be appended to error messages
 */

int fetch_entry(char *entry, response *rsp, char *err_suffix) {
  char tmp[ARG_SIZE];

  if (!get_entry(tmp, HARD_CODED_DATABASE, entry)) { /* Fetch entry, check return value */
    gen_error("nonexistent", err_suffix, rsp);
    return 0;
  }

  if (!is_literal(tmp)) {	/* Is returned data a literal? */
    gen_error("bogus contents", err_suffix, rsp);
    return 0;
  }
  return atoi(tmp);
}

/* 
   Handle a SLEEP request. Sleep arg1 seconds.
 */

int handle_SLEEP(command *cmd, response *rsp) {
  rsp->is_message = 0;		/* This command yields no result */
  if (is_literal(cmd->arg1)) {
    int x = atoi(cmd->arg1);
    if (x < 0) return FALSE;
    sleep(x);
    return TRUE;
  }
  return FALSE;
}

/* 
   Handle a MAGIC request. Pass command and response to magic() and return
   whatever magic() returns.
 */

int handle_MAGIC(command *cmd, response *rsp) {
  if (strcmp(cmd->arg1, USER_COOKIE) == 0) {
    if (strcmp(cmd->arg2, USER_COOKIE) == 0 && /* Middleware version request? */
	strcmp(cmd->arg3, USER_COOKIE) == 0) {
      rsp->is_message = 1;
      rsp->is_error = 0;
      sprintf(rsp->result, "%d %d %s", getuid(), getpid(), VERSION_STRING);
      if (strlen(rsp->result) > (ARG_SIZE - 1)) {
	debug_out(2, "Result buffer overrun.");
	exit(1);
      }
      return TRUE;
    }
  } else return magic(cmd, rsp); /* magic() sets response and return code */
  return FALSE;
}

/* 
   Handle an ADD request. Add values of arg2 and arg3 and store in arg1.
 */

int handle_ADD(command *cmd, response *rsp) {
  int arg1, arg2, arg3;		/* Arguments of operation */

  rsp->is_message = 0;		/* Initially no reply */
  rsp->is_error = 0;		/* Initially no errors */

  if (!is_entry(cmd->arg1)) {	/* Is arg1 an entry? */
    gen_error("not an entry", ": arg1", rsp);
    return FALSE;
  }

  if (is_entry(cmd->arg2)) {	/* Is arg2 an entry? */
    arg2 = fetch_entry(cmd->arg2, rsp, ": arg2");
    if (!arg2 && rsp->is_error) { /* Failed to fetch entry? */
      return FALSE;
    }
  } else if (is_literal(cmd->arg2)) {
    arg2 = atoi(cmd->arg2);
  } else {
    gen_error("object not literal or entry", ": arg2", rsp);
    return FALSE;
  }

  if (is_entry(cmd->arg3)) {	/* Is arg3 an entry? */
    arg3 = fetch_entry(cmd->arg3, rsp, ": arg3");
    if (!arg3 && rsp->is_error) { /* Failed to fetch entry? */
      return FALSE;
    }
  } else if (is_literal(cmd->arg3)) {
    arg3 = atoi(cmd->arg3);
  } else {
    gen_error("object not literal or entry", ": arg3", rsp);
    return FALSE;
  }

  arg1 = arg2 + arg3;

  {
    char tmp[ARG_SIZE];
    int res;
  
    sprintf(tmp, "%d", arg1);
    res = replace_entry(tmp, HARD_CODED_DATABASE, cmd->arg1);
    if (!res) {			/* Failed to inset? */
      gen_error("assign failed", ": arg1", rsp);
      return FALSE;
    }
  }
  
  return TRUE;
}

/* 
   Handle an ASSIGN request. Store value of arg2 in arg1
 */

int handle_ASSIGN(command *cmd, response *rsp) {
  int arg2;

  rsp->is_message = 0;		/* Initially no reply */
  rsp->is_error = 0;		/* Initially no errors */

  if (!is_entry(cmd->arg1)) {	/* Is arg1 an entry? */
    gen_error("not an entry", ": arg1", rsp);
    return FALSE;
  }

  if (is_entry(cmd->arg2)) {	/* Is arg2 an entry? */
    arg2 = fetch_entry(cmd->arg2, rsp, ": arg2");
    if (!arg2 && rsp->is_error) { /* Failed to fetch entry? */
      return FALSE;
    }
  } else if (is_literal(cmd->arg2)) {
    arg2 = atoi(cmd->arg2);
  } else {
    gen_error("object not literal or entry", ": arg3", rsp);
    return FALSE;
  }

  {
    char tmp[ARG_SIZE];
    int res;
    sprintf(tmp, "%d", arg2);
    res = replace_entry(tmp, HARD_CODED_DATABASE, cmd->arg1);
    if (!res) {
      gen_error("assign failed", ": arg1", rsp);
      return FALSE;
    }
  }
  
  return TRUE;
}

/* 
   Handle a DELETE request. Remove entry arg1. Fails if arg1 does not exist.
 */

int handle_DELETE(command *cmd, response *rsp) {

  rsp->is_message = 0;		/* Initially no reply */
  rsp->is_error = 0;		/* Initially no errors */

  if (!is_entry(cmd->arg1)) {	/* Is arg1 an entry? */
    gen_error("not an entry", ": arg1", rsp);
    return FALSE;
  }

  {
    int res;
    res = delete_entry(HARD_CODED_DATABASE, cmd->arg1);
    if (!res) {
      gen_error("delete failed", ": arg1", rsp);
      return FALSE;
    }
  }
  
  return TRUE;
}

/* 
   Handle PRINT request. Insert value of arg1 into response.
 */

int handle_PRINT(command *cmd, response *rsp) {
  int arg1;

  rsp->is_message = 0;		/* Initially no reply */
  rsp->is_error = 0;		/* Initially no errors */

  if (is_entry(cmd->arg1)) {	/* Is arg1 an entry? */
    arg1 = fetch_entry(cmd->arg1, rsp, ": arg1");
    if (!arg1 && rsp->is_error) { /* Failed to fetch entry? */
      return FALSE;
    }
  } else if (is_literal(cmd->arg1)) {
    arg1 = atoi(cmd->arg1);
  } else {
    gen_error("object not literal or entry", ": arg1", rsp);
    return FALSE;
  }
  
  rsp->is_message = 1;
  sprintf(rsp->result, "%d", arg1);
  return TRUE;
}

/* 
   Handle IGNORE request. Make parser ignore next error.
 */

int handle_IGNORE(command *cmd, response *rsp) {
  rsp->is_message = 0;		/* Initially no reply */
  rsp->is_error = 0;		/* Initially no errors */

  /* Ignore errors in next two command (this + next command) */
  ignore_next_error = 2;

  return TRUE;
}

/* 
   Handle QUIT request. Terminate the middleware.
 */

int handle_QUIT(command *cmd, response *rsp) {
  rsp->is_message = 0;		/* Initially no reply */
  rsp->is_error = 0;		/* Initially no errors */

  terminate_program = TRUE;

  return TRUE;
}

/* 
   The dispatcher structure maps operations to functions.

   First argument is an operator, second argument is a pointer to a function
   that takes a pointer to a command and a response as argument and returns an
   integer (assumed to be a truth value)
 */

struct dispatcher {
  operator op;
  int (*function)(command *cmd, response *rsp);
} dispatch[] = {
  {ASSIGN, &handle_ASSIGN},
  {ADD, &handle_ADD},
  {PRINT, &handle_PRINT},
  {DELETE, &handle_DELETE},
  {SLEEP, &handle_SLEEP},
  {IGNORE, &handle_IGNORE},
  {MAGIC, &handle_MAGIC},
  {QUIT, &handle_QUIT},
  {NOCMD, NULL}
};

/* 
   A very simple parser for commands. Returns TRUE if the command could be
   parsed correctly, and FALSE otherwise.
 */

int quick_parse(command *cmd, response *rsp) {
  int i;
  int status = FALSE;

  for (i = 0; dispatch[i].op; i++) {
    if (cmd->op == dispatch[i].op) { /* Does the operators match? */
      status = (dispatch[i].function)(cmd, rsp); /* Call the function */
      break;
    }
  }
  return status;
}

int main(int argc, char **argv) {
  /*int myargc = argc;*/        /* Temporary copy of argument count */
  char **myargv = argv + 1;	/* Temporary copy of argument vector */
  int sock;			/* Socket to accept connections on */
  long num_rsp = 0;		/* Number of responses */

  set_severity(2);		/* Set debug level for debug_out() */

  while (myargv[0] && myargv[0][0] == '-') { /* Search for flags */
    if (strcmp(myargv[0], "-d") == 0) {
      set_severity(0);
      debug_out(0, "Found -d\n");
      myargv++;
      continue;
    }
    debug_out(2, "Unrecognized flag '%s'\n", myargv[0]);
    exit(1);
  }

  /* Set argc/argv - note: argv[0] is no longer program name */
  argc -= (myargv - argv);
  argv = myargv;

  /* Start middleware (open response socket and register it) */
  sock = start_middleware_frontend(HARD_CODED_MIDDLEWARE);

  while (!terminate_program) {	/* Loop until user selects QUIT function */
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    int psock;			/* Socket for a client connection */
    int res;			/* Result of I/O operations */
    long num_cmds;		/* Number of commands in this request */

    /* Accept new request */
    psock = accept(sock, (struct sockaddr *)&addr, &addrlen);

    if (!psock) {
      debug_out(2, "Failed to accept incoming connection.\n");
      exit(1);
    }

    /* Read number of commands */
    res = force_read(psock, &num_cmds, sizeof(num_cmds));

    if (res < sizeof(num_cmds)) {
      if (res < 0) my_perror(2, "force_read()");
      else debug_out(2, "Failed to read number of lines (%d)\n", res);
    }

    num_cmds = ntohl(num_cmds);	/* And convert from network byteorder */

    num_rsp = 0;		/* Reset number of responses */

    for (; num_cmds; num_cmds--) { /* Fetch all commands */
      command cmd;
      response rsp;

      memset(&rsp, 0, sizeof(rsp)); /* Clear response */

      res = force_read(psock, &cmd, sizeof(command)); /* Read command */

      if (res < sizeof(command)) {
	if (res < 0) my_perror(2, "force_read()");
	else debug_out(2, "Failed to read command (%d)\n", res);
	exit(1);
      }

      cmd.op = ntohl(cmd.op);	/* Convert from network byte order */
      rsp.seq = cmd.seq;	/* Just copy, don't need to convert */

      if (!quick_parse(&cmd, &rsp)) { /* Parse command */
	if (ignore_next_error) {
	  rsp.is_message = 0;
	  rsp.is_error = 0;
	} else {
	  if (!rsp.is_error) {
	    debug_out(2, "Could not handle message.\n");
	    rsp.is_message = 1;
	    rsp.is_error = 1;
	    strncpy(rsp.result, "Operation failed or unknown: %d\n", ARG_SIZE);
	    rsp.result[ARG_SIZE - 1] = '\0';
	  }
	}
      }
      if (rsp.is_message) { /* Got a response, handle it. */
	if (rsp.is_error && ignore_next_error) {
	  debug_out(2, "Ignoring error.\n");
	  rsp.is_message = 0;
	  rsp.is_error = 0;
	} else {
	  insert_response(&rsp); /* Insert into response list */
	  num_rsp++;		/* And we have another response */
	}
      }
      if (ignore_next_error) ignore_next_error--; /* Decrement ignore counter */
    }

    num_rsp = htonl(num_rsp);	/* Convert to network byteorder */

    res = write(psock, &num_rsp, sizeof(num_rsp)); /* Write response count */

    if (res < sizeof(num_rsp)) {
      debug_out(2, "Failed to write number of responses.\n");
      exit(1);
    } else {
      response *tmprsp;

      while ((tmprsp = get_next_response()) != NULL) { /* While there are more responses */
	res = write(psock, tmprsp, sizeof(response));
	if (res < sizeof(response)) {
	  debug_out(2, "Failed to write a response.\n");
	  exit(1);
	}
      }
    }

    close(psock);
  }

  stop_middleware_frontend(sock);
  return 0;
}
