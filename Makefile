CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
HDRS=$(wildcard *.h)
OBJS=$(SRCS:.c=.o)

jcc: $(OBJS)
	make style
	$(CC) -o jcc $(OBJS) $(LDFLAGS)

$(OBJS): $(HDRS)

test: jcc
	./test.sh

style: $(SRCS) $(HDRS)
	cpplint $(SRCS) $(HDRS)

clean:
	rm -f jcc *.o *~ tmp*

.PHONY: test clean style
