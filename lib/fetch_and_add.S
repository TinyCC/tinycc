/* ---------------------------------------------- */
#if defined __arm__

        .text
        .align  2
        .global fetch_and_add_arm
        .type   fetch_and_add_arm, %function
fetch_and_add_arm:
        mcr     p15, #0, r0, c7, c10, #5
.L0:
        ldrex   r3, [r0]
        add     r3, r3, r1
        strex   r2, r3, [r0]
        cmp     r2, #0
        bne     .L0
        mcr     p15, #0, r0, c7, c10, #5
        bx      lr
	.size   fetch_and_add_arm, .-fetch_and_add_arm

/* ---------------------------------------------- */
#elif defined __aarch64__

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

/* ---------------------------------------------- */
#elif defined __riscv

        .text
        .align  2
        .global fetch_and_add_riscv64
        .type   fetch_and_add_riscv64, %function
fetch_and_add_riscv64:
#ifdef __TINYC__
        .int   0x0f50000f
        .int   0x004b5202f
        .short 0x8082
#else
        fence iorw,ow
        amoadd.w.aq zero,a1,0(a0)
        ret
#endif

        .size   fetch_and_add_riscv64, .-fetch_and_add_riscv64

/* ---------------------------------------------- */
#endif
