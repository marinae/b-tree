CC=gcc
CFLAGS=-std=c99
LDFLAGS=-shared -fPIC
SOURCES=blocks.c helpful.c insert.c select.c delete.c database.c mydb.c
LIBRARY=libmydb.so

all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(LIBRARY)

sophia:
	make -C sophia/

clean:
	rm $(LIBRARY)
	rm mydbpath

test1:
	./runner/test_speed ./workloads/workload.lat $(LIBRARY)
	diff ./workloads/workload.lat.out ./workloads/workload.lat.out.yours

test2:
	./runner/test_speed ./workloads/workload.old $(LIBRARY)
	diff ./workloads/workload.old.out ./workloads/workload.old.out.yours

test3:
	./runner/test_speed ./workloads/workload.uni $(LIBRARY)
	diff ./workloads/workload.uni.out ./workloads/workload.uni.out.yours

test4:
	./runner/test_speed ./workloads/rwd-workloads $(LIBRARY)
	#diff ./workloads/rwd-workloads.out ./workloads/rwd-workloads.out.yours

test:
	make test1
	make test2
	make test3
	make test4
	
	
	
	

