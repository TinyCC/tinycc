#!/bin/bash
set -e

CC="${CC:-emcc}"
CFLAGS="-DTCC_SYNTAX_ONLY -DTCC_TARGET_X86_64 -DPTR_SIZE=8 -DLONG_SIZE=8 -O3 -DONE_SOURCE=1"
LDFLAGS="-sINITIAL_MEMORY=33554432 -sEXPORTED_RUNTIME_METHODS=ccall,UTF8ToString,stackAlloc -sEXPORTED_FUNCTIONS=_check_syntax,_get_type_info_buffer,_get_type_info_length,_get_debug_calls_buffer,_get_debug_calls_length,_get_error_count,_get_error,_get_error_filename,_get_error_line_num,_get_error_is_warning,_get_error_msg -sENVIRONMENT=worker -sEXIT_RUNTIME=0 -sSTRICT=1"
OUTPUT="syntax_check.mjs"
WARNS="-Wno-string-plus-int -Wno-undefined-internal"

rm -f $OUTPUT syntax_check.wasm

mv em_include/task.h.pch .
$CC -sEXPORT_NAME=TCC -sMODULARIZE=1 -sEVAL_CTORS -sWASM_BIGINT -flto $WARNS $CFLAGS $LDFLAGS -o $OUTPUT syntax_check.c libtcc.c tccpp.c tccgen.c tcc-stubs.c -I. --preload-file em_include@/
mv task.h.pch em_include/
