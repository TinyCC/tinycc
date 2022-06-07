/* ------------------------------------------------------------------ */
/* WARNING: relative order of tokens is important.                    */

// See https://riscv.org/wp-content/uploads/2017/05/riscv-spec-v2.2.pdf

/* register */

 DEF_ASM(x0)
 DEF_ASM(x1)
 DEF_ASM(x2)
 DEF_ASM(x3)
 DEF_ASM(x4)
 DEF_ASM(x5)
 DEF_ASM(x6)
 DEF_ASM(x7)
 DEF_ASM(x8)
 DEF_ASM(x9)
 DEF_ASM(x10)
 DEF_ASM(x11)
 DEF_ASM(x12)
 DEF_ASM(x13)
 DEF_ASM(x14)
 DEF_ASM(x15)
 DEF_ASM(x16)
 DEF_ASM(x17)
 DEF_ASM(x18)
 DEF_ASM(x19)
 DEF_ASM(x20)
 DEF_ASM(x21)
 DEF_ASM(x22)
 DEF_ASM(x23)
 DEF_ASM(x24)
 DEF_ASM(x25)
 DEF_ASM(x26)
 DEF_ASM(x27)
 DEF_ASM(x28)
 DEF_ASM(x29)
 DEF_ASM(x30)
 DEF_ASM(x31)

/* register macros */

 DEF_ASM(zero)
 DEF_ASM(ra)
 DEF_ASM(sp)
 DEF_ASM(gp)
 DEF_ASM(tp)
 DEF_ASM(t0)
 DEF_ASM(t1)
 DEF_ASM(t2)
 DEF_ASM(fp)
 DEF_ASM(s1)
 DEF_ASM(a0)
 DEF_ASM(a1)
 DEF_ASM(a2)
 DEF_ASM(a3)
 DEF_ASM(a4)
 DEF_ASM(a5)
 DEF_ASM(a6)
 DEF_ASM(a7)
 DEF_ASM(s2)
 DEF_ASM(s3)
 DEF_ASM(s4)
 DEF_ASM(s5)
 DEF_ASM(s6)
 DEF_ASM(s7)
 DEF_ASM(s8)
 DEF_ASM(s9)
 DEF_ASM(s10)
 DEF_ASM(s11)
 DEF_ASM(t3)
 DEF_ASM(t4)
 DEF_ASM(t5)
 DEF_ASM(t6)

 DEF_ASM(s0) // = x8

 DEF_ASM(pc)

#define DEF_ASM_WITH_SUFFIX(x, y) \
  DEF(TOK_ASM_ ## x ## _ ## y, #x #y)

/*   Loads */

 DEF_ASM(lb)
 DEF_ASM(lh)
 DEF_ASM(lw)
 DEF_ASM(lbu)
 DEF_ASM(lhu)
 DEF_ASM(ld)
 DEF_ASM(lq)
 DEF_ASM(lwu)
 DEF_ASM(ldu)

/* Stores */

 DEF_ASM(sb)
 DEF_ASM(sh)
 DEF_ASM(sw)
 DEF_ASM(sd)
 DEF_ASM(sq)

/* Shifts */

 DEF_ASM(sll)
 DEF_ASM(slli)
 DEF_ASM(srl)
 DEF_ASM(srli)
 DEF_ASM(sra)
 DEF_ASM(srai)

 DEF_ASM(sllw)
 DEF_ASM(slld)
 DEF_ASM(slliw)
 DEF_ASM(sllid)
 DEF_ASM(srlw)
 DEF_ASM(srld)
 DEF_ASM(srliw)
 DEF_ASM(srlid)
 DEF_ASM(sraw)
 DEF_ASM(srad)
 DEF_ASM(sraiw)
 DEF_ASM(sraid)

/* Arithmetic */

 DEF_ASM(add)
 DEF_ASM(addi)
 DEF_ASM(sub)
 DEF_ASM(lui)
 DEF_ASM(auipc)

 DEF_ASM(addw)
 DEF_ASM(addd)
 DEF_ASM(addiw)
 DEF_ASM(addid)
 DEF_ASM(subw)
 DEF_ASM(subd)

/* Logical */

 DEF_ASM(xor)
 DEF_ASM(xori)
 DEF_ASM(or)
 DEF_ASM(ori)
 DEF_ASM(and)
 DEF_ASM(andi)

/* Compare */

 DEF_ASM(slt)
 DEF_ASM(slti)
 DEF_ASM(sltu)
 DEF_ASM(sltiu)

/* Branch */

 DEF_ASM(beq)
 DEF_ASM(bne)
 DEF_ASM(blt)
 DEF_ASM(bge)
 DEF_ASM(bltu)
 DEF_ASM(bgeu)

/* Sync */

 DEF_ASM(fence)
 DEF_ASM_WITH_SUFFIX(fence, i)

/* System call */

 DEF_ASM(scall)
 DEF_ASM(sbreak)

/* Counters */

 DEF_ASM(rdcycle)
 DEF_ASM(rdcycleh)
 DEF_ASM(rdtime)
 DEF_ASM(rdtimeh)
 DEF_ASM(rdinstret)
 DEF_ASM(rdinstreth)

/* Privileged Instructions */

 DEF_ASM(ecall)
 DEF_ASM(ebreak)

 DEF_ASM(mrts)
 DEF_ASM(mrth)
 DEF_ASM(hrts)
 DEF_ASM(wfi)

