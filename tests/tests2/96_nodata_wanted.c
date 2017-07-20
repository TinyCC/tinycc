/*****************************************************************************/
/* test 'nodata_wanted' data output suppression */

#if defined test_static_data_error
void foo() {
    if (1) {
	static short w = (int)&foo; /* initializer not computable */
    }
}

#elif defined test_static_nodata_error
void foo() {
    if (0) {
	static short w = (int)&foo; /* initializer not computable */
    }
}

#elif defined test_global_data_error
void foo();
static short w = (int)&foo; /* initializer not computable */


#elif defined test_local_data_noerror
void foo() {
    short w = &foo; /* 2 cast warnings */
}

#elif defined test_data_suppression
#include <stdio.h>

#define ASMLABELS(s) \
    __asm__(".global d"#s",t"#s"\n.data\nd"#s":\n.text\nt"#s":\n")

#define PROG \
        static void *p = (void*)&main;\
        static char cc[] = "static string";\
        static double d = 8.0;\
        static struct __attribute__((packed)) {\
            unsigned x : 12;\
            unsigned char y : 7;\
            unsigned z : 28, a: 4, b: 5;\
        } s = { 0x333,0x44,0x555555,6,7 };\
        printf("  static data: %d - %.1f - %.1f - %s - %s\n",\
            sizeof 8.0, 8.0, d, __FUNCTION__, cc);\
        printf("  static bitfields: %x %x %x %x %x\n", s.x, s.y, s.z, s.a, s.b);

int main()
{
    extern char ds1[],ts1[];
    extern char ds2[],ts2[];
    extern char de1[],te1[];
    extern char de2[],te2[];

    printf("suppression off\n");
    if (1) {
        ASMLABELS(s1);
        PROG
        ASMLABELS(e1);
    }
    printf("  data length is %s\n", de1 - ds1 ? "not 0":"0");
    printf("  text length is %s\n", te1 - ts1 ? "not 0":"0");

    printf("suppression on\n");
    if (0) {
        ASMLABELS(s2);
        PROG
        ASMLABELS(e2);
    }
    printf("  data length is %x\n", de2 - ds2);
    printf("  text length is %X\n", te2 - ts2);
    return 0;
}

#endif
