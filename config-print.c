#include <inttypes.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    switch(argc == 2 ? argv[1][0] : 0) {
        case 'v':
#ifdef __GNUC__
# if __GNUC__ >= 4
            puts("4");
# elif __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 2)
            puts("3");
# else
            puts("2");
# endif
#else
            puts("0");
#endif
            break;
        case 'm':
#ifdef __GNUC__
            printf("%d\n", __GNUC_MINOR__);
#else
	    puts("-1");
#endif
            break;
        case 'e':
            {
                volatile uint32_t i=0x01234567;
                if ((*((uint8_t*)(&i))) == 0x67)
                    puts("yes");
            }
        break;
    }
    return 0;
}
