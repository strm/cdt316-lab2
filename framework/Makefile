CC = gcc
#
# Note: If you're running a local version, you need to set NAMESERVER in 
#       nameserver.h to a machine running your central
#       nameserver (basically a database server holding the database "nameserver",
#       not to be confused with DNS)

CFLAGS = -pipe -Wall -g -DLINUX -I. -I../. -Ihidden/.


OBJSERVER = hidden/server.o hidden/io.o hidden/db.o hidden/msg.o
OBJMIDDLE = middle.o hidden/middle-support.o hidden/io.o hidden/msg.o
OBJFRONTEND = hidden/frontend.o hidden/io.o hidden/msg.o

# Linux link libraries
LIBS = -L. -lgdbm -lgdbm_compat

PROGRAMS = dbserver middle frontend

all: ${PROGRAMS}

clean:
	rm -f ${OBJMIDDLE} ${OBJFRONTEND} ${OBJSERVER} ${PROGRAMS} *.pag *.dir

dbserver: ${OBJSERVER}
	${CC} -o $@ ${OBJSERVER} ${LFLAGS} ${LIBS}

middle: ${OBJMIDDLE}
	${CC} -o $@ ${OBJMIDDLE} ${LFLAGS} ${LIBS}

frontend: ${OBJFRONTEND}
	${CC} -o $@ ${OBJFRONTEND} ${LFLAGS} ${LIBS}

hidden/server.o: hidden/server.c  hidden/message.h hidden/io.h hidden/db.h nameserver.h hidden/msg.h cmd.h middle-support.h Makefile

middle.o: middle.c middle-support.h cmd.h Makefile

hidden/middle-support.o: hidden/middle-support.c middle-support.h cmd.h nameserver.h hidden/io.h hidden/msg.h Makefile

hidden/frontend.o: hidden/frontend.c hidden/io.h nameserver.h hidden/msg.h Makefile hidden/io.h Makefile

hidden/db.o: hidden/db.c hidden/db.h Makefile

hidden/io.o: hidden/io.c hidden/io.h Makefile

hidden/msg.h: hidden/message.h

hidden/msg.o: hidden/msg.c hidden/msg.h hidden/message.h Makefile hidden/io.h

hidden/userver.o: hidden/userver.c hidden/io.h Makefile
