        .text
        .align  2
        .global fetch_and_add_arm64
        .type   fetch_and_add_arm64, %function
fetch_and_add_arm64:
#ifdef __TINYC__
        .int 0x885f7c02
        .int 0x0b010042
        .int 0x8803fc02
        .int 0x35ffffa3
        .int 0xd5033bbf
        .int 0xd65f03c0
#else
        ldxr    w2, [x0]
        add     w2, w2, w1
        stlxr   w3, w2, [x0]
        cbnz    w3, fetch_and_add_arm64
        dmb     ish
        ret
#endif

        .size   fetch_and_add_arm64, .-fetch_and_add_arm64
