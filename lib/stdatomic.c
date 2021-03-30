// for libtcc1, avoid including files that are not part of tcc
// #include <stdint.h>
#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t unsigned long long
#define bool _Bool
#define true 1
#define __ATOMIC_RELAXED 0
#define __ATOMIC_CONSUME 1
#define __ATOMIC_ACQUIRE 2
#define __ATOMIC_RELEASE 3
#define __ATOMIC_ACQ_REL 4
#define __ATOMIC_SEQ_CST 5

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

#define ATOMIC_EXCHANGE(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, exchange, value)
#define ATOMIC_FETCH_ADD(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_add, (cmp + value))
#define ATOMIC_FETCH_SUB(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_sub, (cmp - value))
#define ATOMIC_FETCH_AND(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_and, (cmp & value))
#define ATOMIC_FETCH_OR(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_or, (cmp | value))
#define ATOMIC_FETCH_XOR(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_xor, (cmp ^ value))

ATOMIC_X86_STORE(uint8_t, 1)
ATOMIC_X86_STORE(uint16_t, 2)
ATOMIC_X86_STORE(uint32_t, 4)

ATOMIC_X86_LOAD(uint8_t, 1)
ATOMIC_X86_LOAD(uint16_t, 2)
ATOMIC_X86_LOAD(uint32_t, 4)

ATOMIC_X86_COMPARE_EXCHANGE(uint8_t, 1, "b")
ATOMIC_X86_COMPARE_EXCHANGE(uint16_t, 2, "w")
ATOMIC_X86_COMPARE_EXCHANGE(uint32_t, 4, "l")

ATOMIC_EXCHANGE(uint8_t, 1)
ATOMIC_EXCHANGE(uint16_t, 2)
ATOMIC_EXCHANGE(uint32_t, 4)

ATOMIC_FETCH_ADD(uint8_t, 1)
ATOMIC_FETCH_ADD(uint16_t, 2)
ATOMIC_FETCH_ADD(uint32_t, 4)

ATOMIC_FETCH_SUB(uint8_t, 1)
ATOMIC_FETCH_SUB(uint16_t, 2)
ATOMIC_FETCH_SUB(uint32_t, 4)

ATOMIC_FETCH_AND(uint8_t, 1)
ATOMIC_FETCH_AND(uint16_t, 2)
ATOMIC_FETCH_AND(uint32_t, 4)

ATOMIC_FETCH_OR(uint8_t, 1)
ATOMIC_FETCH_OR(uint16_t, 2)
ATOMIC_FETCH_OR(uint32_t, 4)

ATOMIC_FETCH_XOR(uint8_t, 1)
ATOMIC_FETCH_XOR(uint16_t, 2)
ATOMIC_FETCH_XOR(uint32_t, 4)

#if defined __x86_64__
ATOMIC_X86_STORE(uint64_t, 8)
ATOMIC_X86_LOAD(uint64_t, 8)
ATOMIC_X86_COMPARE_EXCHANGE(uint64_t, 8, "q")
ATOMIC_EXCHANGE(uint64_t, 8)
ATOMIC_FETCH_ADD(uint64_t, 8)
ATOMIC_FETCH_SUB(uint64_t, 8)
ATOMIC_FETCH_AND(uint64_t, 8)
ATOMIC_FETCH_OR(uint64_t, 8)
ATOMIC_FETCH_XOR(uint64_t, 8)
#endif
