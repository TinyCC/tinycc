#ifdef TARGET_DEFS_ONLY

// Number of registers available to allocator:
#define NB_REGS 16 // x10-x17 aka a0-a7, f10-f17 aka fa0-fa7

#define TREG_R(x) (x) // x = 0..7
#define TREG_F(x) (x + 8) // x = 0..7

// Register classes sorted from more general to more precise:
#define RC_INT (1 << 0)
#define RC_FLOAT (1 << 1)
#define RC_R(x) (1 << (2 + (x))) // x = 0..7
#define RC_F(x) (1 << (10 + (x))) // x = 0..7

#define RC_IRET (RC_R(0)) // int return register class
#define RC_FRET (RC_F(0)) // float return register class

#define REG_IRET (TREG_R(0)) // int return register number
#define REG_FRET (TREG_F(0)) // float return register number

#define PTR_SIZE 8

#define LDOUBLE_SIZE 16
#define LDOUBLE_ALIGN 16

#define MAX_ALIGN 16

#define CHAR_IS_UNSIGNED

#else
#include "tcc.h"
#include <assert.h>

#define XLEN 8

#define TREG_RA 17
#define TREG_SP 18

ST_DATA const int reg_classes[NB_REGS] = {
  RC_INT | RC_R(0),
  RC_INT | RC_R(1),
  RC_INT | RC_R(2),
  RC_INT | RC_R(3),
  RC_INT | RC_R(4),
  RC_INT | RC_R(5),
  RC_INT | RC_R(6),
  RC_INT | RC_R(7),
  RC_FLOAT | RC_F(0),
  RC_FLOAT | RC_F(1),
  RC_FLOAT | RC_F(2),
  RC_FLOAT | RC_F(3),
  RC_FLOAT | RC_F(4),
  RC_FLOAT | RC_F(5),
  RC_FLOAT | RC_F(6),
  RC_FLOAT | RC_F(7)
};

static int ireg(int r)
{
    if (r == TREG_RA)
      return 1; // ra
    if (r == TREG_SP)
      return 2; // sp
    assert(r >= 0 && r < 8);
    return r + 10;  // tccrX --> aX == x(10+X)
}

static int is_ireg(int r)
{
    return r < 8 || r == TREG_RA || r == TREG_SP;
}

static int freg(int r)
{
    assert(r >= 8 && r < 16);
    return r - 8 + 10;  // tccfX --> faX == f(10+X)
}

static int is_freg(int r)
{
    return r >= 8 && r < 16;
}

ST_FUNC void o(unsigned int c)
{
    int ind1 = ind + 4;
    if (nocode_wanted)
        return;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    write32le(cur_text_section->data + ind, c);
    ind = ind1;
}

static void EI(uint32_t opcode, uint32_t func3,
               uint32_t rd, uint32_t rs1, uint32_t imm)
{
    assert(! ((imm + (1 << 11)) >> 12));
    o(opcode | (func3 << 12) | (rd << 7) | (rs1 << 15) | (imm << 20));
}

static void ES(uint32_t opcode, uint32_t func3,
               uint32_t rs1, uint32_t rs2, uint32_t imm)
{
    assert(! ((imm + (1 << 11)) >> 12));
    o(opcode | (func3 << 12) | ((imm & 0x1f) << 7) | (rs1 << 15)
      | (rs2 << 20) | ((imm >> 5) << 25));
}

// Patch all branches in list pointed to by t to branch to a:
ST_FUNC void gsym_addr(int t_, int a_)
{
    uint32_t t = t_;
    uint32_t a = a_;
    while (t) {
        unsigned char *ptr = cur_text_section->data + t;
        uint32_t next = read32le(ptr);
        uint32_t r = a - t, imm;
        if ((r + (1 << 21)) & ~((1U << 22) - 2))
          tcc_error("out-of-range branch chain");
        imm =   (((r >> 12) &  0xff) << 12)
            | (((r >> 11) &     1) << 20)
            | (((r >>  1) & 0x3ff) << 21)
            | (((r >> 20) &     1) << 31);
        write32le(ptr, r == 4 ? 0x33 : 0x6f | imm); // nop || j imm
        t = next;
    }
}

