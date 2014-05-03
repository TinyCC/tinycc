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
#define NB_ASM_REGS     8

/* a register can belong to several classes. The classes must be
   sorted from more general to more precise (see gv2() code which does
   assumptions on it). */
#define RC_INT    	0x0001 /* generic integer register */
#define RC_FLOAT 	0x0002 /* generic float register */
#define RC_RAX    	0x0004
#define RC_RCX   	0x0008
#define RC_RDX   	0x0010
#define RC_ST0		0x0020 /* only for long double */
#define RC_R8   	0x0040
#define RC_R9   	0x0080
#define RC_XMM0  	0x0100
#define RC_XMM1  	0x0200
#define RC_XMM2  	0x0400
#define RC_XMM3  	0x0800
#define RC_XMM4  	0x1000
#define RC_XMM5  	0x2000
#define RC_XMM6   	0x4000
#define RC_XMM7  	0x8000
#define RC_RSI		0x10000
#define RC_RDI		0x20000
#define RC_INT1  	0x40000	/* function_pointer */
#define RC_INT2		0x80000
#define RC_RBX		0x100000
#define RC_R10		0x200000
#define RC_R11		0x400000
#define RC_R12		0x800000
#define RC_R13		0x1000000
#define RC_R14		0x2000000
#define RC_R15		0x4000000
#define RC_IRET  	RC_RAX /* function return: integer register */
#define RC_LRET   	RC_RDX /* function return: second integer register */
#define RC_FRET   	RC_XMM0 /* function return: float register */
#define RC_QRET   	RC_XMM1 /* function return: second float register */
#define RC_MASK	  	(RC_INT|RC_INT1|RC_INT2|RC_FLOAT)

/* pretty names for the registers */
enum {
    TREG_RAX = 0,
    TREG_RCX = 1,
    TREG_RDX = 2,
    TREG_RSP = 4,
    TREG_ST0 = 5,
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

};

#define REX_BASE(reg) (((reg) >> 3) & 1)
#define REG_VALUE(reg) ((reg) & 7)
#define FLAG_GOT 	0X01

/* return registers for function */
#define REG_IRET TREG_RAX /* single word int return register */
#define REG_LRET TREG_RDX /* second word return register (for long long) */
#define REG_FRET TREG_XMM0 /* float return register */
#define REG_QRET TREG_XMM1 /* second float return register */

/* defined if function parameters must be evaluated in reverse order */
#define INVERT_FUNC_PARAMS

/* pointer size, in bytes */
#define PTR_SIZE 8

/* long double size and alignment, in bytes */
#define LDOUBLE_SIZE  16
#define LDOUBLE_ALIGN 16
/* maximum alignment (for aligned attribute support) */
#define MAX_ALIGN     16

/******************************************************/
/* ELF defines */

#define EM_TCC_TARGET EM_X86_64

/* relocation type for 32 bit data relocation */
#define R_DATA_32   R_X86_64_32
#define R_DATA_PTR  R_X86_64_64
#define R_JMP_SLOT  R_X86_64_JUMP_SLOT
#define R_COPY      R_X86_64_COPY

#define ELF_START_ADDR 0x400000
#define ELF_PAGE_SIZE  0x200000

/******************************************************/
#else /* ! TARGET_DEFS_ONLY */
/******************************************************/
#include "tcc.h"
#include <assert.h>

ST_DATA const int reg_classes[NB_REGS] = {
    /* eax */ RC_INT|RC_RAX|RC_INT2,
    /* ecx */ RC_INT|RC_RCX|RC_INT2,
    /* edx */ RC_INT|RC_RDX,
	RC_INT|RC_INT1|RC_INT2|RC_RBX,
    0,
    /* st0 */ RC_ST0,
    RC_RSI|RC_INT2,
    RC_RDI|RC_INT2,
    RC_INT|RC_R8|RC_INT2,
    RC_INT|RC_R9|RC_INT2,
    RC_INT|RC_INT1|RC_INT2|RC_R10,
	RC_INT|RC_INT1|RC_INT2|RC_R11,
	RC_INT|RC_INT1|RC_INT2|RC_R12,
	RC_INT|RC_INT1|RC_INT2|RC_R13,
	RC_INT|RC_INT1|RC_INT2|RC_R14,
	RC_INT|RC_INT1|RC_INT2|RC_R15,
	/* xmm0 */ RC_FLOAT | RC_XMM0,
	RC_FLOAT|RC_XMM1,
	RC_FLOAT|RC_XMM2,
	RC_FLOAT|RC_XMM3,
	RC_FLOAT|RC_XMM4,
	RC_FLOAT|RC_XMM5,
	RC_FLOAT|RC_XMM6,
	RC_FLOAT|RC_XMM7,
};

static unsigned long func_sub_sp_offset;
static int func_ret_sub;

/* XXX: make it faster ? */
void g(int c)
{
    int ind1;
    ind1 = ind + 1;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    cur_text_section->data[ind] = c;
    ind = ind1;
}

void o(unsigned int c)
{
    while (c) {
        g(c);
        c = c >> 8;
    }
}

void gen_le16(int v)
{
    g(v);
    g(v >> 8);
}

void gen_le32(int c)
{
    g(c);
    g(c >> 8);
    g(c >> 16);
    g(c >> 24);
}

void gen_le64(int64_t c)
{
    g(c);
    g(c >> 8);
    g(c >> 16);
    g(c >> 24);
    g(c >> 32);
    g(c >> 40);
    g(c >> 48);
    g(c >> 56);
}

void orex(int ll, int r, int r2, int b)
{
    if ((r & VT_VALMASK) >= VT_CONST)
        r = 0;
    if ((r2 & VT_VALMASK) >= VT_CONST)
        r2 = 0;
    if (ll || REX_BASE(r) || REX_BASE(r2))
        o(0x40 | REX_BASE(r) | (REX_BASE(r2) << 2) | (ll << 3));
    o(b);
}

/* output a symbol and patch all calls to it */
void gsym_addr(int t, int a)
{
    int n, *ptr;
    while (t) {
        ptr = (int *)(cur_text_section->data + t);
        n = *ptr; /* next value */
        *ptr = a - t - 4;
        t = n;
    }
}

void gsym(int t)
{
    gsym_addr(t, ind);
}

/* psym is used to put an instruction with a data field which is a
   reference to a symbol. It is in fact the same as oad ! */
#define psym oad

static int is64_type(int t)
{
    return ((t & VT_BTYPE) == VT_PTR ||
            (t & VT_BTYPE) == VT_FUNC ||
            (t & VT_BTYPE) == VT_LLONG);
}

/* instruction + 4 bytes data. Return the address of the data */
ST_FUNC int oad(int c, int s)
{
    int ind1;

    o(c);
    ind1 = ind + 4;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    *(int *)(cur_text_section->data + ind) = s;
    s = ind;
    ind = ind1;
    return s;
}

ST_FUNC void gen_addr32(int r, Sym *sym, int c)
{
    if (r & VT_SYM)
        greloc(cur_text_section, sym, ind, R_X86_64_32);
    gen_le32(c);
}

/* output constant with relocation if 'r & VT_SYM' is true */
ST_FUNC void gen_addr64(int r, Sym *sym, int64_t c)
{
    if (r & VT_SYM)
        greloc(cur_text_section, sym, ind, R_X86_64_64);
    gen_le64(c);
}

/* output constant with relocation if 'r & VT_SYM' is true */
ST_FUNC void gen_addrpc32(int r, Sym *sym, int c)
{
    if (r & VT_SYM)
        greloc(cur_text_section, sym, ind, R_X86_64_PC32);
    gen_le32(c-4);
}

