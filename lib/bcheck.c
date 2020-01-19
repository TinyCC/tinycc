/*
 *  Tiny C Memory and bounds checker
 * 
 *  Copyright (c) 2002 Fabrice Bellard
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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if !defined(__FreeBSD__) \
 && !defined(__FreeBSD_kernel__) \
 && !defined(__DragonFly__) \
 && !defined(__OpenBSD__) \
 && !defined(__NetBSD__)
#include <malloc.h>
#endif

#if !defined(_WIN32)
#include <unistd.h>
#endif

#define BOUND_DEBUG             (1)
#define BOUND_STATISTIC         (1)

#if BOUND_DEBUG
 #define dprintf(a...)         if (print_calls) fprintf(a)
#else
 #define dprintf(a...)
#endif

#ifdef __attribute__
  /* an __attribute__ macro is defined in the system headers */
  #undef __attribute__ 
#endif
#define FASTCALL __attribute__((regparm(3)))

#ifdef _WIN32
# define DLL_EXPORT __declspec(dllexport)
#else
# define DLL_EXPORT
#endif

#if defined(__FreeBSD__) \
 || defined(__FreeBSD_kernel__) \
 || defined(__DragonFly__) \
 || defined(__OpenBSD__) \
 || defined(__NetBSD__) \
 || defined(__dietlibc__)

#define INIT_SEM()
#define EXIT_SEM()
#define WAIT_SEM()
#define POST_SEM()
#define HAVE_MEMALIGN          (0)
#define HAS_ENVIRON            (0)
#define MALLOC_REDIR           (0)
#define HAVE_PTHREAD_CREATE    (0)
#define HAVE_CTYPE             (0)
#define HAVE_ERRNO             (0)

#elif defined(_WIN32)

#include <windows.h>
static CRITICAL_SECTION bounds_sem;
#define INIT_SEM()             InitializeCriticalSection(&bounds_sem)
#define EXIT_SEM()             DeleteCriticalSection(&bounds_sem)
#define WAIT_SEM()             EnterCriticalSection(&bounds_sem)
#define POST_SEM()             LeaveCriticalSection(&bounds_sem)
#define HAVE_MEMALIGN          (0)
#define HAS_ENVIRON            (0)
#define MALLOC_REDIR           (0)
#define HAVE_PTHREAD_CREATE    (0)
#define HAVE_CTYPE             (0)
#define HAVE_ERRNO             (0)

#else

#define __USE_GNU              /* get RTLD_NEXT */
#include <sys/mman.h>
#include <ctype.h>
#include <pthread.h>
#include <dlfcn.h>
#include <errno.h>
#if 0
#include <semaphore.h>
static sem_t bounds_sem;
#define INIT_SEM()             sem_init (&bounds_sem, 0, 1)
#define EXIT_SEM()             sem_destroy (&bounds_sem)
#define WAIT_SEM()             if (use_sem) while (sem_wait (&bounds_sem) < 0 \
                                                   && errno == EINTR)
#define POST_SEM()             if (use_sem) sem_post (&bounds_sem)
#else
static pthread_spinlock_t bounds_spin;
/* about 25% faster then semaphore. */
#define INIT_SEM()             pthread_spin_init (&bounds_spin, 0)
#define EXIT_SEM()             pthread_spin_destroy (&bounds_spin)
#define WAIT_SEM()             if (use_sem) pthread_spin_lock (&bounds_spin)
#define POST_SEM()             if (use_sem) pthread_spin_unlock (&bounds_spin)
#endif
#define HAVE_MEMALIGN          (1)
#define HAS_ENVIRON            (1)
#define MALLOC_REDIR           (1)
#define HAVE_PTHREAD_CREATE    (1)
#define HAVE_CTYPE             (1)
#define HAVE_ERRNO             (1)

static void *(*malloc_redir) (size_t);
static void *(*calloc_redir) (size_t, size_t);
static void (*free_redir) (void *);
static void *(*realloc_redir) (void *, size_t);
static void *(*memalign_redir) (size_t, size_t);
static int (*pthread_create_redir) (pthread_t *thread,
                                    const pthread_attr_t *attr,
                                    void *(*start_routine)(void *), void *arg);
static unsigned int pool_index;
static unsigned char __attribute__((aligned(16))) initial_pool[256];
static unsigned char use_sem;

#endif

#define TCC_TYPE_NONE           (0)
#define TCC_TYPE_MALLOC         (1)
#define TCC_TYPE_CALLOC         (2)
#define TCC_TYPE_REALLOC        (3)
#define TCC_TYPE_MEMALIGN       (4)
#define TCC_TYPE_STRDUP         (5)

/* this pointer is generated when bound check is incorrect */
#define INVALID_POINTER ((void *)(-2))

typedef struct tree_node Tree;
struct tree_node {
    Tree * left, * right;
    size_t start;
    size_t size;
    unsigned char type;
    unsigned char is_invalid; /* true if pointers outside region are invalid */
};

typedef struct alloca_list_struct {
    size_t fp;
    void *p;
    struct alloca_list_struct *next;
} alloca_list_type;

#define BOUND_STATISTIC_SPLAY   (0)
static Tree * splay (size_t addr, Tree *t);
static Tree * splay_end (size_t addr, Tree *t);
static Tree * splay_insert(size_t addr, size_t size, Tree * t);
static Tree * splay_delete(size_t addr, Tree *t);
void splay_printtree(Tree * t, int d);

/* external interface */
void __bound_checking (int no_check);
void __bound_never_fatal (int no_check);
DLL_EXPORT void * __bound_ptr_add(void *p, size_t offset);
DLL_EXPORT void * __bound_ptr_indir1(void *p, size_t offset);
DLL_EXPORT void * __bound_ptr_indir2(void *p, size_t offset);
DLL_EXPORT void * __bound_ptr_indir4(void *p, size_t offset);
DLL_EXPORT void * __bound_ptr_indir8(void *p, size_t offset);
DLL_EXPORT void * __bound_ptr_indir12(void *p, size_t offset);
DLL_EXPORT void * __bound_ptr_indir16(void *p, size_t offset);
DLL_EXPORT void FASTCALL __bound_local_new(void *p1);
DLL_EXPORT void FASTCALL __bound_local_delete(void *p1);
void __bound_init(size_t *);
void __bound_main_arg(char **p);
void __bound_exit(void);
#if !defined(_WIN32)
void *__bound_mmap (void *start, size_t size, int prot, int flags, int fd,
                    off_t offset);
int __bound_munmap (void *start, size_t size);
#endif
DLL_EXPORT void __bound_new_region(void *p, size_t size);
DLL_EXPORT void *__bound_memcpy(void *dst, const void *src, size_t size);
DLL_EXPORT int __bound_memcmp(const void *s1, const void *s2, size_t size);
DLL_EXPORT void *__bound_memmove(void *dst, const void *src, size_t size);
DLL_EXPORT void *__bound_memset(void *dst, int c, size_t size);
DLL_EXPORT int __bound_strlen(const char *s);
DLL_EXPORT char *__bound_strcpy(char *dst, const char *src);
DLL_EXPORT char *__bound_strncpy(char *dst, const char *src, size_t n);
DLL_EXPORT int __bound_strcmp(const char *s1, const char *s2);
DLL_EXPORT int __bound_strncmp(const char *s1, const char *s2, size_t n);
DLL_EXPORT char *__bound_strcat(char *dest, const char *src);
DLL_EXPORT char *__bound_strchr(const char *string, int ch);
DLL_EXPORT char *__bound_strdup(const char *s);

