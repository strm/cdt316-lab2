#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include "io.h"
#define MSG
#include "msg.h"

static int junk = 0;
int reqno = 0;


#if 0
int num_entries = 0;

struct msg_queue {
  struct msg_queue *next, *prev;	/* Link */
  int sock;			/* Descriptor */
  struct sockaddr_in local;		/* Local address - should match current sockname of sock */
  struct sockaddr_in from;		/* Sender address */
  int fromlen;			/* Length of address */
  message msg;			/* Actual message */
  time_t when;			/* Timestamp */
} *start = NULL, *end = NULL;
#endif

struct msglist {
  struct msglist *next;
  message msg;
  struct sockaddr_in from;
} *basemsg = NULL;

void add_to_msglist(message *msg, struct sockaddr_in *from) {
  struct msglist *item = (struct msglist *) calloc(1, sizeof(struct msglist));

  memcpy(&item->msg, msg, sizeof(message));
  memcpy(&item->from, from, sizeof(struct sockaddr_in));

  if (!basemsg) {		/* Empty list */
    basemsg = item;
  } else {		/* Go to end of list, and append it */
    struct msglist *ptr = basemsg;
    
    while (ptr->next) ptr = ptr->next;
    ptr->next = item;
  }
}

long gen_id() {			/* Generate pseudo-random id */
  static long id = 0;

  if (id == 0) {		/* Initialize on first use */
    register long tmp = ((getpid()<<16) | getuid());
    debug_out(1, "Generated ID seed is %ld\n", tmp);
    srand48(tmp);
    id = lrand48();
  } else {
    id++;
  }
  return htonl(id);		/* Set to network byteorder */
}

cookies *new_cookie() {
  cookies *tmp = (cookies *)calloc(1, sizeof(cookies));
  if (!tmp) {
    client_error("Failed to allocate memory.\n");
    exit(1);
  }
  return tmp;
}

cookies *insert_cookie(char *cookie, int req, cookies *first) {
  if (!first || req < first->req) {
    cookies *next = new_cookie();
    next->req = req;
    next->cookie = cookie;
    next->next = first;
    return next;
  } else {
    first->next = insert_cookie(cookie, req, first->next);
    return first;
  }
}

int reply_loops = 1;		/* Default to one message */

void reply_copies(int copies) {
  reply_loops = copies;
}

void junk_next(int i) {
  junk = i;
}

int delay_ack = 0;

void set_delay_ack(int sec) {
  if (sec >= 0) delay_ack = sec;
}

int client_error(char *fmt, ...) {
  int res;
  va_list ap;

  va_start(ap, fmt);
  fprintf(stdout, "CLIENT_ERROR\n");
  res = vfprintf(stdout, fmt, ap);
  if (res < 0) {
    res = -1;
  }
  va_end(ap);
  return res;
}

int internal_error(char *fmt, ...) {
  int res;
  va_list ap;

  va_start(ap, fmt);
  fprintf(stdout, "INTERNAL_ERROR\n");
  res = vfprintf(stdout, fmt, ap);
  if (res < 0) {
    res = -1;
  }
  va_end(ap);
  return res;
}

int nameserver_error(char *fmt, ...) {
  int res;
  va_list ap;

  va_start(ap, fmt);
  fprintf(stdout, "NAMESERVER_ERROR\n");
  res = vfprintf(stdout, fmt, ap);
  if (res < 0) {
    res = -1;
  }
  va_end(ap);
  return res;
}

int extract_host_address(char *buf, char *host, int *port) {
  char *start = buf, *end = buf, *tmp, *realhost = host;

  while (*end && *end != '%' && *end != '\n') end++; /* Terminate at '%' or '\n'
 */
  *end = '\0';                  /* End string. */
 
  /* Extract hostname */
  tmp = start;
  while (*start && *start != ':') *(host++) = *(start++);
  *host = '\0';
 
  debug_out(0, "Extracted host '%s'\n", realhost);

  /* We should have moved startpoint, and be at a ':' */
  if (*start != ':' || tmp == start) return FALSE;
 
  /* Skip to start of portnumber */
  while (*start && *start == ':') start++;
 
  /* Extract portnumber */
  *port = 0;
  tmp = start;
  while (*start && isdigit(*start)) *port = (*port * 10) + (*(start++) - '0');
 
  debug_out(0, "Extracted port '%d'\n", *port);

  /* Did we get something? */
  if (tmp == start) return FALSE; /* Nothing extracted */
 
  return TRUE;
}