/* output got address with relocation */
static void gen_gotpcrel(int r, Sym *sym, int c)
{
#ifndef TCC_TARGET_PE
    Section *sr;
    ElfW(Rela) *rel;
    greloc(cur_text_section, sym, ind, R_X86_64_GOTPCREL);
    sr = cur_text_section->reloc;
    rel = (ElfW(Rela) *)(sr->data + sr->data_offset - sizeof(ElfW(Rela)));
    rel->r_addend = -4;
#else
    printf("picpic: %s %x %x | %02x %02x %02x\n", get_tok_str(sym->v, NULL), c, r,
        cur_text_section->data[ind-3],
        cur_text_section->data[ind-2],
        cur_text_section->data[ind-1]
        );
    greloc(cur_text_section, sym, ind, R_X86_64_PC32);
#endif
    gen_le32(0);
    if (c) {
        /* we use add c, %xxx for displacement */
        orex(1, r, 0, 0x81);
        o(0xc0 + REG_VALUE(r));
        gen_le32(c);
    }
}

static void gen_modrm_impl(int op_reg, int fr, Sym *sym, int c, int flag)
{
	int r = fr & VT_VALMASK;
    op_reg = REG_VALUE(op_reg) << 3;
    if (r == VT_CONST) {
        /* constant memory reference */
        o(0x05 | op_reg);
        if (flag & FLAG_GOT) {
            gen_gotpcrel(fr, sym, c);
        } else {
            gen_addrpc32(fr, sym, c);
        }
    } else if (r == VT_LOCAL) {
        /* currently, we use only ebp as base */
        if (c == (char)c) {
            /* short reference */
            o(0x45 | op_reg);
            g(c);
        } else {
            oad(0x85 | op_reg, c);
        }
    } else if (c) {
		if (c == (char)c) {
			/* short reference */
			g(0x40 | op_reg | REG_VALUE(fr));
			if(r == TREG_RSP)
				g(0x24);
			g(c);
        } else {
			g(0x80 | op_reg | REG_VALUE(fr));
			if(r == TREG_RSP)
				g(0x24);
			gen_le32(c);
        }
    } else {
		g(0x00 | op_reg | REG_VALUE(fr));
		if(r == TREG_RSP)
			g(0x24);
    }
}

/* generate a modrm reference. 'op_reg' contains the addtionnal 3
   opcode bits */
static void gen_modrm(int op_reg, int r, Sym *sym, int c)
{
    gen_modrm_impl(op_reg, r, sym, c, 0);
}

/* generate a modrm reference. 'op_reg' contains the addtionnal 3
   opcode bits */
static void gen_modrm64(int opcode, int op_reg, int r, Sym *sym, int c)
{
    int flag;
    if((op_reg & TREG_MEM) && !(sym->type.t & VT_STATIC))
		flag = FLAG_GOT;
    orex(1, r, op_reg, opcode);
    gen_modrm_impl(op_reg, r, sym, c, flag);
}


/* load 'r' from value 'sv' */
void load(int r, SValue *sv)
{
    int v, t, ft, fc, fr, ll;
    SValue v1;

#ifdef TCC_TARGET_PE
    SValue v2;
    sv = pe_getimport(sv, &v2);
#endif

    fr = sv->r;
    ft = sv->type.t & ~VT_DEFSIGN;
    fc = sv->c.ul;
	ll = is64_type(ft);

#ifndef TCC_TARGET_PE
    /* we use indirect access via got */
    if ((fr & VT_VALMASK) == VT_CONST && (fr & VT_SYM) &&
        (fr & VT_LVAL) && !(sv->sym->type.t & VT_STATIC)) {
        /* use the result register as a temporal register */
        int tr;
        if (is_float(ft)) {
            /* we cannot use float registers as a temporal register */
            tr = get_reg(RC_INT) | TREG_MEM;
        }else{
			tr = r | TREG_MEM;
		}
        gen_modrm64(0x8b, tr, fr, sv->sym, 0);
        /* load from the temporal register */
        fr = tr | VT_LVAL;
    }
#endif

    v = fr & VT_VALMASK;
    if (fr & VT_LVAL) {
        if(fr & VT_TMP){
			int size, align;
			if((ft & VT_BTYPE) == VT_FUNC)
				size = PTR_SIZE;
			else
				size = type_size(&sv->type, &align);
			loc_stack(size, 0);
		}
        if (v == VT_LLOCAL) {
            v1.type.t = VT_PTR;
            v1.r = VT_LOCAL | VT_LVAL;
            v1.c.ul = fc;
            fr = r;
            if (!(reg_classes[fr] & RC_INT))
                fr = get_reg(RC_INT);
            load(fr, &v1);
			fc = 0;
        }
		int b;
        if ((ft & VT_BTYPE) == VT_FLOAT) {
            b = 0x100ff3; /* movss */
        } else if ((ft & VT_BTYPE) == VT_DOUBLE) {
            b = 0x100ff2; /* movds */
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
        } else {
			assert(((ft & VT_BTYPE) == VT_INT) || ((ft & VT_BTYPE) == VT_LLONG)
                   || ((ft & VT_BTYPE) == VT_PTR) || ((ft & VT_BTYPE) == VT_ENUM)
                   || ((ft & VT_BTYPE) == VT_FUNC));
            b = 0x8b;
        }
		orex(ll, fr, r, b);
		gen_modrm(r, fr, sv->sym, fc);
    } else {
        if (v == VT_CONST) {
            if (fr & VT_SYM) {
#ifdef TCC_TARGET_PE
                orex(1,0,r,0x8d);
                o(0x05 + REG_VALUE(r) * 8); /* lea xx(%rip), r */
                gen_addrpc32(fr, sv->sym, fc);
#else
                if (sv->sym->type.t & VT_STATIC) {
                    orex(1,0,r,0x8d);
                    o(0x05 + REG_VALUE(r) * 8); /* lea xx(%rip), r */
                    gen_addrpc32(fr, sv->sym, fc);
                } else {
                    orex(1,0,r,0x8b);
                    o(0x05 + REG_VALUE(r) * 8); /* mov xx(%rip), r */
                    gen_gotpcrel(r, sv->sym, fc);
                }
#endif
            } else {
				orex(ll,r,0, 0xb8 + REG_VALUE(r)); /* mov $xx, r */
				if (ll)
					gen_le64(sv->c.ull);
				else
					gen_le32(fc);
			}
        } else if (v == VT_LOCAL) {
            orex(1,0,r,0x8d); /* lea xxx(%ebp), r */
            gen_modrm(r, VT_LOCAL, sv->sym, fc);
        } else if (v == VT_CMP) {
			orex(0, r, 0, 0xb8 + REG_VALUE(r));
			if ((fc & ~0x100) == TOK_NE){
				gen_le32(1);/* mov $0, r */
			}else{
				gen_le32(0);/* mov $1, r */
			}
			if (fc & 0x100){
				fc &= ~0x100;
				/* This was a float compare.  If the parity bit is
				set the result was unordered, meaning false for everything
				except TOK_NE, and true for TOK_NE.  */
				o(0x037a + (REX_BASE(r) << 8));/* jp 3*/
			}
			orex(0,r,0, 0x0f); /* setxx %br */
			o(fc);
			o(0xc0 + REG_VALUE(r));
        } else if (v == VT_JMP || v == VT_JMPI) {
            t = v & 1;
            orex(0,r,0,0);
            oad(0xb8 + REG_VALUE(r), t); /* mov $1, r */
            o(0x05eb + (REX_BASE(r) << 8)); /* jmp after */
            gsym(fc);
            orex(0,r,0,0);
            oad(0xb8 + REG_VALUE(r), t ^ 1); /* mov $0, r */
        } else if (v != r) {
            if (reg_classes[r] & RC_FLOAT) {
                if(v == TREG_ST0){
					/* gen_cvt_ftof(VT_DOUBLE); */
					o(0xf0245cdd); /* fstpl -0x10(%rsp) */
					/* movsd -0x10(%rsp),%xmm0 */
					o(0x100ff2);
					o(0xf02444 + REG_VALUE(r)*8);
				}else if(reg_classes[v] & RC_FLOAT){
					o(0x7e0ff3);
					o(0xc0 + REG_VALUE(v) + REG_VALUE(r)*8);
				}else
					assert(0);
            } else if (r == TREG_ST0) {
                assert(reg_classes[v] & RC_FLOAT);
                /* gen_cvt_ftof(VT_LDOUBLE); */
                /* movsd %xmm0,-0x10(%rsp) */
				o(0x110ff2);
				o(0xf02444 + REG_VALUE(v)*8);
                o(0xf02444dd); /* fldl -0x10(%rsp) */
            } else {
				if(fc){
					orex(1,fr,r,0x8d); /* lea xxx(%ebp), r */
					gen_modrm(r, fr, sv->sym, fc);
				}else{
					orex(ll,v,r, 0x8b);
					o(0xc0 + REG_VALUE(v) + REG_VALUE(r) * 8); /* mov v, r */
				}
            }
        }
    }
}

