/* ---------------------------------------------- */
/* alloca-bt.S */

#ifdef __leading_underscore
# define _(s) _##s
#else
# define _(s) s
#endif

/* ---------------------------------------------- */
#if defined __i386__

.globl _(__bound_alloca)
_(__bound_alloca):
    pop     %edx
    pop     %eax
    mov     %eax, %ecx
    add     $3+1,%eax
    and     $-4,%eax
    jz      p6

#ifdef _WIN32
p4:
    cmp     $4096,%eax
    jbe     p5
    test    %eax,-4096(%esp)
    sub     $4096,%esp
    sub     $4096,%eax
    jmp p4

p5:
#endif

    sub     %eax,%esp
    mov     %esp,%eax

    push    %edx
    push    %eax
    push    %ecx
    push    %eax
    call    _(__bound_new_region)
    add     $8, %esp
    pop     %eax
    pop     %edx

p6:
    push    %edx
    push    %edx
    ret

/* ---------------------------------------------- */
#elif defined __x86_64__

.globl _(__bound_alloca)
_(__bound_alloca):
#ifdef _WIN32
    inc %rcx            # add one extra to separate regions
    jmp _(alloca)
.globl _(__bound_alloca_nr)
_(__bound_alloca_nr):
    dec     %rcx
    push    %rax
    mov     %rcx,%rdx
    mov     %rax,%rcx
    sub     $32,%rsp
    call    _(__bound_new_region)
    add     $32,%rsp
    pop     %rax
    ret
#else
    pop     %rdx
    mov     %rdi,%rax
    mov     %rax,%rsi	# size, a second parm to the __bound_new_region

    add     $15 + 1,%rax  # add one extra to separate regions
    and     $-16,%rax
    jz      p3


    sub     %rax,%rsp
    mov     %rsp,%rdi	# pointer, a first parm to the __bound_new_region
    mov     %rsp,%rax

    push    %rdx
    push    %rax
    call    _(__bound_new_region)
    pop     %rax
    pop     %rdx

p3:
    push    %rdx
    ret
#endif

/* ---------------------------------------------- */
#endif
