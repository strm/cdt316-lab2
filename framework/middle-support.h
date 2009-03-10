#ifndef SUPPORT_H

#define MWPREFIX "MIDDLEWARE"
#define DBPREFIX "DATABASE"
#define MANAGEMENT "MANAGEMENT"
    
/* Debugging output functions */

/* All messages with this, or higher, severity will be printed out */
int set_severity(int new);
/* Mimics perror(), but with severity handling */
void my_perror(int level, char *str);
/* Mimics fprintf(stderr, char *fmt, ...), but with severity handling */
int debug_out(int level, char *fmt, ...);

/* Identifying objects
   These functions only do a syntactical test. I.e., is_entry() will not tell
   you if a given string is an entry that exists, just that the string
   syntactically is a possible name of an entry.
 */
int is_literal(char *arg);	/* Returns true if arg is a literal */
int is_entry(char *arg);	/* Returns true if arg is an entry */

/*  Functions to get, replace and delete entries in a database.
    Functions false for failure, true otherwise

    get_entry() fetches data from the specified entry in the specified database

    Arguments for get_entry() are:

    str		- Data area to put answer in.
    database	- Database to query
    entry	- Entry in database to query

    Function returns false if entry nonexistent or could not be retrieved.

    replace_entry() replaces, or inserts, data for a given database/entry
    
    Arguments for replace_entry() are:

    str		- String to enter into the database
    database	- Database to insert data into
    entry	- Entry in database to insert data into

    Function returns false if entry could not be inserted/replaced.

    delete_entry() erases the specified entry from the specified database

    Arguments for delete_entry() are:

    database	- Database to delete entry from
    entry	- Entry in database to delete

    Function returns false if entry nonexistent or could not be deleted.

 */
int get_entry(char *str, char *database, char *entry);
int replace_entry(char *str, char *database, char *entry);
int delete_entry(char *database, char *entry);

/* Handling of magic cookies */
int magic(command *cmd, response *rsp);

/* Start/stop middleware */
int start_middleware_frontend(char *database);
void stop_middleware_frontend(int sock);

#define SUPPORT_H
#endif
