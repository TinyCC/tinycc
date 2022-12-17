/* uses alias to allow building with gcc/clang */
#ifdef __TINYC__
#define	BUILTIN(x)	__builtin_##x
#define	BUILTINN(x)	"__builtin_" # x
#else
#define	BUILTIN(x)	__tcc_builtin_##x
#define	BUILTINN(x)	"__tcc_builtin_" # x
#endif

/* ---------------------------------------------- */
/* This file implements:
 * __builtin_ffs
 * __builtin_clz
 * __builtin_ctz
 * __builtin_clrsb
 * __builtin_popcount
 * __builtin_parity
 * for int, long and long long
 */

static const unsigned char table_1_32[] = {
     0,  1, 28,  2, 29, 14, 24,  3, 30, 22, 20, 15, 25, 17,  4,  8, 
    31, 27, 13, 23, 21, 19, 16,  7, 26, 12, 18,  6, 11,  5, 10,  9
};
static const unsigned char table_2_32[32] = {
    31, 22, 30, 21, 18, 10, 29,  2, 20, 17, 15, 13,  9,  6, 28,  1,
    23, 19, 11,  3, 16, 14,  7, 24, 12,  4,  8, 25,  5, 26, 27,  0
};
static const unsigned char table_1_64[] = {
     0,  1,  2, 53,  3,  7, 54, 27,  4, 38, 41,  8, 34, 55, 48, 28,
    62,  5, 39, 46, 44, 42, 22,  9, 24, 35, 59, 56, 49, 18, 29, 11,
    63, 52,  6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
    51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12
};
static const unsigned char table_2_64[] = {
    63, 16, 62,  7, 15, 36, 61,  3,  6, 14, 22, 26, 35, 47, 60,  2,
     9,  5, 28, 11, 13, 21, 42, 19, 25, 31, 34, 40, 46, 52, 59,  1,
    17,  8, 37,  4, 23, 27, 48, 10, 29, 12, 43, 20, 32, 41, 53, 18,
    38, 24, 49, 30, 44, 33, 54, 39, 50, 45, 55, 51, 56, 57, 58,  0
};

#define FFSI(x) \
    return table_1_32[((x & -x) * 0x077cb531u) >> 27] + (x != 0);
#define FFSL(x) \
    return table_1_64[((x & -x) * 0x022fdd63cc95386dull) >> 58] + (x != 0);
#define CTZI(x) \
    return table_1_32[((x & -x) * 0x077cb531u) >> 27];
#define CTZL(x) \
    return table_1_64[((x & -x) * 0x022fdd63cc95386dull) >> 58];
#define CLZI(x)   \
    x |= x >> 1;  \
    x |= x >> 2;  \
    x |= x >> 4;  \
    x |= x >> 8;  \
    x |= x >> 16; \
    return table_2_32[(x * 0x07c4acddu) >> 27];
#define CLZL(x)   \
    x |= x >> 1;  \
    x |= x >> 2;  \
    x |= x >> 4;  \
    x |= x >> 8;  \
    x |= x >> 16; \
    x |= x >> 32; \
    return table_2_64[x * 0x03f79d71b4cb0a89ull >> 58];
#define POPCOUNTI(x, m)                                                   \
    x = x - ((x >> 1) & 0x55555555);                                      \
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);                       \
    x = (x + (x >> 4)) & 0xf0f0f0f;                                       \
    return ((x * 0x01010101) >> 24) & m; 
#define POPCOUNTL(x, m)                                                   \
    x = x - ((x >> 1) & 0x5555555555555555ull);                           \
    x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull); \
    x = (x + (x >> 4)) & 0xf0f0f0f0f0f0f0full;                            \
    return ((x * 0x0101010101010101ull) >> 56) & m;

/* Returns one plus the index of the least significant 1-bit of x,
   or if x is zero, returns zero. */
int BUILTIN(ffs) (int x) { FFSI(x) }
int BUILTIN(ffsll) (long long x) { FFSL(x) }
#if __SIZEOF_LONG__ == 4
int BUILTIN(ffsl) (long x) __attribute__((alias(BUILTINN(ffs))));
#else
int BUILTIN(ffsl) (long x) __attribute__((alias(BUILTINN(ffsll))));
#endif

/* Returns the number of leading 0-bits in x, starting at the most significant
   bit position. If x is 0, the result is undefined.  */
int BUILTIN(clz) (unsigned int x) { CLZI(x) }
int BUILTIN(clzll) (unsigned long long x) { CLZL(x) }
#if __SIZEOF_LONG__ == 4
int BUILTIN(clzl) (unsigned long x) __attribute__((alias(BUILTINN(clz))));
#else
int BUILTIN(clzl) (unsigned long x) __attribute__((alias(BUILTINN(clzll))));
#endif

