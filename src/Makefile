#make file for pthread communcation testing

PROG 	= app.out
OBJS	= msg_queue.o global.o main.o lock.o trans.o
CC 	= gcc
LD 	= gcc
CFLAGS 	= -c -Wall

OBJMIDDLE = ../framework/middle.o ../framework/hidden/middle-support.o ../framework/hidden/io.o ../framework/hidden/msg.o parser.o global.o lock.o trans.o msg_queue.o

all: $(PROG)

$(PROG) : $(OBJS) 
	$(LD) -lpthread -o $(PROG) $(OBJS)

main.o: main.c 
	$(CC) $(CFLAGS) -o main.o main.c

global.o: global.c
	$(CC) $(CFLAGS) -o global.o global.c

msg_queue.o: msg_queue.c
	$(CC) $(CFLAGS) -o msg_queue.o msg_queue.c
lock.o: lock.c lock.h
	$(CC) $(CFLAGS) -o lock.o lock.c
trans.o: trans.c trans.h
	$(CC) $(CFLAGS) -o trans.o trans.c
parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -o parser.o parser.c
middle: ${OBJMIDDLE}
	$(CC) -DLINUX -pipe -I../framework/ -I../framework/hidden -o middle ${OBJMIDDLE} -L../framework -lgdbm
clean:
	rm -f $(PROG) $(OBJS) *~
