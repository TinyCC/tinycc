#include <stdarg.h>
#include <stdio.h>

/* Struct arguments larger than 16 bytes are copied to the outgoing argument
   area.  On arm64 the copy of `s` lies more than 4096 bytes above sp, past
   the copy of `b`. */
struct big { char data[4900]; int tag; };
struct small { char *a, *b, *c, *d; long e, f, g, h; };

int take(struct small s, struct big b)
{
    return (int)s.e + b.tag + b.data[10];
}

static long take_va(int n, ...)
{
    va_list ap;
    va_start(ap, n);
    struct small s = va_arg(ap, struct small);
    va_end(ap);
    return s.e;
}

int main(void)
{
    struct big b = {0};
    const volatile struct small s = { .e = 100 };
    b.tag = 40;
    b.data[10] = 2;
    printf("%d\n", take(s, b));
    printf("%ld\n", take_va(1, s));
    return 0;
}
