#include <stdio.h>

int x3(void)
{
    printf("x3\n");
    return 3;
}

/* That callx4 is defined globally (as if ".globl callx4")
   is a TCC extension.  GCC doesn't behave like this.  */
void callx4(void);
__asm__("callx4: call x4; ret");

extern void x5(void);
void callx5_again(void);
void callx5_again(void)
{
    x5();
}
