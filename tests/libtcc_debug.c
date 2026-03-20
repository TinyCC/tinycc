#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libtcc.h"

static const char program[] =
"#include <stdio.h>\n"
"int fib(int n)\n"
"{\n"
"    if (n <= 2)\n"
"        return 1;\n"
"    else\n"
"        return fib(n-1) + fib(n-2);\n"
"}\n"
"int tst(void)\n"
"{\n"
"   int i;\n"
"   for (i = 2; i < 20; i++)\n"
"     printf(\"%d \", fib(i));\n"
"   printf(\"\\n\");\n"
"   return 0;\n"
"}\n";

void handle_error(void *opaque, const TCCErrorInfo *info)
{
    if (info->filename) {
        if (info->line_num)
            fprintf(opaque, "%s:%d: ", info->filename, info->line_num);
        else
            fprintf(opaque, "%s: ", info->filename);
    }
    fprintf(opaque, "%s: %s\n", info->is_warning ? "warning" : "error", info->msg);
}

int
main(void)
{
    int (*func)(void);
    TCCState *s = tcc_new(0);

    if (!s) {
        fprintf(stderr, __FILE__ ": could not create tcc state\n");
        return 1;
    }
#if 1
    /* If -g option is not set the debugging files tst.c en tst.o will
       not be created. */
    tcc_set_options(s, "-g");
#endif
    tcc_set_error_func(s, stdout, handle_error);
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    if (tcc_compile_string_file(s, program, "tst.c") == -1)
	return 1;
    if (tcc_relocate(s) < 0)
        return 1;
    elf_output_obj(s, "tst.o");
    /* set breakpoint on next line. and load symbol file with
       gdb command add-symbol-file.
       Then set breakpoint on tst and continue. */
    if ((func = tcc_get_symbol(s, "tst")))
        func();
    tcc_delete(s);
    return 0;
}
