/* ------------------------------------------------------------------ */
/* ARM64 (AArch64) assembler token definitions for TCC                */

/* General purpose registers - 64-bit */
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

/* General purpose registers - 32-bit */
 DEF_ASM(w0)
 DEF_ASM(w1)
 DEF_ASM(w2)
 DEF_ASM(w3)
 DEF_ASM(w4)
 DEF_ASM(w5)
 DEF_ASM(w6)
 DEF_ASM(w7)
 DEF_ASM(w8)
 DEF_ASM(w9)
 DEF_ASM(w10)
 DEF_ASM(w11)
 DEF_ASM(w12)
 DEF_ASM(w13)
 DEF_ASM(w14)
 DEF_ASM(w15)
 DEF_ASM(w16)
 DEF_ASM(w17)
 DEF_ASM(w18)
 DEF_ASM(w19)
 DEF_ASM(w20)
 DEF_ASM(w21)
 DEF_ASM(w22)
 DEF_ASM(w23)
 DEF_ASM(w24)
 DEF_ASM(w25)
 DEF_ASM(w26)
 DEF_ASM(w27)
 DEF_ASM(w28)
 DEF_ASM(w29)
 DEF_ASM(w30)

/* Special registers */
 DEF_ASM(sp)
 DEF_ASM(xzr)
 DEF_ASM(wzr)

/* SIMD/FP registers - 128-bit views */
 DEF_ASM(v0)
 DEF_ASM(v1)
 DEF_ASM(v2)
 DEF_ASM(v3)
 DEF_ASM(v4)
 DEF_ASM(v5)
 DEF_ASM(v6)
 DEF_ASM(v7)
 DEF_ASM(v8)
 DEF_ASM(v9)
 DEF_ASM(v10)
 DEF_ASM(v11)
 DEF_ASM(v12)
 DEF_ASM(v13)
 DEF_ASM(v14)
 DEF_ASM(v15)
 DEF_ASM(v16)
 DEF_ASM(v17)
 DEF_ASM(v18)
 DEF_ASM(v19)
 DEF_ASM(v20)
 DEF_ASM(v21)
 DEF_ASM(v22)
 DEF_ASM(v23)
 DEF_ASM(v24)
 DEF_ASM(v25)
 DEF_ASM(v26)
 DEF_ASM(v27)
 DEF_ASM(v28)
 DEF_ASM(v29)
 DEF_ASM(v30)
 DEF_ASM(v31)

/* SIMD/FP registers - 64-bit views (double) */
 DEF_ASM(d0)
 DEF_ASM(d1)
 DEF_ASM(d2)
 DEF_ASM(d3)
 DEF_ASM(d4)
 DEF_ASM(d5)
 DEF_ASM(d6)
 DEF_ASM(d7)
 DEF_ASM(d8)
 DEF_ASM(d9)
 DEF_ASM(d10)
 DEF_ASM(d11)
 DEF_ASM(d12)
 DEF_ASM(d13)
 DEF_ASM(d14)
 DEF_ASM(d15)
 DEF_ASM(d16)
 DEF_ASM(d17)
 DEF_ASM(d18)
 DEF_ASM(d19)
 DEF_ASM(d20)
 DEF_ASM(d21)
 DEF_ASM(d22)
 DEF_ASM(d23)
 DEF_ASM(d24)
 DEF_ASM(d25)
 DEF_ASM(d26)
 DEF_ASM(d27)
 DEF_ASM(d28)
 DEF_ASM(d29)
 DEF_ASM(d30)
 DEF_ASM(d31)

/* SIMD/FP registers - 32-bit views (single) */
 DEF_ASM(s0)
 DEF_ASM(s1)
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
 DEF_ASM(s12)
 DEF_ASM(s13)
 DEF_ASM(s14)
 DEF_ASM(s15)
 DEF_ASM(s16)
 DEF_ASM(s17)
 DEF_ASM(s18)
 DEF_ASM(s19)
 DEF_ASM(s20)
 DEF_ASM(s21)
 DEF_ASM(s22)
 DEF_ASM(s23)
 DEF_ASM(s24)
 DEF_ASM(s25)
 DEF_ASM(s26)
 DEF_ASM(s27)
 DEF_ASM(s28)
 DEF_ASM(s29)
 DEF_ASM(s30)
 DEF_ASM(s31)

