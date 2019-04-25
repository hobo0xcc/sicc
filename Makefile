CC=gcc
AS=as
LD=ld
RM=rm -rf

sicc: main.c
	$(CC) -o $@ $^

clean:
	$(RM) sicc tst tst.*
