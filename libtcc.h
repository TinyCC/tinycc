#ifndef LIBTCC_H
#define LIBTCC_H

#ifdef __cplusplus
extern "C" {
#endif

struct TCCState;

typedef struct TCCState TCCState;

/* create a new TCC compilation context */
TCCState *tcc_new(void);

/* free a TCC compilation context */
void tcc_delete(TCCState *s);

/* add debug information in the generated code */
void tcc_enable_debug(TCCState *s);

/* set error/warning display callback */
void tcc_set_error_func(TCCState *s, void *error_opaque,
                        void (*error_func)(void *opaque, const char *msg));

/* set/reset a warning */
int tcc_set_warning(TCCState *s, const char *warning_name, int value);

/*****************************/
/* preprocessor */

/* add include path */
int tcc_add_include_path(TCCState *s, const char *pathname);

/* add in system include path */
int tcc_add_sysinclude_path(TCCState *s, const char *pathname);

/* define preprocessor symbol 'sym'. Can put optional value */
void tcc_define_symbol(TCCState *s, const char *sym, const char *value);

/* undefine preprocess symbol 'sym' */
void tcc_undefine_symbol(TCCState *s, const char *sym);

/*****************************/
/* compiling */

/* add a file (either a C file, dll, an object, a library or an ld
   script). Return -1 if error. */
int tcc_add_file(TCCState *s, const char *filename);

/* compile a string containing a C source. Return non zero if
   error. */
int tcc_compile_string(TCCState *s, const char *buf);

/*****************************/
/* linking commands */

/* set output type. MUST BE CALLED before any compilation */
#define TCC_OUTPUT_MEMORY   0 /* output will be ran in memory (no
                                 output file) (default) */
#define TCC_OUTPUT_EXE      1 /* executable file */
#define TCC_OUTPUT_DLL      2 /* dynamic library */
#define TCC_OUTPUT_OBJ      3 /* object file */
int tcc_set_output_type(TCCState *s, int output_type);

#define TCC_OUTPUT_FORMAT_ELF    0 /* default output format: ELF */
#define TCC_OUTPUT_FORMAT_BINARY 1 /* binary image output */
#define TCC_OUTPUT_FORMAT_COFF   2 /* COFF */

/* equivalent to -Lpath option */
int tcc_add_library_path(TCCState *s, const char *pathname);

/* the library name is the same as the argument of the '-l' option */
int tcc_add_library(TCCState *s, const char *libraryname);

/* add a symbol to the compiled program */
int tcc_add_symbol(TCCState *s, const char *name, unsigned long val);

/* output an executable, library or object file. DO NOT call
   tcc_relocate() before. */
int tcc_output_file(TCCState *s, const char *filename);

/* link and run main() function and return its value. DO NOT call
   tcc_relocate() before. */
int tcc_run(TCCState *s, int argc, char **argv);

/* do all relocations (needed before using tcc_get_symbol()). Return
   non zero if link error. */
int tcc_relocate(TCCState *s);

/* return symbol value. return 0 if OK, -1 if symbol not found */
int tcc_get_symbol(TCCState *s, unsigned long *pval, const char *name);

#ifdef __cplusplus
}
#endif

#endif
