#include <stdio.h>

#if defined _WIN32 && !defined __TINYC__
# define _ "_"
#else
# define _
#endif

static int x1_c(void)
{
    printf("x1\n");
    return 1;
}

asm(".text;"_"x1: call "_"x1_c; ret");

void callx4(void);
int main(int argc, char *argv[])
{
    asm("call "_"x1");
    asm("call "_"x2");
    asm("call "_"x3");
    callx4();
    return 0;
}

static
int x2(void)
{
    printf("x2\n");
    return 2;
}

extern int x3(void);

void x4(void)
{
    printf("x4\n");
}
