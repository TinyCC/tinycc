#include <stdio.h>
#include <float.h>

union ieee_double_extract
{
    struct {
        unsigned int manl:32;
        unsigned int manh:20;
        unsigned int exp:11;
        unsigned int sig:1;
    } s;
    double d;
};

double scale(double d)
{
    union ieee_double_extract x;

    x.d = d;
    x.d *= 1000;
    return x.d;
}

int
main(void)
{
    printf("%g\n", scale(42));
    return 0;
}
