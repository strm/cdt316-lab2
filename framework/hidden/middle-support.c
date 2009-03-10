#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include "nameserver.h"
#include "cmd.h"
#include "middle-support.h"
#include "io.h"
#include "msg.h"

enum state {empty, minus, negative, positive, entry, bogus};

static int io_delay = 0;

/* Return state of arg */
enum state check_type(char *arg) {
  char *p = arg;
  enum state type = empty;
  if (*p && *p == '-') {
    type = minus;
    p++;
  }
  while (*p) {
    if (isdigit(*p)) {
      if (type == empty) {
	type = positive;
      } else if (type == minus) {
	type = negative;
      }
    } else if (isalpha(*p)) {
      if (type == minus || type == negative) return bogus;
      type = entry;
    } else {
      return bogus;
    }
    p++;
  }
  if (type == minus) type = bogus;
  return type;
}

/* True if arg is a literal */
int is_literal(char *arg) {
  enum state type = check_type(arg);
  if (type == positive || type == negative) return TRUE;
  else return FALSE;
}

/* True if arg is an entry */
int is_entry(char *arg) {
  enum state type = check_type(arg);
  if (type == entry) return TRUE;
  else return FALSE;
}

/* Handle backend cookies */
int magic(command *cmd, response *rsp) {
  rsp->is_message = 0;

  if (cmd->op != MAGIC) return FALSE;

  if (strcmp(cmd->arg1, "1") == 0 &&
      strcmp(cmd->arg2, "1") == 0 &&
      is_literal(cmd->arg3)) {
    int i = atoi(cmd->arg3);
    if (i < 0) {
      io_delay = 0;
    } else {
      io_delay = i;
    }
  }
  return TRUE;
}

int op_entry(request req, char *str, char *database, char *entry) {
  int sock;			/* Socket */
  char tmpstr[DATALEN] = "";
  char *p;
  char db[TABLENAMELEN];
  int status;

  if (strncmp(database, DBPREFIX, strlen(DBPREFIX)) != 0 &&
      strcmp(database, MANAGEMENT) != 0 &&
      strcmp(database, NSDB) != 0) {
    debug_out(2, "Bogus database '%s'\n", database);
    return FALSE;
  }

  if (strcmp(database, MANAGEMENT) != 0 &&
      strcmp(database, NSDB) != 0) { /* Not management or nameserver */
    char *q;			/* temporary pointer */

    p = q = database + strlen(DBPREFIX);

    while (*p && *p >= '0' && *p <= '9') p++;

    /* 12 characters are needed for a text representation of a 32-bit integer */

    /* If a (non-null) character at p, or no digits detected, or name too long */
    if (*p || p == q || strlen(database) > (TABLENAMELEN - 12)) {
      debug_out(2, "Bogus database *'%s'\n", database);
      return FALSE;
    }
  }

  if (strcmp(database, NSDB) != 0) {
    sprintf(db, "%d%s", getuid(), database);
  } else {
    sprintf(db, "%s", database);
  }

  if (io_delay) {
    debug_out(0, "Sleeping for %d seconds\n", io_delay);
    sleep(io_delay); /* Go to sleep for a while */
  }

  sock = setup_port(0, UDP, FALSE);
  if (sock < 0) {
    debug_out(2, "Failed to open port.\n");
    exit(1);
  }
  switch (req) {
  case GET:
    status = do_get(sock, db, entry, tmpstr, NULL);
    if (status == EXISTS) {
      strncpy(str, tmpstr, ARG_SIZE);
      str[ARG_SIZE - 1] = 0;
      close(sock);
      return TRUE;
    }
    break;
  case DEL:
    status = do_del(sock, db, entry, tmpstr, NULL);
    if (status == EXISTS) {
      strncpy(str, tmpstr, ARG_SIZE);
      str[ARG_SIZE - 1] = 0;
      close(sock);
      return TRUE;
    }
    break;
  case PUT:
    strncpy(tmpstr, str, ARG_SIZE);
    status = do_put(sock, db, entry, tmpstr, NULL);
    if (status == NONEXISTENT) {
      strncpy(str, tmpstr, ARG_SIZE);
      str[ARG_SIZE - 1] = 0;
      close(sock);
      return TRUE;
    }
    break;
  case RPL:
    strncpy(tmpstr, str, ARG_SIZE);
    status = do_rpl(sock, db, entry, tmpstr, NULL);
    if (status == REPLACED) {
      strncpy(str, tmpstr, ARG_SIZE);
      str[ARG_SIZE - 1] = 0;
      close(sock);
      return TRUE;
    }
    break;
  }
  close(sock);
  return FALSE;
}

int get_entry(char *str, char *database, char *entry) {
  return op_entry(GET, str, database, entry);
}

int replace_entry(char *str, char *database, char *entry) {
  return op_entry(RPL, str, database, entry);
}

int delete_entry(char *database, char *entry) {
  char str[ARG_SIZE];
  return op_entry(DEL, str, database, entry);
}

#define MWPREFIX "MIDDLEWARE"

static char myname[TABLENAMELEN] = "";

int start_middleware_frontend(char *database) {
  int sock;
  char address[ARG_SIZE];
  int reallen;
  struct sockaddr_in real;
  char db[TABLENAMELEN];
  char *p;

  if (myname[0] != '\0') {
    debug_out(2, "Already registered!\n");
    exit(1);
  }

  if (strncmp(database, MWPREFIX, strlen(MWPREFIX)) != 0) {
    debug_out(2, "Bogus database '%s'\n", database);
    return FALSE;
  }

  {
    char *q;			/* temporary pointer */

    p = q = database + strlen(MWPREFIX);

    while (*p && *p >= '0' && *p <= '9') p++;

    /* 12 characters are needed for a text representation of a 32-bit integer */

    /* If a (non-null) character at p, or no digits detected, or name too long */
    if (*p || p == q || strlen(database) > (TABLENAMELEN - 12)) {
      debug_out(2, "Bogus database '%s'\n", database);
      exit(1);
    }
  }

  sprintf(db, "%d%s", getuid(), database);

  strcpy(myname, db);

  sock = setup_port(0, TCP, FALSE);
  if (sock < 0) {
    debug_out(2,"Error setting up port\n");
    exit(1);
  }

  reallen = sizeof(struct sockaddr_in);
  if (getsockname(sock, (struct sockaddr *) &real, &reallen) < 0) {
    my_perror(2,"getsockname");
    exit(1);
  }
      
  if (reallen != sizeof(struct sockaddr_in)) {
    debug_out(2, "Length mismatch in getsockname()\n");
    exit(1);
  }

  sprintf(address, "%s:%d", get_host_name(), ntohs(real.sin_port));

  if (!replace_entry(address , "nameserver", db)) {
    debug_out(2, "Failed to enter data into nameserver\n");
    exit(1);
  }

  return sock;
}

void stop_middleware_frontend(int sock) {

  if (myname[0] == '\0') {
    debug_out(2, "Not registered!\n");
    exit(1);
  }

  if (!delete_entry(NSDB, myname)) {
    debug_out(2, "Failed to delete data from nameserver\n");
    exit(1);
  }

  myname[0] = '\0';
  close(sock);
}
