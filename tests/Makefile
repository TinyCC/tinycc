#
# Tiny C Compiler Makefile - tests
#

TOP = ..
include $(TOP)/Makefile
SRCDIR = $(top_srcdir)/tests
VPATH = $(SRCDIR) $(top_srcdir)

# clear CFLAGS and LDFLAGS
CFLAGS :=
LDFLAGS :=

# what tests to run
TESTS = \
 hello-exe \
 hello-run \
 libtest \
 test3 \
 abitest \
 vla_test-run \
 tests2-dir pp-dir

BTESTS = test1b test3b btest

ifdef CONFIG_CROSS
 TESTS += hello-cross
endif

# test4 -- problem with -static
# asmtest -- minor differences with gcc
# btest -- works on i386 (including win32)

# bounds-checking is supported only on i386
ifneq ($(ARCH),i386)
 TESTS := $(filter-out $(BTESTS),$(TESTS))
endif
ifdef CONFIG_WIN32
 TESTS := $(filter-out $(BTESTS),$(TESTS))
endif
ifeq ($(TARGETOS),Darwin)
 TESTS := $(filter-out hello-exe test3 $(BTESTS),$(TESTS))
endif
ifeq (,$(filter arm64 i386 x86-64,$(ARCH)))
 TESTS := $(filter-out vla_test-run,$(TESTS))
endif
ifeq ($(CONFIG_arm_eabi),yes)
 TESTS := $(filter-out test3,$(TESTS))
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
TCCFLAGS = -B$(TOP) -I$(TOP) -I$(top_srcdir) -I$(top_srcdir)/include -L$(TOP)
ifdef CONFIG_WIN32
 TCCFLAGS = -B$(top_srcdir)/win32 -I$(top_srcdir) -I$(top_srcdir)/include -L$(TOP)
endif
XTCCFLAGS = -B$(TOP) -B$(top_srcdir)/win32 -I$(TOP) -I$(top_srcdir) -I$(top_srcdir)/include

TCC = $(TOP)/tcc $(TCCFLAGS)
RUN_TCC = $(NATIVE_DEFINES) -DONE_SOURCE -run $(top_srcdir)/tcc.c $(TCCFLAGS)

DISAS = objdump -d

# libtcc test
ifdef LIBTCC1
 ifdef CONFIG_WIN32
    LIBTCC1:=$(TOP)/win32/libtcc/libtcc.a
 else
    LIBTCC1:=$(TOP)/$(LIBTCC1)
 endif
endif

all test : $(TESTS)

hello-exe: ../examples/ex1.c
	@echo ------------ $@ ------------
	$(TCC) $< -o hello$(EXESUF) || ($(TOP)/tcc -vv; exit 1) && ./hello$(EXESUF)

hello-cross: ../examples/ex1.c
	@echo ------------ $@ ------------
	for XTCC in $(PROGS_CROSS) ; \
		do echo -n "Test of $$XTCC...  "; \
		out=$$($(TOP)/$$XTCC $(XTCCFLAGS) -c $< 2>&1); \
		test $$? -ne 0 && { echo "Failed\n$$out\n" ; $(TOP)/$$XTCC -vv; exit 1; } ; \
		echo "Success"; \
	done

hello-run: ../examples/ex1.c
	@echo ------------ $@ ------------
	$(TCC) -run $<

libtest: libtcc_test$(EXESUF) $(LIBTCC1)
	@echo ------------ $@ ------------
	./libtcc_test$(EXESUF) $(TCCFLAGS)

libtcc_test$(EXESUF): libtcc_test.c $(top_builddir)/$(LIBTCC)
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(NATIVE_DEFINES) $(LIBS) $(LINK_LIBTCC) $(LDFLAGS) -I$(top_srcdir)

%-dir:
	@echo ------------ $@ ------------
	$(MAKE) -k -C $*

# test.ref - generate using cc
test.ref: tcctest.c
	$(CC) -o tcctest.cc $< -I$(top_srcdir) $(CPPFLAGS) -w $(CFLAGS) $(NATIVE_DEFINES) -std=gnu99 -O0 -fno-omit-frame-pointer $(LDFLAGS)
	./tcctest.cc > $@

# auto test
test1 test1b: tcctest.c test.ref
	@echo ------------ $@ ------------
	$(TCC) -run $< > test.out1
	@diff -u test.ref test.out1 && echo "Auto Test OK"

# iterated test2 (compile tcc then compile tcctest.c !)
test2 test2b: tcctest.c test.ref
	@echo ------------ $@ ------------
	$(TCC) $(RUN_TCC) $(RUN_TCC) -run $< > test.out2
	@diff -u test.ref test.out2 && echo "Auto Test2 OK"

