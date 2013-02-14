#include <stdio.h>

/* Define architecture */
#if defined(__i386__)
# define TRIPLET_ARCH "i386"
#elif defined(__x86_64__)
# define TRIPLET_ARCH "x86_64"
#elif defined(__arm__)
# define TRIPLET_ARCH "arm"
#else
# define TRIPLET_ARCH "unknown"
#endif

/* Define OS */
#if defined (__linux__)
# define TRIPLET_OS "linux"
#elif defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
# define TRIPLET_OS "kfreebsd"
#elif !defined (__GNU__)
# define TRIPLET_OS "unknown"
#endif

/* Define calling convention and ABI */
#if defined (__ARM_EABI__)
# if defined (__ARM_PCS_VFP)
#  define TRIPLET_ABI "gnueabihf"
# else
#  define TRIPLET_ABI "gnueabi"
# endif
#else
# define TRIPLET_ABI "gnu"
#endif

#ifdef __GNU__
# define TRIPLET TRIPLET_ARCH "-" TRIPLET_ABI
#else
# define TRIPLET TRIPLET_ARCH "-" TRIPLET_OS "-" TRIPLET_ABI
#endif

int main(int argc, char *argv[])
{
    switch(argc == 2 ? argv[1][0] : 0) {
        case 'b':
        {
            volatile unsigned foo = 0x01234567;
            puts(*(unsigned char*)&foo == 0x67 ? "no" : "yes");
            break;
        }
#ifdef __GNUC__
        case 'm':
            printf("%d\n", __GNUC_MINOR__);
            break;
        case 'v':
            printf("%d\n", __GNUC__);
            break;
#else
        case 'm':
        case 'v':
            puts("0");
            break;
#endif
        case 't':
            puts(TRIPLET);
            break;
        case -1:
            /* to test -Wno-unused-result */
            fread(NULL, 1, 1, NULL);
            break;
    }
    return 0;
}
