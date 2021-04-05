#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

#define NR_THREADS 16
#define NR_STEPS   ((uint32_t)UINT16_MAX * 4)

#define BUG_ON(COND) \
    do { \
        if (!!(COND)) \
            abort(); \
    } while (0)

static
void *adder_simple(void *arg)
{
    size_t step;
    atomic_size_t *counter = arg;

    for (step = 0; step < NR_STEPS; ++step)
        atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);

    return NULL;
}

static
void *adder_cmpxchg(void *arg)
{
    size_t step;
    atomic_size_t *counter = arg;

    for (step = 0; step < NR_STEPS; ++step) {
        size_t xchg;
        size_t cmp = atomic_load_explicit(counter, memory_order_relaxed);

        do {
            xchg = (cmp + 1);
        } while (!atomic_compare_exchange_strong_explicit(counter,
            &cmp, xchg, memory_order_relaxed, memory_order_relaxed));
    }

    return NULL;
}

static
void atomic_counter_test(void *(*adder)(void *arg))
{
    size_t index;
    atomic_size_t counter;
    pthread_t thread[NR_THREADS];

    atomic_init(&counter, 0);

    for (index = 0; index < NR_THREADS; ++index)
        BUG_ON(pthread_create(&thread[index], NULL, adder, (void *)&counter));

    for (index = 0; index < NR_THREADS; ++index)
        BUG_ON(pthread_join(thread[index], NULL));

    if (atomic_load(&counter) == (NR_THREADS * NR_STEPS))
        printf("SUCCESS\n");
    else
        printf("FAILURE\n");
}

int main(void)
{
    atomic_counter_test(adder_simple);
    atomic_counter_test(adder_cmpxchg);

    return 0;
}
