        .text
        .align  2
        .global fetch_and_add_arm64
        .type   fetch_and_add_arm64, %function
fetch_and_add_arm64:
        .int 0x885f7c01 #      ldxr    w1, [x0]
        .int 0x11000421 #      add     w1, w1, #0x1
        .int 0x8802fc01 #      stlxr   w2, w1, [x0]
        .int 0x35ffffa2 #      cbnz    w2, 0 <fetch_and_add_arm64>
        .int 0xd5033bbf #      dmb     ish
        .int 0x2a0103e0 #      mov     w0, w1
        .int 0xd65f03c0 #      ret

	.size   fetch_and_add_arm64, .-fetch_and_add_arm64
