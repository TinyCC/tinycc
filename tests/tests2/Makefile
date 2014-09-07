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
 30_hanoi.test \
 31_args.test \
 32_led.test \
 33_ternary_op.test \
 34_array_assignment.test \
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
 46_grep.test \
 47_switch_return.test \
 48_nested_break.test \
 49_bracket_evaluation.test \
 50_logical_second_arg.test \
 51_static.test \
 52_unnamed_enum.test \
 54_goto.test \
 55_lshift_type.test \
 56_btype_excess-1.test \
 57_btype_excess-2.test \
 58_function_redefinition.test \
 59_function_array.test \
 60_enum_redefinition.test \
 61_undefined_enum.test \
 62_enumerator_redefinition.test \
 63_local_enumerator_redefinition.test \
 64_macro_nesting.test \
 65_macro_concat_start.test \
 66_macro_concat_end.test \
 67_macro_concat.test \
 68_macro_param_list_err_1.test \
 69_macro_param_list_err_2.test

# 34_array_assignment.test -- array assignment is not in C standard

SKIP = 34_array_assignment.test

# some tests do not pass on all platforms, remove them for now
ifeq ($(TARGETOS),Darwin)
 SKIP += 40_stdio.test
endif
ifdef CONFIG_WIN32
 SKIP += 24_math_library.test # don't have round()
 SKIP += 28_strings.test # don't have r/index() / strings.h
endif

# Some tests might need arguments
ARGS =
31_args.test : ARGS = arg1 arg2 arg3 arg4 arg5
46_grep.test : ARGS = '[^* ]*[:a:d: ]+\:\*-/: $$' 46_grep.c

all test: $(filter-out $(SKIP),$(TESTS))

%.test: %.c %.expect
	@echo Test: $*...

	@$(TCC) -run $< $(ARGS) >$*.output 2>&1 || true
	@diff -bu $*.expect $*.output && rm -f $*.output

	@($(TCC) $< -o $*.exe && ./$*.exe $(ARGS)) >$*.output2 2>&1 || true
	@diff -bu $*.expect $*.output2 && rm -f $*.output2 $*.exe

clean:
	rm -vf fred.txt *.output* *.exe
