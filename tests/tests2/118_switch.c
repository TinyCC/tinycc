#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int ibdg(long long n)
{
    switch (n) {
    case                     1LL ...                   9LL: return 1;
    case                    10LL ...                  99LL: return 2;
    case                   100LL ...                 999LL: return 3;
    case                  1000LL ...                9999LL: return 4;
    case                 10000LL ...               99999LL: return 5;
    case                100000LL ...              999999LL: return 6;
    case               1000000LL ...             9999999LL: return 7;
    case              10000000LL ...            99999999LL: return 8;
    case             100000000LL ...           999999999LL: return 9;
    case            1000000000LL ...          9999999999LL: return 10;
    case           10000000000LL ...         99999999999LL: return 11;
    case          100000000000LL ...        999999999999LL: return 12;
    case         1000000000000LL ...       9999999999999LL: return 13;
    case        10000000000000LL ...      99999999999999LL: return 14;
    case       100000000000000LL ...     999999999999999LL: return 15;
    case      1000000000000000LL ...    9999999999999999LL: return 16;
    case     10000000000000000LL ...   99999999999999999LL: return 17;
    case    100000000000000000LL ...  999999999999999999LL: return 18;
    case   1000000000000000000LL ... 9223372036854775807LL: return 19;
    case  -9223372036854775807LL-1LL ...              -1LL: return 20;
    }
    return 0;
}

int ubdg(unsigned long long n)
{
    switch (n) {
    case                    1ULL ...                    9ULL: return 1;
    case                   10ULL ...                   99ULL: return 2;
    case                  100ULL ...                  999ULL: return 3;
    case                 1000ULL ...                 9999ULL: return 4;
    case                10000ULL ...                99999ULL: return 5;
    case               100000ULL ...               999999ULL: return 6;
    case              1000000ULL ...              9999999ULL: return 7;
    case             10000000ULL ...             99999999ULL: return 8;
    case            100000000ULL ...            999999999ULL: return 9;
    case           1000000000ULL ...           9999999999ULL: return 10;
    case          10000000000ULL ...          99999999999ULL: return 11;
    case         100000000000ULL ...         999999999999ULL: return 12;
    case        1000000000000ULL ...        9999999999999ULL: return 13;
    case       10000000000000ULL ...       99999999999999ULL: return 14;
    case      100000000000000ULL ...      999999999999999ULL: return 15;
    case     1000000000000000ULL ...     9999999999999999ULL: return 16;
    case    10000000000000000ULL ...    99999999999999999ULL: return 17;
    case   100000000000000000ULL ...   999999999999999999ULL: return 18;
    case  1000000000000000000ULL ...  9999999999999999999ULL: return 19;
    case 10000000000000000000ULL ... 18446744073709551615ULL: return 20;
    }
    return 0;
}

int main(int argc, char **argv)
{
    unsigned int i;
    unsigned long long v = 1;

    v = 1;
    for (i = 1; i <= 20; i++) {
        printf("%lld : %d\n", (long long) v, ibdg((long long)v));
        v *= 10;
    }
    v = 1;
    for (i = 1; i <= 20; i++) {
        printf("%llu : %d\n", v, ubdg(v));
        v *= 10;
    }
    return 0;
}
