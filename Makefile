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

test:
	./runner/test_speed ./workloads/workload.lat libmydb.so
	./runner/test_speed ./workloads/workload.old libmydb.so
	./runner/test_speed ./workloads/workload.uni libmydb.so
	./runner/test_speed ./workloads/rwd-workloads libmydb.so

diff:
	diff ./workloads/workload.lat.out ./workloads/workload.lat.out.yours
	diff ./workloads/workload.old.out ./workloads/workload.old.out.yours
	diff ./workloads/workload.uni.out ./workloads/workload.uni.out.yours
	diff ./workloads/rwd-workloads.out ./workloads/rwd-workloads.out.yours