/* SIMD/FP registers - 16-bit views (half) */
 DEF_ASM(h0)
 DEF_ASM(h1)
 DEF_ASM(h2)
 DEF_ASM(h3)
 DEF_ASM(h4)
 DEF_ASM(h5)
 DEF_ASM(h6)
 DEF_ASM(h7)
 DEF_ASM(h8)
 DEF_ASM(h9)
 DEF_ASM(h10)
 DEF_ASM(h11)
 DEF_ASM(h12)
 DEF_ASM(h13)
 DEF_ASM(h14)
 DEF_ASM(h15)
 DEF_ASM(h16)
 DEF_ASM(h17)
 DEF_ASM(h18)
 DEF_ASM(h19)
 DEF_ASM(h20)
 DEF_ASM(h21)
 DEF_ASM(h22)
 DEF_ASM(h23)
 DEF_ASM(h24)
 DEF_ASM(h25)
 DEF_ASM(h26)
 DEF_ASM(h27)
 DEF_ASM(h28)
 DEF_ASM(h29)
 DEF_ASM(h30)
 DEF_ASM(h31)

/* SIMD/FP registers - 8-bit views (byte) */
 DEF_ASM(b0)
 DEF_ASM(b1)
 DEF_ASM(b2)
 DEF_ASM(b3)
 DEF_ASM(b4)
 DEF_ASM(b5)
 DEF_ASM(b6)
 DEF_ASM(b7)
 DEF_ASM(b8)
 DEF_ASM(b9)
 DEF_ASM(b10)
 DEF_ASM(b11)
 DEF_ASM(b12)
 DEF_ASM(b13)
 DEF_ASM(b14)
 DEF_ASM(b15)
 DEF_ASM(b16)
 DEF_ASM(b17)
 DEF_ASM(b18)
 DEF_ASM(b19)
 DEF_ASM(b20)
 DEF_ASM(b21)
 DEF_ASM(b22)
 DEF_ASM(b23)
 DEF_ASM(b24)
 DEF_ASM(b25)
 DEF_ASM(b26)
 DEF_ASM(b27)
 DEF_ASM(b28)
 DEF_ASM(b29)
 DEF_ASM(b30)
 DEF_ASM(b31)

/* Condition codes */
 DEF_ASM(eq)
 DEF_ASM(ne)
 DEF_ASM(cs)
 DEF_ASM(hs)
 DEF_ASM(cc)
 DEF_ASM(lo)
 DEF_ASM(mi)
 DEF_ASM(pl)
 DEF_ASM(vs)
 DEF_ASM(vc)
 DEF_ASM(hi)
 DEF_ASM(ls)
 DEF_ASM(ge)
 DEF_ASM(lt)
 DEF_ASM(gt)
 DEF_ASM(le)
 DEF_ASM(al)

/* Data processing - arithmetic (no condition suffixes for ARM64) */
 DEF_ASM(add)
 DEF_ASM(adds)
 DEF_ASM(sub)
 DEF_ASM(subs)
 DEF_ASM(cmn)
 DEF_ASM(cmp)
 DEF_ASM(neg)
 DEF_ASM(negs)
 DEF_ASM(adc)
 DEF_ASM(adcs)
 DEF_ASM(sbc)
 DEF_ASM(sbcs)
 DEF_ASM(ngc)
 DEF_ASM(ngcs)

/* Data processing - bitwise */
 DEF_ASM(and)
 DEF_ASM(ands)
 DEF_ASM(bic)
 DEF_ASM(bics)
 DEF_ASM(orr)
 DEF_ASM(orn)
 DEF_ASM(eor)
 DEF_ASM(eon)
 DEF_ASM(mvn)
 DEF_ASM(mov)

/* Shifts */
 DEF_ASM(lsl)
 DEF_ASM(lsr)
 DEF_ASM(asr)
 DEF_ASM(ror)

