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

#define BOUND_DEBUG
#define BOUND_STATISTIC

#ifdef BOUND_DEBUG
 #define dprintf(a...) if (print_calls) fprintf(a)
#else
 #define dprintf(a...)
#endif

/* Check memalign */
#define HAVE_MEMALIGN

#if defined(__FreeBSD__) \
 || defined(__FreeBSD_kernel__) \
 || defined(__DragonFly__) \
 || defined(__OpenBSD__) \
 || defined(__NetBSD__) \
 || defined(__dietlibc__)
#undef HAVE_MEMALIGN
#define INIT_SEM()
#define EXIT_SEM()
#define WAIT_SEM()
#define POST_SEM()
#define HAS_ENVIRON 0
#define MALLOC_REDIR    (0)
#elif defined(_WIN32)
#include <windows.h>
#undef HAVE_MEMALIGN
static CRITICAL_SECTION bounds_sem;
#define INIT_SEM()  InitializeCriticalSection(&bounds_sem)
#define EXIT_SEM()  DeleteCriticalSection(&bounds_sem)
#define WAIT_SEM()  EnterCriticalSection(&bounds_sem)
#define POST_SEM()  LeaveCriticalSection(&bounds_sem)
#define HAS_ENVIRON 0
#define MALLOC_REDIR    (0)
#else
#include <sys/mman.h>
#include <errno.h>
#include <semaphore.h>
static sem_t bounds_sem;
#define INIT_SEM()  sem_init (&bounds_sem, 0, 1)
#define EXIT_SEM()  sem_destroy (&bounds_sem)
#define WAIT_SEM()  while (sem_wait (&bounds_sem) < 0 && errno == EINTR);
#define POST_SEM()  sem_post (&bounds_sem)
#define HAS_ENVIRON 0 /* Disabled for now */
#define __USE_GNU       /* get RTLD_NEXT */
#include <dlfcn.h>
#define MALLOC_REDIR    (1)
static void *(*malloc_redir) (size_t) = NULL;
static void *(*calloc_redir) (size_t, size_t) = NULL;
static void (*free_redir) (void *) = NULL;
static void *(*realloc_redir) (void *, size_t) = NULL;
static void *(*memalign_redir) (size_t, size_t) = NULL;
static int pool_index = 0;
static unsigned char initial_pool[256];
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
    size_t type;
    size_t is_invalid; /* true if pointers outside region are invalid */
};

typedef struct alloca_list_struct {
    size_t fp;
    void *p;
    struct alloca_list_struct *next;
} alloca_list_type;

static Tree * splay (size_t addr, Tree *t);
static Tree * splay_end (size_t addr, Tree *t);
static Tree * splay_insert(size_t addr, size_t size, Tree * t);
static Tree * splay_delete(size_t addr, Tree *t);
void splay_printtree(Tree * t, int d);

/* external interface */
void __bound_init(void);
void __bound_exit(void);

#ifdef __attribute__
  /* an __attribute__ macro is defined in the system headers */
  #undef __attribute__ 
#endif
#define FASTCALL __attribute__((regparm(3)))

#if !MALLOC_REDIR
void *__bound_malloc(size_t size, const void *caller);
void *__bound_memalign(size_t size, size_t align, const void *caller);
void __bound_free(void *ptr, const void *caller);
void *__bound_realloc(void *ptr, size_t size, const void *caller);
void *__bound_calloc(size_t nmemb, size_t size);
#endif

#define FREE_REUSE_SIZE (100)
static int free_reuse_index = 0;
static void *free_reuse_list[FREE_REUSE_SIZE];

/* error message, just for TCC */
const char *__bound_error_msg;

/* runtime error output */
extern void rt_error(size_t pc, const char *fmt, ...);

static Tree *tree = NULL;
#define TREE_REUSE      (1)
#if TREE_REUSE
static Tree *tree_free_list = NULL;
#endif
static alloca_list_type *alloca_list = NULL;

