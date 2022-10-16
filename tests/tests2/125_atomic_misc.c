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

    atomic_store(&a, b + 5);
    r = atomic_exchange(&a,  33);
    printf("%d %d %d\n", r, a, b);

    atomic_store(&a, b + 10);
    r = atomic_exchange(&a, 66);
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

#elif defined test_atomic_op

#define OP1(func, v, e1, e2) atomic_##func(&c, v) == e1 && c == e2
#define OP2(func, v, e1, e2) atomic_##func(&s, v) == e1 && s == e2
#define OP4(func, v, e1, e2) atomic_##func(&i, v) == e1 && i == e2
#if defined __x86_64__ || defined __aarch64__ || defined __riscv
#define OP8(func, v, e1, e2) atomic_##func(&l, v) == e1 && l == e2
#define HAS_64BITS
#else
#define OP8(func, v, e1, e2) 1
#endif

#define OP(func, v, e1, e2) printf ("%s: %s\n", #func,                        \
                                    OP1(func,v,e1,e2) && OP2(func,v,e1,e2) && \
                                    OP4(func,v,e1,e2) && OP8(func,v,e1,e2)    \
                                    ? "SUCCESS" : "FAIL");
 
int main()
{
    atomic_char c;
    atomic_short s;
    atomic_int i;
#ifdef HAS_64BITS
    atomic_size_t l;
#endif

    atomic_init(&c, 0);
    atomic_init(&s, 0);
    atomic_init(&i, 0);
#ifdef HAS_64BITS
    atomic_init(&l, 0);
#endif

    OP(fetch_add, 10, 0, 10);
    OP(fetch_sub, 5, 10, 5);
    OP(fetch_or, 0x10, 5, 21);
    OP(fetch_xor, 0x20, 21, 53);
    OP(fetch_and, 0x0f, 53, 5);
}

#elif defined test_atomic_op2

typedef __SIZE_TYPE__ size64_t;

#define OP1(func, v, e1, e2) \
    __atomic_##func(&c, v, __ATOMIC_SEQ_CST) == e1 && c == e2
#define OP2(func, v, e1, e2)\
    __atomic_##func(&s, v, __ATOMIC_SEQ_CST) == e1 && s == e2
#define OP4(func, v, e1, e2)\
    __atomic_##func(&i, v, __ATOMIC_SEQ_CST) == e1 && i == e2
#if defined __x86_64__ || defined __aarch64__ || defined __riscv
#define OP8(func, v, e1, e2)\
    __atomic_##func(&l, v, __ATOMIC_SEQ_CST) == e1 && l == e2
#define HAS_64BITS
#else
#define OP8(func, v, e1, e2) 1
#endif

#define OP(func, v, e1, e2) printf ("%s: %s\n", #func,                        \
                                    OP1(func,v,e1,e2) && OP2(func,v,e1,e2) && \
                                    OP4(func,v,e1,e2) && OP8(func,v,e1,e2)    \
                                    ? "SUCCESS" : "FAIL");

int main()
{
    signed char c;
    short s;
    int i;
#ifdef HAS_64BITS
    size64_t l;
#endif

    atomic_init(&c, 0);
    atomic_init(&s, 0);
    atomic_init(&i, 0);
#ifdef HAS_64BITS
    atomic_init(&l, 0);
#endif

    OP(fetch_add, 10, 0, 10);
    OP(fetch_sub, 5, 10, 5);
    OP(fetch_or, 0x10, 5, 21);
    OP(fetch_xor, 0x20, 21, 53);
    OP(fetch_and, 0x0f, 53, 5);
    OP(fetch_nand, 0x01, 5, -2);

    atomic_init(&c, 0);
    atomic_init(&s, 0);
    atomic_init(&i, 0);
#ifdef HAS_64BITS
    atomic_init(&l, 0);
#endif

    OP(add_fetch, 10, 10, 10);
    OP(sub_fetch, 5, 5, 5);
    OP(or_fetch, 0x10, 21, 21);
    OP(xor_fetch, 0x20, 53, 53);
    OP(and_fetch, 0x0f, 5, 5);
    OP(nand_fetch, 0x01, -2, -2);
}

#elif defined test_atomic_thread_signal
int main()
{
    int c;

    atomic_thread_fence(__ATOMIC_SEQ_CST);
    atomic_signal_fence(__ATOMIC_SEQ_CST);
    printf ("%d\n", atomic_is_lock_free(&c));
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
