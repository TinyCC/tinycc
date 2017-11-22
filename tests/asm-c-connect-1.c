#include <stdio.h>

#if defined _WIN32 && !defined __TINYC__
# define U "_"
#else
# define U
#endif

const char str[] = "x1\n";
#ifdef __x86_64__
asm(U"x1: push %rbp; mov $"U"str, %rdi; call "U"printf; pop %rbp; ret");
#elif defined (__i386__)
asm(U"x1: push $"U"str; call "U"printf; pop %eax; ret");
#endif

int main(int argc, char *argv[])
{
    asm("call "U"x1");
    asm("call "U"x2");
    asm("call "U"x3");
    return 0;
}

static
int x2(void)
{
    printf("x2\n");
    return 2;
}

extern int x3(void);