/* Multiply/divide */
 DEF_ASM(mul)
 DEF_ASM(madd)
 DEF_ASM(msub)
 DEF_ASM(smaddl)
 DEF_ASM(smsubl)
 DEF_ASM(umaddl)
 DEF_ASM(umsubl)
 DEF_ASM(smulh)
 DEF_ASM(umulh)
 DEF_ASM(udiv)
 DEF_ASM(sdiv)

/* Moves */
 DEF_ASM(movz)
 DEF_ASM(movn)
 DEF_ASM(movk)

/* Compare/test */
 DEF_ASM(tst)
 DEF_ASM(teq)

/* Branch instructions */
 DEF_ASM(b)
 DEF_ASM(bl)
 DEF_ASM(br)
 DEF_ASM(blr)
 DEF_ASM(ret)
 DEF_ASM(cbz)
 DEF_ASM(cbnz)
 DEF_ASM(tbz)
 DEF_ASM(tbnz)

/* Conditional branches */
 DEF_ASM(beq)
 DEF_ASM(bne)
 DEF_ASM(bcs)
 DEF_ASM(bhs)
 DEF_ASM(bcc)
 DEF_ASM(blo)
 DEF_ASM(bmi)
 DEF_ASM(bpl)
 DEF_ASM(bvs)
 DEF_ASM(bvc)
 DEF_ASM(bhi)
 DEF_ASM(bls)
 DEF_ASM(bge)
 DEF_ASM(blt)
 DEF_ASM(bgt)
 DEF_ASM(ble)

/* Conditional select */
 DEF_ASM(csel)
 DEF_ASM(csinc)
 DEF_ASM(csinv)
 DEF_ASM(csneg)

/* Load/Store */
 DEF_ASM(ldr)
 DEF_ASM(ldrb)
 DEF_ASM(ldrh)
 DEF_ASM(ldrsb)
 DEF_ASM(ldrsh)
 DEF_ASM(ldrsw)
 DEF_ASM(str)
 DEF_ASM(strb)
 DEF_ASM(strh)

/* Load/Store - pair */
 DEF_ASM(ldp)
 DEF_ASM(stp)
 DEF_ASM(ldpsw)

/* Address generation */
 DEF_ASM(adr)
 DEF_ASM(adrp)

/* System instructions */
 DEF_ASM(mrs)
 DEF_ASM(msr)
 DEF_ASM(nop)
 DEF_ASM(wfi)
 DEF_ASM(wfit)
 DEF_ASM(wfe)
 DEF_ASM(wfet)
 DEF_ASM(sev)
 DEF_ASM(sevl)
 DEF_ASM(isb)
 DEF_ASM(dsb)
 DEF_ASM(dmb)

/* Hints */
 DEF_ASM(yield)
 DEF_ASM(clrex)

/* Push/pop */
 DEF_ASM(push)
 DEF_ASM(pop)

/* Floating point */
 DEF_ASM(fmov)
 DEF_ASM(fadd)
 DEF_ASM(fsub)
 DEF_ASM(fmul)
 DEF_ASM(fnmul)
 DEF_ASM(fdiv)
 DEF_ASM(fmax)
 DEF_ASM(fmin)
 DEF_ASM(fmaxnm)
 DEF_ASM(fminnm)
 DEF_ASM(fsqrt)
 DEF_ASM(fabs)
 DEF_ASM(fneg)
 DEF_ASM(frintn)
 DEF_ASM(frintp)
 DEF_ASM(frintm)
 DEF_ASM(frintz)
 DEF_ASM(frinta)
 DEF_ASM(frintx)
 DEF_ASM(frinti)
 DEF_ASM(fcmp)
 DEF_ASM(fcmpe)
 DEF_ASM(fccmp)
 DEF_ASM(fccmpe)
 DEF_ASM(fcvts)
 DEF_ASM(fcvtd)
 DEF_ASM(fcvth)
 DEF_ASM(fcvtx)
 DEF_ASM(scvtf)
 DEF_ASM(ucvtf)
 DEF_ASM(fcvtns)
 DEF_ASM(fcvtnu)
 DEF_ASM(fcvtps)
 DEF_ASM(fcvtpu)

