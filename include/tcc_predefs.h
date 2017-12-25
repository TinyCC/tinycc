#ifdef __x86_64__
#ifndef _WIN64

//This should be in sync with the declaration in our lib/libtcc1.c
/* GCC compatible definition of va_list. */
typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    union {
        unsigned int overflow_offset;
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

#elif __arm__
typedef char *__builtin_va_list;
#define _tcc_alignof(type) ((int)&((struct {char c;type x;} *)0)->x)
#define _tcc_align(addr,type) (((unsigned)addr + _tcc_alignof(type) - 1) \
                               & ~(_tcc_alignof(type) - 1))
#define __builtin_va_start(ap,last) ap = ((char *)&(last)) + ((sizeof(last)+3)&~3)
#define __builtin_va_arg(ap,type) (ap = (void *) ((_tcc_align(ap,type)+sizeof(type)+3) \
                        &~3), *(type *)(ap - ((sizeof(type)+3)&~3)))

#elif defined(__aarch64__)
typedef struct {
    void *__stack;
    void *__gr_top;
    void *__vr_top;
    int   __gr_offs;
    int   __vr_offs;
} __builtin_va_list;

#elif defined __riscv
typedef char *__builtin_va_list;
#define __va_reg_size (__riscv_xlen >> 3)
#define _tcc_align(addr,type) (((unsigned long)addr + __alignof__(type) - 1) \
                               & -(__alignof__(type)))
#define __builtin_va_arg(ap,type) (*(sizeof(type) > (2*__va_reg_size) ? *(type **)((ap += __va_reg_size) - __va_reg_size) : (ap = (va_list)(_tcc_align(ap,type) + (sizeof(type)+__va_reg_size - 1)& -__va_reg_size), (type *)(ap - ((sizeof(type)+ __va_reg_size - 1)& -__va_reg_size)))))

#else /* __i386__ */
typedef char *__builtin_va_list;
#define __builtin_va_start(ap,last) ap = ((char *)&(last)) + ((sizeof(last)+3)&~3)
#define __builtin_va_arg(ap,type) (ap += (sizeof(type)+3)&~3, *(type *)(ap - ((sizeof(type)+3)&~3)))
#endif

#ifndef __builtin_va_copy
#define __builtin_va_copy(dest, src) (dest) = (src)
#endif
#define __builtin_va_end(ap) (void)(ap)
