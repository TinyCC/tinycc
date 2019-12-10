#include <stdio.h>

union u {
    unsigned long ul;
    long double ld;
};

void
conv (union u *p)
{
    p->ul = (unsigned int) p->ld;
}

int main (void)
{
    union u v;

    v.ld = 42;
    conv (&v);
    printf ("%lu\n", v.ul);
    return 0;
}
