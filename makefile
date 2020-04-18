VERSION = 0x010000
CFLAGS  = -Wall -Wextra -std=c99 -pedantic -O2 -DNGRAM_VERSION=${VERSION} 
TARGET  = ngram
AR      = ar
ARFLAGS = rcs
RANLIB  = ranlib
DESTDIR = install

.PHONY: all run check clean test install dist

all: ${TARGET}

run: ${TARGET}
	./${TARGET}

${TARGET}.o: ${TARGET}.c ${TARGET}.h

lib${TARGET}.a: ${TARGET}.o
	${AR} ${ARFLAGS} $@ $<
	${RANLIB} $@

main.o: main.c ${TARGET}.h

${TARGET}: main.o lib${TARGET}.a
	${CC} ${CFLAGS} $^ -o $@
	-strip $@

test: ngram.c.ngram
	./${TARGET} -b

${TARGET}.1: readme.md
	-pandoc -s -f markdown -t man $< -o $@

.git:
	git clone https://github.com/howerj/cdb cdb-repo
	mv cdb-repo/.git .
	rm -rf cdb-repo

install: ${TARGET} lib${TARGET}.a ${TARGET}.1 .git
	install -p -D ${TARGET} ${DESTDIR}/bin/${TARGET}
	install -p -m 644 -D lib${TARGET}.a ${DESTDIR}/lib/lib${TARGET}.a
	install -p -m 644 -D ${TARGET}.h ${DESTDIR}/include/${TARGET}.h
	-install -p -m 644 -D ${TARGET}.1 ${DESTDIR}/man/${TARGET}.1
	mkdir -p ${DESTDIR}/src
	cp -a .git ${DESTDIR}/src
	cd ${DESTDIR}/src && git reset --hard HEAD

check:
	cppcheck --enable=all *.c

dist: install
	tar zcf ${TARGET}-${VERSION}.tgz ${DESTDIR}

clean:
	git clean -dffx

%.ngram: % ${TARGET}
	${TRACER} ./${TARGET} -v -l 2 -H 5 < $< > $@


