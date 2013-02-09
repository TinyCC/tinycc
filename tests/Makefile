#
# Tiny C Compiler Makefile - tests
#

TOP = ..
include $(TOP)/Makefile
VPATH = $(top_srcdir)/tests

# what tests to run
TESTS = \
 hello-exe \
 hello-run \
 libtest \
 test3 \
 moretests

# test4 -- problem with -static
# asmtest -- minor differences with gcc
# btest -- works on i386 (including win32)
# test3 -- win32 does not know how to printf long doubles

# bounds-checking is supported only on i386
ifneq ($(ARCH),i386)
 TESTS := $(filter-out btest,$(TESTS))
endif
ifdef CONFIG_WIN32
 TESTS := $(filter-out test3,$(TESTS))
endif
ifeq ($(TARGETOS),Darwin)
 TESTS := $(filter-out hello-exe test3 btest,$(TESTS))
endif

ifdef DISABLE_STATIC
 export LD_LIBRARY_PATH:=$(CURDIR)/..
endif

ifeq ($(TARGETOS),Darwin)
 CFLAGS+=-Wl,-flat_namespace,-undefined,warning
 export MACOSX_DEPLOYMENT_TARGET:=10.2
 NATIVE_DEFINES+=-D_ANSI_SOURCE
endif

# run local version of tcc with local libraries and includes
TCCFLAGS = -B$(TOP)
ifdef CONFIG_WIN32
 TCCFLAGS = -B$(top_srcdir)/win32 -I$(top_srcdir)/include -L$(TOP)
endif

TCC = $(TOP)/tcc $(TCCFLAGS)
RUN_TCC = $(NATIVE_DEFINES) -DONE_SOURCE -run $(TOP)/tcc.c $(TCCFLAGS)

DISAS = objdump -d

# libtcc test
ifdef LIBTCC1
 LIBTCC1:=$(TOP)/$(LIBTCC1)
endif

all test : $(TESTS)

hello-exe: ../examples/ex1.c
	@echo ------------ $@ ------------
	$(TCC) $< -o hello$(EXESUF) || ($(TOP)/tcc -vv; exit 1) && ./hello$(EXESUF)

hello-run: ../examples/ex1.c
	@echo ------------ $@ ------------
	$(TCC) -run $<

libtest: libtcc_test$(EXESUF) $(LIBTCC1)
	@echo ------------ $@ ------------
	./libtcc_test$(EXESUF) lib_path=..

libtcc_test$(EXESUF): libtcc_test.c $(top_builddir)/$(LIBTCC)
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(NATIVE_DEFINES) $(LIBS) $(LINK_LIBTCC) $(LDFLAGS)

moretests:
	@echo ------------ $@ ------------
	$(MAKE) -C tests2

# test.ref - generate using gcc
# copy only tcclib.h so GCC's stddef and stdarg will be used
test.ref: tcctest.c
	cp ../include/tcclib.h .
	gcc -o tcctest.gcc $< -I. $(CPPFLAGS) -w $(CFLAGS) $(NATIVE_DEFINES) -std=gnu99 -O0 -fno-omit-frame-pointer $(LDFLAGS)
	./tcctest.gcc > $@

# auto test
test1: test.ref
	@echo ------------ $@ ------------
	$(TCC) -run tcctest.c > test.out1
	@if diff -u test.ref test.out1 ; then echo "Auto Test OK"; fi

# iterated test2 (compile tcc then compile tcctest.c !)
test2: test.ref
	@echo ------------ $@ ------------
	$(TCC) $(RUN_TCC) $(RUN_TCC) -run tcctest.c > test.out2
	@if diff -u test.ref test.out2 ; then echo "Auto Test2 OK"; fi

# iterated test3 (compile tcc then compile tcc then compile tcctest.c !)
test3: test.ref
	@echo ------------ $@ ------------
	$(TCC) $(RUN_TCC) $(RUN_TCC) $(RUN_TCC) -run tcctest.c > test.out3
	@if diff -u test.ref test.out3 ; then echo "Auto Test3 OK"; fi

# binary output test
test4: test.ref
	@echo ------------ $@ ------------
