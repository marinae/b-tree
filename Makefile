all:
	gcc blocks.c helpful.c insert.c select.c database.c mydb.c -std=c99 -shared -fPIC -o libmydb.so

sophia:
	make -C sophia/

clean:
	rm libmydb.so
	rm mydbpath
