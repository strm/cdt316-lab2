#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ndbm.h"
#include <stdio.h>
#include <errno.h>

#define MAXSTR 1024

/* DBM is the type of the database file descriptor. It is used very similarily
   to FILE file descriptor used with normal stdio functions. */

/* Open database file, returns NULL on failure */
DBM *mydbopen(char *name);

/* Close database file */
void mydbclose(DBM *fd);

/* Delete key from selected database. Returns 0 on success */
int mydbdelete(DBM *fd, char *key);

/* Get key from selected database. Returns NULL on failure.
   Note that the pointer returned is only valid until the next call to
   mydbfetch() */
char *mydbfetch(DBM *fd, char *key);

/* Insert key,data pair into selected database. Returns 0 on success */
int mydbinsert(DBM *fd, char *key, char *data);

/* Insert key,data pair into selected database. Returns 0 on success */
int mydbreplace(DBM *fd, char *key, char *data);
