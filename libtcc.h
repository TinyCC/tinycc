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

/* define preprocessor symbol 'sym'. Can put optional value */
void tcc_define_symbol(TCCState *s, const char *sym, const char *value);

/* undefine preprocess symbol 'sym' */
void tcc_undefine_symbol(TCCState *s, const char *sym);

/*****************************/
/* compiling */

/* compile a file. Return non zero if error. */
int tcc_compile_file(TCCState *s, const char *filename);

/* compile a string. Return non zero if error. */
int tcc_compile_string(TCCState *s, const char *buf);

/* get last error */
int tcc_get_error(TCCState *s, char *buf, int buf_size);

/*****************************/
/* linking commands */

/* add a DYNAMIC library so that the compiled program can use its symbols */
int tcc_add_dll(TCCState *s, const char *library_name);

/* define a global symbol */
int tcc_add_symbol(TCCState *s, const char *name, void *value);

#define TCC_FILE_EXE  0 /* executable file */
#define TCC_FILE_DLL  1 /* dynamic library */
#define TCC_FILE_OBJ  2 /* object file */

/* output an executable file */
int tcc_output_file(TCCState *s, const char *filename, int file_type);

/* link and run main() function and return its value */
int tcc_run(TCCState *s, int argc, char **argv);

#endif