ST_FUNC void load(int r, SValue *sv)
{
    int fr = sv->r;
    int v = fr & VT_VALMASK;
    int rr = is_ireg(r) ? ireg(r) : freg(r);
    int fc = sv->c.i;
    int bt = sv->type.t & VT_BTYPE;
    int align, size = type_size(&sv->type, &align);
    if (fr & VT_LVAL) {
        int func3, opcode = 0x03;
        if (is_freg(r)) {
            assert(bt == VT_DOUBLE || bt == VT_FLOAT);
            opcode = 0x07;
            func3 = bt == VT_DOUBLE ? 3 : 2;
        } else {
            assert(is_ireg(r));
            if (bt == VT_FUNC)
              size = PTR_SIZE;
            func3 = size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : 3;
            if (size < 8 && !is_float(sv->type.t) && (sv->type.t & VT_UNSIGNED))
              func3 |= 4;
        }
        if (v == VT_LOCAL) {
            if (((unsigned)fc + (1 << 11)) >> 12)
              tcc_error("unimp: load(large local ofs) (0x%x)", fc);
            EI(opcode, func3, rr, 8, fc); // l[bhwd][u]/fl[wd] RR, fc(s0)
        } else if (v < VT_CONST) {
            /*if (((unsigned)fc + (1 << 11)) >> 12)
              tcc_error("unimp: load(large addend) (0x%x)", fc);*/
            fc = 0; // XXX store ofs in LVAL(reg)
            EI(opcode, func3, rr, ireg(v), fc); // l[bhwd][u] RR, 0(V)
        } else if (v == VT_CONST && (fr & VT_SYM)) {
            static Sym label;
            int addend = 0, tempr;
            if (1 || ((unsigned)fc + (1 << 11)) >> 12)
              addend = fc, fc = 0;

            greloca(cur_text_section, sv->sym, ind,
                    R_RISCV_PCREL_HI20, addend);
            if (!label.v) {
                label.v = tok_alloc(".L0 ", 4)->tok;
                label.type.t = VT_VOID | VT_STATIC;
            }
            label.c = 0; /* force new local ELF symbol */
            put_extern_sym(&label, cur_text_section, ind, 0);
            tempr = is_ireg(r) ? rr : ireg(get_reg(RC_INT));
            o(0x17 | (tempr << 7));   // auipc TR, 0 %pcrel_hi(sym)+addend
            greloca(cur_text_section, &label, ind,
                    R_RISCV_PCREL_LO12_I, 0);
            EI(opcode, func3, rr, tempr, fc); // l[bhwd][u] RR, fc(TR)
        } else if (v == VT_LLOCAL) {
            int tempr = rr;
            if (((unsigned)fc + (1 << 11)) >> 12)
              tcc_error("unimp: load(large local ofs) (0x%x)", fc);
            if (!is_ireg(r))
              tempr = ireg(get_reg(RC_INT));
            EI(0x03, 3, tempr, 8, fc); // ld TEMPR, fc(s0)
            EI(opcode, func3, rr, tempr, 0); // l[bhwd][u] RR, 0(TEMPR)
        } else {
            tcc_error("unimp: load(non-local lval)");
        }
    } else if (v == VT_CONST) {
        int rb = 0, do32bit = 8;
        assert(!is_float(sv->type.t) && is_ireg(r));
        if (fr & VT_SYM) {
            static Sym label;
            greloca(cur_text_section, sv->sym, ind,
                    R_RISCV_PCREL_HI20, sv->c.i);
            if (!label.v) {
                label.v = tok_alloc(".L0 ", 4)->tok;
                label.type.t = VT_VOID | VT_STATIC;
            }
            label.c = 0; /* force new local ELF symbol */
            put_extern_sym(&label, cur_text_section, ind, 0);
            o(0x17 | (rr << 7));   // auipc RR, 0 %call(func)
            greloca(cur_text_section, &label, ind,
                    R_RISCV_PCREL_LO12_I, 0);
            rb = rr;
            fc = 0;
            sv->c.i = 0;
            do32bit = 0;
        }
        if (is_float(sv->type.t))
          tcc_error("unimp: load(float)");
        if (fc != sv->c.i) {
            int64_t si = sv->c.i;
            uint32_t pi;
            si >>= 32;
            if (si != 0) {
                pi = si;
                if (fc < 0)
                  pi++;
                o(0x37 | (rr << 7) | (((pi + 0x800) & 0xfffff000))); // lui RR, up(up(fc))
                EI(0x13, 0, rr, rr, (int)pi << 20 >> 20);   // addi RR, RR, lo(up(fc))
                EI(0x13, 1, rr, rr, 12); // slli RR, RR, 12
                EI(0x13, 0, rr, rr, (fc + (1 << 19)) >> 20);  // addi RR, RR, up(lo(fc))
                EI(0x13, 1, rr, rr, 12); // slli RR, RR, 12
                fc = fc << 12 >> 12;
                EI(0x13, 0, rr, rr, fc >> 8);  // addi RR, RR, lo1(lo(fc))
                EI(0x13, 1, rr, rr, 8); // slli RR, RR, 8
                fc &= 0xff;
                rb = rr;
                do32bit = 0;
            } else {
                /* A 32bit unsigned constant.  lui always sign extends, so we need
                   tricks.  */
                pi = (uint32_t)sv->c.i;
                o(0x37 | (rr << 7) | (((pi + 0x80000) & 0xfff00000) >> 8)); // lui RR, up(fc)>>8
                EI(0x13, 0, rr, rr,  (((pi + 0x200) & 0x000ffc00) >> 8) | (-((int)(pi + 0x200) & 0x80000) >> 8)); // addi RR, RR, mid(fc)
                EI(0x13, 1, rr, rr, 8); // slli RR, RR, 8
                fc = (pi & 0x3ff) | (-((int)(pi & 0x200)));
                rb = rr;
                do32bit = 0;
            }
        }
        if (((unsigned)fc + (1 << 11)) >> 12)
            o(0x37 | (rr << 7) | ((0x800 + fc) & 0xfffff000)), rb = rr; //lui RR, upper(fc)
        EI(0x13 | do32bit, 0, rr, rb, fc << 20 >> 20);   // addi[w] R, x0|R, FC
    } else if (v == VT_LOCAL) {
        assert(is_ireg(r));
        if (((unsigned)fc + (1 << 11)) >> 12)
          tcc_error("unimp: load(addr large local ofs) (0x%x)", fc);
        EI(0x13, 0, rr, 8, fc); // addi R, s0, FC
    } else if (v < VT_CONST) {
        /* reg-reg */
        //assert(!fc); XXX support offseted regs
        if (is_freg(r) && is_freg(v))
          o(0x53 | (rr << 7) | (freg(v) << 15) | (freg(v) << 20) | ((bt == VT_DOUBLE ? 0x11 : 0x10) << 25)); //fsgnj.[sd] RR, V, V == fmv.[sd] RR, V
        else if (is_ireg(r) && is_ireg(v))
          EI(0x13, 0, rr, ireg(v), 0); // addi RR, V, 0 == mv RR, V
        else {
            int func7 = is_ireg(r) ? 0x70 : 0x78;
            if (size == 8)
              func7 |= 1;
            assert(size == 4 || size == 8);
            o(0x53 | (rr << 7) | ((is_freg(v) ? freg(v) : ireg(v)) << 15)
              | (func7 << 25)); // fmv.{w.x, x.w, d.x, x.d} RR, VR
        }
    } else if (v == VT_CMP) {  // we rely on cmp_r to be the correct result
        EI(0x13, 0, rr, vtop->cmp_r, 0); // mv RR, CMP_R
    } else if ((v & ~1) == VT_JMP) {
        int t = v & 1;
        assert(is_ireg(r));
        EI(0x13, 0, rr, 0, t);      // addi RR, x0, t
        gjmp_addr(ind + 8);
        gsym(fc);
        EI(0x13, 0, rr, 0, t ^ 1);  // addi RR, x0, !t
    } else
      tcc_error("unimp: load(non-const)");
}

