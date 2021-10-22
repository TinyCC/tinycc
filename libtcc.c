/*
 *  TCC - Tiny C Compiler
 *
 *  Copyright (c) 2001-2004 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if !defined ONE_SOURCE || ONE_SOURCE
#include "tccpp.c"
#include "tccgen.c"
#include "tccasm.c"
#include "tccelf.c"
#include "tccrun.c"
#ifdef TCC_TARGET_I386
#include "i386-gen.c"
#include "i386-link.c"
#include "i386-asm.c"
#elif defined(TCC_TARGET_ARM)
#include "arm-gen.c"
#include "arm-link.c"
#include "arm-asm.c"
#elif defined(TCC_TARGET_ARM64)
#include "arm64-gen.c"
#include "arm64-link.c"
#include "arm-asm.c"
#elif defined(TCC_TARGET_C67)
#include "c67-gen.c"
#include "c67-link.c"
#include "tcccoff.c"
#elif defined(TCC_TARGET_X86_64)
#include "x86_64-gen.c"
#include "x86_64-link.c"
#include "i386-asm.c"
#elif defined(TCC_TARGET_RISCV64)
#include "riscv64-gen.c"
#include "riscv64-link.c"
#include "riscv64-asm.c"
#else
#error unknown target
#endif
#ifdef TCC_TARGET_PE
#include "tccpe.c"
#endif
#ifdef TCC_TARGET_MACHO
#include "tccmacho.c"
#endif
#endif /* ONE_SOURCE */

#include "tcc.h"

/********************************************************/
/* global variables */

/* XXX: get rid of this ASAP (or maybe not) */
TCC_SEM(static tcc_compile_sem);

#ifdef MEM_DEBUG
static int nb_states;
#endif

/********************************************************/
#ifdef _WIN32
ST_FUNC char *normalize_slashes(char *path)
{
    char *p;
    for (p = path; *p; ++p)
        if (*p == '\\')
            *p = '/';
    return path;
}

#if defined LIBTCC_AS_DLL && !defined CONFIG_TCCDIR
static HMODULE tcc_module;
BOOL WINAPI DllMain (HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (DLL_PROCESS_ATTACH == dwReason)
        tcc_module = hDll;
    return TRUE;
}
#else
#define tcc_module NULL /* NULL means executable itself */
#endif

#ifndef CONFIG_TCCDIR
/* on win32, we suppose the lib and includes are at the location of 'tcc.exe' */
static inline char *config_tccdir_w32(char *path)
{
    char *p;
    GetModuleFileName(tcc_module, path, MAX_PATH);
    p = tcc_basename(normalize_slashes(strlwr(path)));
    if (p > path)
        --p;
    *p = 0;
    return path;
}
#define CONFIG_TCCDIR config_tccdir_w32(alloca(MAX_PATH))
#endif

#ifdef TCC_TARGET_PE
static void tcc_add_systemdir(TCCState *S)
{
    char buf[1000];
    GetSystemDirectory(buf, sizeof buf);
    tcc_add_library_path(S, normalize_slashes(buf));
}
#endif
#endif

/********************************************************/
#if CONFIG_TCC_SEMLOCK
#if defined _WIN32
ST_FUNC void wait_sem(TCCSem *p)
{
    if (!p->init)
        InitializeCriticalSection(&p->cr), p->init = 1;
    EnterCriticalSection(&p->cr);
}
ST_FUNC void post_sem(TCCSem *p)
{
    LeaveCriticalSection(&p->cr);
}
#elif defined __APPLE__
/* Half-compatible MacOS doesn't have non-shared (process local)
   semaphores.  Use the dispatch framework for lightweight locks.  */
ST_FUNC void wait_sem(TCCSem *p)
{
    if (!p->init)
        p->sem = dispatch_semaphore_create(1), p->init = 1;
    dispatch_semaphore_wait(p->sem, DISPATCH_TIME_FOREVER);
}
ST_FUNC void post_sem(TCCSem *p)
{
    dispatch_semaphore_signal(p->sem);
}
#else
ST_FUNC void wait_sem(TCCSem *p)
{
    if (!p->init)
        sem_init(&p->sem, 0, 1), p->init = 1;
    while (sem_wait(&p->sem) < 0 && errno == EINTR);
}
ST_FUNC void post_sem(TCCSem *p)
{
    sem_post(&p->sem);
}
#endif
#endif

PUB_FUNC void tcc_enter_state(TCCState *S)
{
    if (S->error_set_jmp_enabled)
        return;
    WAIT_SEM(&tcc_compile_sem);
    /*S = S;*/
}

PUB_FUNC void tcc_exit_state(TCCState *S)
{
    if (S->error_set_jmp_enabled)
        return;
    S = NULL;
    POST_SEM(&tcc_compile_sem);
}

/********************************************************/
/* copy a string and truncate it. */
ST_FUNC char *pstrcpy(char *buf, size_t buf_size, const char *s)
{
    char *q, *q_end;
    int c;

    if (buf_size > 0) {
        q = buf;
        q_end = buf + buf_size - 1;
        while (q < q_end) {
            c = *s++;
            if (c == '\0')
                break;
            *q++ = c;
        }
        *q = '\0';
    }
    return buf;
}

/* strcat and truncate. */
ST_FUNC char *pstrcat(char *buf, size_t buf_size, const char *s)
{
    size_t len;
    len = strlen(buf);
    if (len < buf_size)
        pstrcpy(buf + len, buf_size - len, s);
    return buf;
}

ST_FUNC char *pstrncpy(char *out, const char *in, size_t num)
{
    memcpy(out, in, num);
    out[num] = '\0';
    return out;
}

/* extract the basename of a file */
PUB_FUNC char *tcc_basename(const char *name)
{
    char *p = strchr(name, 0);
    while (p > name && !IS_DIRSEP(p[-1]))
        --p;
    return p;
}

/* extract extension part of a file
 *
 * (if no extension, return pointer to end-of-string)
 */
PUB_FUNC char *tcc_fileextension (const char *name)
{
    char *b = tcc_basename(name);
    char *e = strrchr(b, '.');
    return e ? e : strchr(b, 0);
}

ST_FUNC char *tcc_load_text(TCCState *S, int fd)
{
    int len = lseek(fd, 0, SEEK_END);
    char *buf = load_data(S, fd, 0, len + 1);
    buf[len] = 0;
    return buf;
}

/********************************************************/
/* memory management */

#undef free
#undef malloc
#undef realloc

PUB_FUNC void tcc_free_base(void *ptr)
{
    free(ptr);
}

PUB_FUNC void *tcc_malloc_base(unsigned long size)
{
    void *ptr;
    ptr = malloc(size);
    if (!ptr && size) {
        printf("memory full (malloc)\n");
        exit(1);
    }
    return ptr;
}

PUB_FUNC void *tcc_mallocz_base(unsigned long size)
{
    void *ptr;
    ptr = tcc_malloc_base(size);
    if (size)
        memset(ptr, 0, size);
    return ptr;
}

#ifndef MEM_DEBUG

PUB_FUNC void tcc_free(TCCState *S, void *ptr)
{
    free(ptr);
}

PUB_FUNC void *tcc_malloc(TCCState *S, unsigned long size)
{
    void *ptr;
    ptr = malloc(size);
    if (!ptr && size)
        _tcc_error(S, "memory full (malloc)");
    return ptr;
}

PUB_FUNC void *tcc_mallocz(TCCState *S, unsigned long size)
{
    void *ptr;
    ptr = tcc_malloc(S, size);
    if (size)
        memset(ptr, 0, size);
    return ptr;
}

PUB_FUNC void *tcc_realloc(TCCState *S, void *ptr, unsigned long size)
{
    void *ptr1;
    ptr1 = realloc(ptr, size);
    if (!ptr1 && size)
        _tcc_error(S, "memory full (realloc)");
    return ptr1;
}

PUB_FUNC char *tcc_strdup(TCCState *S, const char *str)
{
    char *ptr;
    ptr = tcc_malloc(S, strlen(str) + 1);
    strcpy(ptr, str);
    return ptr;
}

#else

#define MEM_DEBUG_MAGIC1 0xFEEDDEB1
#define MEM_DEBUG_MAGIC2 0xFEEDDEB2
#define MEM_DEBUG_MAGIC3 0xFEEDDEB3
#define MEM_DEBUG_FILE_LEN 40
#define MEM_DEBUG_CHECK3(header) \
    ((mem_debug_header_t*)((char*)header + header->size))->magic3
#define MEM_USER_PTR(header) \
    ((char *)header + offsetof(mem_debug_header_t, magic3))
#define MEM_HEADER_PTR(ptr) \
    (mem_debug_header_t *)((char*)ptr - offsetof(mem_debug_header_t, magic3))

