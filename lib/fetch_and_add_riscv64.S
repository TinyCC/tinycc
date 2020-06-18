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
