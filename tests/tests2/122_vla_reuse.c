#include <stdio.h>

int
main (void)
{
    int n = 0;
    int  first=1;
    int *p[101];
    if (0) {
        lab:;
    }
    int x[n % 100 + 1];
    if (first == 0) {
        if (&x[0] != p[n % 100 + 1]) {
            printf ("ERROR: %p %p\n", &x[0], p[n % 100 + 1]);
            return(1);
        }
    }
    else {
        p[n % 100 + 1] = &x[0];
        first = n < 100;
    }
    x[0] = 1;
    x[n % 100] = 2;
    n++;
    if (n < 100000)
        goto lab;
    printf ("OK\n");
    return 0;
}