struct mem_debug_header {
    unsigned magic1;
    unsigned size;
    struct mem_debug_header *prev;
    struct mem_debug_header *next;
    int line_num;
    char file_name[MEM_DEBUG_FILE_LEN + 1];
    unsigned magic2;
    ALIGNED(16) unsigned char magic3[4];
};

typedef struct mem_debug_header mem_debug_header_t;

static mem_debug_header_t *mem_debug_chain;
static unsigned mem_cur_size;
static unsigned mem_max_size;

static mem_debug_header_t *malloc_check(void *ptr, const char *msg)
{
    mem_debug_header_t * header = MEM_HEADER_PTR(ptr);
    if (header->magic1 != MEM_DEBUG_MAGIC1 ||
        header->magic2 != MEM_DEBUG_MAGIC2 ||
        read32le(MEM_DEBUG_CHECK3(header)) != MEM_DEBUG_MAGIC3 ||
        header->size == (unsigned)-1) {
        fprintf(stderr, "%s check failed\n", msg);
        if (header->magic1 == MEM_DEBUG_MAGIC1)
            fprintf(stderr, "%s:%u: block allocated here.\n",
                header->file_name, header->line_num);
        exit(1);
    }
    return header;
}

PUB_FUNC void *tcc_malloc_debug(TCCState *S, unsigned long size, const char *file, int line)
{
    int ofs;
    mem_debug_header_t *header;

    header = malloc(sizeof(mem_debug_header_t) + size);
    if (!header)
        _tcc_error(S, "memory full (malloc)");

    header->magic1 = MEM_DEBUG_MAGIC1;
    header->magic2 = MEM_DEBUG_MAGIC2;
    header->size = size;
    write32le(MEM_DEBUG_CHECK3(header), MEM_DEBUG_MAGIC3);
    header->line_num = line;
    ofs = strlen(file) - MEM_DEBUG_FILE_LEN;
    strncpy(header->file_name, file + (ofs > 0 ? ofs : 0), MEM_DEBUG_FILE_LEN);
    header->file_name[MEM_DEBUG_FILE_LEN] = 0;

    header->next = mem_debug_chain;
    header->prev = NULL;
    if (header->next)
        header->next->prev = header;
    mem_debug_chain = header;

    mem_cur_size += size;
    if (mem_cur_size > mem_max_size)
        mem_max_size = mem_cur_size;

    return MEM_USER_PTR(header);
}

PUB_FUNC void tcc_free_debug(TCCState *S, void *ptr)
{
    mem_debug_header_t *header;
    if (!ptr)
        return;
    header = malloc_check(ptr, "tcc_free");
    mem_cur_size -= header->size;
    header->size = (unsigned)-1;
    if (header->next)
        header->next->prev = header->prev;
    if (header->prev)
        header->prev->next = header->next;
    if (header == mem_debug_chain)
        mem_debug_chain = header->next;
    free(header);
}

PUB_FUNC void *tcc_mallocz_debug(TCCState *S, unsigned long size, const char *file, int line)
{
    void *ptr;
    ptr = tcc_malloc_debug(S, size,file,line);
    memset(ptr, 0, size);
    return ptr;
}

PUB_FUNC void *tcc_realloc_debug(TCCState *S, void *ptr, unsigned long size, const char *file, int line)
{
    mem_debug_header_t *header;
    int mem_debug_chain_update = 0;
    if (!ptr)
        return tcc_malloc_debug(S, size, file, line);
    header = malloc_check(ptr, "tcc_realloc");
    mem_cur_size -= header->size;
    mem_debug_chain_update = (header == mem_debug_chain);
    header = realloc(header, sizeof(mem_debug_header_t) + size);
    if (!header)
        _tcc_error(S, "memory full (realloc)");
    header->size = size;
    write32le(MEM_DEBUG_CHECK3(header), MEM_DEBUG_MAGIC3);
    if (header->next)
        header->next->prev = header;
    if (header->prev)
        header->prev->next = header;
    if (mem_debug_chain_update)
        mem_debug_chain = header;
    mem_cur_size += size;
    if (mem_cur_size > mem_max_size)
        mem_max_size = mem_cur_size;
    return MEM_USER_PTR(header);
}

PUB_FUNC char *tcc_strdup_debug(TCCState *S, const char *str, const char *file, int line)
{
    char *ptr;
    ptr = tcc_malloc_debug(S, strlen(str) + 1, file, line);
    strcpy(ptr, str);
    return ptr;
}

PUB_FUNC void tcc_memcheck(void)
{
    if (mem_cur_size) {
        mem_debug_header_t *header = mem_debug_chain;
        fprintf(stderr, "MEM_DEBUG: mem_leak= %d bytes, mem_max_size= %d bytes\n",
            mem_cur_size, mem_max_size);
        while (header) {
            fprintf(stderr, "%s:%u: error: %u bytes leaked\n",
                header->file_name, header->line_num, header->size);
            header = header->next;
        }
#if MEM_DEBUG-0 == 2
        exit(2);
#endif
    }
}
#endif /* MEM_DEBUG */

#define free(p) use_tcc_free(p)
#define malloc(s) use_tcc_malloc(s)
#define realloc(p, s) use_tcc_realloc(p, s)

/********************************************************/
/* virtual io */

LIBTCCAPI void tcc_set_vio_module(TCCState *S, vio_module_t *vio_module){
	S->vio_module = vio_module;
	vio_module->S = S;
}

void vio_initialize(vio_fd *fd) {
    fd->fd = -1;
    fd->vio_udata = NULL;
    fd->vio_module = NULL;
}

int vio_open(struct TCCState *S, vio_fd *fd, const char *fn, int oflag) {
    int rc;
    vio_initialize(fd);
    fd->vio_module = S->vio_module;
    if(S->vio_module && (S->vio_module->call_vio_open_flags & CALL_VIO_OPEN_FIRST)) {
	rc =  S->vio_module->vio_open(fd, fn, oflag); 
	if(rc >= 0) return rc;
    }
    
    fd->fd = open(fn, oflag);
    
    if(fd->fd < 0 && S->vio_module && (S->vio_module->call_vio_open_flags & CALL_VIO_OPEN_LAST)) {
	rc =  S->vio_module->vio_open(fd, fn, oflag); 
	if(rc >= 0) return rc;
    }
    //printf("vio_open = %d %s\n", fd->fd, fn);
    return fd->fd;
}

off_t vio_lseek(vio_fd fd, off_t offset, int whence) {
    if(fd.vio_udata) {
        return fd.vio_module->vio_lseek(fd, offset, whence);
    }
    return lseek(fd.fd, offset, whence);
}

size_t vio_read(vio_fd fd, void *buf, size_t bytes) {
    if(fd.vio_udata) {
	return fd.vio_module->vio_read(fd, buf, bytes);
    }
    return read(fd.fd, buf, bytes);
}

int vio_close(vio_fd *fd) {
    int rc = 0;
    if(fd->vio_udata){
	fd->vio_module->vio_close(fd);
    } else rc = close(fd->fd);
    vio_initialize(fd);
    return rc;
}

/********************************************************/
/* dynarrays */

ST_FUNC void dynarray_add(TCCState *S, void *ptab, int *nb_ptr, void *data)
{
    int nb, nb_alloc;
    void **pp;

    nb = *nb_ptr;
    pp = *(void ***)ptab;
    /* every power of two we double array size */
    if ((nb & (nb - 1)) == 0) {
        if (!nb)
            nb_alloc = 1;
        else
            nb_alloc = nb * 2;
        pp = tcc_realloc(S, pp, nb_alloc * sizeof(void *));
        *(void***)ptab = pp;
    }
    pp[nb++] = data;
    *nb_ptr = nb;
}

ST_FUNC void dynarray_reset(TCCState *S, void *pp, int *n)
{
    void **p;
    for (p = *(void***)pp; *n; ++p, --*n)
        if (*p)
            tcc_free(S, *p);
    tcc_free(S, *(void**)pp);
    *(void**)pp = NULL;
}

static void tcc_split_path(TCCState *S, void *p_ary, int *p_nb_ary, const char *in)
{
    const char *p;
    do {
        int c;
        CString str;

        cstr_new(S, &str);
        for (p = in; c = *p, c != '\0' && c != PATHSEP[0]; ++p) {
            if (c == '{' && p[1] && p[2] == '}') {
                c = p[1], p += 2;
                if (c == 'B')
                    cstr_cat(S, &str, S->tcc_lib_path, -1);
                if (c == 'f' && S->tccpp_file) {
                    /* substitute current file's dir */
                    const char *f = S->tccpp_file->true_filename;
                    const char *b = tcc_basename(f);
                    if (b > f)
                        cstr_cat(S, &str, f, b - f - 1);
                    else
                        cstr_cat(S, &str, ".", 1);
                }
            } else {
                cstr_ccat(S, &str, c);
            }
        }
        if (str.size) {
            cstr_ccat(S, &str, '\0');
            dynarray_add(S, p_ary, p_nb_ary, tcc_strdup(S, str.data));
        }
        cstr_free(S, &str);
        in = p+1;
    } while (*p);
}

