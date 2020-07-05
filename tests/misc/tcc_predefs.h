#if defined __x86_64__
#ifndef _WIN64
    /* GCC compatible definition of va_list. */
    /* This should be in sync with the declaration in our lib/libtcc1.c */
    typedef struct {
        unsigned gp_offset, fp_offset;
        union {
            unsigned overflow_offset;
            char *overflow_arg_area;
        };
        char *reg_save_area;
    } __builtin_va_list[1];

    void *__va_arg(__builtin_va_list ap, int arg_type, int size, int align);
    #define __builtin_va_start(ap, last) \
       (*(ap) = *(__builtin_va_list)((char*)__builtin_frame_address(0) - 24))
    #define __builtin_va_arg(ap, t)   \
       (*(t *)(__va_arg(ap, __builtin_va_arg_types(t), sizeof(t), __alignof__(t))))
    #define __builtin_va_copy(dest, src) (*(dest) = *(src))

#else /* _WIN64 */
    typedef char *__builtin_va_list;
    #define __builtin_va_arg(ap, t) ((sizeof(t) > 8 || (sizeof(t) & (sizeof(t) - 1))) \
        ? **(t **)((ap += 8) - 8) : *(t  *)((ap += 8) - 8))
#endif

#elif defined __arm__
    typedef char *__builtin_va_list;
    #define _tcc_alignof(type) ((int)&((struct {char c;type x;} *)0)->x)
    #define _tcc_align(addr,type) (((unsigned)addr + _tcc_alignof(type) - 1) \
                                  & ~(_tcc_alignof(type) - 1))
    #define __builtin_va_start(ap,last) (ap = ((char *)&(last)) + ((sizeof(last)+3)&~3))
    #define __builtin_va_arg(ap,type) (ap = (void *) ((_tcc_align(ap,type)+sizeof(type)+3) \
                           &~3), *(type *)(ap - ((sizeof(type)+3)&~3)))

#elif defined __aarch64__
    typedef struct {
        void *__stack, *__gr_top, *__vr_top;
        int   __gr_offs, __vr_offs;
    } __builtin_va_list;

#elif defined __riscv
    typedef char *__builtin_va_list;
    #define __va_reg_size (__riscv_xlen >> 3)
    #define _tcc_align(addr,type) (((unsigned long)addr + __alignof__(type) - 1) \
                                  & -(__alignof__(type)))
    #define __builtin_va_arg(ap,type) (*(sizeof(type) > (2*__va_reg_size) ? *(type **)((ap += __va_reg_size) - __va_reg_size) : (ap = (va_list)(_tcc_align(ap,type) + (sizeof(type)+__va_reg_size - 1)& -__va_reg_size), (type *)(ap - ((sizeof(type)+ __va_reg_size - 1)& -__va_reg_size)))))

#else /* __i386__ */
    typedef char *__builtin_va_list;
    #define __builtin_va_start(ap,last) (ap = ((char *)&(last)) + ((sizeof(last)+3)&~3))
    #define __builtin_va_arg(ap,t) (*(t*)((ap+=(sizeof(t)+3)&~3)-((sizeof(t)+3)&~3)))

#endif
    #define __builtin_va_end(ap) (void)(ap)
    #ifndef __builtin_va_copy
    # define __builtin_va_copy(dest, src) (dest) = (src)
    #endif


    /* TCC BBUILTIN AND BOUNDS ALIASES */
    #ifdef __BOUNDS_CHECKING_ON
    # define __BUILTIN(ret,name,params) \
       ret __builtin_##name params __attribute__((alias("__bound_" #name)));
    # define __BOUND(ret,name,params) \
       ret name params __attribute__((alias("__bound_" #name)));
    #else
    # define __BUILTIN(ret,name,params) \
       ret __builtin_##name params __attribute__((alias(#name)));
    # define __BOUND(ret,name,params)
    #endif
    # define __BOTH(ret,name,params) \
       __BUILTIN(ret,name,params) __BOUND(ret,name,params)

    __BOTH(void*, memcpy, (void *, const void*, __SIZE_TYPE__))
    __BOTH(void*, memmove, (void *, const void*, __SIZE_TYPE__))
    __BOTH(void*, memset, (void *, int, __SIZE_TYPE__))
    __BOTH(int, memcmp, (const void *, const void*, __SIZE_TYPE__))
    __BOTH(__SIZE_TYPE__, strlen, (const char *))
    __BOTH(char*, strcpy, (char *, const char *))
    __BOTH(char*, strncpy, (char *, const char*, __SIZE_TYPE__))
    __BOTH(int, strcmp, (const char*, const char*))
    __BOTH(int, strncmp, (const char*, const char*, __SIZE_TYPE__))
    __BOTH(char*, strcat, (char*, const char*))
    __BOTH(char*, strchr, (const char*, int))
    __BOTH(char*, strdup, (const char*))

#ifdef _WIN32
    #define __MAYBE_REDIR __BOTH
#else  // HAVE MALLOC_REDIR
    #define __MAYBE_REDIR __BUILTIN
#endif
    __MAYBE_REDIR(void*, malloc, (__SIZE_TYPE__))
    __MAYBE_REDIR(void*, realloc, (void *, __SIZE_TYPE__))
    __MAYBE_REDIR(void*, calloc, (__SIZE_TYPE__, __SIZE_TYPE__))
    __MAYBE_REDIR(void*, memalign, (__SIZE_TYPE__, __SIZE_TYPE__))
    __MAYBE_REDIR(void, free, (void*))

#if defined __i386__ || defined __x86_64__
    __BOTH(void*, alloca, (__SIZE_TYPE__))
#endif
    __BUILTIN(void, abort, (void))
    __BOUND(int, longjmp, ())
#ifndef _WIN32
    __BOUND(void*, mmap, ())
    __BOUND(void*, munmap, ())
#endif
    #undef __BUILTIN
    #undef __BOUND
    #undef __BOTH
    #undef __MAYBE_REDIR
