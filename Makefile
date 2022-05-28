CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
HDRS=$(wildcard *.h)
OBJS=$(SRCS:.c=.o)

jcc: style $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): $(HDRS)

test: jcc
	./test.sh

style: $(SRCS) $(HDRS)
	cpplint $^

clean:
	rm -f jcc *.o *~ tmp*

.PHONY: test clean style
