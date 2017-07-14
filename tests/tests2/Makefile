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

# Always generate certain .expects (don't put these in the GIT),
GEN-ALWAYS = 95_bitfields.expect

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
ifeq (-$(findstring gcc,$(CC)-)-,--)
 SKIP += $(patsubst %.expect,%.test,$(GEN-ALWAYS))
endif
ifeq (-$(CONFIG_WIN32)-$(CONFIG_i386)$(CONFIG_arm)-,--yes-)
 SKIP += 95_bitfields_ms.test # type_align is differnt on 32bit-non-windows
endif

all test tests2.all: $(filter-out $(SKIP),$(TESTS)) ;

%.test: %.c %.expect
	@echo Test: $*...
	@$(if $(NORUN),\
	    ($(TCC) $(FLAGS) $< -o a.exe && ./a.exe $(ARGS)),\
	    $(TCC) $(FLAGS) -run $< $(ARGS)\
	   ) $(FILTER) >$*.output 2>&1 || true
	@diff -Nbu $(filter %.expect,$^) $*.output \
	    && rm -f $*.output $(filter $*.expect,$(GEN-ALWAYS))

F1 = $(or $(filter $1_%,$(TESTS)),$1_???.test)
F2 = $1 UPDATE="$(patsubst %.test,%.expect,$1)"

# run single test and update .expect file, e.g. "make tests2.37+"
tests2.%+:
	@$(MAKE) $(call F2,$(call F1,$*)) --no-print-directory

# run single test, e.g. "make tests2.37"
tests2.%:
	@$(MAKE) $(call F1,$*) --no-print-directory

# automatically generate .expect files with gcc:
%.expect :
	@echo Generating: $@
	@$(CC) -w -std=gnu99 $(FLAGS) $(SRC)/$*.c -o a.exe
	@./a.exe $(ARGS) $(FILTER) >$@ 2>&1
	@rm -f a.exe

# using the ms compiler for the really ms-compatible bitfields
MS-CC = cl
95_bitfields_ms.expect :
	@echo Generating: $@
	@$(MS-CC) $(basename $@).c
	@./$(basename $@).exe >$@ 2>&1
	@rm -f *.exe *.obj *.pdb

# tell make not to delete
.PRECIOUS: %.expect

$(sort $(GEN-ALWAYS) $(UPDATE)) : force
force:

clean:
	rm -f fred.txt *.output a.exe $(GEN-ALWAYS)
