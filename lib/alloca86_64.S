/* ---------------------------------------------- */
/* alloca86_64.S */

.globl alloca

alloca:
    pop     %rdx
#ifdef TCC_TARGET_PE
    mov     %rcx,%rax
#else
    mov     %rdi,%rax
#endif
    add     $15,%rax
    and     $-16,%rax
    jz      p3

#ifdef TCC_TARGET_PE
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
#ifdef TCC_TARGET_PE
    add     $32,%rax
#endif

p3:
    push    %rdx
    ret

/* mark stack as nonexecutable */
#if defined __ELF__ && defined __linux__
    .section    .note.GNU-stack,"",@progbits
#endif
/* ---------------------------------------------- */
