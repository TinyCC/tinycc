TOP = ../..
include $(TOP)/Makefile

# run local version of tcc with local libraries and includes
TCCFLAGS = -B$(TOP) -I$(TOP)
ifdef CONFIG_WIN32
 TCCFLAGS = -B$(TOP)/win32 -I$(TOP)/include -I$(TOP) -L$(TOP)
endif
TCC = $(TOP)/tcc $(TCCFLAGS)

TESTS = $(patsubst %.c,%.test,$(sort $(wildcard *.c)))

# 34_array_assignment.test -- array assignment is not in C standard
SKIP = 34_array_assignment.test

# some tests do not pass on all platforms, remove them for now
ifeq ($(CONFIG_arm_eabi),yes) # not ARM soft-float
 SKIP += 22_floating_point.test
endif
ifeq ($(TARGETOS),Darwin)
 SKIP += 40_stdio.test
endif
ifdef CONFIG_WIN32
 SKIP += 24_math_library.test # don't have round()
 SKIP += 28_strings.test # don't have r/index() / strings.h
endif
ifeq ($(ARCH),x86-64)
 SKIP += 73_arm64.test
endif
ifeq (,$(filter i386 x86-64,$(ARCH)))
 SKIP += 85-asm-outside-function.test
endif

# Some tests might need arguments
ARGS =
31_args.test : ARGS = arg1 arg2 arg3 arg4 arg5
46_grep.test : ARGS = '[^* ]*[:a:d: ]+\:\*-/: $$' 46_grep.c

# Some tests might need different flags
76_dollars_in_identifiers.test : TCCFLAGS += -fdollars-in-identifiers

# Filter some always-warning
FILTER =
ifeq (-$(findstring arm,$(ARCH))-,-arm-)
FILTER = 2>&1 | grep -v 'warning: soft float ABI currently not supported'
endif

all test: $(filter-out $(SKIP),$(TESTS))

%.test: %.c %.expect
	@echo Test: $*...
# test -run
	@$(TCC) -run $< $(ARGS) $(FILTER) >$*.output 2>&1 || true
	@diff -Nbu $*.expect $*.output && rm -f $*.output
# test exe (disabled for speed)
#	 @($(TCC) $(FLAGS) $< -o $*.exe && ./$*.exe $(ARGS)) $(FiLTER) >$*.output2 2>&1 ; \
#	     diff -Nbu $*.expect $*.output2 && rm -f $*.output2 $*.exe

# automatically generate .expect files with gcc:
%.expect :
	(gcc $*.c -o a.exe && ./a.exe $(ARGS)) >$*.expect 2>&1; rm -f a.exe

# tell make not to delete
.PRECIOUS: %.expect

clean:
	rm -f fred.txt *.output a.exe
