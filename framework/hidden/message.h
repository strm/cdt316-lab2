#ifndef MESSAGE_H

#define COOKIELEN 32		/* Max length of a cookie  */
#define TABLENAMELEN 32		/* Max length of database name */
#define KEYLEN 32		/* Max length of a key */
#define DATALEN 768		/* Max length of data */

#define TIMEOUT 5
#define ACK_TIMEOUT 3
#define RETRIES 4

#define COOKIE  "PROT-DISTSYS1"		/* The cookie identifying this specific protocol */

typedef unsigned char request;	/* Define type for requests */
typedef unsigned char reply; /* Define type for responses */

typedef struct {
  char Cookie[COOKIELEN];	/* Cookie */
  char Table[TABLENAMELEN];	/* Database table being accessed */
  request Request;		/* Request type */
  reply Reply;			/* Reply code */
  char Key[KEYLEN];		/* The key field */
  char Data[DATALEN];		/* The data field */
  long Id;			/* Id number (use network byteorder) */
} message;

#define GET 1		/* "Get data" operation for database */
#define PUT 2		/* "Put data" operation for database */
#define DEL 3		/* "Delete data" operation for database */
#define RPL 4		/* "Replace data" operation for database */

#define NONE		0	/* No response code set */
#define EXISTS		1	/* The requested key exists */
#define NONEXISTENT	2	/* The requested key does not exist */
#define REPLACED        3	/* The requested key was replaced */
#define ERROR		4	/* An error occured */
#define ACK		5	/* Acknowledgement of response */

#ifndef NOBOOLEANS
/* Define NOBOOLEANS to avoid having them defined here. */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif /* NOBOOLEANS */

#define MESSAGE_H
#endif /* MESSAGE_H */