/* store register 'r' in lvalue 'v' */
void store(int r, SValue *sv)
{
    int fr, bt, ft, fc, ll, v;

#ifdef TCC_TARGET_PE
    SValue v2;
    sv = pe_getimport(sv, &v2);
#endif
    ft = sv->type.t & ~VT_DEFSIGN;
    fc = sv->c.ul;
    fr = sv->r;
    bt = ft & VT_BTYPE;
	ll = is64_type(ft);
	v = fr & VT_VALMASK;

//#ifndef TCC_TARGET_PE
    /* we need to access the variable via got */
  //  if (fr == VT_CONST && (v->r & VT_SYM)) {
        /* mov xx(%rip), %r11 */
    //    o(0x1d8b4c);
      //  gen_gotpcrel(TREG_R11, v->sym, v->c.ul);
        //pic = is64_type(bt) ? 0x49 : 0x41;
   // }
//#endif

    /* XXX: incorrect if float reg to reg */
    if (bt == VT_FLOAT) {
		orex(0, fr, r, 0x110ff3); /* movss */
    } else if (bt == VT_DOUBLE) {
		orex(0, fr, r, 0x110ff2);/* movds */
    } else if (bt == VT_LDOUBLE) {
        o(0xc0d9); /* fld %st(0) */
		orex(0, fr, r, 0xdb);/* fstpt */
        r = 7;
    } else {
        if (bt == VT_SHORT)
            o(0x66);
		if (bt == VT_BYTE || bt == VT_BOOL)
			orex(ll, fr, r, 0x88);
		else{
			orex(ll, fr, r, 0x89);
		}
    }
	if (v == VT_CONST || v == VT_LOCAL || (fr & VT_LVAL)) {
		gen_modrm(r, fr, sv->sym, fc);
	} else if (v != r) {
		/* XXX: don't we really come here? */
		abort();
		o(0xc0 + REG_VALUE(v) + REG_VALUE(r)*8); /* mov r, fr */
	}
}

/* 'is_jmp' is '1' if it is a jump */
static void gcall_or_jmp(int is_jmp)
{
    int r;
    if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
        /* constant case */
        if (vtop->r & VT_SYM) {
            /* relocation case */
            greloc(cur_text_section, vtop->sym,
                   ind + 1, R_X86_64_PLT32);
        } else {
            /* put an empty PC32 relocation */
            put_elf_reloc(symtab_section, cur_text_section,
                          ind + 1, R_X86_64_PC32, 0);
        }
        oad(0xe8 + is_jmp, vtop->c.ul - 4); /* call/jmp im */
    } else {
        /* otherwise, indirect call */
        r = get_reg(RC_INT1);
        load(r, vtop);
		orex(0, r, 0, 0xff); /* REX call/jmp *r */
        o(0xd0 + REG_VALUE(r) + (is_jmp << 4));
    }
}

static int func_scratch;
static int r_loc;

int reloc_add(int inds)
{
	return psym(0, inds);
}

void reloc_use(int t, int data)
{
	int *ptr;
    while (t) {
        ptr = (int *)(cur_text_section->data + t);
        t = *ptr; /* next value */
        *ptr = data;
    }
}

void struct_copy(SValue *d, SValue *s, SValue *c)
{
	if(!c->c.i)
		return;
	save_reg(TREG_RCX);
	load(TREG_RCX, c);
	load(TREG_RDI, d);
	load(TREG_RSI, s);
	o(0xa4f3);// rep movsb
}

void gen_putz(SValue *d, int size)
{
	if(!size)
		return;
	save_reg(TREG_RAX);
	o(0xb0);
	g(0x00);
	save_reg(TREG_RCX);
	o(0xb8 + REG_VALUE(TREG_RCX)); /* mov $xx, r */
	gen_le32(size);
	load(TREG_RDI, d);
	o(0xaaf3);//rep stos
}

/* Generate function call. The function address is pushed first, then
   all the parameters in call order. This functions pops all the
   parameters and the function address. */
void gen_offs_sp(int b, int r, int off)
{
	if(r & 0x100)
		o(b);
	else
		orex(1, 0, r, b);
	if(!off){
		o(0x2404 | (REG_VALUE(r) << 3));
	}else if (off == (char)off) {
        o(0x2444 | (REG_VALUE(r) << 3));
        g(off);
    } else {
        o(0x2484 | (REG_VALUE(r) << 3));
        gen_le32(off);
    }
}

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
      return arg_regs[idx];
}

/* Return the number of registers needed to return the struct, or 0 if
   returning via struct pointer. */
ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret, int *ret_align)
{
    int size, align;
    *ret_align = 1; // Never have to re-align return values for x86-64
    size = type_size(vt, &align);
    ret->ref = NULL;
    if (size > 8) {
        return 0;
    } else if (size > 4) {
        ret->t = VT_LLONG;
        return 1;
    } else if (size > 2) {
        ret->t = VT_INT;
        return 1;
    } else if (size > 1) {
        ret->t = VT_SHORT;
        return 1;
    } else {
        ret->t = VT_BYTE;
        return 1;
    }
}

static int is_sse_float(int t) {
    int bt;
    bt = t & VT_BTYPE;
    return bt == VT_DOUBLE || bt == VT_FLOAT;
}

int gfunc_arg_size(CType *type) {
    int align;
    if (type->t & (VT_ARRAY|VT_BITFIELD))
        return 8;
    return type_size(type, &align);
}