/* SIMD instructions */
 DEF_ASM(addv)
 DEF_ASM(faddp)
 DEF_ASM(fmaxp)
 DEF_ASM(fminp)
 DEF_ASM(fmaxnmp)
 DEF_ASM(fminnmp)
 DEF_ASM(addp)
 DEF_ASM(bif)
 DEF_ASM(bit)
 DEF_ASM(bsl)
 DEF_ASM(dup)
 DEF_ASM(ext)
 DEF_ASM(ins)
 DEF_ASM(movi)
 DEF_ASM(mvni)
 DEF_ASM(not)
 DEF_ASM(shl)
 DEF_ASM(shll)
 DEF_ASM(shll2)
 DEF_ASM(sli)
 DEF_ASM(sri)
 DEF_ASM(sqshl)
 DEF_ASM(sqshlu)
 DEF_ASM(srshl)
 DEF_ASM(sshll)
 DEF_ASM(sshll2)
 DEF_ASM(sshr)
 DEF_ASM(ushll)
 DEF_ASM(ushll2)
 DEF_ASM(ushr)

/* Misc */
 DEF_ASM(bfm)
 DEF_ASM(sbfm)
 DEF_ASM(ubfm)
 DEF_ASM(extr)
 DEF_ASM(crc32b)
 DEF_ASM(crc32h)
 DEF_ASM(crc32w)
 DEF_ASM(crc32x)
 DEF_ASM(crc32cb)
 DEF_ASM(crc32ch)
 DEF_ASM(crc32cw)
 DEF_ASM(crc32cx)
 DEF_ASM(rev)
 DEF_ASM(rev16)
 DEF_ASM(rev32)
 DEF_ASM(rev64)
 DEF_ASM(clz)
 DEF_ASM(cls)
 DEF_ASM(rbit)

/* Exception generating */
 DEF_ASM(svc)
 DEF_ASM(hvc)
 DEF_ASM(smc)
 DEF_ASM(brk)
 DEF_ASM(hlt)
 DEF_ASM(dcps1)
 DEF_ASM(dcps2)
 DEF_ASM(dcps3)

/* Conditional branches */
 DEF_ASM(b_eq)
 DEF_ASM(b_ne)
 DEF_ASM(b_cs)
 DEF_ASM(b_cc)
 DEF_ASM(b_mi)
 DEF_ASM(b_pl)
 DEF_ASM(b_vs)
 DEF_ASM(b_vc)
 DEF_ASM(b_hi)
 DEF_ASM(b_ls)
 DEF_ASM(b_ge)
 DEF_ASM(b_lt)
 DEF_ASM(b_gt)
 DEF_ASM(b_le)

/* LD/ST exclusive */
 DEF_ASM(ldxr)
 DEF_ASM(ldxrb)
 DEF_ASM(ldxrh)
 DEF_ASM(stxr)
 DEF_ASM(stxrb)
 DEF_ASM(stxrh)
 DEF_ASM(ldaxr)
 DEF_ASM(ldaxrb)
 DEF_ASM(ldaxrh)
 DEF_ASM(stlxr)
 DEF_ASM(stlxrb)
 DEF_ASM(stlxrh)

/* LD/ST acquire-release */
 DEF_ASM(ldar)
 DEF_ASM(ldarb)
 DEF_ASM(ldarh)
 DEF_ASM(stlr)
 DEF_ASM(stlrb)
 DEF_ASM(stlrh)
 DEF_ASM(ldalr)
 DEF_ASM(ldalrb)
 DEF_ASM(ldalrh)
 DEF_ASM(stllr)
 DEF_ASM(stllrb)
 DEF_ASM(stllrh)

/* LD/ST unscaled immediate */
 DEF_ASM(ldur)
 DEF_ASM(ldurb)
 DEF_ASM(ldurh)
 DEF_ASM(ldursb)
 DEF_ASM(ldursh)
 DEF_ASM(ldursw)
 DEF_ASM(stur)
 DEF_ASM(sturb)
 DEF_ASM(sturh)

/* Vector load/store */
 DEF_ASM(ld1)
 DEF_ASM(st1)
 DEF_ASM(ld2)
 DEF_ASM(st2)
 DEF_ASM(ld3)
 DEF_ASM(st3)
 DEF_ASM(ld4)
 DEF_ASM(st4)

