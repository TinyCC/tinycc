#
# Tiny C Compiler Makefile
#
prefix=/usr/local

CFLAGS=-O2 -g -Wall
LIBS=-ldl
CFLAGS_P=$(CFLAGS) -pg -static -DCONFIG_TCC_STATIC
LIBS_P=

CFLAGS+=-m386 -malign-functions=0
DISAS=objdump -d
INSTALL=install
VERSION=0.9.3

all: tcc

# auto test

test: test.ref test.out
	@if diff -u test.ref test.out ; then echo "Auto Test OK"; fi

tcctest.ref: tcctest.c 
	gcc $(CFLAGS) -I. -o $@ $<

test.ref: tcctest.ref
	./tcctest.ref > $@

test.out: tcc tcctest.c
	./tcc -I. tcctest.c > $@

run: tcc tcctest.c
	./tcc -I. tcctest.c

# iterated test2 (compile tcc then compile tcctest.c !)
test2: tcc tcc.c tcctest.c test.ref
	./tcc -I. tcc.c -I. tcctest.c > test.out2
	@if diff -u test.ref test.out2 ; then echo "Auto Test2 OK"; fi

# iterated test3 (compile tcc then compile tcc then compile tcctest.c !)
test3: tcc tcc.c tcctest.c test.ref
	./tcc -I. tcc.c -I. tcc.c -I. tcctest.c > test.out3
	@if diff -u test.ref test.out3 ; then echo "Auto Test3 OK"; fi

# memory and bound check auto test
BOUNDS_OK  = 1 4 8 10
BOUNDS_FAIL= 2 5 7 9 11 12

btest: boundtest.c tcc
	@for i in $(BOUNDS_OK); do \
           if ./tcc -b boundtest.c $$i ; then \
               /bin/true ; \
           else\
	       echo Failed positive test $$i ; exit 1 ; \
           fi ;\
        done ;\
        for i in $(BOUNDS_FAIL); do \
           if ./tcc -b boundtest.c $$i ; then \
	       echo Failed negative test $$i ; exit 1 ;\
           else\
               /bin/true ; \
           fi\
        done ;\
        echo Bound test OK

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

tcc_g: tcc.c i386-gen.c bcheck.c Makefile
	gcc $(CFLAGS) -o $@ $< $(LIBS)

tcc: tcc_g
	strip -s -R .comment -R .note -o $@ $<

install: tcc 
	$(INSTALL) -m755 tcc $(prefix)/bin
	mkdir -p $(prefix)/lib/tcc
	$(INSTALL) -m644 stdarg.h stddef.h float.h stdbool.h \
                   tcclib.h $(prefix)/lib/tcc

clean:
	rm -f *~ *.o tcc tcc1 tcct tcc_g tcctest.ref *.bin *.i ex2 \
           core gmon.out test.out test.ref a.out tcc_p

# profiling version
tcc_p: tcc.c Makefile
	gcc $(CFLAGS_P) -o $@ $< $(LIBS_P)

# targets for development

%.bin: %.c tcc
	./tcc -g -o $@ -I. $<
	$(DISAS) $@

instr: instr.o
	objdump -d instr.o

instr.o: instr.S
	gcc -O2 -Wall -g -c -o $@ $<

FILE=tcc-$(VERSION)

tar:
	rm -rf /tmp/$(FILE)
	cp -r ../tcc /tmp/$(FILE)
	( cd /tmp ; tar zcvf ~/$(FILE).tar.gz \
          $(FILE)/Makefile $(FILE)/README $(FILE)/TODO $(FILE)/COPYING \
	  $(FILE)/Changelog $(FILE)/tcc-doc.html \
          $(FILE)/tcc.c $(FILE)/i386-gen.c $(FILE)/bcheck.c \
          $(FILE)/stddef.h $(FILE)/stdarg.h $(FILE)/stdbool.h $(FILE)/float.h \
          $(FILE)/tcclib.h \
          $(FILE)/ex*.c $(FILE)/tcctest.c $(FILE)/boundtest.c )
	rm -rf /tmp/$(FILE)
