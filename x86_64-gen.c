/*
 *  x86-64 code generator for TCC
 *
 *  Copyright (c) 2008 Shinichiro Hamaji
 *
 *  Based on i386-gen.c by Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef TARGET_DEFS_ONLY

/* number of available registers */
#define NB_REGS         25
#define NB_ASM_REGS     16
#define CONFIG_TCC_ASM

/* a register can belong to several classes. The classes must be
   sorted from more general to more precise (see gv2() code which does
   assumptions on it). */
#define RC_INT     0x0001 /* generic integer register */
#define RC_FLOAT   0x0002 /* generic float register */
#define RC_RAX     0x0004
#define RC_RCX     0x0008
#define RC_RDX     0x0010
#define RC_ST0     0x0080 /* only for long double */
#define RC_R8      0x0100
#define RC_R9      0x0200
#define RC_R10     0x0400
#define RC_R11     0x0800
#define RC_XMM0    0x1000
#define RC_XMM1    0x2000
#define RC_XMM2    0x4000
#define RC_XMM3    0x8000
#define RC_XMM4    0x10000
#define RC_XMM5    0x20000
#define RC_XMM6    0x40000
#define RC_XMM7    0x80000
#define RC_IRET    RC_RAX /* function return: integer register */
#define RC_IRE2    RC_RDX /* function return: second integer register */
#define RC_FRET    RC_XMM0 /* function return: float register */
#define RC_FRE2    RC_XMM1 /* function return: second float register */

/* pretty names for the registers */
enum {
    TREG_RAX = 0,
    TREG_RCX = 1,
    TREG_RDX = 2,
    TREG_RSP = 4,
    TREG_RSI = 6,
    TREG_RDI = 7,

    TREG_R8  = 8,
    TREG_R9  = 9,
    TREG_R10 = 10,
    TREG_R11 = 11,

    TREG_XMM0 = 16,
    TREG_XMM1 = 17,
    TREG_XMM2 = 18,
    TREG_XMM3 = 19,
    TREG_XMM4 = 20,
    TREG_XMM5 = 21,
    TREG_XMM6 = 22,
    TREG_XMM7 = 23,

    TREG_ST0 = 24,

    TREG_MEM = 0x20
};

#define REX_BASE(reg) (((reg) >> 3) & 1)
#define REG_VALUE(reg) ((reg) & 7)

/* return registers for function */
#define REG_IRET TREG_RAX /* single word int return register */
#define REG_IRE2 TREG_RDX /* second word return register (for long long) */
#define REG_FRET TREG_XMM0 /* float return register */
#define REG_FRE2 TREG_XMM1 /* second float return register */

/* defined if function parameters must be evaluated in reverse order */
#define INVERT_FUNC_PARAMS

/* pointer size, in bytes */
#define PTR_SIZE 8

/* long double size and alignment, in bytes */
#define LDOUBLE_SIZE  16
#define LDOUBLE_ALIGN 16
/* maximum alignment (for aligned attribute support) */
#define MAX_ALIGN     16

/* define if return values need to be extended explicitely
   at caller side (for interfacing with non-TCC compilers) */
#define PROMOTE_RET
/******************************************************/
#else /* ! TARGET_DEFS_ONLY */
/******************************************************/
#define USING_GLOBALS
#include "tcc.h"
#include <assert.h>

ST_DATA const char * const target_machine_defs =
    "__x86_64__\0"
    "__amd64__\0"
    ;

ST_DATA const int reg_classes[NB_REGS] = {
    /* eax */ RC_INT | RC_RAX,
    /* ecx */ RC_INT | RC_RCX,
    /* edx */ RC_INT | RC_RDX,
    0,
    0,
    0,
    0,
    0,
    RC_R8,
    RC_R9,
    RC_R10,
    RC_R11,
    0,
    0,
    0,
    0,
    /* xmm0 */ RC_FLOAT | RC_XMM0,
    /* xmm1 */ RC_FLOAT | RC_XMM1,
    /* xmm2 */ RC_FLOAT | RC_XMM2,
    /* xmm3 */ RC_FLOAT | RC_XMM3,
    /* xmm4 */ RC_FLOAT | RC_XMM4,
    /* xmm5 */ RC_FLOAT | RC_XMM5,
    /* xmm6 an xmm7 are included so gv() can be used on them,
       but they are not tagged with RC_FLOAT because they are
       callee saved on Windows */
    RC_XMM6,
    RC_XMM7,
    /* st0 */ RC_ST0
};

/* XXX: make it faster ? */
ST_FUNC void g(TCCState* S, int c)
{
    int ind1;
    if (S->tccgen_nocode_wanted)
        return;
    ind1 = S->tccgen_ind + 1;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(S, cur_text_section, ind1);
    cur_text_section->data[S->tccgen_ind] = c;
    S->tccgen_ind = ind1;
}

ST_FUNC void o(TCCState* S, unsigned int c)
{
    while (c) {
        g(S, c);
        c = c >> 8;
    }
}

ST_FUNC void gen_le16(TCCState* S, int v)
{
    g(S, v);
    g(S, v >> 8);
}

ST_FUNC void gen_le32(TCCState* S, int c)
{
    g(S, c);
    g(S, c >> 8);
    g(S, c >> 16);
    g(S, c >> 24);
}

ST_FUNC void gen_le64(TCCState* S, int64_t c)
{
    g(S, c);
    g(S, c >> 8);
    g(S, c >> 16);
    g(S, c >> 24);
    g(S, c >> 32);
    g(S, c >> 40);
    g(S, c >> 48);
    g(S, c >> 56);
}

static void orex(TCCState* S, int ll, int r, int r2, int b)
{
    if ((r & VT_VALMASK) >= VT_CONST)
        r = 0;
    if ((r2 & VT_VALMASK) >= VT_CONST)
        r2 = 0;
    if (ll || REX_BASE(r) || REX_BASE(r2))
        o(S, 0x40 | REX_BASE(r) | (REX_BASE(r2) << 2) | (ll << 3));
    o(S, b);
}

/* output a symbol and patch all calls to it */
ST_FUNC void gsym_addr(TCCState* S, int t, int a)
{
    while (t) {
        unsigned char *ptr = cur_text_section->data + t;
        uint32_t n = read32le(ptr); /* next value */
        write32le(ptr, a < 0 ? -a : a - t - 4);
        t = n;
    }
}

static int is64_type(int t)
{
    return ((t & VT_BTYPE) == VT_PTR ||
            (t & VT_BTYPE) == VT_FUNC ||
            (t & VT_BTYPE) == VT_LLONG);
}

/* instruction + 4 bytes data. Return the address of the data */
static int oad(TCCState *S, int c, int s)
{
    int t;
    if (S->tccgen_nocode_wanted)
        return s;
    o(S, c);
    t = S->tccgen_ind;
    gen_le32(S, s);
    return t;
}

/* generate jmp to a label */
#define gjmp2(S, instr,lbl) oad(S, instr,lbl)

ST_FUNC void gen_addr32(TCCState *S, int r, Sym *sym, int c)
{
    if (r & VT_SYM)
        greloca(S, cur_text_section, sym, S->tccgen_ind, R_X86_64_32S, c), c=0;
    gen_le32(S, c);
}

/* output constant with relocation if 'r & VT_SYM' is true */
ST_FUNC void gen_addr64(TCCState *S, int r, Sym *sym, int64_t c)
{
    if (r & VT_SYM)
        greloca(S, cur_text_section, sym, S->tccgen_ind, R_X86_64_64, c), c=0;
    gen_le64(S, c);
}

/* output constant with relocation if 'r & VT_SYM' is true */
ST_FUNC void gen_addrpc32(TCCState *S, int r, Sym *sym, int c)
{
    if (r & VT_SYM)
        greloca(S, cur_text_section, sym, S->tccgen_ind, R_X86_64_PC32, c-4), c=4;
    gen_le32(S, c-4);
}

/* output got address with relocation */
static void gen_gotpcrel(TCCState *S, int r, Sym *sym, int c)
{
#ifdef TCC_TARGET_PE
    tcc_error(S, "internal error: no GOT on PE: %s %x %x | %02x %02x %02x\n",
        get_tok_str(S, sym->v, NULL), c, r,
        cur_text_section->data[S->tccgen_ind-3],
        cur_text_section->data[S->tccgen_ind-2],
        cur_text_section->data[S->tccgen_ind-1]
        );
#endif
    greloca(S, cur_text_section, sym, S->tccgen_ind, R_X86_64_GOTPCREL, -4);
    gen_le32(S, 0);
    if (c) {
        /* we use add c, %xxx for displacement */
        orex(S, 1, r, 0, 0x81);
        o(S, 0xc0 + REG_VALUE(r));
        gen_le32(S, c);
    }
}

static void gen_modrm_impl(TCCState *S, int op_reg, int r, Sym *sym, int c, int is_got)
{
    op_reg = REG_VALUE(op_reg) << 3;
    if ((r & VT_VALMASK) == VT_CONST) {
        /* constant memory reference */
	if (!(r & VT_SYM)) {
	    /* Absolute memory reference */
	    o(S, 0x04 | op_reg); /* [sib] | destreg */
	    oad(S, 0x25, c);     /* disp32 */
	} else {
	    o(S, 0x05 | op_reg); /* (%rip)+disp32 | destreg */
	    if (is_got) {
		gen_gotpcrel(S, r, sym, c);
	    } else {
		gen_addrpc32(S, r, sym, c);
	    }
	}
    } else if ((r & VT_VALMASK) == VT_LOCAL) {
        /* currently, we use only ebp as base */
        if (c == (char)c) {
            /* short reference */
            o(S, 0x45 | op_reg);
            g(S, c);
        } else {
            oad(S, 0x85 | op_reg, c);
        }
    } else if ((r & VT_VALMASK) >= TREG_MEM) {
        if (c) {
            g(S, 0x80 | op_reg | REG_VALUE(r));
            gen_le32(S, c);
        } else {
            g(S, 0x00 | op_reg | REG_VALUE(r));
        }
    } else {
        g(S, 0x00 | op_reg | REG_VALUE(r));
    }
}

/* generate a modrm reference. 'op_reg' contains the additional 3
   opcode bits */
