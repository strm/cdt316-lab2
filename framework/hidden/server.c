#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "io.h"
#include "msg.h"
#include "db.h"
#include "cmd.h"
#include "middle-support.h"
#include <sys/utsname.h>
#include <netdb.h>

int testing = 0;

#define NUM_RANDOM_ERRORS 6
char *handled_cookies[NUM_RANDOM_ERRORS] = {
  "-REQLOSE", "-RSPLOSE", "-DELAY", "-DUP", "-GARBLE", "-GARBLE_OK"  /*, "-GIVE_ERROR" */
};

int real_argc;
char **real_argv;

int handle_message(message *msg, struct sockaddr_in *from, int sock);

int handle_message(message *msg, struct sockaddr_in *from, int sock) {
  int result = TRUE;
  int reply_loops = 1;		/* Normally just one reply */
  int multi = 0;		/* Simulate multiple users? */
  int cookie_check = 0;		/* If set, GET will return a specific string always */
  char cookie_check_string[] = "***********\nBOOGA-BOOGA\n***********\n";	/* cookie_check */
  int force_error = 0;		/* Force error condition */
  DBM *dbh;

  if (strncmp(msg->Cookie, COOKIE, strlen(COOKIE))) {
    debug_out(1, "Bad cookie from %s(%d)\n", straddr(from), ntohs(from->sin_port));
    return FALSE;
  }
  if ((testing && lrand48()%5==0) ||
      !strncmp(msg->Cookie+strlen(COOKIE), "-RANDOM", COOKIELEN-strlen(COOKIE)+1)) {
    if (lrand48()%3 != 0) {
      strcpy(msg->Cookie+strlen(COOKIE), handled_cookies[lrand48()%NUM_RANDOM_ERRORS]);
      debug_out(1, "Introducing random error (%s) %s(%d)\n", msg->Cookie+strlen(COOKIE),
		straddr(from), ntohs(from->sin_port));
    } else {
      debug_out(1, "-RANDOM detected, but skipped %s(%d)\n", straddr(from), ntohs(from->sin_port));
    }
  }
  if (!strncmp(msg->Cookie+strlen(COOKIE), "-REQLOSE", COOKIELEN-strlen(COOKIE)+1)) {
    debug_out(1, "Losing request %s(%d)\n", straddr(from), ntohs(from->sin_port));
    return TRUE;
  } else if (!strncmp(msg->Cookie+strlen(COOKIE), "-RSPLOSE", COOKIELEN-strlen(COOKIE)+1)) {
    debug_out(1, "Losing response %s(%d)\n", straddr(from), ntohs(from->sin_port));
    reply_loops = 0;
  } else if (!strncmp(msg->Cookie+strlen(COOKIE), "-DELAY", COOKIELEN-strlen(COOKIE)+1)) {
    if (!strncmp(msg->Table, "nameserver", TABLENAMELEN)) {
      debug_out(1, "You may not request delays from the nameserver %s(%d)\n", straddr(from),
		ntohs(from->sin_port));
    } else {
      debug_out(1, "Delaying reply %s(%d)\n", straddr(from), ntohs(from->sin_port));
      sleep(10);
    }
  } else if (!strncmp(msg->Cookie+strlen(COOKIE), "-DUP", COOKIELEN-strlen(COOKIE)+1)) {
    debug_out(1, "Duplicating response %s(%d)\n", straddr(from), ntohs(from->sin_port));
    reply_loops = 5;
  } else if (!strncmp(msg->Cookie+strlen(COOKIE), "-GARBLE", COOKIELEN-strlen(COOKIE)+1)) {
    debug_out(1, "Garbling reply %s(%d)\n", straddr(from), ntohs(from->sin_port));
    reply_loops = 1;
    junk_next(1);
  } else if (!strncmp(msg->Cookie+strlen(COOKIE), "-GARBLE_OK", COOKIELEN-strlen(COOKIE)+1)) {
    debug_out(1, "Sending garbled answer followed by correct %s(%d)\n", straddr(from), ntohs(from->sin_port));
    reply_loops = 2;
    junk_next(1);
  } else if (!strncmp(msg->Cookie+strlen(COOKIE), "-MASSIVE_GARBLE", COOKIELEN-strlen(COOKIE)+1)) {
    debug_out(1, "Garbling multiple replies, then correct %s(%d)\n", straddr(from), ntohs(from->sin_port));
    reply_loops = 6;
    junk_next(5);
  } else if (!strncmp(msg->Cookie+strlen(COOKIE), "-GIVE_ERROR", COOKIELEN-strlen(COOKIE)+1)) {
    debug_out(1, "Forcing error %s(%d)\n", straddr(from), ntohs(from->sin_port));
    force_error = 1;
  } else if (!strncmp(msg->Cookie+strlen(COOKIE), "-MULTI", COOKIELEN-strlen(COOKIE)+1)) {
    debug_out(1, "Simulating multiple users %s(%d)\n", straddr(from), ntohs(from->sin_port));
    multi = 1;
    reply_loops = 0;
  } else if (!strncmp(msg->Cookie+strlen(COOKIE), "-NOLL_MULTI", COOKIELEN-strlen(COOKIE)+1)) {
    debug_out(1, "Simulating multiple users %s(%d)\n", straddr(from), ntohs(from->sin_port));
    multi = 1;
    reply_loops = 1;
  } else if (!strncmp(msg->Cookie+strlen(COOKIE), "-COOKIE_CHECK", COOKIELEN-strlen(COOKIE)+1)) {
    debug_out(1, "Checking cookies %s(%d)\n", straddr(from), ntohs(from->sin_port));
    cookie_check = 1;
  }

  reply_copies(reply_loops);	/* Set number of copies of reply to send */

  if (force_error) {
    debug_out(1, "Forcing error return %s(%d)\n", straddr(from), ntohs(from->sin_port));
    send_reply(COOKIE, msg->Table, msg->Request,  ERROR, msg->Key, "Forced error.", msg->Id , sock, from);
    return FALSE;
  }
  if (strchr(msg->Table, '/')) {
    debug_out(1, "Cracking attempt by %s(%d)\n", straddr(from), ntohs(from->sin_port));
    send_reply(COOKIE, msg->Table, msg->Request,  ERROR, msg->Key, "Privilege violation.", msg->Id, sock, from);
    return FALSE;
  }
  {
    int i;
    for(i = 1; i < real_argc; i++) {
      if (!strcmp(msg->Table, real_argv[i])) break;
    }
    if (i == real_argc) {
      debug_out(1,"Table '%s' requested by %s(%d) is not handled by this server.\n",
		msg->Table, straddr(from), ntohs(from->sin_port));
      send_reply(COOKIE, msg->Table, msg->Request, ERROR, msg->Key, "Table does not exists in this server.",
		 msg->Id,  sock, from);
      return FALSE;
    }
  }
  dbh = mydbopen(msg->Table);
  if (!dbh) {
    debug_out(1,"Failed to open table '%s'.\n", msg->Table);
    send_reply(COOKIE, msg->Table, msg->Request, ERROR, msg->Key, "Cannot open table.",
	       msg->Id, sock, from);
    return FALSE;
  }

  switch (msg->Request) {
  case GET:
    {
      char *p;

      debug_out(0, "GET '%s' from table '%s'\n", msg->Key, msg->Table);
      p = mydbfetch(dbh, msg->Key);
      if (cookie_check) p = cookie_check_string;
      if (!p) {
	send_reply(COOKIE, msg->Table, msg->Request, NONEXISTENT, msg->Key, "",
		   msg->Id, sock, from);
	result = FALSE;
      } else {
	send_reply(COOKIE, msg->Table, msg->Request, EXISTS, msg->Key, p,
		   msg->Id, sock, from);
      }
    }
    break;
  case PUT:
    {
      int res;
      int did_multi = 0;
      char *entry = msg->Key, *data = msg->Data;
 
      if (multi && data[0]) {
	data[0]++;
	did_multi = 1;
      }

      debug_out(0, "PUT '%s' '%s' into table '%s'\n", entry, data, msg->Table);

      res = mydbinsert(dbh, entry, data); /* Insert data */
      if (did_multi) {
	data[0]--;
      }
      if (res != 0 || multi) {
	send_reply(COOKIE, msg->Table, msg->Request, EXISTS, msg->Key, data,
		   msg->Id, sock, from);
	result = FALSE;
      } else {
	send_reply(COOKIE, msg->Table, msg->Request, NONEXISTENT, msg->Key, data,
		   msg->Id, sock, from);
      }
    }
    break;
  case RPL:
    {
      int res;
      int did_multi = 0;
      char *entry = msg->Key, *data = msg->Data;
 
      if (multi && data[0]) {
	data[0]++;
	did_multi = 1;
      }

      debug_out(0, "RPL '%s' '%s' into table '%s'\n", entry, data, msg->Table);

      res = mydbreplace(dbh, entry, data); /* Insert data */
      if (did_multi) {
	data[0]--;
      }
      if (res != 0 || multi) {
	send_reply(COOKIE, msg->Table, msg->Request, ERROR, "Failed to replace data", data,
		   msg->Id, sock, from);
	result = FALSE;
      } else {
	send_reply(COOKIE, msg->Table, msg->Request, REPLACED, msg->Key, data,
		   msg->Id, sock, from);
      }
    }
    break;
  case DEL:
    {
      int res;

      debug_out(0, "DEL '%s' from table '%s'\n", msg->Key, msg->Table);
      res = mydbdelete(dbh, msg->Key);
      if (res != 0 || multi) {
	send_reply(COOKIE, msg->Table, msg->Request, NONEXISTENT, msg->Key, "",
		   msg->Id, sock, from);
	result = FALSE;
      } else {
	send_reply(COOKIE, msg->Table, msg->Request, EXISTS, msg->Key, "",
		   msg->Id, sock, from);
      }
    }
    break;
  default:
    {
      char num[80];
      sprintf(num, "%d", msg->Request);
      send_reply(COOKIE, msg->Table, msg->Request, ERROR, msg->Key, "Unrecognized operation.",
		 msg->Id, sock, from);
      result = FALSE;
      break;
    }
  }
  mydbclose(dbh);
  return result;
}