void gfunc_call(int nb_args)
{
    int size, r, args_size, i, d, bt, struct_size;
    int arg;

    args_size = (nb_args < REGN ? REGN : nb_args) * PTR_SIZE;
    arg = nb_args;

    /* for struct arguments, we need to call memcpy and the function
       call breaks register passing arguments we are preparing.
       So, we process arguments which will be passed by stack first. */
    struct_size = args_size;
    for(i = 0; i < nb_args; i++) {
        SValue *sv;
        
        --arg;
        sv = &vtop[-i];
        bt = (sv->type.t & VT_BTYPE);
        size = gfunc_arg_size(&sv->type);

        if (size <= 8)
            continue; /* arguments smaller than 8 bytes passed in registers or on stack */

        if (bt == VT_STRUCT) {
            /* align to stack align size */
            size = (size + 15) & ~15;
            /* generate structure store */
            r = get_reg(RC_INT);
            gen_offs_sp(0x8d, r, struct_size);
            struct_size += size;

            /* generate memcpy call */
            vset(&sv->type, r | VT_LVAL, 0);
            vpushv(sv);
            vstore();
            --vtop;
        } else if (bt == VT_LDOUBLE) {
            gv(RC_ST0);
            gen_offs_sp(0xdb, 0x107, struct_size);
            struct_size += 16;
        }
    }

    if (func_scratch < struct_size)
        func_scratch = struct_size;

    arg = nb_args;
    struct_size = args_size;

    for(i = 0; i < nb_args; i++) {
        --arg;
        bt = (vtop->type.t & VT_BTYPE);

        size = gfunc_arg_size(&vtop->type);
        if (size > 8) {
            /* align to stack align size */
            size = (size + 15) & ~15;
            if (arg >= REGN) {
                d = get_reg(RC_INT);
                gen_offs_sp(0x8d, d, struct_size);
                gen_offs_sp(0x89, d, arg*8);
            } else {
                d = arg_prepare_reg(arg);
                gen_offs_sp(0x8d, d, struct_size);
            }
            struct_size += size;
        } else {
            if (is_sse_float(vtop->type.t)) {
                gv(RC_XMM0); /* only use one float register */
                if (arg >= REGN) {
                    /* movq %xmm0, j*8(%rsp) */
                    gen_offs_sp(0xd60f66, 0x100, arg*8);
                } else {
                    /* movaps %xmm0, %xmmN */
                    o(0x280f);
                    o(0xc0 + (arg << 3));
                    d = arg_prepare_reg(arg);
                    /* mov %xmm0, %rxx */
                    o(0x66);
                    orex(1,d,0, 0x7e0f);
                    o(0xc0 + REG_VALUE(d));
                }
            } else {
                if (bt == VT_STRUCT) {
                    vtop->type.ref = NULL;
                    vtop->type.t = size > 4 ? VT_LLONG : size > 2 ? VT_INT
                        : size > 1 ? VT_SHORT : VT_BYTE;
                }
                
                r = gv(RC_INT);
                if (arg >= REGN) {
                    gen_offs_sp(0x89, r, arg*8);
                } else {
                    d = arg_prepare_reg(arg);
                    orex(1,d,r,0x89); /* mov */
                    o(0xc0 + REG_VALUE(r) * 8 + REG_VALUE(d));
                }
            }
        }
        vtop--;
    }
    save_regs(0);
    
    /* Copy R10 and R11 into RCX and RDX, respectively */
    if (nb_args > 0) {
        o(0xd1894c); /* mov %r10, %rcx */
        if (nb_args > 1) {
            o(0xda894c); /* mov %r11, %rdx */
        }
    }
    
    gcall_or_jmp(0);
    vtop--;
}


#define FUNC_PROLOG_SIZE 11

/* generate function prolog of type 't' */
void gfunc_prolog(CType *func_type)
{
    int addr, reg_param_index, bt, size;
    Sym *sym;
    CType *type;

    func_ret_sub = func_scratch = r_loc = 0;
    pop_stack = loc = 0;

    addr = PTR_SIZE * 2;
    ind += FUNC_PROLOG_SIZE;
    func_sub_sp_offset = ind;
    reg_param_index = 0;

    sym = func_type->ref;

    /* if the function returns a structure, then add an
       implicit pointer parameter */
    func_vt = sym->type;
    func_var = (sym->c == FUNC_ELLIPSIS);
    size = gfunc_arg_size(&func_vt);
    if (size > 8) {
        gen_modrm64(0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
        func_vc = addr;
        reg_param_index++;
        addr += 8;
    }

    /* define parameters */
    while ((sym = sym->next) != NULL) {
        type = &sym->type;
        bt = type->t & VT_BTYPE;
        size = gfunc_arg_size(type);
        if (size > 8) {
            if (reg_param_index < REGN) {
                gen_modrm64(0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
            }
            sym_push(sym->v & ~SYM_FIELD, type, VT_LOCAL | VT_LVAL | VT_REF, addr);
        } else {
            if (reg_param_index < REGN) {
                /* save arguments passed by register */
                if ((bt == VT_FLOAT) || (bt == VT_DOUBLE)) {
                    o(0xd60f66); /* movq */
                    gen_modrm(reg_param_index, VT_LOCAL, NULL, addr);
                } else {
                    gen_modrm64(0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
                }
            }
            sym_push(sym->v & ~SYM_FIELD, type, VT_LOCAL | VT_LVAL, addr);
        }
        addr += 8;
        reg_param_index++;
    }

    while (reg_param_index < REGN) {
        if (func_type->ref->c == FUNC_ELLIPSIS) {
            gen_modrm64(0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
            addr += 8;
        }
        reg_param_index++;
    }
}

/* generate function epilog */
void gfunc_epilog(void)
{
    int v, saved_ind;

    o(0xc9); /* leave */
    if (func_ret_sub == 0) {
        o(0xc3); /* ret */
    } else {
        o(0xc2); /* ret n */
        g(func_ret_sub);
        g(func_ret_sub >> 8);
    }

    saved_ind = ind;
    ind = func_sub_sp_offset - FUNC_PROLOG_SIZE;
    /* align local size to word & save local variables */
    v = (func_scratch + -loc + 15) & -16;
    reloc_use(r_loc, func_scratch);
    if (v >= 4096) {
        Sym *sym = external_global_sym(TOK___chkstk, &func_old_type, 0);
        oad(0xb8, v); /* mov stacksize, %eax */
        oad(0xe8, -4); /* call __chkstk, (does the stackframe too) */
        greloc(cur_text_section, sym, ind-4, R_X86_64_PC32);
        o(0x90); /* fill for FUNC_PROLOG_SIZE = 11 bytes */
    } else {
        o(0xe5894855);  /* push %rbp, mov %rsp, %rbp */
        o(0xec8148);  /* sub rsp, stacksize */
        gen_le32(v);
    }

    cur_text_section->data_offset = saved_ind;
    pe_add_unwind_data(ind, saved_ind, v);
    ind = cur_text_section->data_offset;
}

#else

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
	case VT_QLONG:
    case VT_BOOL:
    case VT_PTR:
    case VT_FUNC:
    case VT_ENUM: return x86_64_mode_integer;
    
    case VT_FLOAT:
	case VT_QFLOAT:
    case VT_DOUBLE: return x86_64_mode_sse;
    
    case VT_LDOUBLE: return x86_64_mode_x87;
      
    case VT_STRUCT:
        f = ty->ref;

        // Detect union
        if (f->next && (f->c == f->next->c))
          return x86_64_mode_memory;

        mode = x86_64_mode_none;
        for (f = f->next; f; f = f->next)
            mode = classify_x86_64_merge(mode, classify_x86_64_inner(&f->type));
        
        return mode;
    }
    
    assert(0);
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
			ret_t = ty->t;
        } else {
            mode = classify_x86_64_inner(ty);
            switch (mode) {
            case x86_64_mode_integer:
                if (size > 8) {
                    *reg_count = 2;
                    ret_t = VT_QLONG;
                } else {
					*reg_count = 1;
					if(size > 4)
						ret_t = VT_LLONG;
					else if(size > 2){
						ret_t = VT_INT;
					}else if(size > 1)
						ret_t = VT_SHORT;
					else
						ret_t = VT_BYTE;
				}
				ret_t |= (ty->t & VT_UNSIGNED);
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
			default:
				ret_t = ty->t;
				break; /* nothing to be done for x86_64_mode_memory and x86_64_mode_none*/
            }
        }
    }
    
    if (ret) {
        ret->ref = ty->ref;
        ret->t = ret_t;
    }
    
    return mode;
}

ST_FUNC int classify_x86_64_va_arg(CType *ty)
{
    /* This definition must be synced with stdarg.h */
    enum __va_arg_type {
		__va_gen_reg, __va_float_reg, __va_ld_reg, __va_stack
	};
    int size, align, reg_count;
    X86_64_Mode mode = classify_x86_64_arg(ty, NULL, &size, &align, &reg_count);
    switch (mode) {
    default: return __va_stack;
	case x86_64_mode_x87: return __va_ld_reg;
    case x86_64_mode_integer: return __va_gen_reg;
    case x86_64_mode_sse: return __va_float_reg;
    }
}

