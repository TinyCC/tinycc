/* ---------------------------------------------- */
/* This file implements for arm/arm64/riscv:
 * __atomic_compare_exchange_1
 * __atomic_compare_exchange_2
 * __atomic_compare_exchange_4
 * __atomic_compare_exchange_8
 */

#ifdef __leading_underscore
# define _(s) _##s
#else
# define _(s) s
#endif

#if defined __i386__
        .text
        .align  2

        .global _(__atomic_test_and_set)
        .type   _(__atomic_test_and_set), %function
_(__atomic_test_and_set):
        movl    4(%esp), %edx
        movl    $1, %eax
        xchgb   (%edx), %al
        ret
	.size   _(__atomic_test_and_set), .-_(__atomic_test_and_set)

        .global _(__atomic_clear)
        .type   _(__atomic_clear), %function
_(__atomic_clear):
        movl    4(%esp), %edx
        xorl    %eax, %eax
        xchgb   (%edx), %al
        ret
	.size   _(__atomic_clear), .-_(__atomic_clear)

#elif defined __x86_64__
        .text
        .align  2

        .global _(__atomic_test_and_set)
        .type   _(__atomic_test_and_set), %function
_(__atomic_test_and_set):
        movl    $1, %eax
        xchgb   (%rdi), %al
        ret
	.size   _(__atomic_test_and_set), .-_(__atomic_test_and_set)

        .global _(__atomic_clear)
        .type   _(__atomic_clear), %function
_(__atomic_clear):
        xorl    %eax, %eax
        xchgb   (%rdi), %al
        ret
	.size   _(__atomic_clear), .-_(__atomic_clear)

#elif defined __arm__

#ifndef __TINYC__
	.arch armv6k
	.syntax unified
#endif
        .text
        .align  2

        .global _(fetch_and_add_arm)
        .type   _(fetch_and_add_arm), %function
_(fetch_and_add_arm):
        mcr     p15, #0, r0, c7, c10, #5
.L0:
        ldrex   r3, [r0]
        add     r3, r3, r1
        strex   r2, r3, [r0]
        cmp     r2, #0
        bne     .L0
        mcr     p15, #0, r0, c7, c10, #5
        bx      lr
        .size   _(fetch_and_add_arm), .-_(fetch_and_add_arm)

        .global _(__atomic_test_and_set)
        .type   _(__atomic_test_and_set), %function
_(__atomic_test_and_set):
#ifdef __TINYC__
	.int	0xe92d4030
	.int	0xee070fba
	.int	0xe5d03000
	.int	0xe24dd014
	.int	0xe1a05000
	.int	0xe2533000
	.int	0xe1a04001
	.int	0x13a03001
	.int	0xee070fba
	.int	0xe5cd300f
	.int	0xe3a03001
	.int	0xe1a02003
	.int	0xe28d100f
	.int	0xe1a00005
	.int	0xe58d4004
	.int	0xe58d4000
	.int	0xeb000009
	.int	0xe3500000
	.int	0x0afffff6
	.int	0xe5dd000f
	.int	0xe28dd014
	.int	0xe8bd8030