/* ------------------------------------------------------------------ */
/* ARM64 instruction opcode constants and encoding helpers           */
/* ------------------------------------------------------------------ */

/* Data processing - immediate */
#define ARM64_ADD_IMM     0x11000000U
#define ARM64_ADDS_IMM    0x2B000000U
#define ARM64_SUB_IMM     0x51000000U
#define ARM64_SUBS_IMM    0x6B000000U

/* Data processing - register */
#define ARM64_ADD_REG     0x0B000000U
#define ARM64_ADDS_REG    0x2B000000U
#define ARM64_SUB_REG     0x4B000000U
#define ARM64_SUBS_REG    0x6B000000U
#define ARM64_ADD_EXT     0x0B200000U  /* ADD (extended register): Rn 31 is SP */
#define ARM64_AND_REG     0x0A000000U
#define ARM64_ANDS_REG    0x6A000000U
#define ARM64_ORR_REG     0x2A000000U
#define ARM64_EOR_REG     0x4A000000U
#define ARM64_MUL_REG     0x1B000000U  /* Base opcode, Rm/Rn/Rd must be filled in */

/* Move wide immediate */
#define ARM64_MOVZ        0x52800000U
#define ARM64_MOVN        0x12800000U
#define ARM64_MOVK        0xF2800000U
/* ARM64_MOVI_W/X removed: MOVI is a SIMD&FP instruction, not general-purpose */
/* Use MOVZ/MOVN/MOVK for general-purpose, or SIMD MOVI variants (0x0F000400, etc.) */

/* MOVZ/MOVN 64-bit base opcodes */
#define ARM64_MOVZ64      0xD2800000U  /* MOVZ (64-bit), use with ARM64_HW() */
#define ARM64_MOVN64      0x92800000U  /* MOVN (64-bit), use with ARM64_HW() */

/* Move wide immediate shift field (LSL #0/16/32/48 encoded as hw*16) */
#define ARM64_HW(v)       (((uint32_t)(v) & 3) << 21)

/* Load/store register (unsigned immediate) */
#define ARM64_LDR_X       0xF9400000U
#define ARM64_LDR_W       0xB9400000U
#define ARM64_LDR_B       0x39400000U
#define ARM64_LDR_H       0x79400000U
#define ARM64_LDR_D       0xFD400000U
#define ARM64_LDR_S       0xBD400000U
#define ARM64_STR_X       0xF9000000U
#define ARM64_STR_W       0xB9000000U
#define ARM64_STR_B       0x39000000U
#define ARM64_STR_H       0x79000000U
#define ARM64_STR_D       0xFD000000U
#define ARM64_STR_S       0xBD000000U

/* Load/store register (unscaled immediate) */
#define ARM64_LDUR_X      0xF8400000U
#define ARM64_LDUR_W      0xB8400000U
#define ARM64_LDUR_B      0x38400000U
#define ARM64_LDUR_H      0x78400000U
#define ARM64_STUR_X      0xF8000000U
#define ARM64_STUR_W      0xB8000000U
#define ARM64_STUR_B      0x38000000U
#define ARM64_STUR_H      0x78000000U

/* Load/store register (register offset) */
#define ARM64_LDR_X_REG   0xF8606800U
#define ARM64_LDR_W_REG   0xB8606800U
#define ARM64_LDR_B_REG   0x38606800U
#define ARM64_LDR_H_REG   0x78606800U
#define ARM64_STR_X_REG   0xF8206800U
#define ARM64_STR_W_REG   0xB8206800U
#define ARM64_STR_B_REG   0x38206800U
#define ARM64_STR_H_REG   0x78206800U

/* Load/store (pre/post-indexed) */
#define ARM64_STR_X_PRE   0xF8000000U  /* STR X pre-indexed base */
#define ARM64_LDR_X_POST  0xF8400000U  /* LDR X post-indexed base */

