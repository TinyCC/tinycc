#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ATOMIC_X86_COMPARE_EXCHANGE(TYPE, MODE, SUFFIX) \
    bool __atomic_compare_exchange_##MODE(_Atomic(TYPE) *atom, TYPE *ref, TYPE xchg) \
    { \
        TYPE rv; \
        TYPE cmp = *ref; \
        asm volatile( \
            "lock cmpxchg" SUFFIX " %2,%1\n" \
            : "=a" (rv), "+m" (*atom) \
            : "q" (xchg), "0" (cmp) \
            : "memory" \
        ); \
        *ref = rv; \
        return (rv == cmp); \
    }

#define ATOMIC_X86_LOAD(TYPE, MODE) \
    TYPE __atomic_load_##MODE(const _Atomic(TYPE) *atom) \
    { \
        return *(volatile TYPE *)atom; \
    }

#define ATOMIC_X86_STORE(TYPE, MODE) \
    void __atomic_store_##MODE(_Atomic(TYPE) *atom, TYPE value) \
    { \
        *(volatile TYPE *)atom = value; \
    }