# iterated test3 (compile tcc then compile tcc then compile tcctest.c !)
test3 test3b: tcctest.c test.ref
	@echo ------------ $@ ------------
	$(TCC) $(RUN_TCC) $(RUN_TCC) $(RUN_TCC) -run $< > test.out3
	@diff -u test.ref test.out3 && echo "Auto Test3 OK"

test%b : TCCFLAGS += -b

# binary output test
test4: tcctest.c test.ref
	@echo ------------ $@ ------------
# object + link output
	$(TCC) -c -o tcctest3.o $<
	$(TCC) -o tcctest3 tcctest3.o
	./tcctest3 > test3.out
	@if diff -u test.ref test3.out ; then echo "Object Auto Test OK"; fi
# dynamic output
	$(TCC) -o tcctest1 $<
	./tcctest1 > test1.out
	@if diff -u test.ref test1.out ; then echo "Dynamic Auto Test OK"; fi
# dynamic output + bound check
	$(TCC) -b -o tcctest4 $<
	./tcctest4 > test4.out
	@if diff -u test.ref test4.out ; then echo "BCheck Auto Test OK"; fi
# static output
	$(TCC) -static -o tcctest2 $<
	./tcctest2 > test2.out
	@if diff -u test.ref test2.out ; then echo "Static Auto Test OK"; fi

# memory and bound check auto test
BOUNDS_OK  = 1 4 8 10 14
BOUNDS_FAIL= 2 5 7 9 11 12 13 15

btest: boundtest.c
	@echo ------------ $@ ------------
	@for i in $(BOUNDS_OK); do \
	   echo ; echo --- boundtest $$i ---; \
	   if $(TCC) -b -run $< $$i ; then \
	       echo succeeded as expected; \
	   else\
	       echo Failed positive test $$i ; exit 1 ; \
	   fi ;\
	done ;\
	for i in $(BOUNDS_FAIL); do \
	   echo ; echo --- boundtest $$i ---; \
	   if $(TCC) -b -run $< $$i ; then \
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

weaktest: tcctest.c test.ref
	$(TCC) -c $< -o weaktest.tcc.o $(CPPFLAGS) $(CFLAGS)
	 $(CC) -c $< -o weaktest.cc.o -I. $(CPPFLAGS) -w $(CFLAGS)
	objdump -t weaktest.tcc.o | grep ' w ' | sed -e 's/.* \([a-zA-Z0-9_]*\)$$/\1/' | LC_ALL=C sort > weaktest.tcc.o.txt
	objdump -t weaktest.cc.o | grep ' w ' | sed -e 's/.* \([a-zA-Z0-9_]*\)$$/\1/' | LC_ALL=C sort > weaktest.cc.o.txt
	diff weaktest.cc.o.txt weaktest.tcc.o.txt && echo "Weak Auto Test OK"

ex%: $(top_srcdir)/examples/ex%.c
	$(CC) -o $@ $< $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)

# tiny assembler testing
asmtest.ref: asmtest.S
	$(CC) -m32 -Wa,-W -o asmtest.ref.o -c asmtest.S
	objdump -D asmtest.ref.o > asmtest.ref

asmtest: asmtest.ref
	@echo ------------ $@ ------------
	$(TCC) -c asmtest.S
	objdump -D asmtest.o > asmtest.out
	@if diff -u --ignore-matching-lines="file format" asmtest.ref asmtest.out ; then echo "ASM Auto Test OK"; fi

# Check that code generated by libtcc is binary compatible with
# that generated by CC
abitest-cc$(EXESUF): abitest.c $(top_builddir)/$(LIBTCC)
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(NATIVE_DEFINES) $(LIBS) $(LINK_LIBTCC) $(LDFLAGS) -I$(top_srcdir)

abitest-tcc$(EXESUF): abitest.c libtcc.c
	$(TCC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(NATIVE_DEFINES) -DONE_SOURCE $(LIBS) $(LDFLAGS) -I$(top_srcdir)

ABITESTS := abitest-cc$(EXESUF)
ifneq ($(CONFIG_arm_eabi),yes) # not ARM soft-float
	ABITESTS += abitest-tcc$(EXESUF)
endif

abitest: $(ABITESTS)
	@echo ------------ $@ ------------
	./abitest-cc$(EXESUF) $(TCCFLAGS)
	if [ "$(CONFIG_arm_eabi)" != "yes" ]; then ./abitest-tcc$(EXESUF) $(TCCFLAGS); fi

vla_test$(EXESUF): vla_test.c
	$(TCC) -o $@ $^ $(CPPFLAGS) $(CFLAGS)
vla_test-run: vla_test$(EXESUF)
	@echo ------------ $@ ------------
	./vla_test$(EXESUF)

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
	rm -vf *~ *.o *.a *.bin *.i *.ref *.out *.out? *.out?b *.cc \
		*-cc *-tcc *.exe \
		hello libtcc_test vla_test tcctest[1234] ex? tcc_g