int lookup_table(int sock, char *table, struct sockaddr_in *addr) {
  struct sockaddr_in ns_adr;
  char nsanswer[DATALEN];
  char host[DATALEN];
  int port;
  int res;

  nsanswer[0] = '\0';

  /* Build address to nameserver */
  if (!setup_address(NSPORT, &ns_adr, NAMESERVER)) {
    nameserver_error("Failed to setup address.\n");
    return FALSE;
  }

  if ((res = send_cmd(sock, &ns_adr, GET, NSDB, table, nsanswer)) != EXISTS) {
    nameserver_error("Failed to consult nameserver\n");
    return FALSE;
  }

  if (!extract_host_address(nsanswer, host, &port)) {
    nameserver_error( "No sensible data from nameserver: '%s'.\n", nsanswer);
    return FALSE;
  }

  if (!setup_address(port, addr, host)) {
    nameserver_error("Failed to setup address\n");
    return FALSE;
  }

  return TRUE;
}

int do_get(int sock, char *table, char *key, char *data, struct sockaddr_in *extdest) {
  struct sockaddr_in dest;	/* Temporary address */
  int status;

  if (!extdest) {
    if (!lookup_table(sock, table, &dest)) {
      exit(1);
    }
  } else memcpy(&dest, extdest, sizeof(struct sockaddr_in));

  data[0] = '\0';
  status = send_cmd(sock, &dest, GET, table, key, data);
  switch (status) {
  case INTERNAL_ERROR:
    exit(1);
  }
  return status;
}

int do_del(int sock, char *table, char *key, char *data, struct sockaddr_in *extdest) {
  struct sockaddr_in dest;	/* Temporary address */
  int status;

  if (!extdest) {
    if (!lookup_table(sock, table, &dest)) {
      exit(1);
    }
  } else memcpy(&dest, extdest, sizeof(struct sockaddr_in));

#ifdef BOGUS_SECURITY
  tmpdata[0] = '\0';
  status = send_cmd(sock, &dest, GET, table, key, tmpdata);
  switch (status) {
  case INTERNAL_ERROR:
    exit(1);
  case EXISTS:
    break;			/* Do nothing */
  case NONEXISTENT:
  case ERROR:
    return status;
  default:
    return status;
  }
#endif
  {
    int reqcnt = reqno;
    data[0] = '\0';
    status = send_cmd(sock, &dest, DEL, table, key, data);
    switch (status) {
    case NONEXISTENT:
      if (reqno > (reqcnt+2)) {
	return EXISTS;
      }
      else return NONEXISTENT;
    case EXISTS:
      return EXISTS;
    case INTERNAL_ERROR:
      exit(1);
    }
  }

  return status;
}

int do_put_rpl(int sock, char *table, char *key, char *data, struct sockaddr_in *extdest, request op) {
  struct sockaddr_in dest;	/* Temporary address */
  int status;
  char checkdata[DATALEN];
  if (!extdest) {
    if (!lookup_table(sock, table, &dest)) {
      exit(1);
    }
  } else memcpy(&dest, extdest, sizeof(struct sockaddr_in));

#ifdef BOGUS_SECURITY
  if (op == PUT) {
    tmpdata[0] = '\0';		/* Null-terminate string */

    status = send_cmd(sock, &dest, GET, table, key, tmpdata);
    switch (status) {
    case INTERNAL_ERROR:
      exit(1);
    case NONEXISTENT:
      break;			/* Do nothing */
    case EXISTS:
    case ERROR:
      strncpy(data, tmpdata, DATALEN);
      return status;
    default:
      return status;
    }
  }
#endif

  strncpy(checkdata, data, DATALEN);

  status = send_cmd(sock, &dest, op, table, key, data);
  switch (status) {
  case INTERNAL_ERROR:
    exit(1);
  case EXISTS:
    break;			/* Do nothing */
  case REPLACED:
    if (op != RPL) return ERROR; /* Can't get this unless op is RPL */
  case NONEXISTENT:
  case ERROR:
    return status;
  default:
    return status;
  }

#ifdef BOGUS_SECURITY
  /* Can't land here unless op == PUT */
  tmpdata[0] = '\0';		/* Null-terminate string */
  status = send_cmd(sock, &dest, GET, table, key, tmpdata);
  switch (status) {
  case INTERNAL_ERROR:
    exit(1);
  case EXISTS:
    debug_out(0, "Checking '%s' == '%s'\n", checkdata, tmpdata);
    if (!strncmp(checkdata, tmpdata, DATALEN)) { /* Same as we put there? */
      return NONEXISTENT;	/* Due to semantics of PUT */
    } else return EXISTS;	/* I.e we failed because data was already present. */
  case NONEXISTENT:
    return EXISTS;
  case ERROR:
    strncmp(data, tmpdata, DATALEN);
    return status;
  }
#endif
  return status;
}

