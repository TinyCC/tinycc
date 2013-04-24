include_directories(${CMAKE_SOURCE_DIR})

add_executable(abitest-cc abitest.c)
target_link_libraries(abitest-cc libtcc)
add_test(NAME abitest-cc WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND abitest-cc lib_path=${CMAKE_BINARY_DIR} include=${CMAKE_SOURCE_DIR}/include)

set(ABITEST_TCC abitest-tcc${CMAKE_EXECUTABLE_SUFFIX})
get_property(LIBTCC_LIB TARGET libtcc PROPERTY LOCATION)
add_custom_command(OUTPUT ${ABITEST_TCC} COMMAND tcc ${TCC_CFLAGS} -g -I${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/abitest.c ${LIBTCC_LIB} -o ${ABITEST_TCC} DEPENDS abitest.c)
add_custom_target(abitest-tcc-exe ALL DEPENDS ${ABITEST_TCC})

add_test(NAME abitest-tcc WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${ABITEST_TCC} lib_path=${CMAKE_BINARY_DIR} include=${CMAKE_SOURCE_DIR}/include)

add_executable(tcctest-cc tcctest.c)
target_link_libraries(tcctest-cc libtcc)
set_target_properties(tcctest-cc PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR};${CMAKE_SOURCE_DIR}/include" COMPILE_FLAGS -std=gnu99)

set(TCC_TEST_CFLAGS ${TCC_CFLAGS} -B${CMAKE_BINARY_DIR} -I${CMAKE_BINARY_DIR})
if(WIN32)
  set(TCC_TEST_CFLAGS ${TCC_TEST_CFLAGS} -I${CMAKE_SOURCE_DIR}/win32/include/winapi)
endif()
set(TCC_TEST_SOURCE ${TCC_TEST_CFLAGS} -run ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.c)
set(TCC_TEST_RUN ${TCC_TEST_CFLAGS} ${TCC_TARGET_FLAGS} -DONE_SOURCE -run ${CMAKE_SOURCE_DIR}/tcc.c)

add_custom_command(OUTPUT tcctest.ref COMMAND tcctest-cc > tcctest.ref)
add_custom_command(OUTPUT tcctest.1 COMMAND tcc ${TCC_TEST_SOURCE} > tcctest.1)
add_custom_command(OUTPUT tcctest.2 COMMAND tcc ${TCC_TEST_RUN} ${TCC_TEST_SOURCE} > tcctest.2)
add_custom_command(OUTPUT tcctest.3 COMMAND tcc ${TCC_TEST_RUN} ${TCC_TEST_RUN} ${TCC_TEST_SOURCE} > tcctest.3)
add_custom_target(tcctest-ref ALL DEPENDS tcctest.ref tcctest.1 tcctest.2 tcctest.3)

find_program(DIFF diff C:/MinGW/msys/1.0/bin)

add_test(test1 ${DIFF} tcctest.ref tcctest.1)
add_test(test2 ${DIFF} tcctest.ref tcctest.2)
add_test(test3 ${DIFF} tcctest.ref tcctest.3)

set(MORETESTS
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
)
if(WIN32)
  list(REMOVE_ITEM MORETESTS 24_math_library)
  list(REMOVE_ITEM MORETESTS 28_strings)
endif()
foreach(testfile ${MORETESTS})
  add_test(NAME ${testfile} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests2
           COMMAND tcc ${TCC_CFLAGS} -run ${testfile}.c - arg1 arg2 arg3 arg4 | ${DIFF} - ${testfile}.expect)
endforeach()