#if !MALLOC_REDIR
DLL_EXPORT void *__bound_malloc(size_t size, const void *caller);
DLL_EXPORT void *__bound_memalign(size_t size, size_t align, const void *caller);
DLL_EXPORT void __bound_free(void *ptr, const void *caller);
DLL_EXPORT void *__bound_realloc(void *ptr, size_t size, const void *caller);
DLL_EXPORT void *__bound_calloc(size_t nmemb, size_t size);
#endif

#define FREE_REUSE_SIZE (100)
static unsigned int free_reuse_index;
static void *free_reuse_list[FREE_REUSE_SIZE];

static Tree *tree = NULL;
#define TREE_REUSE      (1)
#if TREE_REUSE
static Tree *tree_free_list;
#endif
static alloca_list_type *alloca_list;

static unsigned char inited;
static unsigned char print_warn_ptr_add;
static unsigned char print_calls;
static unsigned char print_heap;
static unsigned char print_statistic;
static unsigned char no_strdup;
static signed char never_fatal;
static signed char no_checking = 1;
static char exec[100];

#if BOUND_STATISTIC
static unsigned long long bound_ptr_add_count;
static unsigned long long bound_ptr_indir1_count;
static unsigned long long bound_ptr_indir2_count;
static unsigned long long bound_ptr_indir4_count;
static unsigned long long bound_ptr_indir8_count;
static unsigned long long bound_ptr_indir12_count;
static unsigned long long bound_ptr_indir16_count;
static unsigned long long bound_local_new_count;
static unsigned long long bound_local_delete_count;
static unsigned long long bound_malloc_count;
static unsigned long long bound_calloc_count;
static unsigned long long bound_realloc_count;
static unsigned long long bound_free_count;
static unsigned long long bound_memalign_count;
static unsigned long long bound_mmap_count;
static unsigned long long bound_munmap_count;
static unsigned long long bound_alloca_count;
static unsigned long long bound_mempcy_count;
static unsigned long long bound_memcmp_count;
static unsigned long long bound_memmove_count;
static unsigned long long bound_memset_count;
static unsigned long long bound_strlen_count;
static unsigned long long bound_strcpy_count;
static unsigned long long bound_strncpy_count;
static unsigned long long bound_strcmp_count;
static unsigned long long bound_strncmp_count;
static unsigned long long bound_strcat_count;
static unsigned long long bound_strchr_count;
static unsigned long long bound_strdup_count;
static unsigned long long bound_not_found;
#define INCR_COUNT(x)          ++x
#else
#define INCR_COUNT(x)
#endif
#if BOUND_STATISTIC_SPLAY
static unsigned long long bound_splay;
static unsigned long long bound_splay_end;
static unsigned long long bound_splay_insert;
static unsigned long long bound_splay_delete;
#define INCR_COUNT_SPLAY(x)    ++x
#else
#define INCR_COUNT_SPLAY(x)
#endif

/* currently only i386/x86_64 supported. Change for other platforms */
static void fetch_and_add(signed char* variable, signed char value)
{
#if defined __i386__ || defined __x86_64__
      __asm__ volatile("lock; addb %0, %1"
        : "+r" (value), "+m" (*variable) // input+output
        : // No input-only
        : "memory"
      );
#else
      *variable += value;
#endif
}

/* enable/disable checking. This can be used in signal handlers. */
void __bound_checking (int no_check)
{
    fetch_and_add (&no_checking, no_check);
}

/* enable/disable checking. This can be used in signal handlers. */
void __bound_never_fatal (int neverfatal)
{
    fetch_and_add (&never_fatal, neverfatal);
}

int tcc_backtrace(const char *fmt, ...);

/* print a bound error message */
#define bound_warning(...) \
    tcc_backtrace("^bcheck.c^BCHECK: " __VA_ARGS__)

#define bound_error(...)            \
    do {                            \
        bound_warning(__VA_ARGS__); \
        if (never_fatal == 0)       \
            exit(255);              \
    } while (0)

static void bound_alloc_error(const char *s)
{
    fprintf(stderr,"FATAL: %s\n",s);
    exit (1);
}

static void bound_not_found_warning(const char *file, const char *function,
                                    void *ptr)
{
    dprintf(stderr, "%s%s, %s(): Not found %p\n", exec, file, function, ptr);
}

/* return '(p + offset)' for pointer arithmetic (a pointer can reach
   the end of a region in this case */
void * __bound_ptr_add(void *p, size_t offset)
{
    size_t addr = (size_t)p;

    if (no_checking)
        return p + offset;

    dprintf(stderr, "%s, %s(): %p 0x%lx\n",
            __FILE__, __FUNCTION__, p, (unsigned long)offset);

    WAIT_SEM ();
    INCR_COUNT(bound_ptr_add_count);
    if (tree) {
        addr -= tree->start;
        if (addr >= tree->size) {
            addr = (size_t)p;
            tree = splay (addr, tree);
            addr -= tree->start;
        }
        if (addr >= tree->size) {
            addr = (size_t)p;
            tree = splay_end (addr, tree);
            addr -= tree->start;
        }
        if (addr <= tree->size) {
            if (tree->is_invalid || addr + offset > tree->size) {
                POST_SEM ();
                if (print_warn_ptr_add)
                    bound_warning("%p is outside of the region", p + offset);
                if (never_fatal <= 0)
                    return INVALID_POINTER; /* return an invalid pointer */
                return p + offset;
            }
        }
        else if (p) { /* Allow NULL + offset. offsetoff is using it. */
            INCR_COUNT(bound_not_found);
            POST_SEM ();
            bound_not_found_warning (__FILE__, __FUNCTION__, p);
            return p + offset;
        }
    }
    POST_SEM ();
    return p + offset;
}

/* return '(p + offset)' for pointer indirection (the resulting must
   be strictly inside the region */
