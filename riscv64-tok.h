/* ------------------------------------------------------------------ */
/* WARNING: relative order of tokens is important.                    */

/*
 * The specifications are available under https://riscv.org/technical/specifications/
 */

#define DEF_ASM_WITH_SUFFIX(x, y) \
  DEF(TOK_ASM_ ## x ## _ ## y, #x "." #y)

#define DEF_ASM_WITH_SUFFIXES(x, y, z) \
  DEF(TOK_ASM_ ## x ## _ ## y ## _ ## z, #x "." #y "." #z)

#define DEF_ASM_FENCE(x) \
  DEF(TOK_ASM_ ## x ## _fence, #x)

/* register */
 /* integer */
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
 /* float */
 DEF_ASM(f0)
 DEF_ASM(f1)
 DEF_ASM(f2)
 DEF_ASM(f3)
 DEF_ASM(f4)
 DEF_ASM(f5)
 DEF_ASM(f6)
 DEF_ASM(f7)
 DEF_ASM(f8)
 DEF_ASM(f9)
 DEF_ASM(f10)
 DEF_ASM(f11)
 DEF_ASM(f12)
 DEF_ASM(f13)
 DEF_ASM(f14)
 DEF_ASM(f15)
 DEF_ASM(f16)
 DEF_ASM(f17)
 DEF_ASM(f18)
 DEF_ASM(f19)
 DEF_ASM(f20)
 DEF_ASM(f21)
 DEF_ASM(f22)
 DEF_ASM(f23)
 DEF_ASM(f24)
 DEF_ASM(f25)
 DEF_ASM(f26)
 DEF_ASM(f27)
 DEF_ASM(f28)
 DEF_ASM(f29)
 DEF_ASM(f30)
 DEF_ASM(f31)

/* register ABI mnemonics, refer to RISC-V ABI 1.0 */
 /* integer */
 DEF_ASM(zero)
 DEF_ASM(ra)
 DEF_ASM(sp)
 DEF_ASM(gp)
 DEF_ASM(tp)
 DEF_ASM(t0)
 DEF_ASM(t1)
 DEF_ASM(t2)
 DEF_ASM(s0)
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
 /* float */
 DEF_ASM(ft0)
 DEF_ASM(ft1)
 DEF_ASM(ft2)
 DEF_ASM(ft3)
 DEF_ASM(ft4)
 DEF_ASM(ft5)
 DEF_ASM(ft6)
 DEF_ASM(ft7)
 DEF_ASM(fs0)
 DEF_ASM(fs1)
 DEF_ASM(fa0)
 DEF_ASM(fa1)
 DEF_ASM(fa2)
 DEF_ASM(fa3)
 DEF_ASM(fa4)
 DEF_ASM(fa5)
 DEF_ASM(fa6)
 DEF_ASM(fa7)
 DEF_ASM(fs2)
 DEF_ASM(fs3)
 DEF_ASM(fs4)
 DEF_ASM(fs5)
 DEF_ASM(fs6)
 DEF_ASM(fs7)
 DEF_ASM(fs8)
 DEF_ASM(fs9)
 DEF_ASM(fs10)
 DEF_ASM(fs11)
 DEF_ASM(ft8)
 DEF_ASM(ft9)
 DEF_ASM(ft10)
 DEF_ASM(ft11)
 /* not in the ABI */
 DEF_ASM(pc)

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
 DEF_ASM(slliw)
 DEF_ASM(srlw)
 DEF_ASM(srliw)
 DEF_ASM(srai)
 DEF_ASM(sraw)
 DEF_ASM(sraiw)

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
 DEF_ASM_WITH_SUFFIX(c, nop)
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
 DEF_ASM_WITH_SUFFIX(c, srli)
 DEF_ASM_WITH_SUFFIX(c, srai)

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
 /* registers */
 DEF_ASM(cycle)
 DEF_ASM(fcsr)
 DEF_ASM(fflags)
 DEF_ASM(frm)
 DEF_ASM(instret)
 DEF_ASM(time)
 /* RV32I-only */
 DEF_ASM(cycleh)
 DEF_ASM(instreth)
 DEF_ASM(timeh)
 /* pseudo */
 DEF_ASM(csrc)
 DEF_ASM(csrci)
 DEF_ASM(csrr)
 DEF_ASM(csrs)
 DEF_ASM(csrsi)
 DEF_ASM(csrw)
 DEF_ASM(csrwi)
 DEF_ASM(frcsr)
 DEF_ASM(frflags)
 DEF_ASM(frrm)
 DEF_ASM(fscsr)
 DEF_ASM(fsflags)
 DEF_ASM(fsrm)

/* Privileged Instructions */

 DEF_ASM(mrts)
 DEF_ASM(mrth)
 DEF_ASM(hrts)
 DEF_ASM(wfi)

/* pseudoinstructions */
 DEF_ASM(beqz)
 DEF_ASM(bgez)
 DEF_ASM(bgt)
 DEF_ASM(bgtu)
 DEF_ASM(bgtz)
 DEF_ASM(ble)
 DEF_ASM(bleu)
 DEF_ASM(blez)
 DEF_ASM(bltz)
 DEF_ASM(bnez)
 DEF_ASM(call)
 DEF_ASM_WITH_SUFFIX(fabs, d)
 DEF_ASM_WITH_SUFFIX(fabs, s)
 DEF_ASM(fld)
 DEF_ASM(flw)
 DEF_ASM_WITH_SUFFIX(fmv, d)
 DEF_ASM_WITH_SUFFIX(fmv, s)
 DEF_ASM_WITH_SUFFIX(fneg, d)
 DEF_ASM_WITH_SUFFIX(fneg, s)
 DEF_ASM(fsd)
 DEF_ASM(fsw)
 DEF_ASM(j)
 DEF_ASM(jump)
 DEF_ASM(jr)
 DEF_ASM(la)
 DEF_ASM(li)
 DEF_ASM(lla)
 DEF_ASM(mv)
 DEF_ASM(neg)
 DEF_ASM(negw)
 DEF_ASM(nop)
 DEF_ASM(not)
 DEF_ASM(ret)
 DEF_ASM(seqz)
 DEF_ASM_WITH_SUFFIX(sext, w)
 DEF_ASM(sgtz)
 DEF_ASM(sltz)
 DEF_ASM(snez)
 DEF_ASM(tail)

/* Possible values for .option directive */
 DEF_ASM(arch)
 DEF_ASM(rvc)
 DEF_ASM(norvc)
 DEF_ASM(pic)
 DEF_ASM(nopic)
 DEF_ASM(relax)
 DEF_ASM(norelax)
 DEF_ASM(push)
 DEF_ASM(pop)

/* “A” Standard Extension for Atomic Instructions, Version 2.1 */
 /* XXX: Atomic memory operations */
 DEF_ASM_WITH_SUFFIX(lr, w)
 DEF_ASM_WITH_SUFFIXES(lr, w, aq)
 DEF_ASM_WITH_SUFFIXES(lr, w, rl)
 DEF_ASM_WITH_SUFFIXES(lr, w, aqrl)

 DEF_ASM_WITH_SUFFIX(lr, d)
 DEF_ASM_WITH_SUFFIXES(lr, d, aq)
 DEF_ASM_WITH_SUFFIXES(lr, d, rl)
 DEF_ASM_WITH_SUFFIXES(lr, d, aqrl)


 DEF_ASM_WITH_SUFFIX(sc, w)
 DEF_ASM_WITH_SUFFIXES(sc, w, aq)
 DEF_ASM_WITH_SUFFIXES(sc, w, rl)
 DEF_ASM_WITH_SUFFIXES(sc, w, aqrl)

 DEF_ASM_WITH_SUFFIX(sc, d)
 DEF_ASM_WITH_SUFFIXES(sc, d, aq)
 DEF_ASM_WITH_SUFFIXES(sc, d, rl)
 DEF_ASM_WITH_SUFFIXES(sc, d, aqrl)

/* `fence` arguments */
/* NOTE: Order is important */
 DEF_ASM_FENCE(w)
 DEF_ASM_FENCE(r)
 DEF_ASM_FENCE(rw)

 DEF_ASM_FENCE(o)
 DEF_ASM_FENCE(ow)
 DEF_ASM_FENCE(or)
 DEF_ASM_FENCE(orw)

 DEF_ASM_FENCE(i)
 DEF_ASM_FENCE(iw)
 DEF_ASM_FENCE(ir)
 DEF_ASM_FENCE(irw)

 DEF_ASM_FENCE(io)
 DEF_ASM_FENCE(iow)
 DEF_ASM_FENCE(ior)
 DEF_ASM_FENCE(iorw)

#undef DEF_ASM_WITH_SUFFIX