#else
        push    {r4, r5, lr}
        mcr     p15, 0, r0, c7, c10, 5
        ldrb    r3, [r0]
        sub     sp, sp, #20
        mov     r5, r0
        subs    r3, r3, #0
        mov     r4, r1
        movne   r3, #1
        mcr     p15, 0, r0, c7, c10, 5
        strb    r3, [sp, #15]
.L20:
        mov     r3, #1
        mov     r2, r3
        add     r1, sp, #15
        mov     r0, r5
        str     r4, [sp, #4]
        str     r4, [sp]
        bl      __atomic_compare_exchange_1
        cmp     r0, #0
        beq     .L20
        ldrb    r0, [sp, #15]
        add     sp, sp, #20
        pop     {r4, r5, pc}
#endif
	.size   _(__atomic_test_and_set), .-_(__atomic_test_and_set)

        .global _(__atomic_clear)
        .type   _(__atomic_clear), %function
_(__atomic_clear):
#ifdef __TINYC__
	.int	0xe3a03000
	.int	0xee070fba
	.int	0xe5c03000
	.int	0xee070fba
	.int	0xe12fff1e
#else
         mov     r3, #0
         mcr     p15, 0, r0, c7, c10, 5
         strb    r3, [r0]
         mcr     p15, 0, r0, c7, c10, 5
         bx      lr
#endif
	.size   _(__atomic_clear), .-_(__atomic_clear)

        .global _(__atomic_compare_exchange_1)
        .type   _(__atomic_compare_exchange_1), %function
_(__atomic_compare_exchange_1):
#ifdef __TINYC__
	.int	0xe52de004
	.int	0xe5d13000
	.int	0xf57ff05b
	.int	0xe1d0cf9f
	.int	0xe15c0003
	.int	0x1a000002
	.int	0xe1c0ef92
	.int	0xe35e0000
	.int	0x1afffff9
	.int	0x03a00001
	.int	0x13a00000
	.int	0xf57ff05b
	.int	0x15c1c000
	.int	0xe49df004
#else
        str     lr, [sp, #-4]!
        ldrb    r3, [r1]
        mcr     p15, 0, r0, c7, c10, 5
.L1:
        ldrexb  ip, [r0]
        cmp     ip, r3
        bne     .L2
        strexb  lr, r2, [r0]
        cmp     lr, #0
        bne     .L1
.L2:
        mcr     p15, 0, r0, c7, c10, 5
        moveq   r0, #1
        movne   r0, #0
        strbne  ip, [r1]
        ldr     pc, [sp], #4
#endif
	.size   _(__atomic_compare_exchange_1), .-_(__atomic_compare_exchange_1)

        .global _(__atomic_compare_exchange_2)
        .type   _(__atomic_compare_exchange_2), %function
_(__atomic_compare_exchange_2):
#ifdef __TINYC__
	.int	0xe52de004
	.int	0xe1d130b0
	.int	0xf57ff05b
	.int	0xe1f0cf9f
	.int	0xe15c0003
	.int	0x1a000002
	.int	0xe1e0ef92
	.int	0xe35e0000
	.int	0x1afffff9
	.int	0x03a00001
	.int	0x13a00000
	.int	0xf57ff05b
	.int	0x11c1c0b0
	.int	0xe49df004
#else
        str     lr, [sp, #-4]!
        ldrh    r3, [r1]
        mcr     p15, 0, r0, c7, c10, 5
.L3:
        ldrexh  ip, [r0]
        cmp     ip, r3
        bne     .L4
        strexh  lr, r2, [r0]
        cmp     lr, #0
        bne     .L3
.L4:
        mcr     p15, 0, r0, c7, c10, 5
        moveq   r0, #1
        movne   r0, #0
        strhne  ip, [r1]
	ldr     pc, [sp], #4
#endif
	.size   _(__atomic_compare_exchange_2), .-_(__atomic_compare_exchange_2)

        .global _(__atomic_compare_exchange_4)
        .type   _(__atomic_compare_exchange_4), %function
_(__atomic_compare_exchange_4):
#ifdef __TINYC__
	.int	0xe52de004
	.int	0xe5913000
	.int	0xf57ff05b
	.int	0xe190cf9f
	.int	0xe15c0003
	.int	0x1a000002
	.int	0xe180ef92
	.int	0xe35e0000
	.int	0x1afffff9
	.int	0x03a00001
	.int	0x13a00000
	.int	0xf57ff05b
	.int	0x1581c000
	.int	0xe49df004
#else
        str     lr, [sp, #-4]!
        ldr     r3, [r1]
        mcr     p15, 0, r0, c7, c10, 5
.L5:
        ldrex   ip, [r0]
        cmp     ip, r3
        bne     .L6
        strex   lr, r2, [r0]
        cmp     lr, #0
        bne     .L5
.L6:
        mcr     p15, 0, r0, c7, c10, 5
        moveq   r0, #1
        movne   r0, #0
        strne   ip, [r1]
        ldr     pc, [sp], #4
#endif
	.size   _(__atomic_compare_exchange_4), .-_(__atomic_compare_exchange_4)

/* ---------------------------------------------- */
#elif defined __aarch64__

        .text
        .align  2

        .global _(fetch_and_add_arm64)
        .type   _(fetch_and_add_arm64), %function
_(fetch_and_add_arm64):
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
        cbnz    w3, _(fetch_and_add_arm64)
        dmb     ish
        ret
#endif
        .size   _(fetch_and_add_arm64), .-_(fetch_and_add_arm64)

        .global _(__atomic_test_and_set)
        .type   _(__atomic_test_and_set), %function
_(__atomic_test_and_set):
#ifdef __TINYC__
	.int	0xa9bf7bfd
	.int	0xaa0003e1
	.int	0x52800020
	.int	0x910003fd
	.int	0x2a0003f0
	.int	0x085ffc20
	.int	0x0811fc30
	.int	0x35ffffd1
	.int	0xa8c17bfd
	.int	0xd65f03c0
#else
         stp     x29, x30, [sp, -16]!
         mov     x1, x0
         mov     w0, 1
         mov     x29, sp
         mov     w16, w0
.L20:
         ldaxrb  w0, [x1]
         stlxrb  w17, w16, [x1]
         cbnz    w17, .L20
         ldp     x29, x30, [sp], 16
	 ret
#endif
	.size   _(__atomic_test_and_set), .-_(__atomic_test_and_set)

        .global _(__atomic_clear)
        .type   _(__atomic_clear), %function
_(__atomic_clear):
#ifdef __TINYC__
	.int	0x089ffc1f
	.int	0xd65f03c0
#else
         stlrb   wzr, [x0]
        ret
#endif
	.size   _(__atomic_clear), .-_(__atomic_clear)

        .global _(__atomic_compare_exchange_1)
        .type   _(__atomic_compare_exchange_1), %function
_(__atomic_compare_exchange_1):
#ifdef __TINYC__
	.int	0xa9be7bfd
	.int	0x910003fd
	.int	0xa90153f3
	.int	0xaa0103f3
	.int	0x12001c41
	.int	0xaa0003e2
	.int	0x39400274
	.int	0x2a1403e0
	.int	0x53001c10
	.int	0x085ffc40
	.int	0x6b10001f
	.int	0x54000061
	.int	0x0811fc41
	.int	0x35ffff91
	.int	0x6b34001f
	.int	0x1a9f17e1
	.int	0x54000040
	.int	0x39000260
	.int	0x2a0103e0
	.int	0xa94153f3
	.int	0xa8c27bfd
	.int	0xd65f03c0
#else
        stp     x29, x30, [sp, -32]!
        mov     x29, sp
        stp     x19, x20, [sp, 16]
        mov     x19, x1
        and     w1, w2, 255
        mov     x2, x0
        ldrb    w20, [x19]
        mov     w0, w20
	uxtb    w16, w0
.L1:
	ldaxrb  w0, [x2]
	cmp     w0, w16
	b.ne    .L2
	stlxrb  w17, w1, [x2]
	cbnz    w17, .L1
.L2:
        cmp     w0, w20, uxtb
        cset    w1, eq
        beq     .L3
        strb    w0, [x19]
.L3:
        mov     w0, w1
        ldp     x19, x20, [sp, 16]
        ldp     x29, x30, [sp], 32
        ret
#endif
        .size   _(__atomic_compare_exchange_1), .-_(__atomic_compare_exchange_1)

        .global _(__atomic_compare_exchange_2)
        .type   _(__atomic_compare_exchange_2), %function
_(__atomic_compare_exchange_2):
#ifdef __TINYC__
	.int	0xa9be7bfd
	.int	0x910003fd
	.int	0xa90153f3
	.int	0xaa0103f3
	.int	0x12003c41
	.int	0xaa0003e2
	.int	0x79400274
	.int	0x2a1403e0
	.int	0x53003c10
	.int	0x485ffc40
	.int	0x6b10001f
	.int	0x54000061
	.int	0x4811fc41
	.int	0x35ffff91
	.int	0x6b34201f
	.int	0x1a9f17e1
	.int	0x54000040
	.int	0x79000260
	.int	0x2a0103e0
	.int	0xa94153f3
	.int	0xa8c27bfd
	.int	0xd65f03c0
#else
        stp     x29, x30, [sp, -32]!
        mov     x29, sp
        stp     x19, x20, [sp, 16]
        mov     x19, x1
        and     w1, w2, 65535
        mov     x2, x0
        ldrh    w20, [x19]
        mov     w0, w20
	uxth    w16, w0
.L4:
	ldaxrh  w0, [x2]
	cmp     w0, w16
	b.ne    .L5
	stlxrh  w17, w1, [x2]
	cbnz    w17, .L4
.L5:
        cmp     w0, w20, uxth
        cset    w1, eq
        beq     .L6
        strh    w0, [x19]
.L6:
        mov     w0, w1
        ldp     x19, x20, [sp, 16]
        ldp     x29, x30, [sp], 32
        ret
#endif
        .size   _(__atomic_compare_exchange_2), .-_(__atomic_compare_exchange_2)

        .global _(__atomic_compare_exchange_4)
        .type   _(__atomic_compare_exchange_4), %function
_(__atomic_compare_exchange_4):
#ifdef __TINYC__
	.int	0xa9be7bfd
	.int	0x910003fd
	.int	0xa90153f3
	.int	0xaa0103f3
	.int	0x2a0203e1
	.int	0xaa0003e2
	.int	0xb9400274
	.int	0x2a1403e0
	.int	0x2a0003f0
	.int	0x885ffc40
	.int	0x6b10001f
	.int	0x54000061
	.int	0x8811fc41
	.int	0x35ffff91
	.int	0x6b14001f
	.int	0x1a9f17e1
	.int	0x54000040
	.int	0xb9000260
	.int	0x2a0103e0
	.int	0xa94153f3
	.int	0xa8c27bfd
	.int	0xd65f03c0
#else
        stp     x29, x30, [sp, -32]!
        mov     x29, sp
        stp     x19, x20, [sp, 16]
        mov     x19, x1
        mov     w1, w2
        mov     x2, x0
        ldr     w20, [x19]
        mov     w0, w20
	mov     w16, w0
.L7:
	ldaxr   w0, [x2]
	cmp     w0, w16
	b.ne    .L8
	stlxr   w17, w1, [x2]
	cbnz    w17, .L7
.L8:
        cmp     w0, w20
        cset    w1, eq
        beq     .L9
        str     w0, [x19]
.L9:
        mov     w0, w1
        ldp     x19, x20, [sp, 16]
        ldp     x29, x30, [sp], 32
        ret
#endif
        .size   _(__atomic_compare_exchange_4), .-_(__atomic_compare_exchange_4)

        .global _(__atomic_compare_exchange_8)
        .type   _(__atomic_compare_exchange_8), %function
_(__atomic_compare_exchange_8):
#ifdef __TINYC__
	.int	0xa9be7bfd
	.int	0x910003fd
	.int	0xa90153f3
	.int	0xaa0103f3
	.int	0xaa0203e1
	.int	0xaa0003e2
	.int	0xf9400274
	.int	0xaa1403e0
	.int	0xaa0003f0
	.int	0xc85ffc40
	.int	0xeb10001f
	.int	0x54000061
	.int	0xc811fc41
	.int	0x35ffff91
	.int	0xeb14001f
	.int	0x1a9f17e1
	.int	0x54000040
	.int	0xf9000260
	.int	0x2a0103e0
	.int	0xa94153f3
	.int	0xa8c27bfd
	.int	0xd65f03c0
#else
        stp     x29, x30, [sp, -32]!
        mov     x29, sp
        stp     x19, x20, [sp, 16]
        mov     x19, x1
        mov     x1, x2
        mov     x2, x0
        ldr     x20, [x19]
        mov     x0, x20
	mov     x16, x0
.L10:
	ldaxr   x0, [x2]
	cmp     x0, x16
	b.ne    .L11
	stlxr   w17, x1, [x2]
	cbnz    w17, .L10
.L11:
        cmp     x0, x20
        cset    w1, eq
        beq     .L12
        str     x0, [x19]
.L12:
        mov     w0, w1
        ldp     x19, x20, [sp, 16]
        ldp     x29, x30, [sp], 32
        ret
#endif
        .size   _(__atomic_compare_exchange_8), .-_(__atomic_compare_exchange_8)

/* ---------------------------------------------- */
#elif defined __riscv

        .text
        .align  2

        .global _(fetch_and_add_riscv64)
        .type   _(fetch_and_add_riscv64), %function
_(fetch_and_add_riscv64):
#ifdef __TINYC__
        .int   0x0f50000f
        .int   0x004b5202f
        .short 0x8082
#else
        fence iorw,ow
        amoadd.w.aq zero,a1,0(a0)
        ret
#endif
        .size   _(fetch_and_add_riscv64), .-_(fetch_and_add_riscv64)

        .global _(__atomic_test_and_set)
        .type   _(__atomic_test_and_set), %function
_(__atomic_test_and_set):
#ifdef __TINYC__
	.int	0x00357793
	.int	0x0037979b
	.short	0x4685
	.short	0x9971
	.int	0x00f696bb
	.int	0x0f50000f
	.int	0x44d5272f
	.int	0x00f7553b
	.int	0x0ff57513
	.short	0x8082
#else
        andi    a5,a0,3
        slliw   a5,a5,3
        li      a3,1
        andi    a0,a0,-4
        sllw    a3,a3,a5
        fence iorw,ow; amoor.w.aq a4,a3,0(a0)
        srlw    a0,a4,a5
        andi    a0,a0,0xff
        ret
#endif
	.size   _(__atomic_test_and_set), .-_(__atomic_test_and_set)

        .global _(__atomic_clear)
        .type   _(__atomic_clear), %function
_(__atomic_clear):
#ifdef __TINYC__
	.int	0x0ff0000f
	.int	0x00050023
	.int	0x0ff0000f
	.short	0x8082
#else
        fence   iorw,iorw
        sb      zero,0(a0)
        fence   iorw,iorw
        ret
#endif
	.size   _(__atomic_clear), .-_(__atomic_clear)

        .global _(__atomic_compare_exchange_1)
        .type   _(__atomic_compare_exchange_1), %function
_(__atomic_compare_exchange_1):
#ifdef __TINYC__
	.short	0x1141
	.short	0x86ba
	.short	0x873e
	.short	0xe406
	.int	0x0ff0000f
	.int	0x0005c803
	.int	0xff857893
	.int	0x0008b783
	.short	0x891d
	.short	0x050e
	.int	0x0ff00693
	.int	0x00a696b3
	.int	0x00a81833
	.int	0x00a61633
	.int	0xfff6c713
	.short	0x8f7d
	.int	0x00f6f333
	.short	0x8f51
	.int	0x03031263
	.int	0x1008b32f
	.int	0x00f31663
	.int	0x18e8be2f
	.int	0xfe0e1ae3
	.int	0x40f30733
	.short	0x879a
	.short	0xff69
	.int	0x0ff0000f
	.short	0x4505
	.short	0xa801
	.int	0x00a7d7b3
	.int	0x00f58023
	.int	0x0ff0000f
	.short	0x4501
	.short	0x60a2
	.short	0x0141
	.short	0x8082
#else
        addi    sp,sp,-16
        mv      a3,a4
        mv      a4,a5
        sd      ra,8(sp)
	fence
	lbu     a6,0(a1)
	andi    a7,a0,-8
	ld      a5,0(a7)
	andi    a0,a0,7
	slli    a0,a0,0x3
	li      a3,255
	sll     a3,a3,a0
	sll     a6,a6,a0
	sll     a2,a2,a0
.L1:
	not     a4,a3
	and     a4,a4,a5
	and     t1,a3,a5
	or      a4,a4,a2
	bne     t1,a6,.L4
.L2:
	lr.d    t1,(a7)
	bne     t1,a5,.L3
	sc.d    t3,a4,(a7)
	bnez    t3,.L2
.L3:
	sub     a4,t1,a5
	mv      a5,t1
	bnez    a4,.L1
	fence
	li      a0,1
	j	.L5
.L4:
	srl     a5,a5,a0
	sb      a5,0(a1)
	fence
	li      a0,0
.L5:
        ld      ra,8(sp)
        addi    sp,sp,16
        jr      ra
#endif
        .size   _(__atomic_compare_exchange_1), .-_(__atomic_compare_exchange_1)

        .global _(__atomic_compare_exchange_2)
        .type   _(__atomic_compare_exchange_2), %function
_(__atomic_compare_exchange_2):
#ifdef __TINYC__
	.short	0x1141
	.short	0x86ba
	.short	0x873e
	.short	0xe406
	.int	0x0ff0000f
	.int	0x0005d803
	.int	0xff857893
	.short	0x67c1
	.short	0x891d
	.int	0x0008b703
	.short	0x050e
	.short	0x17fd
	.int	0x00a797b3
	.int	0x00a81833
	.int	0x00a61633
	.int	0xfff7c693
	.short	0x8ef9
	.int	0x00e7f333
	.short	0x8ed1
	.int	0x03031263
	.int	0x1008b32f
	.int	0x00e31663
	.int	0x18d8be2f
	.int	0xfe0e1ae3
	.int	0x40e306b3
	.short	0x871a
	.short	0xfee9
	.int	0x0ff0000f
	.short	0x4505
	.short	0xa801
	.int	0x00a75733
	.int	0x00e59023
	.int	0x0ff0000f
	.short	0x4501
	.short	0x60a2
	.short	0x0141
	.short	0x8082
#else
        addi    sp,sp,-16
        mv      a3,a4
        mv      a4,a5
        sd      ra,8(sp)
	fence
	lhu     a6,0(a1)
	andi    a7,a0,-8
	lui     a5,0x10
	andi    a0,a0,7
	ld      a4,0(a7)
	slli    a0,a0,0x3
	addi    a5,a5,-1
	sll     a5,a5,a0
	sll     a6,a6,a0
	sll     a2,a2,a0
.L6:
	not     a3,a5
	and     a3,a3,a4
	and     t1,a5,a4
	or      a3,a3,a2
	bne     t1,a6,.L9
.L7:
	lr.d    t1,(a7)
	bne     t1,a4,.L8
	sc.d    t3,a3,(a7)
	bnez    t3,.L7
.L8:
	sub     a3,t1,a4
	mv      a4,t1
	bnez    a3,.L6
	fence
	li      a0,1
	j	.L10
.L9:
	srl     a4,a4,a0
	sh      a4,0(a1)
	fence
	li      a0,0
.L10:
        ld      ra,8(sp)
        addi    sp,sp,16
        jr      ra
#endif
        .size   _(__atomic_compare_exchange_2), .-_(__atomic_compare_exchange_2)

        .global _(__atomic_compare_exchange_4)
        .type   _(__atomic_compare_exchange_4), %function
_(__atomic_compare_exchange_4):
#ifdef __TINYC__
        .short 0x419c
        .int   0x0f50000f
        .int   0x1405272f
        .int   0x00f71663
        .int   0x1cc5282f
        .int   0xfe081ae3
        .int   0x40f707bb
        .int   0x0017b513
        .short 0xc391
        .short 0xc198
        .short 0x8905
        .short 0x8082
#else
	lw      a5,0(a1)
	fence	iorw,ow;
.L11:
	lr.w.aq	a4,0(a0)
	bne	a4,a5,.L12
	sc.w.aq	a6,a2,0(a0)
	bnez	a6,.L11
.L12:
	subw    a5,a4,a5
	seqz    a0,a5
	beq     a5,zero,.L13
	sw      a4,0(a1)
.L13:
	andi    a0,a0,1
        ret
#endif
        .size   _(__atomic_compare_exchange_4), .-_(__atomic_compare_exchange_4)

        .global _(__atomic_compare_exchange_8)
        .type   _(__atomic_compare_exchange_8), %function
_(__atomic_compare_exchange_8):
#ifdef __TINYC__
        .short 0x619c
        .int   0x0f50000f
        .int   0x1405372f
        .int   0x00f71563
        .int   0x1cc536af
        .short 0xfaf5
        .int   0x40f707b3
        .int   0x0017b513
        .short 0xc391
        .short 0xe198
        .short 0x8905
        .short 0x8082
#else
	ld      a5,0(a1)
	fence	iorw,ow;
.L14:
	lr.d.aq	a4,0(a0)
	bne	a4,a5,.L15
	sc.d.aq	a3,a2,0(a0)
	bnez	a3,.L14
.L15:
	sub	a5,a4,a5
	seqz    a0,a5
	beq     a5,zero,.L16
	sd      a4,0(a1)
.L16:
	andi    a0,a0,1
        ret
#endif
        .size   _(__atomic_compare_exchange_8), .-_(__atomic_compare_exchange_8)

/* ---------------------------------------------- */
#endif