#define BOUND_PTR_INDIR(dsize)                                                 \
void * __bound_ptr_indir ## dsize (void *p, size_t offset)                     \
{                                                                              \
    size_t addr = (size_t)p;                                                   \
                                                                               \
    if (no_checking)                                                           \
        return p + offset;                                                     \
                                                                               \
    dprintf(stderr, "%s, %s(): %p 0x%lx\n",                                    \
            __FILE__, __FUNCTION__, p, (unsigned long)offset);                 \
    WAIT_SEM ();                                                               \
    INCR_COUNT(bound_ptr_indir ## dsize ## _count);                            \
    if (tree) {                                                                \
        addr -= tree->start;                                                   \
        if (addr >= tree->size) {                                              \
            addr = (size_t)p;                                                  \
            tree = splay (addr, tree);                                         \
            addr -= tree->start;                                               \
        }                                                                      \
        if (addr >= tree->size) {                                              \
            addr = (size_t)p;                                                  \
            tree = splay_end (addr, tree);                                     \
            addr -= tree->start;                                               \
        }                                                                      \
        if (addr <= tree->size) {                                              \
            if (tree->is_invalid || addr + offset + dsize > tree->size) {      \
                POST_SEM ();                                                   \
                bound_warning("%p is outside of the region", p + offset); \
                if (never_fatal <= 0)                                          \
                    return INVALID_POINTER; /* return an invalid pointer */    \
                return p + offset;                                             \
            }                                                                  \
        }                                                                      \
        else {                                                                 \
            INCR_COUNT(bound_not_found);                                       \
            POST_SEM ();                                                       \
            bound_not_found_warning (__FILE__, __FUNCTION__, p);               \
            return p + offset;                                                 \
        }                                                                      \
    }                                                                          \
    POST_SEM ();                                                               \
    return p + offset;                                                         \
}

BOUND_PTR_INDIR(1)
BOUND_PTR_INDIR(2)
BOUND_PTR_INDIR(4)
BOUND_PTR_INDIR(8)
BOUND_PTR_INDIR(12)
BOUND_PTR_INDIR(16)

#if defined(__GNUC__) && (__GNUC__ >= 6)
/*
 * At least gcc 6.2 complains when __builtin_frame_address is used with
 * nonzero argument.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"
#endif

/* return the frame pointer of the caller */
#define GET_CALLER_FP(fp)\
{\
    fp = (size_t)__builtin_frame_address(1);\
}

/* called when entering a function to add all the local regions */
void FASTCALL __bound_local_new(void *p1) 
{
    size_t addr, fp, *p = p1;

    if (no_checking)
         return;
    GET_CALLER_FP(fp);
    dprintf(stderr, "%s, %s(): p1=%p fp=%p\n",
            __FILE__, __FUNCTION__, p, (void *)fp);
    WAIT_SEM ();
    while ((addr = p[0])) {
        INCR_COUNT(bound_local_new_count);
        tree = splay_insert(addr + fp, p[1], tree);
        p += 2;
    }
    POST_SEM ();
#if BOUND_DEBUG
    if (print_calls) {
        p = p1;
        while ((addr = p[0])) {
            if (addr == 1) {
                dprintf(stderr, "%s, %s(): alloca/vla used\n",
                        __FILE__, __FUNCTION__);
            }
            else {
                dprintf(stderr, "%s, %s(): %p 0x%lx\n",
                        __FILE__, __FUNCTION__,
                        (void *) (addr + fp), (unsigned long) p[1]);
            }
            p += 2;
        }
    }
#endif
}

/* called when leaving a function to delete all the local regions */
void FASTCALL __bound_local_delete(void *p1) 
{
    size_t addr, fp, *p = p1;
    alloca_list_type *free_list = NULL;

    if (no_checking)
         return;
    GET_CALLER_FP(fp);
    dprintf(stderr, "%s, %s(): p1=%p fp=%p\n",
            __FILE__, __FUNCTION__, p, (void *)fp);
    WAIT_SEM ();
    while ((addr = p[0])) {
        INCR_COUNT(bound_local_delete_count);
        tree = splay_delete(addr + fp, tree);
        p += 2;
    }
    {
            alloca_list_type *last = NULL;
            alloca_list_type *cur = alloca_list;

            while (cur) {
                if (cur->fp == fp) {
                    if (last)
                        last->next = cur->next;
                    else
                        alloca_list = cur->next;
                    tree = splay_delete ((size_t) cur->p, tree);
                    cur->next = free_list;
                    free_list = cur;
                    cur = last ? last->next : alloca_list;
                 }
                 else {
                     last = cur;
                     cur = cur->next;
                 }
            }
    }

    POST_SEM ();
    while (free_list) {
        alloca_list_type *next = free_list->next;

        dprintf(stderr, "%s, %s(): remove alloca/vla %p\n",
               __FILE__, __FUNCTION__, free_list->p);
#if MALLOC_REDIR
        free_redir (free_list);
#else
        free (free_list);
#endif
        free_list = next;
    }
#if BOUND_DEBUG
    if (print_calls) {
        p = p1;
        while ((addr = p[0])) {
            if (addr != 1) {
                dprintf(stderr, "%s, %s(): %p 0x%lx\n",
                        __FILE__, __FUNCTION__,
                        (void *) (addr + fp), (unsigned long) p[1]);
            }
            p+= 2;
        }
    }
#endif
}

/* used by alloca */
void __bound_new_region(void *p, size_t size)
{
    size_t fp;
    alloca_list_type *last;
    alloca_list_type *cur;
    alloca_list_type *new;

    dprintf(stderr, "%s, %s(): %p, 0x%lx\n",
            __FILE__, __FUNCTION__, p, (unsigned long)size);
    GET_CALLER_FP (fp);
#if MALLOC_REDIR
    new = malloc_redir (sizeof (alloca_list_type));
#else
    new = malloc (sizeof (alloca_list_type));
#endif
    WAIT_SEM ();
    INCR_COUNT(bound_alloca_count);
    last = NULL;
    cur = alloca_list;
    while (cur) {
        if (cur->fp == fp && cur->p == p) {
            if (last)
                last->next = cur->next;
            else
                alloca_list = cur->next;
            tree = splay_delete((size_t)p, tree);
            break;
        }
        last = cur;
        cur = cur->next;
    }
    if (no_checking == 0)
        tree = splay_insert((size_t)p, size, tree);
    if (new) {
        new->fp = fp;
        new->p = p;
        new->next = alloca_list;
        alloca_list = new;
    }
    POST_SEM ();
    if (cur) {
        dprintf(stderr, "%s, %s(): remove alloca/vla %p\n",
                __FILE__, __FUNCTION__, cur->p);
#if MALLOC_REDIR
        free_redir (cur);
#else
        free (cur);
#endif
    }
}

#if defined(__GNUC__) && (__GNUC__ >= 6)
#pragma GCC diagnostic pop
#endif

void __bound_init(size_t *p)
{
    dprintf(stderr, "%s, %s(): start\n", __FILE__, __FUNCTION__);

    if (inited) {
        WAIT_SEM();
        goto add_bounds;
    }
    inited = 1;

    print_warn_ptr_add = getenv ("TCC_BOUNDS_WARN_POINTER_ADD") != NULL;
    print_calls = getenv ("TCC_BOUNDS_PRINT_CALLS") != NULL;
    print_heap = getenv ("TCC_BOUNDS_PRINT_HEAP") != NULL;
    print_statistic = getenv ("TCC_BOUNDS_PRINT_STATISTIC") != NULL;
    never_fatal = getenv ("TCC_BOUNDS_NEVER_FATAL") != NULL;

    INIT_SEM ();

#if MALLOC_REDIR
    {
        void *addr = RTLD_NEXT;

        /* tcc -run required RTLD_DEFAULT. Normal usage requires RTLD_NEXT */
        *(void **) (&malloc_redir) = dlsym (addr, "malloc");
        if (malloc_redir == NULL) {
            dprintf(stderr, "%s, %s(): use RTLD_DEFAULT\n",
                    __FILE__, __FUNCTION__);
            addr = RTLD_DEFAULT;
            *(void **) (&malloc_redir) = dlsym (addr, "malloc");
        }
        *(void **) (&calloc_redir) = dlsym (addr, "calloc");
        *(void **) (&free_redir) = dlsym (addr, "free");
        *(void **) (&realloc_redir) = dlsym (addr, "realloc");
        *(void **) (&memalign_redir) = dlsym (addr, "memalign");
        dprintf(stderr, "%s, %s(): malloc_redir %p\n",
                __FILE__, __FUNCTION__, malloc_redir);
        dprintf(stderr, "%s, %s(): free_redir %p\n",
                __FILE__, __FUNCTION__, free_redir);
        dprintf(stderr, "%s, %s(): realloc_redir %p\n",
                __FILE__, __FUNCTION__, realloc_redir);
        dprintf(stderr, "%s, %s(): memalign_redir %p\n",
                __FILE__, __FUNCTION__, memalign_redir);
        if (malloc_redir == NULL || free_redir == NULL)
            bound_alloc_error ("Cannot redirect malloc/free");
#if HAVE_PTHREAD_CREATE
        *(void **) (&pthread_create_redir) = dlsym (addr, "pthread_create");
        dprintf(stderr, "%s, %s(): pthread_create_redir %p\n",
                __FILE__, __FUNCTION__, pthread_create_redir);
#endif
    }
#endif

#ifdef __linux__
    {
        FILE *fp;
        unsigned char found;
        unsigned long long start;
        unsigned long long end;
        unsigned long long ad =
            (unsigned long long) __builtin_return_address(0);
        char line[1000];

        /* Display exec name. Usefull when a lot of code is compiled with tcc */
        fp = fopen ("/proc/self/comm", "r");
        if (fp) {
            memset (exec, 0, sizeof(exec));
            fread (exec, 1, sizeof(exec) - 2, fp);
            if (strchr(exec,'\n'))
                *strchr(exec,'\n') = '\0';
            strcat (exec, ":");
            fclose (fp);
        }
        /* check if dlopen is used (is threre a better way?) */ 
        found = 0;
        fp = fopen ("/proc/self/maps", "r");
        if (fp) {
            while (fgets (line, sizeof(line), fp)) {
                if (sscanf (line, "%Lx-%Lx", &start, &end) == 2 &&
                            ad >= start && ad < end) {
                    found = 1;
                    break;
                }
                if (strstr (line,"[heap]"))
                    break;
            }
            fclose (fp);
        }
        if (found == 0) {
            use_sem = 1;
            no_strdup = 1;
        }
    }
#endif

    WAIT_SEM ();

#if HAVE_CTYPE
    /* XXX: Does not work if locale is changed */
    tree = splay_insert((size_t) __ctype_b_loc(),
                        sizeof (unsigned short *), tree);
    tree = splay_insert((size_t) (*__ctype_b_loc() - 128),
                        384 * sizeof (unsigned short), tree);
    tree = splay_insert((size_t) __ctype_tolower_loc(),
                        sizeof (__int32_t *), tree);
    tree = splay_insert((size_t) (*__ctype_tolower_loc() - 128),
                        384 * sizeof (__int32_t), tree);
    tree = splay_insert((size_t) __ctype_toupper_loc(),
                        sizeof (__int32_t *), tree);
    tree = splay_insert((size_t) (*__ctype_toupper_loc() - 128),
                        384 * sizeof (__int32_t), tree);
#endif
#if HAVE_ERRNO
    tree = splay_insert((size_t) (&errno), sizeof (int), tree);
#endif

add_bounds:
    if (!p)
        goto no_bounds;

    /* add all static bound check values */
    while (p[0] != 0) {
        tree = splay_insert(p[0], p[1], tree);
#if BOUND_DEBUG
        if (print_calls) {
            dprintf(stderr, "%s, %s(): static var %p 0x%lx\n",
                    __FILE__, __FUNCTION__,
                    (void *) p[0], (unsigned long) p[1]);
        }
#endif
        p += 2;
    }
no_bounds:

    POST_SEM ();
    no_checking = 0;
    dprintf(stderr, "%s, %s(): end\n\n", __FILE__, __FUNCTION__);
}

void __bound_main_arg(char **p)
{
    char *start = (char *) p;

    WAIT_SEM ();
    while (*p) {
        tree = splay_insert((size_t) *p, strlen (*p) + 1, tree);
        ++p;
    }
    tree = splay_insert((size_t) start, (char *) p - start, tree);
    POST_SEM ();
#if BOUND_DEBUG
    if (print_calls) {
        p = (char **) start;
        while (*p) {
            dprintf(stderr, "%s, %s(): %p 0x%lx\n",
                    __FILE__, __FUNCTION__,
                    *p, (unsigned long)(strlen (*p) + 1));
            ++p;
        }
        dprintf(stderr, "%s, %s(): argv %p 0x%lx\n",
                __FILE__, __FUNCTION__,
                start, (unsigned long)((char *) p - start));
    }
#endif

#if HAS_ENVIRON
    {
        extern char **environ;

        WAIT_SEM ();
        p = environ;
        start = (char *) p;
        while (*p) {
            tree = splay_insert((size_t) *p, strlen (*p) + 1, tree);
            ++p;
        }
        tree = splay_insert((size_t) start, (char *) p - start, tree);
        POST_SEM ();
#if BOUND_DEBUG
        if (print_calls) {
            p = environ;
            while (*p) {
                dprintf(stderr, "%s, %s(): %p 0x%lx\n",
                        __FILE__, __FUNCTION__,
                        *p, (unsigned long)(strlen (*p) + 1));
                ++p;
            }
            dprintf(stderr, "%s, %s(): environ %p 0x%lx\n",
                    __FILE__, __FUNCTION__,
                    start, (unsigned long)((char *) p - start));
        }
#endif
    }
#endif
}

void __attribute__((destructor)) __bound_exit(void)
{
    int i;
    static const char * const alloc_type[] = {
        "", "malloc", "calloc", "realloc", "memalign", "strdup"
    };

    dprintf(stderr, "%s, %s():\n", __FILE__, __FUNCTION__);

    if (inited) {
#if !defined(_WIN32)
        if (print_heap) {
            extern void __libc_freeres (void);
            __libc_freeres ();
        }
#endif

        no_checking = 1;

        WAIT_SEM ();
        while (alloca_list) {
            alloca_list_type *next = alloca_list->next;

            tree = splay_delete ((size_t) alloca_list->p, tree);
#if MALLOC_REDIR
            free_redir (alloca_list);
#else
            free (alloca_list);
#endif
            alloca_list = next;
        }
        for (i = 0; i < FREE_REUSE_SIZE; i++) {
            if (free_reuse_list[i]) {
                tree = splay_delete ((size_t) free_reuse_list[i], tree);
#if MALLOC_REDIR
                free_redir (free_reuse_list[i]);
#else
                free (free_reuse_list[i]);
#endif
             }
        }
        while (tree) {
            if (print_heap && tree->type != 0)
                fprintf (stderr, "%s, %s(): %s found size %lu\n",
                         __FILE__, __FUNCTION__, alloc_type[tree->type],
                         (unsigned long) tree->size);
            tree = splay_delete (tree->start, tree);
        }
#if TREE_REUSE
        while (tree_free_list) {
            Tree *next = tree_free_list->left;
#if MALLOC_REDIR
            free_redir (tree_free_list);
#else
            free (tree_free_list);
#endif
            tree_free_list = next;
        }
#endif
        POST_SEM ();
        EXIT_SEM ();
        inited = 0;
        if (print_statistic) {
#if BOUND_STATISTIC
            fprintf (stderr, "bound_ptr_add_count      %llu\n", bound_ptr_add_count);
            fprintf (stderr, "bound_ptr_indir1_count   %llu\n", bound_ptr_indir1_count);
            fprintf (stderr, "bound_ptr_indir2_count   %llu\n", bound_ptr_indir2_count);
            fprintf (stderr, "bound_ptr_indir4_count   %llu\n", bound_ptr_indir4_count);
            fprintf (stderr, "bound_ptr_indir8_count   %llu\n", bound_ptr_indir8_count);
            fprintf (stderr, "bound_ptr_indir12_count  %llu\n", bound_ptr_indir12_count);
            fprintf (stderr, "bound_ptr_indir16_count  %llu\n", bound_ptr_indir16_count);
            fprintf (stderr, "bound_local_new_count    %llu\n", bound_local_new_count);
            fprintf (stderr, "bound_local_delete_count %llu\n", bound_local_delete_count);
            fprintf (stderr, "bound_malloc_count       %llu\n", bound_malloc_count);
            fprintf (stderr, "bound_calloc_count       %llu\n", bound_calloc_count);
            fprintf (stderr, "bound_realloc_count      %llu\n", bound_realloc_count);
            fprintf (stderr, "bound_free_count         %llu\n", bound_free_count);
            fprintf (stderr, "bound_memalign_count     %llu\n", bound_memalign_count);
            fprintf (stderr, "bound_mmap_count         %llu\n", bound_mmap_count);
            fprintf (stderr, "bound_munmap_count       %llu\n", bound_munmap_count);
            fprintf (stderr, "bound_alloca_count       %llu\n", bound_alloca_count);
            fprintf (stderr, "bound_mempcy_count       %llu\n", bound_mempcy_count);
            fprintf (stderr, "bound_memcmp_count       %llu\n", bound_memcmp_count);
            fprintf (stderr, "bound_memmove_count      %llu\n", bound_memmove_count);
            fprintf (stderr, "bound_memset_count       %llu\n", bound_memset_count);
            fprintf (stderr, "bound_strlen_count       %llu\n", bound_strlen_count);
            fprintf (stderr, "bound_strcpy_count       %llu\n", bound_strcpy_count);
            fprintf (stderr, "bound_strncpy_count      %llu\n", bound_strncpy_count);
            fprintf (stderr, "bound_strcmp_count       %llu\n", bound_strcmp_count);
            fprintf (stderr, "bound_strncmp_count      %llu\n", bound_strncmp_count);
            fprintf (stderr, "bound_strcat_count       %llu\n", bound_strcat_count);
            fprintf (stderr, "bound_strchr_count       %llu\n", bound_strchr_count);
            fprintf (stderr, "bound_strdup_count       %llu\n", bound_strdup_count);
            fprintf (stderr, "bound_not_found          %llu\n", bound_not_found);
#endif
#if BOUND_STATISTIC_SPLAY
            fprintf (stderr, "bound_splay              %llu\n", bound_splay);
            fprintf (stderr, "bound_splay_end          %llu\n", bound_splay_end);
            fprintf (stderr, "bound_splay_insert       %llu\n", bound_splay_insert);
            fprintf (stderr, "bound_splay_delete       %llu\n", bound_splay_delete);
#endif
        }
    }
}

#if HAVE_PTHREAD_CREATE
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg)
{
    use_sem = 1;
    dprintf (stderr, "%s, %s()n", __FILE__, __FUNCTION__);
    return pthread_create_redir(thread, attr, start_routine, arg);
}
#endif

