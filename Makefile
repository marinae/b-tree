CC=gcc
CFLAGS=-std=c99
LDFLAGS=-shared -fPIC
SOURCES=blocks.c helpful.c insert.c select.c delete.c database.c mydb.c
EXECUTABLE=libmydb.so

all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(EXECUTABLE)

sophia:
	make -C sophia/

clean:
	rm $(EXECUTABLE)
	rm mydbpath
