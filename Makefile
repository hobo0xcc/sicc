CC=gcc
AS=as
LD=gcc
RM=rm -rf

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

sicc: $(OBJS)
	$(LD) -o $@ $^

$(OBJS): sicc.h

clean:
	$(RM) sicc $(OBJS) tst tst.*