static void gen_modrm(TCCState *S, int op_reg, int r, Sym *sym, int c)
{
    gen_modrm_impl(S, op_reg, r, sym, c, 0);
}

/* generate a modrm reference. 'op_reg' contains the additional 3
   opcode bits */
static void gen_modrm64(TCCState *S, int opcode, int op_reg, int r, Sym *sym, int c)
{
    int is_got;
    is_got = (op_reg & TREG_MEM) && !(sym->type.t & VT_STATIC);
    orex(S, 1, r, op_reg, opcode);
    gen_modrm_impl(S, op_reg, r, sym, c, is_got);
}


/* load 'r' from value 'sv' */
void load(TCCState *S, int r, SValue *sv)
{
    int v, t, ft, fc, fr;
    SValue v1;

#ifdef TCC_TARGET_PE
    SValue v2;
    sv = pe_getimport(S, sv, &v2);
#endif

    fr = sv->r;
    ft = sv->type.t & ~VT_DEFSIGN;
    fc = sv->c.i;
    if (fc != sv->c.i && (fr & VT_SYM))
      tcc_error(S, "64 bit addend in load");

    ft &= ~(VT_VOLATILE | VT_CONSTANT);

#ifndef TCC_TARGET_PE
    /* we use indirect access via got */
    if ((fr & VT_VALMASK) == VT_CONST && (fr & VT_SYM) &&
        (fr & VT_LVAL) && !(sv->sym->type.t & VT_STATIC)) {
        /* use the result register as a temporal register */
        int tr = r | TREG_MEM;
        if (is_float(ft)) {
            /* we cannot use float registers as a temporal register */
            tr = get_reg(S, RC_INT) | TREG_MEM;
        }
        gen_modrm64(S, 0x8b, tr, fr, sv->sym, 0);

        /* load from the temporal register */
        fr = tr | VT_LVAL;
    }
#endif

    v = fr & VT_VALMASK;
    if (fr & VT_LVAL) {
        int b, ll;
        if (v == VT_LLOCAL) {
            v1.type.t = VT_PTR;
            v1.r = VT_LOCAL | VT_LVAL;
            v1.c.i = fc;
            fr = r;
            if (!(reg_classes[fr] & (RC_INT|RC_R11)))
                fr = get_reg(S, RC_INT);
            load(S, fr, &v1);
        }
	if (fc != sv->c.i) {
	    /* If the addends doesn't fit into a 32bit signed
	       we must use a 64bit move.  We've checked above
	       that this doesn't have a sym associated.  */
	    v1.type.t = VT_LLONG;
	    v1.r = VT_CONST;
	    v1.c.i = sv->c.i;
	    fr = r;
	    if (!(reg_classes[fr] & (RC_INT|RC_R11)))
	        fr = get_reg(S, RC_INT);
	    load(S, fr, &v1);
	    fc = 0;
	}
        ll = 0;
	/* Like GCC we can load from small enough properly sized
	   structs and unions as well.
	   XXX maybe move to generic operand handling, but should
	   occur only with asm, so tccasm.c might also be a better place */
	if ((ft & VT_BTYPE) == VT_STRUCT) {
	    int align;
	    switch (type_size(&sv->type, &align)) {
		case 1: ft = VT_BYTE; break;
		case 2: ft = VT_SHORT; break;
		case 4: ft = VT_INT; break;
		case 8: ft = VT_LLONG; break;
		default:
		    tcc_error(S, "invalid aggregate type for register load");
		    break;
	    }
	}
        if ((ft & VT_BTYPE) == VT_FLOAT) {
            b = 0x6e0f66;
            r = REG_VALUE(r); /* movd */
        } else if ((ft & VT_BTYPE) == VT_DOUBLE) {
            b = 0x7e0ff3; /* movq */
            r = REG_VALUE(r);
        } else if ((ft & VT_BTYPE) == VT_LDOUBLE) {
            b = 0xdb, r = 5; /* fldt */
        } else if ((ft & VT_TYPE) == VT_BYTE || (ft & VT_TYPE) == VT_BOOL) {
            b = 0xbe0f;   /* movsbl */
        } else if ((ft & VT_TYPE) == (VT_BYTE | VT_UNSIGNED)) {
            b = 0xb60f;   /* movzbl */
        } else if ((ft & VT_TYPE) == VT_SHORT) {
            b = 0xbf0f;   /* movswl */
        } else if ((ft & VT_TYPE) == (VT_SHORT | VT_UNSIGNED)) {
            b = 0xb70f;   /* movzwl */
        } else if ((ft & VT_TYPE) == (VT_VOID)) {
            /* Can happen with zero size structs */
            return;
        } else {
            assert(((ft & VT_BTYPE) == VT_INT)
                   || ((ft & VT_BTYPE) == VT_LLONG)
                   || ((ft & VT_BTYPE) == VT_PTR)
                   || ((ft & VT_BTYPE) == VT_FUNC)
                );
            ll = is64_type(ft);
            b = 0x8b;
        }
        if (ll) {
            gen_modrm64(S, b, r, fr, sv->sym, fc);
        } else {
            orex(S, ll, fr, r, b);
            gen_modrm(S, r, fr, sv->sym, fc);
        }
    } else {
        if (v == VT_CONST) {
            if (fr & VT_SYM) {
#ifdef TCC_TARGET_PE
                orex(S, 1,0,r,0x8d);
                o(S, 0x05 + REG_VALUE(r) * 8); /* lea xx(%rip), r */
                gen_addrpc32(S, fr, sv->sym, fc);
#else
                if (sv->sym->type.t & VT_STATIC) {
                    orex(S, 1,0,r,0x8d);
                    o(S, 0x05 + REG_VALUE(r) * 8); /* lea xx(%rip), r */
                    gen_addrpc32(S, fr, sv->sym, fc);
                } else {
                    orex(S, 1,0,r,0x8b);
                    o(S, 0x05 + REG_VALUE(r) * 8); /* mov xx(%rip), r */
                    gen_gotpcrel(S, r, sv->sym, fc);
                }
#endif
            } else if (is64_type(ft)) {
                orex(S, 1,r,0, 0xb8 + REG_VALUE(r)); /* mov $xx, r */
                gen_le64(S, sv->c.i);
            } else {
                orex(S, 0,r,0, 0xb8 + REG_VALUE(r)); /* mov $xx, r */
                gen_le32(S, fc);
            }
        } else if (v == VT_LOCAL) {
            orex(S, 1,0,r,0x8d); /* lea xxx(%ebp), r */
            gen_modrm(S, r, VT_LOCAL, sv->sym, fc);
        } else if (v == VT_CMP) {
	    if (fc & 0x100)
	      {
                v = S->tccgen_vtop->cmp_r;
                fc &= ~0x100;
	        /* This was a float compare.  If the parity bit is
		   set the result was unordered, meaning false for everything
		   except TOK_NE, and true for TOK_NE.  */
                orex(S, 0, r, 0, 0xb0 + REG_VALUE(r)); /* mov $0/1,%al */
                g(S, v ^ fc ^ (v == TOK_NE));
                o(S, 0x037a + (REX_BASE(r) << 8));
              }
            orex(S, 0,r,0, 0x0f); /* setxx %br */
            o(S, fc);
            o(S, 0xc0 + REG_VALUE(r));
            orex(S, 0,r,0, 0x0f);
            o(S, 0xc0b6 + REG_VALUE(r) * 0x900); /* movzbl %al, %eax */
        } else if (v == VT_JMP || v == VT_JMPI) {
            t = v & 1;
            orex(S, 0,r,0,0);
            oad(S, 0xb8 + REG_VALUE(r), t); /* mov $1, r */
            o(S, 0x05eb + (REX_BASE(r) << 8)); /* jmp after */
            gsym(S, fc);
            orex(S, 0,r,0,0);
            oad(S, 0xb8 + REG_VALUE(r), t ^ 1); /* mov $0, r */
        } else if (v != r) {
            if ((r >= TREG_XMM0) && (r <= TREG_XMM7)) {
                if (v == TREG_ST0) {
                    /* gen_cvt_ftof(VT_DOUBLE); */
                    o(S, 0xf0245cdd); /* fstpl -0x10(%rsp) */
                    /* movsd -0x10(%rsp),%xmmN */
                    o(S, 0x100ff2);
                    o(S, 0x44 + REG_VALUE(r)*8); /* %xmmN */
                    o(S, 0xf024);
                } else {
                    assert((v >= TREG_XMM0) && (v <= TREG_XMM7));
                    if ((ft & VT_BTYPE) == VT_FLOAT) {
                        o(S, 0x100ff3);
                    } else {
                        assert((ft & VT_BTYPE) == VT_DOUBLE);
                        o(S, 0x100ff2);
                    }
                    o(S, 0xc0 + REG_VALUE(v) + REG_VALUE(r)*8);
                }
            } else if (r == TREG_ST0) {
                assert((v >= TREG_XMM0) && (v <= TREG_XMM7));
                /* gen_cvt_ftof(VT_LDOUBLE); */
                /* movsd %xmmN,-0x10(%rsp) */
                o(S, 0x110ff2);
                o(S, 0x44 + REG_VALUE(r)*8); /* %xmmN */
                o(S, 0xf024);
                o(S, 0xf02444dd); /* fldl -0x10(%rsp) */
            } else {
                orex(S, is64_type(ft), r, v, 0x89);
                o(S, 0xc0 + REG_VALUE(r) + REG_VALUE(v) * 8); /* mov v, r */
            }
        }
    }
}

