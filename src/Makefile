#make file for pthread communcation testing

PROG 	= middleware
OBJS	= msg_queue.o global.o main.o lock.o trans.o
CC 	= gcc
LD 	= gcc
#CFLAGS  = -pipe -Wall -g -DLINUX -I. -I../framework -I../framework/hidden
LIBS	= -L. -lgdbm -lpthread
CFLAGS 	= -c -Wall -I. -I../framework -I../framework/hidden

OBJMIDDLE = ../framework/middle.o ../framework/hidden/middle-support.o ../framework/hidden/io.o ../framework/hidden/msg.o parser.o global.o lock.o trans.o msg_queue.o
OBJMIDDLEWARE = middleware.o ../framework/hidden/middle-support.o ../framework/hidden/io.o ../framework/hidden/msg.o middle_com.o listen_thread.o connection_list.o global.o lock.o trans.o msg_queue.o parser.o work_thread.o

all: $(PROG)

global.o: global.c
	$(CC) $(CFLAGS) -o global.o global.c
work_thread.o: work_thread.c
	$(CC) $(CFLAGS) -o work_thread.o work_thread.c
msg_queue.o: msg_queue.c
	$(CC) $(CFLAGS) -o msg_queue.o msg_queue.c
lock.o: lock.c lock.h
	$(CC) $(CFLAGS) -o lock.o lock.c
trans.o: trans.c trans.h
	$(CC) $(CFLAGS) -o trans.o trans.c
parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -o parser.o parser.c
listen_thread.o: listen_thread.c listen_thread.h
	$(CC) $(CFLAGS) -o listen_thread.o listen_thread.c
connection_list.o: connection_list.c connection_list.h
	$(CC) $(CFLAGS) -o connection_list.o connection_list.c
middle_com.o: middle_com.c middle_com.h
	$(CC) $(CFLAGS) -o middle_com.o middle_com.c
middleware.o: middleware.c
	${CC} -o middleware.o middleware.c ${CFLAGS}
../framwork/hidden/middle-support.o: ../framework/hidden/middle-support.c
	${CC} ${CFLAGS} -o ../framework/hidden/middle-support.o ../framework/hidden/middle-support.c
../framework/hidden/cmd.o: ../framework/hidden/cmd.c
	${CC} ${CFLAGS} -o ../framwork/hidden/cmd.o ../framework/hidden/cmd.c
../framework/hidden/io.o: ../framework/hidden/io.c
	${CC} ${CFLAGS} -o ../framework/hidden/io.o
../framework/msg.o:
middle: ${OBJMIDDLE}
#	$(CC) $(CFLAGS) -o middle ${OBJMIDDLE} ${LIBS}
	$(CC) -DLINUX -pipe -I../framework/ -I../framework/hidden -o middle ${OBJMIDDLE} -L../framework -lgdbm
middleware: ${OBJMIDDLEWARE}
#	${CC} ${CFLAGS} -o $@ ${OBJMIDDLEWARE} ${LIBS}
#	$(CC) ${CFLAGS} -o $@ ${OBJMIDDLEWARE} ${LIBS}
	$(CC)  -DLINUX -pipe -I../framework -I../framework/hidden -o middleware ${OBJMIDDLEWARE} ${LIBS}
clean:
	rm -f $(PROG) $(OBJS) *~ middleware middle *.o