/* Return the number of registers needed to return the struct, or 0 if
   returning via struct pointer. */
ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret, int *ret_align)
{
    int size, align, reg_count;
    *ret_align = 1; // Never have to re-align return values for x86-64
    return (classify_x86_64_arg(vt, ret, &size, &align, &reg_count) != x86_64_mode_memory);
}

#define REGN 6
static const uint8_t arg_regs[REGN] = {
    TREG_RDI, TREG_RSI, TREG_RDX, TREG_RCX, TREG_R8, TREG_R9
};

/* Generate function call. The function address is pushed first, then
   all the parameters in call order. This functions pops all the
   parameters and the function address. */
void gfunc_call(int nb_args)
{
	X86_64_Mode mode;
	int size, align, args_size, s, e, i, reg_count;
    int nb_reg_args = 0;
    int nb_sse_args = 0;
	int gen_reg, sse_reg;
	CType type;

	/* fetch cpu flag before the following sub will change the value */
	if (vtop >= vstack && (vtop->r & VT_VALMASK) == VT_CMP)
		gv(RC_INT);
    /* calculate the number of integer/float register arguments */
    for(i = 0; i < nb_args; i++) {
        mode = classify_x86_64_arg(&vtop[-i].type, NULL, &size, &align, &reg_count);
        if (mode == x86_64_mode_sse)
            nb_sse_args += reg_count;
        else if (mode == x86_64_mode_integer)
            nb_reg_args += reg_count;
    }

    args_size = 0;
	gen_reg = nb_reg_args;
	sse_reg = nb_sse_args;
	/* for struct arguments, we need to call memcpy and the function
	call breaks register passing arguments we are preparing.
	So, we process arguments which will be passed by stack first. */
    for(i = 0; i < nb_args; i++) {
		mode = classify_x86_64_arg(&vtop[-i].type, NULL, &size, &align, &reg_count);
		switch (mode) {
		case x86_64_mode_x87:
			if((vtop[-i].type.t & VT_BTYPE) == VT_STRUCT)
				goto stack_arg1;
			else
				args_size = (args_size + 15) & ~15;
		case x86_64_mode_memory:
            stack_arg1:
			args_size += size;
			break;
		case x86_64_mode_sse:
			sse_reg -= reg_count;
			if (sse_reg + reg_count > 8)
				goto stack_arg1;
			break;
		case x86_64_mode_integer:
			gen_reg -= reg_count;
			if (gen_reg + reg_count > REGN)
				goto stack_arg1;
			break;
		default: break; /* nothing to be done for x86_64_mode_none */
		}
    }

	args_size = (args_size + 15) & ~15;
	if (func_scratch < args_size)
        func_scratch = args_size;

	gen_reg = nb_reg_args;
    sse_reg = nb_sse_args;
	for(s = e = 0; s < nb_args; s = e){
		int run_gen, run_sse, st_size;
		run_gen = gen_reg;
		run_sse = sse_reg;
		st_size = 0;
		for(i = s; i < nb_args; i++) {
			mode = classify_x86_64_arg(&vtop[-i].type, NULL, &size, &align, &reg_count);
			switch (mode) {
			case x86_64_mode_x87:
				if((vtop[-i].type.t & VT_BTYPE) == VT_STRUCT){
					goto stack_arg2;
				}else{
					++i;
					goto doing;
				}
			case x86_64_mode_memory:
				stack_arg2:
				st_size += size;
				break;
			case x86_64_mode_sse:
				sse_reg -= reg_count;
				if (sse_reg + reg_count > 8)
					goto stack_arg2;
				break;
			case x86_64_mode_integer:
				gen_reg -= reg_count;
				if (gen_reg + reg_count > REGN)
					goto stack_arg2;
				break;
			default: break; /* nothing to be done for x86_64_mode_none */
			}
		}
doing:
		e = i;
		st_size = -st_size & 15;// 16 - (size & 15)
		if(st_size)
			args_size -= st_size;

		gen_reg = run_gen;
		sse_reg = run_sse;
		for(i = s; i < e; i++) {
			SValue tmp;
			/* Swap argument to top, it will possibly be changed here,
			and might use more temps.  All arguments must remain on the
			stack, so that get_reg can correctly evict some of them onto
			stack.  We could use also use a vrott(nb_args) at the end
			of this loop, but this seems faster.  */
			if(i != 0){
				tmp = vtop[0];
				vtop[0] = vtop[-i];
				vtop[-i] = tmp;
			}

			mode = classify_x86_64_arg(&vtop->type, &type, &size, &align, &reg_count);
			switch (mode) {
			case x86_64_mode_x87:
				/* Must ensure TREG_ST0 only */
				if((vtop->type.t & VT_BTYPE) == VT_STRUCT){
					vdup();
                    vtop[-1].r = VT_CONST;
					vtop->type = type;
					gv(RC_ST0);
					args_size -= size;
					gen_offs_sp(0xdb, 0x107, args_size);
					vtop--;//Release TREG_ST0
				}else{
					gv(RC_ST0);
					args_size -= size;
					gen_offs_sp(0xdb, 0x107, args_size);
					vtop->r = VT_CONST;//Release TREG_ST0
				}
                break;
			case x86_64_mode_memory:
				args_size -= size;
				vset(&char_pointer_type, TREG_RSP, args_size);/* generate memcpy RSP */
				vpushv(&vtop[-1]);
				vtop->type = char_pointer_type;
				gaddrof();
				vpushs(size);
				struct_copy(&vtop[-2], &vtop[-1], &vtop[0]);
				vtop -= 3;
				break;
			case x86_64_mode_sse:
				sse_reg -= reg_count;
				if (sse_reg + reg_count > 8){
					args_size -= size;
					goto gen_code;
				}
				break;
			case x86_64_mode_integer:
				gen_reg -= reg_count;
				if (gen_reg + reg_count > REGN){
					args_size -= size;
					gen_code:
					vset(&type, TREG_RSP | VT_LVAL, args_size);
					vpushv(&vtop[-1]);
					vtop->type = type;
					vstore();
					vtop--;
				}
				break;
			default: break; /* nothing to be done for x86_64_mode_none */
			}
			if(i != 0){
				tmp = vtop[0];
				vtop[0] = vtop[-i];
				vtop[-i] = tmp;
			}
		}
		run_gen = gen_reg;
		run_sse = sse_reg;
	}

	gen_reg = nb_reg_args;
    sse_reg = nb_sse_args;
    for(i = 0; i < nb_args; i++) {
		int d;
		mode = classify_x86_64_arg(&vtop->type, &type, &size, &align, &reg_count);
        /* Alter stack entry type so that gv() knows how to treat it */
        vtop->type = type;
        /* Alter stack entry type so that gv() knows how to treat it */
        if (mode == x86_64_mode_sse) {
			sse_reg -= reg_count;
			if (sse_reg + reg_count <= 8) {
				if (reg_count == 2) {
					ex_rc = RC_XMM0 << (sse_reg + 1);
					gv(RC_XMM0 << sse_reg);
				}else{
					assert(reg_count == 1);
					/* Load directly to register */
					gv(RC_XMM0 << sse_reg);
				}
			}
        } else if (mode == x86_64_mode_integer) {
			gen_reg -= reg_count;
			if (gen_reg + reg_count <= REGN) {
				if (reg_count == 2) {
					d = arg_regs[gen_reg+1];
					ex_rc = reg_classes[d] & ~RC_MASK;
					d = arg_regs[gen_reg];
					gv(reg_classes[d] & ~RC_MASK);
				}else{
					assert(reg_count == 1);
					d = arg_regs[gen_reg];
					gv(reg_classes[d] & ~RC_MASK);
				}
			}
        }
		vpop();
    }
	save_regs(0);
    oad(0xb8, nb_sse_args < 8 ? nb_sse_args : 8); /* mov nb_sse_args, %eax */
    gcall_or_jmp(0);
    vtop--;
}


