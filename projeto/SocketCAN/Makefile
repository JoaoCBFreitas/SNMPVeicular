CFLAGS=-fno-stack-protector
all: generator receiver
generator: canGen.c Makefile
		$(CC) $(CFLAGS) -o generator canGen.c
receiver: canReceive.h Makefile canReceive.c
		$(CC) -c -o canReceive.o canReceive.c -lm -ggdb3
