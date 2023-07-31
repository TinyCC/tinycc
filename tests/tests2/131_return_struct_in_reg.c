#include<stdio.h>

struct s1 {
    unsigned char a:4, b:4;
} g1 = { 0x05, 0x0a };

struct s2 {
    unsigned char a, b;
} g2 = { 0x12, 0x34 };

struct s4 {
    unsigned short a, b;
} g4 = { 0x1245, 0x5678 };

struct s8 {
    unsigned a, b;
} g8 = { 0x12345678, 0x9abcdef0 };

/* returned in 2 registers on riscv64 */
struct s16 {
    unsigned long long a, b;
} g16 = { 0x123456789abcdef0ULL, 0xfedcba9876543210ULL };

/* Homogeneous float aggregate on ARM hard-float */
struct s_f4 {
    double a, b, c, d;
} g_f4 = { 1,2,3,4 };

#define def(S) \
struct s##S f##S(int x) \
{ \
    struct s##S l##S = g##S, *p##S = &l##S; \
    if (x == 0) \
        return g##S; \
    else if (x == 1) \
        return l##S; \
    else \
        return *p##S; \
}

def(1)
def(2)
def(4)
def(8)
def(16)
def(_f4)

#define chk(S,x) \
    struct s##S l##S = f##S(x); \
    printf("%02llx %02llx\n", \
        (unsigned long long)l##S.a, \
        (unsigned long long)l##S.b \
        );

int main()
{
    for (int x = 0;;) {
        chk(1,x);
        chk(2,x);
        chk(4,x);
        chk(8,x);
        chk(16,x);
        struct s_f4 l_f4 = f_f4(x);
        printf("%.1f %.1f %.1f %.1f\n", l_f4.a, l_f4.b, l_f4.c, l_f4.d);
        if (++x > 2)
            break;
        printf("\n");
    }
    return 0;
}