/* SIMD load/store (unsigned immediate) */
#define ARM64_LDR_SCALAR  0x3D400000U  /* Base for scalar load (size built dynamically) */
#define ARM64_LDR_S_VEC   0xBD400000U
#define ARM64_LDR_D_VEC   0xFD400000U
#define ARM64_LDR_Q_VEC   0x3DC00000U
#define ARM64_STR_SCALAR  0x3D000000U  /* Base for scalar store (size built dynamically) */
#define ARM64_STR_S_VEC   0xBD000000U
#define ARM64_STR_D_VEC   0xFD000000U
#define ARM64_STR_Q_VEC   0x3D800000U

/* SIMD load/store (unscaled immediate) */
#define ARM64_LDUR_S_SIMD 0xBC400000U
#define ARM64_LDUR_D_SIMD 0xFC400000U
#define ARM64_LDUR_Q_SIMD 0x3C400000U
#define ARM64_STUR_S_SIMD 0xBC000000U
#define ARM64_STUR_D_SIMD 0xFC000000U
#define ARM64_STUR_Q_SIMD 0x3C000000U

/* SIMD load/store (register offset) */
#define ARM64_LDR_S_REG   0xBC606800U
#define ARM64_LDR_D_REG   0xFC606800U
#define ARM64_LDR_Q_REG   0x3C606800U
#define ARM64_STR_S_REG   0xBC206800U
#define ARM64_STR_D_REG   0xFC206800U
#define ARM64_STR_Q_REG   0x3C206800U

/* Load/store pair */
#define ARM64_LDP_X       0xA9400000U
#define ARM64_LDP_X_PRE   0xA9C00000U
#define ARM64_LDP_X_POST  0xA8C00000U
#define ARM64_STP_X       0xA9000000U
#define ARM64_STP_X_PRE   0xA9800000U
#define ARM64_STP_X_POST  0xA8800000U
#define ARM64_LDP_D       0x6D400000U
#define ARM64_LDP_D_PRE   0x6DC00000U
#define ARM64_LDP_D_POST  0x6CC00000U
#define ARM64_STP_D       0x6D000000U
#define ARM64_STP_D_PRE   0x6D800000U
#define ARM64_STP_D_POST  0x6C800000U

/* Branch instructions */
#define ARM64_B           0x14000000U
#define ARM64_BL          0x94000000U
#define ARM64_BR          0xD61F0000U
#define ARM64_BLR         0xD63F0000U
#define ARM64_RET         0xD65F0000U

/* Conditional branch */
#define ARM64_B_COND      0x54000000U

/* Compare and branch */
#define ARM64_CBZ         0x34000000U
#define ARM64_CBNZ        0x35000000U

/* System instructions */
#define ARM64_NOP         0xD503201FU
#define ARM64_ISB         0xD50330DFU
#define ARM64_DSB         0xD503309FU
#define ARM64_DMB         0xD50330BFU
#define ARM64_MRS         0xD5380000U
#define ARM64_MSR         0xD5180000U
#define ARM64_MRS_FPCR    0xD53B4400U
#define ARM64_MRS_FPSR    0xD53B4420U
#define ARM64_MSR_FPCR    0xD51B4400U
#define ARM64_MSR_FPSR    0xD51B4420U
#define ARM64_WFE         0xD503205FU
#define ARM64_WFET        0xD5031000U
#define ARM64_WFI         0xD503207FU
#define ARM64_WFIT        0xD5031020U

/* Shifts (register) */
#define ARM64_LSL_REG     0x1AC02000U
#define ARM64_LSR_REG     0x1AC02400U
#define ARM64_ASR_REG     0x1AC02800U
#define ARM64_ROR_REG     0x1AC02C00U

/* Shifts (immediate - UBFM/SBFM) */
#define ARM64_LSL_IMM     0xD3400000U
#define ARM64_LSR_IMM     0xD3400000U
#define ARM64_LSR_IMM_32  0x53000000U  /* 32-bit LSR base */
#define ARM64_ASR_IMM     0x93400000U

/* Shifted register encoding for ORR/AND/EOR */
#define ARM64_SHIFT_LSL(imm)  (((uint32_t)(imm) & 63) << 10)
#define ARM64_SHIFT_LSR(imm)  (0x00200000U | (((uint32_t)(imm) & 63) << 10))
#define ARM64_SHIFT_ASR(imm)  (0x00400000U | (((uint32_t)(imm) & 63) << 10))
#define ARM64_SHIFT_ROR(imm)  (0x00600000U | (((uint32_t)(imm) & 63) << 10))

