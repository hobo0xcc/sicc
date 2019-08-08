CC=gcc
AS=as
LD=gcc
RM=rm -rf

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

CFLAGS=-g

.PHONY: sicc
sicc: $(OBJS)
	$(LD) -o $@ $^

$(OBJS): sicc.h

.PHONY: test
test: sicc
	./test.sh

.PHONY: clean
clean:
	$(RM) sicc $(OBJS) tst tst.*
