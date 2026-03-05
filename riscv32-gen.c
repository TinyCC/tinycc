#ifdef TARGET_DEFS_ONLY

// Number of registers available to allocator:
// x10-x17 aka a0-a7, xxx, ra, sp
// No float registers (soft-float RV32IMA)
#define NB_REGS 11
#define CONFIG_TCC_ASM

#define TREG_R(x) (x) // x = 0..7

// Register classes sorted from more general to more precise:
#define RC_INT (1 << 0)
#define RC_FLOAT (1 << 1) // defined but no regs in this class (soft-float)
#define RC_R(x) (1 << (2 + (x))) // x = 0..7

#define RC_IRET (RC_R(0)) // int return register class
#define RC_IRE2 (RC_R(1)) // int 2nd return register class
#define RC_FRET (RC_R(0)) // soft-float: float returns in int regs

#define REG_IRET (TREG_R(0)) // int return register number
#define REG_IRE2 (TREG_R(1)) // int 2nd return register number
#define REG_FRET (TREG_R(0)) // soft-float: float returns in int regs

#define PTR_SIZE 4

#define LDOUBLE_SIZE 8
#define LDOUBLE_ALIGN 8

#define MAX_ALIGN 16

#define CHAR_IS_UNSIGNED

#else
#define USING_GLOBALS
#include "tcc.h"
#include <assert.h>

#define UPPER(x)	(((unsigned)(x) + 0x800u) & 0xfffff000)
#define LOW_OVERFLOW(x)	UPPER(x)
#define SIGN7(x)	((((x) & 0xff) ^ 0x80) - 0x80)
#define SIGN11(x)	((((x) & 0xfff) ^ 0x800) - 0x800)

ST_DATA const char * const target_machine_defs =
    "__riscv\0"
    "__riscv_xlen 32\0"
    "__riscv_div\0"
    "__riscv_mul\0"
    "__riscv_float_abi_soft\0"
    ;

#define XLEN 4

#define TREG_RA 9
#define TREG_SP 10

ST_DATA const int reg_classes[NB_REGS] = {
  RC_INT | RC_FLOAT | RC_R(0),  /* soft-float: floats use int regs */
  RC_INT | RC_FLOAT | RC_R(1),
  RC_INT | RC_FLOAT | RC_R(2),
  RC_INT | RC_FLOAT | RC_R(3),
  RC_INT | RC_FLOAT | RC_R(4),
  RC_INT | RC_FLOAT | RC_R(5),
  RC_INT | RC_FLOAT | RC_R(6),
  RC_INT | RC_FLOAT | RC_R(7),
  0,
  1 << TREG_RA,
  1 << TREG_SP
};

#if defined(CONFIG_TCC_BCHECK)
static addr_t func_bound_offset;
static unsigned long func_bound_ind;
ST_DATA int func_bound_add_epilog;
#endif

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
    return (unsigned)r < 8 || r == TREG_RA || r == TREG_SP;
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

static void EIu(uint32_t opcode, uint32_t func3,
               uint32_t rd, uint32_t rs1, uint32_t imm)
{
    o(opcode | (func3 << 12) | (rd << 7) | (rs1 << 15) | (imm << 20));
}

static void ER(uint32_t opcode, uint32_t func3,
               uint32_t rd, uint32_t rs1, uint32_t rs2, uint32_t func7)
{
    o(opcode | func3 << 12 | rd << 7 | rs1 << 15 | rs2 << 20 | func7 << 25);
}

static void EI(uint32_t opcode, uint32_t func3,
               uint32_t rd, uint32_t rs1, uint32_t imm)
{
    assert(! LOW_OVERFLOW(imm));
    EIu(opcode, func3, rd, rs1, imm);
}

static void ES(uint32_t opcode, uint32_t func3,
               uint32_t rs1, uint32_t rs2, uint32_t imm)
{
    assert(! LOW_OVERFLOW(imm));
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
        imm = (((r >> 12) &  0xff) << 12)
            | (((r >> 11) &     1) << 20)
            | (((r >>  1) & 0x3ff) << 21)
            | (((r >> 20) &     1) << 31);
        write32le(ptr, r == 4 ? 0x33 : 0x6f | imm); // nop || j imm
        t = next;
    }
}

static int load_symofs(int r, SValue *sv, int forstore, int *new_fc)
{
    int rr, doload = 0, large_addend = 0;
    int fc = sv->c.i, v = sv->r & VT_VALMASK;
    if (sv->r & VT_SYM) {
        Sym label = {0};
        assert(v == VT_CONST);
        if (sv->sym->type.t & VT_STATIC) { // XXX do this per linker relax
            greloca(cur_text_section, sv->sym, ind,
                    R_RISCV_PCREL_HI20, sv->c.i);
            *new_fc = 0;
        } else {
            if (LOW_OVERFLOW(fc)){
              large_addend = 1;
            }
            greloca(cur_text_section, sv->sym, ind,
                    R_RISCV_GOT_HI20, 0);
            doload = 1;
        }
        label.type.t = VT_VOID | VT_STATIC;
	if (!nocode_wanted)
            put_extern_sym(&label, cur_text_section, ind, 0);
        rr = ireg(r);
        o(0x17 | (rr << 7));   // auipc RR, 0 %pcrel_hi(sym)+addend
        greloca(cur_text_section, &label, ind,
                doload || !forstore
                  ? R_RISCV_PCREL_LO12_I : R_RISCV_PCREL_LO12_S, 0);
        if (doload) {
            EI(0x03, 2, rr, rr, 0); // lw RR, 0(RR)
            if (large_addend) {
                o(0x37 | (6 << 7) | UPPER(fc)); //lui t1, high(fc)
                ER(0x33, 0, rr, rr, 6, 0); // add RR, RR, t1
                *new_fc = SIGN11(fc);
            }
        }
    } else if (v == VT_LOCAL || v == VT_LLOCAL) {
        rr = 8; // s0
        if (fc != sv->c.i)
          tcc_error("unimp: store(giant local off) (0x%lx)", (long)sv->c.i);
        if (LOW_OVERFLOW(fc)) {
            rr = ireg(r); // use dest reg as temp
            o(0x37 | (rr << 7) | UPPER(fc)); //lui RR, upper(fc)
            ER(0x33, 0, rr, rr, 8, 0); // add RR, RR, s0
            *new_fc = SIGN11(fc);
        }
    } else
      tcc_error("uhh");
    return rr;
}