ST_FUNC void store(int r, SValue *sv)
{
    int fr = sv->r & VT_VALMASK;
    int rr = is_ireg(r) ? ireg(r) : freg(r);
    int fc = sv->c.i;
    int ft = sv->type.t;
    int bt = ft & VT_BTYPE;
    int align, size = type_size(&sv->type, &align);
    assert(!is_float(bt) || is_freg(r));
    if (bt == VT_STRUCT)
      tcc_error("unimp: store(struct)");
    if (size > 8)
      tcc_error("unimp: large sized store");
    assert(sv->r & VT_LVAL);
    if (fr == VT_LOCAL) {
        if (((unsigned)fc + (1 << 11)) >> 12)
          tcc_error("unimp: store(large local off) (0x%x)", fc);
        if (is_freg(r))
          ES(0x27, size == 4 ? 2 : 3, 8, rr, fc); // fs[wd] RR, fc(s0)
        else
          ES(0x23, size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : 3,
             8, rr, fc); // s[bhwd] RR, fc(s0)
    } else if (fr < VT_CONST) {
        int ptrreg = ireg(fr);
        /*if (((unsigned)fc + (1 << 11)) >> 12)
          tcc_error("unimp: store(large addend) (0x%x)", fc);*/
        fc = 0; // XXX support offsets regs
        if (is_freg(r))
          ES(0x27, size == 4 ? 2 : 3, ptrreg, rr, fc); // fs[wd] RR, fc(PTRREG)
        else
          ES(0x23, size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : 3,
             ptrreg, rr, fc); // s[bhwd] RR, fc(PTRREG)
    } else if (sv->r == (VT_CONST | VT_SYM | VT_LVAL)) {
        static Sym label;
        int tempr, addend = 0;
        if (1 || ((unsigned)fc + (1 << 11)) >> 12)
          addend = fc, fc = 0;

        tempr = ireg(get_reg(RC_INT));
        greloca(cur_text_section, sv->sym, ind,
                R_RISCV_PCREL_HI20, addend);
        if (!label.v) {
            label.v = tok_alloc(".L0 ", 4)->tok;
            label.type.t = VT_VOID | VT_STATIC;
        }
        label.c = 0; /* force new local ELF symbol */
        put_extern_sym(&label, cur_text_section, ind, 0);
        o(0x17 | (tempr << 7));   // auipc TEMPR, 0 %pcrel_hi(sym)+addend
        greloca(cur_text_section, &label, ind,
                R_RISCV_PCREL_LO12_S, 0);
        if (is_freg(r))
          ES(0x27, size == 4 ? 2 : 3, tempr, rr, fc); // fs[wd] RR, fc(TEMPR)
        else
          ES(0x23, size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : 3,
             tempr, rr, fc); // s[bhwd] RR, fc(TEMPR)
    } else
      tcc_error("implement me: %s(!local)", __FUNCTION__);
}