int main(int argc, char **argv) {
  int serverport;
  int reallen;
  struct sockaddr_in real, ns_adr;
  int i, local_nameserver = 0, got_nsadr = 0;
  DBM *dbh;

  srand48(time(NULL));

  real_argc = argc;
  real_argv = argv;

  if (argc < 2) {
    debug_out(2, "Must specify at least one database.\n");
    exit(1);
  }

  if (!strncmp(argv[1], "-d", 3)) {
    real_argc = argc - 1;
    real_argv = argv + 1;
    set_severity(0);
  } else {
    set_severity(2);
  }

  serverport = setup_port(0, UDP, TRUE);
  if (serverport < 0) {
    debug_out(2,"Error setting up port\n");
    exit(1);
  }

  reallen = sizeof(struct sockaddr_in);
  if (getsockname(serverport, (struct sockaddr *) &real, &reallen) < 0) {
    my_perror(2,"getsockname");
    exit(1);
  }

  if (reallen != sizeof(struct sockaddr_in)) {
    debug_out(2,"Length mismatch in getsockname()\n");
    exit(1);
  }

  debug_out(0,"Opened local port: %d\n", ntohs(real.sin_port));

  for (i = 1; i < real_argc; i++) {
    char *db;
    char *database = real_argv[i];

    db = (char *) calloc(TABLENAMELEN, sizeof(char));

    if (!db) {
      debug_out(2, "Failed to allocate memory.\n");
      exit(1);
    }

    if (!strcmp(NSDB, database)) {
#ifndef LOCAL_VERSION
      /*if (getuid() != NSOWNER_UID) {
	debug_out(2, "You are not allowed to start the nameserver.\n");
	exit(1);
      }*/
#endif

      fflush(stdout);
      local_nameserver = TRUE;
      close(serverport);
      serverport = setup_port(NSPORT, UDP, TRUE);
      if (serverport < 0) {
	debug_out(2,"Error setting up port.\n");
	exit(1);
      }
      reallen = sizeof(struct sockaddr_in);
      if (getsockname(serverport, (struct sockaddr *) &real, &reallen) < 0) {
	my_perror(2,"getsockname");
	exit(1);
      }
      
      if (reallen != sizeof(struct sockaddr_in)) {
	debug_out(2, "Length mismatch in getsockname()\n");
	exit(1);
      }
      
      debug_out(0,"This is the nameserver, switching to nameserver-port (%d).\n", ntohs(real.sin_port));

      dbh = mydbopen(NSDB);
      if (!dbh) {
	debug_out(2, "Failed to open nameserver table.\n");
	exit(1);
      }
    }

    if (strncmp(database, DBPREFIX, strlen(DBPREFIX)) != 0 &&
	strcmp(database, MANAGEMENT) != 0 &&
	strcmp(database, NSDB) != 0) {
      debug_out(2, "Bogus database '%s' specified\n", database);
      exit(1);
    }

    if (strcmp(database, MANAGEMENT) != 0 &&
	strcmp(database, NSDB) != 0) { /* Not management or nameserver */
      char *p,*q;			/* temporary pointers */
	
      p = q = database + strlen(DBPREFIX);
	
      while (*p && *p >= '0' && *p <= '9') p++;
	
      /* 12 characters are needed for a text representation of a 32-bit integer */
	
      /* If a (non-null) character at p, or no digits detected, or name too long */
      if (*p || p == q || strlen(database) > (TABLENAMELEN - 12)) {
	debug_out(2, "Bogus database '%s'\n", database);
	return FALSE;
      }
    }
      
    if (strcmp(database, NSDB) != 0) {
      sprintf(db, "%d%s", getuid(), database);
    } else {
      sprintf(db, "%s", database);
    }
    real_argv[i] = db;		/* Patch this list... */

    if (local_nameserver) {
      char address[120];
      int res;
      sprintf(address, "%s:%d", get_host_name(), NSPORT);
      res = mydbdelete(dbh, db); /* Don't care about result */
      res = mydbinsert(dbh, db, address); /* Insert data */
      if (res < 0) {
	debug_out(2, "failed to dbinsert(%s, %s)\n", db, address);
	exit(1);
      }
      debug_out(0, "Inserted '%s' (%s) into local nameserver.\n", db, address);
    } else {
      char address[DATALEN];
      char tmpdata[DATALEN];
      int status;
      int allow_insert = TRUE;

      alarm(43200);		/* Signal (and die) after 12 hours */

      /* Check argument, and mangle into DATABASE<n> */

      if (!got_nsadr) {
	if (!lookup_table(serverport, "nameserver", &ns_adr)) {
	  exit(1);
	}
	got_nsadr = 1;
      }

      sprintf(address, "%s:%d", get_host_name(), ntohs(real.sin_port)); /* Create entry */

      status = do_get(serverport, "nameserver", db, tmpdata, &ns_adr);

      switch (status) {
      case NONEXISTENT:
	allow_insert = TRUE;
	break;
      case EXISTS:
	status = do_get(serverport, db, "probe", tmpdata, NULL);
	if (status != TIMEOUT_ERROR && status != ERROR)
	  allow_insert = FALSE;
	else {
	  status = do_del(serverport, "nameserver", db, tmpdata, &ns_adr);
	  if (status != EXISTS) {
	    debug_out(1, "There's something screwy going on here...\n"
		      "\tUnable to delete entry for table '%s' in nameserver\n", db);
	  }
	  allow_insert = TRUE;
	}
	break;

      }

      if (allow_insert) {
	status = do_put(serverport, "nameserver", db, address, &ns_adr);
	if (status == NONEXISTENT)
	  debug_out(1, "Inserted '%s' into nameserver.\n", db);
	else
	  debug_out(1, "Failed to insert '%s' into nameserver.\n", db);
      } else {
	debug_out(1, "Table '%s' already exists in nameserver.\n", db);
      }
    }
  }
  if (local_nameserver) mydbclose(dbh);

  for (;;) {
    int fromlen;
    message msg;
    struct sockaddr_in from;
    int res;

    fromlen = sizeof(struct sockaddr_in);
    res = receive_msg(serverport, &msg, &from, &fromlen);
    if (res < 0) {
      continue;
    }

    debug_out(1,"Received %s(%d): {%s}\n", straddr(&from), ntohs(from.sin_port),  print_message(&msg));

    if (!handle_message(&msg, &from, serverport)) {
      debug_out(0,"Message could not be handled.\n");
    }
  }
  return 0;
}
