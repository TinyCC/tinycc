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
	.section	.note.GNU-stack,"",%progbits