/********************************************************/
/* warning / error */

/* warn_... option bits */
#define WARN_ON  1 /* warning is on (-Woption) */
#define WARN_ERR 2 /* warning is an error (-Werror=option) */
#define WARN_NOE 4 /* warning is not an error (-Wno-error=option) */

/* error1() modes */
enum { ERROR_WARN, ERROR_NOABORT, ERROR_ERROR };

static void error1(TCCState *S, int mode, const char *fmt, va_list ap)
{
    BufferedFile **pf, *f;
    CString cs;

    cstr_new(S, &cs);

    if (S == NULL)
        /* can happen only if called from tcc_malloc(): 'out of memory' */
        goto no_file;

    tcc_exit_state(S);

    if (mode == ERROR_WARN) {
        if (S->warn_error)
            mode = ERROR_ERROR;
        if (S->warn_num) {
            /* handle tcc_warning_c(warn_option)(fmt, ...) */
            int wopt = *(&S->warn_none + S->warn_num);
            S->warn_num = 0;
            if (0 == (wopt & WARN_ON))
                return;
            if (wopt & WARN_ERR)
                mode = ERROR_ERROR;
            if (wopt & WARN_NOE)
                mode = ERROR_WARN;
        }
        if (S->warn_none)
            return;
    }

    f = NULL;
    if (S->error_set_jmp_enabled) { /* we're called while parsing a file */
        /* use upper file if inline ":asm:" or token ":paste:" */
        for (f = S->tccpp_file; f && f->filename[0] == ':'; f = f->prev)
            ;
    }
    if (f) {
        for(pf = S->include_stack; pf < S->include_stack_ptr; pf++)
            cstr_printf(S, &cs, "In file included from %s:%d:\n",
                (*pf)->filename, (*pf)->line_num);
        cstr_printf(S, &cs, "%s:%d: ",
            f->filename, (*pf)->line_num - !!(S->tok_flags & TOK_FLAG_BOL));
    } else if (S->current_filename) {
        cstr_printf(S, &cs, "%s: ", S->current_filename);
    }

no_file:
    if (0 == cs.size)
        cstr_printf(S, &cs, "tcc: ");
    cstr_printf(S, &cs, mode == ERROR_WARN ? "warning: " : "error: ");
    cstr_vprintf(S, &cs, fmt, ap);
    if (!S || !S->error_func) {
        /* default case: stderr */
        if (S && S->output_type == TCC_OUTPUT_PREPROCESS && S->ppfp == stdout)
            printf("\n"); /* print a newline during tcc -E */
        fflush(stdout); /* flush -v output */
        fprintf(stderr, "%s\n", (char*)cs.data);
        fflush(stderr); /* print error/warning now (win32) */
    } else {
        S->error_func(S->error_opaque, (char*)cs.data);
    }
    cstr_free(S, &cs);
    if (S) {
        if (mode != ERROR_WARN)
            S->nb_errors++;
        if (mode != ERROR_ERROR)
            return;
        if (S->error_set_jmp_enabled)
            longjmp(S->error_jmp_buf, 1);
    }
    exit(1);
}

LIBTCCAPI void tcc_set_error_func(TCCState *S, void *error_opaque, TCCErrorFunc error_func)
{
    S->error_opaque = error_opaque;
    S->error_func = error_func;
}

LIBTCCAPI TCCErrorFunc tcc_get_error_func(TCCState *S)
{
    return S->error_func;
}

LIBTCCAPI void *tcc_get_error_opaque(TCCState *S)
{
    return S->error_opaque;
}

/* error without aborting current compilation */
PUB_FUNC void _tcc_error_noabort(TCCState *S, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    error1(S, ERROR_NOABORT, fmt, ap);
    va_end(ap);
}

PUB_FUNC void _tcc_error(TCCState *S, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    for (;;) error1(S, ERROR_ERROR, fmt, ap);
}

PUB_FUNC void _tcc_warning(TCCState *S, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    error1(S, ERROR_WARN, fmt, ap);
    va_end(ap);
}

/********************************************************/
/* I/O layer */

ST_FUNC void tcc_open_bf(TCCState *S, const char *filename, int initlen)
{
    BufferedFile *bf;
    int buflen = initlen ? initlen : IO_BUF_SIZE;

    bf = tcc_mallocz(S, sizeof(BufferedFile) + buflen);
    bf->buf_ptr = bf->buffer;
    bf->buf_end = bf->buffer + initlen;
    bf->buf_end[0] = CH_EOB; /* put eob symbol */
    pstrcpy(bf->filename, sizeof(bf->filename), filename);
#ifdef _WIN32
    normalize_slashes(bf->filename);
#endif
    bf->true_filename = bf->filename;
    bf->line_num = 1;
    bf->ifdef_stack_ptr = S->ifdef_stack_ptr;
    bf->fd = -1;
    bf->prev = S->tccpp_file;
    S->tccpp_file = bf;
    S->tok_flags = TOK_FLAG_BOL | TOK_FLAG_BOF;
}

ST_FUNC void tcc_close(TCCState *S)
{
    BufferedFile *bf = S->tccpp_file;
    if (bf->fd > 0) {
        close(bf->fd);
        total_lines += bf->line_num;
    }
    if (bf->true_filename != bf->filename)
        tcc_free(S, bf->true_filename);
    S->tccpp_file = bf->prev;
    tcc_free(S, bf);
}

static int _tcc_open(TCCState *S, const char *filename)
{
    int fd;
    if (strcmp(filename, "-") == 0)
        fd = 0, filename = "<stdin>";
    else
        fd = open(filename, O_RDONLY | O_BINARY);
    if ((S->verbose == 2 && fd >= 0) || S->verbose == 3)
        printf("%s %*s%s\n", fd < 0 ? "nf":"->",
               (int)(S->include_stack_ptr - S->include_stack), "", filename);
    return fd;
}

ST_FUNC int tcc_open(TCCState *S, const char *filename)
{
    int fd = _tcc_open(S, filename);
    if (fd < 0)
        return -1;
    tcc_open_bf(S, filename, 0);
    S->tccpp_file->fd = fd;
    return 0;
}

/* compile the file opened in 'file'. Return non zero if errors. */
static int tcc_compile(TCCState *S, int filetype, const char *str, int fd)
{
    /* Here we enter the code section where we use the global variables for
       parsing and code generation (tccpp.c, tccgen.c, <target>-gen.c).
       Other threads need to wait until we're done.

       Alternatively we could use thread local storage for those global
       variables, which may or may not have advantages */

    tcc_enter_state(S);
    S->error_set_jmp_enabled = 1;

    if (setjmp(S->error_jmp_buf) == 0) {
        S->nb_errors = 0;

        if (fd == -1) {
            int len = strlen(str);
            tcc_open_bf(S, "<string>", len);
            memcpy(S->tccpp_file->buffer, str, len);
        } else {
            tcc_open_bf(S, str, 0);
            S->tccpp_file->fd = fd;
        }

        tccelf_begin_file(S);
        preprocess_start(S, filetype);
        tccgen_init(S);
        if (S->output_type == TCC_OUTPUT_PREPROCESS) {
            tcc_preprocess(S);
        } else if (filetype & (AFF_TYPE_ASM | AFF_TYPE_ASMPP)) {
            tcc_assemble(S, !!(filetype & AFF_TYPE_ASMPP));
        } else {
            tccgen_compile(S);
        }
    }
    tccgen_finish(S);
    preprocess_end(S);

    S->error_set_jmp_enabled = 0;
    tcc_exit_state(S);

    tccelf_end_file(S);
    return S->nb_errors != 0 ? -1 : 0;
}

LIBTCCAPI int tcc_compile_string(TCCState *S, const char *str)
{
    return tcc_compile(S, S->filetype, str, -1);
}

/* define a preprocessor symbol. value can be NULL, sym can be "sym=val" */
LIBTCCAPI void tcc_define_symbol(TCCState *S, const char *sym, const char *value)
{
    const char *eq;
    if (NULL == (eq = strchr(sym, '=')))
        eq = strchr(sym, 0);
    if (NULL == value)
        value = *eq ? eq + 1 : "1";
    cstr_printf(S, &S->cmdline_defs, "#define %.*s %s\n", (int)(eq-sym), sym, value);
}

/* undefine a preprocessor symbol */
LIBTCCAPI void tcc_undefine_symbol(TCCState *S, const char *sym)
{
    cstr_printf(S, &S->cmdline_defs, "#undef %s\n", sym);
}


