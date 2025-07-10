CC = gcc
CFLAGS = -g 
CPPFLAGS = -Wall -Wextra -pedantic -std=gnu90

chatty: main.o liblist.a
	$(CC) $(CFLAGS) -L. -o chatty main.o -llist -lpthread

main.o: main.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -I. -c -o main.o main.c

liblist.a: list_adders.o list_removers.o list_movers.o
	ar -rcs liblist.a list_adders.o list_removers.o list_movers.o

list_adders.o: list_adders.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -I. -c -o list_adders.o list_adders.c

list_movers.o: list_movers.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -I. -c -o list_movers.o list_movers.c

list_removers.o: list_removers.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -I. -c -o list_removers.o list_removers.c

listtest: listtest.o liblist.a
	$(CC) $(CFLAGS) -L. -o listtest listtest.o -llist

listtest.o: listtest.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -I. -c -o listtest.o listtest.c

clean:
	rm -f *.o chatty liblist.a
