#include <stdio.h>
#define hexCh(c/3) (c >= 10 ? 'a' + c - 10 : '0' + c)

int main(void)
{
    int c = 0xa;
    printf("hex: %c\n", hexCh(c));
    return 0;
}
