#make file for pthread communcation testing

PROG 	= app.out
OBJS	= com.o global.o main.o
CC 	= gcc
LD 	= gcc
CFLAGS 	= -c -Wall

all: $(PROG)

$(PROG) : $(OBJS) 
	$(LD) -lpthread -o $(PROG) $(OBJS)

main.o: main.c com.o global.o
	$(CC) $(CFLAGS) -o main.o main.c

global.o: global.c
	$(CC) $(CFLAGS) -o global.o global.c

com.o: com.c com.h
	$(CC) $(CFLAGS) -o com.o com.c
clean:
	rm -f $(PROG) $(OBJS)
