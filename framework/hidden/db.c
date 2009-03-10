#include <string.h>
#include "db.h"

/* This is the database front-end. */

/* Open the database, returns a database-pointer */
DBM *mydbopen(char *name) {
  DBM *fd;

  fd = dbm_open(name, O_CREAT | O_RDWR, 0666);
#if 0
  if (!fd) {
    my_perror(2,"dbm_open failed in dbopen");
    exit(1);
  }
#endif
  return fd;
}

/* Close the database */
void mydbclose(DBM *fd) {
  dbm_close(fd);
}

/* Delete an entry */
int mydbdelete(DBM *fd, char *key) {
  datum dat;
  int res;
  dat.dptr = key;
  dat.dsize = strlen(key);
  res = dbm_delete(fd, dat);
#if 0
  if (dbm_error(fd)) {
    debug_out(2, "Error deleting entry.\n");
    exit (1);
  }
#endif
#ifdef DEBUG
  if (res != 0)
    debug_out(0,"Failed to delete entry - didn't exist?\n");
  else
    debug_out(0,"Deleted entry\n", dat.dsize);
#endif
  return res;
}

/* Fetch an entry */
char *mydbfetch(DBM *fd, char *key) {
  static char buf[MAXSTR];
  int datlen;
  datum dat,res;
  dat.dptr = key;
  dat.dsize = strlen(key);
  res = dbm_fetch(fd, dat);
#if 0
  if (dbm_error(fd)) {
    debug_out(2, "Error fetching entry.\n");
    exit (1);
  }
#ifdef DEBUG
  else {
    debug_out(0,"Got %d databytes\n", res.dsize);
  }
#endif
#endif
  if (res.dptr == NULL) return NULL;
  datlen = (res.dsize >= MAXSTR? MAXSTR - 1: res.dsize);
  memcpy(buf,res.dptr,datlen);
  buf[datlen] = '\0';
  return buf;
}

/* Insert an entry */
int mydbinsert(DBM *fd, char *key, char *data) {
  datum ke,dat;
  int res;
  dat.dptr = data;
  dat.dsize = strlen(data);
  ke.dptr = key;
  ke.dsize = strlen(key);
  res = dbm_store(fd, ke, dat, DBM_INSERT);
#if 0
  if (dbm_error(fd)) {
    debug_out(2, "Error inserting entry.\n");
    exit (1); 
  }
#ifdef DEBUG
  if (res != 0)
    debug_out(0,"Failed to insert entry - already present?\n");
  else
    debug_out(0,"Inserted %d databytes\n", dat.dsize);
#endif
#endif
  return res;
}

/* Insert an entry */
int mydbreplace(DBM *fd, char *key, char *data) {
  datum ke,dat;
  int res;
  dat.dptr = data;
  dat.dsize = strlen(data);
  ke.dptr = key;
  ke.dsize = strlen(key);
  res = dbm_store(fd, ke, dat, DBM_REPLACE);
#if 0
  if (dbm_error(fd)) {
    debug_out(2, "Error inserting entry.\n");
    exit (1); 
  }
#ifdef DEBUG
  if (res != 0)
    debug_out(0,"Failed to insert entry\n");
  else
    debug_out(0,"Inserted %d databytes\n", dat.dsize);
#endif
#endif
  return res;
}