/* Handle alarms */
void
handle_alarm(int sig) {
}

int send_cmd(int sock, struct sockaddr_in *dest, request req, char *table, char *key, char *data) {
  message reqmsg, replymsg;
  int retries;
  int fromlen;
  struct sockaddr_in from;
  int id = gen_id();		/* Generate unique ID for this transaction */
 
  /* Create message */
  strncpy(reqmsg.Table, table, TABLENAMELEN);
  reqmsg.Table[TABLENAMELEN-1] = '\0';
  reqmsg.Request = req;
  reqmsg.Reply = NONE;
  strncpy(reqmsg.Key, key, KEYLEN);
  reqmsg.Key[KEYLEN-1] = '\0';
  strncpy(reqmsg.Data, data, DATALEN);
  reqmsg.Data[DATALEN-1] = '\0';
  reqmsg.Id = id;

  for (retries = 0; retries < RETRIES; retries++) {
    int res;
    int timeout, ok;

    /* Set cookie */
    strncpy(reqmsg.Cookie, COOKIE, COOKIELEN);
    reqmsg.Cookie[COOKIELEN-1] = '\0';

    /* Any cookies for this send? */
    while (base && base->req < reqno) {
      cookies *tmp = base;
      debug_out(0, "Dumping %d (%s)\n", base->req, base->cookie);
      base = base->next;
      free(tmp);
    }

    if (base && base->req == reqno) {
      cookies *tmp = base;
      strncat(reqmsg.Cookie, base->cookie, COOKIELEN-strlen(reqmsg.Cookie));
      base = base->next;
      free(tmp);
    }

    reqmsg.Cookie[COOKIELEN-1] = '\0';

    reqno++;

    debug_out(1,"Sending  %s(%d): {%s}\n", straddr(dest), ntohs(dest->sin_port), print_message(&reqmsg));

    res = sendto(sock, (char *)&reqmsg, sizeof(message), 0,
		 (struct sockaddr *)dest, sizeof(struct sockaddr_in));

    if (res < 0) {
      if (errno == ECONNREFUSED) break;
      internal_error("sendto(): %s\n", strerror(errno));
      return INTERNAL_ERROR;
    } else if (res != sizeof(message)) {
      internal_error("Sent bogus size message\n");
      return INTERNAL_ERROR;
    }

    timeout = FALSE;
    do {
      ok = TRUE;

#ifdef LINUX
      if (signal(SIGALRM, &handle_alarm) == SIG_ERR) { /* setup signal handler */
	internal_error("signal(): %s\n", strerror(errno));
	exit(1);
      }
#else
      if (sigset(SIGALRM, &handle_alarm) == SIG_ERR) { /* setup signal handler */
	internal_error("signal(): %s\n", strerror(errno));
	exit(1);
      }
#endif

      alarm(TIMEOUT);
      fromlen = sizeof(struct sockaddr_in);
      errno = 0;
      res = recvfrom(sock, (char *)&replymsg, sizeof(message), 0,
		     (struct sockaddr *)&from, &fromlen);
      alarm(0);

      if (res < 0) {
	if (errno == ECONNREFUSED) {
	  debug_out(0, "recvfrom(): %s. (treated like a timeout)\n", strerror(errno));
	  timeout = TRUE;
	  continue;
	} else if (errno == EINTR) {
	  debug_out(0, "Read operation timed out.\n");
	  timeout = TRUE;
	  continue;
	}
	internal_error("recvfrom(): %s\n", strerror(errno));
	return INTERNAL_ERROR;
      } else if (res < sizeof(message)) {
	ok = FALSE;
	debug_out(0,"Incoming message too short.\n");
	continue;
      }
      debug_out(1,"Received %s(%d): {%s}\n", straddr(&from), ntohs(from.sin_port), print_message(&replymsg));

      if (replymsg.Id != id || strncmp(replymsg.Cookie, COOKIE, strlen(COOKIE)) ||
	  strncmp(replymsg.Table, reqmsg.Table, TABLENAMELEN) ||
	  replymsg.Request != reqmsg.Request ||
	  strncmp(replymsg.Key, reqmsg.Key, KEYLEN)) {
	debug_out(0,"Bogus reply.\n");
	ok = FALSE;
      }

    } while (!ok);

    if (timeout) continue;

    reqmsg.Reply = ACK;	/* Set ACK status */

    /* Set cookie */
    strncpy(reqmsg.Cookie, COOKIE, COOKIELEN);
    reqmsg.Cookie[COOKIELEN-1] = '\0';

    /* Any cookies for this send? */
    while (base && base->req < reqno) {
      cookies *tmp = base;
      debug_out(0, "Dumping %d (%s)\n", base->req, base->cookie);
      base = base->next;
      free(tmp);
    }

    if (base && base->req == reqno) {
      cookies *tmp = base;
      strncat(reqmsg.Cookie, base->cookie, COOKIELEN-strlen(reqmsg.Cookie));
      base = base->next;
      free(tmp);
    }

    reqmsg.Cookie[COOKIELEN-1] = '\0';

    reqno++;

    debug_out(1,"Sending  %s(%d): {%s}\n", straddr(dest), ntohs(dest->sin_port), print_message(&reqmsg));

    if (delay_ack) {
      debug_out(0, "Delaying ack for %d seconds\n", delay_ack);
      sleep(delay_ack);
    }

    res = sendto(sock, (char *)&reqmsg, sizeof(message), 0,
		 (struct sockaddr *)dest, sizeof(struct sockaddr_in));

    if (res < 0) {
      if (errno == ECONNREFUSED) break;
      internal_error("sendto(): %s\n", strerror(errno));
      return INTERNAL_ERROR;
    } else if (res != sizeof(message)) {
      internal_error("Sent bogus size message\n");
      return INTERNAL_ERROR;
    }

    strncpy(data, replymsg.Data, DATALEN);
    replymsg.Data[DATALEN-1] = '\0';
    return replymsg.Reply;
  }
  return TIMEOUT_ERROR;
}

