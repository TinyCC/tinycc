/* va_list.c - tinycc support for va_list on X86_64 */

#if defined __x86_64__

#include "stdarg.h"

/* Avoid include files, they may not be available when cross compiling */
extern void *memset(void *s, int c, __SIZE_TYPE__ n);
extern void abort(void);

/* This should be in sync with our include/stdarg.h */
#define __VA_GEN_REG    0
#define __VA_FLOAT_REG  1
#define __VA_STACK      2

/* GCC compatible definition of va_list. */
typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    union {
        unsigned int overflow_offset;
        char *overflow_arg_area;
    };
    char *reg_save_area;
} __va_list_struct;

void __va_start(va_list ap, void *fp)
{
    __va_list_struct * _ap = (__va_list_struct*) ap;
    memset(_ap, 0, sizeof(__va_list_struct));
    *_ap = *(__va_list_struct *)((char *)fp - 16);
    _ap->overflow_arg_area = (char *)fp + _ap->overflow_offset;
    _ap->reg_save_area = (char *)fp - 176 - 16;
}

void *__va_arg(va_list ap,
               int arg_type,
               int size, int align)
{
    __va_list_struct * _ap = (__va_list_struct*) ap;
    size = (size + 7) & ~7;
    align = (align + 7) & ~7;
    switch (arg_type) {
    case __VA_GEN_REG:
        if (_ap->gp_offset + size <= 48) {
            _ap->gp_offset += size;
            return _ap->reg_save_area + _ap->gp_offset - size;
        }
        goto use_overflow_area;

    case __VA_FLOAT_REG:
        if (_ap->fp_offset < 128 + 48) {
            _ap->fp_offset += 16;
            return _ap->reg_save_area + _ap->fp_offset - 16;
        }
        size = 8;
        goto use_overflow_area;

    case __VA_STACK:
    use_overflow_area:
        _ap->overflow_arg_area += size;
        _ap->overflow_arg_area = (char*)((long long)(_ap->overflow_arg_area + align - 1) & -align);
        return _ap->overflow_arg_area - size;

    default: /* should never h_appen */
        abort();
        return 0;
    }
}
#endif
