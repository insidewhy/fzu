VERSION=1.0

CPPFLAGS=-DVERSION=\"${VERSION}\" -D_GNU_SOURCE
CFLAGS+=-Wall -Wextra -g -std=c99 -O3 -pedantic -Ideps
PREFIX?=/usr/local
MANDIR?=$(PREFIX)/share/man
BINDIR?=$(PREFIX)/bin
DEBUGGER?=

LIBXDG_BASEDIRFLAGS=$(shell pkg-config --libs libxdg-basedir)
ifneq (${LIBXDG_BASEDIRFLAGS},)
	LIBCONFIGFLAGS=$(shell pkg-config --libs libconfig)
	ifneq (${LIBCONFIGFLAGS},)
		LINKFLAGS=${LIBXDG_BASEDIRFLAGS} ${LIBCONFIGFLAGS}
		CFLAGS+=-DWITH_LIBCONFIG $(shell pkg-config --cflags libxdg-basedir libconfig)
	endif
endif

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=${INSTALL} -m 644

LIBS=-lpthread
OBJECTS=src/fzu.o src/match.o src/tty.o src/choices.o src/options.o src/tty_interface.o src/configuration.o src/memory.o
THEFTDEPS = deps/theft/theft.o deps/theft/theft_bloom.o deps/theft/theft_mt.o deps/theft/theft_hash.o
TESTOBJECTS=test/fzutest.c test/test_properties.c test/test_choices.c test/test_match.c src/match.o src/choices.o src/memory.o src/options.o $(THEFTDEPS)

all: fzu

test/fzutest: $(TESTOBJECTS)
	$(CC) $(CFLAGS) $(CCFLAGS) -Isrc -o $@ $(TESTOBJECTS) $(LIBS)

acceptance: fzu
	cd test/acceptance && bundle --quiet && bundle exec ruby acceptance_test.rb

test: check
check: test/fzutest
	$(DEBUGGER) ./test/fzutest

fzu: $(OBJECTS)
	$(CC) $(CFLAGS) $(CCFLAGS) $(LINKFLAGS) -o $@ $(OBJECTS) $(LIBS)

%.o: %.c config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

config.h:
	cp src/config.def.h config.h

install: fzu
	mkdir -p $(DESTDIR)$(BINDIR)
	cp fzu $(DESTDIR)$(BINDIR)/
	chmod 755 ${DESTDIR}${BINDIR}/fzu
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp fzu.1 $(DESTDIR)$(MANDIR)/man1/
	chmod 644 ${DESTDIR}${MANDIR}/man1/fzu.1

fmt:
	clang-format -i src/*.c src/*.h

clean:
	rm -f fzu test/fzutest src/*.o deps/*/*.o

veryclean: clean
	rm -f config.h

.PHONY: test check all clean veryclean install fmt acceptance
