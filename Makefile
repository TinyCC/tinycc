#
# Tiny C Compiler Makefile
#
include config.mak

CFLAGS=-O2 -g -Wall
LIBS=-ldl
CFLAGS_P=$(CFLAGS) -pg -static -DCONFIG_TCC_STATIC
LIBS_P=

CFLAGS+=-mpreferred-stack-boundary=2
ifeq ($(GCC_MAJOR),2)
CFLAGS+=-m386 -malign-functions=0
else
CFLAGS+=-march=i386 -falign-functions=0
endif

DISAS=objdump -d
INSTALL=install

# run local version of tcc with local libraries and includes
TCC=./tcc -B. -I.

all: tcc libtcc1.o bcheck.o tcc-doc.html libtcc.a libtcc_test

Makefile: config.mak

# auto test

test: test.ref test.out
	@if diff -u test.ref test.out ; then echo "Auto Test OK"; fi

tcctest.ref: tcctest.c 
	$(CC) $(CFLAGS) -I. -o $@ $<

test.ref: tcctest.ref
	./tcctest.ref > $@

test.out: tcc tcctest.c
	$(TCC) -run tcctest.c > $@

run: tcc tcctest.c
	$(TCC) -run tcctest.c

# iterated test2 (compile tcc then compile tcctest.c !)
test2: tcc tcc.c tcctest.c test.ref
	$(TCC) -run tcc.c -B. -I. -run tcctest.c > test.out2
	@if diff -u test.ref test.out2 ; then echo "Auto Test2 OK"; fi

# iterated test3 (compile tcc then compile tcc then compile tcctest.c !)
test3: tcc tcc.c tcctest.c test.ref
	$(TCC) -run tcc.c -B. -I. -run tcc.c -B. -I. -run tcctest.c > test.out3
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
           if $(TCC) -b -run boundtest.c $$i ; then \
               /bin/true ; \
           else\
	       echo Failed positive test $$i ; exit 1 ; \
           fi ;\
        done ;\
        for i in $(BOUNDS_FAIL); do \
           if $(TCC) -b -run boundtest.c $$i ; then \
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
	$(CC) $(CFLAGS) -o $@ $<

ex3: ex3.c
	$(CC) $(CFLAGS) -o $@ $<

# Native Tiny C Compiler

tcc_g: tcc.c i386-gen.c tccelf.c tccasm.c i386-asm.c tcctok.h libtcc.h i386-asm.h Makefile
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

tcc: tcc_g Makefile
	strip -s -R .comment -R .note -o $@ $<

# TinyCC runtime libraries
libtcc1.o: libtcc1.c
	$(CC) -O2 -Wall -c -o $@ $<

bcheck.o: bcheck.c
	$(CC) -O2 -Wall -c -o $@ $<

install: tcc_install libinstall

tcc_install: tcc libtcc1.o bcheck.o
	$(INSTALL) -m755 tcc $(bindir)
	$(INSTALL) tcc.1 $(mandir)/man1
	mkdir -p $(libdir)/tcc
	mkdir -p $(libdir)/tcc/include
	$(INSTALL) -m644 libtcc1.o bcheck.o $(libdir)/tcc
	$(INSTALL) -m644 stdarg.h stddef.h stdbool.h float.h varargs.h \
                   tcclib.h $(libdir)/tcc/include

clean:
	rm -f *~ *.o tcc tcc1 tcct tcc_g tcctest.ref *.bin *.i ex2 \
           core gmon.out test.out test.ref a.out tcc_p \
           *.exe tcc-doc.html libtcc.a libtcc_test \
           tcctest[1234] test[1234].out

distclean: clean
	rm -f config.h config.mak config.texi

# win32 TCC
tcc_g.exe: tcc.c i386-gen.c bcheck.c Makefile
	i386-mingw32msvc-gcc $(CFLAGS) -DCONFIG_TCC_STATIC -o $@ $<

tcc.exe: tcc_g.exe
	i386-mingw32msvc-strip -o $@ $<

# profiling version
tcc_p: tcc.c Makefile
	$(CC) $(CFLAGS_P) -o $@ $< $(LIBS_P)

# libtcc generation and example
libinstall: libtcc.a 
	$(INSTALL) -m644 libtcc.a $(libdir)
	$(INSTALL) -m644 libtcc.h $(includedir)

libtcc.o: tcc.c i386-gen.c bcheck.c Makefile
	$(CC) $(CFLAGS) -DLIBTCC -c -o $@ $<

libtcc.a: libtcc.o 
	$(AR) rcs $@ $^

libtcc_test: libtcc_test.c libtcc.a 
	$(CC) $(CFLAGS) -I. -o $@ $< -L. -ltcc -ldl

libtest: libtcc_test
	./libtcc_test

# targets for development

%.bin: %.c tcc
	$(TCC) -g -o $@ $<
	$(DISAS) $@

instr: instr.o
	objdump -d instr.o

# tiny assembler testing

asmtest.ref: asmtest.S
	$(CC) -c -o asmtest.ref.o asmtest.S
	objdump -D asmtest.ref.o > $@

# XXX: we compute tcc.c to go faster during development !
asmtest.out: asmtest.S tcc
#	./tcc tcc.c -c asmtest.S
#asmtest.out: asmtest.S tcc
	./tcc -c asmtest.S
	objdump -D asmtest.o > $@

asmtest: asmtest.out asmtest.ref
	@if diff -u --ignore-matching-lines="file format" asmtest.ref asmtest.out ; then echo "ASM Auto Test OK"; fi

instr.o: instr.S
	$(CC) -O2 -Wall -g -c -o $@ $<

cache: tcc_g
	cachegrind ./tcc_g -o /tmp/linpack -lm bench/linpack.c
	vg_annotate tcc.c > /tmp/linpack.cache.log

# documentation
tcc-doc.html: tcc-doc.texi
	texi2html -monolithic -number $<

FILES= Makefile Makefile.uClibc configure VERSION \
       README TODO COPYING \
       Changelog tcc-doc.texi tcc-doc.html \
       tcc.1 \
       tcc.c i386-gen.c tccelf.c tcctok.h tccasm.c  i386-asm.c i386-asm.h\
       bcheck.c libtcc1.c \
       elf.h stab.h stab.def \
       stddef.h stdarg.h stdbool.h float.h varargs.h \
       tcclib.h libtcc.h libtcc_test.c \
       ex1.c ex2.c ex3.c ex4.c ex5.c \
       tcctest.c boundtest.c gcctestsuite.sh

FILE=tcc-$(VERSION)

tar:
	rm -rf /tmp/$(FILE)
	mkdir -p /tmp/$(FILE)
	cp -P $(FILES) /tmp/$(FILE)
	( cd /tmp ; tar zcvf ~/$(FILE).tar.gz $(FILE) )
	rm -rf /tmp/$(FILE)
