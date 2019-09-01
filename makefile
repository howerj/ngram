CFLAGS  = -Wall -Wextra -std=c99 -pedantic -g
TARGET  = ngram
AR      = ar
ARFLAGS = rcs
RANLIB  = ranlib

.PHONY: all run check clean test

all: ${TARGET}

run: ${TARGET}
	./${TARGET}

${TARGET}.o: ${TARGET}.c ${TARGET}.h

main.o: main.c lib${TARGET}.a ${TARGET}.h

${TARGET}: lib${TARGET}.a main.o

test: ngram.c.ngram
	./${TARGET} -b

check:
	cppcheck --enable=all *.c

clean:
	git clean -dfx

%.ngram: % ${TARGET}
	${TRACER} ./${TARGET} -v -l 2 -H 5 < $< > $@

lib${TARGET}.a: ${TARGET}.o
	${AR} ${ARFLAGS} $@ $<
	${RANLIB} $@

