CC = gcc
CFLAGS = -g 
CPPFLAGS = -Wall -Wextra -pedantic -std=gnu90

chatty: main.o
	$(CC) $(CFLAGS) -o chatty main.o -lpthread

main.o: main.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o main.o main.c

clean:
	rm -f *.o chatty