ST_FUNC void load(int r, SValue *sv)
{
    int fr = sv->r;
    int v = fr & VT_VALMASK;
    int rr = ireg(r);
    int fc = sv->c.i;
    int bt = sv->type.t & VT_BTYPE;
    int align, size;
    if (fr & VT_LVAL) {
        int func3, opcode = 0x03, br;
        size = type_size(&sv->type, &align);
        if (bt == VT_PTR || bt == VT_FUNC) /* XXX should be done in generic code */
          size = PTR_SIZE;
        /* On RV32, max single-register load is 4 bytes */
        if (size > 4)
          size = 4;
        func3 = size == 1 ? 0 : size == 2 ? 1 : 2; /* lb, lh, lw */
        if (size < 4 && !is_float(sv->type.t) && (sv->type.t & VT_UNSIGNED))
          func3 |= 4; /* lbu, lhu */
        if (v == VT_LOCAL || (fr & VT_SYM)) {
            br = load_symofs(r, sv, 0, &fc);
        } else if (v < VT_CONST) {
            br = ireg(v);
            fc = 0; // XXX store ofs in LVAL(reg)
        } else if (v == VT_LLOCAL) {
            br = load_symofs(r, sv, 0, &fc);
            EI(0x03, 2, rr, br, fc); // lw RR, fc(BR)
            br = rr;
            fc = 0;
        } else if (v == VT_CONST) {
            o(0x37 | (rr << 7) | UPPER(fc)); //lui RR, upper(fc)
            fc = SIGN11(fc);
            br = rr;
	} else {
            tcc_error("unimp: load(non-local lval)");
        }
        EI(opcode, func3, rr, br, fc); // l[bhw][u] RR, fc(BR)
    } else if (v == VT_CONST) {
        int rb = 0;
        assert(is_ireg(r));
        if (fr & VT_SYM) {
            rb = load_symofs(r, sv, 0, &fc);
        }
        /* On RV64, float consts use FPU loads - not supported without FPU.
           On RV32 soft-float, float/double consts are loaded as integers
           (handled below via lui/addi), no special action needed. */
        if (LOW_OVERFLOW(fc))
            o(0x37 | (rr << 7) | UPPER(fc)), rb = rr; //lui RR, upper(fc)
        if (fc || (rr != rb) || (fr & VT_SYM))
          EI(0x13, 0, rr, rb, SIGN11(fc)); // addi R, x0|R, FC
    } else if (v == VT_LOCAL) {
        int br = load_symofs(r, sv, 0, &fc);
        assert(is_ireg(r));
        EI(0x13, 0, rr, br, fc); // addi R, s0, FC
    } else if (v < VT_CONST) { /* reg-reg */
        //assert(!fc); XXX support offseted regs
        if (is_ireg(r) && is_ireg(v))
          EI(0x13, 0, rr, ireg(v), 0); // addi RR, V, 0 == mv RR, V
        else {
          tcc_error("unimp: load(non-int reg-reg)");
        }
    } else if (v == VT_CMP) {
        int op = vtop->cmp_op;
        int a = vtop->cmp_r & 0xff;
        int b = (vtop->cmp_r >> 8) & 0xff;
        int inv = 0;
        switch (op) {
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
                ER(0x33, (op > TOK_UGT) ? 2 : 3, rr, a, b, 0); // slt[u] d, a, b
                if (inv)
                  EI(0x13, 4, rr, rr, 1); // xori d, d, 1
                break;
            case TOK_NE:
            case TOK_EQ:
                if (rr != a || b)
                  ER(0x33, 0, rr, a, b, 0x20); // sub d, a, b
                if (op == TOK_NE)
                  ER(0x33, 3, rr, 0, rr, 0); // sltu d, x0, d == snez d,d
                else
                  EI(0x13, 3, rr, rr, 1); // sltiu d, d, 1 == seqz d,d
                break;
        }
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
    int rr = ireg(r), ptrreg;
    int fc = sv->c.i;
    int bt = sv->type.t & VT_BTYPE;
    int align, size = type_size(&sv->type, &align);
    /* long doubles are in two integer registers, but the load/store
       primitives only deal with one, so do as if it's one reg.  */
    if (bt == VT_LDOUBLE)
      size = align = 4;
    if (bt == VT_STRUCT)
      tcc_error("unimp: store(struct)");
    /* On RV32, max single-register store is 4 bytes */
    if (size > 4)
      size = 4;
    assert(sv->r & VT_LVAL);
    if (fr == VT_LOCAL || (sv->r & VT_SYM)) {
        ptrreg = load_symofs(-1, sv, 1, &fc);
    } else if (fr < VT_CONST) {
        ptrreg = ireg(fr);
        fc = 0; // XXX support offsets regs
    } else if (fr == VT_CONST) {
        ptrreg = 8; // s0
        o(0x37 | (ptrreg << 7) | UPPER(fc)); //lui RR, upper(fc)
        fc = SIGN11(fc);
    } else
      tcc_error("implement me: %s(!local)", __FUNCTION__);
    ES(0x23,                                                    // s...
       size == 1 ? 0 : size == 2 ? 1 : 2,                     // [bhw]
       ptrreg, rr, fc);                                         // RR, fc(base)
}

static void gcall_or_jmp(int docall)
{
    int tr = docall ? 1 : 5; // ra or t0
    if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST &&
        ((vtop->r & VT_SYM) && vtop->c.i == (int)vtop->c.i)) {
        /* constant symbolic case -> simple relocation */
        greloca(cur_text_section, vtop->sym, ind,
                R_RISCV_CALL_PLT, (int)vtop->c.i);
        o(0x17 | (tr << 7));   // auipc TR, 0 %call(func)
        EI(0x67, 0, tr, tr, 0);// jalr  TR, r(TR)
    } else if (vtop->r < VT_CONST) {
        int r = ireg(vtop->r);
        EI(0x67, 0, tr, r, 0);      // jalr TR, 0(R)
    } else {
        int r = TREG_RA;
        load(r, vtop);
        r = ireg(r);
        EI(0x67, 0, tr, r, 0);      // jalr TR, 0(R)
    }
}

#if defined(CONFIG_TCC_BCHECK)

