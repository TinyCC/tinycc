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
