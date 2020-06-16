        .text
        .align  2
        .global fetch_and_add_riscv64
        .type   fetch_and_add_riscv64, %function
fetch_and_add_riscv64:
        .short 0x4705     #              li      a4,1
        .int   0x0f50000f #              fence   iorw,ow
        .int   0x04e527af #              amoadd.w.aq     a5,a4,(a0)
        .int   0x0017851b #              addiw   a0,a5,1
        .short 0x8082     #              ret

        .size   fetch_and_add_riscv64, .-fetch_and_add_riscv64