static void gen_bounds_call(int v)
{
    Sym *sym = external_helper_sym(v);

    greloca(cur_text_section, sym, ind, R_RISCV_CALL_PLT, 0);
    o(0x17 | (1 << 7));   // auipc TR, 0 %call(func)
    EI(0x67, 0, 1, 1, 0); // jalr  TR, r(TR)
}

static void gen_bounds_prolog(void)
{
    /* leave some room for bound checking code */
    func_bound_offset = lbounds_section->data_offset;
    func_bound_ind = ind;
    func_bound_add_epilog = 0;
    o(0x00000013);  /* nop -> load lbound section pointer */
    o(0x00000013);
    o(0x00000013);  /* nop -> call __bound_local_new */
    o(0x00000013);
}

static void gen_bounds_epilog(void)
{
    addr_t saved_ind;
    addr_t *bounds_ptr;
    Sym *sym_data;
    Sym label = {0};

    int offset_modified = func_bound_offset != lbounds_section->data_offset;

    if (!offset_modified && !func_bound_add_epilog)
        return;

    /* add end of table info */
    bounds_ptr = section_ptr_add(lbounds_section, sizeof(addr_t));
    *bounds_ptr = 0;

    sym_data = get_sym_ref(&char_pointer_type, lbounds_section,
                           func_bound_offset, PTR_SIZE);

    label.type.t = VT_VOID | VT_STATIC;
    /* generate bound local allocation */
    if (offset_modified) {
        saved_ind = ind;
        ind = func_bound_ind;
        put_extern_sym(&label, cur_text_section, ind, 0);
        greloca(cur_text_section, sym_data, ind, R_RISCV_GOT_HI20, 0);
        o(0x17 | (10 << 7));    // auipc a0, 0 %pcrel_hi(sym)+addend
        greloca(cur_text_section, &label, ind, R_RISCV_PCREL_LO12_I, 0);
        EI(0x03, 2, 10, 10, 0); // lw a0, 0(a0)
        gen_bounds_call(TOK___bound_local_new);
        ind = saved_ind;
        label.c = 0; /* force new local ELF symbol */
    }

    /* generate bound check local freeing */
    /* addi sp,sp,-16; sw a0,0(sp); sw a1,4(sp) */
    EI(0x13, 0, 2, 2, -16);     // addi sp, sp, -16
    ES(0x23, 2, 2, 10, 0);      // sw a0, 0(sp)
    ES(0x23, 2, 2, 11, 4);      // sw a1, 4(sp)
    put_extern_sym(&label, cur_text_section, ind, 0);
    greloca(cur_text_section, sym_data, ind, R_RISCV_GOT_HI20, 0);
    o(0x17 | (10 << 7));    // auipc a0, 0 %pcrel_hi(sym)+addend
    greloca(cur_text_section, &label, ind, R_RISCV_PCREL_LO12_I, 0);
    EI(0x03, 2, 10, 10, 0); // lw a0, 0(a0)
    gen_bounds_call(TOK___bound_local_delete);
    EI(0x03, 2, 10, 2, 0);      // lw a0, 0(sp)
    EI(0x03, 2, 11, 2, 4);      // lw a1, 4(sp)
    EI(0x13, 0, 2, 2, 16);      // addi sp, sp, 16
}
#endif

static void reg_pass_rec(CType *type, int *rc, int *fieldofs, int ofs)
{
    if ((type->t & VT_BTYPE) == VT_STRUCT) {
        Sym *f;
        if (type->ref->type.t == VT_UNION)
          rc[0] = -1;
        else for (f = type->ref->next; f; f = f->next)
          reg_pass_rec(&f->type, rc, fieldofs, ofs + f->c);
    } else if (type->t & VT_ARRAY) {
        if (type->ref->c < 0 || type->ref->c > 2)
          rc[0] = -1;
        else {
            int a, sz = type_size(&type->ref->type, &a);
            reg_pass_rec(&type->ref->type, rc, fieldofs, ofs);
            if (rc[0] > 2 || (rc[0] == 2 && type->ref->c > 1))
              rc[0] = -1;
            else if (type->ref->c == 2 && rc[0] && rc[1] == RC_INT) {
              rc[++rc[0]] = RC_INT;
              fieldofs[rc[0]] = ((ofs + sz) << 4)
                                | (type->ref->type.t & VT_BTYPE);
            } else if (type->ref->c == 2)
              rc[0] = -1;
        }
    } else if (rc[0] == 2 || rc[0] < 0
               || (type->t & VT_BTYPE) == VT_LDOUBLE
               || (type->t & VT_BTYPE) == VT_DOUBLE
               || (type->t & VT_BTYPE) == VT_LLONG)
      /* On RV32 soft-float, double/llong/ldouble are wider than XLEN
         and need register pairs; handled by reg_pass fallback */
      rc[0] = -1;
    else if (!rc[0] || rc[1] == RC_INT) {
      /* soft-float: all types go in integer registers */
      rc[++rc[0]] = RC_INT;
      fieldofs[rc[0]] = (ofs << 4) | ((type->t & VT_BTYPE) == VT_PTR ? VT_INT : type->t & VT_BTYPE);
    } else
      rc[0] = -1;
}

static void reg_pass(CType *type, int *prc, int *fieldofs, int named)
{
    prc[0] = 0;
    reg_pass_rec(type, prc, fieldofs, 0);
    if (prc[0] <= 0 || !named) {
        int align, size = type_size(type, &align);
        prc[0] = (size + 3) >> 2; /* number of 4-byte slots */
        prc[1] = prc[2] = RC_INT;
        fieldofs[1] = (0 << 4) | (size <= 1 ? VT_BYTE : size <= 2 ? VT_SHORT : VT_INT);
        fieldofs[2] = (4 << 4) | (size <= 5 ? VT_BYTE : size <= 6 ? VT_SHORT : VT_INT);
    }
}