#if MALLOC_REDIR
void *malloc(size_t size)
#else
void *__bound_malloc(size_t size, const void *caller)
#endif
{
    void *ptr;
    
#if MALLOC_REDIR
    /* This will catch the first dlsym call from __bound_init */
    if (malloc_redir == NULL) {
        __bound_init (0);
        if (malloc_redir == NULL) {
            ptr = &initial_pool[pool_index];
            pool_index = (pool_index + size + 15) & ~15;
            if (pool_index >= sizeof (initial_pool))
                bound_alloc_error ("initial memory pool too small");
            dprintf (stderr, "%s, %s(): initial %p, 0x%lx\n",
                     __FILE__, __FUNCTION__, ptr, (unsigned long)size);
            return ptr;
        }
    }
#endif
    /* we allocate one more byte to ensure the regions will be
       separated by at least one byte. With the glibc malloc, it may
       be in fact not necessary */
#if MALLOC_REDIR
    ptr = malloc_redir (size + 1);
#else
    ptr = malloc(size + 1);
#endif
    dprintf(stderr, "%s, %s(): %p, 0x%lx\n",
            __FILE__, __FUNCTION__, ptr, (unsigned long)size);
    
    if (no_checking == 0) {
        WAIT_SEM ();
        INCR_COUNT(bound_malloc_count);

        if (ptr) {
            tree = splay_insert ((size_t) ptr, size, tree);
            if (tree && tree->start == (size_t) ptr)
                tree->type = TCC_TYPE_MALLOC;
        }
        POST_SEM ();
    }
    return ptr;
}

