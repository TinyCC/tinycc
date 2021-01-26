/* ------------------------------------------------------------- */
/* for linking rt_printline and the signal/exception handler
   from tccrun.c into executables. */

#define CONFIG_TCC_BACKTRACE_ONLY
#include "../tccrun.c"

int (*__rt_error)(void*, void*, const char *, va_list);

#ifndef _WIN32
# define __declspec(n)
#endif

__declspec(dllexport)
void __bt_init(rt_context *p, int num_callers)
{
    __attribute__((weak)) int main();
    __attribute__((weak)) void __bound_init(void*, int);
    struct rt_context *rc = &g_rtctxt;
    //fprintf(stderr, "__bt_init %d %p %p\n", num_callers, p->stab_sym, p->bounds_start), fflush(stderr);
    if (num_callers) {
        memcpy(rc, p, offsetof(rt_context, next));
        rc->num_callers = num_callers - 1;
        rc->top_func = main;
        __rt_error = _rt_error;
        set_exception_handler();
    } else {
        p->next = rc->next, rc->next = p;
    }
}

/* copy a string and truncate it. */
static char *pstrcpy(char *buf, size_t buf_size, const char *s)
{
    int l = strlen(s);
    if (l >= buf_size)
        l = buf_size - 1;
    memcpy(buf, s, l);
    buf[l] = 0;
    return buf;
}
