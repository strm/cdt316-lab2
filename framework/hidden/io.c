#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/utsname.h>
#include "io.h"

char *get_host_name() {
#define HNAMELEN 256
  static char hostname[HNAMELEN];
  struct utsname foo;
  struct hostent *ent1;
  if (uname(&foo) < 0) {
    perror("uname()");
    exit(1);
  }
  ent1 = gethostbyname(foo.nodename);
  if (!ent1) exit(1);
  strncpy(hostname, ent1->h_name, HNAMELEN);
  hostname[HNAMELEN-1] = '\0';
#ifndef LOCAL_VERSION
  return hostname;
#else
  return "localhost";
#endif
}

int setup_socket(int protocol) {
  int mysock;
  struct protoent *pent;
  /* Create a socket */
  if (protocol == TCP) {
    pent = getprotobyname("tcp");
    if (!pent) {
      debug_out(0, "Failed to get protocol\n");
      return -1;
    }
    mysock = socket(PF_INET, SOCK_STREAM, pent->p_proto);
  } else if (protocol == UDP) {
    pent = getprotobyname("udp");
    if (!pent) {
      debug_out(0, "Failed to get protocol\n");
      return -1;
    }
    mysock = socket(PF_INET, SOCK_DGRAM, pent->p_proto);
  } else {
    debug_out(0, "No protocol specified.\n");
    return -1;
  }
  if (mysock < 0) {
    my_perror(0,"Socket");
    return -1;
  }
  debug_out(0, "Setup %s socket.\n", (protocol == UDP ? "UDP" : "TCP"));

  return mysock;
}

int setup_address(int portno, struct sockaddr_in *address, char *hostname) {

  address->sin_family = AF_INET;
  address->sin_port = htons(portno);

  if (hostname) {
    struct hostent *host;

    host = gethostbyname(hostname);
    if (!host) {
      debug_out(0,"Unknown host: %s\n", hostname);
      return FALSE;
    }
    memcpy((char *) &address->sin_addr, (char *)host->h_addr, host->h_length);
  } else {
    address->sin_addr.s_addr = INADDR_ANY;
  }
  debug_out(0, "sockaddr setup for port %d address %s\n", ntohs(address->sin_port),
	  straddr(address));
  return TRUE;
}

int setup_port(unsigned long portno, int protocol, int reuse) {
  struct sockaddr_in address;
  int mysock;
  int sockopt = 1;

  if ((mysock = setup_socket(protocol)) < 0) return -1;

  if (!setup_address(portno, &address, NULL)) return -1;

  /* re-use the port number (if not still in use) */

  if (reuse) {
    if (setsockopt(mysock,SOL_SOCKET,SO_REUSEADDR, (char *)&sockopt,
		   (int) sizeof(int)) < 0) {
      my_perror(0,"create_server: Could not reuse the socket");
      (void) close(mysock);
      return -1;
    }
  }

  /* Bind address to socket */
  if (bind(mysock, (struct sockaddr *) &address, sizeof(address)) < 0) {
    my_perror(0,"Bind");
    return -1;
  }
  if (protocol == TCP) {
    /* Accept incoming calls */
    if (listen(mysock,1) < 0) {
      my_perror(0,"Listen");
      return -1;
    }
  }

  return mysock;
}

char *stream_gets(char *buf, int buflen, int stream) {
  char tmpbuf = '\0';
  char *p = buf;
  int res;
  while ((p - buf) < buflen && tmpbuf != '\n') {
    if ((res = read(stream, &tmpbuf, 1 )) != 1) {
      if (res < 0) {
	my_perror(0,"read()");
      }
      return NULL;
    } else *p++ = tmpbuf;
    if (tmpbuf == '\r') p--;
  }
  *p = '\0';
  return buf;
}

int stream_printf(int stream, char *fmt, ...) {
  int res, tmpres;
  va_list ap;
  char bigstring[BIGSTRING];	/* Assumed to be big enough... */

  va_start(ap, fmt);
  res = vsprintf(bigstring, fmt, ap);
  tmpres = write(stream, bigstring, strlen(bigstring));
  if (tmpres < strlen(bigstring)) {
    debug_out(0,"Unable to write to stream.\n");
    if (tmpres < 0) {
      res = -1;
    }
  }
  va_end(ap);
  return res;
}

int open_conn(char *hostname, int portno, int protocol) {
  int mysock;
  struct sockaddr_in address;

  if ((mysock = setup_socket(protocol)) < 0) return -1;
 
  if (!setup_address(portno, &address, hostname)) return -1;

  if (connect(mysock, (struct sockaddr *) &address, sizeof(struct sockaddr_in)) < 0) {
    my_perror(0,"Connect");
    return -1;
  }
#if 0
  if (protocol == UDP) {
    if (!setup_address(0, &other, NULL)) return -1;
    if (bind(mysock, (struct sockaddr *) &other, sizeof(struct sockaddr_in)) < 0) {
      my_perror(0,"bind()");
      return -1;
    }
  }
#endif
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    my_perror(0,"Signal");
    return -1;
  }
  return mysock;
}

static int severity = 0;	/* How severe errors should be reported */
				/* 0 = informational */
				/* 1 = warning/important */
				/* 2 = causes abort */

/* Sets new severity and returns the old value */
int set_severity(int new) {
  int tmp = severity;

  severity = new;
  return tmp;
}

int debug_out(int level, char *fmt, ...) {
  int res;
  va_list ap;
  time_t now;
  char *time_str;

  if (level < severity) return 0; /* Don't print this message. */

  time(&now);
  time_str = ctime(&now);
  if (time_str[strlen(time_str) - 1] == '\n') time_str[strlen(time_str) - 1] = '\0';

  va_start(ap, fmt);
  fprintf(stderr, "%s: ", time_str);
  res = vfprintf(stderr, fmt, ap);
  if (res < 0) {
    res = -1;
  }
  va_end(ap);
  return res;
}

void my_perror(int level, char *str) {
  time_t now;
  char *time_str;
  char *errtxt;
  char localerror[] = "Unknown error, strerror() failed.";

  if (level < severity) return; /* Don't print this message. */

  errtxt = strerror(errno);
  if (!errtxt) {
    errtxt = localerror;
  }
  time(&now);
  time_str = ctime(&now);
  if (time_str[strlen(time_str) - 1] == '\n') time_str[strlen(time_str) - 1] = '\0';
  fprintf(stderr, "%s: %s: %s\n", time_str, str, errtxt);
}

#define ADDRBUF 80
char *straddr(struct sockaddr_in *addr) {
  int res;
  static char address[ADDRBUF];
  res = sprintf(address, "%d.%d.%d.%d",
		((unsigned char *)(&addr->sin_addr.s_addr))[0],
		((unsigned char *)(&addr->sin_addr.s_addr))[1],
		((unsigned char *)(&addr->sin_addr.s_addr))[2],
		((unsigned char *)(&addr->sin_addr.s_addr))[3]);
  if (res > ADDRBUF - 1) {
    debug_out(2, "Buffer overrun!\n");
    exit(1);
  }
  return address;
}
