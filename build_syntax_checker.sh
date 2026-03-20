#!/bin/bash
set -e

CC="${CC:-musl-gcc}"
# on O0 missing dead code elimination triggers linker errors
CFLAGS="-DTCC_SYNTAX_ONLY -ggdb -O2 -DONE_SOURCE=1"
LDFLAGS="-static"
OUTPUT="syntax_check_file"
LIBRARY="libtcc-syntax.a"

$CC $CFLAGS $LDFLAGS -o $OUTPUT syntax_check_file.c tcc-stubs.c libtcc.c tccpp.c tccgen.c -I.