static void gcall(void)
{
    if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST &&
        ((vtop->r & VT_SYM) && vtop->c.i == (int)vtop->c.i)) {
        /* constant symbolic case -> simple relocation */
        greloca(cur_text_section, vtop->sym, ind,
                R_RISCV_CALL_PLT, (int)vtop->c.i);
        o(0x17 | (1 << 7));   // auipc ra, 0 %call(func)
        o(0x80e7);             // jalr  ra, 0 %call(func)
    } else if ((vtop->r & VT_VALMASK) < VT_CONST) {
        int r = ireg(vtop->r & VT_VALMASK);
        EI(0x67, 0, 1, r, 0);      // jalr ra, 0(R)
    } else {
        int r = TREG_RA;
        load(r, vtop);
        r = ireg(r);
        EI(0x67, 0, 1, r, 0);      // jalr ra, 0(R)
    }
}

ST_FUNC void gfunc_call(int nb_args)
{
    int i, align, size, aireg, afreg;
    int info[nb_args ? nb_args : 1];
    int stack_adj = 0, ofs;
    int force_stack = 0;
    SValue *sv;
    Sym *sa;
    aireg = afreg = 0;
    sa = vtop[-nb_args].type.ref->next;
    for (i = 0; i < nb_args; i++) {
        int *pareg, nregs, infreg = 0;
        sv = &vtop[1 + i - nb_args];
        sv->type.t &= ~VT_ARRAY; // XXX this should be done in tccgen.c
        size = type_size(&sv->type, &align);
        if ((size > 8 && ((sv->type.t & VT_BTYPE) != VT_LDOUBLE))
            || ((sv->type.t & VT_BTYPE) == VT_STRUCT))
          tcc_error("unimp: call arg %d wrong type", nb_args - i);
        nregs = 1;
        if ((sv->type.t & VT_BTYPE) == VT_LDOUBLE) {
          infreg = 0, nregs = 2;
          if (!sa) {
              aireg = (aireg + 1) & ~1;
          }
        } else
          infreg = sa && is_float(sv->type.t);
        pareg = infreg ? &afreg : &aireg;
        if ((*pareg < 8) && !force_stack) {
            info[i] = *pareg + (infreg ? 8 : 0);
            (*pareg)++;
            if (nregs == 1)
              ;
            else if (*pareg < 8)
              (*pareg)++;
            else {
                info[i] |= 16;
                stack_adj += 8;
                tcc_error("unimp: param passing half in reg, half on stack");
            }
        } else {
            info[i] = 32;
            stack_adj += (size + align - 1) & -align;
            if (!sa)
              force_stack = 1;
        }
        if (sa)
          sa = sa->next;
    }
    stack_adj = (stack_adj + 15) & -16;
    if (stack_adj) {
        EI(0x13, 0, 2, 2, -stack_adj);      // addi sp, sp, -adj
        for (i = ofs = 0; i < nb_args; i++) {
            if (1 && info[i] >= 32) {
                vrotb(nb_args - i);
                size = type_size(&vtop->type, &align);
                /* Once we support offseted regs we can do this:
                     vset(&vtop->type, TREG_SP | VT_LVAL, ofs);
                   to construct the lvalue for the outgoing stack slot,
                   until then we have to jump through hoops.  */
                vset(&char_pointer_type, TREG_SP, 0);
                vpushi(ofs);
                gen_op('+');
                indir();
                vtop->type = vtop[-1].type;
                vswap();
                vstore();
                vrott(nb_args - i);
                ofs += (size + align - 1) & -align;
                ofs = (ofs + 7) & -8;
            }
        }
    }
    for (i = 0; i < nb_args; i++) {
        int r = info[nb_args - 1 - i];
        if (r < 32) {
            r &= 15;
            vrotb(i+1);
            gv(r < 8 ? RC_R(r) : RC_F(r - 8));
            if (vtop->r2 < VT_CONST) {
                assert((vtop->type.t & VT_BTYPE) == VT_LDOUBLE);
                assert(vtop->r < 7);
                if (vtop->r2 != 1 + vtop->r) {
                    /* XXX we'd like to have 'gv' move directly into
                       the right class instead of us fixing it up.  */
                    EI(0x13, 0, ireg(vtop->r) + 1, ireg(vtop->r2), 0); // mv Ra+1, RR2
                    vtop->r2 = 1 + vtop->r;
                }
            }
            vrott(i+1);
        }
    }
    vrotb(nb_args + 1);
    gcall();
    vtop -= nb_args + 1;
    if (stack_adj)
      EI(0x13, 0, 2, 2, stack_adj);      // addi sp, sp, adj
}

