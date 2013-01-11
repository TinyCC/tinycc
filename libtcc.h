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
    struct TCCState *tcc_state;
    int call_vio_open_flags; /*CALL_VIO_OPEN_FIRST, CALL_VIO_OPEN_LAST, one or both */
    int (*vio_open)(vio_fd *fd, const char *fn, int oflag) ;
    off_t (*vio_lseek)(vio_fd fd, off_t offset, int whence);
    size_t (*vio_read)(vio_fd fd, void *buf, size_t bytes);
    int (*vio_close)(vio_fd *fd);
} vio_module_t;


/* create a new TCC compilation context */
LIBTCCAPI TCCState *tcc_new(void);

/* free a TCC compilation context */
LIBTCCAPI void tcc_delete(TCCState *s);

/* add debug information in the generated code */
LIBTCCAPI void tcc_enable_debug(TCCState *s);

/* set error/warning display callback */
LIBTCCAPI void tcc_set_error_func(TCCState *s, void *error_opaque,
                        void (*error_func)(void *opaque, const char *msg));

/* set/reset a warning */
LIBTCCAPI int tcc_set_warning(TCCState *s, const char *warning_name, int value);

/* set linker option */
LIBTCCAPI const char * tcc_set_linker(TCCState *s, char *option, int multi);

/* set virtual io module */
LIBTCCAPI void tcc_set_vio_module(TCCState *s, vio_module_t *vio_module);

/*****************************/
/* preprocessor */

/* add include path */
LIBTCCAPI int tcc_add_include_path(TCCState *s, const char *pathname);

/* add in system include path */
LIBTCCAPI int tcc_add_sysinclude_path(TCCState *s, const char *pathname);

/* define preprocessor symbol 'sym'. Can put optional value */
LIBTCCAPI void tcc_define_symbol(TCCState *s, const char *sym, const char *value);

/* undefine preprocess symbol 'sym' */
LIBTCCAPI void tcc_undefine_symbol(TCCState *s, const char *sym);

/*****************************/
/* compiling */

/* add a file (either a C file, dll, an object, a library or an ld
   script). Return -1 if error. */
LIBTCCAPI int tcc_add_file(TCCState *s, const char *filename);

/* compile a string containing a C source. Return non zero if
   error. */
LIBTCCAPI int tcc_compile_string(TCCState *s, const char *buf);

/* compile a string containing a C source. Return non zero if
   error. Can associate a name with string for errors. */
int tcc_compile_named_string(TCCState *s, const char *buf, const char *strname);

/*****************************/
/* linking commands */

/* set output type. MUST BE CALLED before any compilation */
#define TCC_OUTPUT_MEMORY   0 /* output will be ran in memory (no
                                 output file) (default) */
#define TCC_OUTPUT_EXE      1 /* executable file */
#define TCC_OUTPUT_DLL      2 /* dynamic library */
#define TCC_OUTPUT_OBJ      3 /* object file */
#define TCC_OUTPUT_PREPROCESS 4 /* preprocessed file (used internally) */
LIBTCCAPI int tcc_set_output_type(TCCState *s, int output_type);

#define TCC_OUTPUT_FORMAT_ELF    0 /* default output format: ELF */
#define TCC_OUTPUT_FORMAT_BINARY 1 /* binary image output */
#define TCC_OUTPUT_FORMAT_COFF   2 /* COFF */

/* equivalent to -Lpath option */
LIBTCCAPI int tcc_add_library_path(TCCState *s, const char *pathname);

/* the library name is the same as the argument of the '-l' option */
LIBTCCAPI int tcc_add_library(TCCState *s, const char *libraryname);

/* add a symbol to the compiled program */
LIBTCCAPI int tcc_add_symbol(TCCState *s, const char *name, const void *val);

/* output an executable, library or object file. DO NOT call
   tcc_relocate() before. */
LIBTCCAPI int tcc_output_file(TCCState *s, const char *filename);

/* link and run main() function and return its value. DO NOT call
   tcc_relocate() before. */
LIBTCCAPI int tcc_run(TCCState *s, int argc, char **argv);

/* do all relocations (needed before using tcc_get_symbol())
   possible values for 'ptr':
   - TCC_RELOCATE_AUTO : Allocate and manage memory internally
   - NULL              : return required memory size for the step below
   - memory address    : copy code to memory passed by the caller
   returns -1 on error. */
LIBTCCAPI int tcc_relocate(TCCState *s1, void *ptr);
#define TCC_RELOCATE_AUTO (void*)1

/* return symbol value or NULL if not found */
LIBTCCAPI void *tcc_get_symbol(TCCState *s, const char *name);

/* set CONFIG_TCCDIR at runtime */
LIBTCCAPI void tcc_set_lib_path(TCCState *s, const char *path);

#ifdef __cplusplus
}
#endif

#endif
