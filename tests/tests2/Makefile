TOP = ../..
include $(TOP)/Makefile
VPATH = $(top_srcdir)/tests/tests2

TCCFLAGS = -B$(TOP)
ifdef CONFIG_WIN32
 TCCFLAGS = -B$(top_srcdir)/win32 -I$(top_srcdir)/include -L$(TOP)
endif

ifeq ($(TARGETOS),Darwin)
 CFLAGS += -Wl,-flat_namespace,-undefined,warning
 TCCFLAGS += -D_ANSI_SOURCE
 export MACOSX_DEPLOYMENT_TARGET:=10.2
endif

TCC_RUN = $(TOP)/tcc $(TCCFLAGS) -run

TESTS =	\
 00_assignment.test \
 01_comment.test \
 02_printf.test \
 03_struct.test \
 04_for.test \
 05_array.test \
 06_case.test \
 07_function.test \
 08_while.test \
 09_do_while.test \
 10_pointer.test \
 11_precedence.test \
 12_hashdefine.test \
 13_integer_literals.test \
 14_if.test \
 15_recursion.test \
 16_nesting.test \
 17_enum.test \
 18_include.test \
 19_pointer_arithmetic.test \
 20_pointer_comparison.test \
 21_char_array.test \
 22_floating_point.test \
 23_type_coercion.test \
 24_math_library.test \
 25_quicksort.test \
 26_character_constants.test \
 27_sizeof.test \
 28_strings.test \
 29_array_address.test \
 31_args.test \
 32_led.test \
 33_ternary_op.test \
 35_sizeof.test \
 36_array_initialisers.test \
 37_sprintf.test \
 38_multiple_array_index.test \
 39_typedef.test \
 40_stdio.test \
 41_hashif.test \
 42_function_pointer.test \
 43_void_param.test \
 44_scoped_declarations.test \
 45_empty_for.test \
 47_switch_return.test \
 48_nested_break.test \
 49_bracket_evaluation.test \
 50_logical_second_arg.test \
 51_static.test \
 52_unnamed_enum.test \
 54_goto.test \
 55_lshift_type.test

# 30_hanoi.test -- seg fault in the code, gcc as well
# 34_array_assignment.test -- array assignment is not in C standard
# 46_grep.test -- does not compile even with gcc

# some tests do not pass on all platforms, remove them for now
ifeq ($(TARGETOS),Darwin)
  TESTS := $(filter-out 40_stdio.test,$(TESTS))
endif
ifdef CONFIG_WIN32
  TESTS := $(filter-out 24_math_library.test 28_strings.test,$(TESTS))
endif

%.test: %.c %.expect
	@echo Test: $*...
	@if [ "x`echo $* | grep args`" != "x" ]; \
	then $(TCC_RUN) $< - arg1 arg2 arg3 arg4 >$*.output; \
	else $(TCC_RUN) $< >$*.output; \
	fi
	@if diff -bu $(<:.c=.expect) $*.output ; \
	then rm -f $*.output; \
	else exit 1; \
	fi

all test: $(TESTS)

clean:
	rm -vf fred.txt *.output