static int func_sub_sp_offset;

ST_FUNC void gfunc_prolog(CType *func_type)
{
    int i, addr, align, size;
    int param_addr = 0;
    int aireg, afreg;
    Sym *sym;
    CType *type;

    sym = func_type->ref;
    func_vt = sym->type;
    loc = -16; // for ra and s0
    func_sub_sp_offset = ind;
    ind += 4 * 4;
    if (sym->f.func_type == FUNC_ELLIPSIS) {
        tcc_error("unimp: vararg prologue");
    }

    aireg = afreg = 0;
    addr = 0; // XXX not correct
    /* if the function returns a structure, then add an
       implicit pointer parameter */
    size = type_size(&func_vt, &align);
    if (size > 2 * XLEN) {
        tcc_error("unimp: struct return");
        func_vc = loc;
    }
    /* define parameters */
    while ((sym = sym->next) != NULL) {
        type = &sym->type;
        size = type_size(type, &align);
        if (size > 2 * XLEN) {
          from_stack:
            addr = (addr + align - 1) & -align;
            param_addr = addr;
            addr += size;
        } else {
            int regcount = 1;
            if (size > XLEN)
              regcount++, tcc_error("unimp: scalars > 64bit");
            if (regcount + (is_float(type->t) ? afreg : aireg) >= 8)
              goto from_stack;
            loc -= regcount * 8; // XXX could reserve only 'size' bytes
            param_addr = loc;
            for (i = 0; i < regcount; i++) {
                if (is_float(type->t)) {
                    assert(type->t == VT_FLOAT || type->t == VT_DOUBLE);
                    ES(0x27, size == 4 ? 2 : 3, 8, 10 + afreg, loc + i*8); // fs[wd] FAi, loc(s0)
                    afreg++;
                } else {
                    ES(0x23, 3, 8, 10 + aireg, loc + i*8); // sd aX, loc(s0) // XXX
                    aireg++;
                }
            }
        }
        sym_push(sym->v & ~SYM_FIELD, type,
                 VT_LOCAL | lvalue_type(type->t), param_addr);
    }
}

ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret,
                       int *ret_align, int *regsize)
{
    /* generic code can only deal with structs of pow(2) sizes
       (it always deals with whole registers), so go through our own
       code.  */
    return 0;
}