/* UBFM/SBFM immediate fields (for LSL/LSR/ASR immediate aliases) */
#define ARM64_IMM_R(r)        (((uint32_t)(r) & 0x3F) << 16)
#define ARM64_IMM_S(s)        (((uint32_t)(s) & 0x3F) << 10)

/* Extended register encoding */
#define ARM64_EXTEND_LSL(lsl) (((uint32_t)(lsl) & 7) << 10)
#define ARM64_EXTEND_UXTX     (3U << 13)

/* MOV (register) - ORR with zero register */
#define ARM64_MOV_REG     0x2A0003E0U

/* Address generation */
#define ARM64_ADRP        0x90000000U
#define ARM64_ADR         0x10000000U

/* Logical immediate */
#define ARM64_AND_IMM     0x12000000U
#define ARM64_ORR_IMM_BASE 0x32000000U
#define ARM64_EOR_IMM     0x52000000U
#define ARM64_ANDS_IMM    0x72000000U
#define ARM64_ORR_IMM     0x320003E0U  /* ORR immediate alias with Rn = XZR/WZR */

/* ------------------------------------------------------------------ */
/* ARM64 instruction encoding helper macros                           */
/* ------------------------------------------------------------------ */

/* Register field encodings */
#define ARM64_RD(r)     ((uint32_t)(r) & 0x1FU)
#define ARM64_RN(r)     (((uint32_t)(r) & 0x1FU) << 5)
#define ARM64_RM(r)     (((uint32_t)(r) & 0x1FU) << 16)
#define ARM64_RT(r)     ((uint32_t)(r) & 0x1FU)
#define ARM64_RT2(r)    (((uint32_t)(r) & 0x1FU) << 10)

/* Immediate field encodings */
#define ARM64_IMM12(v)  (((uint32_t)(v) & 0xFFFU) << 10)
#define ARM64_IMM7(v)   (((uint32_t)(v) & 0x7FU) << 15)
#define ARM64_IMM14(v)  (((uint32_t)(v) & 0x3FFFU) << 5)
#define ARM64_IMM16(v)  (((uint32_t)(v) & 0xFFFFU) << 5)
#define ARM64_IMM_HW(v, hw) (((uint32_t)(v) & 0xFFFFU) << 5 | (((hw) & 3) << 21))

/* Shift and size encodings */
#define ARM64_SIZE(s)   (((uint32_t)(s) & 3) << 30)
#define ARM64_SF(s)     (((uint32_t)(s) & 1) << 31)
#define ARM64_S(v)      (((uint32_t)(v) & 1) << 29)
#define ARM64_N(v)      (((uint32_t)(v) & 1) << 22)
#define ARM64_SH(v)     (((uint32_t)(v) & 1) << 22)

/* Condition code encoding */
#define ARM64_COND(c)   ((uint32_t)(c) & 0xFU)

/* Branch offset encoding */
#define ARM64_OFFSET26(v) (((uint32_t)(v) >> 2) & 0x3FFFFFFU)
#define ARM64_OFFSET19(v) (((uint32_t)(v) >> 2) & 0x7FFFFU)
#define ARM64_OFFSET14(v) (((uint32_t)(v) >> 2) & 0x3FFFU)

/* Special register field (for MRS/MSR) */
#undef ARM64_SYSREG
#define ARM64_SYSREG(op0, op1, crn, crm, op2) \
    ((((op0) & 3) << 19) | (((op1) & 7) << 16) | \
     (((crn) & 15) << 12) | (((crm) & 15) << 8) | (((op2) & 7) << 5))

/* Barrier option encoding */
#define ARM64_ISB_OPTION(opt) (((uint32_t)(opt) & 0xFU) << 8)
#define ARM64_DSB_OPTION(opt) (((uint32_t)(opt) & 0xFU) << 8)
#define ARM64_DMB_OPTION(opt) (((uint32_t)(opt) & 0xFU) << 8)