ST_FUNC void gfunc_call(int nb_args)
{
    int i, align, size, areg[2];
    int *info = tcc_malloc((nb_args + 1) * sizeof (int));
    int stack_adj = 0, tempspace = 0, stack_add, ofs, splitofs = 0;
    int old = (vtop[-nb_args].type.ref->f.func_type == FUNC_OLD);
    SValue *sv;
    Sym *sa;

#ifdef CONFIG_TCC_BCHECK
    int bc_save = tcc_state->do_bounds_check;
    if (tcc_state->do_bounds_check)
        gbound_args(nb_args);
#endif

    areg[0] = 0; /* int arg regs */
    areg[1] = 0; /* no float arg regs (soft-float) */
    sa = vtop[-nb_args].type.ref->next;
    for (i = 0; i < nb_args; i++) {
        int nregs, byref = 0, tempofs;
        int prc[3], fieldofs[3];
        sv = &vtop[1 + i - nb_args];
        sv->type.t &= ~VT_ARRAY; // XXX this should be done in tccgen.c
        size = type_size(&sv->type, &align);
        if (size > 2 * XLEN) {
            if (align < XLEN)
              align = XLEN;
            tempspace = (tempspace + align - 1) & -align;
            tempofs = tempspace;
            tempspace += size;
            size = align = XLEN;
            byref = 64 | (tempofs << 7);
        }
        reg_pass(&sv->type, prc, fieldofs, old || sa != 0);
        if (!old && !sa && align == 2*XLEN && size <= 2*XLEN)
          areg[0] = (areg[0] + 1) & ~1;
        nregs = prc[0];
        if (size == 0)
            info[i] = 0;
        else if (prc[1] == RC_INT && areg[0] >= 8) {
            info[i] = 32;
            if (align < XLEN)
              align = XLEN;
            stack_adj += (size + align - 1) & -align;
            if (!old && !sa) /* one vararg on stack forces the rest on stack */
              areg[0] = 8;
        } else {
            info[i] = areg[0]++;
            if (!byref)
              info[i] |= (fieldofs[1] & VT_BTYPE) << 12;
            assert(!(fieldofs[1] >> 4));
            if (nregs == 2) {
                if (areg[0] < 8)
                  info[i] |= (1 + areg[0]++) << 7;
                else {
                    info[i] |= 16;
                    stack_adj += XLEN;
                }
                if (!byref) {
                    assert((fieldofs[2] >> 4) < 2048);
                    info[i] |= fieldofs[2] << (12 + 4); // includes offset
                }
            }
        }
        info[i] |= byref;
        if (sa)
          sa = sa->next;
    }
    stack_adj = (stack_adj + 15) & -16;
    tempspace = (tempspace + 15) & -16;
    stack_add = stack_adj + tempspace;

    if (stack_add) {
        if (stack_add >= 0x800) {
            o(0x37 | (5 << 7) | UPPER(-stack_add)); //lui t0, upper(v)
            EI(0x13, 0, 5, 5, SIGN11(-stack_add)); // addi t0, t0, lo(v)
            ER(0x33, 0, 2, 2, 5, 0); // add sp, sp, t0
        }
        else
            EI(0x13, 0, 2, 2, -stack_add);   // addi sp, sp, -adj
        for (i = ofs = 0; i < nb_args; i++) {
            if (info[i] & (64 | 32)) {
                vrotb(nb_args - i);
                size = type_size(&vtop->type, &align);
                if (info[i] & 64) {
                    vset(&char_pointer_type, TREG_SP, 0);
                    vpushi(stack_adj + (info[i] >> 7));
                    gen_op('+');
                    vpushv(vtop); // this replaces the old argument
                    vrott(3);
                    indir();
                    vtop->type = vtop[-1].type;
                    vswap();
                    vstore();
                    vpop();
                    size = align = XLEN;
                }
                if (info[i] & 32) {
                    if (align < XLEN)
                      align = XLEN;
                    vset(&char_pointer_type, TREG_SP, 0);
                    ofs = (ofs + align - 1) & -align;
                    vpushi(ofs);
                    gen_op('+');
                    indir();
                    vtop->type = vtop[-1].type;
                    vswap();
                    vstore();
                    vtop->r = vtop->r2 = VT_CONST; // this arg is done
                    ofs += size;
                }
                vrott(nb_args - i);
            } else if (info[i] & 16) {
                assert(!splitofs);
                splitofs = ofs;
                ofs += XLEN;
            }
        }
    }
    for (i = 0; i < nb_args; i++) {
        int ii = info[nb_args - 1 - i], r = ii, r2 = r;
        if (!(r & 32)) {
            CType origtype;
            int loadt;
            r &= 15;
            r2 = r2 & 64 ? 0 : (r2 >> 7) & 31;
            assert(r2 <= 16);
            vrotb(i+1);
            origtype = vtop->type;
            size = type_size(&vtop->type, &align);
            if (size == 0)
                goto done;
            loadt = vtop->type.t & VT_BTYPE;
            if (loadt == VT_STRUCT) {
                loadt = (ii >> 12) & VT_BTYPE;
            }
            if (info[nb_args - 1 - i] & 16) {
                assert(!r2);
                r2 = 1 + TREG_RA;
            }
            if (loadt == VT_LDOUBLE
                || (r2 && (loadt == VT_DOUBLE))) {
                /* Double/ldouble: two-word value handled via offset below */
                assert(r2);
                r2--;
            } else if (r2) {
                test_lvalue();
                vpushv(vtop);
            }
            vtop->type.t = loadt | (vtop->type.t & VT_UNSIGNED);
            gv(RC_R(r));
            vtop->type = origtype;

            if (r2 && loadt != VT_LDOUBLE && loadt != VT_DOUBLE) {
                r2--;
                assert(r2 < 16 || r2 == TREG_RA);
                vswap();
                gaddrof();
                vtop->type = char_pointer_type;
                vpushi(ii >> 20);
#ifdef CONFIG_TCC_BCHECK
		if ((origtype.t & VT_BTYPE) == VT_STRUCT)
                    tcc_state->do_bounds_check = 0;
#endif
                gen_op('+');
#ifdef CONFIG_TCC_BCHECK
		tcc_state->do_bounds_check = bc_save;
#endif
                indir();
                vtop->type = origtype;
                loadt = vtop->type.t & VT_BTYPE;
                if (loadt == VT_STRUCT) {
                    loadt = (ii >> 16) & VT_BTYPE;
                }
                save_reg_upstack(r2, 1);
                vtop->type.t = loadt | (vtop->type.t & VT_UNSIGNED);
                load(r2, vtop);
                assert(r2 < VT_CONST);
                vtop--;
                vtop->r2 = r2;
            }
            if (info[nb_args - 1 - i] & 16) {
                ES(0x23, 2, 2, ireg(vtop->r2), splitofs); // sw t0, ofs(sp)
                vtop->r2 = VT_CONST;
            } else if ((loadt == VT_LDOUBLE || loadt == VT_DOUBLE) && vtop->r2 != r2) {
                assert(vtop->r2 <= 7 && r2 <= 7);
                EI(0x13, 0, ireg(r2), ireg(vtop->r2), 0); // mv Ra+1, RR2
                vtop->r2 = r2;
            }
done:
            vrott(i+1);
        }
    }
    vrotb(nb_args + 1);
    save_regs(nb_args + 1);
    gcall_or_jmp(1);
    vtop -= nb_args + 1;
    if (stack_add) {
        if (stack_add >= 0x800) {
            o(0x37 | (5 << 7) | UPPER(stack_add)); //lui t0, upper(v)
            EI(0x13, 0, 5, 5, SIGN11(stack_add)); // addi t0, t0, lo(v)
            ER(0x33, 0, 2, 2, 5, 0); // add sp, sp, t0
        }
        else
            EI(0x13, 0, 2, 2, stack_add);      // addi sp, sp, adj
   }
   tcc_free(info);
}

