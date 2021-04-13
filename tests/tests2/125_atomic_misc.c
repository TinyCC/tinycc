#include <stdatomic.h>
int printf(const char*,...);

#if defined test_atomic_compare_exchange
int main()
{
    _Atomic int a = 12;
    int b = 77;
    int r;

    atomic_store(&a, b + 0);
    r = atomic_compare_exchange_strong(&a, &b, 99);
    printf("%d %d %d\n", r, a, b);

    atomic_store(&a, b + 3);
    r = atomic_compare_exchange_strong(&a, &b, 99);
    printf("%d %d %d\n", r, a, b);

    return 0;
}

#elif defined test_atomic_store
int main()
{
    int _Atomic i;
    int r;
    atomic_store(&i, 12);
    r = atomic_fetch_add(&i, i);
    printf("r = %d, i = %d\n", r, i);
}

#elif defined test_atomic_store_pointer
typedef struct { char c[4]; } c4;
int main()
{
    int i = 1;
    int _Atomic *p = &i;
    int k = 2;
    atomic_store(&p, &k);
    printf("*p = %d\n", *p);
}

#elif defined test_atomic_store_struct
typedef struct { char c[4]; } c4;
int main()
{
    c4 _Atomic p;
    c4 v = { 1,2,3,4 };
    atomic_store(&p, v);
    printf("%d %d %d %d\n", p.c[0], p.c[1], p.c[2], p.c[3]);
}

#elif defined test_atomic_error_1
int main()
{
    int _Atomic i;
    atomic_load(i);
}

#elif defined test_atomic_error_2
int main()
{
    struct { char c[3]; } _Atomic c3;
    atomic_load(&c3);
}

#elif defined test_atomic_error_3
int main()
{
    _Atomic int *p = 0;
    atomic_fetch_add(&p, 1);
}

#elif defined test_atomic_error_4
int main()
{
    int _Atomic i = 1;
    char c = 2;
    atomic_compare_exchange_strong(&i, &c, 0);
}

#elif defined test_atomic_warn_1
int main()
{
    size_t _Atomic i = 1;
    /* assignment to integer from pointer */
    atomic_store(&i, &i);
}

#elif defined test_atomic_warn_2
int main()
{
    int i = 1;
    char c = 2;
    int _Atomic *p = &i;
    /* assignment from incompatible pointer */
    atomic_store(&p, &c);
}

#elif defined test_atomic_warn_3
int main()
{
    int const i = 1;
    /* assignment to read-only -location */
    atomic_fetch_add(&i, 2);
}

#endif