ST_FUNC void gfunc_return(CType *func_type)
{
    int align, size = type_size(func_type, &align);
    if ((func_type->t & VT_BTYPE) == VT_STRUCT
        || size > 2 * XLEN) {
        tcc_error("unimp: struct or large return");
    }
    if (is_float(func_type->t))
      gv(RC_FRET);
    else
      gv(RC_IRET);
    vtop--;
}

ST_FUNC void gfunc_epilog(void)
{
    int v, saved_ind;

    v = (-loc + 15) & -16;

    EI(0x03, 3, 1, 2, v - 8);  // ld ra, v-8(sp)
    EI(0x03, 3, 8, 2, v - 16); // ld s0, v-16(sp)
    EI(0x13, 0, 2, 2, v);      // addi sp, sp, v
    EI(0x67, 0, 0, 1, 0);      // jalr x0, 0(x1), aka ret
    saved_ind = ind;
    ind = func_sub_sp_offset;
    EI(0x13, 0, 2, 2, -v);     // addi sp, sp, -v
    ES(0x23, 3, 2, 1, v - 8);  // sd ra, v-8(sp)
    ES(0x23, 3, 2, 8, v - 16); // sd s0, v-16(sp)
    EI(0x13, 0, 8, 2, v);      // addi s0, sp, v
    ind = saved_ind;
}

ST_FUNC void gen_va_start(void)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_va_arg(CType *t)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_fill_nops(int bytes)
{
    tcc_error("implement me: %s", __FUNCTION__);
    if ((bytes & 3))
      tcc_error("alignment of code section not multiple of 4");
}

// Generate forward branch to label:
ST_FUNC int gjmp(int t)
{
    if (nocode_wanted)
      return t;
    o(t);
    return ind - 4;
}

