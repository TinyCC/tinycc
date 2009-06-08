/* ---------------------------------------------- */
/* alloca86_64.S */

#include "../config.h"

.globl alloca

alloca:
    pop     %rdx
    mov     %rdi,%rax
    add     $3,%rax
    and     $-4,%rax
    jz      p3

#ifdef TCC_TARGET_PE
    /* XXX: not tested */
p1:
    cmp     $4096,%rax
    jle     p2
    sub     $4096,%rsp
    sub     $4096,%rax
    test    %rax,(%rsp)
    jmp p1
p2:
#endif

    sub     %rax,%rsp
    mov     %rsp,%rax
p3:
    push    %rdx
    ret

/* ---------------------------------------------- */
