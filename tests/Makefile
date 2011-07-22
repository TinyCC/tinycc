#
# Tiny C Compiler Makefile - tests
#

# what tests to run
TESTS = libtest test3

# these should work too
# TESTS += test1 test2 speedtest btest weaktest

# these don't work as they should
# TESTS += test4 asmtest

TOP = ..
include $(TOP)/Makefile

ifdef DISABLE_STATIC
export LD_LIBRARY_PATH:=$(CURDIR)/..
endif

# run local version of tcc with local libraries and includes
TCC = ../tcc -B..
RUN_TCC = $(NATIVE_DEFINES) -run -DONE_SOURCE ../tcc.c -B..
DISAS=objdump -d

all test : $(TESTS)

# make sure that tcc exists
test1 test2 test3 test4 btest speedtest asmtest weaktest : ../tcc
../%:
	$(MAKE) -C .. $*

# libtcc test
ifdef LIBTCC1
LIBTCC1:=$(TOP)/$(LIBTCC1)
endif

libtest: libtcc_test$(EXESUF) $(LIBTCC1)
	@echo ------------ $@ ------------
	./libtcc_test$(EXESUF) lib_path=..

libtcc_test$(EXESUF): libtcc_test.c ../$(LIBTCC)
	$(CC) -o $@ $^ -I.. $(CFLAGS) $(LIBS) $(LINK_LIBTCC)

# test.ref - generate using gcc
# copy only tcclib.h so GCC's stddef and stdarg will be used
test.ref: tcctest.c
	cp -u ../include/tcclib.h .
	$(CC) -o tcctest.gcc $< -I. -w $(CFLAGS) -std=gnu99
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
# dynamic output
	$(TCC) -o tcctest1 tcctest.c
	./tcctest1 > test1.out
	@if diff -u test.ref test1.out ; then echo "Dynamic Auto Test OK"; fi
# object + link output
	$(TCC) -c -o tcctest3.o tcctest.c
	$(TCC) -o tcctest3 tcctest3.o
	./tcctest3 > test3.out
	@if diff -u test.ref test3.out ; then echo "Object Auto Test OK"; fi
# static output
	$(TCC) -static -o tcctest2 tcctest.c
	./tcctest2 > test2.out
	@if diff -u test.ref test2.out ; then echo "Static Auto Test OK"; fi
# dynamic output + bound check
	$(TCC) -b -o tcctest4 tcctest.c
	./tcctest4 > test4.out
	@if diff -u test.ref test4.out ; then echo "BCheck Auto Test OK"; fi

# memory and bound check auto test
BOUNDS_OK  = 1 4 8 10 14
BOUNDS_FAIL= 2 5 7 9 11 12 13 15

btest: boundtest.c ../bcheck.o
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
	time $(TCC) -run ../examples/ex2.c 1238 2 3 4 10 13 4
	time ./ex3 35
	time $(TCC) -run ../examples/ex3.c 35

weaktest: test.ref
	$(TCC) -c tcctest.c -o weaktest.tcc.o
	 $(CC) -c tcctest.c -o weaktest.gcc.o -I. -w $(CFLAGS)
	objdump -t weaktest.tcc.o | grep ' w ' | sed -e 's/.* \([a-zA-Z0-9_]*\)$$/\1/' | LC_ALL=C sort > weaktest.tcc.o.txt
	objdump -t weaktest.gcc.o | grep ' w ' | sed -e 's/.* \([a-zA-Z0-9_]*\)$$/\1/' | LC_ALL=C sort > weaktest.gcc.o.txt
	diff weaktest.gcc.o.txt weaktest.tcc.o.txt && echo "Weak Auto Test OK"

ex%: ../examples/ex%.c
	$(CC) -o $@ $< $(CFLAGS)

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
	rm -vf *~ *.o *.a *.bin *.i *.ref *.out *.out? *.gcc \
	   tcctest[1234] ex? libtcc_test$(EXESUF) tcc_g tcclib.h
