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
int __builtin_ffs (int x) { FFSI(x) }
#if __SIZEOF_LONG__ == 4
int __builtin_ffsl (long x) { FFSI(x) }
#else
int __builtin_ffsl (long x) { FFSL(x) }
#endif
int __builtin_ffsll (long long x) { FFSL(x) }

/* Returns the number of leading 0-bits in x, starting at the most significant
   bit position. If x is 0, the result is undefined.  */
int __builtin_clz (unsigned int x) { CLZI(x) }
#if __SIZEOF_LONG__ == 4
int __builtin_clzl (unsigned long x) { CLZI(x) }
#else
int __builtin_clzl (unsigned long x) { CLZL(x) }
#endif
int __builtin_clzll (unsigned long long x) { CLZL(x) }

/* Returns the number of trailing 0-bits in x, starting at the least
   significant bit position. If x is 0, the result is undefined. */
int __builtin_ctz (unsigned int x) { CTZI(x) }
#if __SIZEOF_LONG__ == 4
int __builtin_ctzl (unsigned long x) { CTZI(x) }
#else
int __builtin_ctzl (unsigned long x) { CTZL(x) }
#endif
int __builtin_ctzll (unsigned long long x) { CTZL(x) }

/* Returns the number of leading redundant sign bits in x, i.e. the number
   of bits following the most significant bit that are identical to it.
   There are no special cases for 0 or other values. */
int __builtin_clrsb (int x) { if (x < 0) x = ~x; x <<= 1; CLZI(x) }
#if __SIZEOF_LONG__ == 4
int __builtin_clrsbl (long x) { if (x < 0) x = ~x; x <<= 1; CLZI(x) }
#else
int __builtin_clrsbl (long x) { if (x < 0) x = ~x; x <<= 1; CLZL(x) }
#endif
int __builtin_clrsbll (long long x) { if (x < 0) x = ~x; x <<= 1; CLZL(x) }

/* Returns the number of 1-bits in x.*/
int __builtin_popcount (unsigned int x) { POPCOUNTI(x, 0x3f) }
#if __SIZEOF_LONG__ == 4
int __builtin_popcountl (unsigned long x) { POPCOUNTI(x, 0x3f) }
#else
int __builtin_popcountl (unsigned long x) { POPCOUNTL(x, 0x7f) }
#endif
int __builtin_popcountll (unsigned long long x) { POPCOUNTL(x, 0x7f) }

/* Returns the parity of x, i.e. the number of 1-bits in x modulo 2. */
int __builtin_parity (unsigned int x) { POPCOUNTI(x, 0x01) }
#if __SIZEOF_LONG__ == 4
int __builtin_parityl (unsigned long x) { POPCOUNTI(x, 0x01) }
#else
int __builtin_parityl (unsigned long x) { POPCOUNTL(x, 0x01) }
#endif
int __builtin_parityll (unsigned long long x) { POPCOUNTL(x, 0x01) }