static int inited = 0;
static int print_calls = 0;
static int print_heap = 0;
static int print_statistic = 0;
static int never_fatal = 0;
static int no_checking = 0;

#ifdef BOUND_STATISTIC
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
#define INCR_COUNT(x)    x++
#else
#define INCR_COUNT(x)
#endif

/* enable/disable checking. This can be used for signal handlers. */
void __bound_checking (int no_check)
{
    no_checking = no_check;
}

#define no_FASTCALL
//#define no_checking 1

/* print a bound error message */
static void bound_error(const char *fmt, ...)
{
    __bound_error_msg = fmt;
    fprintf(stderr,"%s %s: %s\n", __FILE__, __FUNCTION__, fmt);
    if (never_fatal == 0)
        *(void **)0 = 0; /* force a runtime error */
}

static void bound_alloc_error(void)
{
    bound_error("not enough memory for bound checking code");
}

/* return '(p + offset)' for pointer arithmetic (a pointer can reach
   the end of a region in this case */
void * no_FASTCALL __bound_ptr_add(void *p, size_t offset)
{
    size_t addr = (size_t)p;

    if (no_checking) {
        return p + offset;
    }

    dprintf(stderr, "%s %s : %p 0x%x\n",
            __FILE__, __FUNCTION__, p, (unsigned)offset);

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
            addr += offset;
            if (tree->is_invalid || addr > tree->size) {
        #if 0
                fprintf(stderr,"%s %s : %p is outside of the region\n",
                        __FILE__, __FUNCTION__, p + offset);
        #endif
                if (never_fatal == 0) {
                    POST_SEM ();
                    return INVALID_POINTER; /* return an invalid pointer */
                }
            }
        }
    }
    POST_SEM ();
    return p + offset;
}

/* return '(p + offset)' for pointer indirection (the resulting must
   be strictly inside the region */
#define BOUND_PTR_INDIR(dsize)                                                 \
void * no_FASTCALL __bound_ptr_indir ## dsize (void *p, size_t offset)         \
{                                                                              \
    size_t addr = (size_t)p;                                                   \
                                                                               \
    if (no_checking) {                                                         \
        return p + offset;                                                     \
    }                                                                          \
    dprintf(stderr, "%s %s : %p 0x%x start\n",                                 \
            __FILE__, __FUNCTION__, p, (unsigned)offset);                      \
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
            addr += offset + dsize;                                            \
            if (tree->is_invalid || addr > tree->size) {                       \
                fprintf(stderr,"%s %s : %p is outside of the region\n",        \
                    __FILE__, __FUNCTION__, p + offset);                       \
                if (never_fatal == 0) {                                        \
                    POST_SEM ();                                               \
                    return INVALID_POINTER; /* return an invalid pointer */    \
                }                                                              \
            }                                                                  \
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
    size_t addr, size, fp, *p = p1;

    if (no_checking)
         return;
    GET_CALLER_FP(fp);
    dprintf(stderr, "%s, %s local new p1=%p fp=%p\n",
           __FILE__, __FUNCTION__, p, (void *)fp);
    WAIT_SEM ();
    for(;;) {
        addr = p[0];
        if (addr == 0)
            break;
        INCR_COUNT(bound_local_new_count);
        if (addr == 1) {
            dprintf(stderr, "%s, %s() alloca/vla used\n",
                    __FILE__, __FUNCTION__);
        }
        else {
            addr += fp;
            size = p[1];
            dprintf(stderr, "%s, %s() (%p 0x%lx)\n",
                    __FILE__, __FUNCTION__, (void *) addr, (unsigned long) size);
            tree = splay_insert(addr, size, tree);
        }
        p += 2;
    }
    POST_SEM ();
}