LIBTCCAPI TCCState *tcc_new(void)
{
    TCCState *s;

    s = tcc_mallocz_base(sizeof(TCCState));
    if (!s)
        return NULL;
#ifdef MEM_DEBUG
    ++nb_states;
#endif

#undef gnu_ext

    s->gnu_ext = 1;
    s->tcc_ext = 1;
    s->nocommon = 1;
    s->dollars_in_identifiers = 1; /*on by default like in gcc/clang*/
    s->cversion = 199901; /* default unless -std=c11 is supplied */
    s->warn_implicit_function_declaration = 1;
    s->warn_discarded_qualifiers = 1;
    s->ms_extensions = 1;

#ifdef CHAR_IS_UNSIGNED
    s->char_is_unsigned = 1;
#endif
#ifdef TCC_TARGET_I386
    s->seg_size = 32;
#endif
    /* enable this if you want symbols with leading underscore on windows: */
#if defined TCC_TARGET_MACHO /* || defined TCC_TARGET_PE */
    s->leading_underscore = 1;
#endif
#ifdef TCC_TARGET_ARM
    s->float_abi = ARM_FLOAT_ABI;
#endif

    s->ppfp = stdout;
    /* might be used in error() before preprocess_start() */
    s->include_stack_ptr = s->include_stack;

    tccelf_new(s);

    tcc_set_lib_path(s, CONFIG_TCCDIR);
    return s;
}

LIBTCCAPI void tcc_delete(TCCState *S)
{
    /* free sections */
    tccelf_delete(S);

    /* free library paths */
    dynarray_reset(S, &S->library_paths, &S->nb_library_paths);
    dynarray_reset(S, &S->crt_paths, &S->nb_crt_paths);

    /* free include paths */
    dynarray_reset(S, &S->include_paths, &S->nb_include_paths);
    dynarray_reset(S, &S->sysinclude_paths, &S->nb_sysinclude_paths);

    tcc_free(S, S->tcc_lib_path);
    tcc_free(S, S->soname);
    tcc_free(S, S->rpath);
    tcc_free(S, S->init_symbol);
    tcc_free(S, S->fini_symbol);
    tcc_free(S, S->outfile);
    tcc_free(S, S->deps_outfile);
    dynarray_reset(S, &S->files, &S->nb_files);
    dynarray_reset(S, &S->target_deps, &S->nb_target_deps);
    dynarray_reset(S, &S->pragma_libs, &S->nb_pragma_libs);
    dynarray_reset(S, &S->argv, &S->argc);
    cstr_free(S, &S->cmdline_defs);
    cstr_free(S, &S->cmdline_incl);
#ifdef TCC_IS_NATIVE
    /* free runtime memory */
    tcc_run_free(S);
#endif

    tcc_free_base(S);
#ifdef MEM_DEBUG
    if (0 == --nb_states)
        tcc_memcheck();
#endif
}

LIBTCCAPI int tcc_set_output_type(TCCState *S, int output_type)
{
    S->output_type = output_type;

    if (!S->nostdinc) {
        /* default include paths */
        /* -isystem paths have already been handled */
        tcc_add_sysinclude_path(S, CONFIG_TCC_SYSINCLUDEPATHS);
    }
#ifdef CONFIG_TCC_BCHECK
    if (S->do_bounds_check) {
        /* if bound checking, then add corresponding sections */
        tccelf_bounds_new(S);
    }
#endif
    if (S->do_debug) {
        /* add debug sections */
        tccelf_stab_new(S);
    }
    if (output_type == TCC_OUTPUT_OBJ) {
        /* always elf for objects */
        S->output_format = TCC_OUTPUT_FORMAT_ELF;
        return 0;
    }

    tcc_add_library_path(S, CONFIG_TCC_LIBPATHS);

#ifdef TCC_TARGET_PE
# ifdef _WIN32
    /* allow linking with system dll's directly */
    tcc_add_systemdir(S);
# endif
    /* target PE has its own startup code in libtcc1.a */
    return 0;

#elif defined TCC_TARGET_MACHO
# ifdef TCC_IS_NATIVE
    tcc_add_macos_sdkpath(S);
# endif
    /* Mach-O with LC_MAIN doesn't need any crt startup code.  */
    return 0;

#else
    /* paths for crt objects */
    tcc_split_path(S, &S->crt_paths, &S->nb_crt_paths, CONFIG_TCC_CRTPREFIX);
    /* add libc crt1/crti objects */
    if ((output_type == TCC_OUTPUT_EXE || output_type == TCC_OUTPUT_DLL) &&
        !S->nostdlib) {
#if TARGETOS_OpenBSD
        if (output_type != TCC_OUTPUT_DLL)
	    tcc_add_crt(S, "crt0.o");
        if (output_type == TCC_OUTPUT_DLL)
            tcc_add_crt(S, "crtbeginS.o");
        else
            tcc_add_crt(S, "crtbegin.o");
#elif TARGETOS_FreeBSD
        if (output_type != TCC_OUTPUT_DLL)
            tcc_add_crt(S, "crt1.o");
        tcc_add_crt(S, "crti.o");
        if (S->static_link)
            tcc_add_crt(S, "crtbeginT.o");
        else if (output_type == TCC_OUTPUT_DLL)
            tcc_add_crt(S, "crtbeginS.o");
        else
            tcc_add_crt(S, "crtbegin.o");
#elif TARGETOS_NetBSD
        if (output_type != TCC_OUTPUT_DLL)
            tcc_add_crt(S, "crt0.o");
        tcc_add_crt(S, "crti.o");
        if (S->static_link)
            tcc_add_crt(S, "crtbeginT.o");
        else if (output_type == TCC_OUTPUT_DLL)
            tcc_add_crt(S, "crtbeginS.o");
        else
            tcc_add_crt(S, "crtbegin.o");
#else
        if (output_type != TCC_OUTPUT_DLL)
            tcc_add_crt(S, "crt1.o");
        tcc_add_crt(S, "crti.o");
#endif
    }
    return 0;
#endif
}

LIBTCCAPI int tcc_add_include_path(TCCState *S, const char *pathname)
{
    tcc_split_path(S, &S->include_paths, &S->nb_include_paths, pathname);
    return 0;
}

LIBTCCAPI int tcc_add_sysinclude_path(TCCState *S, const char *pathname)
{
    tcc_split_path(S, &S->sysinclude_paths, &S->nb_sysinclude_paths, pathname);
    return 0;
}

ST_FUNC DLLReference *tcc_add_dllref(TCCState *S, const char *dllname)
{
    DLLReference *ref = tcc_mallocz(S, sizeof(DLLReference) + strlen(dllname));
    strcpy(ref->name, dllname);
    dynarray_add(S, &S->loaded_dlls, &S->nb_loaded_dlls, ref);
    return ref;
}

/* OpenBSD: choose latest from libxxx.so.x.y versions */
#if defined TARGETOS_OpenBSD && !defined _WIN32
#include <glob.h>
static int tcc_glob_so(TCCState *S, const char *pattern, char *buf, int size)
{
    const char *star;
    glob_t g;
    char *p;
    int i, v, v1, v2, v3;

    star = strchr(pattern, '*');
    if (!star || glob(pattern, 0, NULL, &g))
        return -1;
    for (v = -1, i = 0; i < g.gl_pathc; ++i) {
        p = g.gl_pathv[i];
        if (2 != sscanf(p + (star - pattern), "%d.%d.%d", &v1, &v2, &v3))
            continue;
        if ((v1 = v1 * 1000 + v2) > v)
            v = v1, pstrcpy(buf, size, p);
    }
    globfree(&g);
    return v;
}
#endif

