#make file for pthread communcation testing

PROG 	= app.out
OBJS	= com.o main.o
CC 	= gcc
LD 	= gcc
CFLAGS 	= -c -Wall

all: $(PROG)

$(PROG) : $(OBJS) 
	$(LD) -lpthread -o $(PROG) $(OBJS)

com.o: com.c com.h
	$(CC) $(CFLAGS) -o com.o com.c
main.o: main.c
	$(CC) $(CFLAGS) -o main.o main.c
clean:
	rm -f $(PROG) $(OBJS)