// Generate branch to known address:
ST_FUNC void gjmp_addr(int a)
{
    uint32_t r = a - ind, imm;
    if ((r + (1 << 21)) & ~((1U << 22) - 2))
      tcc_error("out-of-range jump");
    imm =   (((r >> 12) &  0xff) << 12)
          | (((r >> 11) &     1) << 20)
          | (((r >>  1) & 0x3ff) << 21)
          | (((r >> 20) &     1) << 31);
    o(0x6f | imm); // jal x0, imm ==  j imm
}

ST_FUNC int gjmp_cond(int op, int t)
{
    int inv = op & 1;
    assert(op == TOK_EQ || op == TOK_NE);
    assert(vtop->cmp_r >= 10 && vtop->cmp_r < 18);
    o(0x63 | (!inv << 12) | (vtop->cmp_r << 15) | (8 << 7)); // bne/beq x0,r,+4
    return gjmp(t);
}

ST_FUNC int gjmp_append(int n, int t)
{
    void *p;
    /* insert jump list n into t */
    if (n) {
        uint32_t n1 = n, n2;
        while ((n2 = read32le(p = cur_text_section->data + n1)))
            n1 = n2;
        write32le(p, t);
        t = n;
    }
    return t;
}

static void gen_opil(int op, int ll)
{
    int a, b, d;
    int inv = 0;
    int func3 = 0, func7 = 0;
    /* XXX We could special-case some constant args.  */
    gv2(RC_INT, RC_INT);
    a = ireg(vtop[-1].r);
    b = ireg(vtop[0].r);
    vtop -= 2;
    d = get_reg(RC_INT);
    vtop++;
    vtop[0].r = d;
    d = ireg(d);
    ll = ll ? 0 : 8;
    switch (op) {
    case '%':
    case TOK_PDIV:
    default:
        tcc_error("implement me: %s(%s)", __FUNCTION__, get_tok_str(op, NULL));

    case '+':
        o(0x33 | (d << 7) | (a << 15) | (b << 20)); // add d, a, b
        break;
    case '-':
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (0x20 << 25)); //sub d, a, b
        break;
    case TOK_SAR:
        o(0x33 | ll | (d << 7) | (a << 15) | (b << 20) | (5 << 12) | (1 << 30)); //sra d, a, b
        break;
    case TOK_SHR:
        o(0x33 | ll | (d << 7) | (a << 15) | (b << 20) | (5 << 12)); //srl d, a, b
        break;
    case TOK_SHL:
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (1 << 12)); //sll d, a, b
        break;
    case '*':
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (0x01 << 25)); //mul d, a, b
        break;
    case '/':
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (0x01 << 25) | (4 << 12)); //div d, a, b
        break;
    case '&':
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (7 << 12)); // and d, a, b
        break;
    case '^':
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (4 << 12)); // xor d, a, b
        break;
    case '|':
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (6 << 12)); // or d, a, b
        break;
    case TOK_UMOD:
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (0x01 << 25) | (7 << 12)); //remu d, a, b
        break;
    case TOK_UDIV:
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (0x01 << 25) | (5 << 12)); //divu d, a, b
        break;

    case TOK_ULT:
    case TOK_UGE:
    case TOK_ULE:
    case TOK_UGT:
    case TOK_LT:
    case TOK_GE:
    case TOK_LE:
    case TOK_GT:
        if (op & 1) { // remove [U]GE,GT
            inv = 1;
            op--;
        }
        if ((op & 7) == 6) { // [U]LE
            int t = a; a = b; b = t;
            inv ^= 1;
        }
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (((op > TOK_UGT) ? 2 : 3) << 12)); // slt[u] d, a, b
        if (inv)
          EI(0x13, 4, d, d, 1); // xori d, d, 1
        vset_VT_CMP(TOK_NE);
        vtop->cmp_r = d;
        break;
    case TOK_NE:
    case TOK_EQ:
        o(0x33 | (d << 7) | (a << 15) | (b << 20) | (0x20 << 25)); // sub d, a, b
        if (op == TOK_NE)
          o(0x33 | (3 << 12) | (d << 7) | (0 << 15) | (d << 20)); // sltu d, x0, d == snez d,d
        else
          EI(0x13, 3, d, d, 1); // sltiu d, d, 1 == seqz d,d
        vset_VT_CMP(TOK_NE);
        vtop->cmp_r = d;
        break;
    }
}

