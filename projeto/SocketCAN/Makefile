CFLAGS=-fno-stack-protector
all: generator receiver
generator: 
		$(CC) $(CFLAGS) -o generator canGen.c
receiver:
		$(CC) -c -o canReceive.o canReceive.c -lm -ggdb3