/* called when leaving a function to delete all the local regions */
void FASTCALL __bound_local_delete(void *p1) 
{
    size_t addr, fp, *p = p1;

    if (no_checking)
         return;
    GET_CALLER_FP(fp);
    dprintf(stderr, "%s, %s local delete p1=%p fp=%p\n",
            __FILE__, __FUNCTION__, p, (void *)fp);
    WAIT_SEM ();
    for(;;) {
        addr = p[0];
        if (addr == 0)
            break;
        INCR_COUNT(bound_local_delete_count);
        if (addr == 1) {
            while (alloca_list && alloca_list->fp == fp) {
                alloca_list_type *next = alloca_list->next;

                dprintf(stderr, "%s, %s() remove alloca/vla %p\n",
                        __FILE__, __FUNCTION__, alloca_list->p);
                tree = splay_delete ((size_t) alloca_list->p, tree);
#if MALLOC_REDIR
                free_redir (alloca_list);
#else
                free (alloca_list);
#endif
                alloca_list = next;
            }
        }
        else {
            addr += fp;
            dprintf(stderr, "%s, %s() (%p 0x%lx)\n",
                    __FILE__, __FUNCTION__, (void *) addr, (unsigned long) p[1]);
            tree = splay_delete(addr, tree);
        }
        p += 2;
    }
    POST_SEM ();
}

void __bound_init(void)
{
    if (inited)
        return;

    inited = 1;

    print_calls = getenv ("TCC_BOUNDS_PRINT_CALLS") != NULL;
    print_heap = getenv ("TCC_BOUNDS_PRINT_HEAP") != NULL;
    print_statistic = getenv ("TCC_BOUNDS_PRINT_STATISTIC") != NULL;
    never_fatal = getenv ("TCC_BOUNDS_NEVER_FATAL") != NULL;

    dprintf(stderr, "%s, %s() start\n", __FILE__, __FUNCTION__);

    INIT_SEM ();

#if MALLOC_REDIR
    {
        void *addr = RTLD_NEXT;

        /* tcc -run required RTLD_DEFAULT. Normal usage requires RTLD_NEXT */
        *(void **) (&malloc_redir) = dlsym (addr, "malloc");
        if (malloc_redir == NULL) {
            dprintf(stderr, "%s, %s() use RTLD_DEFAULT\n",
                    __FILE__, __FUNCTION__);
            addr = RTLD_DEFAULT;
            *(void **) (&malloc_redir) = dlsym (addr, "malloc");
        }
        *(void **) (&calloc_redir) = dlsym (addr, "calloc");
        *(void **) (&free_redir) = dlsym (addr, "free");
        *(void **) (&realloc_redir) = dlsym (addr, "realloc");
        *(void **) (&memalign_redir) = dlsym (addr, "memalign");
        dprintf(stderr, "%s, %s() malloc_redir %p\n",
                __FILE__, __FUNCTION__, malloc_redir);
        dprintf(stderr, "%s, %s() free_redir %p\n",
                __FILE__, __FUNCTION__, free_redir);
        dprintf(stderr, "%s, %s() realloc_redir %p\n",
                __FILE__, __FUNCTION__, realloc_redir);
        dprintf(stderr, "%s, %s() memalign_redir %p\n",
                __FILE__, __FUNCTION__, memalign_redir);
    }
#endif

    tree = NULL;

    /* save malloc hooks and install bound check hooks */
    memset (free_reuse_list, 0, sizeof (free_reuse_list));

    dprintf(stderr, "%s, %s() end\n\n", __FILE__, __FUNCTION__);
}

void __bounds_add_static_var (size_t *p)
{
    dprintf(stderr, "%s, %s()\n", __FILE__, __FUNCTION__);
    /* add all static bound check values */
    WAIT_SEM ();
    while (p[0] != 0) {
        dprintf(stderr, "%s, %s() (%p 0x%lx)\n",
                __FILE__, __FUNCTION__, (void *) p[0], (unsigned long) p[1]);
        tree = splay_insert(p[0], p[1], tree);
        p += 2;
    }
    POST_SEM ();
}