ST_FUNC int tcc_add_file_internal(TCCState *S, const char *filename, int flags)
{
    int fd, ret = -1;

#if defined TARGETOS_OpenBSD && !defined _WIN32
    char buf[1024];
    if (tcc_glob_so(S, filename, buf, sizeof buf) >= 0)
        filename = buf;
#endif

    /* open the file */
    fd = _tcc_open(S, filename);
    if (fd < 0) {
        if (flags & AFF_PRINT_ERROR)
            tcc_error_noabort(S, "file '%s' not found", filename);
        return ret;
    }

    S->current_filename = filename;
    if (flags & AFF_TYPE_BIN) {
        ElfW(Ehdr) ehdr;
        int obj_type;

        obj_type = tcc_object_type(fd, &ehdr);
        lseek(fd, 0, SEEK_SET);

        switch (obj_type) {

        case AFF_BINTYPE_REL:
            ret = tcc_load_object_file(S, fd, 0);
            break;

        case AFF_BINTYPE_AR:
            ret = tcc_load_archive(S, fd, !(flags & AFF_WHOLE_ARCHIVE));
            break;

#ifdef TCC_TARGET_PE
        default:
            ret = pe_load_file(S, fd, filename);
            goto check_success;

#elif defined TCC_TARGET_MACHO
        case AFF_BINTYPE_DYN:
        case_dyn_or_tbd:
            if (S->output_type == TCC_OUTPUT_MEMORY) {
#ifdef TCC_IS_NATIVE
                void* dl;
                const char* soname = filename;
                if (obj_type != AFF_BINTYPE_DYN)
                    soname = macho_tbd_soname(filename);
                dl = dlopen(soname, RTLD_GLOBAL | RTLD_LAZY);
                if (dl)
                    tcc_add_dllref(S, soname)->handle = dl, ret = 0;
	        if (filename != soname)
		    tcc_free(S, (void *)soname);
#endif
            } else if (obj_type == AFF_BINTYPE_DYN) {
                ret = macho_load_dll(S, fd, filename, (flags & AFF_REFERENCED_DLL) != 0);
            } else {
                ret = macho_load_tbd(S, fd, filename, (flags & AFF_REFERENCED_DLL) != 0);
            }
            break;
        default:
        {
            const char *ext = tcc_fileextension(filename);
            if (!strcmp(ext, ".tbd"))
                goto case_dyn_or_tbd;
            if (!strcmp(ext, ".dylib")) {
                obj_type = AFF_BINTYPE_DYN;
                goto case_dyn_or_tbd;
            }
            goto check_success;
        }

#else /* unix */
        case AFF_BINTYPE_DYN:
            if (S->output_type == TCC_OUTPUT_MEMORY) {
#ifdef TCC_IS_NATIVE
                void* dl = dlopen(filename, RTLD_GLOBAL | RTLD_LAZY);
                if (dl)
                    tcc_add_dllref(S, filename)->handle = dl, ret = 0;
#endif
            } else
                ret = tcc_load_dll(S, fd, filename, (flags & AFF_REFERENCED_DLL) != 0);
            break;

        default:
            /* as GNU ld, consider it is an ld script if not recognized */
            ret = tcc_load_ldscript(S, fd);
            goto check_success;

#endif /* pe / macos / unix */

check_success:
            if (ret < 0)
                tcc_error_noabort(S, "%s: unrecognized file type", filename);
            break;

#ifdef TCC_TARGET_COFF
        case AFF_BINTYPE_C67:
            ret = tcc_load_coff(S, fd);
            break;
#endif
        }
        close(fd);
    } else {
        /* update target deps */
        dynarray_add(S, &S->target_deps, &S->nb_target_deps, tcc_strdup(S, filename));
        ret = tcc_compile(S, flags, filename, fd);
    }
    S->current_filename = NULL;
    return ret;
}

LIBTCCAPI int tcc_add_file(TCCState *S, const char *filename)
{
    int filetype = S->filetype;
    if (0 == (filetype & AFF_TYPE_MASK)) {
        /* use a file extension to detect a filetype */
        const char *ext = tcc_fileextension(filename);
        if (ext[0]) {
            ext++;
            if (!strcmp(ext, "S"))
                filetype = AFF_TYPE_ASMPP;
            else if (!strcmp(ext, "s"))
                filetype = AFF_TYPE_ASM;
            else if (!PATHCMP(ext, "c")
                     || !PATHCMP(ext, "h")
                     || !PATHCMP(ext, "i"))
                filetype = AFF_TYPE_C;
            else
                filetype |= AFF_TYPE_BIN;
        } else {
            filetype = AFF_TYPE_C;
        }
    }
    return tcc_add_file_internal(S, filename, filetype | AFF_PRINT_ERROR);
}

LIBTCCAPI int tcc_add_library_path(TCCState *S, const char *pathname)
{
    tcc_split_path(S, &S->library_paths, &S->nb_library_paths, pathname);
    return 0;
}

static int tcc_add_library_internal(TCCState *S, const char *fmt,
    const char *filename, int flags, char **paths, int nb_paths)
{
    char buf[1024];
    int i;

    for(i = 0; i < nb_paths; i++) {
        snprintf(buf, sizeof(buf), fmt, paths[i], filename);
        if (tcc_add_file_internal(S, buf, flags | AFF_TYPE_BIN) == 0)
            return 0;
    }
    return -1;
}

#ifndef TCC_TARGET_MACHO
/* find and load a dll. Return non zero if not found */
/* XXX: add '-rpath' option support ? */
ST_FUNC int tcc_add_dll(TCCState *S, const char *filename, int flags)
{
    return tcc_add_library_internal(S, "%s/%s", filename, flags,
        S->library_paths, S->nb_library_paths);
}
#endif

#if !defined TCC_TARGET_PE && !defined TCC_TARGET_MACHO
ST_FUNC int tcc_add_crt(TCCState *S, const char *filename)
{
    if (-1 == tcc_add_library_internal(S, "%s/%s",
        filename, 0, S->crt_paths, S->nb_crt_paths))
        tcc_error_noabort(S, "file '%s' not found", filename);
    return 0;
}
#endif

/* the library name is the same as the argument of the '-l' option */
LIBTCCAPI int tcc_add_library(TCCState *S, const char *libraryname)
{
#if defined TCC_TARGET_PE
    static const char * const libs[] = { "%s/%s.def", "%s/lib%s.def", "%s/%s.dll", "%s/lib%s.dll", "%s/lib%s.a", NULL };
    const char * const *pp = S->static_link ? libs + 4 : libs;
#elif defined TCC_TARGET_MACHO
    static const char * const libs[] = { "%s/lib%s.dylib", "%s/lib%s.tbd", "%s/lib%s.a", NULL };
    const char * const *pp = S->static_link ? libs + 2 : libs;
#elif defined TARGETOS_OpenBSD
    static const char * const libs[] = { "%s/lib%s.so.*", "%s/lib%s.a", NULL };
    const char * const *pp = S->static_link ? libs + 1 : libs;
#else
    static const char * const libs[] = { "%s/lib%s.so", "%s/lib%s.a", NULL };
    const char * const *pp = S->static_link ? libs + 1 : libs;
#endif
    int flags = S->filetype & AFF_WHOLE_ARCHIVE;
    while (*pp) {
        if (0 == tcc_add_library_internal(S, *pp,
            libraryname, flags, S->library_paths, S->nb_library_paths))
            return 0;
        ++pp;
    }
    return -1;
}

PUB_FUNC int tcc_add_library_err(TCCState *S, const char *libname)
{
    int ret = tcc_add_library(S, libname);
    if (ret < 0)
        tcc_error_noabort(S, "library '%s' not found", libname);
    return ret;
}

/* handle #pragma comment(lib,) */
ST_FUNC void tcc_add_pragma_libs(TCCState *S)
{
    int i;
    for (i = 0; i < S->nb_pragma_libs; i++)
        tcc_add_library_err(S, S->pragma_libs[i]);
}

LIBTCCAPI int tcc_add_symbol(TCCState *S, const char *name, const void *val)
{
#ifdef TCC_TARGET_PE
    /* On x86_64 'val' might not be reachable with a 32bit offset.
       So it is handled here as if it were in a DLL. */
    pe_putimport(S, 0, name, (uintptr_t)val);
#else
    char buf[256];
    if (S->leading_underscore) {
        buf[0] = '_';
        pstrcpy(buf + 1, sizeof(buf) - 1, name);
        name = buf;
    }
    set_global_sym(S, name, NULL, (addr_t)(uintptr_t)val); /* NULL: SHN_ABS */
#endif
    return 0;
}

LIBTCCAPI void tcc_set_lib_path(TCCState *S, const char *path)
{
    tcc_free(S, S->tcc_lib_path);
    S->tcc_lib_path = tcc_strdup(S, path);
}

/********************************************************/
/* options parser */

static int strstart(const char *val, const char **str)
{
    const char *p, *q;
    p = *str;
    q = val;
    while (*q) {
        if (*p != *q)
            return 0;
        p++;
        q++;
    }
    *str = p;
    return 1;
}

/* Like strstart, but automatically takes into account that ld options can
 *
 * - start with double or single dash (e.g. '--soname' or '-soname')
 * - arguments can be given as separate or after '=' (e.g. '-Wl,-soname,x.so'
 *   or '-Wl,-soname=x.so')
 *
 * you provide `val` always in 'option[=]' form (no leading -)
 */
static int link_option(const char *str, const char *val, const char **ptr)
{
    const char *p, *q;
    int ret;

    /* there should be 1 or 2 dashes */
    if (*str++ != '-')
        return 0;
    if (*str == '-')
        str++;

    /* then str & val should match (potentially up to '=') */
    p = str;
    q = val;

    ret = 1;
    if (q[0] == '?') {
        ++q;
        if (strstart("no-", &p))
            ret = -1;
    }

    while (*q != '\0' && *q != '=') {
        if (*p != *q)
            return 0;
        p++;
        q++;
    }

    /* '=' near eos means ',' or '=' is ok */
    if (*q == '=') {
        if (*p == 0)
            *ptr = p;
        if (*p != ',' && *p != '=')
            return 0;
        p++;
    } else if (*p) {
        return 0;
    }
    *ptr = p;
    return ret;
}

static const char *skip_linker_arg(const char **str)
{
    const char *s1 = *str;
    const char *s2 = strchr(s1, ',');
    *str = s2 ? s2++ : (s2 = s1 + strlen(s1));
    return s2;
}

