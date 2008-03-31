#
# Tiny C Compiler Makefile
#
include config.mak

CFLAGS+=-g -Wall
ifndef CONFIG_WIN32
LIBS=-lm
ifndef CONFIG_NOLDL
LIBS+=-ldl
endif
BCHECK_O=bcheck.o
endif
CFLAGS_P=$(CFLAGS) -pg -static -DCONFIG_TCC_STATIC
LIBS_P=

CFLAGS+=-mpreferred-stack-boundary=2
ifeq ($(GCC_MAJOR),2)
CFLAGS+=-m386 -malign-functions=0
else
CFLAGS+=-march=i386 -falign-functions=0 -fno-strict-aliasing
ifneq ($(GCC_MAJOR),3)
CFLAGS+=-Wno-pointer-sign -Wno-sign-compare
endif
endif

DISAS=objdump -d
INSTALL=install

ifdef CONFIG_WIN32
PROGS=tcc$(EXESUF)
ifdef CONFIG_CROSS
PROGS+=c67-tcc$(EXESUF) arm-tcc$(EXESUF)
endif
PROGS+=tiny_impdef$(EXESUF) tiny_libmaker$(EXESUF)
else
ifeq ($(ARCH),i386)
PROGS=tcc$(EXESUF)
ifdef CONFIG_CROSS
PROGS+=arm-tcc$(EXESUF)
endif
endif
ifeq ($(ARCH),arm)
PROGS=tcc$(EXESUF)
ifdef CONFIG_CROSS
PROGS+=i386-tcc$(EXESUF)
endif
endif
ifdef CONFIG_CROSS
PROGS+=c67-tcc$(EXESUF) i386-win32-tcc$(EXESUF)
endif
endif

# run local version of tcc with local libraries and includes
TCC=./tcc -B. -I.

all: $(PROGS) \
     libtcc1.a $(BCHECK_O) tcc-doc.html tcc.1 libtcc.a \
     libtcc_test$(EXESUF)

Makefile: config.mak

# auto test

test: test.ref test.out
	@if diff -u test.ref test.out ; then echo "Auto Test OK"; fi

tcctest.ref: tcctest.c 
	$(CC) $(CFLAGS) -w -I. -o $@ $<

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

# Host Tiny C Compiler
ifdef CONFIG_WIN32
tcc$(EXESUF): tcc.c i386-gen.c tccelf.c tccasm.c i386-asm.c tcctok.h libtcc.h i386-asm.h tccpe.c
	$(CC) $(CFLAGS) -DTCC_TARGET_PE -o $@ $< $(LIBS)
else
ifeq ($(ARCH),i386)
tcc$(EXESUF): tcc.c i386-gen.c tccelf.c tccasm.c i386-asm.c tcctok.h libtcc.h i386-asm.h
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)
endif
ifeq ($(ARCH),arm)
tcc$(EXESUF): tcc.c arm-gen.c tccelf.c tccasm.c tcctok.h libtcc.h
	$(CC) $(CFLAGS) -DTCC_TARGET_ARM -o $@ $< $(LIBS)
endif
endif

# Cross Tiny C Compilers
i386-tcc$(EXESUF): tcc.c i386-gen.c tccelf.c tccasm.c i386-asm.c tcctok.h libtcc.h i386-asm.h
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

c67-tcc$(EXESUF): tcc.c c67-gen.c tccelf.c tccasm.c tcctok.h libtcc.h tcccoff.c
	$(CC) $(CFLAGS) -DTCC_TARGET_C67 -o $@ $< $(LIBS)

arm-tcc$(EXESUF): tcc.c arm-gen.c tccelf.c tccasm.c tcctok.h libtcc.h
	$(CC) $(CFLAGS) -DTCC_TARGET_ARM -DTCC_ARM_EABI -o $@ $< $(LIBS)

i386-win32-tcc$(EXESUF): tcc.c i386-gen.c tccelf.c tccasm.c i386-asm.c tcctok.h libtcc.h i386-asm.h tccpe.c
	$(CC) $(CFLAGS) -DTCC_TARGET_PE -o $@ $< $(LIBS)

# windows utilities
tiny_impdef$(EXESUF): win32/tools/tiny_impdef.c
	$(CC) $(CFLAGS) -o $@ $< -lkernel32