#define FUNC_PROLOG_SIZE 11

static void push_arg_reg(int i) {
    loc -= 8;
    gen_modrm64(0x89, arg_regs[i], VT_LOCAL, NULL, loc);
}

/* generate function prolog of type 't' */
void gfunc_prolog(CType *func_type)
{
    X86_64_Mode mode;
    int i, addr, align, size, reg_count;
    int param_addr = 0, reg_param_index, sse_param_index;
    Sym *sym;
    CType *type;

    sym = func_type->ref;
    addr = PTR_SIZE * 2;
    pop_stack = loc = 0;
	func_scratch = r_loc = 0;
    ind += FUNC_PROLOG_SIZE;
    func_sub_sp_offset = ind;
    func_ret_sub = 0;

    if (func_type->ref->c == FUNC_ELLIPSIS) {
        int seen_reg_num, seen_sse_num, seen_stack_size;
        seen_reg_num = seen_sse_num = 0;
        /* frame pointer and return address */
        seen_stack_size = PTR_SIZE * 2;
        /* count the number of seen parameters */
        while ((sym = sym->next) != NULL) {
            type = &sym->type;
            mode = classify_x86_64_arg(type, NULL, &size, &align, &reg_count);
            switch (mode) {
            default:
            stack_arg:
                seen_stack_size = ((seen_stack_size + align - 1) & -align) + size;
                break;
                
            case x86_64_mode_integer:
                if (seen_reg_num + reg_count <= REGN) {
                    seen_reg_num += reg_count;
                } else {
                    seen_reg_num = 8;
                    goto stack_arg;
                }
                break;
                
            case x86_64_mode_sse:
                if (seen_sse_num + reg_count <= 8) {
                    seen_sse_num += reg_count;
                } else {
                    seen_sse_num = 8;
                    goto stack_arg;
                }
                break;
            }
        }

        loc -= 16;
        /* movl $0x????????, -0x10(%rbp) */
        o(0xf045c7);
        gen_le32(seen_reg_num * 8);
        /* movl $0x????????, -0xc(%rbp) */
        o(0xf445c7);
        gen_le32(seen_sse_num * 16 + 48);
        /* movl $0x????????, -0x8(%rbp) */
        o(0xf845c7);
        gen_le32(seen_stack_size);

		o(0xc084);/* test   %al,%al */
		o(0x74);/* je */
		g(4*(8 - seen_sse_num) + 3);

        /* save all register passing arguments */
        for (i = 0; i < 8; i++) {
            loc -= 16;
            o(0x290f);/* movaps %xmm1-7,-XXX(%rbp) */
            gen_modrm(7 - i, VT_LOCAL, NULL, loc);
        }
        for (i = 0; i < (REGN - seen_reg_num); i++) {
            push_arg_reg(REGN-1 - i);
		}
    }

    sym = func_type->ref;
    reg_param_index = 0;
    sse_param_index = 0;

    /* if the function returns a structure, then add an
       implicit pointer parameter */
    func_vt = sym->type;
    mode = classify_x86_64_arg(&func_vt, NULL, &size, &align, &reg_count);
    if (mode == x86_64_mode_memory) {
        push_arg_reg(reg_param_index);
        func_vc = loc;
        reg_param_index++;
    }
    /* define parameters */
    while ((sym = sym->next) != NULL) {
        type = &sym->type;
        mode = classify_x86_64_arg(type, NULL, &size, &align, &reg_count);
        switch (mode) {
        case x86_64_mode_sse:
            if (sse_param_index + reg_count <= 8) {
                /* save arguments passed by register */
                loc -= reg_count * 8;
                param_addr = loc;
                for (i = 0; i < reg_count; ++i) {
                    o(0xd60f66); /* movq */
                    gen_modrm(sse_param_index, VT_LOCAL, NULL, param_addr + i*8);
                    ++sse_param_index;
                }
            } else {
                addr = (addr + align - 1) & -align;
                param_addr = addr;
                addr += size;
                sse_param_index += reg_count;
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
                loc -= reg_count * 8;
                param_addr = loc;
                for (i = 0; i < reg_count; ++i) {
                    gen_modrm64(0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, param_addr + i*8);
                    ++reg_param_index;
                }
            } else {
                addr = (addr + align - 1) & -align;
                param_addr = addr;
                addr += size;
                reg_param_index += reg_count;
            }
            break;
        }
	default: break; /* nothing to be done for x86_64_mode_none */
        }
        sym_push(sym->v & ~SYM_FIELD, type,
                 VT_LOCAL | VT_LVAL, param_addr);
    }
}

/* generate function epilog */
void gfunc_epilog(void)
{
    int v, saved_ind;

    o(0xc9); /* leave */
    if (func_ret_sub == 0) {
        o(0xc3); /* ret */
    } else {
        o(0xc2); /* ret n */
        g(func_ret_sub);
        g(func_ret_sub >> 8);
    }
    /* align local size to word & save local variables */
    v = (func_scratch -loc + 15) & -16;
    reloc_use(r_loc, func_scratch);
    saved_ind = ind;
    ind = func_sub_sp_offset - FUNC_PROLOG_SIZE;
    o(0xe5894855);  /* push %rbp, mov %rsp, %rbp */
    o(0xec8148);  /* sub rsp, stacksize */
    gen_le32(v);
    ind = saved_ind;
}

#endif /* not PE */

/* generate a jump to a label */
int gjmp(int t)
{
    return psym(0xe9, t);
}

/* generate a jump to a fixed address */
void gjmp_addr(int a)
{
    int r;
    r = a - ind - 2;
    if (r == (char)r) {
        g(0xeb);
        g(r);
    } else {
        oad(0xe9, a - ind - 5);
    }
}

/* generate a test. set 'inv' to invert test. Stack entry is popped */
int gtst(int inv, int t)
{
    int v, *p;

    v = vtop->r & VT_VALMASK;
    if (v == VT_CMP) {
        /* fast case : can jump directly since flags are set */
	if (vtop->c.i & 0x100)
	  {
	    /* This was a float compare.  If the parity flag is set
	       the result was unordered.  For anything except != this
	       means false and we don't jump (anding both conditions).
	       For != this means true (oring both).
	       Take care about inverting the test.  We need to jump
	       to our target if the result was unordered and test wasn't NE,
	       otherwise if unordered we don't want to jump.  */
	    vtop->c.i &= ~0x100;
	    if (!inv == (vtop->c.i != TOK_NE))
	      o(0x067a);  /* jp +6 */
	    else
	      {
	        g(0x0f);
		t = psym(0x8a, t); /* jp t */
	      }
	  }
        g(0x0f);
        t = psym((vtop->c.i - 16) ^ inv, t);
    } else if (v == VT_JMP || v == VT_JMPI) {
        /* && or || optimization */
        if ((v & 1) == inv) {
            /* insert vtop->c jump list in t */
            p = &vtop->c.i;
            while (*p != 0)
                p = (int *)(cur_text_section->data + *p);
            *p = t;
            t = vtop->c.i;
        } else {
            t = gjmp(t);
            gsym(vtop->c.i);
        }
	} else {
        if (is_float(vtop->type.t) ||
            (vtop->type.t & VT_BTYPE) == VT_LLONG) {
            vpushi(0);
            gen_op(TOK_NE);
        }
        if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST) {
            /* constant jmp optimization */
            if ((vtop->c.i != 0) != inv)
                t = gjmp(t);
        } else {
            v = gv(RC_INT);
            orex(0,v,v,0x85);
            o(0xc0 + REG_VALUE(v) * 9);
            g(0x0f);
            t = psym(0x85 ^ inv, t);
        }
    }
    vtop--;
    return t;
}

