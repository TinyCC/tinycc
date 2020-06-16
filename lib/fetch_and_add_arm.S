        .text
        .align  2
        .global fetch_and_add_arm
        .type   fetch_and_add_arm, %function
fetch_and_add_arm:
        .int 0xee070fba #      mcr     15, 0, r0, cr7, cr10, {5}
        .int 0xe1903f9f #      ldrex   r3, [r0]
        .int 0xe2833001 #      add     r3, r3, #1
        .int 0xe1802f93 #      strex   r2, r3, [r0]
        .int 0xe3520000 #      cmp     r2, #0
        .int 0x1afffffa #      bne     4 <fetch_and_add_arm+0x4>
        .int 0xee070fba #      mcr     15, 0, r0, cr7, cr10, {5}
        .int 0xe1a00003 #      mov     r0, r3
        .int 0xe12fff1e #      bx      lr

	.size   fetch_and_add_arm, .-fetch_and_add_arm