/* store register 'r' in lvalue 'v' */
void store(TCCState *S, int r, SValue *v)
{
    int fr, bt, ft, fc;
    int op64 = 0;
    /* store the REX prefix in this variable when PIC is enabled */
    int pic = 0;

#ifdef TCC_TARGET_PE
    SValue v2;
    v = pe_getimport(S, v, &v2);
#endif

    fr = v->r & VT_VALMASK;
    ft = v->type.t;
    fc = v->c.i;
    if (fc != v->c.i && (fr & VT_SYM))
      tcc_error(S, "64 bit addend in store");
    ft &= ~(VT_VOLATILE | VT_CONSTANT);
    bt = ft & VT_BTYPE;

#ifndef TCC_TARGET_PE
    /* we need to access the variable via got */
    if (fr == VT_CONST
        && (v->r & VT_SYM)
        && !(v->sym->type.t & VT_STATIC)) {
        /* mov xx(%rip), %r11 */
        o(S, 0x1d8b4c);
        gen_gotpcrel(S, TREG_R11, v->sym, v->c.i);
        pic = is64_type(bt) ? 0x49 : 0x41;
    }
#endif

    /* XXX: incorrect if float reg to reg */
    if (bt == VT_FLOAT) {
        o(S, 0x66);
        o(S, pic);
        o(S, 0x7e0f); /* movd */
        r = REG_VALUE(r);
    } else if (bt == VT_DOUBLE) {
        o(S, 0x66);
        o(S, pic);
        o(S, 0xd60f); /* movq */
        r = REG_VALUE(r);
    } else if (bt == VT_LDOUBLE) {
        o(S, 0xc0d9); /* fld %st(0) */
        o(S, pic);
        o(S, 0xdb); /* fstpt */
        r = 7;
    } else {
        if (bt == VT_SHORT)
            o(S, 0x66);
        o(S, pic);
        if (bt == VT_BYTE || bt == VT_BOOL)
            orex(S, 0, 0, r, 0x88);
        else if (is64_type(bt))
            op64 = 0x89;
        else
            orex(S, 0, 0, r, 0x89);
    }
    if (pic) {
        /* xxx r, (%r11) where xxx is mov, movq, fld, or etc */
        if (op64)
            o(S, op64);
        o(S, 3 + (r << 3));
    } else if (op64) {
        if (fr == VT_CONST || fr == VT_LOCAL || (v->r & VT_LVAL)) {
            gen_modrm64(S, op64, r, v->r, v->sym, fc);
        } else if (fr != r) {
            orex(S, 1, fr, r, op64);
            o(S, 0xc0 + fr + r * 8); /* mov r, fr */
        }
    } else {
        if (fr == VT_CONST || fr == VT_LOCAL || (v->r & VT_LVAL)) {
            gen_modrm(S, r, v->r, v->sym, fc);
        } else if (fr != r) {
            o(S, 0xc0 + fr + r * 8); /* mov r, fr */
        }
    }
}

/* 'is_jmp' is '1' if it is a jump */
static void gcall_or_jmp(TCCState *S, int is_jmp)
{
    int r;
    if ((S->tccgen_vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST &&
	((S->tccgen_vtop->r & VT_SYM) && (S->tccgen_vtop->c.i-4) == (int)(S->tccgen_vtop->c.i-4))) {
        /* constant symbolic case -> simple relocation */
#ifdef TCC_TARGET_PE
        greloca(S, cur_text_section, S->tccgen_vtop->sym, S->tccgen_ind + 1, R_X86_64_PC32, (int)(S->tccgen_vtop->c.i-4));
#else
        greloca(S, cur_text_section, S->tccgen_vtop->sym, S->tccgen_ind + 1, R_X86_64_PLT32, (int)(S->tccgen_vtop->c.i-4));
#endif
        oad(S, 0xe8 + is_jmp, 0); /* call/jmp im */
    } else {
        /* otherwise, indirect call */
        r = TREG_R11;
        load(S, r, S->tccgen_vtop);
        o(S, 0x41); /* REX */
        o(S, 0xff); /* call/jmp *r */
        o(S, 0xd0 + REG_VALUE(r) + (is_jmp << 4));
    }
}

#if defined(CONFIG_TCC_BCHECK)

static void gen_bounds_call(TCCState *S, int v)
{
    Sym *sym = external_helper_sym(S, v);
    oad(S, 0xe8, 0);
#ifdef TCC_TARGET_PE
    greloca(S, cur_text_section, sym, S->tccgen_ind-4, R_X86_64_PC32, -4);
#else
    greloca(S, cur_text_section, sym, S->tccgen_ind-4, R_X86_64_PLT32, -4);
#endif
}

#ifdef TCC_TARGET_PE
# define TREG_FASTCALL_1 TREG_RCX
#else
# define TREG_FASTCALL_1 TREG_RDI
#endif

static void gen_bounds_prolog(TCCState *S)
{
    /* leave some room for bound checking code */
    S->func_bound_offset = lbounds_section->data_offset;
    S->func_bound_ind = S->tccgen_ind;
    S->func_bound_add_epilog = 0;
    o(S, 0x0d8d48 + ((TREG_FASTCALL_1 == TREG_RDI) * 0x300000)); /*lbound section pointer */
    gen_le32(S, 0);
    oad(S, 0xb8, 0); /* call to function */
}

static void gen_bounds_epilog(TCCState *S)
{
    addr_t saved_ind;
    addr_t *bounds_ptr;
    Sym *sym_data;
    int offset_modified = S->func_bound_offset != lbounds_section->data_offset;

    if (!offset_modified && !S->func_bound_add_epilog)
        return;

    /* add end of table info */
    bounds_ptr = section_ptr_add(S, lbounds_section, sizeof(addr_t));
    *bounds_ptr = 0;

    sym_data = get_sym_ref(S, &S->tccgen_char_pointer_type, lbounds_section, 
                           S->func_bound_offset, lbounds_section->data_offset);

    /* generate bound local allocation */
    if (offset_modified) {
        saved_ind = S->tccgen_ind;
        S->tccgen_ind = S->func_bound_ind;
        greloca(S, cur_text_section, sym_data, S->tccgen_ind + 3, R_X86_64_PC32, -4);
        S->tccgen_ind = S->tccgen_ind + 7;
        gen_bounds_call(S, TOK___bound_local_new);
        S->tccgen_ind = saved_ind;
    }

    /* generate bound check local freeing */
    o(S, 0x5250); /* save returned value, if any */
    greloca(S, cur_text_section, sym_data, S->tccgen_ind + 3, R_X86_64_PC32, -4);
    o(S, 0x0d8d48 + ((TREG_FASTCALL_1 == TREG_RDI) * 0x300000)); /* lea xxx(%rip), %rcx/rdi */
    gen_le32(S, 0);
    gen_bounds_call(S, TOK___bound_local_delete);
    o(S, 0x585a); /* restore returned value, if any */
}
#endif

#ifdef TCC_TARGET_PE

#define REGN 4
static const uint8_t arg_regs[REGN] = {
    TREG_RCX, TREG_RDX, TREG_R8, TREG_R9
};

/* Prepare arguments in R10 and R11 rather than RCX and RDX
   because gv() will not ever use these */
static int arg_prepare_reg(int idx) {
  if (idx == 0 || idx == 1)
      /* idx=0: r10, idx=1: r11 */
      return idx + 10;
  else
      return idx >= 0 && idx < REGN ? arg_regs[idx] : 0;
}

/* Generate function call. The function address is pushed first, then
   all the parameters in call order. This functions pops all the
   parameters and the function address. */

static void gen_offs_sp(TCCState* S, int b, int r, int d)
{
    orex(S, 1,0,r & 0x100 ? 0 : r, b);
    if (d == (char)d) {
        o(S, 0x2444 | (REG_VALUE(r) << 3));
        g(S, d);
    } else {
        o(S, 0x2484 | (REG_VALUE(r) << 3));
        gen_le32(S, d);
    }
}

static int using_regs(int size)
{
    return !(size > 8 || (size & (size - 1)));
}

/* Return the number of registers needed to return the struct, or 0 if
   returning via struct pointer. */
ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret, int *ret_align, int *regsize)
{
    int size, align;
    *ret_align = 1; // Never have to re-align return values for x86-64
    *regsize = 8;
    size = type_size(vt, &align);
    if (!using_regs(size))
        return 0;
    if (size == 8)
        ret->t = VT_LLONG;
    else if (size == 4)
        ret->t = VT_INT;
    else if (size == 2)
        ret->t = VT_SHORT;
    else
        ret->t = VT_BYTE;
    ret->ref = NULL;
    return 1;
}

static int is_sse_float(int t) {
    int bt;
    bt = t & VT_BTYPE;
    return bt == VT_DOUBLE || bt == VT_FLOAT;
}

static int gfunc_arg_size(CType *type) {
    int align;
    if (type->t & (VT_ARRAY|VT_BITFIELD))
        return 8;
    return type_size(type, &align);
}

