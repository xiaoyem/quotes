obj-m       += quotes.o
quotes-objs := src/btree.o src/quotes.o
ccflags-y    = -I$(PWD)/include -Wall -O3
CC           = gcc
CPPFLAGS     = -I./include/ -Wall -O3
TEST-RCV     = test-recver

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	strip --strip-unneeded quotes.ko
	$(CC) $(CPPFLAGS) test/test-recver.c -o $(TEST-RCV)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f $(TEST-RCV)

