include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

set(TCC_CFLAGS -s -r -w -I${CMAKE_SOURCE_DIR} -I${CMAKE_SOURCE_DIR}/include -B${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
if(WIN32)
  set(TCC_CFLAGS ${TCC_CFLAGS} -I${CMAKE_SOURCE_DIR}/win32/include)
else()
  set(TCC_MATH_LDFLAGS -lm)
  set(LIBTCC_EXTRA_LIBS dl)
  set(LIBTCC_LDFLAGS -ldl -lm -Wl,-rpath=${CMAKE_BINARY_DIR})
endif()

if( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL i386 )
    set(TCC_CFLAGS ${TCC_CFLAGS} -fno-PIC -fno-PIE -Wl,-z,notext )
elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm_eabi
    OR ${CMAKE_TARGET_ARCHITECTURES} STREQUAL armv7
    OR ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64
    OR ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm )
        set(TCC_CFLAGS ${TCC_CFLAGS} -marm )
endif()

set(TESTS2
 00_assignment
 01_comment
 02_printf
 03_struct
 04_for
 05_array
 06_case
 07_function
 08_while
 09_do_while
 10_pointer
 11_precedence
 12_hashdefine
 13_integer_literals
 14_if
 15_recursion
 16_nesting
 17_enum
 18_include
 19_pointer_arithmetic
 20_pointer_comparison
 21_char_array
 22_floating_point
 23_type_coercion
 24_math_library
 25_quicksort
 26_character_constants
 27_sizeof
 28_strings
 29_array_address
 31_args
 32_led
 33_ternary_op
 35_sizeof
 36_array_initialisers
 37_sprintf
 38_multiple_array_index
 39_typedef
 40_stdio
 41_hashif
 42_function_pointer
 43_void_param
 44_scoped_declarations
 45_empty_for
 47_switch_return
 48_nested_break
 49_bracket_evaluation
 50_logical_second_arg
 51_static
 52_unnamed_enum
 54_goto
 55_lshift_type
 60_errors_and_warnings
 61_integers
 64_macro_nesting
 67_macro_concat
 70_floating_point_literals
 71_macro_empty_arg
 72_long_long_constant
 73_arm64
 75_array_in_struct_init
 76_dollars_in_identifiers
 77_push_pop_macro
 78_vla_label
 79_vla_continue
 80_flexarray
 81_types
 82_attribs_position
 83_utf8_in_identifiers
 84_hex-float
 85_asm-outside-function
 86_memory-model
 87_dead_code
 88_codeopt
 89_nocode_wanted
 90_struct-init
 91_ptr_longlong_arith32
 92_enum_bitfield
 93_integer_promotion
 94_generic
 95_bitfields
 95_bitfields_ms
 96_nodata_wanted
 97_utf8_string_literal
 98_al_ax_extend
 99_fastcall
 100_c99array-decls
 101_cleanup
 102_alignas
 103_implicit_memmove
 104+_inline
 104_inline
 105_local_extern
 106_versym
 107_stack_safe
 108_constructor
 109_float_struct_calling
 110_average
 111_conversion
 112_backtrace
 113_btdll
 114_bound_signal
 115_bound_setjmp
 116_bound_setjmp2
 117_builtins
 118_switch
 119_random_stuff
 120+_alias
 120_alias
 121_struct_return
 122_vla_reuse
 123_vla_bug
 124_atomic_counter
 125_atomic_misc
 126_bound_global
 127_asm_goto
 128_run_atexit
 129_scopes
 130_large_argument
 131_return_struct_in_reg
 132_bound_test
)

list(REMOVE_ITEM TESTS2 60_errors_and_warnings)
list(REMOVE_ITEM TESTS2 96_nodata_wanted)
list(REMOVE_ITEM TESTS2 99_fastcall)
list(REMOVE_ITEM TESTS2 101_cleanup)
list(REMOVE_ITEM TESTS2 104+_inline)
list(REMOVE_ITEM TESTS2 104_inline)
list(REMOVE_ITEM TESTS2 113_btdll)
list(REMOVE_ITEM TESTS2 120+_alias)
list(REMOVE_ITEM TESTS2 120_alias)
list(REMOVE_ITEM TESTS2 125_atomic_misc)
list(REMOVE_ITEM TESTS2 128_run_atexit)

if(WIN32)
  list(REMOVE_ITEM TESTS2 24_math_library)
  list(REMOVE_ITEM TESTS2 28_strings)
  list(REMOVE_ITEM TESTS2 106_versym)
  list(REMOVE_ITEM TESTS2 114_bound_signal)
  list(REMOVE_ITEM TESTS2 116_bound_setjmp2)
  list(REMOVE_ITEM TESTS2 117_builtins)
  list(REMOVE_ITEM TESTS2 124_atomic_counter)
elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64
  OR ${CMAKE_TARGET_ARCHITECTURES} STREQUAL riscv64
  OR ${CMAKE_TARGET_ARCHITECTURES} STREQUAL armv7
  OR ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm_eabi
  OR ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm )
    list(REMOVE_ITEM TESTS2 85_asm-outside-function )
    list(REMOVE_ITEM TESTS2 98_al_ax_extend )
    list(REMOVE_ITEM TESTS2 127_asm_goto )
elseif(APPLE)
    list(REMOVE_ITEM TESTS2 114_bound_signal )
    list(REMOVE_ITEM TESTS2 115_bound_setjmp )
    list(REMOVE_ITEM TESTS2 116_bound_setjmp2 )
    list(REMOVE_ITEM TESTS2 117_builtins )
    list(REMOVE_ITEM TESTS2 124_atomic_counter )
endif()

foreach(testfile ${TESTS2})
  add_test(NAME ${testfile} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests2
           COMMAND tcc ${TCC_CFLAGS} ${TCC_MATH_LDFLAGS} -run ${testfile}.c - arg1 arg2 arg3 arg4 | ${DIFF} - ${testfile}.expect)
endforeach()
