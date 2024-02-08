// for libtcc1, avoid including files that are not part of tcc
// #include <stdint.h>
#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t unsigned long long
#define bool _Bool
#define false 0
#define true 1
#define __ATOMIC_RELAXED 0
#define __ATOMIC_CONSUME 1
#define __ATOMIC_ACQUIRE 2
#define __ATOMIC_RELEASE 3
#define __ATOMIC_ACQ_REL 4
#define __ATOMIC_SEQ_CST 5
typedef __SIZE_TYPE__ size_t;

void __atomic_thread_fence(int memorder);
#define MemoryBarrier(memorder) __atomic_thread_fence(memorder)

#if defined __i386__ || defined __x86_64__
#define ATOMIC_COMPARE_EXCHANGE(TYPE, MODE, SUFFIX) \
    bool __atomic_compare_exchange_##MODE \
        (volatile void *atom, void *ref, TYPE xchg, \
         bool weak, int success_memorder, int failure_memorder) \
    { \
        TYPE rv; \
        TYPE cmp = *(TYPE *)ref; \
        __asm__ volatile( \
            "lock cmpxchg" SUFFIX " %2,%1\n" \
            : "=a" (rv), "+m" (*(TYPE *)atom) \
            : "q" (xchg), "0" (cmp) \
            : "memory" \
        ); \
        *(TYPE *)ref = rv; \
        return (rv == cmp); \
    }
#else
#define ATOMIC_COMPARE_EXCHANGE(TYPE, MODE, SUFFIX) \
    extern bool __atomic_compare_exchange_##MODE \
        (volatile void *atom, void *ref, TYPE xchg, \
         bool weak, int success_memorder, int failure_memorder);
#endif

#define ATOMIC_LOAD(TYPE, MODE) \
    TYPE __atomic_load_##MODE(const volatile void *atom, int memorder) \
    { \
        MemoryBarrier(__ATOMIC_ACQUIRE); \
        return *(volatile TYPE *)atom; \
    }

#define ATOMIC_STORE(TYPE, MODE) \
    void __atomic_store_##MODE(volatile void *atom, TYPE value, int memorder) \
    { \
        *(volatile TYPE *)atom = value; \
        MemoryBarrier(__ATOMIC_ACQ_REL); \
    }

#define ATOMIC_GEN_OP(TYPE, MODE, NAME, OP, RET) \
    TYPE __atomic_##NAME##_##MODE(volatile void *atom, TYPE value, int memorder) \
    { \
        TYPE xchg, cmp; \
        __atomic_load((TYPE *)atom, (TYPE *)&cmp, __ATOMIC_RELAXED); \
        do { \
            xchg = (OP); \
        } while (!__atomic_compare_exchange((TYPE *)atom, &cmp, &xchg, true, \
                                            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)); \
        return RET; \
    }

#define ATOMIC_EXCHANGE(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, exchange, value, cmp)
#define ATOMIC_ADD_FETCH(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, add_fetch, (cmp + value), xchg)
#define ATOMIC_SUB_FETCH(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, sub_fetch, (cmp - value), xchg)
#define ATOMIC_AND_FETCH(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, and_fetch, (cmp & value), xchg)
#define ATOMIC_OR_FETCH(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, or_fetch, (cmp | value), xchg)
#define ATOMIC_XOR_FETCH(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, xor_fetch, (cmp ^ value), xchg)
#define ATOMIC_NAND_FETCH(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, nand_fetch, ~(cmp & value), xchg)
#define ATOMIC_FETCH_ADD(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_add, (cmp + value), cmp)
#define ATOMIC_FETCH_SUB(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_sub, (cmp - value), cmp)
#define ATOMIC_FETCH_AND(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_and, (cmp & value), cmp)
#define ATOMIC_FETCH_OR(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_or, (cmp | value), cmp)
#define ATOMIC_FETCH_XOR(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_xor, (cmp ^ value), cmp)
#define ATOMIC_FETCH_NAND(TYPE, MODE) \
    ATOMIC_GEN_OP(TYPE, MODE, fetch_nand, ~(cmp & value), cmp)

#define ATOMIC_GEN(TYPE, SIZE, SUFFIX) \
    ATOMIC_STORE(TYPE, SIZE) \
    ATOMIC_LOAD(TYPE, SIZE) \
    ATOMIC_COMPARE_EXCHANGE(TYPE, SIZE, SUFFIX) \
    ATOMIC_EXCHANGE(TYPE, SIZE) \
    ATOMIC_ADD_FETCH(TYPE, SIZE) \
    ATOMIC_SUB_FETCH(TYPE, SIZE) \
    ATOMIC_AND_FETCH(TYPE, SIZE) \
    ATOMIC_OR_FETCH(TYPE, SIZE) \
    ATOMIC_XOR_FETCH(TYPE, SIZE) \
    ATOMIC_NAND_FETCH(TYPE, SIZE) \
    ATOMIC_FETCH_ADD(TYPE, SIZE) \
    ATOMIC_FETCH_SUB(TYPE, SIZE) \
    ATOMIC_FETCH_AND(TYPE, SIZE) \
    ATOMIC_FETCH_OR(TYPE, SIZE) \
    ATOMIC_FETCH_XOR(TYPE, SIZE) \
    ATOMIC_FETCH_NAND(TYPE, SIZE)

ATOMIC_GEN(uint8_t, 1, "b")
ATOMIC_GEN(uint16_t, 2, "w")
ATOMIC_GEN(uint32_t, 4, "l")
#if defined __x86_64__ || defined __aarch64__ || defined __riscv
ATOMIC_GEN(uint64_t, 8, "q")
#endif

/* uses alias to allow building with gcc/clang */
#ifdef __TINYC__
#define ATOMIC(x)      __atomic_##x
#else
#define ATOMIC(x)      __tcc_atomic_##x
#endif

void ATOMIC(signal_fence) (int memorder)
{
}

void ATOMIC(thread_fence) (int memorder)
{
#if defined __i386__
        __asm__ volatile("lock orl $0, (%esp)");
#elif defined __x86_64__
        __asm__ volatile("lock orq $0, (%rsp)");
#elif defined __arm__
        __asm__ volatile(".int 0xee070fba"); // mcr p15, 0, r0, c7, c10, 5
#elif defined __aarch64__
        __asm__ volatile(".int 0xd5033bbf"); // dmb ish
#elif defined __riscv
        __asm__ volatile(".int 0x0ff0000f"); // fence iorw,iorw
#endif
}

bool ATOMIC(is_lock_free) (unsigned long size, const volatile void *ptr)
{
    bool ret;

    switch (size) {
    case 1: ret = true; break;
    case 2: ret = true; break;
    case 4: ret = true; break;
#if defined __x86_64__ || defined __aarch64__ || defined __riscv
    case 8: ret = true; break;
#else
    case 8: ret = false; break;
#endif
    default: ret = false; break;
    }
    return ret;
}

#ifndef __TINYC__
void __atomic_signal_fence(int memorder) __attribute__((alias("__tcc_atomic_signal_fence")));
void __atomic_thread_fence(int memorder) __attribute__((alias("__tcc_atomic_thread_fence")));
bool __atomic_is_lock_free(unsigned long size, const volatile void *ptr) __attribute__((alias("__tcc_atomic_is_lock_free")));
#endif