#if MALLOC_REDIR
void *memalign(size_t size, size_t align)
#else
void *__bound_memalign(size_t size, size_t align, const void *caller)
#endif
{
    void *ptr;

#if HAVE_MEMALIGN
    /* we allocate one more byte to ensure the regions will be
       separated by at least one byte. With the glibc malloc, it may
       be in fact not necessary */
#if MALLOC_REDIR
    ptr = memalign_redir(size + 1, align);
#else
    ptr = memalign(size + 1, align);
#endif
#else
    if (align > 4) {
        /* XXX: handle it ? */
        ptr = NULL;
    } else {
        /* we suppose that malloc aligns to at least four bytes */
#if MALLOC_REDIR
        ptr = malloc_redir(size + 1);
#else
        ptr = malloc(size + 1);
#endif
    }
#endif
    dprintf(stderr, "%s, %s(): %p, 0x%lx\n",
            __FILE__, __FUNCTION__, ptr, (unsigned long)size);

    if (no_checking == 0) {
        WAIT_SEM ();
        INCR_COUNT(bound_memalign_count);

        if (ptr) {
            tree = splay_insert((size_t) ptr, size, tree);
            if (tree && tree->start == (size_t) ptr)
                tree->type = TCC_TYPE_MEMALIGN;
        }
        POST_SEM ();
    }
    return ptr;
}

