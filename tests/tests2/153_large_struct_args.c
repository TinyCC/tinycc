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

int main(void)
{
    struct big b = {0};
    struct small s = {0};
    b.tag = 40;
    b.data[10] = 2;
    s.e = 100;
    printf("%d\n", take(s, b));
    return 0;
}