int send_reply(char *cookie, char *table, request req, reply rep, char *key, char *data, long id,
	  int sock, struct sockaddr_in *dest) {
  message reqmsg, replymsg;
  struct sockaddr_in from;
  int retries;
  int fromlen;
  int res;
 
  for (; junk > 0 && reply_loops > 0; junk--, reply_loops--) {
    message gmsg;
    char *p = (char *)&gmsg;
    int i;
    
    for (i = 0; i < sizeof(gmsg); i++) {
      p[i] = lrand48()%32 + '@';
    }
    
    debug_out(1,"Sending garbled message %s(%d)\n", straddr(dest), ntohs(dest->sin_port));
    
    res = sendto(sock, (char *)&gmsg, sizeof(message), 0,
		 (struct sockaddr *)dest, sizeof(struct sockaddr_in));
  }

  /* Create message */
  strncpy(reqmsg.Table, table, TABLENAMELEN);
  reqmsg.Table[TABLENAMELEN-1] = '\0';
  reqmsg.Request = req;
  reqmsg.Reply = rep;
  strncpy(reqmsg.Key, key, KEYLEN);
  reqmsg.Key[KEYLEN-1] = '\0';
  strncpy(reqmsg.Data, data, DATALEN);
  reqmsg.Data[DATALEN-1] = '\0';
  reqmsg.Id = id;

  for (retries = 0; retries < RETRIES; retries++) {
    int res;
    int timeout, ok;

    /* Set cookie */
    strncpy(reqmsg.Cookie, cookie, COOKIELEN);
    reqmsg.Cookie[COOKIELEN-1] = '\0';

    /* Any cookies for this send? */
    while (base && base->req < reqno) {
      cookies *tmp = base;
      debug_out(0, "Dumping %d (%s)\n", base->req, base->cookie);
      base = base->next;
      free(tmp);
    }

    if (base && base->req == reqno) {
      cookies *tmp = base;
      strncat(reqmsg.Cookie, base->cookie, COOKIELEN-strlen(reqmsg.Cookie));
      base = base->next;
      free(tmp);
    }

    reqmsg.Cookie[COOKIELEN-1] = '\0';

    reqno++;

    for (; reply_loops > 1; reply_loops--) { /* Produce duplicates */
      debug_out(1,"Sending duplicate %s(%d): {%s}\n", straddr(dest), ntohs(dest->sin_port),
		print_message(&reqmsg));
      res = sendto(sock, (char *)&reqmsg, sizeof(message), 0,
		   (struct sockaddr *)dest, sizeof(struct sockaddr_in));
    }
    if (reply_loops > 0) {
      debug_out(1,"Sending  %s(%d): {%s}\n", straddr(dest), ntohs(dest->sin_port), print_message(&reqmsg));
      res = sendto(sock, (char *)&reqmsg, sizeof(message), 0,
		   (struct sockaddr *)dest, sizeof(struct sockaddr_in));

      if (res < 0) {
#if 0
	if (errno == ECONNREFUSED) break;
#endif
	internal_error("%s\n", strerror(errno));
	return FALSE;
      } else if (res != sizeof(message)) {
	internal_error("Sent bogus size message\n");
	return FALSE;
      }
    } else {
      reply_loops = 1;
    }
    timeout = FALSE;
    do {
      ok = TRUE;

#ifdef LINUX
      if (signal(SIGALRM, &handle_alarm) == SIG_ERR) { /* setup signal handler */
	internal_error("signal(): %s\n", strerror(errno));
	exit(1);
      }
#else
      if (sigset(SIGALRM, &handle_alarm) == SIG_ERR) { /* setup signal handler */
	internal_error("signal(): %s\n", strerror(errno));
	exit(1);
      }
#endif

      alarm(ACK_TIMEOUT);
      fromlen = sizeof(struct sockaddr_in);
      errno = 0;
      res = recvfrom(sock, (char *)&replymsg, sizeof(message), 0,
		     (struct sockaddr *)&from, &fromlen);
      alarm(0);

      if (res < 0) {
	if (errno == ECONNREFUSED) {
	  debug_out(0, "recvfrom(): %s. (treated like a timeout)\n", strerror(errno));
	  timeout = TRUE;
	  continue;
	} else if (errno == EINTR) {
	  debug_out(0, "Read operation timed out.\n");
	  timeout = TRUE;
	  continue;
	}
	internal_error("recvfrom(): %s\n", strerror(errno));
	return INTERNAL_ERROR;
      } else if (res < sizeof(message)) {
	ok = FALSE;
	debug_out(0,"Incoming message too short.\n");
	continue;
      }
      debug_out(1,"Received %s(%d): {%s}\n", straddr(&from), ntohs(from.sin_port), print_message(&replymsg));

      if (replymsg.Id != id || strncmp(replymsg.Cookie, COOKIE, strlen(COOKIE)) ||
	  strncmp(replymsg.Table, reqmsg.Table, TABLENAMELEN) ||
	  replymsg.Request != reqmsg.Request ||
	  strncmp(replymsg.Key, reqmsg.Key, KEYLEN)) {
	debug_out(0, "Bogus ack - requeuing message.\n");
	add_to_msglist(&replymsg, &from); /* Requeue */
	ok = FALSE;
      }

    } while (!ok);

    if (!timeout && replymsg.Reply == ACK) return TRUE;
  }
  return FALSE;
}