static int func_sub_sp_offset, num_va_regs, func_va_list_ofs;

ST_FUNC void gfunc_prolog(Sym *func_sym)
{
    CType *func_type = &func_sym->type;
    int i, addr, align, size;
    int param_addr = 0;
    int areg[2];
    Sym *sym;
    CType *type;

    sym = func_type->ref;
    loc = -8; // for ra and s0 (each 4 bytes)
    func_sub_sp_offset = ind;
    ind += 5 * 4;

    areg[0] = 0, areg[1] = 0;
    addr = 0;
    /* if the function returns by reference, then add an
       implicit pointer parameter */
    size = type_size(&func_vt, &align);
    if (size > 2 * XLEN) {
        loc -= XLEN;
        func_vc = loc;
        ES(0x23, 2, 8, 10 + areg[0]++, loc); // sw a0, loc(s0)
    }
    /* define parameters */
    while ((sym = sym->next) != NULL) {
        int byref = 0;
        int regcount;
        int prc[3], fieldofs[3];
        type = &sym->type;
        size = type_size(type, &align);
        if (size > 2 * XLEN) {
            type = &char_pointer_type;
            size = align = byref = XLEN;
        }
        reg_pass(type, prc, fieldofs, 1);
        regcount = prc[0];
        if (areg[prc[1] - 1] >= 8
            || (regcount == 2 && areg[0] >= 7)) {
            if (align < XLEN)
              align = XLEN;
            addr = (addr + align - 1) & -align;
            param_addr = addr;
            addr += size;
        } else {
            loc -= regcount * XLEN;
            param_addr = loc;
            for (i = 0; i < regcount; i++) {
                if (areg[0] >= 8) {
                    assert(i == 1 && regcount == 2 && !(addr & (XLEN-1)));
                    EI(0x03, 2, 5, 8, addr); // lw t0, addr(s0)
                    addr += XLEN;
                    ES(0x23, 2, 8, 5, loc + i*XLEN); // sw t0, loc(s0)
                } else {
                    ES(0x23, 2, 8, 10 + areg[0]++, loc + i*XLEN); // sw aX, loc(s0)
                }
            }
        }
        gfunc_set_param(sym, param_addr, byref);
    }
    func_va_list_ofs = addr;
    num_va_regs = 0;
    if (func_var) {
        for (; areg[0] < 8; areg[0]++) {
            num_va_regs++;
            ES(0x23, 2, 8, 10 + areg[0], -XLEN + num_va_regs * XLEN); // sw aX, loc(s0)
        }
    }
#ifdef CONFIG_TCC_BCHECK
    if (tcc_state->do_bounds_check)
        gen_bounds_prolog();
#endif
}

ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret,
                       int *ret_align, int *regsize)
{
    int align, size = type_size(vt, &align), nregs;
    int prc[3], fieldofs[3];
    *ret_align = 1;
    *regsize = XLEN;
    if (size > 2 * XLEN)
      return 0;
    reg_pass(vt, prc, fieldofs, 1);
    nregs = prc[0];
    if (nregs == 2 && prc[1] != prc[2])
      return -1;  /* generic code can't deal with this case */
    ret->t = fieldofs[1] & VT_BTYPE;
    ret->ref = NULL;
    return nregs;
}

ST_FUNC void arch_transfer_ret_regs(int aftercall)
{
    int prc[3], fieldofs[3];
    reg_pass(&vtop->type, prc, fieldofs, 1);
    assert(prc[0] == 2 && prc[1] != prc[2] && !(fieldofs[1] >> 4));
    assert(vtop->r == (VT_LOCAL | VT_LVAL));
    vpushv(vtop);
    vtop->type.t = fieldofs[1] & VT_BTYPE;
    (aftercall ? store : load)(REG_IRET, vtop);
    vtop->c.i += fieldofs[2] >> 4;
    vtop->type.t = fieldofs[2] & VT_BTYPE;
    (aftercall ? store : load)(REG_IRET, vtop);
    vtop--;
}

ST_FUNC void gfunc_epilog(void)
{
    int v, saved_ind, d, large_ofs_ind;

#ifdef CONFIG_TCC_BCHECK
    if (tcc_state->do_bounds_check)
        gen_bounds_epilog();
#endif

    loc = (loc - num_va_regs * XLEN);
    d = v = (-loc + 15) & -16;

    EI(0x13, 0, 2, 8, num_va_regs * XLEN); // addi sp, s0, num_va_regs*XLEN
    EI(0x03, 2, 1, 8, -4); // lw ra, -4(s0)
    EI(0x03, 2, 8, 8, -8); // lw s0, -8(s0)
    EI(0x67, 0, 0, 1, 0); // jalr x0, 0(x1), aka ret

    large_ofs_ind = ind;
    if (v >= (1 << 11)) {
        d = 8; // space for ra+s0
        EI(0x13, 0, 8, 2, d - num_va_regs * XLEN);      // addi s0, sp, d
        o(0x37 | (5 << 7) | UPPER(v-8)); //lui t0, upper(v)
        EI(0x13, 0, 5, 5, SIGN11(v-8)); // addi t0, t0, lo(v)
        ER(0x33, 0, 2, 2, 5, 0x20); // sub sp, sp, t0
        gjmp_addr(func_sub_sp_offset + 5*4);
    }
    saved_ind = ind;

    ind = func_sub_sp_offset;
    EI(0x13, 0, 2, 2, -d);     // addi sp, sp, -d
    ES(0x23, 2, 2, 1, d - 4 - num_va_regs * XLEN);  // sw ra, d-4(sp)
    ES(0x23, 2, 2, 8, d - 8 - num_va_regs * XLEN);  // sw s0, d-8(sp)
    if (v < (1 << 11))
      EI(0x13, 0, 8, 2, d - num_va_regs * XLEN);      // addi s0, sp, d
    else
      gjmp_addr(large_ofs_ind);
    if ((ind - func_sub_sp_offset) != 5*4)
      EI(0x13, 0, 0, 0, 0);      // addi x0, x0, 0 == nop
    ind = saved_ind;
}

