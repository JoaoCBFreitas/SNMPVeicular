CFLAGS=-fno-stack-protector
all: generator receiver
generator: 
		$(CC) $(CFLAGS) -o generator canGen.c
receiver:
		$(CC) -o receiver canReceive.c -lm
clean:
	rm generator receiver
