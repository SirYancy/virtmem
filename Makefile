CC = gcc

CFLAGS = -Wall

OBJECTS = page_table.o disk.o program.o main.o

default: clean
default: virtmem

debug: CFLAGS += -g -o0
debug: clean
debug: virtmem

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

page_table.o: page_table.c
	$(CC) $(CFLAGS) -c page_table.c -o page_table.o

disk.o: disk.c
	$(CC) $(CFLAGS) -c disk.c -o disk.o

program.o: program.c
	$(CC) $(CFLAGS) -c program.c -o program.o

virtmem: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o virtmem

clean:
	rm -f *.o virtmem myvirtualdisk core
