#ifndef CMD_H

/* A command is basically a three address code where numerical arguments are
   literals and alphanumerical arguments are database entries.

   Example: 12   -> literal
            -2   -> literal
	    ABBA -> database entry           
	    A7   -> database entry
	    2A   -> database entry (but leading digits are discouraged)

   Operators are:

   <ASSIGN, arg1, arg2>  ->  Assigns database entry arg1 the value of arg2
   Example: ASSIGN FOO 666 ->  Assigns the database entry 'FOO' the value 666
            ASSIGN FOO BAR ->  Assigns the database entry 'FOO' the value of the
                  	        database entry 'BAR' (fails if BAR nonexistent)
            ASSIGN 666 BAR ->  Illegal, arg1 must be a database entry.

   <ADD, arg1, arg2, arg3>   ->  Assigns database entry arg1 the sum of the
                                 values arg2 and arg3. Semantics of arg1 are
				 identical to ASSIGN (e.g must be a db entry)
   Example: ADD FOO 666 4711 ->  Assigns the database entry 'FOO' the value of
                                 5377
            ADD FOO FOO 1    ->  Assigns database entry FOO the value of FOO,
                         	 incremented by one. Fails if FOO nonexistent.

   <PRINT, arg1>             ->  Causes the value of arg1 to be inserted into
                                 the answer. Arg1 can be a literal or db entry.
   Example: PRINT FOO        -> Puts the value of FOO into the response.


   <DELETE, arg1>             ->  Delete entry arg1 from database.

   Example: DELETE FOO        ->  Removes entry FOO from database.
            DELETE 4711       ->  Failure, because arg1 must be an entry.


   <SLEEP, arg1>             ->  Causes the middleware to pause for arg1
                                 seconds, arg1 must be a (positive) literal. 
   Example: SLEEP 5          ->  Causes the middleware to call sleep() with the
                                 argument 5.

   <IGNORE>                  ->  Ignore error return from next statement. This
                                 is useful if you know that an error might
				 occur as a result of the following operation.
                                 Error message will be suppressed.

   Example: IGNORE
            DELETE FOO       ->  Execution will not abort or report errors even
                                 if FOO does not exist.


   <MAGIC, arg1, arg2, arg3> ->  Passes a magic cookie from the client to the
				 the middleware and beyond.

				 Magic cookies with an arg1 of "0" is intended
				 for the middleware. The special case where
				 arg1, arg2 and arg3 are all "0" means that the
				 middleware should report current UID, PID and
				 version. Other values of arg2 and arg3 can be
				 assigned at the middlewares own discretion.

				 Other values of arg1 means that it should be
				 passed on directly to the backend via the
				 magic() function call an without any further
				 processing in the middleware.


   Example: MAGIC 1 0 0      ->  Causes the middleware to call magic() with the
                                 complete command structure as argument.

            MAGIC 0 0 0      ->  The response should contain the UID, PID and
                     	         version of the program separated with spaces.
				 The format of the version is user defined, but
				 care should be taken so that the total length
				 of the string does not exceed ARG_SIZE.
				 E.g. "5033  4711   MidWare v1.42"

            MAGIC 0 1 1      ->  User defined.

   <QUIT>             -> Causes the middleware to terminate.

   Example: QUIT      -> Terminates the middleware.


 */

#define ARG_SIZE 64		/* Size of an argument/result string */

#define USER_COOKIE "0"		/* A user cookie */

typedef enum {NOCMD = 0, ASSIGN = 1, ADD = 2, PRINT = 3, DELETE = 4, SLEEP = 5,
	      IGNORE = 6, MAGIC = 666, QUIT = 4711
} operator;

struct _command {
  operator op;			/* Operator */
  long seq;			/* Sequence number of command */
  char arg1[ARG_SIZE];		/* Arguments... */
  char arg2[ARG_SIZE];
  char arg3[ARG_SIZE];
};

typedef struct _command command;

struct _response {
  long seq;			/* Sequence number (matches command) */
  char is_message;		/* Indicates if any result was returned */
  char is_error;		/* This is an error message */
  char result[ARG_SIZE];
};

typedef struct _response response;

#define CMD_H
#endif
