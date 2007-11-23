/* ---------------------------------------------- */
/* alloca86.S */

.globl _alloca

_alloca:
    pop     %edx
    pop     %eax
    add     $3,%eax
    and     $-4,%eax
    jz      p3
p1:
    cmp     $4096,%eax
    jle     p2
    sub     $4096,%esp
    sub     $4096,%eax
    test    %eax,(%esp)
    jmp p1
p2:
    sub     %eax,%esp
    mov     %esp,%eax
p3:
    push    %edx
    push    %edx
    ret

/* ---------------------------------------------- */
