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
    bool __atomic_compare_exchange_##MODE \
        (volatile void *atom, void *ref, TYPE xchg, \
         bool weak, int success_memorder, int failure_memorder) \
    { \
        TYPE rv; \
        TYPE cmp = *(TYPE *)ref; \
        asm volatile( \
            "lock cmpxchg" SUFFIX " %2,%1\n" \
            : "=a" (rv), "+m" (*(TYPE *)atom) \
            : "q" (xchg), "0" (cmp) \
            : "memory" \
        ); \
        *(TYPE *)ref = rv; \
        return (rv == cmp); \
    }

#define ATOMIC_X86_LOAD(TYPE, MODE) \
    TYPE __atomic_load_##MODE(const volatile void *atom, int memorder) \
    { \
        return *(volatile TYPE *)atom; \
    }

#define ATOMIC_X86_STORE(TYPE, MODE) \
    void __atomic_store_##MODE(volatile void *atom, TYPE value, int memorder) \
    { \
        *(volatile TYPE *)atom = value; \
    }

/* Some tcc targets set __GNUC__ */
#if defined(__GNUC__) && !defined(__TINYC__)
#define	ATOMIC_LOAD(t,a,b,c)              t b; __atomic_load((t *)a, (t *)&b, c)
#define COMPARE_EXCHANGE(t,a,b,c,d,e,f)   __atomic_compare_exchange((t *)a,b,&c,d,e,f)
#else
#define ATOMIC_LOAD(t,a,b,c)              t b = __atomic_load((t *)a, c)
#define COMPARE_EXCHANGE(t,a,b,c,d,e,f)   __atomic_compare_exchange((t *)a,b,c,d,e,f)
#endif

#define ATOMIC_GEN_OP(TYPE, MODE, NAME, OP) \
    TYPE __atomic_##NAME##_##MODE(volatile void *atom, TYPE value, int memorder) \
    { \
        TYPE xchg; \
        ATOMIC_LOAD(TYPE, atom, cmp, __ATOMIC_RELAXED); \
        do { \
            xchg = (OP); \
        } while (!COMPARE_EXCHANGE(TYPE, atom, &cmp, xchg, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)); \
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
