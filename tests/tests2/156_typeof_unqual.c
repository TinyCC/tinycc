#include <stdio.h>

typedef const volatile int Array2x3[2][3];
_Optional int *const restrict crpoi;

_Static_assert(__builtin_types_compatible_p(typeof_unqual(crpoi) *, _Optional int **),
               "pointer loses const and restrict and retains its target type");
_Static_assert(__builtin_types_compatible_p(__typeof_unqual__(_Optional _Atomic int) *, int *),
               "scalar result is non-atomic and unqualified");
_Static_assert(__builtin_types_compatible_p(typeof_unqual(Array2x3) *, int (*)[2][3]),
               "array retains its dimensions and has unqualified elements");
_Static_assert(__builtin_types_compatible_p(Array2x3 *, const volatile int (*)[2][3]),
               "original array retains its element qualifiers");

static int vla_size(int n)
{
    const int a[n];
    typeof_unqual(a) b;
    b[0] = 1;
    return sizeof b / sizeof b[0];
}

int main(void)
{
    printf("%d\n", vla_size(3));
    return 0;
}