int receive_msg(int sock, message *msg, struct sockaddr_in *from, int *fromlen) {
  int res;

  if (basemsg) {
    struct msglist *tmp = basemsg;

    debug_out(0, "Getting message off queue.\n");
    memcpy(msg, &basemsg->msg, sizeof(message));
    memcpy(from, &basemsg->from, sizeof(struct sockaddr_in));
    *fromlen = sizeof(struct sockaddr_in);
    basemsg = basemsg->next;
    free(tmp);
    return sizeof(message);
  }

  *fromlen = sizeof(struct sockaddr_in);
  res = recvfrom(sock, (char *)msg, sizeof(message), 0,
		 (struct sockaddr *)from, fromlen);
  if (res < 0) {
    if (errno == ECONNREFUSED) {
      debug_out(0, "recvfrom(): %s (ignored)\n", strerror(errno));
      return res;
    }
    my_perror(2,"recvfrom()");
    exit(1);
  } else if (*fromlen != sizeof(struct sockaddr_in)) {
    debug_out(1,"Incoming address of unexpected size. (%d vs %d)\n", *fromlen,
	      sizeof(struct sockaddr_in));
    return res;
  }
  return res;
}


int do_rpl(int sock, char *table, char *key, char *data, struct sockaddr_in *extdest) {
  return do_put_rpl(sock, table, key, data, extdest, RPL);
}

int do_put(int sock, char *table, char *key, char *data, struct sockaddr_in *extdest) {
  return do_put_rpl(sock, table, key, data, extdest, PUT);
}

char *
print_message(message *msg) {
  static char buf[4096];

  msg->Table[TABLENAMELEN-1] = '\0';
  msg->Key[KEYLEN-1] = '\0';
  msg->Data[DATALEN-1] = '\0';
  msg->Cookie[COOKIELEN-1] = '\0';

  sprintf(buf, "\"%s\", \"%s\", %d, %d, \"%s\", \"%s\", %ld",
          msg->Cookie, msg->Table, msg->Request, msg->Reply, 
          msg->Key, msg->Data, (long int)ntohl(msg->Id));
  return buf;
}
