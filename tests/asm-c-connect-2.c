#include <stdio.h>

int x3(void)
{
    printf("x3\n");
    return 3;
}

void callx4(void);
__asm__("callx4: call x4; ret");
