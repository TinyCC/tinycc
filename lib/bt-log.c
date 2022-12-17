/* ------------------------------------------------------------- */
/* function to get a stack backtrace on demand with a message */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int (*__rt_error)(void*, void*, const char *, va_list);

#ifdef _WIN32
# define DLL_EXPORT __declspec(dllexport)
#else
# define DLL_EXPORT
#endif

/* Needed when using ...libtcc1-usegcc=yes in lib/Makefile */
#if (defined(__GNUC__) && (__GNUC__ >= 6)) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"
#endif

DLL_EXPORT int tcc_backtrace(const char *fmt, ...)
{
    va_list ap;
    int ret;

    if (__rt_error) {
        void *fp = __builtin_frame_address(1);
        void *ip = __builtin_return_address(0);
        va_start(ap, fmt);
        ret = __rt_error(fp, ip, fmt, ap);
        va_end(ap);
    } else {
        const char *p;
        if (fmt[0] == '^' && (p = strchr(fmt + 1, fmt[0])))
            fmt = p + 1;
        va_start(ap, fmt);
        ret = vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, "\n"), fflush(stderr);
    }
    return ret;
}

#if (defined(__GNUC__) && (__GNUC__ >= 6)) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