#if MALLOC_REDIR
void free(void *ptr)
#else
void __bound_free(void *ptr, const void *caller)
#endif
{
    size_t addr = (size_t) ptr;
    void *p;

    if (ptr == NULL || tree == NULL || no_checking
#if MALLOC_REDIR
        || ((unsigned char *) ptr >= &initial_pool[0] &&
            (unsigned char *) ptr < &initial_pool[sizeof(initial_pool)])
#endif
        )
        return;

    dprintf(stderr, "%s, %s(): %p\n", __FILE__, __FUNCTION__, ptr);

    WAIT_SEM ();
    INCR_COUNT(bound_free_count);
    tree = splay (addr, tree);
    if (tree->start == addr) {
        if (tree->is_invalid) {
            POST_SEM ();
            bound_error("freeing invalid region");
            return;
        }
        tree->is_invalid = 1;
        memset (ptr, 0x5a, tree->size);
        p = free_reuse_list[free_reuse_index];
        free_reuse_list[free_reuse_index] = ptr;
        free_reuse_index = (free_reuse_index + 1) % FREE_REUSE_SIZE;
        if (p)
            tree = splay_delete((size_t)p, tree);
        ptr = p;
    }
    POST_SEM ();
#if MALLOC_REDIR
    free_redir (ptr);
#else
    free(ptr);
#endif
}

#if MALLOC_REDIR
void *realloc(void *ptr, size_t size)
#else
void *__bound_realloc(void *ptr, size_t size, const void *caller)
#endif
{
    void *new_ptr;

    if (size == 0) {
#if MALLOC_REDIR
        free(ptr);
#else
        __bound_free(ptr, caller);
#endif
        return NULL;
    }

#if MALLOC_REDIR
    new_ptr = realloc_redir (ptr, size);
#else
    new_ptr = realloc (ptr, size);
#endif
    dprintf(stderr, "%s, %s(): %p, 0x%lx\n",
            __FILE__, __FUNCTION__, new_ptr, (unsigned long)size);

    if (no_checking == 0) {
        WAIT_SEM ();
        INCR_COUNT(bound_realloc_count);

        if (ptr)
            tree = splay_delete ((size_t) ptr, tree);
        if (new_ptr) {
            tree = splay_insert ((size_t) new_ptr, size, tree);
            if (tree && tree->start == (size_t) new_ptr)
                tree->type = TCC_TYPE_REALLOC;
        }
        POST_SEM ();
    }
    return new_ptr;
}

#if MALLOC_REDIR
void *calloc(size_t nmemb, size_t size)
#else
void *__bound_calloc(size_t nmemb, size_t size)
#endif
{
    void *ptr;

    size *= nmemb;
#if MALLOC_REDIR
    /* This will catch the first dlsym call from __bound_init */
    if (malloc_redir == NULL) {
        __bound_init (0);
        if (malloc_redir == NULL) {
            ptr = &initial_pool[pool_index];
            pool_index = (pool_index + size + 15) & ~15;
            if (pool_index >= sizeof (initial_pool))
                bound_alloc_error ("initial memory pool too small");
            dprintf (stderr, "%s, %s(): initial %p, 0x%lx\n",
                     __FILE__, __FUNCTION__, ptr, (unsigned long)size);
            memset (ptr, 0, size);
            return ptr;
        }
    }
#endif
#if MALLOC_REDIR
    ptr = malloc_redir(size + 1);
#else
    ptr = malloc(size + 1);
#endif
    dprintf (stderr, "%s, %s(): %p, 0x%lx\n",
             __FILE__, __FUNCTION__, ptr, (unsigned long)size);

    if (ptr) {
        memset (ptr, 0, size);
        if (no_checking == 0) {
            WAIT_SEM ();
            INCR_COUNT(bound_calloc_count);
            tree = splay_insert ((size_t) ptr, size, tree);
            if (tree && tree->start == (size_t) ptr)
                tree->type = TCC_TYPE_CALLOC;
            POST_SEM ();
        }
    }
    return ptr;
}

#if !defined(_WIN32)
void *__bound_mmap (void *start, size_t size, int prot,
                    int flags, int fd, off_t offset)
{
    void *result;

    dprintf(stderr, "%s, %s(): %p, 0x%lx\n",
            __FILE__, __FUNCTION__, start, (unsigned long)size);
    result = mmap (start, size, prot, flags, fd, offset);
    if (result && no_checking == 0) {
        WAIT_SEM ();
        INCR_COUNT(bound_mmap_count);
        tree = splay_insert((size_t)result, size, tree);
        POST_SEM ();
    }
    return result;
}

int __bound_munmap (void *start, size_t size)
{
    int result;

    dprintf(stderr, "%s, %s(): %p, 0x%lx\n",
            __FILE__, __FUNCTION__, start, (unsigned long)size);
    if (start && no_checking == 0) {
        WAIT_SEM ();
        INCR_COUNT(bound_munmap_count);
        tree = splay_delete ((size_t) start, tree);
        POST_SEM ();
    }
    result = munmap (start, size);
    return result;
}
#endif

/* some useful checked functions */

/* check that (p ... p + size - 1) lies inside 'p' region, if any */
static void __bound_check(const void *p, size_t size, const char *function)
{
    if (no_checking == 0 && size != 0 &&
        __bound_ptr_add((void *)p, size) == INVALID_POINTER) {
        bound_error("invalid pointer %p, size 0x%lx in %s",
                p, (unsigned long)size, function);
    }
}

static int check_overlap (const void *p1, size_t n1,
                          const void *p2, size_t n2,
                          const char *function)
{
    const void *p1e = (const void *) ((const char *) p1 + n1);
    const void *p2e = (const void *) ((const char *) p2 + n2);

    if (no_checking == 0 && n1 != 0 && n2 !=0 &&
        ((p1 <= p2 && p1e > p2) ||     /* p1----p2====p1e----p2e */
         (p2 <= p1 && p2e > p1))) {    /* p2----p1====p2e----p1e */
        bound_error("overlapping regions %p(0x%lx), %p(0x%lx) in %s",
                p1, (unsigned long)n1, p2, (unsigned long)n2, function);
        return never_fatal < 0;
    }
    return 0;
}

void *__bound_memcpy(void *dest, const void *src, size_t n)
{
    dprintf(stderr, "%s, %s(): %p, %p, 0x%lx\n",
            __FILE__, __FUNCTION__, dest, src, (unsigned long)n);
    INCR_COUNT(bound_mempcy_count);
    __bound_check(dest, n, "memcpy dest");
    __bound_check(src, n, "memcpy src");
    if (check_overlap(dest, n, src, n, "memcpy"))
        return dest;
    return memcpy(dest, src, n);
}