ST_FUNC void gen_va_start(void)
{
    vtop--;
    vset(&char_pointer_type, VT_LOCAL, func_va_list_ofs);
}

ST_FUNC void gen_fill_nops(int bytes)
{
    if ((bytes & 3))
      tcc_error("alignment of code section not multiple of 4");
    while (bytes > 0) {
        EI(0x13, 0, 0, 0, 0);      // addi x0, x0, 0 == nop
        bytes -= 4;
    }
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
    if ((r + (1 << 21)) & ~((1U << 22) - 2)) {
        o(0x17 | (5 << 7) | UPPER(r)); // lui RR, up(r)
        r = SIGN11(r);
        EI(0x67, 0, 0, 5, r);      // jalr x0, r(t0)
    } else {
        imm = (((r >> 12) &  0xff) << 12)
            | (((r >> 11) &     1) << 20)
            | (((r >>  1) & 0x3ff) << 21)
            | (((r >> 20) &     1) << 31);
        o(0x6f | imm); // jal x0, imm ==  j imm
    }
}

ST_FUNC int gjmp_cond(int op, int t)
{
    int tmp;
    int a = vtop->cmp_r & 0xff;
    int b = (vtop->cmp_r >> 8) & 0xff;
    switch (op) {
        case TOK_ULT: op = 6; break;
        case TOK_UGE: op = 7; break;
        case TOK_ULE: op = 7; tmp = a; a = b; b = tmp; break;
        case TOK_UGT: op = 6; tmp = a; a = b; b = tmp; break;
        case TOK_LT:  op = 4; break;
        case TOK_GE:  op = 5; break;
        case TOK_LE:  op = 5; tmp = a; a = b; b = tmp; break;
        case TOK_GT:  op = 4; tmp = a; a = b; b = tmp; break;
        case TOK_NE:  op = 1; break;
        case TOK_EQ:  op = 0; break;
    }
    o(0x63 | (op ^ 1) << 12 | a << 15 | b << 20 | 8 << 7); // bOP a,b,+4
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

/* RV32: carry/borrow register for long long add/sub.
   We use x5 (t0) which is not managed by the register allocator.
   Between TOK_ADDC1/SUBC1 and TOK_ADDC2/SUBC2, no other code
   generation occurs (only vstack manipulation), so t0 is safe. */
#define CARRY_REG 5 /* x5 = t0 */

static void gen_opil(int op)
{
    int a, b, d;
    int func3 = 0;
    if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST) {
        int fc = vtop->c.i;
        if (fc == vtop->c.i && !LOW_OVERFLOW(fc)) {
            int m = 31; /* RV32: shift mask is 5 bits */
            vswap();
            gv(RC_INT);
            a = ireg(vtop[0].r);
            --vtop;
            d = get_reg(RC_INT);
            ++vtop;
            vswap();
            switch (op) {
                case '-':
                    if (fc <= -(1 << 11))
                      break;
                    fc = -fc;
                case '+':
                    func3 = 0; // addi d, a, fc
                do_cop:
                    EI(0x13, func3, ireg(d), a, fc);
                    --vtop;
                    if (op >= TOK_ULT && op <= TOK_GT) {
                      vset_VT_CMP(TOK_NE);
                      vtop->cmp_r = ireg(d) | 0 << 8;
                    } else
                      vtop[0].r = d;
                    return;
                case TOK_LE:
                    if (fc >= (1 << 11) - 1)
                      break;
                    ++fc;
                case TOK_LT:  func3 = 2; goto do_cop; // slti d, a, fc
                case TOK_ULE:
                    if (fc >= (1 << 11) - 1 || fc == -1)
                      break;
                    ++fc;
                case TOK_ULT: func3 = 3; goto do_cop; // sltiu d, a, fc
                case '^':     func3 = 4; goto do_cop; // xori d, a, fc
                case '|':     func3 = 6; goto do_cop; // ori  d, a, fc
                case '&':     func3 = 7; goto do_cop; // andi d, a, fc
                case TOK_SHL: func3 = 1; fc &= m; goto do_cop; // slli d, a, fc
                case TOK_SHR: func3 = 5; fc &= m; goto do_cop; // srli d, a, fc
                case TOK_SAR: func3 = 5; fc = 1024 | (fc & m); goto do_cop;

                case TOK_UGE: /* -> TOK_ULT */
                case TOK_UGT: /* -> TOK_ULE */
                case TOK_GE:  /* -> TOK_LT */
                case TOK_GT:  /* -> TOK_LE */
                    gen_opil(op - 1);
                    vtop->cmp_op ^= 1;
                    return;

                case TOK_NE:
                case TOK_EQ:
                    if (fc)
                      gen_opil('-'), a = ireg(vtop++->r);
                    --vtop;
                    vset_VT_CMP(op);
                    vtop->cmp_r = a | 0 << 8;
                    return;
            }
        }
    }
    gv2(RC_INT, RC_INT);
    a = ireg(vtop[-1].r);
    b = ireg(vtop[0].r);
    vtop -= 2;
    d = get_reg(RC_INT);
    vtop++;
    vtop[0].r = d;
    d = ireg(d);
    switch (op) {
    default:
        if (op >= TOK_ULT && op <= TOK_GT) {
            vset_VT_CMP(op);
            vtop->cmp_r = a | b << 8;
            break;
        }
        tcc_error("implement me: %s(%s)", __FUNCTION__, get_tok_str(op, NULL));
        break;

    case '+':
        ER(0x33, 0, d, a, b, 0); // add d, a, b
        break;
    case '-':
        ER(0x33, 0, d, a, b, 0x20); // sub d, a, b
        break;
    case TOK_SAR:
        ER(0x33, 5, d, a, b, 0x20); // sra d, a, b
        break;
    case TOK_SHR:
        ER(0x33, 5, d, a, b, 0); // srl d, a, b
        break;
    case TOK_SHL:
        ER(0x33, 1, d, a, b, 0); // sll d, a, b
        break;
    case '*':
        ER(0x33, 0, d, a, b, 1); // mul d, a, b
        break;
    case '/':
    case TOK_PDIV:
        ER(0x33, 4, d, a, b, 1); // div d, a, b
        break;
    case '&':
        ER(0x33, 7, d, a, b, 0); // and d, a, b
        break;
    case '^':
        ER(0x33, 4, d, a, b, 0); // xor d, a, b
        break;
    case '|':
        ER(0x33, 6, d, a, b, 0); // or d, a, b
        break;
    case '%':
        ER(0x33, 6, d, a, b, 1); // rem d, a, b
        break;
    case TOK_UMOD:
        ER(0x33, 7, d, a, b, 1); // remu d, a, b
        break;
    case TOK_UDIV:
        ER(0x33, 5, d, a, b, 1); // divu d, a, b
        break;

    /* Long long carry operations (called by tccgen.c gen_opl) */
    case TOK_ADDC1: // add low words, save carry in t0
        ER(0x33, 0, d, a, b, 0);              // add d, a, b
        ER(0x33, 3, CARRY_REG, d, b, 0);      // sltu t0, d, b
        break;
    case TOK_ADDC2: // add high words with carry from t0
        ER(0x33, 0, d, a, b, 0);              // add d, a, b
        ER(0x33, 0, d, d, CARRY_REG, 0);      // add d, d, t0
        break;
    case TOK_SUBC1: // sub low words, save borrow in t0
        ER(0x33, 3, CARRY_REG, a, b, 0);      // sltu t0, a, b
        ER(0x33, 0, d, a, b, 0x20);           // sub d, a, b
        break;
    case TOK_SUBC2: // sub high words with borrow from t0
        ER(0x33, 0, d, a, b, 0x20);           // sub d, a, b
        ER(0x33, 0, d, d, CARRY_REG, 0x20);   // sub d, d, t0
        break;
    }
}

