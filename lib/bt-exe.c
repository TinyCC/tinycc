/* ------------------------------------------------------------- */
/* for linking rt_printline and the signal/exception handler
   from tccrun.c into executables. */

#define CONFIG_TCC_BACKTRACE_ONLY
#define ONE_SOURCE 1
#define pstrcpy tcc_pstrcpy
#define TCC_SEM_IMPL 1
#include "../tccrun.c"

#ifndef _WIN32
# define __declspec(n)
#endif

__declspec(dllexport)
void __bt_init(rt_context *p, int is_exe)
{
    __attribute__((weak)) int main();
    __attribute__((weak)) void __bound_init(void*, int);

    //fprintf(stderr, "__bt_init %d %p %p %p\n", is_exe, p, p->stab_sym, p->bounds_start), fflush(stderr);

    /* call __bound_init here due to redirection of sigaction */
    /* needed to add global symbols */
    if (p->bounds_start)
	__bound_init(p->bounds_start, -1);

    /* add to chain */
    rt_wait_sem();
    p->next = g_rc, g_rc = p;
    rt_post_sem();
    if (is_exe) {
        /* we are the executable (not a dll) */
        p->top_func = main;
        set_exception_handler();
    }
}

__declspec(dllexport)
void __bt_exit(rt_context *p)
{
    struct rt_context *rc, **pp;
    __attribute__((weak)) void __bound_exit_dll(void*);

    //fprintf(stderr, "__bt_exit %d %p\n", !!p->top_func, p);

    if (p->bounds_start)
	__bound_exit_dll(p->bounds_start);

    /* remove from chain */
    rt_wait_sem();
    for (pp = &g_rc; rc = *pp, rc; pp = &rc->next)
        if (rc == p) {
            *pp = rc->next;
            break;
        }
    rt_post_sem();
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
