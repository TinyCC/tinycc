#
# Tiny C Compiler Makefile - tests
#

# what tests to run
TESTS = libtest test3

# these should work too
# TESTS += test1 test2 speed

# these don't work as they should
# TESTS += test4 btest asmtest


TOP = ..
include $(TOP)/Makefile

# run local version of tcc with local libraries and includes
TCC = ../tcc -B..
RUN_TCC = $(NATIVE_TARGET) -run ../tcc.c -B..
DISAS=objdump -d

all test : $(TESTS)

# make sure that tcc exists
$(TESTS) : ../tcc
../tcc ../libtcc.a :
	$(MAKE) -C ..

# libtcc test
libtest: libtcc_test$(EXESUF)
	@echo ------------ $@ ------------
	./libtcc_test lib_path=..

libtcc_test$(EXESUF): libtcc_test.c ../libtcc.a
	$(CC) -o $@ $^ -I.. $(CFLAGS) $(LIBS)

# test.ref - generate using gcc
test.ref: tcctest.c
	cp -u ../include/tcclib.h .
	$(CC) -o tcctest.gcc $< -I. -w $(CFLAGS)
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
BOUNDS_OK  = 1 4 8 10
BOUNDS_FAIL= 2 5 7 9 11 12 13

btest: boundtest.c
	@echo ------------ $@ ------------
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
	   fi ;\
	done ;\
	echo Bound test OK

# speed test
speed: ex2 ex3
	@echo ------------ $@ ------------
	time ./ex2 1238 2 3 4 10 13 4
	time $(TCC) -run ../examples/ex2.c 1238 2 3 4 10 13 4
	time ./ex3 35
	time $(TCC) -run ../examples/ex3.c 35

ex%: ../examples/ex%.c
	$(CC) -o $@ $< $(CFLAGS)

# tiny assembler testing
asmtest.ref: asmtest.S
	$(CC) -o asmtest.ref.o -c asmtest.S
	objdump -D asmtest.ref.o > $@

asmtest: asmtest.ref
	@echo ------------ $@ ------------
	$(TCC) -c asmtest.S
	objdump -D asmtest.o > $@
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