int __bound_memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *u1 = (const unsigned char *) s1;
    const unsigned char *u2 = (const unsigned char *) s2;
    int retval = 0;

    dprintf(stderr, "%s, %s(): %p, %p, 0x%lx\n",
            __FILE__, __FUNCTION__, s1, s2, (unsigned long)n);
    INCR_COUNT(bound_memcmp_count);
    for (;;) {
        if ((ssize_t) --n == -1)
            break;
        else if (*u1 != *u2) {
            retval = *u1++ - *u2++;
            break;
        }
        ++u1;
        ++u2;
    }
    __bound_check(s1, (const void *)u1 - s1, "memcmp s1");
    __bound_check(s2, (const void *)u2 - s2, "memcmp s2");
    return retval;
}

void *__bound_memmove(void *dest, const void *src, size_t n)
{
    dprintf(stderr, "%s, %s(): %p, %p, 0x%lx\n",
            __FILE__, __FUNCTION__, dest, src, (unsigned long)n);
    INCR_COUNT(bound_memmove_count);
    __bound_check(dest, n, "memmove dest");
    __bound_check(src, n, "memmove src");
    return memmove(dest, src, n);
}

void *__bound_memset(void *s, int c, size_t n)
{
    dprintf(stderr, "%s, %s(): %p, %d, 0x%lx\n",
            __FILE__, __FUNCTION__, s, c, (unsigned long)n);
    INCR_COUNT(bound_memset_count);
    __bound_check(s, n, "memset");
    return memset(s, c, n);
}

int __bound_strlen(const char *s)
{
    const char *p = s;

    dprintf(stderr, "%s, %s(): %p\n",
            __FILE__, __FUNCTION__, s);
    INCR_COUNT(bound_strlen_count);
    while (*p++);
    __bound_check(s, p - s, "strlen");
    return (p - s) - 1;
}

char *__bound_strcpy(char *dest, const char *src)
{
    size_t len;
    const char *p = src;

    dprintf(stderr, "%s, %s(): %p, %p\n",
            __FILE__, __FUNCTION__, dest, src);
    INCR_COUNT(bound_strcpy_count);
    while (*p++);
    len = p - src;
    __bound_check(dest, len, "strcpy dest");
    __bound_check(src, len, "strcpy src");
    if (check_overlap(dest, len, src, len, "strcpy"))
        return dest;
    return strcpy (dest, src);
}

char *__bound_strncpy(char *dest, const char *src, size_t n)
{
    size_t len = n;
    const char *p = src;

    dprintf(stderr, "%s, %s(): %p, %p, 0x%lx\n",
            __FILE__, __FUNCTION__, dest, src, (unsigned long)n);
    INCR_COUNT(bound_strncpy_count);
    while (len-- && *p++);
    len = p - src;
    __bound_check(dest, len, "strncpy dest");
    __bound_check(src, len, "strncpy src");
    if (check_overlap(dest, len, src, len, "strncpy"))
        return dest;
    return strncpy(dest, src, n);
}

int __bound_strcmp(const char *s1, const char *s2)
{
    const unsigned char *u1 = (const unsigned char *) s1;
    const unsigned char *u2 = (const unsigned char *) s2;

    dprintf(stderr, "%s, %s(): %p, %p\n",
            __FILE__, __FUNCTION__, s1, s2);
    INCR_COUNT(bound_strcmp_count);
    while (*u1 && *u1 == *u2) {
        ++u1;
        ++u2;
    }
    __bound_check(s1, ((const char *)u1 - s1) + 1, "strcmp s1");
    __bound_check(s2, ((const char *)u2 - s2) + 1, "strcmp s2");
    return *u1 - *u2;
}

int __bound_strncmp(const char *s1, const char *s2, size_t n)
{
    const unsigned char *u1 = (const unsigned char *) s1;
    const unsigned char *u2 = (const unsigned char *) s2;
    int retval = 0;

    dprintf(stderr, "%s, %s(): %p, %p, 0x%lx\n",
            __FILE__, __FUNCTION__, s1, s2, (unsigned long)n);
    INCR_COUNT(bound_strncmp_count);
    do {
        if ((ssize_t) --n == -1)
            break;
        else if (*u1 != *u2) {
            retval = *u1++ - *u2++;
            break;
        }
        ++u2;
    } while (*u1++);
    __bound_check(s1, (const char *)u1 - s1, "strncmp s1");
    __bound_check(s2, (const char *)u2 - s2, "strncmp s2");
    return retval;
}

char *__bound_strcat(char *dest, const char *src)
{
    char *r = dest;
    const char *s = src;

    dprintf(stderr, "%s, %s(): %p, %p\n",
            __FILE__, __FUNCTION__, dest, src);
    INCR_COUNT(bound_strcat_count);
    while (*dest++);
    while (*src++);
    __bound_check(r, (dest - r) + (src - s) - 1, "strcat dest");
    __bound_check(s, src - s, "strcat src");
    if (check_overlap(r, (dest - r) + (src - s) - 1, s, src - s, "strcat"))
        return dest;
    return strcat(r, s);
}

char *__bound_strchr(const char *s, int c)
{
    const unsigned char *str = (const unsigned char *) s;
    unsigned char ch = c;

    dprintf(stderr, "%s, %s(): %p, %d\n",
            __FILE__, __FUNCTION__, s, ch);
    INCR_COUNT(bound_strchr_count);
    while (*str) {
        if (*str == ch)
            break;
        ++str;
    }
    __bound_check(s, ((const char *)str - s) + 1, "strchr");
    return *str == ch ? (char *) str : NULL;
}

char *__bound_strdup(const char *s)
{
    const char *p = s;
    char *new;

    INCR_COUNT(bound_strdup_count);
    while (*p++);
    __bound_check(s, p - s, "strdup");
#if MALLOC_REDIR
    new = malloc_redir ((p - s) + 1);
#else
    new = malloc ((p - s) + 1);
#endif
    dprintf(stderr, "%s, %s(): %p, 0x%lx\n",
            __FILE__, __FUNCTION__, new, (unsigned long)(p -s));
    if (new) {
        if (no_checking == 0 && no_strdup == 0) {
            WAIT_SEM ();
            tree = splay_insert((size_t)new, p - s, tree);
            if (tree && tree->start == (size_t) new)
                tree->type = TCC_TYPE_STRDUP;
            POST_SEM ();
        }
        memcpy (new, s, p - s);
    }
    return new;
}

