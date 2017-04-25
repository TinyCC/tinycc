TOP = ../..
include $(TOP)/Makefile
SRC = $(TOPSRC)/tests/tests2
VPATH = $(SRC)

TESTS = $(patsubst %.c,%.test,$(sort $(notdir $(wildcard $(SRC)/*.c))))

# Some tests might need arguments
ARGS =
31_args.test : ARGS = arg1 arg2 arg3 arg4 arg5
46_grep.test : ARGS = '[^* ]*[:a:d: ]+\:\*-/: $$' $(SRC)/46_grep.c

# And some tests don't test the right thing with -run
NORUN =
42_function_pointer.test : NORUN = true

# Some tests might need different flags
FLAGS =
76_dollars_in_identifiers.test : FLAGS += -fdollars-in-identifiers

# Filter source directory in warnings/errors (out-of-tree builds)
FILTER = 2>&1 | sed 's,$(SRC)/,,g'
# Filter some always-warning
ifeq (-$(findstring arm,$(ARCH))-,-arm-)
FILTER += 2>&1 | grep -v 'warning: soft float ABI currently not supported'
endif

# some tests do not pass on all platforms, remove them for now
SKIP = 34_array_assignment.test # array assignment is not in C standard
ifeq ($(CONFIG_arm_eabi),yes) # not ARM soft-float
 SKIP += 22_floating_point.test
endif
ifdef CONFIG_OSX
 SKIP += 40_stdio.test 42_function_pointer.test
 FLAGS += -w
endif
ifeq ($(ARCH),x86_64)
 SKIP += 73_arm64.test
endif
ifeq (,$(filter i386 x86_64,$(ARCH)))
 SKIP += 85_asm-outside-function.test
endif

all test: $(filter-out $(SKIP),$(TESTS))

%.test: %.c %.expect
	@echo Test: $*...
# test -run
	@if test -z "$(NORUN)"; then \
	    $(TCC) $(FLAGS) -run $< $(ARGS) $(FILTER) >$*.output 2>&1 || true; \
	  else \
	    $(TCC) $(FLAGS) $< -o ./$*.exe $(FILTER) >$*.output 2>&1; \
	    ./$*.exe $(ARGS) >>$*.output 2>&1 || true; \
	  fi
	@diff -Nbu $(SRC)/$*.expect $*.output && rm -f $*.output $*.exe

# automatically generate .expect files with gcc:
%.expect : # %.c
	(gcc -w $*.c -o a.exe && ./a.exe $(ARGS)) $(FILTER) >$*.expect 2>&1; rm -f a.exe

# tell make not to delete
.PRECIOUS: %.expect

clean:
	rm -f fred.txt *.output a.exe