ST_FUNC void gen_opi(int op)
{
    /* Handle TOK_UMULL specially: needs two result registers */
    if (op == TOK_UMULL) {
        int a, b, dl, dh;
        gv2(RC_INT, RC_INT);
        a = ireg(vtop[-1].r);
        b = ireg(vtop[0].r);
        vtop--;
        dl = get_reg(RC_INT);
        vtop->r = dl;  /* mark dl in-use so get_reg returns a different reg */
        dh = get_reg(RC_INT);
        /* Compute high first (reads a,b), then low (may clobber if dl==a or dl==b) */
        ER(0x33, 3, ireg(dh), a, b, 1); // mulhu dh, a, b
        ER(0x33, 0, ireg(dl), a, b, 1); // mul dl, a, b
        vtop->r = dl;
        vtop->r2 = dh;
        return;
    }
    gen_opil(op);
}

/* On RV32, gen_opl is provided by tccgen.c (PTR_SIZE==4) which
   decomposes long long ops into TOK_ADDC1/ADDC2/SUBC1/SUBC2/UMULL
   handled by gen_opi above. */

ST_FUNC void gen_opf(int op)
{
    /* RV32IMA: no FPU, all float ops through library calls */
    int func = 0;
    int cond = -1;
    int ft = vtop[0].type.t & VT_BTYPE;
    CType type = vtop[0].type;

    if (ft == VT_FLOAT) {
        switch (op) {
        case '*': func = TOK___mulsf3; break;
        case '+': func = TOK___addsf3; break;
        case '-': func = TOK___subsf3; break;
        case '/': func = TOK___divsf3; break;
        case TOK_EQ: func = TOK___eqsf2; cond = 1; break;
        case TOK_NE: func = TOK___nesf2; cond = 0; break;
        case TOK_LT: func = TOK___ltsf2; cond = 10; break;
        case TOK_GE: func = TOK___gesf2; cond = 11; break;
        case TOK_LE: func = TOK___lesf2; cond = 12; break;
        case TOK_GT: func = TOK___gtsf2; cond = 13; break;
        default: assert(0); break;
        }
    } else if (ft == VT_DOUBLE || ft == VT_LDOUBLE) {
        switch (op) {
        case '*': func = TOK___muldf3; break;
        case '+': func = TOK___adddf3; break;
        case '-': func = TOK___subdf3; break;
        case '/': func = TOK___divdf3; break;
        case TOK_EQ: func = TOK___eqdf2; cond = 1; break;
        case TOK_NE: func = TOK___nedf2; cond = 0; break;
        case TOK_LT: func = TOK___ltdf2; cond = 10; break;
        case TOK_GE: func = TOK___gedf2; cond = 11; break;
        case TOK_LE: func = TOK___ledf2; cond = 12; break;
        case TOK_GT: func = TOK___gtdf2; cond = 13; break;
        default: assert(0); break;
        }
    } else {
        assert(0);
    }

    vpush_helper_func(func);
    vrott(3);
    gfunc_call(2);
    vpushi(0);
    vtop->r = REG_IRET;
    vtop->r2 = VT_CONST;
    if (cond < 0) {
        vtop->type = type;
        if (ft == VT_DOUBLE || ft == VT_LDOUBLE)
            vtop->r2 = TREG_R(1);
    } else {
        vpushi(0);
        gen_opil(op);
    }
}

