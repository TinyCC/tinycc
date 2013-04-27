include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

set(TCC_CFLAGS -I${CMAKE_SOURCE_DIR} -I${CMAKE_SOURCE_DIR}/include -B${CMAKE_BINARY_DIR})
if(WIN32)
  set(TCC_CFLAGS ${TCC_CFLAGS} -I${CMAKE_SOURCE_DIR}/win32/include)
else()
  set(TCC_MATH_LDFLAGS -lm)
  set(LIBTCC_EXTRA_LIBS dl)
  set(LIBTCC_LDFLAGS -ldl -lm -Wl,-rpath=${CMAKE_BINARY_DIR})
endif()

add_executable(abitest-cc abitest.c)
target_link_libraries(abitest-cc libtcc ${LIBTCC_EXTRA_LIBS})
add_test(NAME abitest-cc WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND abitest-cc lib_path=${CMAKE_BINARY_DIR} include=${CMAKE_SOURCE_DIR}/include)

set(ABITEST_TCC abitest-tcc${CMAKE_EXECUTABLE_SUFFIX})
get_property(LIBTCC_LIB TARGET libtcc PROPERTY LOCATION)
add_custom_command(OUTPUT ${ABITEST_TCC} COMMAND tcc ${TCC_CFLAGS} -g ${CMAKE_CURRENT_SOURCE_DIR}/abitest.c ${LIBTCC_LDFLAGS} ${LIBTCC_LIB} -o ${ABITEST_TCC} DEPENDS tcc ${CMAKE_CURRENT_SOURCE_DIR}/abitest.c)
add_custom_target(abitest-tcc-exe ALL DEPENDS ${ABITEST_TCC})

add_test(NAME abitest-tcc WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${ABITEST_TCC} lib_path=${CMAKE_BINARY_DIR} include=${CMAKE_SOURCE_DIR}/include)

set(VLA_TEST vla_test${CMAKE_EXECUTABLE_SUFFIX})
add_custom_command(OUTPUT ${VLA_TEST} COMMAND tcc ${TCC_CFLAGS} -g ${CMAKE_CURRENT_SOURCE_DIR}/vla_test.c -o ${VLA_TEST} DEPENDS tcc ${CMAKE_CURRENT_SOURCE_DIR}/vla_test.c)
add_custom_target(vla_test-exe ALL DEPENDS ${VLA_TEST})
add_test(vla_test vla_test)

add_executable(tcctest-cc tcctest.c)
target_link_libraries(tcctest-cc libtcc)
set_target_properties(tcctest-cc PROPERTIES COMPILE_FLAGS -std=gnu99)

find_package(PythonInterp)
if(PYTHONINTERP_FOUND)
  set(TCC_TEST_CFLAGS ${TCC_CFLAGS} -B${CMAKE_BINARY_DIR} -I${CMAKE_BINARY_DIR})
  if(WIN32)
    set(TCC_TEST_CFLAGS ${TCC_TEST_CFLAGS} -I${CMAKE_SOURCE_DIR}/win32/include/winapi)
  endif()
  set(TCC_TEST_SOURCE ${TCC_TEST_CFLAGS} ${TCC_MATH_LDFLAGS} -run ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.c)
  set(TCC_TEST_RUN ${TCC_TEST_CFLAGS} ${TCC_NATIVE_FLAGS} -DONE_SOURCE -run ${CMAKE_SOURCE_DIR}/tcc.c)
  get_property(TCC TARGET tcc PROPERTY LOCATION)
  get_property(TCCTESTCC TARGET tcctest-cc PROPERTY LOCATION)
  set(TCCTEST_PY ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.py ${TCCTESTCC})
  add_test(test1 ${TCCTEST_PY} ${TCC} ${TCC_TEST_SOURCE})
  add_test(test2 ${TCCTEST_PY} ${TCC} ${TCC_TEST_RUN} ${TCC_TEST_SOURCE})
  add_test(test3 ${TCCTEST_PY} ${TCC} ${TCC_TEST_RUN} ${TCC_TEST_RUN} ${TCC_TEST_SOURCE})

  # Object + link output
  set(TEST4 test4${CMAKE_EXECUTABLE_SUFFIX})
  add_custom_command(OUTPUT test4.o COMMAND tcc ${TCC_TEST_CFLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.c -c -o test4.o DEPENDS tcc ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.c)
  add_custom_command(OUTPUT ${TEST4} COMMAND tcc ${TCC_TEST_CFLAGS} test4.o -o ${TEST4} DEPENDS tcc test4.o)
  add_custom_target(test4-exe ALL DEPENDS ${TEST4})
  add_test(test4 ${TCCTEST_PY} ${CMAKE_CURRENT_BINARY_DIR}/${TEST4})

  # Dynamic output
  set(TEST5 test5${CMAKE_EXECUTABLE_SUFFIX})
  add_custom_command(OUTPUT ${TEST5} COMMAND tcc ${TCC_TEST_CFLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.c -o ${TEST5} DEPENDS tcc ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.c)
  add_custom_target(test5-exe ALL DEPENDS ${TEST5})
  add_test(test5 ${TCCTEST_PY} ${CMAKE_CURRENT_BINARY_DIR}/${TEST5})

  if(TCC_BCHECK)
    # Dynamic output + bound check
    set(TEST6 test6${CMAKE_EXECUTABLE_SUFFIX})
    add_custom_command(OUTPUT ${TEST6} COMMAND tcc ${TCC_TEST_CFLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.c -b -o ${TEST6} DEPENDS tcc ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.c)
    add_custom_target(test6-exe ALL DEPENDS ${TEST6})
    add_test(test6 ${TCCTEST_PY} ${CMAKE_CURRENT_BINARY_DIR}/${TEST6})
  endif()

  if(0)
    # Static output
    set(TEST7 test7${CMAKE_EXECUTABLE_SUFFIX})
    add_custom_command(OUTPUT ${TEST7} COMMAND tcc ${TCC_TEST_CFLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.c -static -o ${TEST7} DEPENDS tcc ${CMAKE_CURRENT_SOURCE_DIR}/tcctest.c)
    add_custom_target(test7-exe ALL DEPENDS ${TEST7})
    add_test(test7 ${TCCTEST_PY} ${CMAKE_CURRENT_BINARY_DIR}/${TEST7})
  endif()
endif()

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
           COMMAND tcc ${TCC_CFLAGS} ${TCC_MATH_LDFLAGS} -run ${testfile}.c - arg1 arg2 arg3 arg4 | ${DIFF} - ${testfile}.expect)
endforeach()