# object + link output
	$(TCC) -c -o tcctest3.o tcctest.c
	$(TCC) -o tcctest3 tcctest3.o
	./tcctest3 > test3.out
	@if diff -u test.ref test3.out ; then echo "Object Auto Test OK"; fi
# dynamic output
	$(TCC) -o tcctest1 tcctest.c
	./tcctest1 > test1.out
	@if diff -u test.ref test1.out ; then echo "Dynamic Auto Test OK"; fi
# dynamic output + bound check
	$(TCC) -b -o tcctest4 tcctest.c
	./tcctest4 > test4.out
	@if diff -u test.ref test4.out ; then echo "BCheck Auto Test OK"; fi
# static output
	$(TCC) -static -o tcctest2 tcctest.c
	./tcctest2 > test2.out
	@if diff -u test.ref test2.out ; then echo "Static Auto Test OK"; fi

# memory and bound check auto test
BOUNDS_OK  = 1 4 8 10 14
BOUNDS_FAIL= 2 5 7 9 11 12 13 15

btest: boundtest.c
	@echo ------------ $@ ------------
	@for i in $(BOUNDS_OK); do \
	   echo ; echo --- boundtest $$i ---; \
	   if $(TCC) -b -run boundtest.c $$i ; then \
	       echo succeded as expected; \
	   else\
	       echo Failed positive test $$i ; exit 1 ; \
	   fi ;\
	done ;\
	for i in $(BOUNDS_FAIL); do \
	   echo ; echo --- boundtest $$i ---; \
	   if $(TCC) -b -run boundtest.c $$i ; then \
	       echo Failed negative test $$i ; exit 1 ;\
	   else\
	       echo failed as expected; \
	   fi ;\
	done ;\
	echo; echo Bound test OK

# speed test
speedtest: ex2 ex3
	@echo ------------ $@ ------------
	time ./ex2 1238 2 3 4 10 13 4
	time $(TCC) -run $(top_srcdir)/examples/ex2.c 1238 2 3 4 10 13 4
	time ./ex3 35
	time $(TCC) -run $(top_srcdir)/examples/ex3.c 35

weaktest: test.ref
	$(TCC) -c tcctest.c -o weaktest.tcc.o $(CPPFLAGS) $(CFLAGS)
	 $(CC) -c tcctest.c -o weaktest.gcc.o -I. $(CPPFLAGS) -w $(CFLAGS)
	objdump -t weaktest.tcc.o | grep ' w ' | sed -e 's/.* \([a-zA-Z0-9_]*\)$$/\1/' | LC_ALL=C sort > weaktest.tcc.o.txt
	objdump -t weaktest.gcc.o | grep ' w ' | sed -e 's/.* \([a-zA-Z0-9_]*\)$$/\1/' | LC_ALL=C sort > weaktest.gcc.o.txt
	diff weaktest.gcc.o.txt weaktest.tcc.o.txt && echo "Weak Auto Test OK"

ex%: $(top_srcdir)/examples/ex%.c
	$(CC) -o $@ $< $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)

# tiny assembler testing
asmtest.ref: asmtest.S
	$(CC) -Wa,-W -o asmtest.ref.o -c asmtest.S
	objdump -D asmtest.ref.o > asmtest.ref

asmtest: asmtest.ref
	@echo ------------ $@ ------------
	$(TCC) -c asmtest.S
	objdump -D asmtest.o > asmtest.out
	@if diff -u --ignore-matching-lines="file format" asmtest.ref asmtest.out ; then echo "ASM Auto Test OK"; fi

# targets for development
%.bin: %.c tcc
	$(TCC) -g -o $@ $<
	$(DISAS) $@

instr: instr.o
	objdump -d instr.o

instr.o: instr.S
	$(CC) -o $@ -c $< -O2 -Wall -g

cache: tcc_g
	cachegrind ./tcc_g -o /tmp/linpack -lm bench/linpack.c
	vg_annotate tcc.c > /tmp/linpack.cache.log

# clean
clean:
	$(MAKE) -C tests2 $@
	rm -vf *~ *.o *.a *.bin *.i *.ref *.out *.out? *.out?b *.gcc *.exe \
	   hello libtcc_test tcctest[1234] ex? tcc_g tcclib.h

