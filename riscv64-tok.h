/* ------------------------------------------------------------------ */
/* WARNING: relative order of tokens is important.                    */

/*
 * The specifications are available under https://riscv.org/technical/specifications/
 */

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
 /* RV64 */
 DEF_ASM(ld)
 DEF_ASM(lwu)

/* Stores */

 DEF_ASM(sb)
 DEF_ASM(sh)
 DEF_ASM(sw)
 /* RV64 */
 DEF_ASM(sd)

/* Shifts */

 DEF_ASM(sll)
 DEF_ASM(srl)
 DEF_ASM(sra)
 /* RV64 */
 DEF_ASM(slli)
 DEF_ASM(srli)
 DEF_ASM(sllw)
 DEF_ASM(slld)
 DEF_ASM(slliw)
 DEF_ASM(sllid)
 DEF_ASM(srlw)
 DEF_ASM(srld)
 DEF_ASM(srliw)
 DEF_ASM(srlid)
 DEF_ASM(srai)
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
 /* RV64 */
 DEF_ASM(addw)
 DEF_ASM(addiw)
 DEF_ASM(subw)

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

/* Jump */

 DEF_ASM(jal)
 DEF_ASM(jalr)

/* Sync */

 DEF_ASM(fence)
 /* Zifencei extension */
 DEF_ASM_WITH_SUFFIX(fence, i)

/* System call */

 /* used to be called scall and sbreak */
 DEF_ASM(ecall)
 DEF_ASM(ebreak)

/* Counters */

 DEF_ASM(rdcycle)
 DEF_ASM(rdcycleh)
 DEF_ASM(rdtime)
 DEF_ASM(rdtimeh)
 DEF_ASM(rdinstret)
 DEF_ASM(rdinstreth)

/* no operation */
 DEF_ASM(nop)
 DEF_ASM_WITH_SUFFIX(c, nop)

/* “M” Standard Extension for Integer Multiplication and Division, V2.0 */
 DEF_ASM(mul)
 DEF_ASM(mulh)
 DEF_ASM(mulhsu)
 DEF_ASM(mulhu)
 DEF_ASM(div)
 DEF_ASM(divu)
 DEF_ASM(rem)
 DEF_ASM(remu)
 /* RV64 */
 DEF_ASM(mulw)
 DEF_ASM(divw)
 DEF_ASM(divuw)
 DEF_ASM(remw)
 DEF_ASM(remuw)

/* "C" Extension for Compressed Instructions, V2.0 */
/* Loads */
 DEF_ASM_WITH_SUFFIX(c, li)
 DEF_ASM_WITH_SUFFIX(c, lw)
 DEF_ASM_WITH_SUFFIX(c, lwsp)
 /* single float */
 DEF_ASM_WITH_SUFFIX(c, flw)
 DEF_ASM_WITH_SUFFIX(c, flwsp)
 /* double float */
 DEF_ASM_WITH_SUFFIX(c, fld)
 DEF_ASM_WITH_SUFFIX(c, fldsp)
 /* RV64 */
 DEF_ASM_WITH_SUFFIX(c, ld)
 DEF_ASM_WITH_SUFFIX(c, ldsp)

/* Stores */

 DEF_ASM_WITH_SUFFIX(c, sw)
 DEF_ASM_WITH_SUFFIX(c, sd)
 DEF_ASM_WITH_SUFFIX(c, swsp)
 DEF_ASM_WITH_SUFFIX(c, sdsp)
 /* single float */
 DEF_ASM_WITH_SUFFIX(c, fsw)
 DEF_ASM_WITH_SUFFIX(c, fswsp)
 /* double float */
 DEF_ASM_WITH_SUFFIX(c, fsd)
 DEF_ASM_WITH_SUFFIX(c, fsdsp)

/* Shifts */
 DEF_ASM_WITH_SUFFIX(c, slli)
 DEF_ASM_WITH_SUFFIX(c, slli64)
 DEF_ASM_WITH_SUFFIX(c, srli)
 DEF_ASM_WITH_SUFFIX(c, srli64)
 DEF_ASM_WITH_SUFFIX(c, srai)
 DEF_ASM_WITH_SUFFIX(c, srai64)

/* Arithmetic */
 DEF_ASM_WITH_SUFFIX(c, add)
 DEF_ASM_WITH_SUFFIX(c, addi)
 DEF_ASM_WITH_SUFFIX(c, addi16sp)
 DEF_ASM_WITH_SUFFIX(c, addi4spn)
 DEF_ASM_WITH_SUFFIX(c, lui)
 DEF_ASM_WITH_SUFFIX(c, sub)
 DEF_ASM_WITH_SUFFIX(c, mv)
 /* RV64 */
 DEF_ASM_WITH_SUFFIX(c, addw)
 DEF_ASM_WITH_SUFFIX(c, addiw)
 DEF_ASM_WITH_SUFFIX(c, subw)

/* Logical */
 DEF_ASM_WITH_SUFFIX(c, xor)
 DEF_ASM_WITH_SUFFIX(c, or)
 DEF_ASM_WITH_SUFFIX(c, and)
 DEF_ASM_WITH_SUFFIX(c, andi)

/* Branch */
 DEF_ASM_WITH_SUFFIX(c, beqz)
 DEF_ASM_WITH_SUFFIX(c, bnez)

/* Jump */
 DEF_ASM_WITH_SUFFIX(c, j)
 DEF_ASM_WITH_SUFFIX(c, jr)
 DEF_ASM_WITH_SUFFIX(c, jal)
 DEF_ASM_WITH_SUFFIX(c, jalr)

/* System call */
 DEF_ASM_WITH_SUFFIX(c, ebreak)

/* XXX F Extension: Single-Precision Floating Point */
/* XXX D Extension: Double-Precision Floating Point */
/* from the spec: Tables 16.5–16.7 list the RVC instructions. */

/* “Zicsr”, Control and Status Register (CSR) Instructions, V2.0 */
 DEF_ASM(csrrw)
 DEF_ASM(csrrs)
 DEF_ASM(csrrc)
 DEF_ASM(csrrwi)
 DEF_ASM(csrrsi)
 DEF_ASM(csrrci)

/* Privileged Instructions */

 DEF_ASM(mrts)
 DEF_ASM(mrth)
 DEF_ASM(hrts)
 DEF_ASM(wfi)