void __bound_main_arg(char **p)
{
    char *start = (char *) p;

    WAIT_SEM ();
    while (*p) {
        dprintf(stderr, "%s, %s() (%p 0x%lx)\n",
                __FILE__, __FUNCTION__, *p, (unsigned long)(strlen (*p) + 1));
        tree = splay_insert((size_t) *p, strlen (*p) + 1, tree);
        p++;
    }
    dprintf(stderr, "%s, %s() argv (%p 0x%lx)\n",
            __FILE__, __FUNCTION__, start, (unsigned long)((char *) p - start));
    tree = splay_insert((size_t) start, (char *) p - start, tree);

#if HAS_ENVIRON
    {
        extern char **environ;

        p = environ;
        start = (char *) p;
        while (*p) {
            dprintf(stderr, "%s, %s() (%p 0x%lx)\n",
                    __FILE__, __FUNCTION__, *p, (unsigned long)(strlen (*p) + 1));
            tree = splay_insert((size_t) *p, strlen (*p) + 1, tree);
            p++;
        }
        dprintf(stderr, "%s, %s() environ(%p 0x%lx)\n",
                __FILE__, __FUNCTION__, start, (unsigned long)((char *) p - start));
        tree = splay_insert((size_t) start, (char *) p - start, tree);
    }
#endif
    POST_SEM ();
}

void __attribute__((destructor)) __bound_exit(void)
{
    int i;
    static const char * const alloc_type[] = {
        "", "malloc", "calloc", "realloc", "memalign", "strdup"
    };

    dprintf(stderr, "%s, %s()\n", __FILE__, __FUNCTION__);

    if (inited) {
#if !defined(_WIN32)
        if (print_heap) {
            extern void __libc_freeres ();
            __libc_freeres ();
        }
#endif
    
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
            if (print_heap && tree->type != 0) {
                fprintf (stderr, "%s, %s() %s found size %lu\n",
                         __FILE__, __FUNCTION__, alloc_type[tree->type],
                         (unsigned long) tree->size);
            }
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
        EXIT_SEM ();
        inited = 0;
#ifdef BOUND_STATISTIC
        if (print_statistic) {
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
        }
#endif
    }
}

/* XXX: we should use a malloc which ensure that it is unlikely that
   two malloc'ed data have the same address if 'free' are made in
   between. */
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
        ptr = &initial_pool[pool_index];
        pool_index = (pool_index + size + 7) & ~8;
        dprintf (stderr, "%s, %s initial (%p, 0x%x)\n",
                 __FILE__, __FUNCTION__, ptr, (unsigned)size);
        return ptr;
    }
#endif
    /* we allocate one more byte to ensure the regions will be
       separated by at least one byte. With the glibc malloc, it may
       be in fact not necessary */
    WAIT_SEM ();
    INCR_COUNT(bound_malloc_count);
#if MALLOC_REDIR
    ptr = malloc_redir (size);
#else
    ptr = malloc(size + 1);
#endif
    
    dprintf(stderr, "%s, %s (%p, 0x%x)\n",
            __FILE__, __FUNCTION__, ptr, (unsigned)size);

    if (ptr) {
        tree = splay_insert ((size_t) ptr, size, tree);
        if (tree && tree->start == (size_t) ptr) {
            tree->type = TCC_TYPE_MALLOC;
        }
    }
    POST_SEM ();
    return ptr;
}

