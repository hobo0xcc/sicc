CC=gcc
AS=as
LD=ld
RM=rm -rf

sicc: main.c
	$(CC) -g -o $@ $^

clean:
	$(RM) sicc tst tst.*
