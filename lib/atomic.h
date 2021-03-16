#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ATOMIC_GEN_OP(TYPE, MODE, NAME, OP) \
    TYPE __atomic_##NAME##_##MODE(_Atomic(TYPE) *atom, TYPE value) \
    { \
        TYPE xchg; \
        TYPE cmp = __atomic_load(atom, __ATOMIC_RELAXED); \
        do { \
            xchg = (OP); \
        } while (!__atomic_compare_exchange(atom, &cmp, xchg, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)); \
        return cmp; \
    }

#ifndef ATOMIC_EXCHANGE
#    define ATOMIC_EXCHANGE(TYPE, MODE)     ATOMIC_GEN_OP(TYPE, MODE, exchange, value)
#endif

#ifndef ATOMIC_FETCH_ADD
#    define ATOMIC_FETCH_ADD(TYPE, MODE)    ATOMIC_GEN_OP(TYPE, MODE, fetch_add, (cmp + value))
#endif

#ifndef ATOMIC_FETCH_SUB
#    define ATOMIC_FETCH_SUB(TYPE, MODE)    ATOMIC_GEN_OP(TYPE, MODE, fetch_sub, (cmp - value))
#endif

#ifndef ATOMIC_FETCH_AND
#    define ATOMIC_FETCH_AND(TYPE, MODE)    ATOMIC_GEN_OP(TYPE, MODE, fetch_and, (cmp & value))
#endif

#ifndef ATOMIC_FETCH_OR
#    define ATOMIC_FETCH_OR(TYPE, MODE)     ATOMIC_GEN_OP(TYPE, MODE, fetch_or, (cmp | value))
#endif

#ifndef ATOMIC_FETCH_XOR
#    define ATOMIC_FETCH_XOR(TYPE, MODE)    ATOMIC_GEN_OP(TYPE, MODE, fetch_xor, (cmp ^ value))
#endif