void gfunc_call(TCCState* S, int nb_args)
{
    int size, r, args_size, i, d, bt, struct_size;
    int arg;

#ifdef CONFIG_TCC_BCHECK
    if (S->do_bounds_check)
        gbound_args(S, nb_args);
#endif

    args_size = (nb_args < REGN ? REGN : nb_args) * PTR_SIZE;
    arg = nb_args;

    /* for struct arguments, we need to call memcpy and the function
       call breaks register passing arguments we are preparing.
       So, we process arguments which will be passed by stack first. */
    struct_size = args_size;
    for(i = 0; i < nb_args; i++) {
        SValue *sv;
        
        --arg;
        sv = &S->tccgen_vtop[-i];
        bt = (sv->type.t & VT_BTYPE);
        size = gfunc_arg_size(&sv->type);

        if (using_regs(size))
            continue; /* arguments smaller than 8 bytes passed in registers or on stack */

        if (bt == VT_STRUCT) {
            /* align to stack align size */
            size = (size + 15) & ~15;
            /* generate structure store */
            r = get_reg(S, RC_INT);
            gen_offs_sp(S, 0x8d, r, struct_size);
            struct_size += size;

            /* generate memcpy call */
            vset(S, &sv->type, r | VT_LVAL, 0);
            vpushv(S, sv);
            vstore(S);
            --S->tccgen_vtop;
        } else if (bt == VT_LDOUBLE) {
            gv(S, RC_ST0);
            gen_offs_sp(S, 0xdb, 0x107, struct_size);
            struct_size += 16;
        }
    }

    if (S->x86_64_gen_func_scratch < struct_size)
        S->x86_64_gen_func_scratch = struct_size;

    arg = nb_args;
    struct_size = args_size;

    for(i = 0; i < nb_args; i++) {
        --arg;
        bt = (S->tccgen_vtop->type.t & VT_BTYPE);

        size = gfunc_arg_size(&S->tccgen_vtop->type);
        if (!using_regs(size)) {
            /* align to stack align size */
            size = (size + 15) & ~15;
            if (arg >= REGN) {
                d = get_reg(S, RC_INT);
                gen_offs_sp(S, 0x8d, d, struct_size);
                gen_offs_sp(S, 0x89, d, arg*8);
            } else {
                d = arg_prepare_reg(arg);
                gen_offs_sp(S, 0x8d, d, struct_size);
            }
            struct_size += size;
        } else {
            if (is_sse_float(S->tccgen_vtop->type.t)) {
		if (S->nosse)
		  tcc_error(S, "SSE disabled");
                if (arg >= REGN) {
                    gv(S, RC_XMM0);
                    /* movq %xmm0, j*8(%rsp) */
                    gen_offs_sp(S, 0xd60f66, 0x100, arg*8);
                } else {
                    /* Load directly to xmmN register */
                    gv(S, RC_XMM0 << arg);
                    d = arg_prepare_reg(arg);
                    /* mov %xmmN, %rxx */
                    o(S, 0x66);
                    orex(S, 1,d,0, 0x7e0f);
                    o(S, 0xc0 + arg*8 + REG_VALUE(d));
                }
            } else {
                if (bt == VT_STRUCT) {
                    S->tccgen_vtop->type.ref = NULL;
                    S->tccgen_vtop->type.t = size > 4 ? VT_LLONG : size > 2 ? VT_INT
                        : size > 1 ? VT_SHORT : VT_BYTE;
                }
                
                r = gv(S, RC_INT);
                if (arg >= REGN) {
                    gen_offs_sp(S, 0x89, r, arg*8);
                } else {
                    d = arg_prepare_reg(arg);
                    orex(S, 1,d,r,0x89); /* mov */
                    o(S, 0xc0 + REG_VALUE(r) * 8 + REG_VALUE(d));
                }
            }
        }
        S->tccgen_vtop--;
    }
    save_regs(S, 0);
    /* Copy R10 and R11 into RCX and RDX, respectively */
    if (nb_args > 0) {
        o(S, 0xd1894c); /* mov %r10, %rcx */
        if (nb_args > 1) {
            o(S, 0xda894c); /* mov %r11, %rdx */
        }
    }
    
    gcall_or_jmp(S, 0);

    if ((S->tccgen_vtop->r & VT_SYM) && S->tccgen_vtop->sym->v == TOK_alloca) {
        /* need to add the "func_scratch" area after alloca */
        o(S, 0x48); S->x86_64_gen_func_alloca = oad(S, 0x05, S->x86_64_gen_func_alloca); /* add $NN, %rax */
#ifdef CONFIG_TCC_BCHECK
        if (S->do_bounds_check)
            gen_bounds_call(S, TOK___bound_alloca_nr); /* new region */
#endif
    }
    S->tccgen_vtop--;
}


#define FUNC_PROLOG_SIZE 11