/*
           An implementation of top-down splaying with sizes
             D. Sleator <sleator@cs.cmu.edu>, January 1994.

  This extends top-down-splay.c to maintain a size field in each node.
  This is the number of nodes in the subtree rooted there.  This makes
  it possible to efficiently compute the rank of a key.  (The rank is
  the number of nodes to the left of the given key.)  It it also
  possible to quickly find the node of a given rank.  Both of these
  operations are illustrated in the code below.  The remainder of this
  introduction is taken from top-down-splay.c.

  "Splay trees", or "self-adjusting search trees" are a simple and
  efficient data structure for storing an ordered set.  The data
  structure consists of a binary tree, with no additional fields.  It
  allows searching, insertion, deletion, deletemin, deletemax,
  splitting, joining, and many other operations, all with amortized
  logarithmic performance.  Since the trees adapt to the sequence of
  requests, their performance on real access patterns is typically even
  better.  Splay trees are described in a number of texts and papers
  [1,2,3,4].

  The code here is adapted from simple top-down splay, at the bottom of
  page 669 of [2].  It can be obtained via anonymous ftp from
  spade.pc.cs.cmu.edu in directory /usr/sleator/public.

  The chief modification here is that the splay operation works even if the
  item being splayed is not in the tree, and even if the tree root of the
  tree is NULL.  So the line:

                              t = splay(i, t);

  causes it to search for item with key i in the tree rooted at t.  If it's
  there, it is splayed to the root.  If it isn't there, then the node put
  at the root is the last one before NULL that would have been reached in a
  normal binary search for i.  (It's a neighbor of i in the tree.)  This
  allows many other operations to be easily implemented, as shown below.

  [1] "Data Structures and Their Algorithms", Lewis and Denenberg,
       Harper Collins, 1991, pp 243-251.
  [2] "Self-adjusting Binary Search Trees" Sleator and Tarjan,
       JACM Volume 32, No 3, July 1985, pp 652-686.
  [3] "Data Structure and Algorithm Analysis", Mark Weiss,
       Benjamin Cummins, 1992, pp 119-130.
  [4] "Data Structures, Algorithms, and Performance", Derick Wood,
       Addison-Wesley, 1993, pp 367-375
*/

/* Code adapted for tcc */

#define compare(start,tstart,tsize) (start < tstart ? -1 : \
                                     start >= tstart+tsize  ? 1 : 0)

static Tree * splay (size_t addr, Tree *t)
/* Splay using the key start (which may or may not be in the tree.) */
/* The starting root is t, and the tree used is defined by rat      */
{
    Tree N, *l, *r, *y;
    int comp;
    
    INCR_COUNT_SPLAY(bound_splay);
    if (t == NULL) return t;
    N.left = N.right = NULL;
    l = r = &N;
 
    for (;;) {
        comp = compare(addr, t->start, t->size);
        if (comp < 0) {
            y = t->left;
            if (y == NULL) break;
            if (compare(addr, y->start, y->size) < 0) {
                t->left = y->right;                    /* rotate right */
                y->right = t;
                t = y;
                if (t->left == NULL) break;
            }
            r->left = t;                               /* link right */
            r = t;
            t = t->left;
        } else if (comp > 0) {
            y = t->right;
            if (y == NULL) break;
            if (compare(addr, y->start, y->size) > 0) {
                t->right = y->left;                    /* rotate left */
                y->left = t;
                t = y;
                if (t->right == NULL) break;
            }
            l->right = t;                              /* link left */
            l = t;
            t = t->right;
        } else {
            break;
        }
    }
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;

    return t;
}

#define compare_end(start,tend) (start < tend ? -1 : \
                                 start > tend  ? 1 : 0)

static Tree * splay_end (size_t addr, Tree *t)
/* Splay using the key start (which may or may not be in the tree.) */
/* The starting root is t, and the tree used is defined by rat  */
{
    Tree N, *l, *r, *y;
    int comp;
    
    INCR_COUNT_SPLAY(bound_splay_end);
    if (t == NULL) return t;
    N.left = N.right = NULL;
    l = r = &N;
 
    for (;;) {
        comp = compare_end(addr, t->start + t->size);
        if (comp < 0) {
            y = t->left;
            if (y == NULL) break;
            if (compare_end(addr, y->start + y->size) < 0) {
                t->left = y->right;                    /* rotate right */
                y->right = t;
                t = y;
                if (t->left == NULL) break;
            }
            r->left = t;                               /* link right */
            r = t;
            t = t->left;
        } else if (comp > 0) {
            y = t->right;
            if (y == NULL) break;
            if (compare_end(addr, y->start + y->size) > 0) {
                t->right = y->left;                    /* rotate left */
                y->left = t;
                t = y;
                if (t->right == NULL) break;
            }
            l->right = t;                              /* link left */
            l = t;
            t = t->right;
        } else {
            break;
        }
    }
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;

    return t;
}

static Tree * splay_insert(size_t addr, size_t size, Tree * t)
/* Insert key start into the tree t, if it is not already there. */
/* Return a pointer to the resulting tree.                       */
{
    Tree * new;

    INCR_COUNT_SPLAY(bound_splay_insert);
    if (t != NULL) {
        t = splay(addr,t);
        if (compare(addr, t->start, t->size)==0) {
            return t;  /* it's already there */
        }
    }
#if TREE_REUSE
    if (tree_free_list) {
          new = tree_free_list;
          tree_free_list = new->left;
    }
    else
#endif
    {
#if MALLOC_REDIR
        new = (Tree *) malloc_redir (sizeof (Tree));
#else
        new = (Tree *) malloc (sizeof (Tree));
#endif
    }
    if (new == NULL) {
        bound_alloc_error("not enough memory for bound checking code");
    }
    else {
        if (t == NULL) {
            new->left = new->right = NULL;
        } else if (compare(addr, t->start, t->size) < 0) {
            new->left = t->left;
            new->right = t;
            t->left = NULL;
        } else {
            new->right = t->right;
            new->left = t;
            t->right = NULL;
        }
        new->start = addr;
        new->size = size;
        new->type = TCC_TYPE_NONE;
        new->is_invalid = 0;
    }
    return new;
}

#define compare_destroy(start,tstart) (start < tstart ? -1 : \
                                       start > tstart  ? 1 : 0)

static Tree * splay_delete(size_t addr, Tree *t)
/* Deletes addr from the tree if it's there.               */
/* Return a pointer to the resulting tree.                 */
{
    Tree * x;

    INCR_COUNT_SPLAY(bound_splay_delete);
    if (t==NULL) return NULL;
    t = splay(addr,t);
    if (compare_destroy(addr, t->start) == 0) {        /* found it */
        if (t->left == NULL) {
            x = t->right;
        } else {
            x = splay(addr, t->left);
            x->right = t->right;
        }
#if TREE_REUSE
        t->left = tree_free_list;
        tree_free_list = t;
#else
#if MALLOC_REDIR
        free_redir(t);
#else
        free(t);
#endif
#endif
        return x;
    } else {
        return t;                                      /* It wasn't there */
    }
}

void splay_printtree(Tree * t, int d)
{
    int i;
    if (t == NULL) return;
    splay_printtree(t->right, d+1);
    for (i=0; i<d; i++) fprintf(stderr," ");
    fprintf(stderr,"%p(0x%lx:%u:%u)\n",
            (void *) t->start, (unsigned long) t->size,
            (unsigned)t->type, (unsigned)t->is_invalid);
    splay_printtree(t->left, d+1);
}