/* generate an integer binary operation */
void gen_opi(int op)
{
    int r, fr, opc, fc, c, ll, uu, cc, tt2;

	fr = vtop[0].r;
	fc = vtop->c.ul;
    ll = is64_type(vtop[-1].type.t);
    cc = (fr & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
	tt2 = (fr & (VT_LVAL | VT_LVAL_TYPE)) == VT_LVAL;

    switch(op) {
    case '+':
    case TOK_ADDC1: /* add with carry generation */
        opc = 0;
    gen_op8:
		vswap();
		r = gv(RC_INT);
		vswap();
        if (cc && (!ll || (int)vtop->c.ll == vtop->c.ll)) {
            /* constant case */
            c = vtop->c.i;
            if (c == (char)c) {
                /* XXX: generate inc and dec for smaller code ? */
				orex(ll, r, 0, 0x83);
				o(0xc0 + REG_VALUE(r) + opc*8);
				g(c);
            } else {
                orex(ll, r, 0, 0x81);
                oad(0xc0 + REG_VALUE(r) + opc*8, c);
            }
        } else {
			if(!tt2)
				fr = gv(RC_INT);
			orex(ll, fr, r, 0x03 + opc*8);
			if(fr >= VT_CONST)
                gen_modrm(r, fr, vtop->sym, fc);
			else
				o(0xc0 + REG_VALUE(fr) + REG_VALUE(r)*8);
        }
        vtop--;
        if (op >= TOK_ULT && op <= TOK_GT) {
            vtop->r = VT_CMP;
            vtop->c.i = op;
        }
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
		opc = 5;
        vswap();
		r = gv(RC_INT);
		vswap();
		if(!tt2)
			fr = gv(RC_INT);
		if(r == TREG_RAX){
			if(fr != TREG_RDX)
				save_reg(TREG_RDX);
			orex(ll, fr, r, 0xf7);
			if(fr >= VT_CONST)
				gen_modrm(opc, fr, vtop->sym, fc);
			else
				o(0xc0 + REG_VALUE(fr)  + opc*8);
		}else{
			orex(ll, fr, r, 0xaf0f);	/* imul fr, r */
			if(fr >= VT_CONST)
				gen_modrm(r, fr, vtop->sym, fc);
			else
				o(0xc0 + REG_VALUE(fr) + REG_VALUE(r)*8);
		}
        vtop--;
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
        if (cc) {
            /* constant case */
            vswap();
            r = gv(RC_INT);
            vswap();
			c = vtop->c.i;
			if(c == 1){
				orex(ll, r, 0, 0xd1);
				o(0xc0 + REG_VALUE(r) + opc*8);
			}else{
				orex(ll, r, 0, 0xc1); /* shl/shr/sar $xxx, r */
				o(0xc0 + REG_VALUE(r) + opc*8);
				g(c & (ll ? 0x3f : 0x1f));
			}
        } else {
            /* we generate the shift in ecx */
            gv2(RC_INT, RC_RCX);
            r = vtop[-1].r;
            orex(ll, r, 0, 0xd3); /* shl/shr/sar %cl, r */
			o(0xc0 + REG_VALUE(r) + opc*8);
        }
        vtop--;
        break;
    case TOK_UDIV:
    case TOK_UMOD:
		opc = 6;
        uu = 1;
        goto divmod;
    case '/':
    case '%':
    case TOK_PDIV:
		opc = 7;
        uu = 0;
    divmod:
        /* first operand must be in eax */
        /* XXX: need better constraint for second operand */
		if(!tt2){
			gv2(RC_RAX, RC_INT2);
			fr = vtop[0].r;
		}else{
			vswap();
			gv(RC_RAX);
			vswap();
		}
		save_reg(TREG_RDX);
		orex(ll, 0, 0, uu ? 0xd231 : 0x99); /* xor %edx,%edx : cdq RDX:RAX <- sign-extend of RAX. */
		orex(ll, fr, 0, 0xf7); /* div fr, %eax */
		if(fr >= VT_CONST)
			gen_modrm(opc, fr, vtop->sym, fc);
		else
			o(0xc0 + REG_VALUE(fr) + opc*8);
        if (op == '%' || op == TOK_UMOD)
            r = TREG_RDX;
        else
            r = TREG_RAX;
        vtop--;
        vtop->r = r;
        break;
    default:
        opc = 7;
        goto gen_op8;
    }
}

void gen_opl(int op)
{
    gen_opi(op);
}

/* generate a floating point operation 'v = t1 op t2' instruction. The
   two operands are guaranted to have the same floating point type */
/* XXX: need to use ST1 too */
void gen_opf(int op)
{
    int a, ft, fc, swapped, fr, r;
    int float_type = (vtop->type.t & VT_BTYPE) == VT_LDOUBLE ? RC_ST0 : RC_FLOAT;

    /* convert constants to memory references */
    if ((vtop[-1].r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
        vswap();
        gv(float_type);
        vswap();
    }
    if ((vtop[0].r & (VT_VALMASK | VT_LVAL)) == VT_CONST)
        gv(float_type);

	swapped = 0;
	fc = vtop->c.ul;
	ft = vtop->type.t;

    if ((ft & VT_BTYPE) == VT_LDOUBLE) {
		/* swap the stack if needed so that t1 is the register and t2 is
		the memory reference */
		/* must put at least one value in the floating point register */
		if ((vtop[-1].r & VT_LVAL) && (vtop[0].r & VT_LVAL)) {
			vswap();
			gv(float_type);
			vswap();
		}
		if (vtop[-1].r & VT_LVAL) {
			vswap();
			swapped = 1;
		}
        if (op >= TOK_ULT && op <= TOK_GT) {
            /* load on stack second operand */
            load(TREG_ST0, vtop);
            save_reg(TREG_RAX); /* eax is used by FP comparison code */
            if (op == TOK_GE || op == TOK_GT)
                swapped = !swapped;
            else if (op == TOK_EQ || op == TOK_NE)
                swapped = 0;
            if (swapped)
                o(0xc9d9); /* fxch %st(1) */
			if (op == TOK_EQ || op == TOK_NE)
				o(0xe9da); /* fucompp */
			else
				o(0xd9de); /* fcompp */
            o(0xe0df); /* fnstsw %ax */
            if (op == TOK_EQ) {
                o(0x45e480); /* and $0x45, %ah */
                o(0x40fC80); /* cmp $0x40, %ah */
            } else if (op == TOK_NE) {
                o(0x45e480); /* and $0x45, %ah */
                o(0x40f480); /* xor $0x40, %ah */
                op = TOK_NE;
            } else if (op == TOK_GE || op == TOK_LE) {
                o(0x05c4f6); /* test $0x05, %ah */
                op = TOK_EQ;
            } else {
                o(0x45c4f6); /* test $0x45, %ah */
                op = TOK_EQ;
            }
            vtop--;
            vtop->r = VT_CMP;
            vtop->c.i = op;
        } else {
            /* no memory reference possible for long double operations */
            load(TREG_ST0, vtop);
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
            o(0xde); /* fxxxp %st, %st(1) */
            o(0xc1 + (a << 3));
            vtop--;
        }
    } else {
		vswap();
		gv(float_type);
		vswap();
		fr = vtop->r;
		r = vtop[-1].r;
        if (op >= TOK_ULT && op <= TOK_GT) {
			switch(op){
			case TOK_LE:
				op = TOK_ULE; /* setae */
				break;
			case TOK_LT:
				op = TOK_ULT;
				break;
			case TOK_GE:
				op = TOK_UGE;
				break;
			case TOK_GT:
				op = TOK_UGT; /* seta */
				break;
			}
			assert(!(vtop[-1].r & VT_LVAL));
			if ((ft & VT_BTYPE) == VT_DOUBLE)
				o(0x66);
			o(0x2e0f); /* ucomisd */
			if(fr >= VT_CONST)
				gen_modrm(r, fr, vtop->sym, fc);
			else
				o(0xc0 + REG_VALUE(fr) + REG_VALUE(r)*8);
            vtop--;
            vtop->r = VT_CMP;
            vtop->c.i = op | 0x100;
        } else {
			assert((vtop->type.t & VT_BTYPE) != VT_LDOUBLE);
            /* no memory reference possible for long double operations */
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
			assert((ft & VT_BTYPE) != VT_LDOUBLE);
			assert(!(vtop[-1].r & VT_LVAL));
			if ((ft & VT_BTYPE) == VT_DOUBLE) {
				o(0xf2);
			} else {
				o(0xf3);
			}
			o(0x0f);
			o(0x58 + a);
			if(fr >= VT_CONST)
				gen_modrm(r, fr, vtop->sym, fc);
			else
				o(0xc0 + REG_VALUE(fr) + REG_VALUE(r)*8);
			vtop--;
        }
    }
}

/* convert integers to fp 't' type. Must handle 'int', 'unsigned int'
   and 'long long' cases. */
void gen_cvt_itof(int t)
{
	int ft, bt, tbt, r;

    ft = vtop->type.t;
    bt = ft & VT_BTYPE;
    tbt = t & VT_BTYPE;
	r = gv(RC_INT);

    if (tbt == VT_LDOUBLE) {
        save_reg(TREG_ST0);
        if ((ft & VT_BTYPE) == VT_LLONG) {
            /* signed long long to float/double/long double (unsigned case
               is handled generically) */
            o(0x50 + REG_VALUE(r)); /* push r */
            o(0x242cdf); /* fildll (%rsp) */
            o(0x08c48348); /* add $8, %rsp */
        } else if ((ft & (VT_BTYPE | VT_UNSIGNED)) == (VT_INT | VT_UNSIGNED)) {
            /* unsigned int to float/double/long double */
            o(0x6a); /* push $0 */
            g(0x00);
            o(0x50 + REG_VALUE(r)); /* push r */
            o(0x242cdf); /* fildll (%rsp) */
            o(0x10c48348); /* add $16, %rsp */
        } else {
            /* int to float/double/long double */
            o(0x50 + REG_VALUE(r)); /* push r */
            o(0x2404db); /* fildl (%rsp) */
            o(0x08c48348); /* add $8, %rsp */
        }
        vtop->r = TREG_ST0;
    } else {
		int r_xmm;
        r_xmm = get_reg(RC_FLOAT);
        o(0xf2 + (tbt == VT_FLOAT));
        if ((ft & (VT_BTYPE | VT_UNSIGNED)) == (VT_INT | VT_UNSIGNED) || bt == VT_LLONG) {
            o(0x48); /* REX */
        }
        o(0x2a0f);
        o(0xc0 + REG_VALUE(r) + REG_VALUE(r_xmm)*8); /* cvtsi2sd or cvtsi2ss */
        vtop->r = r_xmm;
    }
}

/* convert from one floating point type to another */
void gen_cvt_ftof(int t)
{
    int ft, bt, tbt, r;

    ft = vtop->type.t;
    bt = ft & VT_BTYPE;
    tbt = t & VT_BTYPE;

	if(bt == VT_LDOUBLE)
		r = get_reg(RC_FLOAT);
	else
		r = gv(RC_FLOAT);
    if (bt == VT_FLOAT) {		
        if (tbt == VT_DOUBLE) {
            o(0x5a0f); /* cvtps2pd */
			o(0xc0 + REG_VALUE(r) + REG_VALUE(r) * 8);
        } else if (tbt == VT_LDOUBLE) {
            /* movss %xmm0-7,-0x10(%rsp) */
            o(0x110ff3);
            o(0xf02444 + REG_VALUE(r)*8);
            o(0xf02444d9); /* flds -0x10(%rsp) */
            vtop->r = TREG_ST0;
        }
    } else if (bt == VT_DOUBLE) {
        if (tbt == VT_FLOAT) {
            o(0x5a0f66); /* cvtpd2ps */
			o(0xc0 + REG_VALUE(r) + REG_VALUE(r) * 8);
        } else if (tbt == VT_LDOUBLE) {
            /* movsd %xmm0-7,-0x10(%rsp) */
            o(0x110ff2);
            o(0xf02444 + REG_VALUE(r)*8);
            o(0xf02444dd); /* fldl -0x10(%rsp) */
            vtop->r = TREG_ST0;
        }
    } else {
        gv(RC_ST0);
        if (tbt == VT_DOUBLE) {
            o(0xf0245cdd); /* fstpl -0x10(%rsp) */
            /* movsd -0x10(%rsp),%xmm0-7 */
            o(0x100ff2);
            o(0xf02444 + REG_VALUE(r)*8);
            vtop->r = r;
        } else if (tbt == VT_FLOAT) {
            o(0xf0245cd9); /* fstps -0x10(%rsp) */
            /* movss -0x10(%rsp),%xmm0-7 */
            o(0x100ff3);
            o(0xf02444 + REG_VALUE(r)*8);
            vtop->r = r;
        }
    }
}

/* convert fp to int 't' type */
void gen_cvt_ftoi(int t)
{
    int ft, bt, ll, r, r_xmm;

    ft = vtop->type.t;
    bt = ft & VT_BTYPE;

    if (bt == VT_LDOUBLE) {
        gen_cvt_ftof(VT_DOUBLE);
        bt = VT_DOUBLE;
    }
    r_xmm = gv(RC_FLOAT);
    if ((t & VT_BTYPE) == VT_INT)
        ll = 0;
    else
        ll = 1;
    r = get_reg(RC_INT);
    if (bt == VT_FLOAT) {
        o(0xf3);
    } else if (bt == VT_DOUBLE) {
        o(0xf2);
    } else {
        assert(0);
    }
    orex(ll, r, r_xmm, 0x2c0f); /* cvttss2si or cvttsd2si */
    o(0xc0 + REG_VALUE(r_xmm) + (REG_VALUE(r) << 3));
    vtop->r = r;
}

/* computed goto support */
void ggoto(void)
{
    gcall_or_jmp(1);
    vtop--;
}

/* Save the stack pointer onto the stack and return the location of its address */
ST_FUNC void gen_vla_sp_save(int addr) {
    /* mov %rsp,addr(%rbp)*/
    gen_modrm64(0x89, TREG_RSP, VT_LOCAL, NULL, addr);
}

/* Restore the SP from a location on the stack */
ST_FUNC void gen_vla_sp_restore(int addr) {
    gen_modrm64(0x8b, TREG_RSP, VT_LOCAL, NULL, addr);
}

/* Subtract from the stack pointer, and push the resulting value onto the stack */
ST_FUNC void gen_vla_alloc(CType *type, int align) {
    int r;
    r = gv(RC_INT); /* allocation size */
    /* sub r,%rsp */
    o(0x2b48);
    o(0xe0 | REG_VALUE(r));
	/* and ~15, %rsp */
    o(0xf0e48348);
    /* mov %rsp, r */
	orex(1, 0, r, 0x8d);
	o(0x2484 | (REG_VALUE(r)*8));
	r_loc = reloc_add(r_loc);
    vpop();
    vset(type, r, 0);
}


/* end of x86-64 code generator */
/*************************************************************/
#endif /* ! TARGET_DEFS_ONLY */
/******************************************************/