#if MALLOC_REDIR
void *memalign(size_t size, size_t align)
#else
void *__bound_memalign(size_t size, size_t align, const void *caller)
#endif
{
    void *ptr;

    WAIT_SEM ();
    INCR_COUNT(bound_memalign_count);

#ifndef HAVE_MEMALIGN
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
#else
    /* we allocate one more byte to ensure the regions will be
       separated by at least one byte. With the glibc malloc, it may
       be in fact not necessary */
#if MALLOC_REDIR
    ptr = memalign_redir(size + 1, align);
#else
    ptr = memalign(size + 1, align);
#endif
#endif
    if (ptr) {
        dprintf(stderr, "%s, %s (%p, 0x%x)\n",
                __FILE__, __FUNCTION__, ptr, (unsigned)size);
        tree = splay_insert((size_t) ptr, size, tree);
        if (tree && tree->start == (size_t) ptr) {
            tree->type = TCC_TYPE_MEMALIGN;
        }
    }
    POST_SEM ();
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

    if (ptr == NULL || tree == NULL
#if MALLOC_REDIR
        || ((unsigned char *) ptr >= &initial_pool[0] &&
            (unsigned char *) ptr < &initial_pool[sizeof(initial_pool)])
#endif
        )
        return;

    dprintf(stderr, "%s, %s (%p)\n", __FILE__, __FUNCTION__, ptr);

    WAIT_SEM ();
    INCR_COUNT(bound_free_count);
    tree = splay (addr, tree);
    if (tree->start == addr) {
        if (tree->is_invalid) {
            bound_error("freeing invalid region");
            POST_SEM ();
            return;
        }
        tree->is_invalid = 1;
        memset (ptr, 0x5a, tree->size);
        p = free_reuse_list[free_reuse_index];
        free_reuse_list[free_reuse_index] = ptr;
        free_reuse_index = (free_reuse_index + 1) % FREE_REUSE_SIZE;
        if (p) {
            tree = splay_delete((size_t)p, tree);
        }
        ptr = p;
    }
#if MALLOC_REDIR
    free_redir (ptr);
#else
    free(ptr);
#endif
    POST_SEM ();
}

#if MALLOC_REDIR
void *realloc(void *ptr, size_t size)
#else
void *__bound_realloc(void *ptr, size_t size, const void *caller)
#endif
{
    if (size == 0) {
#if MALLOC_REDIR
        free(ptr);
#else
        __bound_free(ptr, caller);
#endif
        return NULL;
    }
    else if (ptr == NULL) {
#if MALLOC_REDIR
        ptr = realloc_redir (ptr, size);
#else
        ptr = realloc (ptr, size);
#endif
        WAIT_SEM ();
        INCR_COUNT(bound_realloc_count);
        if (ptr) {
            tree = splay_insert ((size_t) ptr, size, tree);
            if (tree && tree->start == (size_t) ptr) {
                tree->type = TCC_TYPE_REALLOC;
            }
        }
        POST_SEM ();
        return ptr;
    }
    else {
        WAIT_SEM ();
        tree = splay_delete ((size_t) ptr, tree);
#if MALLOC_REDIR
        ptr = realloc_redir (ptr, size);
#else
        ptr = realloc (ptr, size);
#endif
        if (ptr) {
            tree = splay_insert ((size_t) ptr, size, tree);
            if (tree && tree->start == (size_t) ptr) {
                tree->type = TCC_TYPE_REALLOC;
            }
        }
        POST_SEM ();
        return ptr;
    }
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
        ptr = &initial_pool[pool_index];
        pool_index = (pool_index + size + 7) & ~8;
        dprintf (stderr, "%s, %s initial (%p, 0x%x)\n",
                 __FILE__, __FUNCTION__, ptr, (unsigned)size);
        memset (ptr, 0, size);
        return ptr;
    }
#endif
#if MALLOC_REDIR
    ptr = malloc_redir(size + 1);
#else
    ptr = malloc(size + 1);
#endif
    if (ptr) {
        memset (ptr, 0, size);
        WAIT_SEM ();
        INCR_COUNT(bound_calloc_count);
        tree = splay_insert ((size_t) ptr, size, tree);
        if (tree && tree->start == (size_t) ptr) {
            tree->type = TCC_TYPE_CALLOC;
        }
        POST_SEM ();
    }
    return ptr;
}

