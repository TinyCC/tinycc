#
# Tiny C Compiler Makefile
#
prefix=/usr/local

CFLAGS=-O2 -g -Wall -Wno-parentheses -Wno-missing-braces -I.
LIBS=-ldl
#CFLAGS=-O2 -g -Wall -Wno-parentheses -I. -pg -static -DPROFILE
#LIBS=

CFLAGS+=-m386 -malign-functions=0
DISAS=objdump -D -b binary -m i386
INSTALL=install
VERSION=0.9.1

all: tcc

# auto test

test: test.ref test.out
	@if diff -u test.ref test.out ; then echo "Auto Test OK"; fi

prog.ref: prog.c 
	gcc $(CFLAGS) -o $@ $<

test.ref: prog.ref
	./prog.ref > $@

test.out: tcc prog.c
	./tcc -I. prog.c > $@

run: tcc prog.c
	./tcc -I. prog.c

# iterated test2 (compile tcc then compile prog.c !)
test2: tcc tcc.c prog.c
	./tcc -I. tcc.c -I. prog.c > test.out2
	@if diff -u test.ref test.out2 ; then echo "Auto Test2 OK"; fi

# iterated test3 (compile tcc then compile tcc then compile prog.c !)
test3: tcc tcc.c prog.c
	./tcc -I. tcc.c -I. tcc.c -I. prog.c > test.out3
	@if diff -u test.ref test.out3 ; then echo "Auto Test3 OK"; fi

# speed test
speed: tcc ex2 ex3
	time ./ex2 1238 2 3 4 10 13 4
	time ./tcc -I. ./ex2.c 1238 2 3 4 10 13 4
	time ./ex3 35
	time ./tcc -I. ./ex3.c 35

ex2: ex2.c
	gcc $(CFLAGS) -o $@ $<

ex3: ex3.c
	gcc $(CFLAGS) -o $@ $<

# Tiny C Compiler

tcc_g: tcc.c Makefile
	gcc $(CFLAGS) -o $@ $< $(LIBS)

tcc: tcc_g
	strip -s -R .comment -R .note -o $@ $<

install: tcc 
	$(INSTALL) -m755 tcc $(prefix)/bin
	mkdir -p $(prefix)/lib/tcc
	$(INSTALL) -m644 stdarg.h stddef.h tcclib.h $(prefix)/lib/tcc

clean:
	rm -f *~ *.o tcc tcc1 tcct tcc_g prog.ref *.bin *.i ex2 \
           core gmon.out test.out test.ref a.out

# target for development

%.bin: %.c tcc
	./tcc -o $@ -I. $<
	$(DISAS) $@

instr.o: instr.S
	gcc -O2 -Wall -g -c -o $@ $<

FILE=tcc-$(VERSION)

tar:
	rm -rf /tmp/$(FILE)
	cp -r ../tcc /tmp/$(FILE)
	( cd /tmp ; tar zcvf ~/$(FILE).tar.gz \
          $(FILE)/Makefile $(FILE)/README $(FILE)/TODO $(FILE)/COPYING \
          $(FILE)/tcc.c $(FILE)/stddef.h $(FILE)/stdarg.h $(FILE)/tcclib.h \
          $(FILE)/ex*.c $(FILE)/prog.c )
	rm -rf /tmp/$(FILE)