ST_FUNC void gen_opi(int op)
{
    gen_opil(op, 0);
}

ST_FUNC void gen_opl(int op)
{
    gen_opil(op, 1);
}

ST_FUNC void gen_opf(int op)
{
    int rs1, rs2, rd, dbl, invert;
    gv2(RC_FLOAT, RC_FLOAT);
    assert(vtop->type.t == VT_DOUBLE || vtop->type.t == VT_FLOAT);
    dbl = vtop->type.t == VT_DOUBLE;
    rs1 = freg(vtop[-1].r);
    rs2 = freg(vtop->r);
    vtop--;
    invert = 0;
    switch(op) {
    default:
        assert(0);
    case '+':
        op = 0; // fadd
    arithop:
        rd = get_reg(RC_FLOAT);
        vtop->r = rd;
        rd = freg(rd);
        o(0x53 | (rd << 7) | (rs1 << 15) | (rs2 << 20) | (7 << 12) | (dbl << 25) | (op << 27)); // fop.[sd] RD, RS1, RS2 (dyn rm)
        break;
    case '-':
        op = 1; // fsub
        goto arithop;
    case '*':
        op = 2; // fmul
        goto arithop;
    case '/':
        op = 3; // fdiv
        goto arithop;
    case TOK_EQ:
        op = 2; // EQ
    cmpop:
        rd = get_reg(RC_INT);
        vtop->r = rd;
        rd = ireg(rd);
        o(0x53 | (rd << 7) | (rs1 << 15) | (rs2 << 20) | (op << 12) | (dbl << 25) | (0x14 << 27)); // fcmp.[sd] RD, RS1, RS2 (op == eq/lt/le)
        if (invert)
          EI(0x13, 4, rd, rd, 1); // xori RD, 1
        break;
    case TOK_NE:
        invert = 1;
        op = 2; // EQ
        goto cmpop;
    case TOK_LT:
        op = 1; // LT
        goto cmpop;
    case TOK_LE:
        op = 0; // LE
        goto cmpop;
    case TOK_GT:
        op = 1; // LT
        rd = rs1, rs1 = rs2, rs2 = rd;
        goto cmpop;
    case TOK_GE:
        op = 0; // LE
        rd = rs1, rs1 = rs2, rs2 = rd;
        goto cmpop;
    }
}

ST_FUNC void gen_cvt_sxtw(void)
{
    /* XXX on risc-v the registers are usually sign-extended already.
       Let's try to not do anything here.  */
}

ST_FUNC void gen_cvt_itof(int t)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_cvt_ftoi(int t)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_cvt_ftof(int dt)
{
    int st = vtop->type.t & VT_BTYPE, rs, rd;
    dt &= VT_BTYPE;
    assert (dt == VT_FLOAT || dt == VT_DOUBLE);
    assert (st == VT_FLOAT || st == VT_DOUBLE);
    if (st == dt)
      return;
    rs = gv(RC_FLOAT);
    rd = get_reg(RC_FLOAT);
    if (dt == VT_DOUBLE)
      EI(0x53, 7, freg(rd), freg(rs), 0x21 << 5); // fcvt.d.s RD, RS (dyn rm)
    else
      EI(0x53, 7, freg(rd), freg(rs), (0x20 << 5) | 1); // fcvt.s.d RD, RS
    vtop->r = rd;
}

ST_FUNC void ggoto(void)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_vla_sp_save(int addr)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_vla_sp_restore(int addr)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_vla_alloc(CType *type, int align)
{
    tcc_error("implement me: %s", __FUNCTION__);
}
#endif