/* Additional opcodes for code generator - VERIFIED */
/* Note: Many of these are specific instances, not general templates */

/* Floating-point move - VERIFIED */
#define ARM64_FMOV_D_S    0x1E604000U  /* FMOV Dd,Dn (scalar) */
#define ARM64_FMOV_X_D    0x9E660000U  /* FMOV Xd,Dn (general to FP) */
#define ARM64_FMOV_W_S    0x1E260000U  /* FMOV Wd,Sn (general to FP) */
/* ARM64_FMOV_S_D removed: 0x4EA01C00 is SIMD vector, not scalar FMOV */
/* Use 0x1E204000 for FMOV Sd,Sn or 0x1E604000 variant for cross-size */

/* FMOV variants for code generator */
#define ARM64_FMOV_SCALAR 0x1E604000U  /* FMOV Dd, Dn (scalar FP) */
#define ARM64_FMOV_XD     0x9E660000U  /* FMOV Xd, Dn (FP to GP 64-bit) */
#define ARM64_FMOV_WS     0x1E260000U  /* FMOV Wd, Sn (FP to GP 32-bit) */

/* MOV vector (ORR vector register alias) */
#define ARM64_MOV_V16B    0x4EA01C00U  /* MOV Vd.16B, Vn.16B (ORR vector, Rm=Rn alias) */

/* Load/Store SIMD&FP - Base opcodes (register fields must be filled in) */
#define ARM64_STR_Q_PRE   0x3C800000U  /* STR Q pre-index base */
#define ARM64_LDR_Q_POST  0x3CC00000U  /* LDR Q post-index base */

/* LDPSW - Base opcode (register fields must be filled in) */
/* Use gen_ldst_pair() with appropriate mode for LDPSW */
/* Base encodings: 0x68C00000 (post), 0x69400000 (offset), 0x69C00000 (pre) */

/* ARM64_LDR_S_SIMD removed: 0x0D00801C is not a standard encoding */
/* Use ARM64_LDR_S (0xBD400000) for scalar S or ARM64_LDR_S_VEC for SIMD */

/* MOV between SIMD and general - Use UMOV/SMOV instead */
/* ARM64_MOV_V_D removed: 0x4E083C00 is UMOV/SMOV encoding */
/* Use appropriate UMOV/SMOV base: 0x0E002C00/0x0E003C00 (32-bit) */
/* or 0x4E002C00/0x4E003C00 (64-bit) */

/* Verified from previous section */
#define ARM64_FCMP        0x1E202008U  /* FCMP with zero */
#define ARM64_SDIV        0x1AC00C00U  /* SDIV (32-bit) */

/* EXTR (Extract) */
#define ARM64_EXTR        0x13800000U  /* EXTR Wd, Wn, Wm, #imm (32-bit) */
#define ARM64_EXTR64      0x93C00000U  /* EXTR Xd, Xn, Xm, #imm (64-bit) */

/* ARM64_MUL removed - use ARM64_MUL_REG with gen_dp_reg() */

/* ORR shifted - Base opcodes (register fields must be filled in) */
#define ARM64_ORR_REG_LSL 0x2A000000U  /* ORR (shifted register) base */
/* ARM64_ORR_REG_LSL32 removed: use ARM64_ORR_REG_LSL with SF=1 */
/* ARM64_ORR_REG_MOV is duplicate of ARM64_MOV_REG */

/* LSR immediate - These are UBFM encodings, use gen_shift() instead */
/* Base UBFM encodings: 0x53000000 (W), 0xD3400000 (X) */
/* gen_shift() handles immr/imms encoding for LSR/LSL/ASR */
/* ARM64_LSR_W_8, ARM64_LSR_X_8, ARM64_LSR_X_16, ARM64_LSR_X_24 removed */
/* They are specific instances, not templates */

/* SUB shifted - Base opcode (use gen_sub_reg or asm handler) */
#define ARM64_SUB_REG_LSL 0xCB000000U  /* SUB (shifted register) base */

/* Duplicates removed: ARM64_LDP_X, ARM64_B, ARM64_BL, ARM64_BR, ARM64_NOP */
/* These are already defined in their respective sections above */
