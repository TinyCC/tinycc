/* ---------------------------------------------- */
/* alloca86b.S */

.globl __bound___alloca

__bound___alloca:
    pop     %edx
    pop     %eax
    mov     %eax, %ecx
    add     $3,%eax
    and     $-4,%eax
    jz      p6

p4:
    cmp     $4096,%eax
    jle     p5
    sub     $4096,%esp
    sub     $4096,%eax
    test    %eax,(%esp)
    jmp p4

p5:
    sub     %eax,%esp
    mov     %esp,%eax

    push    %edx
    push    %eax

    push    %ecx
    push    %eax
    call   __bound_new_region
    add    $8, %esp

    pop     %eax
    pop     %edx

p6:
    push    %edx
    push    %edx
    ret

/* ---------------------------------------------- */
