obj-m    += src/quotes.o
ccflags-y = -I$(PWD)/include
CC        = gcc
CPPFLAGS  = -I./include/
TEST-RCV  = test/test-recver

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) $(CPPFLAGS) test/test-recver.c -o $(TEST-RCV)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f $(TEST-RCV)

