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
    assert(r >= 0 && r < 8);
    return r + 10;  // tccrX --> aX == x(10+X)
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
        tcc_error("implement me: %s", __FUNCTION__);
        if (a - t + 0x8000000 >= 0x10000000)
            tcc_error("branch out of range");
        write32le(ptr, (a - t == 4 ? 0xd503201f : // nop
                        0x14000000 | ((a - t) >> 2 & 0x3ffffff))); // b
        t = next;
    }
}

ST_FUNC void load(int r, SValue *sv)
{
    int fr = sv->r;
    int v = fr & VT_VALMASK;
    int rr = ireg(r);
    int fc = sv->c.i;
    if (fr & VT_LVAL) {
        if (v == VT_LOCAL) {
            if (((unsigned)fc + (1 << 11)) >> 12)
              tcc_error("unimp: load(large local ofs) (0x%x)", fc);
            EI(0x03, 3, rr, 8, fc); // ld RR, fc(s0)
        } else {
            tcc_error("unimp: load(non-local lval)");
        }
    } else if (v == VT_CONST) {
        int rb = 0;
        if (fc != sv->c.i)
          tcc_error("unimp: load(very large const)");
        if (((unsigned)fc + (1 << 11)) >> 12)
          tcc_error("unimp: load(large const) (0x%x)", fc);
        if (fr & VT_SYM) {
            static Sym label;
            greloca(cur_text_section, vtop->sym, ind,
                    R_RISCV_PCREL_HI20, fc);
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
        }
        if (is_float(sv->type.t))
          tcc_error("unimp: load(float)");
        EI(0x13, 0, rr, rb, fc);      // addi R, x0|R, FC
    } else
      tcc_error("unimp: load(non-const)");
}

ST_FUNC void store(int r, SValue *sv)
{
    tcc_error("implement me: %s", __FUNCTION__);
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
    } else {
        tcc_error("unimp: indirect call");
    }
}

ST_FUNC void gfunc_call(int nb_args)
{
    int i, align, size, aireg;
    aireg = 0;
    for (i = 0; i < nb_args; i++) {
        size = type_size(&vtop[-i].type, &align);
        if (size > 8 || ((vtop[-i].type.t & VT_BTYPE) == VT_STRUCT)
            || is_float(vtop[-i].type.t))
          tcc_error("unimp: call arg %d wrong type", nb_args - i);
        if (aireg >= 8)
          tcc_error("unimp: too many register args");
        vrotb(i+1);
        gv(RC_R(aireg));
        vrott(i+1);
        aireg++;
    }
    vrotb(nb_args + 1);
    gcall();
    vtop -= nb_args + 1;
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
              regcount++;
            if (regcount + (is_float(type->t) ? afreg : aireg) >= 8)
              goto from_stack;
            loc -= regcount * 8;
            param_addr = loc;
            for (i = 0; i < regcount; i++) {
                if (is_float(type->t)) {
                    tcc_error("unimp: float args");
                } else {
                    ES(0x23, 3, 8, 10 + aireg, loc + i*8); // sd aX, loc(s0) // XXX
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
    tcc_error("implement me: %s", __FUNCTION__);
}

// Generate branch to known address:
ST_FUNC void gjmp_addr(int a)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC int gjmp_cond(int op, int t)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC int gjmp_append(int n, int t)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC int gtst(int inv, int t)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_opi(int op)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_opl(int op)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_opf(int op)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_cvt_sxtw(void)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_cvt_itof(int t)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_cvt_ftoi(int t)
{
    tcc_error("implement me: %s", __FUNCTION__);
}

ST_FUNC void gen_cvt_ftof(int t)
{
    tcc_error("implement me: %s", __FUNCTION__);
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
