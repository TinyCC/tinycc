/*****************************************************************************/
/* test 'nodata_wanted' data output suppression */

/*-* test 1: initializer not computable 1 */
void foo() {
    if (1) {
	static short w = (int)&foo; /* error */
    }
}

/*-* test 2: initializer not computable 2 */
void foo() {
    if (0) {
	static short w = (int)&foo; /* error */
    }
}

/*-* test 3: initializer not computable 3 */
void foo();
static short w = (int)&foo; /* error */


/*-* test 4: 2 cast warnings */
void foo() {
    short w = &foo; /* no error */
}

/*-* test 5; nodata_wanted test */
#include <stdio.h>

#define DATA_LBL(s) \
    __asm__(".global d"#s",t"#s"\n.data\nd"#s":\n.text\nt"#s":\n"); \
    extern char d##s[],t##s[];

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
    printf("suppression off\n");
    DATA_LBL(s1);
    if (1) {
        PROG
    }
    DATA_LBL(e1);
    printf("  data length is %s\n", de1 - ds1 ? "not 0":"0");
    //printf("  text length is %s\n", te1 - ts1 ? "not 0":"0");

    printf("suppression on\n");
    DATA_LBL(s2);
    if (0) {
        PROG
    }
    DATA_LBL(e2);
    printf("  data length is %x\n", de2 - ds2);
    //printf("  text length is %X\n", te2 - ts2);
    return 0;
}

/*-* test 6: some test */
int main()
{
    return 34;
}
