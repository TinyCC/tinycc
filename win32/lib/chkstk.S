/* ---------------------------------------------- */
/* chkstk86.s */

#include "../../config.h"

/* ---------------------------------------------- */
#ifndef TCC_TARGET_X86_64
/* ---------------------------------------------- */

.globl __chkstk

__chkstk:
    xchg    (%esp),%ebp     /* store ebp, get ret.addr */
    push    %ebp            /* push ret.addr */
    lea     4(%esp),%ebp    /* setup frame ptr */
    push    %ecx            /* save ecx */
    mov     %ebp,%ecx
P0:
    sub     $4096,%ecx
    test    %eax,(%ecx)
    sub     $4096,%eax
    cmp     $4096,%eax
    jge     P0
    sub     %eax,%ecx
    test    %eax,(%ecx)

    mov     %esp,%eax
    mov     %ecx,%esp
    mov     (%eax),%ecx     /* restore ecx */
    jmp     *4(%eax)

/* ---------------------------------------------- */
#else
/* ---------------------------------------------- */

.globl __chkstk

__chkstk:
    xchg    (%rsp),%rbp     /* store ebp, get ret.addr */
    push    %rbp            /* push ret.addr */
    lea     8(%rsp),%rbp    /* setup frame ptr */
    push    %rcx            /* save ecx */
    mov     %rbp,%rcx
    movslq  %eax,%rax
P0:
    sub     $4096,%rcx
    test    %rax,(%rcx)
    sub     $4096,%rax
    cmp     $4096,%rax
    jge     P0
    sub     %rax,%rcx
    test    %rax,(%rcx)

    mov     %rsp,%rax
    mov     %rcx,%rsp
    mov     (%rax),%rcx     /* restore ecx */
    jmp     *8(%rax)

/* ---------------------------------------------- */
/* avoid msvcrt unwind stuff for setjmp/longjmp */

.globl tinyc_getsp
tinyc_getsp:
    xor %rax,%rax
    ret

/* ---------------------------------------------- */
#endif
/* ---------------------------------------------- */