static void copy_linker_arg(TCCState *S, char **pp, const char *s, int sep)
{
    const char *q = s;
    char *p = *pp;
    int l = 0;
    if (p && sep)
        p[l = strlen(p)] = sep, ++l;
    skip_linker_arg(&q);
    pstrncpy(l + (*pp = tcc_realloc(S, p, q - s + l + 1)), s, q - s);
}

/* set linker options */
static int tcc_set_linker(TCCState *S, const char *option)
{
    while (*option) {

        const char *p = NULL;
        char *end = NULL;
        int ignoring = 0;
        int ret;

        if (link_option(option, "Bsymbolic", &p)) {
            S->symbolic = 1;
        } else if (link_option(option, "nostdlib", &p)) {
            S->nostdlib = 1;
        } else if (link_option(option, "fini=", &p)) {
            copy_linker_arg(S, &S->fini_symbol, p, 0);
            ignoring = 1;
        } else if (link_option(option, "image-base=", &p)
                || link_option(option, "Ttext=", &p)) {
            S->text_addr = strtoull(p, &end, 16);
            S->has_text_addr = 1;
        } else if (link_option(option, "init=", &p)) {
            copy_linker_arg(S, &S->init_symbol, p, 0);
            ignoring = 1;
        } else if (link_option(option, "oformat=", &p)) {
#if defined(TCC_TARGET_PE)
            if (strstart("pe-", &p)) {
#elif PTR_SIZE == 8
            if (strstart("elf64-", &p)) {
#else
            if (strstart("elf32-", &p)) {
#endif
                S->output_format = TCC_OUTPUT_FORMAT_ELF;
            } else if (!strcmp(p, "binary")) {
                S->output_format = TCC_OUTPUT_FORMAT_BINARY;
#ifdef TCC_TARGET_COFF
            } else if (!strcmp(p, "coff")) {
                S->output_format = TCC_OUTPUT_FORMAT_COFF;
#endif
            } else
                goto err;

        } else if (link_option(option, "as-needed", &p)) {
            ignoring = 1;
        } else if (link_option(option, "O", &p)) {
            ignoring = 1;
        } else if (link_option(option, "export-all-symbols", &p)) {
            S->rdynamic = 1;
        } else if (link_option(option, "export-dynamic", &p)) {
            S->rdynamic = 1;
        } else if (link_option(option, "rpath=", &p)) {
            copy_linker_arg(S, &S->rpath, p, ':');
        } else if (link_option(option, "enable-new-dtags", &p)) {
            S->enable_new_dtags = 1;
        } else if (link_option(option, "section-alignment=", &p)) {
            S->section_align = strtoul(p, &end, 16);
        } else if (link_option(option, "soname=", &p)) {
            copy_linker_arg(S, &S->soname, p, 0);
#ifdef TCC_TARGET_PE
        } else if (link_option(option, "large-address-aware", &p)) {
            S->pe_characteristics |= 0x20;
        } else if (link_option(option, "file-alignment=", &p)) {
            S->pe_file_align = strtoul(p, &end, 16);
        } else if (link_option(option, "stack=", &p)) {
            S->pe_stack_size = strtoul(p, &end, 10);
        } else if (link_option(option, "subsystem=", &p)) {
#if defined(TCC_TARGET_I386) || defined(TCC_TARGET_X86_64)
            if (!strcmp(p, "native")) {
                S->pe_subsystem = 1;
            } else if (!strcmp(p, "console")) {
                S->pe_subsystem = 3;
            } else if (!strcmp(p, "gui") || !strcmp(p, "windows")) {
                S->pe_subsystem = 2;
            } else if (!strcmp(p, "posix")) {
                S->pe_subsystem = 7;
            } else if (!strcmp(p, "efiapp")) {
                S->pe_subsystem = 10;
            } else if (!strcmp(p, "efiboot")) {
                S->pe_subsystem = 11;
            } else if (!strcmp(p, "efiruntime")) {
                S->pe_subsystem = 12;
            } else if (!strcmp(p, "efirom")) {
                S->pe_subsystem = 13;
#elif defined(TCC_TARGET_ARM)
            if (!strcmp(p, "wince")) {
                S->pe_subsystem = 9;
#endif
            } else
                goto err;
#endif
        } else if (ret = link_option(option, "?whole-archive", &p), ret) {
            if (ret > 0)
                S->filetype |= AFF_WHOLE_ARCHIVE;
            else
                S->filetype &= ~AFF_WHOLE_ARCHIVE;
        } else if (link_option(option, "z=", &p)) {
            ignoring = 1;
        } else if (p) {
            return 0;
        } else {
    err:
            tcc_error(S, "unsupported linker option '%s'", option);
        }
        if (ignoring)
            tcc_warning_c(warn_unsupported)(S, "unsupported linker option '%s'", option);
        option = skip_linker_arg(&p);
    }
    return 1;
}

typedef struct TCCOption {
    const char *name;
    uint16_t index;
    uint16_t flags;
} TCCOption;

enum {
    TCC_OPTION_ignored = 0,
    TCC_OPTION_HELP,
    TCC_OPTION_HELP2,
    TCC_OPTION_v,
    TCC_OPTION_I,
    TCC_OPTION_D,
    TCC_OPTION_U,
    TCC_OPTION_P,
    TCC_OPTION_L,
    TCC_OPTION_B,
    TCC_OPTION_l,
    TCC_OPTION_bench,
    TCC_OPTION_bt,
    TCC_OPTION_b,
    TCC_OPTION_ba,
    TCC_OPTION_g,
    TCC_OPTION_c,
    TCC_OPTION_dumpversion,
    TCC_OPTION_d,
    TCC_OPTION_static,
    TCC_OPTION_std,
    TCC_OPTION_shared,
    TCC_OPTION_soname,
    TCC_OPTION_o,
    TCC_OPTION_r,
    TCC_OPTION_Wl,
    TCC_OPTION_Wp,
    TCC_OPTION_W,
    TCC_OPTION_O,
    TCC_OPTION_mfloat_abi,
    TCC_OPTION_m,
    TCC_OPTION_f,
    TCC_OPTION_isystem,
    TCC_OPTION_iwithprefix,
    TCC_OPTION_include,
    TCC_OPTION_nostdinc,
    TCC_OPTION_nostdlib,
    TCC_OPTION_print_search_dirs,
    TCC_OPTION_rdynamic,
    TCC_OPTION_pthread,
    TCC_OPTION_run,
    TCC_OPTION_w,
    TCC_OPTION_E,
    TCC_OPTION_M,
    TCC_OPTION_MD,
    TCC_OPTION_MF,
    TCC_OPTION_MM,
    TCC_OPTION_MMD,
    TCC_OPTION_x,
    TCC_OPTION_ar,
    TCC_OPTION_impdef,
};

#define TCC_OPTION_HAS_ARG 0x0001
#define TCC_OPTION_NOSEP   0x0002 /* cannot have space before option and arg */

static const TCCOption tcc_options[] = {
    { "h", TCC_OPTION_HELP, 0 },
    { "-help", TCC_OPTION_HELP, 0 },
    { "?", TCC_OPTION_HELP, 0 },
    { "hh", TCC_OPTION_HELP2, 0 },
    { "v", TCC_OPTION_v, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "-version", TCC_OPTION_v, 0 }, /* handle as verbose, also prints version*/
    { "I", TCC_OPTION_I, TCC_OPTION_HAS_ARG },
    { "D", TCC_OPTION_D, TCC_OPTION_HAS_ARG },
    { "U", TCC_OPTION_U, TCC_OPTION_HAS_ARG },
    { "P", TCC_OPTION_P, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "L", TCC_OPTION_L, TCC_OPTION_HAS_ARG },
    { "B", TCC_OPTION_B, TCC_OPTION_HAS_ARG },
    { "l", TCC_OPTION_l, TCC_OPTION_HAS_ARG },
    { "bench", TCC_OPTION_bench, 0 },
#ifdef CONFIG_TCC_BACKTRACE
    { "bt", TCC_OPTION_bt, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
#endif
#ifdef CONFIG_TCC_BCHECK
    { "b", TCC_OPTION_b, 0 },
#endif
    { "g", TCC_OPTION_g, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "c", TCC_OPTION_c, 0 },
    { "dumpversion", TCC_OPTION_dumpversion, 0},
    { "d", TCC_OPTION_d, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "static", TCC_OPTION_static, 0 },
    { "std", TCC_OPTION_std, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "shared", TCC_OPTION_shared, 0 },
    { "soname", TCC_OPTION_soname, TCC_OPTION_HAS_ARG },
    { "o", TCC_OPTION_o, TCC_OPTION_HAS_ARG },
    { "pthread", TCC_OPTION_pthread, 0},
    { "run", TCC_OPTION_run, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "rdynamic", TCC_OPTION_rdynamic, 0 },
    { "r", TCC_OPTION_r, 0 },
    { "Wl,", TCC_OPTION_Wl, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "Wp,", TCC_OPTION_Wp, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "W", TCC_OPTION_W, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "O", TCC_OPTION_O, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
#ifdef TCC_TARGET_ARM
    { "mfloat-abi", TCC_OPTION_mfloat_abi, TCC_OPTION_HAS_ARG },
#endif
    { "m", TCC_OPTION_m, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "f", TCC_OPTION_f, TCC_OPTION_HAS_ARG | TCC_OPTION_NOSEP },
    { "isystem", TCC_OPTION_isystem, TCC_OPTION_HAS_ARG },
    { "include", TCC_OPTION_include, TCC_OPTION_HAS_ARG },
    { "nostdinc", TCC_OPTION_nostdinc, 0 },
    { "nostdlib", TCC_OPTION_nostdlib, 0 },
    { "print-search-dirs", TCC_OPTION_print_search_dirs, 0 },
    { "w", TCC_OPTION_w, 0 },
    { "E", TCC_OPTION_E, 0},
    { "M", TCC_OPTION_M, 0},
    { "MD", TCC_OPTION_MD, 0},
    { "MF", TCC_OPTION_MF, TCC_OPTION_HAS_ARG },
    { "MM", TCC_OPTION_MM, 0},
    { "MMD", TCC_OPTION_MMD, 0},
    { "x", TCC_OPTION_x, TCC_OPTION_HAS_ARG },
    { "ar", TCC_OPTION_ar, 0},
#ifdef TCC_TARGET_PE
    { "impdef", TCC_OPTION_impdef, 0},
#endif
    /* ignored (silently, except after -Wunsupported) */
    { "arch", 0, TCC_OPTION_HAS_ARG},
    { "C", 0, 0 },
    { "-param", 0, TCC_OPTION_HAS_ARG },
    { "pedantic", 0, 0 },
    { "pipe", 0, 0 },
    { "s", 0, 0 },
    { "traditional", 0, 0 },
    { NULL, 0, 0 },
};

typedef struct FlagDef {
    uint16_t offset;
    uint16_t flags;
    const char *name;
} FlagDef;

#define WD_ALL    0x0001 /* warning is activated when using -Wall */
#define FD_INVERT 0x0002 /* invert value before storing */

static const FlagDef options_W[] = {
    { offsetof(TCCState, warn_all), WD_ALL, "all" },
    { offsetof(TCCState, warn_error), 0, "error" },
    { offsetof(TCCState, warn_write_strings), 0, "write-strings" },
    { offsetof(TCCState, warn_unsupported), 0, "unsupported" },
    { offsetof(TCCState, warn_implicit_function_declaration), WD_ALL, "implicit-function-declaration" },
    { offsetof(TCCState, warn_discarded_qualifiers), WD_ALL, "discarded-qualifiers" },
    { 0, 0, NULL }
};

static const FlagDef options_f[] = {
    { offsetof(TCCState, char_is_unsigned), 0, "unsigned-char" },
    { offsetof(TCCState, char_is_unsigned), FD_INVERT, "signed-char" },
    { offsetof(TCCState, nocommon), FD_INVERT, "common" },
    { offsetof(TCCState, leading_underscore), 0, "leading-underscore" },
    { offsetof(TCCState, ms_extensions), 0, "ms-extensions" },
    { offsetof(TCCState, dollars_in_identifiers), 0, "dollars-in-identifiers" },
    { offsetof(TCCState, test_coverage), 0, "test-coverage" },
    { 0, 0, NULL }
};

static const FlagDef options_m[] = {
    { offsetof(TCCState, ms_bitfields), 0, "ms-bitfields" },
#ifdef TCC_TARGET_X86_64
    { offsetof(TCCState, nosse), FD_INVERT, "sse" },
#endif
    { 0, 0, NULL }
};

static int set_flag(TCCState *S, const FlagDef *flags, const char *name)
{
    int value, mask, ret;
    const FlagDef *p;
    const char *r;
    unsigned char *f;

    r = name, value = !strstart("no-", &r), mask = 0;

    /* when called with options_W, look for -W[no-]error=<option> */
    if ((flags->flags & WD_ALL) && strstart("error=", &r))
        value = value ? WARN_ON|WARN_ERR : WARN_NOE, mask = WARN_ON;

    for (ret = -1, p = flags; p->name; ++p) {
        if (ret) {
            if (strcmp(r, p->name))
                continue;
        } else {
            if (0 == (p->flags & WD_ALL))
                continue;
        }

        f = (unsigned char *)S + p->offset;
        *f = (*f & mask) | (value ^ !!(p->flags & FD_INVERT));

        if (ret) {
            ret = 0;
            if (strcmp(r, "all"))
                break;
        }
    }
    return ret;
}

static void args_parser_add_file(TCCState *S, const char* filename, int filetype)
{
    struct filespec *f = tcc_malloc(S, sizeof *f + strlen(filename));
    f->type = filetype;
    strcpy(f->name, filename);
    dynarray_add(S, &S->files, &S->nb_files, f);
}

static int args_parser_make_argv(TCCState *S, const char *r, int *argc, char ***argv)
{
    int ret = 0, q, c;
    CString str;
    for(;;) {
        while (c = (unsigned char)*r, c && c <= ' ')
          ++r;
        if (c == 0)
            break;
        q = 0;
        cstr_new(S, &str);
        while (c = (unsigned char)*r, c) {
            ++r;
            if (c == '\\' && (*r == '"' || *r == '\\')) {
                c = *r++;
            } else if (c == '"') {
                q = !q;
                continue;
            } else if (q == 0 && c <= ' ') {
                break;
            }
            cstr_ccat(S, &str, c);
        }
        cstr_ccat(S, &str, 0);
        //printf("<%s>\n", str.data), fflush(stdout);
        dynarray_add(S, argv, argc, tcc_strdup(S, str.data));
        cstr_free(S, &str);
        ++ret;
    }
    return ret;
}

/* read list file */
static void args_parser_listfile(TCCState *S,
    const char *filename, int optind, int *pargc, char ***pargv)
{
    int fd, i;
    char *p;
    int argc = 0;
    char **argv = NULL;

    fd = open(filename, O_RDONLY | O_BINARY);
    if (fd < 0)
        tcc_error(S, "listfile '%s' not found", filename);

    p = tcc_load_text(S, fd);
    for (i = 0; i < *pargc; ++i)
        if (i == optind)
            args_parser_make_argv(S, p, &argc, &argv);
        else
            dynarray_add(S, &argv, &argc, tcc_strdup(S, (*pargv)[i]));

    tcc_free(S, p);
    dynarray_reset(S, &S->argv, &S->argc);
    *pargc = S->argc = argc, *pargv = S->argv = argv;
}

PUB_FUNC int tcc_parse_args(TCCState *S, int *pargc, char ***pargv, int optind)
{
    const TCCOption *popt;
    const char *optarg, *r;
    const char *run = NULL;
    int x;
    CString linker_arg; /* collect -Wl options */
    int tool = 0, arg_start = 0, noaction = optind;
    char **argv = *pargv;
    int argc = *pargc;

    cstr_new(S, &linker_arg);

    while (optind < argc) {
        r = argv[optind];
        if (r[0] == '@' && r[1] != '\0') {
            args_parser_listfile(S, r + 1, optind, &argc, &argv);
            continue;
        }
        optind++;
        if (tool) {
            if (r[0] == '-' && r[1] == 'v' && r[2] == 0)
                ++S->verbose;
            continue;
        }
reparse:
        if (r[0] != '-' || r[1] == '\0') {
            if (r[0] != '@') /* allow "tcc file(s) -run @ args ..." */
                args_parser_add_file(S, r, S->filetype);
            if (run) {
                tcc_set_options(S, run);
                arg_start = optind - 1;
                break;
            }
            continue;
        }

        /* find option in table */
        for(popt = tcc_options; ; ++popt) {
            const char *p1 = popt->name;
            const char *r1 = r + 1;
            if (p1 == NULL)
                tcc_error(S, "invalid option -- '%s'", r);
            if (!strstart(p1, &r1))
                continue;
            optarg = r1;
            if (popt->flags & TCC_OPTION_HAS_ARG) {
                if (*r1 == '\0' && !(popt->flags & TCC_OPTION_NOSEP)) {
                    if (optind >= argc)
                arg_err:
                        tcc_error(S, "argument to '%s' is missing", r);
                    optarg = argv[optind++];
                }
            } else if (*r1 != '\0')
                continue;
            break;
        }

        switch(popt->index) {
        case TCC_OPTION_HELP:
            x = OPT_HELP;
            goto extra_action;
        case TCC_OPTION_HELP2:
            x = OPT_HELP2;
            goto extra_action;
        case TCC_OPTION_I:
            tcc_add_include_path(S, optarg);
            break;
        case TCC_OPTION_D:
            tcc_define_symbol(S, optarg, NULL);
            break;
        case TCC_OPTION_U:
            tcc_undefine_symbol(S, optarg);
            break;
        case TCC_OPTION_L:
            tcc_add_library_path(S, optarg);
            break;
        case TCC_OPTION_B:
            /* set tcc utilities path (mainly for tcc development) */
            tcc_set_lib_path(S, optarg);
            break;
        case TCC_OPTION_l:
            args_parser_add_file(S, optarg, AFF_TYPE_LIB | (S->filetype & ~AFF_TYPE_MASK));
            S->nb_libraries++;
            break;
        case TCC_OPTION_pthread:
            S->option_pthread = 1;
            break;
        case TCC_OPTION_bench:
            S->do_bench = 1;
            break;
#ifdef CONFIG_TCC_BACKTRACE
        case TCC_OPTION_bt:
            S->rt_num_callers = atoi(optarg);
            S->do_backtrace = 1;
            S->do_debug = 1;
            break;
#endif
#ifdef CONFIG_TCC_BCHECK
        case TCC_OPTION_b:
            S->do_bounds_check = 1;
            S->do_backtrace = 1;
            S->do_debug = 1;
            break;
#endif
        case TCC_OPTION_g:
            S->do_debug = 1;
            break;
        case TCC_OPTION_c:
            x = TCC_OUTPUT_OBJ;
        set_output_type:
            if (S->output_type)
                tcc_warning(S, "-%s: overriding compiler action already specified", popt->name);
            S->output_type = x;
            break;
        case TCC_OPTION_d:
            if (*optarg == 'D')
                S->dflag = TCC_OPTION_d_D;
            else if (*optarg == 'M')
                S->dflag = TCC_OPTION_d_M;
            else if (*optarg == 't')
                S->dflag = TCC_OPTION_d_t;
            else if (isnum(*optarg))
                S->g_debug |= atoi(optarg);
            else
                goto unsupported_option;
            break;
        case TCC_OPTION_static:
            S->static_link = 1;
            break;
        case TCC_OPTION_std:
            if (strcmp(optarg, "=c11") == 0)
                S->cversion = 201112;
            break;
        case TCC_OPTION_shared:
            x = TCC_OUTPUT_DLL;
            goto set_output_type;
        case TCC_OPTION_soname:
            S->soname = tcc_strdup(S, optarg);
            break;
        case TCC_OPTION_o:
            if (S->outfile) {
                tcc_warning(S, "multiple -o option");
                tcc_free(S, S->outfile);
            }
            S->outfile = tcc_strdup(S, optarg);
            break;
        case TCC_OPTION_r:
            /* generate a .o merging several output files */
            S->option_r = 1;
            x = TCC_OUTPUT_OBJ;
            goto set_output_type;
        case TCC_OPTION_isystem:
            tcc_add_sysinclude_path(S, optarg);
            break;
        case TCC_OPTION_include:
            cstr_printf(S, &S->cmdline_incl, "#include \"%s\"\n", optarg);
            break;
        case TCC_OPTION_nostdinc:
            S->nostdinc = 1;
            break;
        case TCC_OPTION_nostdlib:
            S->nostdlib = 1;
            break;
        case TCC_OPTION_run:
#ifndef TCC_IS_NATIVE
            tcc_error(S, "-run is not available in a cross compiler");
#endif
            run = optarg;
            x = TCC_OUTPUT_MEMORY;
            goto set_output_type;
        case TCC_OPTION_v:
            do ++S->verbose; while (*optarg++ == 'v');
            ++noaction;
            break;
        case TCC_OPTION_f:
            if (set_flag(S, options_f, optarg) < 0)
                goto unsupported_option;
            break;
#ifdef TCC_TARGET_ARM
        case TCC_OPTION_mfloat_abi:
            /* tcc doesn't support soft float yet */
            if (!strcmp(optarg, "softfp")) {
                S->float_abi = ARM_SOFTFP_FLOAT;
            } else if (!strcmp(optarg, "hard"))
                S->float_abi = ARM_HARD_FLOAT;
            else
                tcc_error(S, "unsupported float abi '%s'", optarg);
            break;
#endif
        case TCC_OPTION_m:
            if (set_flag(S, options_m, optarg) < 0) {
                if (x = atoi(optarg), x != 32 && x != 64)
                    goto unsupported_option;
                if (PTR_SIZE != x/8)
                    return x;
                ++noaction;
            }
            break;
        case TCC_OPTION_W:
            S->warn_none = 0;
            if (optarg[0] && set_flag(S, options_W, optarg) < 0)
                goto unsupported_option;
            break;
        case TCC_OPTION_w:
            S->warn_none = 1;
            break;
        case TCC_OPTION_rdynamic:
            S->rdynamic = 1;
            break;
        case TCC_OPTION_Wl:
            if (linker_arg.size)
                --linker_arg.size, cstr_ccat(S, &linker_arg, ',');
            cstr_cat(S, &linker_arg, optarg, 0);
            if (tcc_set_linker(S, linker_arg.data))
                cstr_free(S, &linker_arg);
            break;
        case TCC_OPTION_Wp:
            r = optarg;
            goto reparse;
        case TCC_OPTION_E:
            x = TCC_OUTPUT_PREPROCESS;
            goto set_output_type;
        case TCC_OPTION_P:
            S->Pflag = atoi(optarg) + 1;
            break;
        case TCC_OPTION_M:
            S->include_sys_deps = 1;
            // fall through
        case TCC_OPTION_MM:
            S->just_deps = 1;
            if(!S->deps_outfile)
                S->deps_outfile = tcc_strdup(S, "-");
            // fall through
        case TCC_OPTION_MMD:
            S->gen_deps = 1;
            break;
        case TCC_OPTION_MD:
            S->gen_deps = 1;
            S->include_sys_deps = 1;
            break;
        case TCC_OPTION_MF:
            S->deps_outfile = tcc_strdup(S, optarg);
            break;
        case TCC_OPTION_dumpversion:
            printf ("%s\n", TCC_VERSION);
            exit(0);
            break;
        case TCC_OPTION_x:
            x = 0;
            if (*optarg == 'c')
                x = AFF_TYPE_C;
            else if (*optarg == 'a')
                x = AFF_TYPE_ASMPP;
            else if (*optarg == 'b')
                x = AFF_TYPE_BIN;
            else if (*optarg == 'n')
                x = AFF_TYPE_NONE;
            else
                tcc_warning(S, "unsupported language '%s'", optarg);
            S->filetype = x | (S->filetype & ~AFF_TYPE_MASK);
            break;
        case TCC_OPTION_O:
            S->optimize = atoi(optarg);
            break;
        case TCC_OPTION_print_search_dirs:
            x = OPT_PRINT_DIRS;
            goto extra_action;
        case TCC_OPTION_impdef:
            x = OPT_IMPDEF;
            goto extra_action;
        case TCC_OPTION_ar:
            x = OPT_AR;
        extra_action:
            arg_start = optind - 1;
            if (arg_start != noaction)
                tcc_error(S, "cannot parse %s here", r);
            tool = x;
            break;
        default:
unsupported_option:
            tcc_warning_c(warn_unsupported)(S, "unsupported option '%s'", r);
            break;
        }
    }
    if (linker_arg.size) {
        r = linker_arg.data;
        goto arg_err;
    }
    *pargc = argc - arg_start;
    *pargv = argv + arg_start;
    if (tool)
        return tool;
    if (optind != noaction)
        return 0;
    if (S->verbose == 2)
        return OPT_PRINT_DIRS;
    if (S->verbose)
        return OPT_V;
    return OPT_HELP;
}

LIBTCCAPI void tcc_set_options(TCCState *S, const char *r)
{
    char **argv = NULL;
    int argc = 0;
    args_parser_make_argv(S, r, &argc, &argv);
    tcc_parse_args(S, &argc, &argv, 0);
    dynarray_reset(S, &argv, &argc);
}

PUB_FUNC void tcc_print_stats(TCCState *S, unsigned total_time)
{
    if (total_time < 1)
        total_time = 1;
    if (total_bytes < 1)
        total_bytes = 1;
    fprintf(stderr, "* %d idents, %d lines, %d bytes\n"
                    "* %0.3f s, %u lines/s, %0.1f MB/s\n",
           total_idents, total_lines, total_bytes,
           (double)total_time/1000,
           (unsigned)total_lines*1000/total_time,
           (double)total_bytes/1000/total_time);
    fprintf(stderr, "* text %d, data.rw %d, data.ro %d, bss %d bytes\n",
           S->total_output[0],
           S->total_output[1],
           S->total_output[2],
           S->total_output[3]
           );
#ifdef MEM_DEBUG
    fprintf(stderr, "* %d bytes memory used\n", mem_max_size);
#endif
}

#include "tccqsort.c"