#if !defined(_WIN32)
void *__bound_mmap (void *start, size_t size, int prot,
                    int flags, int fd, off_t offset)
{
    void *result;

    dprintf(stderr, "%s, %s (%p, 0x%x)\n",
            __FILE__, __FUNCTION__, start, (unsigned)size);
    result = mmap (start, size, prot, flags, fd, offset);
    if (result) {
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

    dprintf(stderr, "%s, %s (%p, 0x%x)\n",
            __FILE__, __FUNCTION__, start, (unsigned)size);
    WAIT_SEM ();
    INCR_COUNT(bound_munmap_count);
    tree = splay_delete ((size_t) start, tree);
    POST_SEM ();
    result = munmap (start, size);
    return result;
}
#endif

/* used by alloca */
void __bound_new_region(void *p, size_t size)
{
    size_t fp;
    alloca_list_type *last;
    alloca_list_type *cur;

    dprintf(stderr, "%s, %s (%p, 0x%x)\n",
            __FILE__, __FUNCTION__, p, (unsigned)size);
    WAIT_SEM ();
    INCR_COUNT(bound_alloca_count);
    GET_CALLER_FP (fp);
    last = NULL;
    cur = alloca_list;
    while (cur && cur->fp == fp) {
        if (cur->p == p) {
            dprintf(stderr, "%s, %s() remove alloca/vla %p\n",
                    __FILE__, __FUNCTION__, alloca_list->p);
            if (last) {
                last->next = cur->next;
            }
            else {
                alloca_list = cur->next;
            }
            tree = splay_delete((size_t)p, tree);
#if MALLOC_REDIR
            free_redir (cur);
#else
            free (cur);
#endif
            break;
        }
        last = cur;
        cur = cur->next;
    }
    tree = splay_insert((size_t)p, size, tree);
#if MALLOC_REDIR
    cur = malloc_redir (sizeof (alloca_list_type));
#else
    cur = malloc (sizeof (alloca_list_type));
#endif
    if (cur) {
        cur->fp = fp;
        cur->p = p;
        cur->next = alloca_list;
        alloca_list = cur;
    }
    POST_SEM ();
}

#if defined(__GNUC__) && (__GNUC__ >= 6)
#pragma GCC diagnostic pop
#endif


/* some useful checked functions */

/* check that (p ... p + size - 1) lies inside 'p' region, if any */
static void __bound_check(const void *p, size_t size, const char *function)
{
    if (no_checking)
        return;
    if (size == 0)
        return;
    p = __bound_ptr_add((void *)p, size);
    if (p == INVALID_POINTER)
        bound_error("invalid pointer");
}

void *__bound_memcpy(void *dst, const void *src, size_t size)
{
    void* p;

    INCR_COUNT(bound_mempcy_count);
    __bound_check(dst, size, "memcpy");
    __bound_check(src, size, "memcpy");
    /* check also region overlap */
    if (no_checking == 0 && src >= dst && src < dst + size)
        bound_error("overlapping regions in memcpy()");

    p = memcpy(dst, src, size);

    return p;
}

int __bound_memcmp(const void *s1, const void *s2, size_t size)
{
    INCR_COUNT(bound_memcmp_count);
    __bound_check(s1, size, "memcmp");
    __bound_check(s2, size, "memcmp");
    return memcmp(s1, s2, size);
}

void *__bound_memmove(void *dst, const void *src, size_t size)
{
    INCR_COUNT(bound_memmove_count);
    __bound_check(dst, size, "memmove");
    __bound_check(src, size, "memmove");
    return memmove(dst, src, size);
}

void *__bound_memset(void *dst, int c, size_t size)
{
    INCR_COUNT(bound_memset_count);
    __bound_check(dst, size, "memset");
    return memset(dst, c, size);
}

int __bound_strlen(const char *s)
{
    const char *p = s;
    size_t len;

    INCR_COUNT(bound_strlen_count);
    while (*p++);
    len = (p - s) - 1;
    p = __bound_ptr_indir1((char *)s, len);
    if (p == INVALID_POINTER)
        bound_error("bad pointer in strlen()");
    return len;
}

char *__bound_strcpy(char *dst, const char *src)
{
    size_t len;
    void *p;

    INCR_COUNT(bound_strcpy_count);
    len = __bound_strlen(src);
    p = __bound_memcpy(dst, src, len + 1);
    return p;
}

char *__bound_strncpy(char *dst, const char *src, size_t n)
{
    size_t len = n;
    const char *p = src;

    while (len-- && *p++);
    len = p - src;
    INCR_COUNT(bound_strncpy_count);
    __bound_check(dst, len, "strncpy");
    __bound_check(src, len, "strncpy");
    return strncpy (dst, src, n);
}

int __bound_strcmp(const char *s1, const char *s2)
{
    const unsigned char *u1 = (const unsigned char *) s1;
    const unsigned char *u2 = (const unsigned char *) s2;

    INCR_COUNT(bound_strcmp_count);
    while (*u1 && *u1 == *u2) {
        u1++;
        u2++;
    }
    __bound_check(s1, ((const char *)u1 - s1) + 1, "strcmp");
    __bound_check(s2, ((const char *)u2 - s2) + 1, "strcmp");
    return (*u1 - *u2);
}

int __bound_strncmp(const char *s1, const char *s2, size_t n)
{
    const unsigned char *u1 = (const unsigned char *) s1;
    const unsigned char *u2 = (const unsigned char *) s2;
    int retval = 0;

    INCR_COUNT(bound_strncmp_count);
    do {
        if ((ssize_t) --n == -1)
            break;
        else if (*u1 != *u2) {
            retval = *u1 - *u2;
            break;
        }
        u2++;
    } while (*u1++);
    __bound_check(s1, ((const char *)u1 - s1) + 1, "strncmp");
    __bound_check(s2, ((const char *)u1 - s1) + 1, "strncmp");
    return retval;
}

char *__bound_strcat(char *dest, const char *src)
{
    char *r = dest;
    const char *s = src;

    INCR_COUNT(bound_strcat_count);
    while (*dest++);
    dest--;
    while ((*dest++ = *src++) != 0);
    __bound_check(r, dest - r, "strcat");
    __bound_check(s, src - s, "strcat");
    return r;
}

char *__bound_strchr(const char *string, int ch)
{
    const unsigned char *s = (const unsigned char *) string;
    unsigned char c = ch;

    INCR_COUNT(bound_strchr_count);
    while (*s) {
        if (*s == c) {
            break;
        }
        s++;
    }
    __bound_check(string, ((const char *)s - string) + 1, "strchr");
    return *s == c ? (char *) s : NULL;
}

char *__bound_strdup(const char *s)
{
    const char *p = s;
    char *new;

    INCR_COUNT(bound_strdup_count);
    while (*p++);
    __bound_check(s, p - s, "strdup");
#if MALLOC_REDIR
    new = malloc_redir (p - s);
#else
    new = malloc (p - s);
#endif
    if (new) {
        WAIT_SEM ();
        tree = splay_insert((size_t)new, p - s, tree);
        if (tree && tree->start == (size_t) new) {
            tree->type = TCC_TYPE_STRDUP;
        }
        memcpy (new, s, p - s);
        POST_SEM ();
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

/* This is the comparison.                                       */
/* Returns <0 if i<j, =0 if i=j, and >0 if i>j                   */
 
static Tree * splay (size_t addr, Tree *t)
/* Splay using the key start (which may or may not be in the tree.) */
/* The starting root is t, and the tree used is defined by rat  */
{
    Tree N, *l, *r, *y;
    int comp;
    
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
/* Return a pointer to the resulting tree.                   */
{
    Tree * new;

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
      bound_alloc_error();
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
/* Return a pointer to the resulting tree.              */
{
    Tree * x;

    if (t==NULL) return NULL;
    t = splay(addr,t);
    if (compare_destroy(addr, t->start) == 0) {               /* found it */
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
        return t;                         /* It wasn't there */
    }
}

void splay_printtree(Tree * t, int d)
{
    int i;
    if (t == NULL) return;
    splay_printtree(t->right, d+1);
    for (i=0; i<d; i++) fprintf(stderr," ");
    fprintf(stderr,"%p(0x%lx:%u)\n",
            (void *) t->start, (unsigned long) t->size, (unsigned)t->is_invalid);
    splay_printtree(t->left, d+1);
}
