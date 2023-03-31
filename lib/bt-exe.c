/* ------------------------------------------------------------- */
/* for linking rt_printline and the signal/exception handler
   from tccrun.c into executables. */

#define CONFIG_TCC_BACKTRACE_ONLY
#define ONE_SOURCE 1
#define pstrcpy tcc_pstrcpy
#include "../tccrun.c"

int (*__rt_error)(void*, void*, const char *, va_list);
__attribute__((weak)) void __bound_checking_lock(void);
__attribute__((weak)) void __bound_checking_unlock(void);

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
    /* call __bound_init here due to redirection of sigaction */
    /* needed to add global symbols */
    if (p->bounds_start) {
	__bound_init(p->bounds_start, -1);
        __bound_checking_lock();
    }
    if (num_callers) {
        memcpy(rc, p, offsetof(rt_context, next));
        rc->num_callers = num_callers - 1;
        rc->top_func = main;
        __rt_error = _rt_error;
        set_exception_handler();
    } else {
        p->next = rc->next, rc->next = p;
    }
    if (p->bounds_start)
        __bound_checking_unlock();
}

__declspec(dllexport)
void __bt_exit(rt_context *p)
{
    __attribute__((weak)) void __bound_exit_dll(void*);
    struct rt_context *rc = &g_rtctxt;

    if (p->bounds_start) {
	__bound_exit_dll(p->bounds_start);
        __bound_checking_lock();
    }
    while (rc) {
        if (rc->next == p) {
	    rc->next = rc->next->next;
	    break;
	}
	rc = rc->next;
    }
    if (p->bounds_start)
        __bound_checking_unlock();
}

/* copy a string and truncate it. */
ST_FUNC char *pstrcpy(char *buf, size_t buf_size, const char *s)
{
    int l = strlen(s);
    if (l >= buf_size)
        l = buf_size - 1;
    memcpy(buf, s, l);
    buf[l] = 0;
    return buf;
}