ST_FUNC void gen_cvt_itof(int t)
{
    int u, l, func;
    /* soft-float: use library calls */
    gv(RC_INT);
    u = vtop->type.t & VT_UNSIGNED;
    l = (vtop->type.t & VT_BTYPE) == VT_LLONG;

    if (t == VT_FLOAT) {
        if (l)
            func = u ? TOK___floatundisf : TOK___floatdisf;
        else
            func = u ? TOK___floatunsisf : TOK___floatsisf;
    } else {
        /* VT_DOUBLE or VT_LDOUBLE */
        if (l)
            func = u ? TOK___floatundidf : TOK___floatdidf;
        else
            func = u ? TOK___floatunsidf : TOK___floatsidf;
    }
    vpush_helper_func(func);
    vrott(2);
    gfunc_call(1);
    vpushi(0);
    vtop->type.t = t;
    vtop->r = REG_IRET;
    if (t == VT_DOUBLE || t == VT_LDOUBLE)
        vtop->r2 = TREG_R(1);
}

ST_FUNC void gen_cvt_ftoi(int t)
{
    /* soft-float: use library calls */
    int ft = vtop->type.t & VT_BTYPE;
    int l = (t & VT_BTYPE) == VT_LLONG;
    int u = t & VT_UNSIGNED;
    int func;

    if (ft == VT_FLOAT) {
        if (l)
            func = u ? TOK___fixunssfdi : TOK___fixsfdi;
        else
            func = u ? TOK___fixunssfsi : TOK___fixsfsi;
    } else {
        /* VT_DOUBLE or VT_LDOUBLE */
        if (l)
            func = u ? TOK___fixunsdfdi : TOK___fixdfdi;
        else
            func = u ? TOK___fixunsdfsi : TOK___fixdfsi;
    }
    vpush_helper_func(func);
    vrott(2);
    gfunc_call(1);
    vpushi(0);
    vtop->type.t = t;
    vtop->r = REG_IRET;
    if (l)
        vtop->r2 = TREG_R(1);
}

ST_FUNC void gen_cvt_ftof(int dt)
{
    int st = vtop->type.t & VT_BTYPE;
    int func;
    dt &= VT_BTYPE;
    if (st == dt)
      return;
    /* soft-float: use library calls for float<->double conversion */
    if (dt == VT_DOUBLE || dt == VT_LDOUBLE) {
        func = TOK___extendsfdf2;
    } else {
        func = TOK___truncdfsf2;
    }
    save_regs(1);
    gv(RC_R(0));
    if (st == VT_DOUBLE || st == VT_LDOUBLE) {
        /* double is in register pair, ensure r2 = r+1 */
        if (vtop->r2 != 1 + vtop->r) {
            EI(0x13, 0, ireg(vtop->r) + 1, ireg(vtop->r2), 0); // mv Ra+1, RR2
            vtop->r2 = 1 + vtop->r;
        }
    }
    vpush_helper_func(func);
    gcall_or_jmp(1);
    vtop -= 2;
    vpushi(0);
    vtop->type.t = dt;
    if (dt == VT_DOUBLE || dt == VT_LDOUBLE)
      vtop->r = REG_IRET, vtop->r2 = REG_IRET+1;
    else
      vtop->r = REG_IRET;
}

/* increment tcov counter */
ST_FUNC void gen_increment_tcov (SValue *sv)
{
    int r1, r2;
    Sym label = {0};
    label.type.t = VT_VOID | VT_STATIC;

    vpushv(sv);
    vtop->r = r1 = get_reg(RC_INT);
    r2 = get_reg(RC_INT);
    r1 = ireg(r1);
    r2 = ireg(r2);
    greloca(cur_text_section, sv->sym, ind, R_RISCV_PCREL_HI20, 0);
    put_extern_sym(&label, cur_text_section, ind, 0);
    o(0x17 | (r1 << 7)); // auipc RR, 0 %pcrel_hi(sym)
    greloca(cur_text_section, &label, ind, R_RISCV_PCREL_LO12_I, 0);
    EI(0x03, 2, r2, r1, 0); // lw r2, x[r1]
    EI(0x13, 0, r2, r2, 1); // addi r2, r2, #1
    greloca(cur_text_section, sv->sym, ind, R_RISCV_PCREL_HI20, 0);
    label.c = 0; /* force new local ELF symbol */
    put_extern_sym(&label, cur_text_section, ind, 0);
    o(0x17 | (r1 << 7)); // auipc RR, 0 %pcrel_hi(sym)
    greloca(cur_text_section, &label, ind, R_RISCV_PCREL_LO12_S, 0);
    ES(0x23, 2, r1, r2, 0); // sw r2, [r1]
    vpop();
}

ST_FUNC void ggoto(void)
{
    gcall_or_jmp(0);
    vtop--;
}

ST_FUNC void gen_vla_sp_save(int addr)
{
    if (LOW_OVERFLOW(addr)) {
	o(0x37 | (5 << 7) | UPPER(addr)); //lui t0,upper(addr)
        ER(0x33, 0, 5, 5, 8, 0); // add t0, t0, s0
        ES(0x23, 2, 5, 2, SIGN11(addr)); // sw sp, fc(t0)
    }
    else
        ES(0x23, 2, 8, 2, addr); // sw sp, fc(s0)
}

ST_FUNC void gen_vla_sp_restore(int addr)
{
    if (LOW_OVERFLOW(addr)) {
	o(0x37 | (5 << 7) | UPPER(addr)); //lui t0,upper(addr)
        ER(0x33, 0, 5, 5, 8, 0); // add t0, t0, s0
        EI(0x03, 2, 2, 5, SIGN11(addr)); // lw sp, fc(t0)
    }
    else
        EI(0x03, 2, 2, 8, addr); // lw sp, fc(s0)
}

ST_FUNC void gen_vla_alloc(CType *type, int align)
{
    int rr;
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check)
        vpushv(vtop);
#endif
    rr = ireg(gv(RC_INT));
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check)
        EI(0x13, 0, rr, rr, 15+1);   // addi RR, RR, 15+1
    else
#endif
    EI(0x13, 0, rr, rr, 15);   // addi RR, RR, 15
    EI(0x13, 7, rr, rr, -16);  // andi, RR, RR, -16
    ER(0x33, 0, 2, 2, rr, 0x20); // sub sp, sp, rr
    vpop();
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check) {
        vpushi(0);
        vtop->r = TREG_R(0);
        o(0x00010513); /* mv a0,sp */
        vswap();
        vpush_helper_func(TOK___bound_new_region);
        vrott(3);
        gfunc_call(2);
        func_bound_add_epilog = 1;
    }
#endif
}
#endif