tiny_libmaker$(EXESUF): win32/tools/tiny_libmaker.c
	$(CC) $(CFLAGS) -o $@ $< -lkernel32

# TinyCC runtime libraries
ifdef CONFIG_WIN32
# for windows, we must use TCC because we generate ELF objects
LIBTCC1_OBJS=$(addprefix win32/lib/, crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o) libtcc1.o
LIBTCC1_CC=./tcc.exe -Bwin32 -DTCC_TARGET_PE
else
LIBTCC1_OBJS=libtcc1.o
LIBTCC1_CC=$(CC)
endif
ifeq ($(ARCH),i386)
LIBTCC1_OBJS+=alloca86.o alloca86-bt.o
endif

%.o: %.c
	$(LIBTCC1_CC) -O2 -Wall -c -o $@ $<

%.o: %.S
	$(LIBTCC1_CC) -c -o $@ $<

libtcc1.a: $(LIBTCC1_OBJS)
	$(AR) rcs $@ $^

bcheck.o: bcheck.c
	$(CC) -O2 -Wall -c -o $@ $<

install: tcc_install libinstall

tcc_install: $(PROGS) tcc.1 libtcc1.a $(BCHECK_O) tcc-doc.html
	mkdir -p "$(bindir)"
	$(INSTALL) -s -m755 $(PROGS) "$(bindir)"
ifndef CONFIG_WIN32
	mkdir -p "$(mandir)/man1"
	$(INSTALL) tcc.1 "$(mandir)/man1"
endif
	mkdir -p "$(tccdir)"
	mkdir -p "$(tccdir)/include"
ifdef CONFIG_WIN32
	mkdir -p "$(tccdir)/lib"
	$(INSTALL) -m644 libtcc1.a win32/lib/*.def "$(tccdir)/lib"
	cp -r win32/include/. "$(tccdir)/include"
	cp -r win32/examples/. "$(tccdir)/examples"
else
	$(INSTALL) -m644 libtcc1.a $(BCHECK_O) "$(tccdir)"
	$(INSTALL) -m644 stdarg.h stddef.h stdbool.h float.h varargs.h \
                   tcclib.h "$(tccdir)/include"
endif
	mkdir -p "$(docdir)"
	$(INSTALL) -m644 tcc-doc.html "$(docdir)"
ifdef CONFIG_WIN32
	$(INSTALL) -m644 win32/readme.txt "$(docdir)"
endif

clean:
	rm -f *~ *.o *.a tcc tcct tcc_g tcctest.ref *.bin *.i ex2 \
           core gmon.out test.out test.ref a.out tcc_p \
           *.exe *.lib tcc.pod libtcc_test \
           tcctest[1234] test[1234].out $(PROGS) win32/lib/*.o

distclean: clean
	rm -f config.h config.mak config.texi

# profiling version
tcc_p: tcc.c Makefile
	$(CC) $(CFLAGS_P) -o $@ $< $(LIBS_P)

# libtcc generation and example
libinstall: libtcc.a 
	mkdir -p "$(libdir)"
	$(INSTALL) -m644 libtcc.a "$(libdir)"
	mkdir -p "$(includedir)"
	$(INSTALL) -m644 libtcc.h "$(includedir)"

libtcc.o: tcc.c i386-gen.c Makefile
ifdef CONFIG_WIN32
	$(CC) $(CFLAGS) -DTCC_TARGET_PE -DLIBTCC -c -o $@ $<
else
	$(CC) $(CFLAGS) -DLIBTCC -c -o $@ $<
endif

libtcc.a: libtcc.o 
	$(AR) rcs $@ $^

libtcc_test$(EXESUF): libtcc_test.c libtcc.a
	$(CC) $(CFLAGS) -o $@ $< libtcc.a $(LIBS)

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

# documentation and man page
tcc-doc.html: tcc-doc.texi
	-texi2html -monolithic -number $<

tcc.1: tcc-doc.texi
	-./texi2pod.pl $< tcc.pod
	-pod2man --section=1 --center=" " --release=" " tcc.pod > $@

FILE=tcc-$(shell cat VERSION)

# tar release (use 'make -k tar' on a checkouted tree)
tar:
	rm -rf /tmp/$(FILE)
	cp -r . /tmp/$(FILE)
	( cd /tmp ; tar zcvf ~/$(FILE).tar.gz $(FILE) --exclude CVS )
	rm -rf /tmp/$(FILE)
