#
# Tiny C Compiler Makefile
#
prefix=/usr/local

CFLAGS=-O2 -g -Wall
LIBS=-ldl
CFLAGS_P=$(CFLAGS) -pg -static -DCONFIG_TCC_STATIC
LIBS_P=

CFLAGS+=-m386 -malign-functions=0
CFLAGS+=-DCONFIG_TCC_PREFIX=\"$(prefix)\"
DISAS=objdump -d
INSTALL=install
VERSION=0.9.10

# run local version of tcc with local libraries and includes
TCC=./tcc -B. -I.

all: tcc libtcc1.o bcheck.o tcc-doc.html

# auto test

test: test.ref test.out
	@if diff -u test.ref test.out ; then echo "Auto Test OK"; fi

tcctest.ref: tcctest.c 
	gcc $(CFLAGS) -I. -o $@ $<

test.ref: tcctest.ref
	./tcctest.ref > $@

test.out: tcc tcctest.c
	$(TCC) tcctest.c > $@

run: tcc tcctest.c
	$(TCC) tcctest.c

# iterated test2 (compile tcc then compile tcctest.c !)
test2: tcc tcc.c tcctest.c test.ref
	$(TCC) tcc.c -B. -I. tcctest.c > test.out2
	@if diff -u test.ref test.out2 ; then echo "Auto Test2 OK"; fi

# iterated test3 (compile tcc then compile tcc then compile tcctest.c !)
test3: tcc tcc.c tcctest.c test.ref
	$(TCC) tcc.c -B. -I. tcc.c -B. -I. tcctest.c > test.out3
	@if diff -u test.ref test.out3 ; then echo "Auto Test3 OK"; fi

# binary output test
test4: tcc test.ref
# dynamic output
	$(TCC) -o tcctest1 tcctest.c
	./tcctest1 > test1.out
	@if diff -u test.ref test1.out ; then echo "Dynamic Auto Test OK"; fi
# static output
	$(TCC) -static -o tcctest2 tcctest.c
	./tcctest2 > test2.out
	@if diff -u test.ref test2.out ; then echo "Static Auto Test OK"; fi
# object + link output
	$(TCC) -c -o tcctest3.o tcctest.c
	$(TCC) -o tcctest3 tcctest3.o
	./tcctest3 > test3.out
	@if diff -u test.ref test3.out ; then echo "Object Auto Test OK"; fi
# dynamic output + bound check
	$(TCC) -b -o tcctest4 tcctest.c
	./tcctest4 > test4.out
	@if diff -u test.ref test4.out ; then echo "BCheck Auto Test OK"; fi

# memory and bound check auto test
BOUNDS_OK  = 1 4 8 10
BOUNDS_FAIL= 2 5 7 9 11 12 13

btest: boundtest.c tcc
	@for i in $(BOUNDS_OK); do \
           if $(TCC) -b boundtest.c $$i ; then \
               /bin/true ; \
           else\
	       echo Failed positive test $$i ; exit 1 ; \
           fi ;\
        done ;\
        for i in $(BOUNDS_FAIL); do \
           if $(TCC) -b boundtest.c $$i ; then \
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

# Native Tiny C Compiler

tcc_g: tcc.c i386-gen.c tccelf.c tcctok.h libtcc.h Makefile
	gcc $(CFLAGS) -o $@ $< $(LIBS)

tcc: tcc_g
	strip -s -R .comment -R .note -o $@ $<

# TinyCC runtime libraries
libtcc1.o: libtcc1.c
	gcc -O2 -Wall -c -o $@ $<

bcheck.o: bcheck.c
	gcc -O2 -Wall -c -o $@ $<

install: tcc libtcc1.o bcheck.o
	$(INSTALL) -m755 tcc $(prefix)/bin
	$(INSTALL) tcc.1 $(prefix)/share/man/man1
	mkdir -p $(prefix)/lib/tcc
	mkdir -p $(prefix)/lib/tcc/include
	$(INSTALL) -m644 libtcc1.o bcheck.o $(prefix)/lib/tcc
	$(INSTALL) -m644 stdarg.h stddef.h float.h stdbool.h \
                   tcclib.h $(prefix)/lib/tcc/include

clean:
	rm -f *~ *.o tcc tcc1 tcct tcc_g tcctest.ref *.bin *.i ex2 \
           core gmon.out test.out test.ref a.out tcc_p \
           *.exe iltcc iltcc_g tcc-doc.html \
           tcctest[1234] test[1234].out

# IL TCC

iltcc_g: tcc.c il-gen.c bcheck.c Makefile
	gcc $(CFLAGS) -DTCC_TARGET_IL -o $@ $< $(LIBS)

iltcc: iltcc_g
	strip -s -R .comment -R .note -o $@ $<

# win32 TCC
tcc_g.exe: tcc.c i386-gen.c bcheck.c Makefile
	i386-mingw32msvc-gcc $(CFLAGS) -DCONFIG_TCC_STATIC -o $@ $<

tcc.exe: tcc_g.exe
	i386-mingw32msvc-strip -o $@ $<

# profiling version
tcc_p: tcc.c Makefile
	gcc $(CFLAGS_P) -o $@ $< $(LIBS_P)

# libtcc generation and example
libinstall: libtcc.a 
	$(INSTALL) -m644 libtcc.a $(prefix)/lib
	$(INSTALL) -m644 libtcc.h $(prefix)/include

libtcc.o: tcc.c i386-gen.c bcheck.c Makefile
	gcc $(CFLAGS) -DLIBTCC -c -o $@ $<

libtcc.a: libtcc.o 
	ar rcs $@ $^

libtcc_test: libtcc_test.c libtcc.a 
	gcc $(CFLAGS) -I. -o $@ $< -L. -ltcc -ldl

# targets for development

%.bin: %.c tcc
	$(TCC) -g -o $@ $<
	$(DISAS) $@

instr: instr.o
	objdump -d instr.o

instr.o: instr.S
	gcc -O2 -Wall -g -c -o $@ $<

# documentation
tcc-doc.html: tcc-doc.texi
	texi2html -monolithic -number $<

FILE=tcc-$(VERSION)

tar:
	rm -rf /tmp/$(FILE)
	cp -r ../tcc /tmp/$(FILE)
	( cd /tmp ; tar zcvf ~/$(FILE).tar.gz \
          $(FILE)/Makefile $(FILE)/Makefile.uClibc \
          $(FILE)/README $(FILE)/TODO $(FILE)/COPYING \
	  $(FILE)/Changelog $(FILE)/tcc-doc.texi $(FILE)/tcc-doc.html \
          $(FILE)/tcc.1 \
          $(FILE)/tcc.c $(FILE)/i386-gen.c $(FILE)/tccelf.c $(FILE)/tcctok.h \
          $(FILE)/bcheck.c $(FILE)/libtcc1.c \
          $(FILE)/il-opcodes.h $(FILE)/il-gen.c \
          $(FILE)/elf.h $(FILE)/stab.h $(FILE)/stab.def \
          $(FILE)/stddef.h $(FILE)/stdarg.h $(FILE)/stdbool.h $(FILE)/float.h \
          $(FILE)/tcclib.h $(FILE)/libtcc.h $(FILE)/libtcc_test.c \
          $(FILE)/ex*.c $(FILE)/tcctest.c $(FILE)/boundtest.c )
	rm -rf /tmp/$(FILE)
