#include <stdio.h>

int main(int argc, char *argv[])
{
    switch(argc == 2 ? argv[1][0] : 0) {
#ifdef __GNUC__
        case 'v':
            printf("%d\n", __GNUC__);
            break;
        case 'm':
            printf("%d\n", __GNUC_MINOR__);
            break;
#else
        case 'v':
        case 'm':
            puts("0");
            break;
#endif
        case 'b':
        {
            volatile unsigned foo = 0x01234567;
            puts(*(unsigned char*)&foo == 0x67 ? "no" : "yes");
            break;
        }
        case -1:
            /* to test -Wno-unused-result */
            fread(NULL, 1, 1, NULL);
            break;
    }
    return 0;
}
