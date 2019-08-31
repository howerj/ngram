CFLAGS=-Wall -Wextra -std=c99 -pedantic -g
TARGET=ngram
AR      = ar
ARFLAGS = rcs
RANLIB  = ranlib

.PHONY: all run check clean test

all: ${TARGET}

run: ${TARGET}
	./${TARGET}

${TARGET}: lib${TARGET}.a main.o

test: ngram.c.ngram

check:
	cppcheck --enable=all *.c

clean:
	git clean -dfx

%.ngram: % ${TARGET}
	./${TARGET} -l 2 -H 3 < $< > $@ 

lib${TARGET}.a: ${TARGET}.o
	${AR} ${ARFLAGS} $@ $<
	${RANLIB} $@

