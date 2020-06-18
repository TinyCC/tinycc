        .text
        .align  2
        .global fetch_and_add_arm
        .type   fetch_and_add_arm, %function
fetch_and_add_arm:
#ifdef __TINYC__
        .int 0xf57ff05b
        .int 0xe1903f9f
        .int 0xe0833001
        .int 0xe1802f93
        .int 0xe3520000
        .int 0x1afffffa
        .int 0xf57ff05b
        .int 0xe12fff1e
#else
        .arch armv7-a

        dmb     ish
.L0:
        ldrex   r3, [r0]
        add     r3, r3, r1
        strex   r2, r3, [r0]
        cmp     r2, #0
        bne     .L0
        dmb     ish
        bx      lr
#endif
	.size   fetch_and_add_arm, .-fetch_and_add_arm
