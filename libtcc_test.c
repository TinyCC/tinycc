/*
 * Simple Test program for libtcc
 *
 * libtcc can be useful to use tcc as a "backend" for a code generator.
 */
#include <stdlib.h>
#include <stdio.h>

#include "libtcc.h"

/* this function is called by the generated code */
int add(int a, int b)
{
    return a + b;
}

char my_program[] = 
"int fib(int n)\n"
"{\n"
"    if (n <= 2)\n"
"        return 1;\n"
"    else\n"
"        return fib(n-1) + fib(n-2);\n"
"}\n"
"\n"
"int foo(int n)\n"
"{\n"
"    printf(\"Hello World!\\n\");\n"
"    printf(\"fib(%d) = %d\\n\", n, fib(n));\n"
"    printf(\"add(%d, %d) = %d\\n\", n, 2 * n, add(n, 2 * n));\n"
"    return 0;\n"
"}\n";

int main(int argc, char **argv)
{
    TCCState *s;
    int (*func)(int);
    unsigned long val;
    
    s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        exit(1);
    }

    /* MUST BE CALLED before any compilation or file loading */
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    tcc_compile_string(s, my_program);

    /* as a test, we add a symbol that the compiled program can be
       linked with. You can have a similar result by opening a dll
       with tcc_add_dll(() and using its symbols directly. */
    tcc_add_symbol(s, "add", (unsigned long)&add);
    
    tcc_relocate(s);

    tcc_get_symbol(s, &val, "foo");
    func = (void *)val;

    func(32);

    tcc_delete(s);
    return 0;
}
