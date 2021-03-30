/* ---------------------------------------------- */
/* alloca.S */

#ifdef __leading_underscore
# define _(s) _##s
#else
# define _(s) s
#endif

/* ---------------------------------------------- */
#if defined __i386__

.globl _(alloca), _(__alloca)
_(alloca):
_(__alloca):
    push    %ebp
    mov     %esp,%ebp
    mov     8(%ebp),%eax
    add     $3,%eax
    and     $-4,%eax
#ifdef _WIN32
    jmp     .+16 #p2
p1:
    sub     $4096,%esp
    sub     $4096,%eax
    test    %eax,(%esp)
p2:
    cmp     $4096,%eax
    jae     p1
#endif
    sub     %eax,%esp
    mov     4(%ebp),%eax
    mov     0(%ebp),%ebp
    add     $8,%esp
    push    %eax
    lea     8(%esp),%eax
    ret

/* ---------------------------------------------- */
#elif defined __x86_64__

.globl _(alloca)
_(alloca):
    pop     %rdx
#ifdef _WIN32
    mov     %rcx,%rax
#else
    mov     %rdi,%rax
#endif
    add     $15,%rax
    and     $-16,%rax
    jz      p3

#ifdef _WIN32
p1:
    cmp     $4096,%rax
    jbe     p2
    test    %rax,-4096(%rsp)
    sub     $4096,%rsp
    sub     $4096,%rax
    jmp p1
p2:
#endif
    sub     %rax,%rsp
    mov     %rsp,%rax
p3:
    push    %rdx
    ret

/* ---------------------------------------------- */
#elif defined __arm__

	.text
	.align	2
	.global	alloca
	.type	alloca, %function
alloca:
	rsb	sp, r0, sp
	bic	sp, sp, #7
	mov	r0, sp
	mov	pc, lr
	.size	alloca, .-alloca

/* ---------------------------------------------- */
#endif