/* generate function prolog of type 't' */
void gfunc_prolog(TCCState* S, Sym *func_sym)
{
    CType *func_type = &func_sym->type;
    int addr, reg_param_index, bt, size;
    Sym *sym;
    CType *type;

    S->x86_64_gen_func_ret_sub = 0;
    S->x86_64_gen_func_scratch = 32;
    S->x86_64_gen_func_alloca = 0;
    S->tccgen_loc = 0;

    addr = PTR_SIZE * 2;
    S->tccgen_ind += FUNC_PROLOG_SIZE;
    S->x86_64_gen_func_sub_sp_offset = S->tccgen_ind;
    reg_param_index = 0;

    sym = func_type->ref;

    /* if the function returns a structure, then add an
       implicit pointer parameter */
    size = gfunc_arg_size(&S->tccgen_func_vt);
    if (!using_regs(size)) {
        gen_modrm64(S, 0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
        S->tccgen_func_vc = addr;
        reg_param_index++;
        addr += 8;
    }

    /* define parameters */
    while ((sym = sym->next) != NULL) {
        type = &sym->type;
        bt = type->t & VT_BTYPE;
        size = gfunc_arg_size(type);
        if (!using_regs(size)) {
            if (reg_param_index < REGN) {
                gen_modrm64(S, 0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
            }
            sym_push(S, sym->v & ~SYM_FIELD, type,
                     VT_LLOCAL | VT_LVAL, addr);
        } else {
            if (reg_param_index < REGN) {
                /* save arguments passed by register */
                if ((bt == VT_FLOAT) || (bt == VT_DOUBLE)) {
		    if (S->nosse)
		      tcc_error(S, "SSE disabled");
                    o(S, 0xd60f66); /* movq */
                    gen_modrm(S, reg_param_index, VT_LOCAL, NULL, addr);
                } else {
                    gen_modrm64(S, 0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
                }
            }
            sym_push(S, sym->v & ~SYM_FIELD, type,
		     VT_LOCAL | VT_LVAL, addr);
        }
        addr += 8;
        reg_param_index++;
    }

    while (reg_param_index < REGN) {
        if (S->tccgen_func_var) {
            gen_modrm64(S, 0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
            addr += 8;
        }
        reg_param_index++;
    }
#ifdef CONFIG_TCC_BCHECK
    if (S->do_bounds_check)
        gen_bounds_prolog(S);
#endif
}

/* generate function epilog */
void gfunc_epilog(TCCState* S)
{
    int v, saved_ind;

    /* align local size to word & save local variables */
    S->x86_64_gen_func_scratch = (S->x86_64_gen_func_scratch + 15) & -16;
    S->tccgen_loc = (S->tccgen_loc & -16) - S->x86_64_gen_func_scratch;

#ifdef CONFIG_TCC_BCHECK
    if (S->do_bounds_check)
        gen_bounds_epilog(S);
#endif

    o(S, 0xc9); /* leave */
    if (S->x86_64_gen_func_ret_sub == 0) {
        o(S, 0xc3); /* ret */
    } else {
        o(S, 0xc2); /* ret n */
        g(S, S->x86_64_gen_func_ret_sub);
        g(S, S->x86_64_gen_func_ret_sub >> 8);
    }

    saved_ind = S->tccgen_ind;
    S->tccgen_ind = S->x86_64_gen_func_sub_sp_offset - FUNC_PROLOG_SIZE;
    v = -S->tccgen_loc;

    if (v >= 4096) {
        Sym *sym = external_helper_sym(S, TOK___chkstk);
        oad(S, 0xb8, v); /* mov stacksize, %eax */
        oad(S, 0xe8, 0); /* call __chkstk, (does the stackframe too) */
        greloca(S, cur_text_section, sym, S->tccgen_ind-4, R_X86_64_PC32, -4);
        o(S, 0x90); /* fill for FUNC_PROLOG_SIZE = 11 bytes */
    } else {
        o(S, 0xe5894855);  /* push %rbp, mov %rsp, %rbp */
        o(S, 0xec8148);  /* sub rsp, stacksize */
        gen_le32(S, v);
    }

    /* add the "func_scratch" area after each alloca seen */
    gsym_addr(S, S->x86_64_gen_func_alloca, -S->x86_64_gen_func_scratch);

    cur_text_section->data_offset = saved_ind;
    pe_add_unwind_data(S, S->tccgen_ind, saved_ind, v);
    S->tccgen_ind = cur_text_section->data_offset;
}

#else

static void gadd_sp(TCCState* S, int val)
{
    if (val == (char)val) {
        o(S, 0xc48348);
        g(S, val);
    } else {
        oad(S, 0xc48148, val); /* add $xxx, %rsp */
    }
}

typedef enum X86_64_Mode {
  x86_64_mode_none,
  x86_64_mode_memory,
  x86_64_mode_integer,
  x86_64_mode_sse,
  x86_64_mode_x87
} X86_64_Mode;

static X86_64_Mode classify_x86_64_merge(X86_64_Mode a, X86_64_Mode b)
{
    if (a == b)
        return a;
    else if (a == x86_64_mode_none)
        return b;
    else if (b == x86_64_mode_none)
        return a;
    else if ((a == x86_64_mode_memory) || (b == x86_64_mode_memory))
        return x86_64_mode_memory;
    else if ((a == x86_64_mode_integer) || (b == x86_64_mode_integer))
        return x86_64_mode_integer;
    else if ((a == x86_64_mode_x87) || (b == x86_64_mode_x87))
        return x86_64_mode_memory;
    else
        return x86_64_mode_sse;
}

static X86_64_Mode classify_x86_64_inner(CType *ty)
{
    X86_64_Mode mode;
    Sym *f;
    
    switch (ty->t & VT_BTYPE) {
    case VT_VOID: return x86_64_mode_none;
    
    case VT_INT:
    case VT_BYTE:
    case VT_SHORT:
    case VT_LLONG:
    case VT_BOOL:
    case VT_PTR:
    case VT_FUNC:
        return x86_64_mode_integer;
    
    case VT_FLOAT:
    case VT_DOUBLE: return x86_64_mode_sse;
    
    case VT_LDOUBLE: return x86_64_mode_x87;
      
    case VT_STRUCT:
        f = ty->ref;

        mode = x86_64_mode_none;
        for (f = f->next; f; f = f->next)
            mode = classify_x86_64_merge(mode, classify_x86_64_inner(&f->type));
        
        return mode;
    }
    assert(0);
    return 0;
}

static X86_64_Mode classify_x86_64_arg(CType *ty, CType *ret, int *psize, int *palign, int *reg_count)
{
    X86_64_Mode mode;
    int size, align, ret_t = 0;
    
    if (ty->t & (VT_BITFIELD|VT_ARRAY)) {
        *psize = 8;
        *palign = 8;
        *reg_count = 1;
        ret_t = ty->t;
        mode = x86_64_mode_integer;
    } else {
        size = type_size(ty, &align);
        *psize = (size + 7) & ~7;
        *palign = (align + 7) & ~7;
    
        if (size > 16) {
            mode = x86_64_mode_memory;
        } else {
            mode = classify_x86_64_inner(ty);
            switch (mode) {
            case x86_64_mode_integer:
                if (size > 8) {
                    *reg_count = 2;
                    ret_t = VT_QLONG;
                } else {
                    *reg_count = 1;
                    if (size > 4)
                        ret_t = VT_LLONG;
                    else if (size > 2)
                        ret_t = VT_INT;
                    else if (size > 1)
                        ret_t = VT_SHORT;
                    else
                        ret_t = VT_BYTE;
                    if ((ty->t & VT_BTYPE) == VT_STRUCT || (ty->t & VT_UNSIGNED))
                        ret_t |= VT_UNSIGNED;
                }
                break;
                
            case x86_64_mode_x87:
                *reg_count = 1;
                ret_t = VT_LDOUBLE;
                break;

            case x86_64_mode_sse:
                if (size > 8) {
                    *reg_count = 2;
                    ret_t = VT_QFLOAT;
                } else {
                    *reg_count = 1;
                    ret_t = (size > 4) ? VT_DOUBLE : VT_FLOAT;
                }
                break;
            default: break; /* nothing to be done for x86_64_mode_memory and x86_64_mode_none*/
            }
        }
    }
    
    if (ret) {
        ret->ref = NULL;
        ret->t = ret_t;
    }
    
    return mode;
}

ST_FUNC int classify_x86_64_va_arg(CType *ty)
{
    /* This definition must be synced with stdarg.h */
    enum __va_arg_type {
        __va_gen_reg, __va_float_reg, __va_stack
    };
    int size, align, reg_count;
    X86_64_Mode mode = classify_x86_64_arg(ty, NULL, &size, &align, &reg_count);
    switch (mode) {
    default: return __va_stack;
    case x86_64_mode_integer: return __va_gen_reg;
    case x86_64_mode_sse: return __va_float_reg;
    }
}

/* Return the number of registers needed to return the struct, or 0 if
   returning via struct pointer. */
ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret, int *ret_align, int *regsize)
{
    int size, align, reg_count;
    *ret_align = 1; // Never have to re-align return values for x86-64
    *regsize = 8;
    return (classify_x86_64_arg(vt, ret, &size, &align, &reg_count) != x86_64_mode_memory);
}

#define REGN 6
static const uint8_t arg_regs[REGN] = {
    TREG_RDI, TREG_RSI, TREG_RDX, TREG_RCX, TREG_R8, TREG_R9
};

static int arg_prepare_reg(int idx) {
  if (idx == 2 || idx == 3)
      /* idx=2: r10, idx=3: r11 */
      return idx + 8;
  else
      return idx >= 0 && idx < REGN ? arg_regs[idx] : 0;
}

/* Generate function call. The function address is pushed first, then
   all the parameters in call order. This functions pops all the
   parameters and the function address. */
void gfunc_call(TCCState* S, int nb_args)
{
    X86_64_Mode mode;
    CType type;
    int size, align, r, args_size, stack_adjust, i, reg_count, k;
    int nb_reg_args = 0;
    int nb_sse_args = 0;
    int sse_reg, gen_reg;
    char *onstack = tcc_malloc(S, (nb_args + 1) * sizeof (char));

#ifdef CONFIG_TCC_BCHECK
    if (S->do_bounds_check)
        gbound_args(S, nb_args);
#endif

    /* calculate the number of integer/float register arguments, remember
       arguments to be passed via stack (in onstack[]), and also remember
       if we have to align the stack pointer to 16 (onstack[i] == 2).  Needs
       to be done in a left-to-right pass over arguments.  */
    stack_adjust = 0;
    for(i = nb_args - 1; i >= 0; i--) {
        mode = classify_x86_64_arg(&S->tccgen_vtop[-i].type, NULL, &size, &align, &reg_count);
        if (size == 0) continue;
        if (mode == x86_64_mode_sse && nb_sse_args + reg_count <= 8) {
            nb_sse_args += reg_count;
	    onstack[i] = 0;
	} else if (mode == x86_64_mode_integer && nb_reg_args + reg_count <= REGN) {
            nb_reg_args += reg_count;
	    onstack[i] = 0;
	} else if (mode == x86_64_mode_none) {
	    onstack[i] = 0;
	} else {
	    if (align == 16 && (stack_adjust &= 15)) {
		onstack[i] = 2;
		stack_adjust = 0;
	    } else
	      onstack[i] = 1;
	    stack_adjust += size;
	}
    }

    if (nb_sse_args && S->nosse)
      tcc_error(S, "SSE disabled but floating point arguments passed");

    /* fetch cpu flag before generating any code */
    if ((S->tccgen_vtop->r & VT_VALMASK) == VT_CMP)
      gv(S, RC_INT);

    /* for struct arguments, we need to call memcpy and the function
       call breaks register passing arguments we are preparing.
       So, we process arguments which will be passed by stack first. */
    gen_reg = nb_reg_args;
    sse_reg = nb_sse_args;
    args_size = 0;
    stack_adjust &= 15;
    for (i = k = 0; i < nb_args;) {
	mode = classify_x86_64_arg(&S->tccgen_vtop[-i].type, NULL, &size, &align, &reg_count);
	if (size) {
            if (!onstack[i + k]) {
	        ++i;
	        continue;
	    }
            /* Possibly adjust stack to align SSE boundary.  We're processing
	       args from right to left while allocating happens left to right
	       (stack grows down), so the adjustment needs to happen _after_
	       an argument that requires it.  */
            if (stack_adjust) {
	        o(S, 0x50); /* push %rax; aka sub $8,%rsp */
                args_size += 8;
	        stack_adjust = 0;
            }
	    if (onstack[i + k] == 2)
	        stack_adjust = 1;
        }

	vrotb(S, i+1);

	switch (S->tccgen_vtop->type.t & VT_BTYPE) {
	    case VT_STRUCT:
		/* allocate the necessary size on stack */
		o(S, 0x48);
		oad(S, 0xec81, size); /* sub $xxx, %rsp */
		/* generate structure store */
		r = get_reg(S, RC_INT);
		orex(S, 1, r, 0, 0x89); /* mov %rsp, r */
		o(S, 0xe0 + REG_VALUE(r));
		vset(S, &S->tccgen_vtop->type, r | VT_LVAL, 0);
		vswap(S);
		vstore(S);
		break;

	    case VT_LDOUBLE:
                gv(S, RC_ST0);
                oad(S, 0xec8148, size); /* sub $xxx, %rsp */
                o(S, 0x7cdb); /* fstpt 0(%rsp) */
                g(S, 0x24);
                g(S, 0x00);
		break;

	    case VT_FLOAT:
	    case VT_DOUBLE:
		assert(mode == x86_64_mode_sse);
		r = gv(S, RC_FLOAT);
		o(S, 0x50); /* push $rax */
		/* movq %xmmN, (%rsp) */
		o(S, 0xd60f66);
		o(S, 0x04 + REG_VALUE(r)*8);
		o(S, 0x24);
		break;

	    default:
		assert(mode == x86_64_mode_integer);
		/* simple type */
		/* XXX: implicit cast ? */
		r = gv(S, RC_INT);
		orex(S, 0,r,0,0x50 + REG_VALUE(r)); /* push r */
		break;
	}
	args_size += size;

	vpop(S);
	--nb_args;
	k++;
    }

    tcc_free(S, onstack);

    /* XXX This should be superfluous.  */
    save_regs(S, 0); /* save used temporary registers */

    /* then, we prepare register passing arguments.
       Note that we cannot set RDX and RCX in this loop because gv()
       may break these temporary registers. Let's use R10 and R11
       instead of them */
    assert(gen_reg <= REGN);
    assert(sse_reg <= 8);
    for(i = 0; i < nb_args; i++) {
        mode = classify_x86_64_arg(&S->tccgen_vtop->type, &type, &size, &align, &reg_count);
        if (size == 0) continue;
        /* Alter stack entry type so that gv() knows how to treat it */
        S->tccgen_vtop->type = type;
        if (mode == x86_64_mode_sse) {
            if (reg_count == 2) {
                sse_reg -= 2;
                gv(S, RC_FRET); /* Use pair load into xmm0 & xmm1 */
                if (sse_reg) { /* avoid redundant movaps %xmm0, %xmm0 */
                    /* movaps %xmm1, %xmmN */
                    o(S, 0x280f);
                    o(S, 0xc1 + ((sse_reg+1) << 3));
                    /* movaps %xmm0, %xmmN */
                    o(S, 0x280f);
                    o(S, 0xc0 + (sse_reg << 3));
                }
            } else {
                assert(reg_count == 1);
                --sse_reg;
                /* Load directly to register */
                gv(S, RC_XMM0 << sse_reg);
            }
        } else if (mode == x86_64_mode_integer) {
            /* simple type */
            /* XXX: implicit cast ? */
            int d;
            gen_reg -= reg_count;
            r = gv(S, RC_INT);
            d = arg_prepare_reg(gen_reg);
            orex(S, 1,d,r,0x89); /* mov */
            o(S, 0xc0 + REG_VALUE(r) * 8 + REG_VALUE(d));
            if (reg_count == 2) {
                d = arg_prepare_reg(gen_reg+1);
                orex(S, 1,d,S->tccgen_vtop->r2,0x89); /* mov */
                o(S, 0xc0 + REG_VALUE(S->tccgen_vtop->r2) * 8 + REG_VALUE(d));
            }
        }
        S->tccgen_vtop--;
    }
    assert(gen_reg == 0);
    assert(sse_reg == 0);

    /* We shouldn't have many operands on the stack anymore, but the
       call address itself is still there, and it might be in %eax
       (or edx/ecx) currently, which the below writes would clobber.
       So evict all remaining operands here.  */
    save_regs(S, 0);

    /* Copy R10 and R11 into RDX and RCX, respectively */
    if (nb_reg_args > 2) {
        o(S, 0xd2894c); /* mov %r10, %rdx */
        if (nb_reg_args > 3) {
            o(S, 0xd9894c); /* mov %r11, %rcx */
        }
    }

    if (S->tccgen_vtop->type.ref->f.func_type != FUNC_NEW) /* implies FUNC_OLD or FUNC_ELLIPSIS */
        oad(S, 0xb8, nb_sse_args < 8 ? nb_sse_args : 8); /* mov nb_sse_args, %eax */
    gcall_or_jmp(S, 0);
    if (args_size)
        gadd_sp(S, args_size);
    S->tccgen_vtop--;
}

#define FUNC_PROLOG_SIZE 11

static void push_arg_reg(TCCState* S, int i) {
    S->tccgen_loc -= 8;
    gen_modrm64(S, 0x89, arg_regs[i], VT_LOCAL, NULL, S->tccgen_loc);
}

/* generate function prolog of type 't' */
void gfunc_prolog(TCCState* S, Sym *func_sym)
{
    CType *func_type = &func_sym->type;
    X86_64_Mode mode, ret_mode;
    int i, addr, align, size, reg_count;
    int param_addr = 0, reg_param_index, sse_param_index;
    Sym *sym;
    CType *type;

    sym = func_type->ref;
    addr = PTR_SIZE * 2;
    S->tccgen_loc = 0;
    S->tccgen_ind += FUNC_PROLOG_SIZE;
    S->x86_64_gen_func_sub_sp_offset = S->tccgen_ind;
    S->x86_64_gen_func_ret_sub = 0;
    ret_mode = classify_x86_64_arg(&S->tccgen_func_vt, NULL, &size, &align, &reg_count);

    if (S->tccgen_func_var) {
        int seen_reg_num, seen_sse_num, seen_stack_size;
        seen_reg_num = ret_mode == x86_64_mode_memory;
        seen_sse_num = 0;
        /* frame pointer and return address */
        seen_stack_size = PTR_SIZE * 2;
        /* count the number of seen parameters */
        sym = func_type->ref;
        while ((sym = sym->next) != NULL) {
            type = &sym->type;
            mode = classify_x86_64_arg(type, NULL, &size, &align, &reg_count);
            switch (mode) {
            default:
            stack_arg:
                seen_stack_size = ((seen_stack_size + align - 1) & -align) + size;
                break;
                
            case x86_64_mode_integer:
                if (seen_reg_num + reg_count > REGN)
		    goto stack_arg;
		seen_reg_num += reg_count;
                break;
                
            case x86_64_mode_sse:
                if (seen_sse_num + reg_count > 8)
		    goto stack_arg;
		seen_sse_num += reg_count;
                break;
            }
        }

        S->tccgen_loc -= 24;
        /* movl $0x????????, -0x18(%rbp) */
        o(S, 0xe845c7);
        gen_le32(S, seen_reg_num * 8);
        /* movl $0x????????, -0x14(%rbp) */
        o(S, 0xec45c7);
        gen_le32(S, seen_sse_num * 16 + 48);
	/* leaq $0x????????, %r11 */
	o(S, 0x9d8d4c);
	gen_le32(S, seen_stack_size);
	/* movq %r11, -0x10(%rbp) */
	o(S, 0xf05d894c);
	/* leaq $-192(%rbp), %r11 */
	o(S, 0x9d8d4c);
	gen_le32(S, -176 - 24);
	/* movq %r11, -0x8(%rbp) */
	o(S, 0xf85d894c);

        /* save all register passing arguments */
        for (i = 0; i < 8; i++) {
            S->tccgen_loc -= 16;
	    if (!S->nosse) {
		o(S, 0xd60f66); /* movq */
		gen_modrm(S, 7 - i, VT_LOCAL, NULL, S->tccgen_loc);
	    }
            /* movq $0, loc+8(%rbp) */
            o(S, 0x85c748);
            gen_le32(S, S->tccgen_loc + 8);
            gen_le32(S, 0);
        }
        for (i = 0; i < REGN; i++) {
            push_arg_reg(S, REGN-1-i);
        }
    }

    sym = func_type->ref;
    reg_param_index = 0;
    sse_param_index = 0;

    /* if the function returns a structure, then add an
       implicit pointer parameter */
    if (ret_mode == x86_64_mode_memory) {
        push_arg_reg(S, reg_param_index);
        S->tccgen_func_vc = S->tccgen_loc;
        reg_param_index++;
    }
    /* define parameters */
    while ((sym = sym->next) != NULL) {
        type = &sym->type;
        mode = classify_x86_64_arg(type, NULL, &size, &align, &reg_count);
        switch (mode) {
        case x86_64_mode_sse:
	    if (S->nosse)
	        tcc_error(S, "SSE disabled but floating point arguments used");
            if (sse_param_index + reg_count <= 8) {
                /* save arguments passed by register */
                S->tccgen_loc -= reg_count * 8;
                param_addr = S->tccgen_loc;
                for (i = 0; i < reg_count; ++i) {
                    o(S, 0xd60f66); /* movq */
                    gen_modrm(S, sse_param_index, VT_LOCAL, NULL, param_addr + i*8);
                    ++sse_param_index;
                }
            } else {
                addr = (addr + align - 1) & -align;
                param_addr = addr;
                addr += size;
            }
            break;
            
        case x86_64_mode_memory:
        case x86_64_mode_x87:
            addr = (addr + align - 1) & -align;
            param_addr = addr;
            addr += size;
            break;
            
        case x86_64_mode_integer: {
            if (reg_param_index + reg_count <= REGN) {
                /* save arguments passed by register */
                S->tccgen_loc -= reg_count * 8;
                param_addr = S->tccgen_loc;
                for (i = 0; i < reg_count; ++i) {
                    gen_modrm64(S, 0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, param_addr + i*8);
                    ++reg_param_index;
                }
            } else {
                addr = (addr + align - 1) & -align;
                param_addr = addr;
                addr += size;
            }
            break;
        }
	default: break; /* nothing to be done for x86_64_mode_none */
        }
        sym_push(S, sym->v & ~SYM_FIELD, type,
                 VT_LOCAL | VT_LVAL, param_addr);
    }

#ifdef CONFIG_TCC_BCHECK
    if (S->do_bounds_check)
        gen_bounds_prolog(S);
#endif
}

/* generate function epilog */
void gfunc_epilog(TCCState* S)
{
    int v, saved_ind;

#ifdef CONFIG_TCC_BCHECK
    if (S->do_bounds_check)
        gen_bounds_epilog(S);
#endif
    o(S, 0xc9); /* leave */
    if (S->x86_64_gen_func_ret_sub == 0) {
        o(S, 0xc3); /* ret */
    } else {
        o(S, 0xc2); /* ret n */
        g(S, S->x86_64_gen_func_ret_sub);
        g(S, S->x86_64_gen_func_ret_sub >> 8);
    }
    /* align local size to word & save local variables */
    v = (-S->tccgen_loc + 15) & -16;
    saved_ind = S->tccgen_ind;
    S->tccgen_ind = S->x86_64_gen_func_sub_sp_offset - FUNC_PROLOG_SIZE;
    o(S, 0xe5894855);  /* push %rbp, mov %rsp, %rbp */
    o(S, 0xec8148);  /* sub rsp, stacksize */
    gen_le32(S, v);
    S->tccgen_ind = saved_ind;
}

#endif /* not PE */

ST_FUNC void gen_fill_nops(TCCState* S, int bytes)
{
    while (bytes--)
      g(S, 0x90);
}

/* generate a jump to a label */
int gjmp(TCCState* S, int t)
{
    return gjmp2(S, 0xe9, t);
}

/* generate a jump to a fixed address */
void gjmp_addr(TCCState* S, int a)
{
    int r;
    r = a - S->tccgen_ind - 2;
    if (r == (char)r) {
        g(S, 0xeb);
        g(S, r);
    } else {
        oad(S, 0xe9, a - S->tccgen_ind - 5);
    }
}

ST_FUNC int gjmp_append(TCCState *S, int n, int t)
{
    void *p;
    /* insert vtop->c jump list in t */
    if (n) {
        uint32_t n1 = n, n2;
        while ((n2 = read32le(p = cur_text_section->data + n1)))
            n1 = n2;
        write32le(p, t);
        t = n;
    }
    return t;
}

ST_FUNC int gjmp_cond(TCCState* S, int op, int t)
{
        if (op & 0x100)
	  {
	    /* This was a float compare.  If the parity flag is set
	       the result was unordered.  For anything except != this
	       means false and we don't jump (anding both conditions).
	       For != this means true (oring both).
	       Take care about inverting the test.  We need to jump
	       to our target if the result was unordered and test wasn't NE,
	       otherwise if unordered we don't want to jump.  */
            int v = S->tccgen_vtop->cmp_r;
            op &= ~0x100;
            if (op ^ v ^ (v != TOK_NE))
              o(S, 0x067a);  /* jp +6 */
	    else
	      {
	        g(S, 0x0f);
		t = gjmp2(S, 0x8a, t); /* jp t */
	      }
	  }
        g(S, 0x0f);
        t = gjmp2(S, op - 16, t);
        return t;
}

/* generate an integer binary operation */
void gen_opi(TCCState* S, int op)
{
    int r, fr, opc, c;
    int ll, uu, cc;

    ll = is64_type(S->tccgen_vtop[-1].type.t);
    uu = (S->tccgen_vtop[-1].type.t & VT_UNSIGNED) != 0;
    cc = (S->tccgen_vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;

    switch(op) {
    case '+':
    case TOK_ADDC1: /* add with carry generation */
        opc = 0;
    gen_op8:
        if (cc && (!ll || (int)S->tccgen_vtop->c.i == S->tccgen_vtop->c.i)) {
            /* constant case */
            vswap(S);
            r = gv(S, RC_INT);
            vswap(S);
            c = S->tccgen_vtop->c.i;
            if (c == (char)c) {
                /* XXX: generate inc and dec for smaller code ? */
                orex(S, ll, r, 0, 0x83);
                o(S, 0xc0 | (opc << 3) | REG_VALUE(r));
                g(S, c);
            } else {
                orex(S, ll, r, 0, 0x81);
                oad(S, 0xc0 | (opc << 3) | REG_VALUE(r), c);
            }
        } else {
            gv2(S, RC_INT, RC_INT);
            r = S->tccgen_vtop[-1].r;
            fr = S->tccgen_vtop[0].r;
            orex(S, ll, r, fr, (opc << 3) | 0x01);
            o(S, 0xc0 + REG_VALUE(r) + REG_VALUE(fr) * 8);
        }
        S->tccgen_vtop--;
        if (op >= TOK_ULT && op <= TOK_GT)
            vset_VT_CMP(S, op);
        break;
    case '-':
    case TOK_SUBC1: /* sub with carry generation */
        opc = 5;
        goto gen_op8;
    case TOK_ADDC2: /* add with carry use */
        opc = 2;
        goto gen_op8;
    case TOK_SUBC2: /* sub with carry use */
        opc = 3;
        goto gen_op8;
    case '&':
        opc = 4;
        goto gen_op8;
    case '^':
        opc = 6;
        goto gen_op8;
    case '|':
        opc = 1;
        goto gen_op8;
    case '*':
        gv2(S, RC_INT, RC_INT);
        r = S->tccgen_vtop[-1].r;
        fr = S->tccgen_vtop[0].r;
        orex(S, ll, fr, r, 0xaf0f); /* imul fr, r */
        o(S, 0xc0 + REG_VALUE(fr) + REG_VALUE(r) * 8);
        S->tccgen_vtop--;
        break;
    case TOK_SHL:
        opc = 4;
        goto gen_shift;
    case TOK_SHR:
        opc = 5;
        goto gen_shift;
    case TOK_SAR:
        opc = 7;
    gen_shift:
        opc = 0xc0 | (opc << 3);
        if (cc) {
            /* constant case */
            vswap(S);
            r = gv(S, RC_INT);
            vswap(S);
            orex(S, ll, r, 0, 0xc1); /* shl/shr/sar $xxx, r */
            o(S, opc | REG_VALUE(r));
            g(S, S->tccgen_vtop->c.i & (ll ? 63 : 31));
        } else {
            /* we generate the shift in ecx */
            gv2(S, RC_INT, RC_RCX);
            r = S->tccgen_vtop[-1].r;
            orex(S, ll, r, 0, 0xd3); /* shl/shr/sar %cl, r */
            o(S, opc | REG_VALUE(r));
        }
        S->tccgen_vtop--;
        break;
    case TOK_UDIV:
    case TOK_UMOD:
        uu = 1;
        goto divmod;
    case '/':
    case '%':
    case TOK_PDIV:
        uu = 0;
    divmod:
        /* first operand must be in eax */
        /* XXX: need better constraint for second operand */
        gv2(S, RC_RAX, RC_RCX);
        r = S->tccgen_vtop[-1].r;
        fr = S->tccgen_vtop[0].r;
        S->tccgen_vtop--;
        save_reg(S, TREG_RDX);
        orex(S, ll, 0, 0, uu ? 0xd231 : 0x99); /* xor %edx,%edx : cqto */
        orex(S, ll, fr, 0, 0xf7); /* div fr, %eax */
        o(S, (uu ? 0xf0 : 0xf8) + REG_VALUE(fr));
        if (op == '%' || op == TOK_UMOD)
            r = TREG_RDX;
        else
            r = TREG_RAX;
        S->tccgen_vtop->r = r;
        break;
    default:
        opc = 7;
        goto gen_op8;
    }
}

void gen_opl(TCCState* S, int op)
{
    gen_opi(S, op);
}

void vpush_const(TCCState* S, int t, int v)
{
    CType ctype = { t | VT_CONSTANT, 0 };
    vpushsym(S, &ctype, external_global_sym(S, v, &ctype));
    S->tccgen_vtop->r |= VT_LVAL;
}

/* generate a floating point operation 'v = t1 op t2' instruction. The
   two operands are guaranteed to have the same floating point type */
/* XXX: need to use ST1 too */
void gen_opf(TCCState* S, int op)
{
    int a, ft, fc, swapped, r;
    int bt = S->tccgen_vtop->type.t & VT_BTYPE;
    int float_type = bt == VT_LDOUBLE ? RC_ST0 : RC_FLOAT;

    if (op == TOK_NEG) { /* unary minus */
        gv(S, float_type);
        if (float_type == RC_ST0) {
            o(S, 0xe0d9); /* fchs */
        } else {
            /* -0.0, in libtcc1.c */
            vpush_const(S, bt, bt == VT_FLOAT ? TOK___mzerosf : TOK___mzerodf);
            gv(S, RC_FLOAT);
            if (bt == VT_DOUBLE)
                o(S, 0x66);
            /* xorp[sd] %xmm1, %xmm0 */
            o(S, 0xc0570f | (REG_VALUE(S->tccgen_vtop[0].r) + REG_VALUE(S->tccgen_vtop[-1].r)*8) << 16);
            S->tccgen_vtop--;
        }
        return;
    }

    /* convert constants to memory references */
    if ((S->tccgen_vtop[-1].r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
        vswap(S);
        gv(S, float_type);
        vswap(S);
    }
    if ((S->tccgen_vtop[0].r & (VT_VALMASK | VT_LVAL)) == VT_CONST)
        gv(S, float_type);

    /* must put at least one value in the floating point register */
    if ((S->tccgen_vtop[-1].r & VT_LVAL) &&
        (S->tccgen_vtop[0].r & VT_LVAL)) {
        vswap(S);
        gv(S, float_type);
        vswap(S);
    }
    swapped = 0;
    /* swap the stack if needed so that t1 is the register and t2 is
       the memory reference */
    if (S->tccgen_vtop[-1].r & VT_LVAL) {
        vswap(S);
        swapped = 1;
    }
    if ((S->tccgen_vtop->type.t & VT_BTYPE) == VT_LDOUBLE) {
        if (op >= TOK_ULT && op <= TOK_GT) {
            /* load on stack second operand */
            load(S, TREG_ST0, S->tccgen_vtop);
            save_reg(S, TREG_RAX); /* eax is used by FP comparison code */
            if (op == TOK_GE || op == TOK_GT)
                swapped = !swapped;
            else if (op == TOK_EQ || op == TOK_NE)
                swapped = 0;
            if (swapped)
                o(S, 0xc9d9); /* fxch %st(1) */
            if (op == TOK_EQ || op == TOK_NE)
                o(S, 0xe9da); /* fucompp */
            else
                o(S, 0xd9de); /* fcompp */
            o(S, 0xe0df); /* fnstsw %ax */
            if (op == TOK_EQ) {
                o(S, 0x45e480); /* and $0x45, %ah */
                o(S, 0x40fC80); /* cmp $0x40, %ah */
            } else if (op == TOK_NE) {
                o(S, 0x45e480); /* and $0x45, %ah */
                o(S, 0x40f480); /* xor $0x40, %ah */
                op = TOK_NE;
            } else if (op == TOK_GE || op == TOK_LE) {
                o(S, 0x05c4f6); /* test $0x05, %ah */
                op = TOK_EQ;
            } else {
                o(S, 0x45c4f6); /* test $0x45, %ah */
                op = TOK_EQ;
            }
            S->tccgen_vtop--;
            vset_VT_CMP(S, op);
        } else {
            /* no memory reference possible for long double operations */
            load(S, TREG_ST0, S->tccgen_vtop);
            swapped = !swapped;

            switch(op) {
            default:
            case '+':
                a = 0;
                break;
            case '-':
                a = 4;
                if (swapped)
                    a++;
                break;
            case '*':
                a = 1;
                break;
            case '/':
                a = 6;
                if (swapped)
                    a++;
                break;
            }
            ft = S->tccgen_vtop->type.t;
            fc = S->tccgen_vtop->c.i;
            o(S, 0xde); /* fxxxp %st, %st(1) */
            o(S, 0xc1 + (a << 3));
            S->tccgen_vtop--;
        }
    } else {
        if (op >= TOK_ULT && op <= TOK_GT) {
            /* if saved lvalue, then we must reload it */
            r = S->tccgen_vtop->r;
            fc = S->tccgen_vtop->c.i;
            if ((r & VT_VALMASK) == VT_LLOCAL) {
                SValue v1;
                r = get_reg(S, RC_INT);
                v1.type.t = VT_PTR;
                v1.r = VT_LOCAL | VT_LVAL;
                v1.c.i = fc;
                load(S, r, &v1);
                fc = 0;
                S->tccgen_vtop->r = r = r | VT_LVAL;
            }

            if (op == TOK_EQ || op == TOK_NE) {
                swapped = 0;
            } else {
                if (op == TOK_LE || op == TOK_LT)
                    swapped = !swapped;
                if (op == TOK_LE || op == TOK_GE) {
                    op = 0x93; /* setae */
                } else {
                    op = 0x97; /* seta */
                }
            }

            if (swapped) {
                gv(S, RC_FLOAT);
                vswap(S);
            }
            assert(!(S->tccgen_vtop[-1].r & VT_LVAL));
            
            if ((S->tccgen_vtop->type.t & VT_BTYPE) == VT_DOUBLE)
                o(S, 0x66);
            if (op == TOK_EQ || op == TOK_NE)
                o(S, 0x2e0f); /* ucomisd */
            else
                o(S, 0x2f0f); /* comisd */

            if (S->tccgen_vtop->r & VT_LVAL) {
                gen_modrm(S, S->tccgen_vtop[-1].r, r, S->tccgen_vtop->sym, fc);
            } else {
                o(S, 0xc0 + REG_VALUE(S->tccgen_vtop[0].r) + REG_VALUE(S->tccgen_vtop[-1].r)*8);
            }

            S->tccgen_vtop--;
            vset_VT_CMP(S, op | 0x100);
            S->tccgen_vtop->cmp_r = op;
        } else {
            assert((S->tccgen_vtop->type.t & VT_BTYPE) != VT_LDOUBLE);
            switch(op) {
            default:
            case '+':
                a = 0;
                break;
            case '-':
                a = 4;
                break;
            case '*':
                a = 1;
                break;
            case '/':
                a = 6;
                break;
            }
            ft = S->tccgen_vtop->type.t;
            fc = S->tccgen_vtop->c.i;
            assert((ft & VT_BTYPE) != VT_LDOUBLE);
            
            r = S->tccgen_vtop->r;
            /* if saved lvalue, then we must reload it */
            if ((S->tccgen_vtop->r & VT_VALMASK) == VT_LLOCAL) {
                SValue v1;
                r = get_reg(S, RC_INT);
                v1.type.t = VT_PTR;
                v1.r = VT_LOCAL | VT_LVAL;
                v1.c.i = fc;
                load(S, r, &v1);
                fc = 0;
                S->tccgen_vtop->r = r = r | VT_LVAL;
            }
            
            assert(!(S->tccgen_vtop[-1].r & VT_LVAL));
            if (swapped) {
                assert(S->tccgen_vtop->r & VT_LVAL);
                gv(S, RC_FLOAT);
                vswap(S);
            }
            
            if ((ft & VT_BTYPE) == VT_DOUBLE) {
                o(S, 0xf2);
            } else {
                o(S, 0xf3);
            }
            o(S, 0x0f);
            o(S, 0x58 + a);
            
            if (S->tccgen_vtop->r & VT_LVAL) {
                gen_modrm(S, S->tccgen_vtop[-1].r, r, S->tccgen_vtop->sym, fc);
            } else {
                o(S, 0xc0 + REG_VALUE(S->tccgen_vtop[0].r) + REG_VALUE(S->tccgen_vtop[-1].r)*8);
            }

            S->tccgen_vtop--;
        }
    }
}

/* convert integers to fp 't' type. Must handle 'int', 'unsigned int'
   and 'long long' cases. */
void gen_cvt_itof(TCCState* S, int t)
{
    if ((t & VT_BTYPE) == VT_LDOUBLE) {
        save_reg(S, TREG_ST0);
        gv(S, RC_INT);
        if ((S->tccgen_vtop->type.t & VT_BTYPE) == VT_LLONG) {
            /* signed long long to float/double/long double (unsigned case
               is handled generically) */
            o(S, 0x50 + (S->tccgen_vtop->r & VT_VALMASK)); /* push r */
            o(S, 0x242cdf); /* fildll (%rsp) */
            o(S, 0x08c48348); /* add $8, %rsp */
        } else if ((S->tccgen_vtop->type.t & (VT_BTYPE | VT_UNSIGNED)) ==
                   (VT_INT | VT_UNSIGNED)) {
            /* unsigned int to float/double/long double */
            o(S, 0x6a); /* push $0 */
            g(S, 0x00);
            o(S, 0x50 + (S->tccgen_vtop->r & VT_VALMASK)); /* push r */
            o(S, 0x242cdf); /* fildll (%rsp) */
            o(S, 0x10c48348); /* add $16, %rsp */
        } else {
            /* int to float/double/long double */
            o(S, 0x50 + (S->tccgen_vtop->r & VT_VALMASK)); /* push r */
            o(S, 0x2404db); /* fildl (%rsp) */
            o(S, 0x08c48348); /* add $8, %rsp */
        }
        S->tccgen_vtop->r = TREG_ST0;
    } else {
        int r = get_reg(S, RC_FLOAT);
        gv(S, RC_INT);
        o(S, 0xf2 + ((t & VT_BTYPE) == VT_FLOAT?1:0));
        if ((S->tccgen_vtop->type.t & (VT_BTYPE | VT_UNSIGNED)) ==
            (VT_INT | VT_UNSIGNED) ||
            (S->tccgen_vtop->type.t & VT_BTYPE) == VT_LLONG) {
            o(S, 0x48); /* REX */
        }
        o(S, 0x2a0f);
        o(S, 0xc0 + (S->tccgen_vtop->r & VT_VALMASK) + REG_VALUE(r)*8); /* cvtsi2sd */
        S->tccgen_vtop->r = r;
    }
}

/* convert from one floating point type to another */
void gen_cvt_ftof(TCCState* S, int t)
{
    int ft, bt, tbt;

    ft = S->tccgen_vtop->type.t;
    bt = ft & VT_BTYPE;
    tbt = t & VT_BTYPE;
    
    if (bt == VT_FLOAT) {
        gv(S, RC_FLOAT);
        if (tbt == VT_DOUBLE) {
            o(S, 0x140f); /* unpcklps */
            o(S, 0xc0 + REG_VALUE(S->tccgen_vtop->r)*9);
            o(S, 0x5a0f); /* cvtps2pd */
            o(S, 0xc0 + REG_VALUE(S->tccgen_vtop->r)*9);
        } else if (tbt == VT_LDOUBLE) {
            save_reg(S, RC_ST0);
            /* movss %xmm0,-0x10(%rsp) */
            o(S, 0x110ff3);
            o(S, 0x44 + REG_VALUE(S->tccgen_vtop->r)*8);
            o(S, 0xf024);
            o(S, 0xf02444d9); /* flds -0x10(%rsp) */
            S->tccgen_vtop->r = TREG_ST0;
        }
    } else if (bt == VT_DOUBLE) {
        gv(S, RC_FLOAT);
        if (tbt == VT_FLOAT) {
            o(S, 0x140f66); /* unpcklpd */
            o(S, 0xc0 + REG_VALUE(S->tccgen_vtop->r)*9);
            o(S, 0x5a0f66); /* cvtpd2ps */
            o(S, 0xc0 + REG_VALUE(S->tccgen_vtop->r)*9);
        } else if (tbt == VT_LDOUBLE) {
            save_reg(S, RC_ST0);
            /* movsd %xmm0,-0x10(%rsp) */
            o(S, 0x110ff2);
            o(S, 0x44 + REG_VALUE(S->tccgen_vtop->r)*8);
            o(S, 0xf024);
            o(S, 0xf02444dd); /* fldl -0x10(%rsp) */
            S->tccgen_vtop->r = TREG_ST0;
        }
    } else {
        int r;
        gv(S, RC_ST0);
        r = get_reg(S, RC_FLOAT);
        if (tbt == VT_DOUBLE) {
            o(S, 0xf0245cdd); /* fstpl -0x10(%rsp) */
            /* movsd -0x10(%rsp),%xmm0 */
            o(S, 0x100ff2);
            o(S, 0x44 + REG_VALUE(r)*8);
            o(S, 0xf024);
            S->tccgen_vtop->r = r;
        } else if (tbt == VT_FLOAT) {
            o(S, 0xf0245cd9); /* fstps -0x10(%rsp) */
            /* movss -0x10(%rsp),%xmm0 */
            o(S, 0x100ff3);
            o(S, 0x44 + REG_VALUE(r)*8);
            o(S, 0xf024);
            S->tccgen_vtop->r = r;
        }
    }
}

/* convert fp to int 't' type */
void gen_cvt_ftoi(TCCState* S, int t)
{
    int ft, bt, size, r;
    ft = S->tccgen_vtop->type.t;
    bt = ft & VT_BTYPE;
    if (bt == VT_LDOUBLE) {
        gen_cvt_ftof(S, VT_DOUBLE);
        bt = VT_DOUBLE;
    }

    gv(S, RC_FLOAT);
    if (t != VT_INT)
        size = 8;
    else
        size = 4;

    r = get_reg(S, RC_INT);
    if (bt == VT_FLOAT) {
        o(S, 0xf3);
    } else if (bt == VT_DOUBLE) {
        o(S, 0xf2);
    } else {
        assert(0);
    }
    orex(S, size == 8, r, 0, 0x2c0f); /* cvttss2si or cvttsd2si */
    o(S, 0xc0 + REG_VALUE(S->tccgen_vtop->r) + REG_VALUE(r)*8);
    S->tccgen_vtop->r = r;
}

// Generate sign extension from 32 to 64 bits:
ST_FUNC void gen_cvt_sxtw(TCCState* S)
{
    int r = gv(S, RC_INT);
    /* x86_64 specific: movslq */
    o(S, 0x6348);
    o(S, 0xc0 + (REG_VALUE(r) << 3) + REG_VALUE(r));
}

/* char/short to int conversion */
ST_FUNC void gen_cvt_csti(TCCState* S, int t)
{
    int r, sz, xl, ll;
    r = gv(S, RC_INT);
    sz = !(t & VT_UNSIGNED);
    xl = (t & VT_BTYPE) == VT_SHORT;
    ll = (S->tccgen_vtop->type.t & VT_BTYPE) == VT_LLONG;
    orex(S, ll, r, 0, 0xc0b60f /* mov[sz] %a[xl], %eax */
        | (sz << 3 | xl) << 8
        | (REG_VALUE(r) << 3 | REG_VALUE(r)) << 16
        );
}

/* increment tcov counter */
ST_FUNC void gen_increment_tcov (TCCState* S, SValue *sv)
{
   o(S, 0x058348); /* addq $1, xxx(%rip) */
   greloca(S, cur_text_section, sv->sym, S->tccgen_ind, R_X86_64_PC32, -5);
   gen_le32(S, 0);
   o(S, 1);
}

/* computed goto support */
void ggoto(TCCState* S)
{
    gcall_or_jmp(S, 1);
    S->tccgen_vtop--;
}

/* Save the stack pointer onto the stack and return the location of its address */
ST_FUNC void gen_vla_sp_save(TCCState* S, int addr) {
    /* mov %rsp,addr(%rbp)*/
    gen_modrm64(S, 0x89, TREG_RSP, VT_LOCAL, NULL, addr);
}

/* Restore the SP from a location on the stack */
ST_FUNC void gen_vla_sp_restore(TCCState* S, int addr) {
    gen_modrm64(S, 0x8b, TREG_RSP, VT_LOCAL, NULL, addr);
}

#ifdef TCC_TARGET_PE
/* Save result of gen_vla_alloc onto the stack */
ST_FUNC void gen_vla_result(TCCState* S, int addr) {
    /* mov %rax,addr(%rbp)*/
    gen_modrm64(S, 0x89, TREG_RAX, VT_LOCAL, NULL, addr);
}
#endif

/* Subtract from the stack pointer, and push the resulting value onto the stack */
ST_FUNC void gen_vla_alloc(TCCState* S, CType *type, int align) {
    int use_call = 0;

#if defined(CONFIG_TCC_BCHECK)
    use_call = S->do_bounds_check;
#endif
#ifdef TCC_TARGET_PE	/* alloca does more than just adjust %rsp on Windows */
    use_call = 1;
#endif
    if (use_call)
    {
        vpush_helper_func(S, TOK_alloca);
        vswap(S); /* Move alloca ref past allocation size */
        gfunc_call(S, 1);
    }
    else {
        int r;
        r = gv(S, RC_INT); /* allocation size */
        /* sub r,%rsp */
        o(S, 0x2b48);
        o(S, 0xe0 | REG_VALUE(r));
        /* We align to 16 bytes rather than align */
        /* and ~15, %rsp */
        o(S, 0xf0e48348);
        vpop(S);
    }
}


/* end of x86-64 code generator */
/*************************************************************/
#endif /* ! TARGET_DEFS_ONLY */
/******************************************************/
