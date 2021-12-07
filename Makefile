CFLAGS=-std=c11 -g -static

jcc: jcc.c
	make style
	cc ${CFLAGS} $? -o $@

test: jcc
	./test.sh

style: jcc.c
	cpplint jcc.c

clean:
	rm -f jcc *.o *~ tmp*

.PHONY: test clean style
