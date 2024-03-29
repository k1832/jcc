CFLAGS=-std=c11 -g -static -Wall -Werror
SRCS=$(wildcard *.c)
HDRS=$(wildcard *.h)
# https://www.gnu.org/software/make/manual/make.html#Substitution-Refs
OBJS=$(SRCS:.c=.o)

jcc: $(OBJS)
	$(MAKE) style
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): $(HDRS)

test: jcc
	./test.sh

style: $(SRCS) $(HDRS)
	cpplint $^

clean:
	rm -f jcc *.o *~ tmp*

.PHONY: test clean style
