#include "message.h"
#include "nameserver.h"

#define INTERNAL_ERROR -1
#define TIMEOUT_ERROR -666

/* Keep a sorted list of cookies  */
struct _cookies {
  struct _cookies *next;
  int req;
  char *cookie;
};

typedef struct _cookies cookies;

#ifdef MSG
cookies *base = NULL;
#else
extern cookies *base;
#endif

cookies *new_cookie(void);
cookies *insert_cookie(char *cookie, int req, cookies *first);
long gen_id(void);
int client_error(char *fmt, ...);
int nameserver_error(char *fmt, ...);
int extract_host_address(char *buf, char *host, int *port);
int lookup_table(int sock, char *table, struct sockaddr_in *addr);
int do_get(int sock, char *table, char *key, char *data, struct sockaddr_in *extdest);
int do_del(int sock, char *table, char *key, char *data, struct sockaddr_in *extdest);
int do_put(int sock, char *table, char *key, char *data, struct sockaddr_in *extdest);
int do_rpl(int sock, char *table, char *key, char *data, struct sockaddr_in *extdest);
int send_cmd(int sock, struct sockaddr_in *dest, request req, char *table, char *key, char *data);
int send_reply(char *cookie, char *table, request req, reply rep, char *key, char *data, long id,
	  int sock, struct sockaddr_in *dest);
char *print_message(message *msg);
void junk_next(int i);
void reply_copies(int copies);
int receive_msg(int sock, message *msg, struct sockaddr_in *from, int *fromlen);
void set_delay_ack(int sec);

