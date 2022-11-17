#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

#define NR_THREADS 16
#define NR_STEPS   ((uint32_t)UINT16_MAX)

#define BUG_ON(COND) \
    do { \
        if (!!(COND)) \
            abort(); \
    } while (0)

#if defined __x86_64__ || defined __aarch64__ || defined __riscv
#define HAS_64BITS
#endif

typedef struct {
    atomic_flag flag;
    atomic_uchar uc;
    atomic_ushort us;
    atomic_uint ui;
#ifdef HAS_64BITS
    atomic_size_t ul;
#endif
} counter_type;

static
void *adder_simple(void *arg)
{
    size_t step;
    counter_type *counter = arg;

    for (step = 0; step < NR_STEPS; ++step) {
        atomic_fetch_add_explicit(&counter->uc, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&counter->us, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&counter->ui, 1, memory_order_relaxed);
#ifdef HAS_64BITS
        atomic_fetch_add_explicit(&counter->ul, 1, memory_order_relaxed);
#endif
    }

    return NULL;
}

static
void *adder_cmpxchg(void *arg)
{
    size_t step;
    counter_type *counter = arg;

    for (step = 0; step < NR_STEPS; ++step) {
        unsigned char xchgc;
        unsigned short xchgs;
        unsigned int xchgi;
#ifdef HAS_64BITS
        size_t xchgl;
#endif
        unsigned char cmpc = atomic_load_explicit(&counter->uc, memory_order_relaxed);
        unsigned short cmps = atomic_load_explicit(&counter->us, memory_order_relaxed);
        unsigned int cmpi = atomic_load_explicit(&counter->ui, memory_order_relaxed);
#ifdef HAS_64BITS
        size_t cmpl = atomic_load_explicit(&counter->ul, memory_order_relaxed);
#endif

        do {
            xchgc = (cmpc + 1);
        } while (!atomic_compare_exchange_strong_explicit(&counter->uc,
            &cmpc, xchgc, memory_order_relaxed, memory_order_relaxed));
        do {
            xchgs = (cmps + 1);
        } while (!atomic_compare_exchange_strong_explicit(&counter->us,
            &cmps, xchgs, memory_order_relaxed, memory_order_relaxed));
        do {
            xchgi = (cmpi + 1);
        } while (!atomic_compare_exchange_strong_explicit(&counter->ui,
            &cmpi, xchgi, memory_order_relaxed, memory_order_relaxed));
#ifdef HAS_64BITS
        do {
            xchgl = (cmpl + 1);
        } while (!atomic_compare_exchange_strong_explicit(&counter->ul,
            &cmpl, xchgl, memory_order_relaxed, memory_order_relaxed));
#endif
    }

    return NULL;
}

static
void *adder_test_and_set(void *arg)
{
    size_t step;
    counter_type *counter = arg;

    for (step = 0; step < NR_STEPS; ++step) {
        while (atomic_flag_test_and_set(&counter->flag));
        ++counter->uc;
        ++counter->us;
        ++counter->ui;
#ifdef HAS_64BITS
        ++counter->ul;
#endif
        atomic_flag_clear(&counter->flag);
    }

    return NULL;
}

static
void atomic_counter_test(void *(*adder)(void *arg))
{
    size_t index;
    counter_type counter;
    pthread_t thread[NR_THREADS];

    atomic_flag_clear(&counter.flag);
    atomic_init(&counter.uc, 0);
    atomic_init(&counter.us, 0);
    atomic_init(&counter.ui, 0);
#ifdef HAS_64BITS
    atomic_init(&counter.ul, 0);
#endif

    for (index = 0; index < NR_THREADS; ++index)
        BUG_ON(pthread_create(&thread[index], NULL, adder, (void *)&counter));

    for (index = 0; index < NR_THREADS; ++index)
        BUG_ON(pthread_join(thread[index], NULL));

    if (atomic_load(&counter.uc) == ((NR_THREADS * NR_STEPS) & 0xffu)
        && atomic_load(&counter.us) == ((NR_THREADS * NR_STEPS) & 0xffffu)
        && atomic_load(&counter.ui) == (NR_THREADS * NR_STEPS)
#ifdef HAS_64BITS
        && atomic_load(&counter.ul) == (NR_THREADS * NR_STEPS)
#endif
        )
        printf("SUCCESS\n");
    else
        printf("FAILURE\n");
}

int main(void)
{
    atomic_counter_test(adder_simple);
    atomic_counter_test(adder_cmpxchg);
    atomic_counter_test(adder_test_and_set);

    return 0;
}
