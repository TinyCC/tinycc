#ifndef LIBTCC_H
#define LIBTCC_H

struct TCCState;

typedef struct TCCState TCCState;

/* create a new TCC compilation context */
TCCState *tcc_new(void);

/* free a TCC compilation context */
void tcc_delete(TCCState *s);

/* add debug information in the generated code */
void tcc_enable_debug(TCCState *s);

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
   script */
void tcc_add_file(TCCState *s, const char *filename);

/* compile a string containing a C source. Return non zero if
   error. */
int tcc_compile_string(TCCState *s, const char *buf);

/* get last error */
int tcc_get_error(TCCState *s, char *buf, int buf_size);

/*****************************/
/* linking commands */

/* set output type. MUST BE CALLED before any compilation */
#define TCC_OUTPUT_MEMORY   0 /* output will be ran in memory (no
                                 output file) (default) */
#define TCC_OUTPUT_EXE      1 /* executable file */
#define TCC_OUTPUT_DLL      2 /* dynamic library */
#define TCC_OUTPUT_OBJ      3 /* object file */
int tcc_set_output_type(TCCState *s, int output_type);

/* equivalent to -Lpath option */
int tcc_add_library_path(TCCState *s, const char *pathname);

/* the library name is the same as the argument of the '-l' option */
int tcc_add_library(TCCState *s, const char *libraryname);

/* add a symbol to the compiled program */
int tcc_add_symbol(TCCState *s, const char *name, unsigned long val);

/* output an executable file */
int tcc_output_file(TCCState *s, const char *filename);

/* link and run main() function and return its value */
int tcc_run(TCCState *s, int argc, char **argv);

#endif
