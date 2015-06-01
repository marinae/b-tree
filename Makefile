CC=gcc
CFLAGS=-std=c99
LDFLAGS=-shared -fPIC
SOURCES=blocks.c cache.c logger.c helpful.c insert.c select.c delete.c database.c mydb.c
LIBRARY=libmydb.so
DATABASE=mydbpath
WAL=wal
EXE=./runner/test_speed
WDIR=./workloads

all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(LIBRARY)

sophia:
	make -C sophia/

clean:
	rm $(LIBRARY)
	rm $(DATABASE)
	rm $(WAL)
	rm $(WDIR)/*.yours

test1:
	$(EXE) $(WDIR)/workload.lat $(LIBRARY)
	diff $(WDIR)/workload.lat.out $(WDIR)/workload.lat.out.yours

test2:
	$(EXE) $(WDIR)/workload.old $(LIBRARY)
	diff $(WDIR)/workload.old.out $(WDIR)/workload.old.out.yours

test3:
	$(EXE) $(WDIR)/workload.uni $(LIBRARY)
	diff $(WDIR)/workload.uni.out $(WDIR)/workload.uni.out.yours

test4:
	$(EXE) $(WDIR)/rwd-workloads $(LIBRARY)
	diff $(WDIR)/rwd-workloads.out $(WDIR)/rwd-workloads.out.yours

wal_test:
	rm $(DATABASE)
	rm $(WAL)
	$(EXE) $(WDIR)/insert_workload $(LIBRARY)
	$(EXE) $(WDIR)/select_workload $(LIBRARY)
	diff $(WDIR)/select_workload.out $(WDIR)/select_workload.out.yours

test:
	rm $(DATABASE)
	rm $(WAL)
	make test1
	rm $(DATABASE)
	rm $(WAL)
	make test2
	rm $(DATABASE)
	rm $(WAL)
	make test3
	rm $(DATABASE)
	rm $(WAL)
	make test4
	
	
	
	

