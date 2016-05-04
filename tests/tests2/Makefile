TOP = ../..
include $(TOP)/Makefile

# clear CFLAGS and LDFLAGS
CFLAGS :=
LDFLAGS :=

ifdef CONFIG_WIN32
 TCCFLAGS = -B$(top_srcdir)/win32 -I$(top_srcdir)/include -L$(TOP)
else
 TCCFLAGS = -B$(TOP) -I$(top_srcdir)/include -lm
endif

ifeq ($(TARGETOS),Darwin)
 CFLAGS += -Wl,-flat_namespace,-undefined,warning
 TCCFLAGS += -D_ANSI_SOURCE
 export MACOSX_DEPLOYMENT_TARGET:=10.2
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
FLAGS =
76_dollars_in_identifiers.test : FLAGS = -fdollars-in-identifiers

# Filter some always-warning
FILTER =
ifeq (-$(findstring arm,$(ARCH))-,-arm-)
FILTER = 2>&1 | grep -v 'warning: soft float ABI currently not supported'
endif

all test: $(filter-out $(SKIP),$(TESTS))

%.test: %.c %.expect
	@echo Test: $*...
# test -run
	@$(TCC) $(FLAGS) -run $< $(ARGS) $(FILTER) >$*.output 2>&1 || true
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
	rm -vf fred.txt *.output a.exe
