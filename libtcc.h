#ifndef LIBTCC_H
#define LIBTCC_H

#ifndef LIBTCCAPI
# define LIBTCCAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct TCCState;

typedef struct TCCState TCCState;

typedef void (*TCCErrorFunc)(void *opaque, const char *msg);

#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
#ifndef _OFF_T_
#define _OFF_T_
  typedef long _off_t;
#ifndef _SIZE_T_
#define _SIZE_T_
  typedef unsigned long size_t;
#endif
#if !defined(NO_OLDNAMES) || defined(_POSIX)
  typedef long off_t;
#endif
#endif
#endif

struct vio_module_t;

typedef struct vio_fd {
    int fd;
    void *vio_udata;
    struct vio_module_t *vio_module;
} vio_fd;

#define CALL_VIO_OPEN_FIRST 0x01
#define CALL_VIO_OPEN_LAST 0x02

typedef struct vio_module_t {
    void *user_data;
    struct TCCState *S;
    int call_vio_open_flags; /*CALL_VIO_OPEN_FIRST, CALL_VIO_OPEN_LAST, one or both */
    int (*vio_open)(vio_fd *fd, const char *fn, int oflag) ;
    off_t (*vio_lseek)(vio_fd fd, off_t offset, int whence);
    size_t (*vio_read)(vio_fd fd, void *buf, size_t bytes);
    int (*vio_close)(vio_fd *fd);
} vio_module_t;

/* create a new TCC compilation context */
LIBTCCAPI TCCState *tcc_new(void);

/* free a TCC compilation context */
LIBTCCAPI void tcc_delete(TCCState *S);

/* set CONFIG_TCCDIR at runtime */
LIBTCCAPI void tcc_set_lib_path(TCCState *S, const char *path);

/* set error/warning display callback */
LIBTCCAPI void tcc_set_error_func(TCCState *S, void *error_opaque, TCCErrorFunc error_func);

/* return error/warning callback */
LIBTCCAPI TCCErrorFunc tcc_get_error_func(TCCState *S);

/* return error/warning callback opaque pointer */
LIBTCCAPI void *tcc_get_error_opaque(TCCState *S);

/* set options as from command line (multiple supported) */
LIBTCCAPI void tcc_set_options(TCCState *S, const char *str);

/* set virtual io module */
LIBTCCAPI void tcc_set_vio_module(TCCState *S, vio_module_t *vio_module);

/*****************************/
/* preprocessor */

/* add include path */
LIBTCCAPI int tcc_add_include_path(TCCState *S, const char *pathname);

/* add in system include path */
LIBTCCAPI int tcc_add_sysinclude_path(TCCState *S, const char *pathname);

/* define preprocessor symbol 'sym'. value can be NULL, sym can be "sym=val" */
LIBTCCAPI void tcc_define_symbol(TCCState *S, const char *sym, const char *value);

/* undefine preprocess symbol 'sym' */
LIBTCCAPI void tcc_undefine_symbol(TCCState *S, const char *sym);

/*****************************/
/* compiling */

/* add a file (C file, dll, object, library, ld script). Return -1 if error. */
LIBTCCAPI int tcc_add_file(TCCState *S, const char *filename);

/* compile a string containing a C source. Return -1 if error. */
LIBTCCAPI int tcc_compile_string(TCCState *S, const char *buf);

/*****************************/
/* linking commands */

/* set output type. MUST BE CALLED before any compilation */
LIBTCCAPI int tcc_set_output_type(TCCState *S, int output_type);
#define TCC_OUTPUT_MEMORY   1 /* output will be run in memory (default) */
#define TCC_OUTPUT_EXE      2 /* executable file */
#define TCC_OUTPUT_DLL      3 /* dynamic library */
#define TCC_OUTPUT_OBJ      4 /* object file */
#define TCC_OUTPUT_PREPROCESS 5 /* only preprocess (used internally) */

/* equivalent to -Lpath option */
LIBTCCAPI int tcc_add_library_path(TCCState *S, const char *pathname);

/* the library name is the same as the argument of the '-l' option */
LIBTCCAPI int tcc_add_library(TCCState *S, const char *libraryname);

/* add a symbol to the compiled program */
LIBTCCAPI int tcc_add_symbol(TCCState *S, const char *name, const void *val);

/* output an executable, library or object file. DO NOT call
   tcc_relocate() before. */
LIBTCCAPI int tcc_output_file(TCCState *S, const char *filename);

/* link and run main() function and return its value. DO NOT call
   tcc_relocate() before. */
LIBTCCAPI int tcc_run(TCCState *S, int argc, char **argv);

/* do all relocations (needed before using tcc_get_symbol()) */
LIBTCCAPI int tcc_relocate(TCCState *S, void *ptr);
/* possible values for 'ptr':
   - TCC_RELOCATE_AUTO : Allocate and manage memory internally
   - NULL              : return required memory size for the step below
   - memory address    : copy code to memory passed by the caller
   returns -1 if error. */
#define TCC_RELOCATE_AUTO (void*)1

/* return symbol value or NULL if not found */
LIBTCCAPI void *tcc_get_symbol(TCCState *S, const char *name);

/* return symbol value or NULL if not found */
LIBTCCAPI void tcc_list_symbols(TCCState *S, void *ctx,
    void (*symbol_cb)(void *ctx, const char *name, const void *val));

typedef int (*tcc_cmpfun)(const void *, const void *, void *);
LIBTCCAPI void tcc_qsort_s(void *base, size_t nel, size_t width, tcc_cmpfun cmp, void *ctx);

enum { /*need better names for some of then*/
    TCC_OPTION_d_BI = 1,
    TCC_OPTION_d_D = 3,
    TCC_OPTION_d_4 = 4,
    TCC_OPTION_d_M = 7,
    TCC_OPTION_d_t = 16,
    TCC_OPTION_d_32 = 32,
};

#ifdef __cplusplus
}
#endif

#endif