/* Returns the number of trailing 0-bits in x, starting at the least
   significant bit position. If x is 0, the result is undefined. */
int BUILTIN(ctz) (unsigned int x) { CTZI(x) }
int BUILTIN(ctzll) (unsigned long long x) { CTZL(x) }
#if __SIZEOF_LONG__ == 4
int BUILTIN(ctzl) (unsigned long x) __attribute__((alias(BUILTINN(ctz))));
#else
int BUILTIN(ctzl) (unsigned long x) __attribute__((alias(BUILTINN(ctzll))));
#endif

/* Returns the number of leading redundant sign bits in x, i.e. the number
   of bits following the most significant bit that are identical to it.
   There are no special cases for 0 or other values. */
int BUILTIN(clrsb) (int x) { if (x < 0) x = ~x; x <<= 1; CLZI(x) }
int BUILTIN(clrsbll) (long long x) { if (x < 0) x = ~x; x <<= 1; CLZL(x) }
#if __SIZEOF_LONG__ == 4
int BUILTIN(clrsbl) (long x) __attribute__((alias(BUILTINN(clrsb))));
#else
int BUILTIN(clrsbl) (long x) __attribute__((alias(BUILTINN(clrsbll))));
#endif

/* Returns the number of 1-bits in x.*/
int BUILTIN(popcount) (unsigned int x) { POPCOUNTI(x, 0x3f) }
int BUILTIN(popcountll) (unsigned long long x) { POPCOUNTL(x, 0x7f) }
#if __SIZEOF_LONG__ == 4
int BUILTIN(popcountl) (unsigned long x) __attribute__((alias(BUILTINN(popcount))));
#else
int BUILTIN(popcountl ) (unsigned long x) __attribute__((alias(BUILTINN(popcountll))));
#endif

/* Returns the parity of x, i.e. the number of 1-bits in x modulo 2. */
int BUILTIN(parity) (unsigned int x) { POPCOUNTI(x, 0x01) }
int BUILTIN(parityll) (unsigned long long x) { POPCOUNTL(x, 0x01) }
#if __SIZEOF_LONG__ == 4
int BUILTIN(parityl) (unsigned long x) __attribute__((alias(BUILTINN(parity))));
#else
int BUILTIN(parityl) (unsigned long x) __attribute__((alias(BUILTINN(parityll))));
#endif

#ifndef __TINYC__
#if defined(__GNUC__) && (__GNUC__ >= 6)
/* gcc overrides alias from __builtin_ffs... to ffs.. so use assembly code */
__asm__(".globl  __builtin_ffs");
__asm__(".set __builtin_ffs,__tcc_builtin_ffs");
__asm__(".globl  __builtin_ffsl");
__asm__(".set __builtin_ffsl,__tcc_builtin_ffsl");
__asm__(".globl  __builtin_ffsll");
__asm__(".set __builtin_ffsll,__tcc_builtin_ffsll");
#else
int __builtin_ffs(int x) __attribute__((alias("__tcc_builtin_ffs")));
int __builtin_ffsl(long x) __attribute__((alias("__tcc_builtin_ffsl")));
int __builtin_ffsll(long long x) __attribute__((alias("__tcc_builtin_ffsll")));
#endif
int __builtin_clz(unsigned int x) __attribute__((alias("__tcc_builtin_clz")));
int __builtin_clzl(unsigned long x) __attribute__((alias("__tcc_builtin_clzl")));
int __builtin_clzll(unsigned long long x) __attribute__((alias("__tcc_builtin_clzll")));
int __builtin_ctz(unsigned int x) __attribute__((alias("__tcc_builtin_ctz")));
int __builtin_ctzl(unsigned long x) __attribute__((alias("__tcc_builtin_ctzl")));
int __builtin_ctzll(unsigned long long x) __attribute__((alias("__tcc_builtin_ctzll")));
int __builtin_clrsb(int x) __attribute__((alias("__tcc_builtin_clrsb")));
int __builtin_clrsbl(long x) __attribute__((alias("__tcc_builtin_clrsbl")));
int __builtin_clrsbll(long long x) __attribute__((alias("__tcc_builtin_clrsbll")));
int __builtin_popcount(unsigned int x) __attribute__((alias("__tcc_builtin_popcount")));
int __builtin_popcountl(unsigned long x) __attribute__((alias("__tcc_builtin_popcountl")));
int __builtin_popcountll(unsigned long long x) __attribute__((alias("__tcc_builtin_popcountll")));
int __builtin_parity(unsigned int x) __attribute__((alias("__tcc_builtin_parity")));
int __builtin_parityl(unsigned long x) __attribute__((alias("__tcc_builtin_parityl")));
int __builtin_parityll(unsigned long long x) __attribute__((alias("__tcc_builtin_parityll")));
#endif
