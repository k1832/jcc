CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

jcc: $(OBJS)
	make style
	$(CC) -o jcc $(OBJS) $(LDFLAGS)

$(OBJS): jcc.h

test: jcc
	./test.sh

style: jcc.c
	cpplint jcc.c

clean:
	rm -f jcc *.o *~ tmp*

.PHONY: test clean style
