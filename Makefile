obj-m       += quotes.o
quotes-objs := src/btree.o src/quotes.o
ccflags-y    = -I$(PWD)/include -O2
CC           = gcc
CPPFLAGS     = -I./include/
TEST-RCV     = test-recver

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) $(CPPFLAGS) -O2 test/test-recver.c -o $(TEST-RCV)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f $(TEST-RCV)

