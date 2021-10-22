/*
 *  ARMv4 code generator for TCC
 *
 *  Copyright (c) 2003 Daniel Glöckner
 *  Copyright (c) 2012 Thomas Preud'homme
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

#if defined(TCC_ARM_EABI) && !defined(TCC_ARM_VFP)
#error "Currently TinyCC only supports float computation with VFP instructions"
#endif

/* number of available registers */
#ifdef TCC_ARM_VFP
#define NB_REGS            13
#else
#define NB_REGS             9
#endif

#ifndef TCC_CPU_VERSION
# define TCC_CPU_VERSION 5
#endif

/* a register can belong to several classes. The classes must be
   sorted from more general to more precise (see gv2() code which does
   assumptions on it). */
#define RC_INT     0x0001 /* generic integer register */
#define RC_FLOAT   0x0002 /* generic float register */
#define RC_R0      0x0004
#define RC_R1      0x0008
#define RC_R2      0x0010
#define RC_R3      0x0020
#define RC_R12     0x0040
#define RC_F0      0x0080
#define RC_F1      0x0100
#define RC_F2      0x0200
#define RC_F3      0x0400
#ifdef TCC_ARM_VFP
#define RC_F4      0x0800
#define RC_F5      0x1000
#define RC_F6      0x2000
#define RC_F7      0x4000
#endif
#define RC_IRET    RC_R0  /* function return: integer register */
#define RC_IRE2    RC_R1  /* function return: second integer register */
#define RC_FRET    RC_F0  /* function return: float register */

/* pretty names for the registers */
enum {
    TREG_R0 = 0,
    TREG_R1,
    TREG_R2,
    TREG_R3,
    TREG_R12,
    TREG_F0,
    TREG_F1,
    TREG_F2,
    TREG_F3,
#ifdef TCC_ARM_VFP
    TREG_F4,
    TREG_F5,
    TREG_F6,
    TREG_F7,
#endif
    TREG_SP = 13,
    TREG_LR,
};

#ifdef TCC_ARM_VFP
#define T2CPR(t) (((t) & VT_BTYPE) != VT_FLOAT ? 0x100 : 0)
#endif

/* return registers for function */
#define REG_IRET TREG_R0 /* single word int return register */
#define REG_IRE2 TREG_R1 /* second word return register (for long long) */
#define REG_FRET TREG_F0 /* float return register */

#ifdef TCC_ARM_EABI
#define TOK___divdi3 TOK___aeabi_ldivmod
#define TOK___moddi3 TOK___aeabi_ldivmod
#define TOK___udivdi3 TOK___aeabi_uldivmod
#define TOK___umoddi3 TOK___aeabi_uldivmod
#endif

/* defined if function parameters must be evaluated in reverse order */
#define INVERT_FUNC_PARAMS

/* defined if structures are passed as pointers. Otherwise structures
   are directly pushed on stack. */
/* #define FUNC_STRUCT_PARAM_AS_PTR */

/* pointer size, in bytes */
#define PTR_SIZE 4

/* long double size and alignment, in bytes */
#ifdef TCC_ARM_VFP
#define LDOUBLE_SIZE  8
#endif

#ifndef LDOUBLE_SIZE
#define LDOUBLE_SIZE  8
#endif

#ifdef TCC_ARM_EABI
#define LDOUBLE_ALIGN 8
#else
#define LDOUBLE_ALIGN 4
#endif

/* maximum alignment (for aligned attribute support) */
#define MAX_ALIGN     8

#define CHAR_IS_UNSIGNED

#ifdef TCC_ARM_HARDFLOAT
# define ARM_FLOAT_ABI ARM_HARD_FLOAT
#else
# define ARM_FLOAT_ABI ARM_SOFTFP_FLOAT
#endif

/******************************************************/
#else /* ! TARGET_DEFS_ONLY */
/******************************************************/
#define USING_GLOBALS
#include "tcc.h"

ST_DATA const char * const target_machine_defs =
    "__arm__\0"
    "__arm\0"
    "arm\0"
    "__arm_elf__\0"
    "__arm_elf\0"
    "arm_elf\0"
    "__ARM_ARCH_4__\0"
    "__ARMEL__\0"
    "__APCS_32__\0"
#if defined TCC_ARM_EABI
    "__ARM_EABI__\0"
#endif
    ;

enum float_abi float_abi;

ST_DATA const int reg_classes[NB_REGS] = {
    /* r0 */ RC_INT | RC_R0,
    /* r1 */ RC_INT | RC_R1,
    /* r2 */ RC_INT | RC_R2,
    /* r3 */ RC_INT | RC_R3,
    /* r12 */ RC_INT | RC_R12,
    /* f0 */ RC_FLOAT | RC_F0,
    /* f1 */ RC_FLOAT | RC_F1,
    /* f2 */ RC_FLOAT | RC_F2,
    /* f3 */ RC_FLOAT | RC_F3,
#ifdef TCC_ARM_VFP
 /* d4/s8 */ RC_FLOAT | RC_F4,
/* d5/s10 */ RC_FLOAT | RC_F5,
/* d6/s12 */ RC_FLOAT | RC_F6,
/* d7/s14 */ RC_FLOAT | RC_F7,
#endif
};

static int func_sub_sp_offset, last_itod_magic;
static int leaffunc;

#if defined(TCC_ARM_EABI) && defined(TCC_ARM_VFP)
ST_FUNC void arm_init(TCCState *S)
{
    S->armgen_float_type.t = VT_FLOAT;
    S->armgen_double_type.t = VT_DOUBLE;
    S->armgen_func_float_type.t = VT_FUNC;
    S->armgen_func_float_type.ref = sym_push(S, SYM_FIELD, &S->armgen_float_type, FUNC_CDECL, FUNC_OLD);
    S->armgen_func_double_type.t = VT_FUNC;
    S->armgen_func_double_type.ref = sym_push(S, SYM_FIELD, &S->armgen_double_type, FUNC_CDECL, FUNC_OLD);

    float_abi = S->float_abi;
#ifndef TCC_ARM_HARDFLOAT
// XXX: Works on OpenBSD
// # warning "soft float ABI currently not supported: default to softfp"
#endif
}
#else
#define func_float_type func_old_type
#define func_double_type func_old_type
#define func_ldouble_type func_old_type
ST_FUNC void arm_init(struct TCCState *s)
{
#if 0
#if !defined (TCC_ARM_VFP)
    tcc_warning("Support for FPA is deprecated and will be removed in next"
                " release");
#endif
#if !defined (TCC_ARM_EABI)
    tcc_warning("Support for OABI is deprecated and will be removed in next"
                " release");
#endif
#endif
}
#endif

#define CHECK_R(r) ((r) >= TREG_R0 && (r) <= TREG_LR)

static int two2mask(TCCState *S, int a,int b) {
  if (!CHECK_R(a) || !CHECK_R(b))
    tcc_error(S, "compiler error! registers %i,%i is not valid",a,b);
  return (reg_classes[a]|reg_classes[b])&~(RC_INT|RC_FLOAT);
}

static int regmask(TCCState *S, int r) {
  if (!CHECK_R(r))
    tcc_error(S, "compiler error! register %i is not valid",r);
  return reg_classes[r]&~(RC_INT|RC_FLOAT);
}

/******************************************************/

#if defined(TCC_ARM_EABI) && !defined(CONFIG_TCC_ELFINTERP)
const char *default_elfinterp(struct TCCState *s)
{
    if (s->float_abi == ARM_HARD_FLOAT)
        return "/lib/ld-linux-armhf.so.3";
    else
        return "/lib/ld-linux.so.3";
}
#endif

void o(TCCState *S, uint32_t i)
{
  /* this is a good place to start adding big-endian support*/
  int ind1;
  if (S->nocode_wanted)
    return;
  ind1 = S->ind + 4;
  if (!cur_text_section)
    tcc_error(S, "compiler error! This happens f.ex. if the compiler\n"
         "can't evaluate constant expressions outside of a function.");
  if (ind1 > cur_text_section->data_allocated)
    section_realloc(S, cur_text_section, ind1);
  cur_text_section->data[S->ind++] = i&255;
  i>>=8;
  cur_text_section->data[S->ind++] = i&255;
  i>>=8;
  cur_text_section->data[S->ind++] = i&255;
  i>>=8;
  cur_text_section->data[S->ind++] = i;
}

static uint32_t stuff_const(uint32_t op, uint32_t c)
{
  int try_neg=0;
  uint32_t nc = 0, negop = 0;

  switch(op&0x1F00000)
  {
    case 0x800000: //add
    case 0x400000: //sub
      try_neg=1;
      negop=op^0xC00000;
      nc=-c;
      break;
    case 0x1A00000: //mov
    case 0x1E00000: //mvn
      try_neg=1;
      negop=op^0x400000;
      nc=~c;
      break;
    case 0x200000: //xor
      if(c==~0)
	return (op&0xF010F000)|((op>>16)&0xF)|0x1E00000;
      break;
    case 0x0: //and
      if(c==~0)
	return (op&0xF010F000)|((op>>16)&0xF)|0x1A00000;
    case 0x1C00000: //bic
      try_neg=1;
      negop=op^0x1C00000;
      nc=~c;
      break;
    case 0x1800000: //orr
      if(c==~0)
	return (op&0xFFF0FFFF)|0x1E00000;
      break;
  }
  do {
    uint32_t m;
    int i;
    if(c<256) /* catch undefined <<32 */
      return op|c;
    for(i=2;i<32;i+=2) {
      m=(0xff>>i)|(0xff<<(32-i));
      if(!(c&~m))
	return op|(i<<7)|(c<<i)|(c>>(32-i));
    }
    op=negop;
    c=nc;
  } while(try_neg--);
  return 0;
}


//only add,sub
void stuff_const_harder(TCCState *S, uint32_t op, uint32_t v) {
  uint32_t x;
  x=stuff_const(op,v);
  if(x)
    o(S, x);
  else {
    uint32_t a[16], nv, no, o2, n2;
    int i,j,k;
    a[0]=0xff;
    o2=(op&0xfff0ffff)|((op&0xf000)<<4);;
    for(i=1;i<16;i++)
      a[i]=(a[i-1]>>2)|(a[i-1]<<30);
    for(i=0;i<12;i++)
      for(j=i<4?i+12:15;j>=i+4;j--)
	if((v&(a[i]|a[j]))==v) {
	  o(S, stuff_const(op,v&a[i]));
	  o(S, stuff_const(o2,v&a[j]));
	  return;
	}
    no=op^0xC00000;
    n2=o2^0xC00000;
    nv=-v;
    for(i=0;i<12;i++)
      for(j=i<4?i+12:15;j>=i+4;j--)
	if((nv&(a[i]|a[j]))==nv) {
	  o(S, stuff_const(no,nv&a[i]));
	  o(S, stuff_const(n2,nv&a[j]));
	  return;
	}
    for(i=0;i<8;i++)
      for(j=i+4;j<12;j++)
	for(k=i<4?i+12:15;k>=j+4;k--)
	  if((v&(a[i]|a[j]|a[k]))==v) {
	    o(S, stuff_const(op,v&a[i]));
	    o(S, stuff_const(o2,v&a[j]));
	    o(S, stuff_const(o2,v&a[k]));
	    return;
	  }
    no=op^0xC00000;
    nv=-v;
    for(i=0;i<8;i++)
      for(j=i+4;j<12;j++)
	for(k=i<4?i+12:15;k>=j+4;k--)
	  if((nv&(a[i]|a[j]|a[k]))==nv) {
	    o(S, stuff_const(no,nv&a[i]));
	    o(S, stuff_const(n2,nv&a[j]));
	    o(S, stuff_const(n2,nv&a[k]));
	    return;
	  }
    o(S, stuff_const(op,v&a[0]));
    o(S, stuff_const(o2,v&a[4]));
    o(S, stuff_const(o2,v&a[8]));
    o(S, stuff_const(o2,v&a[12]));
  }
}

uint32_t encbranch(TCCState *S, int pos, int addr, int fail)
{
  addr-=pos+8;
  addr/=4;
  if(addr>=0x1000000 || addr<-0x1000000) {
    if(fail)
      tcc_error(S, "FIXME: function bigger than 32MB");
    return 0;
  }
  return 0x0A000000|(addr&0xffffff);
}

int decbranch(TCCState *S, int pos)
{
  int x;
  x=*(uint32_t *)(cur_text_section->data + pos);
  x&=0x00ffffff;
  if(x&0x800000)
    x-=0x1000000;
  return x*4+pos+8;
}

/* output a symbol and patch all calls to it */
void gsym_addr(TCCState *S, int t, int a)
{
  uint32_t *x;
  int lt;
  while(t) {
    x=(uint32_t *)(cur_text_section->data + t);
    t=decbranch(S, lt=t);
    if(a==lt+4)
      *x=0xE1A00000; // nop
    else {
      *x &= 0xff000000;
      *x |= encbranch(S, lt,a,1);
    }
  }
}

#ifdef TCC_ARM_VFP
static uint32_t vfpr(TCCState *S, int r)
{
  if(r<TREG_F0 || r>TREG_F7)
    tcc_error(S, "compiler error! register %i is no vfp register",r);
  return r - TREG_F0;
}
#else
static uint32_t fpr(TCCState *S, int r)
{
  if(r<TREG_F0 || r>TREG_F3)
    tcc_error(S, "compiler error! register %i is no fpa register",r);
  return r - TREG_F0;
}
#endif

static uint32_t intr(TCCState *S, int r)
{
  if(r == TREG_R12)
    return 12;
  if(r >= TREG_R0 && r <= TREG_R3)
    return r - TREG_R0;
  if (!(r >= TREG_SP && r <= TREG_LR))
    tcc_error(S, "compiler error! register %i is no int register",r);
  return r + (13 - TREG_SP);
}

static void calcaddr(TCCState *S, uint32_t *base, int *off, int *sgn, int maxoff, unsigned shift)
{
  if(*off>maxoff || *off&((1<<shift)-1)) {
    uint32_t x, y;
    x=0xE280E000;
    if(*sgn)
      x=0xE240E000;
    x|=(*base)<<16;
    *base=14; // lr
    y=stuff_const(x,*off&~maxoff);
    if(y) {
      o(S, y);
      *off&=maxoff;
      return;
    }
    y=stuff_const(x,(*off+maxoff)&~maxoff);
    if(y) {
      o(S, y);
      *sgn=!*sgn;
      *off=((*off+maxoff)&~maxoff)-*off;
      return;
    }
    stuff_const_harder(S, x,*off&~maxoff);
    *off&=maxoff;
  }
}

static uint32_t mapcc(TCCState *S, int cc)
{
  switch(cc)
  {
    case TOK_ULT:
      return 0x30000000; /* CC/LO */
    case TOK_UGE:
      return 0x20000000; /* CS/HS */
    case TOK_EQ:
      return 0x00000000; /* EQ */
    case TOK_NE:
      return 0x10000000; /* NE */
    case TOK_ULE:
      return 0x90000000; /* LS */
    case TOK_UGT:
      return 0x80000000; /* HI */
    case TOK_Nset:
      return 0x40000000; /* MI */
    case TOK_Nclear:
      return 0x50000000; /* PL */
    case TOK_LT:
      return 0xB0000000; /* LT */
    case TOK_GE:
      return 0xA0000000; /* GE */
    case TOK_LE:
      return 0xD0000000; /* LE */
    case TOK_GT:
      return 0xC0000000; /* GT */
  }
  tcc_error(S, "unexpected condition code");
  return 0xE0000000; /* AL */
}

static int negcc(TCCState *S, int cc)
{
  switch(cc)
  {
    case TOK_ULT:
      return TOK_UGE;
    case TOK_UGE:
      return TOK_ULT;
    case TOK_EQ:
      return TOK_NE;
    case TOK_NE:
      return TOK_EQ;
    case TOK_ULE:
      return TOK_UGT;
    case TOK_UGT:
      return TOK_ULE;
    case TOK_Nset:
      return TOK_Nclear;
    case TOK_Nclear:
      return TOK_Nset;
    case TOK_LT:
      return TOK_GE;
    case TOK_GE:
      return TOK_LT;
    case TOK_LE:
      return TOK_GT;
    case TOK_GT:
      return TOK_LE;
  }
  tcc_error(S, "unexpected condition code");
  return TOK_NE;
}

/* Load value into register r.
   Use relative/got addressing to avoid setting DT_TEXTREL */
static void load_value(TCCState *S, SValue *sv, int r)
{
    o(S, 0xE59F0000|(intr(S, r)<<12)); /* ldr r, [pc] */
    o(S, 0xEA000000); /* b $+4 */
#ifndef CONFIG_TCC_PIE
    if(sv->r & VT_SYM)
        greloc(S, cur_text_section, sv->sym, S->ind, R_ARM_ABS32);
    o(S, sv->c.i);
#else
    if(sv->r & VT_SYM) {
	if (sv->sym->type.t & VT_STATIC) {
            greloc(cur_text_section, sv->sym, ind, R_ARM_REL32);
            o(S, sv->c.i - 12);
            o(S, 0xe080000f | (intr(S, r)<<12) | (intr(S, r)<<16));  // add rx,rx,pc
        }
        else {
            greloc(cur_text_section, sv->sym, ind, R_ARM_GOT_PREL);
            o(S, -12);
            o(S, 0xe080000f | (intr(S, r)<<12) | (intr(S, r)<<16));  // add rx,rx,pc
            o(S, 0xe5900000 | (intr(S, r)<<12) | (intr(S, r)<<16));  // ldr rx,[rx]
            if (sv->c.i)
              stuff_const_harder(S, 0xe2800000 | (intr(S, r)<<12) | (intr(S, r)<<16),
                                 sv->c.i);
        }
    }
    else
        o(S, sv->c.i);
#endif
}

/* load 'r' from value 'sv' */
void load(TCCState *S, int r, SValue *sv)
{
  int v, ft, fc, fr, sign;
  uint32_t op;
  SValue v1;

  fr = sv->r;
  ft = sv->type.t;
  fc = sv->c.i;

  if(fc>=0)
    sign=0;
  else {
    sign=1;
    fc=-fc;
  }

  v = fr & VT_VALMASK;
  if (fr & VT_LVAL) {
    uint32_t base = 0xB; // fp
    if(v == VT_LLOCAL) {
      v1.type.t = VT_PTR;
      v1.r = VT_LOCAL | VT_LVAL;
      v1.c.i = sv->c.i;
      load(S, TREG_LR, &v1);
      base = 14; /* lr */
      fc=sign=0;
      v=VT_LOCAL;
    } else if(v == VT_CONST) {
      v1.type.t = VT_PTR;
      v1.r = fr&~VT_LVAL;
      v1.c.i = sv->c.i;
      v1.sym=sv->sym;
      load(S, TREG_LR, &v1);
      base = 14; /* lr */
      fc=sign=0;
      v=VT_LOCAL;
    } else if(v < VT_CONST) {
      base=intr(S, v);
      fc=sign=0;
      v=VT_LOCAL;
    }
    if(v == VT_LOCAL) {
      if(is_float(ft)) {
	calcaddr(S, &base,&fc,&sign,1020,2);
#ifdef TCC_ARM_VFP
        op=0xED100A00; /* flds */
        if(!sign)
          op|=0x800000;
        if ((ft & VT_BTYPE) != VT_FLOAT)
          op|=0x100;   /* flds -> fldd */
        o(S, op|(vfpr(S, r)<<12)|(fc>>2)|(base<<16));
#else
	op=0xED100100;
	if(!sign)
	  op|=0x800000;
#if LDOUBLE_SIZE == 8
	if ((ft & VT_BTYPE) != VT_FLOAT)
	  op|=0x8000;
#else
	if ((ft & VT_BTYPE) == VT_DOUBLE)
	  op|=0x8000;
	else if ((ft & VT_BTYPE) == VT_LDOUBLE)
	  op|=0x400000;
#endif
	o(S, op|(fpr(S, r)<<12)|(fc>>2)|(base<<16));
#endif
      } else if((ft & (VT_BTYPE|VT_UNSIGNED)) == VT_BYTE
                || (ft & VT_BTYPE) == VT_SHORT) {
	calcaddr(S, &base,&fc,&sign,255,0);
	op=0xE1500090;
	if ((ft & VT_BTYPE) == VT_SHORT)
	  op|=0x20;
	if ((ft & VT_UNSIGNED) == 0)
	  op|=0x40;
	if(!sign)
	  op|=0x800000;
	o(S, op|(intr(S, r)<<12)|(base<<16)|((fc&0xf0)<<4)|(fc&0xf));
      } else {
	calcaddr(S, &base,&fc,&sign,4095,0);
	op=0xE5100000;
	if(!sign)
	  op|=0x800000;
        if ((ft & VT_BTYPE) == VT_BYTE || (ft & VT_BTYPE) == VT_BOOL)
          op|=0x400000;
        o(S, op|(intr(S, r)<<12)|fc|(base<<16));
      }
      return;
    }
  } else {
    if (v == VT_CONST) {
      op=stuff_const(0xE3A00000|(intr(S, r)<<12),sv->c.i);
      if (fr & VT_SYM || !op)
	load_value(S, sv, r);
      else
        o(S, op);
      return;
    } else if (v == VT_LOCAL) {
      op=stuff_const(0xE28B0000|(intr(S, r)<<12),sv->c.i);
      if (fr & VT_SYM || !op) {
	load_value(S, sv, r);
	o(S, 0xE08B0000|(intr(S, r)<<12)|intr(S, r));
      } else
	o(S, op);
      return;
    } else if(v == VT_CMP) {
      o(S, mapcc(S, sv->c.i)|0x3A00001|(intr(S, r)<<12));
      o(S, mapcc(S, negcc(S, sv->c.i))|0x3A00000|(intr(S, r)<<12));
      return;
    } else if (v == VT_JMP || v == VT_JMPI) {
      int t;
      t = v & 1;
      o(S, 0xE3A00000|(intr(S, r)<<12)|t);
      o(S, 0xEA000000);
      gsym(S, sv->c.i);
      o(S, 0xE3A00000|(intr(S, r)<<12)|(t^1));
      return;
    } else if (v < VT_CONST) {
      if(is_float(ft))
#ifdef TCC_ARM_VFP
        o(S, 0xEEB00A40|(vfpr(S, r)<<12)|vfpr(S, v)|T2CPR(ft)); /* fcpyX */
#else
	o(S, 0xEE008180|(fpr(S, r)<<12)|fpr(S, v));
#endif
      else
	o(S, 0xE1A00000|(intr(S, r)<<12)|intr(S, v));
      return;
    }
  }
  tcc_error(S, "load unimplemented!");
}

/* store register 'r' in lvalue 'v' */
void store(TCCState *S, int r, SValue *sv)
{
  SValue v1;
  int v, ft, fc, fr, sign;
  uint32_t op;

  fr = sv->r;
  ft = sv->type.t;
  fc = sv->c.i;

  if(fc>=0)
    sign=0;
  else {
    sign=1;
    fc=-fc;
  }

  v = fr & VT_VALMASK;
  if (fr & VT_LVAL || fr == VT_LOCAL) {
    uint32_t base = 0xb; /* fp */
    if(v < VT_CONST) {
      base=intr(S, v);
      v=VT_LOCAL;
      fc=sign=0;
    } else if(v == VT_CONST) {
      v1.type.t = ft;
      v1.r = fr&~VT_LVAL;
      v1.c.i = sv->c.i;
      v1.sym=sv->sym;
      load(S, TREG_LR, &v1);
      base = 14; /* lr */
      fc=sign=0;
      v=VT_LOCAL;
    }
    if(v == VT_LOCAL) {
       if(is_float(ft)) {
	calcaddr(S, &base,&fc,&sign,1020,2);
#ifdef TCC_ARM_VFP
        op=0xED000A00; /* fsts */
        if(!sign)
          op|=0x800000;
        if ((ft & VT_BTYPE) != VT_FLOAT)
          op|=0x100;   /* fsts -> fstd */
        o(S, op|(vfpr(S, r)<<12)|(fc>>2)|(base<<16));
#else
	op=0xED000100;
	if(!sign)
	  op|=0x800000;
#if LDOUBLE_SIZE == 8
	if ((ft & VT_BTYPE) != VT_FLOAT)
	  op|=0x8000;
#else
	if ((ft & VT_BTYPE) == VT_DOUBLE)
	  op|=0x8000;
	if ((ft & VT_BTYPE) == VT_LDOUBLE)
	  op|=0x400000;
#endif
	o(S, op|(fpr(S, r)<<12)|(fc>>2)|(base<<16));
#endif
	return;
      } else if((ft & VT_BTYPE) == VT_SHORT) {
	calcaddr(S, &base,&fc,&sign,255,0);
	op=0xE14000B0;
	if(!sign)
	  op|=0x800000;
	o(S, op|(intr(S, r)<<12)|(base<<16)|((fc&0xf0)<<4)|(fc&0xf));
      } else {
	calcaddr(S, &base,&fc,&sign,4095,0);
	op=0xE5000000;
	if(!sign)
	  op|=0x800000;
        if ((ft & VT_BTYPE) == VT_BYTE || (ft & VT_BTYPE) == VT_BOOL)
          op|=0x400000;
        o(S, op|(intr(S, r)<<12)|fc|(base<<16));
      }
      return;
    }
  }
  tcc_error(S, "store unimplemented");
}

static void gadd_sp(TCCState *S, int val)
{
  stuff_const_harder(S, 0xE28DD000,val);
}

/* 'is_jmp' is '1' if it is a jump */
static void gcall_or_jmp(TCCState *S, int is_jmp)
{
  int r;
  uint32_t x;
  if ((S->vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
    /* constant case */
    if(S->vtop->r & VT_SYM){
	x=encbranch(S, S->ind, S->ind+S->vtop->c.i,0);
	if(x) {
	    /* relocation case */
	    greloc(S, cur_text_section, S->vtop->sym, S->ind, R_ARM_PC24);
	    o(S, x|(is_jmp?0xE0000000:0xE1000000));
	} else {
	    r = TREG_LR;
	    load_value(S, S->vtop, r);
	    if(is_jmp)
	        o(S, 0xE1A0F000 | intr(S, r)); // mov pc, r
	    else
		o(S, 0xe12fff30 | intr(S, r)); // blx r
	}
     }else{
	if(!is_jmp)
	    o(S, 0xE28FE004); // add lr,pc,#4
	o(S, 0xE51FF004);   // ldr pc,[pc,#-4]
	o(S, S->vtop->c.i);
     }
  } else {
    /* otherwise, indirect call */
#ifdef CONFIG_TCC_BCHECK
    S->vtop->r &= ~VT_MUSTBOUND;
#endif
    r = gv(S, RC_INT);
    if(!is_jmp)
      o(S, 0xE1A0E00F);       // mov lr,pc
    o(S, 0xE1A0F000|intr(S, r)); // mov pc,r
  }
}

#if defined(CONFIG_TCC_BCHECK)

static void gen_bounds_call(TCCState *S, int v)
{
    Sym *sym = external_helper_sym(S, v);

    greloc(S, cur_text_section, sym, S->ind, R_ARM_PC24);
    o(S, 0xebfffffe);
}

static void gen_bounds_prolog(TCCState *S)
{
    /* leave some room for bound checking code */
    S->func_bound_offset = lbounds_section->data_offset;
    S->func_bound_ind = S->ind;
    S->func_bound_add_epilog = 0;
    o(S, 0xe1a00000);  /* ld r0,lbounds_section->data_offset */
    o(S, 0xe1a00000);
    o(S, 0xe1a00000);
    o(S, 0xe1a00000);
    o(S, 0xe1a00000);  /* call __bound_local_new */
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

    sym_data = get_sym_ref(S, &S->char_pointer_type, lbounds_section,
                           S->func_bound_offset, lbounds_section->data_offset);

    /* generate bound local allocation */
    if (offset_modified) {
        saved_ind = S->ind;
        S->ind = S->func_bound_ind;
        o(S, 0xe59f0000);  /* ldr r0, [pc] */
        o(S, 0xea000000);  /* b $+4 */
        greloc(S, cur_text_section, sym_data, S->ind, R_ARM_REL32);
        o(S, -12);  /* lbounds_section->data_offset */
	o(S, 0xe080000f);  /* add r0,r0,pc */
        gen_bounds_call(S, TOK___bound_local_new);
        S->ind = saved_ind;
    }

    /* generate bound check local freeing */
    o(S, 0xe92d0003);  /* push {r0,r1} */
    o(S, 0xed2d0b02);  /* vpush {d0} */
    o(S, 0xe59f0000);  /* ldr r0, [pc] */
    o(S, 0xea000000);  /* b $+4 */
    greloc(S, cur_text_section, sym_data, S->ind, R_ARM_REL32);
    o(S, -12);  /* lbounds_section->data_offset */
    o(S, 0xe080000f);  /* add r0,r0,pc */
    gen_bounds_call(S, TOK___bound_local_delete);
    o(S, 0xecbd0b02); /* vpop {d0} */
    o(S, 0xe8bd0003); /* pop {r0,r1} */
}
#endif

static int unalias_ldbl(int btype)
{
#if LDOUBLE_SIZE == 8
    if (btype == VT_LDOUBLE)
      btype = VT_DOUBLE;
#endif
    return btype;
}

/* Return whether a structure is an homogeneous float aggregate or not.
   The answer is true if all the elements of the structure are of the same
   primitive float type and there is less than 4 elements.

   type: the type corresponding to the structure to be tested */
static int is_hgen_float_aggr(CType *type)
{
  if ((type->t & VT_BTYPE) == VT_STRUCT) {
    struct Sym *ref;
    int btype, nb_fields = 0;

    ref = type->ref->next;
    if (ref) {
      btype = unalias_ldbl(ref->type.t & VT_BTYPE);
      if (btype == VT_FLOAT || btype == VT_DOUBLE) {
        for(; ref && btype == unalias_ldbl(ref->type.t & VT_BTYPE); ref = ref->next, nb_fields++);
        return !ref && nb_fields <= 4;
      }
    }
  }
  return 0;
}

struct avail_regs {
  signed char avail[3]; /* 3 holes max with only float and double alignments */
  int first_hole; /* first available hole */
  int last_hole; /* last available hole (none if equal to first_hole) */
  int first_free_reg; /* next free register in the sequence, hole excluded */
};

/* Find suitable registers for a VFP Co-Processor Register Candidate (VFP CPRC
   param) according to the rules described in the procedure call standard for
   the ARM architecture (AAPCS). If found, the registers are assigned to this
   VFP CPRC parameter. Registers are allocated in sequence unless a hole exists
   and the parameter is a single float.

   avregs: opaque structure to keep track of available VFP co-processor regs
   align: alignment constraints for the param, as returned by type_size()
   size: size of the parameter, as returned by type_size() */
int assign_vfpreg(struct avail_regs *avregs, int align, int size)
{
  int first_reg = 0;

  if (avregs->first_free_reg == -1)
    return -1;
  if (align >> 3) { /* double alignment */
    first_reg = avregs->first_free_reg;
    /* alignment constraint not respected so use next reg and record hole */
    if (first_reg & 1)
      avregs->avail[avregs->last_hole++] = first_reg++;
  } else { /* no special alignment (float or array of float) */
    /* if single float and a hole is available, assign the param to it */
    if (size == 4 && avregs->first_hole != avregs->last_hole)
      return avregs->avail[avregs->first_hole++];
    else
      first_reg = avregs->first_free_reg;
  }
  if (first_reg + size / 4 <= 16) {
    avregs->first_free_reg = first_reg + size / 4;
    return first_reg;
  }
  avregs->first_free_reg = -1;
  return -1;
}

/* Returns whether all params need to be passed in core registers or not.
   This is the case for function part of the runtime ABI. */
int floats_in_core_regs(SValue *sval)
{
  if (!sval->sym)
    return 0;

  switch (sval->sym->v) {
    case TOK___floatundisf:
    case TOK___floatundidf:
    case TOK___fixunssfdi:
    case TOK___fixunsdfdi:
#ifndef TCC_ARM_VFP
    case TOK___fixunsxfdi:
#endif
    case TOK___floatdisf:
    case TOK___floatdidf:
    case TOK___fixsfdi:
    case TOK___fixdfdi:
      return 1;

    default:
      return 0;
  }
}

/* Return the number of registers needed to return the struct, or 0 if
   returning via struct pointer. */
ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret, int *ret_align, int *regsize) {
#ifdef TCC_ARM_EABI
    int size, align;
    size = type_size(vt, &align);
    if (float_abi == ARM_HARD_FLOAT && !variadic &&
        (is_float(vt->t) || is_hgen_float_aggr(vt))) {
        *ret_align = 8;
	*regsize = 8;
        ret->ref = NULL;
        ret->t = VT_DOUBLE;
        return (size + 7) >> 3;
    } else if (size > 0 && size <= 4) {
        *ret_align = 4;
	*regsize = 4;
        ret->ref = NULL;
        ret->t = VT_INT;
        return 1;
    } else
        return 0;
#else
    return 0;
#endif
}

/* Parameters are classified according to how they are copied to their final
   destination for the function call. Because the copying is performed class
   after class according to the order in the union below, it is important that
   some constraints about the order of the members of this union are respected:
   - CORE_STRUCT_CLASS must come after STACK_CLASS;
   - CORE_CLASS must come after STACK_CLASS, CORE_STRUCT_CLASS and
     VFP_STRUCT_CLASS;
   - VFP_STRUCT_CLASS must come after VFP_CLASS.
   See the comment for the main loop in copy_params() for the reason. */
enum reg_class {
	STACK_CLASS = 0,
	CORE_STRUCT_CLASS,
	VFP_CLASS,
	VFP_STRUCT_CLASS,
	CORE_CLASS,
	NB_CLASSES
};

struct param_plan {
    int start; /* first reg or addr used depending on the class */
    int end; /* last reg used or next free addr depending on the class */
    SValue *sval; /* pointer to SValue on the value stack */
    struct param_plan *prev; /*  previous element in this class */
};

struct plan {
    struct param_plan *pplans; /* array of all the param plans */
    struct param_plan *clsplans[NB_CLASSES]; /* per class lists of param plans */
    int nb_plans;
};

static void add_param_plan(struct plan* plan, int cls, int start, int end, SValue *v)
{
    struct param_plan *p = &plan->pplans[plan->nb_plans++];
    p->prev = plan->clsplans[cls];
    plan->clsplans[cls] = p;
    p->start = start, p->end = end, p->sval = v;
}

/* Assign parameters to registers and stack with alignment according to the
   rules in the procedure call standard for the ARM architecture (AAPCS).
   The overall assignment is recorded in an array of per parameter structures
   called parameter plans. The parameter plans are also further organized in a
   number of linked lists, one per class of parameter (see the comment for the
   definition of union reg_class).

   nb_args: number of parameters of the function for which a call is generated
   float_abi: float ABI in use for this function call
   plan: the structure where the overall assignment is recorded
   todo: a bitmap that record which core registers hold a parameter

   Returns the amount of stack space needed for parameter passing

   Note: this function allocated an array in plan->pplans with tcc_malloc. It
   is the responsibility of the caller to free this array once used (ie not
   before copy_params). */
static int assign_regs(TCCState *S, int nb_args, int float_abi, struct plan *plan, int *todo)
{
  int i, size, align;
  int ncrn /* next core register number */, nsaa /* next stacked argument address*/;
  struct avail_regs avregs = {{0}};

  ncrn = nsaa = 0;
  *todo = 0;

  for(i = nb_args; i-- ;) {
    int j, start_vfpreg = 0;
    CType type = S->vtop[-i].type;
    type.t &= ~VT_ARRAY;
    size = type_size(&type, &align);
    size = (size + 3) & ~3;
    align = (align + 3) & ~3;
    switch(S->vtop[-i].type.t & VT_BTYPE) {
      case VT_STRUCT:
      case VT_FLOAT:
      case VT_DOUBLE:
      case VT_LDOUBLE:
      if (float_abi == ARM_HARD_FLOAT) {
        int is_hfa = 0; /* Homogeneous float aggregate */

        if (is_float(S->vtop[-i].type.t)
            || (is_hfa = is_hgen_float_aggr(&S->vtop[-i].type))) {
          int end_vfpreg;

          start_vfpreg = assign_vfpreg(&avregs, align, size);
          end_vfpreg = start_vfpreg + ((size - 1) >> 2);
          if (start_vfpreg >= 0) {
            add_param_plan(plan, is_hfa ? VFP_STRUCT_CLASS : VFP_CLASS,
                start_vfpreg, end_vfpreg, &S->vtop[-i]);
            continue;
          } else
            break;
        }
      }
      ncrn = (ncrn + (align-1)/4) & ~((align/4) - 1);
      if (ncrn + size/4 <= 4 || (ncrn < 4 && start_vfpreg != -1)) {
        /* The parameter is allocated both in core register and on stack. As
	 * such, it can be of either class: it would either be the last of
	 * CORE_STRUCT_CLASS or the first of STACK_CLASS. */
        for (j = ncrn; j < 4 && j < ncrn + size / 4; j++)
          *todo|=(1<<j);
        add_param_plan(plan, CORE_STRUCT_CLASS, ncrn, j, &S->vtop[-i]);
        ncrn += size/4;
        if (ncrn > 4)
          nsaa = (ncrn - 4) * 4;
      } else {
        ncrn = 4;
        break;
      }
      continue;
      default:
      if (ncrn < 4) {
        int is_long = (S->vtop[-i].type.t & VT_BTYPE) == VT_LLONG;

        if (is_long) {
          ncrn = (ncrn + 1) & -2;
          if (ncrn == 4)
            break;
        }
        add_param_plan(plan, CORE_CLASS, ncrn, ncrn + is_long, &S->vtop[-i]);
        ncrn += 1 + is_long;
        continue;
      }
    }
    nsaa = (nsaa + (align - 1)) & ~(align - 1);
    add_param_plan(plan, STACK_CLASS, nsaa, nsaa + size, &S->vtop[-i]);
    nsaa += size; /* size already rounded up before */
  }
  return nsaa;
}

/* Copy parameters to their final destination (core reg, VFP reg or stack) for
   function call.

   nb_args: number of parameters the function take
   plan: the overall assignment plan for parameters
   todo: a bitmap indicating what core reg will hold a parameter

   Returns the number of SValue added by this function on the value stack */
static int copy_params(TCCState *S, int nb_args, struct plan *plan, int todo)
{
  int size, align, r, i, nb_extra_sval = 0;
  struct param_plan *pplan;
  int pass = 0;

   /* Several constraints require parameters to be copied in a specific order:
      - structures are copied to the stack before being loaded in a reg;
      - floats loaded to an odd numbered VFP reg are first copied to the
        preceding even numbered VFP reg and then moved to the next VFP reg.

      It is thus important that:
      - structures assigned to core regs must be copied after parameters
        assigned to the stack but before structures assigned to VFP regs because
        a structure can lie partly in core registers and partly on the stack;
      - parameters assigned to the stack and all structures be copied before
        parameters assigned to a core reg since copying a parameter to the stack
        require using a core reg;
      - parameters assigned to VFP regs be copied before structures assigned to
        VFP regs as the copy might use an even numbered VFP reg that already
        holds part of a structure. */
again:
  for(i = 0; i < NB_CLASSES; i++) {
    for(pplan = plan->clsplans[i]; pplan; pplan = pplan->prev) {

      if (pass
          && (i != CORE_CLASS || pplan->sval->r < VT_CONST))
        continue;

      vpushv(S, pplan->sval);
      pplan->sval->r = pplan->sval->r2 = VT_CONST; /* disable entry */
      switch(i) {
        case STACK_CLASS:
        case CORE_STRUCT_CLASS:
        case VFP_STRUCT_CLASS:
          if ((pplan->sval->type.t & VT_BTYPE) == VT_STRUCT) {
            int padding = 0;
            size = type_size(&pplan->sval->type, &align);
            /* align to stack align size */
            size = (size + 3) & ~3;
            if (i == STACK_CLASS && pplan->prev)
              padding = pplan->start - pplan->prev->end;
            size += padding; /* Add padding if any */
            /* allocate the necessary size on stack */
            gadd_sp(S, -size);
            /* generate structure store */
            r = get_reg(S, RC_INT);
            o(S, 0xE28D0000|(intr(S, r)<<12)|padding); /* add r, sp, padding */
            vset(S, &S->vtop->type, r | VT_LVAL, 0);
            vswap(S);
	    /* XXX: optimize. Save all register because memcpy can use them */
	    o(S, 0xED2D0A00|(0&1)<<22|(0>>1)<<12|16); /* vpush {s0-s15} */
            vstore(S); /* memcpy to current sp + potential padding */
	    o(S, 0xECBD0A00|(0&1)<<22|(0>>1)<<12|16); /* vpop {s0-s15} */

            /* Homogeneous float aggregate are loaded to VFP registers
               immediately since there is no way of loading data in multiple
               non consecutive VFP registers as what is done for other
               structures (see the use of todo). */
            if (i == VFP_STRUCT_CLASS) {
              int first = pplan->start, nb = pplan->end - first + 1;
              /* vpop.32 {pplan->start, ..., pplan->end} */
              o(S, 0xECBD0A00|(first&1)<<22|(first>>1)<<12|nb);
              /* No need to write the register used to a SValue since VFP regs
                 cannot be used for gcall_or_jmp */
            }
          } else {
            if (is_float(pplan->sval->type.t)) {
#ifdef TCC_ARM_VFP
              r = vfpr(S, gv(S, RC_FLOAT)) << 12;
              if ((pplan->sval->type.t & VT_BTYPE) == VT_FLOAT)
                size = 4;
              else {
                size = 8;
                r |= 0x101; /* vpush.32 -> vpush.64 */
              }
              o(S, 0xED2D0A01 + r); /* vpush */
#else
              r = fpr(S, gv(S, RC_FLOAT)) << 12;
              if ((pplan->sval->type.t & VT_BTYPE) == VT_FLOAT)
                size = 4;
              else if ((pplan->sval->type.t & VT_BTYPE) == VT_DOUBLE)
                size = 8;
              else
                size = LDOUBLE_SIZE;

              if (size == 12)
                r |= 0x400000;
              else if(size == 8)
                r|=0x8000;

              o(S, 0xED2D0100|r|(size>>2)); /* some kind of vpush for FPA */
#endif
            } else {
              /* simple type (currently always same size) */
              /* XXX: implicit cast ? */
              size=4;
              if ((pplan->sval->type.t & VT_BTYPE) == VT_LLONG) {
                lexpand(S);
                size = 8;
                r = gv(S, RC_INT);
                o(S, 0xE52D0004|(intr(S, r)<<12)); /* push r */
                S->vtop--;
              }
              r = gv(S, RC_INT);
              o(S, 0xE52D0004|(intr(S, r)<<12)); /* push r */
            }
            if (i == STACK_CLASS && pplan->prev)
              gadd_sp(S, pplan->prev->end - pplan->start); /* Add padding if any */
          }
          break;

        case VFP_CLASS:
          gv(S, regmask(S, TREG_F0 + (pplan->start >> 1)));
          if (pplan->start & 1) { /* Must be in upper part of double register */
            o(S, 0xEEF00A40|((pplan->start>>1)<<12)|(pplan->start>>1)); /* vmov.f32 s(n+1), sn */
            S->vtop->r = VT_CONST; /* avoid being saved on stack by gv for next float */
          }
          break;

        case CORE_CLASS:
          if ((pplan->sval->type.t & VT_BTYPE) == VT_LLONG) {
            lexpand(S);
            gv(S, regmask(S, pplan->end));
            pplan->sval->r2 = S->vtop->r;
            S->vtop--;
          }
          gv(S, regmask(S, pplan->start));
          /* Mark register as used so that gcall_or_jmp use another one
             (regs >=4 are free as never used to pass parameters) */
          pplan->sval->r = S->vtop->r;
          break;
      }
      S->vtop--;
    }
  }

  /* second pass to restore registers that were saved on stack by accident.
     Maybe redundant after the "lvalue_save" patch in tccgen.c:gv() */
  if (++pass < 2)
    goto again;

  /* Manually free remaining registers since next parameters are loaded
   * manually, without the help of gv(int). */
  save_regs(S, nb_args);

  if(todo) {
    o(S, 0xE8BD0000|todo); /* pop {todo} */
    for(pplan = plan->clsplans[CORE_STRUCT_CLASS]; pplan; pplan = pplan->prev) {
      int r;
      pplan->sval->r = pplan->start;
      /* An SValue can only pin 2 registers at best (r and r2) but a structure
         can occupy more than 2 registers. Thus, we need to push on the value
         stack some fake parameter to have on SValue for each registers used
         by a structure (r2 is not used). */
      for (r = pplan->start + 1; r <= pplan->end; r++) {
        if (todo & (1 << r)) {
          nb_extra_sval++;
          vpushi(S, 0);
          S->vtop->r = r;
        }
      }
    }
  }
  return nb_extra_sval;
}

/* Generate function call. The function address is pushed first, then
   all the parameters in call order. This functions pops all the
   parameters and the function address. */
void gfunc_call(TCCState *S, int nb_args)
{
  int r, args_size;
  int def_float_abi = float_abi;
  int todo;
  struct plan plan;
#ifdef TCC_ARM_EABI
  int variadic;
#endif

#ifdef CONFIG_TCC_BCHECK
  if (S->do_bounds_check)
    gbound_args(S, nb_args);
#endif

#ifdef TCC_ARM_EABI
  if (float_abi == ARM_HARD_FLOAT) {
    variadic = (S->vtop[-nb_args].type.ref->f.func_type == FUNC_ELLIPSIS);
    if (variadic || floats_in_core_regs(&S->vtop[-nb_args]))
      float_abi = ARM_SOFTFP_FLOAT;
  }
#endif
  /* cannot let cpu flags if other instruction are generated. Also avoid leaving
     VT_JMP anywhere except on the top of the stack because it would complicate
     the code generator. */
  r = S->vtop->r & VT_VALMASK;
  if (r == VT_CMP || (r & ~1) == VT_JMP)
    gv(S, RC_INT);

  memset(&plan, 0, sizeof plan);
  if (nb_args)
    plan.pplans = tcc_malloc(S, nb_args * sizeof(*plan.pplans));

  args_size = assign_regs(S, nb_args, float_abi, &plan, &todo);

#ifdef TCC_ARM_EABI
  if (args_size & 7) { /* Stack must be 8 byte aligned at fct call for EABI */
    args_size = (args_size + 7) & ~7;
    o(S, 0xE24DD004); /* sub sp, sp, #4 */
  }
#endif

  nb_args += copy_params(S, nb_args, &plan, todo);
  tcc_free(S, plan.pplans);

  /* Move fct SValue on top as required by gcall_or_jmp */
  vrotb(S, nb_args + 1);
  gcall_or_jmp(S, 0);
  if (args_size)
      gadd_sp(S, args_size); /* pop all parameters passed on the stack */
#if defined(TCC_ARM_EABI) && defined(TCC_ARM_VFP)
  if(float_abi == ARM_SOFTFP_FLOAT && is_float(S->vtop->type.ref->type.t)) {
    if((S->vtop->type.ref->type.t & VT_BTYPE) == VT_FLOAT) {
      o(S, 0xEE000A10); /*vmov s0, r0 */
    } else {
      o(S, 0xEE000B10); /* vmov.32 d0[0], r0 */
      o(S, 0xEE201B10); /* vmov.32 d0[1], r1 */
    }
  }
#endif
  S->vtop -= nb_args + 1; /* Pop all params and fct address from value stack */
  leaffunc = 0; /* we are calling a function, so we aren't in a leaf function */
  float_abi = def_float_abi;
}

/* generate function prolog of type 't' */
void gfunc_prolog(TCCState *S, Sym *func_sym)
{
  CType *func_type = &func_sym->type;
  Sym *sym,*sym2;
  int n, nf, size, align, rs, struct_ret = 0;
  int addr, pn, sn; /* pn=core, sn=stack */
  CType ret_type;

#ifdef TCC_ARM_EABI
  struct avail_regs avregs = {{0}};
#endif

  sym = func_type->ref;

  n = nf = 0;
  if ((S->tccgen_func_vt.t & VT_BTYPE) == VT_STRUCT &&
      !gfunc_sret(&S->tccgen_func_vt, S->tccgen_func_var, &ret_type, &align, &rs))
  {
    n++;
    struct_ret = 1;
    S->tccgen_func_vc = 12; /* Offset from fp of the place to store the result */
  }
  for(sym2 = sym->next; sym2 && (n < 4 || nf < 16); sym2 = sym2->next) {
    size = type_size(&sym2->type, &align);
#ifdef TCC_ARM_EABI
    if (float_abi == ARM_HARD_FLOAT && !S->tccgen_func_var &&
        (is_float(sym2->type.t) || is_hgen_float_aggr(&sym2->type))) {
      int tmpnf = assign_vfpreg(&avregs, align, size);
      tmpnf += (size + 3) / 4;
      nf = (tmpnf > nf) ? tmpnf : nf;
    } else
#endif
    if (n < 4)
      n += (size + 3) / 4;
  }
  o(S, 0xE1A0C00D); /* mov ip,sp */
  if (S->tccgen_func_var)
    n=4;
  if (n) {
    if(n>4)
      n=4;
#ifdef TCC_ARM_EABI
    n=(n+1)&-2;
#endif
    o(S, 0xE92D0000|((1<<n)-1)); /* save r0-r4 on stack if needed */
  }
  if (nf) {
    if (nf>16)
      nf=16;
    nf=(nf+1)&-2; /* nf => HARDFLOAT => EABI */
    o(S, 0xED2D0A00|nf); /* save s0-s15 on stack if needed */
  }
  o(S, 0xE92D5800); /* save fp, ip, lr */
  o(S, 0xE1A0B00D); /* mov fp, sp */
  func_sub_sp_offset = S->ind;
  o(S, 0xE1A00000); /* nop, leave space for stack adjustment in epilog */

#ifdef TCC_ARM_EABI
  if (float_abi == ARM_HARD_FLOAT) {
    S->tccgen_func_vc += nf * 4;
    memset(&avregs, 0, sizeof avregs);
  }
#endif
  pn = struct_ret, sn = 0;
  while ((sym = sym->next)) {
    CType *type;
    type = &sym->type;
    size = type_size(type, &align);
    size = (size + 3) >> 2;
    align = (align + 3) & ~3;
#ifdef TCC_ARM_EABI
    if (float_abi == ARM_HARD_FLOAT && !S->tccgen_func_var && (is_float(sym->type.t)
        || is_hgen_float_aggr(&sym->type))) {
      int fpn = assign_vfpreg(&avregs, align, size << 2);
      if (fpn >= 0)
        addr = fpn * 4;
      else
        goto from_stack;
    } else
#endif
    if (pn < 4) {
#ifdef TCC_ARM_EABI
        pn = (pn + (align-1)/4) & -(align/4);
#endif
      addr = (nf + pn) * 4;
      pn += size;
      if (!sn && pn > 4)
        sn = (pn - 4);
    } else {
#ifdef TCC_ARM_EABI
from_stack:
        sn = (sn + (align-1)/4) & -(align/4);
#endif
      addr = (n + nf + sn) * 4;
      sn += size;
    }
    sym_push(S, sym->v & ~SYM_FIELD, type, VT_LOCAL | VT_LVAL,
             addr + 12);
  }
  last_itod_magic=0;
  leaffunc = 1;
  S->loc = 0;
#ifdef CONFIG_TCC_BCHECK
  if (S->do_bounds_check)
    gen_bounds_prolog(S);
#endif
}

/* generate function epilog */
void gfunc_epilog(TCCState *S)
{
  uint32_t x;
  int diff;

#ifdef CONFIG_TCC_BCHECK
  if (S->do_bounds_check)
    gen_bounds_epilog(S);
#endif
  /* Copy float return value to core register if base standard is used and
     float computation is made with VFP */
#if defined(TCC_ARM_EABI) && defined(TCC_ARM_VFP)
  if ((float_abi == ARM_SOFTFP_FLOAT || S->tccgen_func_var) && is_float(S->tccgen_func_vt.t)) {
    if((S->tccgen_func_vt.t & VT_BTYPE) == VT_FLOAT)
      o(S, 0xEE100A10); /* fmrs r0, s0 */
    else {
      o(S, 0xEE100B10); /* fmrdl r0, d0 */
      o(S, 0xEE301B10); /* fmrdh r1, d0 */
    }
  }
#endif
  o(S, 0xE89BA800); /* restore fp, sp, pc */
  diff = (-S->loc + 3) & -4;
#ifdef TCC_ARM_EABI
  if(!leaffunc)
    diff = ((diff + 11) & -8) - 4;
#endif
  if(diff > 0) {
    x=stuff_const(0xE24BD000, diff); /* sub sp,fp,# */
    if(x)
      *(uint32_t *)(cur_text_section->data + func_sub_sp_offset) = x;
    else {
      int addr;
      addr=S->ind;
      o(S, 0xE59FC004); /* ldr ip,[pc+4] */
      o(S, 0xE04BD00C); /* sub sp,fp,ip  */
      o(S, 0xE1A0F00E); /* mov pc,lr */
      o(S, diff);
      *(uint32_t *)(cur_text_section->data + func_sub_sp_offset) = 0xE1000000|encbranch(S, func_sub_sp_offset,addr,1);
    }
  }
}

ST_FUNC void gen_fill_nops(TCCState *S, int bytes)
{
    if ((bytes & 3))
      tcc_error(S, "alignment of code section not multiple of 4");
    while (bytes > 0) {
	o(S, 0xE1A00000);
	bytes -= 4;
    }
}

/* generate a jump to a label */
ST_FUNC int gjmp(TCCState *S, int t)
{
  int r;
  if (S->nocode_wanted)
    return t;
  r=S->ind;
  o(S, 0xE0000000|encbranch(S, r,t,1));
  return r;
}

/* generate a jump to a fixed address */
ST_FUNC void gjmp_addr(TCCState *S, int a)
{
  gjmp(S, a);
}

ST_FUNC int gjmp_cond(TCCState *S, int op, int t)
{
  int r;
  if (S->nocode_wanted)
    return t;
  r=S->ind;
  op=mapcc(S, op);
  op|=encbranch(S, r,t,1);
  o(S, op);
  return r;
}

ST_FUNC int gjmp_append(TCCState *S, int n, int t)
{
  uint32_t *x;
  int p,lp;
  if(n) {
    p = n;
    do {
      p = decbranch(S, lp=p);
    } while(p);
    x = (uint32_t *)(cur_text_section->data + lp);
    *x &= 0xff000000;
    *x |= encbranch(S, lp,t,1);
    t = n;
  }
  return t;
}

/* generate an integer binary operation */
void gen_opi(TCCState *S, int op)
{
  int c, func = 0;
  uint32_t opc = 0, r, fr;
  unsigned short retreg = REG_IRET;

  c=0;
  switch(op) {
    case '+':
      opc = 0x8;
      c=1;
      break;
    case TOK_ADDC1: /* add with carry generation */
      opc = 0x9;
      c=1;
      break;
    case '-':
      opc = 0x4;
      c=1;
      break;
    case TOK_SUBC1: /* sub with carry generation */
      opc = 0x5;
      c=1;
      break;
    case TOK_ADDC2: /* add with carry use */
      opc = 0xA;
      c=1;
      break;
    case TOK_SUBC2: /* sub with carry use */
      opc = 0xC;
      c=1;
      break;
    case '&':
      opc = 0x0;
      c=1;
      break;
    case '^':
      opc = 0x2;
      c=1;
      break;
    case '|':
      opc = 0x18;
      c=1;
      break;
    case '*':
      gv2(S, RC_INT, RC_INT);
      r = S->vtop[-1].r;
      fr = S->vtop[0].r;
      S->vtop--;
      o(S, 0xE0000090|(intr(S, r)<<16)|(intr(S, r)<<8)|intr(S, fr));
      return;
    case TOK_SHL:
      opc = 0;
      c=2;
      break;
    case TOK_SHR:
      opc = 1;
      c=2;
      break;
    case TOK_SAR:
      opc = 2;
      c=2;
      break;
    case '/':
    case TOK_PDIV:
      func=TOK___divsi3;
      c=3;
      break;
    case TOK_UDIV:
      func=TOK___udivsi3;
      c=3;
      break;
    case '%':
#ifdef TCC_ARM_EABI
      func=TOK___aeabi_idivmod;
      retreg=REG_IRE2;
#else
      func=TOK___modsi3;
#endif
      c=3;
      break;
    case TOK_UMOD:
#ifdef TCC_ARM_EABI
      func=TOK___aeabi_uidivmod;
      retreg=REG_IRE2;
#else
      func=TOK___umodsi3;
#endif
      c=3;
      break;
    case TOK_UMULL:
      gv2(S, RC_INT, RC_INT);
      r=intr(S, S->vtop[-1].r2=get_reg(S, RC_INT));
      c=S->vtop[-1].r;
      S->vtop[-1].r=get_reg_ex(S, RC_INT,regmask(S, c));
      S->vtop--;
      o(S, 0xE0800090|(r<<16)|(intr(S, S->vtop->r)<<12)|(intr(S, c)<<8)|intr(S, S->vtop[1].r));
      return;
    default:
      opc = 0x15;
      c=1;
      break;
  }
  switch(c) {
    case 1:
      if((S->vtop[-1].r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST) {
	if(opc == 4 || opc == 5 || opc == 0xc) {
	  vswap(S);
	  opc|=2; // sub -> rsb
	}
      }
      if ((S->vtop->r & VT_VALMASK) == VT_CMP ||
          (S->vtop->r & (VT_VALMASK & ~1)) == VT_JMP)
        gv(S, RC_INT);
      vswap(S);
      c=intr(S, gv(S, RC_INT));
      vswap(S);
      opc=0xE0000000|(opc<<20);
      if((S->vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST) {
	uint32_t x;
	x=stuff_const(opc|0x2000000|(c<<16),S->vtop->c.i);
	if(x) {
	  if ((x & 0xfff00000) == 0xe3500000)   // cmp rx,#c
	    o(S, x);
	  else {
	    r=intr(S, S->vtop[-1].r=get_reg_ex(S, RC_INT,regmask(S, S->vtop[-1].r)));
	    o(S, x|(r<<12));
	  }
	  goto done;
	}
      }
      fr=intr(S, gv(S, RC_INT));
#ifdef CONFIG_TCC_BCHECK
      if ((S->vtop[-1].r & VT_VALMASK) >= VT_CONST) {
        vswap(S);
        c=intr(S, gv(S, RC_INT));
        vswap(S);
      }
#endif
      if ((opc & 0xfff00000) == 0xe1500000) // cmp rx,ry
	o(S, opc|(c<<16)|fr);
      else {
        r=intr(S, S->vtop[-1].r=get_reg_ex(S, RC_INT,two2mask(S, S->vtop->r,S->vtop[-1].r)));
        o(S, opc|(c<<16)|(r<<12)|fr);
      }
done:
      S->vtop--;
      if (op >= TOK_ULT && op <= TOK_GT)
        vset_VT_CMP(S, op);
      break;
    case 2:
      opc=0xE1A00000|(opc<<5);
      if ((S->vtop->r & VT_VALMASK) == VT_CMP ||
          (S->vtop->r & (VT_VALMASK & ~1)) == VT_JMP)
        gv(S, RC_INT);
      vswap(S);
      r=intr(S, gv(S, RC_INT));
      vswap(S);
      if ((S->vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST) {
	fr=intr(S, S->vtop[-1].r=get_reg_ex(S, RC_INT,regmask(S, S->vtop[-1].r)));
	c = S->vtop->c.i & 0x1f;
	o(S, opc|r|(c<<7)|(fr<<12));
      } else {
        fr=intr(S, gv(S, RC_INT));
#ifdef CONFIG_TCC_BCHECK
        if ((S->vtop[-1].r & VT_VALMASK) >= VT_CONST) {
          vswap(S);
          r=intr(S, gv(S, RC_INT));
          vswap(S);
        }
#endif
	c=intr(S, S->vtop[-1].r=get_reg_ex(S, RC_INT,two2mask(S, S->vtop->r,S->vtop[-1].r)));
	o(S, opc|r|(c<<12)|(fr<<8)|0x10);
      }
      S->vtop--;
      break;
    case 3:
      vpush_helper_func(S, func);
      vrott(S, 3);
      gfunc_call(S, 2);
      vpushi(S, 0);
      S->vtop->r = retreg;
      break;
    default:
      tcc_error(S, "gen_opi %i unimplemented!",op);
  }
}

#ifdef TCC_ARM_VFP
static int is_zero(TCCState *S, int i)
{
  if((S->vtop[i].r & (VT_VALMASK | VT_LVAL | VT_SYM)) != VT_CONST)
    return 0;
  if (S->vtop[i].type.t == VT_FLOAT)
    return (S->vtop[i].c.f == 0.f);
  else if (S->vtop[i].type.t == VT_DOUBLE)
    return (S->vtop[i].c.d == 0.0);
  return (S->vtop[i].c.ld == 0.l);
}

/* generate a floating point operation 'v = t1 op t2' instruction. The
 *    two operands are guaranteed to have the same floating point type */
void gen_opf(TCCState *S, int op)
{
  uint32_t x;
  int fneg=0,r;
  x=0xEE000A00|T2CPR(S->vtop->type.t);
  switch(op) {
    case '+':
      if(is_zero(S, -1))
        vswap(S);
      if(is_zero(S, 0)) {
        S->vtop--;
        return;
      }
      x|=0x300000;
      break;
    case '-':
      x|=0x300040;
      if(is_zero(S, 0)) {
        S->vtop--;
        return;
      }
      if(is_zero(S, -1)) {
        x|=0x810000; /* fsubX -> fnegX */
        vswap(S);
        S->vtop--;
        fneg=1;
      }
      break;
    case '*':
      x|=0x200000;
      break;
    case '/':
      x|=0x800000;
      break;
    default:
      if(op < TOK_ULT || op > TOK_GT) {
        tcc_error(S, "unknown fp op %x!",op);
        return;
      }
      if(is_zero(S, -1)) {
        vswap(S);
        switch(op) {
          case TOK_LT: op=TOK_GT; break;
          case TOK_GE: op=TOK_ULE; break;
          case TOK_LE: op=TOK_GE; break;
          case TOK_GT: op=TOK_ULT; break;
        }
      }
      x|=0xB40040; /* fcmpX */
      if(op!=TOK_EQ && op!=TOK_NE)
        x|=0x80; /* fcmpX -> fcmpeX */
      if(is_zero(S, 0)) {
        S->vtop--;
        o(S, x|0x10000|(vfpr(S, gv(S, RC_FLOAT))<<12)); /* fcmp(e)X -> fcmp(e)zX */
      } else {
        gv2(S, RC_FLOAT,RC_FLOAT);
        x|=vfpr(S, S->vtop[0].r);
        o(S, x|(vfpr(S, S->vtop[-1].r) << 12));
        S->vtop--;
      }
      o(S, 0xEEF1FA10); /* fmstat */

      switch(op) {
        case TOK_LE: op=TOK_ULE; break;
        case TOK_LT: op=TOK_ULT; break;
        case TOK_UGE: op=TOK_GE; break;
        case TOK_UGT: op=TOK_GT; break;
      }
      vset_VT_CMP(S, op);
      return;
  }
  r=gv(S, RC_FLOAT);
  x|=vfpr(S, r);
  r=regmask(S, r);
  if(!fneg) {
    int r2;
    vswap(S);
    r2=gv(S, RC_FLOAT);
    x|=vfpr(S, r2)<<16;
    r|=regmask(S, r2);
#ifdef CONFIG_TCC_BCHECK
    if ((S->vtop[-1].r & VT_VALMASK) >= VT_CONST) {
      vswap(S);
      r=gv(S, RC_FLOAT);
      vswap(S);
      x=(x&~0xf)|vfpr(S, r);
    }
#endif
  }
  S->vtop->r=get_reg_ex(S, RC_FLOAT,r);
  if(!fneg)
    S->vtop--;
  o(S, x|(vfpr(S, S->vtop->r)<<12));
}

#else
static uint32_t is_fconst(TCCState *S)
{
  long double f;
  uint32_t r;
  if((S->vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) != VT_CONST)
    return 0;
  if (S->vtop->type.t == VT_FLOAT)
    f = S->vtop->c.f;
  else if (S->vtop->type.t == VT_DOUBLE)
    f = S->vtop->c.d;
  else
    f = S->vtop->c.ld;
  if(!ieee_finite(f))
    return 0;
  r=0x8;
  if(f<0.0) {
    r=0x18;
    f=-f;
  }
  if(f==0.0)
    return r;
  if(f==1.0)
    return r|1;
  if(f==2.0)
    return r|2;
  if(f==3.0)
    return r|3;
  if(f==4.0)
    return r|4;
  if(f==5.0)
    return r|5;
  if(f==0.5)
    return r|6;
  if(f==10.0)
    return r|7;
  return 0;
}

/* generate a floating point operation 'v = t1 op t2' instruction. The
   two operands are guaranteed to have the same floating point type */
void gen_opf(TCCState *S, int op)
{
  uint32_t x, r, r2, c1, c2;
  //fputs("gen_opf\n",stderr);
  vswap(S);
  c1 = is_fconst(S);
  vswap(S);
  c2 = is_fconst(S);
  x=0xEE000100;
#if LDOUBLE_SIZE == 8
  if ((S->vtop->type.t & VT_BTYPE) != VT_FLOAT)
    x|=0x80;
#else
  if ((S->vtop->type.t & VT_BTYPE) == VT_DOUBLE)
    x|=0x80;
  else if ((S->vtop->type.t & VT_BTYPE) == VT_LDOUBLE)
    x|=0x80000;
#endif
  switch(op)
  {
    case '+':
      if(!c2) {
	vswap(S);
	c2=c1;
      }
      vswap(S);
      r=fpr(S, gv(S, RC_FLOAT));
      vswap(S);
      if(c2) {
	if(c2>0xf)
	  x|=0x200000; // suf
	r2=c2&0xf;
      } else {
	r2=fpr(S, gv(S, RC_FLOAT));
#ifdef CONFIG_TCC_BCHECK
        if ((S->vtop[-1].r & VT_VALMASK) >= VT_CONST) {
          vswap(S);
          r=fpr(S, gv(S, RC_FLOAT));
          vswap(S);
        }
#endif
      }
      break;
    case '-':
      if(c2) {
	if(c2<=0xf)
	  x|=0x200000; // suf
	r2=c2&0xf;
	vswap(S);
	r=fpr(S, gv(S, RC_FLOAT));
	vswap(S);
      } else if(c1 && c1<=0xf) {
	x|=0x300000; // rsf
	r2=c1;
	r=fpr(S, gv(S, RC_FLOAT));
	vswap(S);
      } else {
	x|=0x200000; // suf
	vswap(S);
	r=fpr(S, gv(S, RC_FLOAT));
	vswap(S);
	r2=fpr(S, gv(S, RC_FLOAT));
#ifdef CONFIG_TCC_BCHECK
        if ((S->vtop[-1].r & VT_VALMASK) >= VT_CONST) {
          vswap(S);
          r=fpr(S, gv(S, RC_FLOAT));
          vswap(S);
        }
#endif
      }
      break;
    case '*':
      if(!c2 || c2>0xf) {
	vswap(S);
	c2=c1;
      }
      vswap(S);
      r=fpr(S, gv(S, RC_FLOAT));
      vswap(S);
      if(c2 && c2<=0xf)
	r2=c2;
      else {
	r2=fpr(S, gv(S, RC_FLOAT));
#ifdef CONFIG_TCC_BCHECK
        if ((S->vtop[-1].r & VT_VALMASK) >= VT_CONST) {
          vswap(S);
          r=fpr(S, gv(S, RC_FLOAT));
          vswap(S);
        }
#endif
      }
      x|=0x100000; // muf
      break;
    case '/':
      if(c2 && c2<=0xf) {
	x|=0x400000; // dvf
	r2=c2;
	vswap(S);
	r=fpr(S, gv(S, RC_FLOAT));
	vswap(S);
      } else if(c1 && c1<=0xf) {
	x|=0x500000; // rdf
	r2=c1;
	r=fpr(S, gv(S, RC_FLOAT));
	vswap(S);
      } else {
	x|=0x400000; // dvf
	vswap(S);
	r=fpr(S, gv(S, RC_FLOAT));
	vswap(S);
	r2=fpr(S, gv(S, RC_FLOAT));
#ifdef CONFIG_TCC_BCHECK
        if ((S->vtop[-1].r & VT_VALMASK) >= VT_CONST) {
          vswap(S);
          r=fpr(S, gv(S, RC_FLOAT));
          vswap(S);
        }
#endif
      }
      break;
    default:
      if(op >= TOK_ULT && op <= TOK_GT) {
	x|=0xd0f110; // cmfe
/* bug (intention?) in Linux FPU emulator
   doesn't set carry if equal */
	switch(op) {
	  case TOK_ULT:
	  case TOK_UGE:
	  case TOK_ULE:
	  case TOK_UGT:
            tcc_error(S, "unsigned comparison on floats?");
	    break;
	  case TOK_LT:
            op=TOK_Nset;
	    break;
	  case TOK_LE:
            op=TOK_ULE; /* correct in unordered case only if AC bit in FPSR set */
	    break;
	  case TOK_EQ:
	  case TOK_NE:
	    x&=~0x400000; // cmfe -> cmf
	    break;
	}
	if(c1 && !c2) {
	  c2=c1;
	  vswap(S);
	  switch(op) {
            case TOK_Nset:
              op=TOK_GT;
	      break;
            case TOK_GE:
	      op=TOK_ULE;
	      break;
	    case TOK_ULE:
              op=TOK_GE;
	      break;
            case TOK_GT:
              op=TOK_Nset;
	      break;
	  }
	}
	vswap(S);
	r=fpr(S, gv(S, RC_FLOAT));
	vswap(S);
	if(c2) {
	  if(c2>0xf)
	    x|=0x200000;
	  r2=c2&0xf;
	} else {
	  r2=fpr(S, gv(S, RC_FLOAT));
#ifdef CONFIG_TCC_BCHECK
          if ((S->vtop[-1].r & VT_VALMASK) >= VT_CONST) {
            vswap(S);
            r=fpr(S, gv(S, RC_FLOAT));
            vswap(S);
          }
#endif
	}
        --S->vtop;
        vset_VT_CMP(S, op);
        ++S->vtop;
      } else {
        tcc_error(S, "unknown fp op %x!",op);
	return;
      }
  }
  if(S->vtop[-1].r == VT_CMP)
    c1=15;
  else {
    c1=S->vtop->r;
    if(r2&0x8)
      c1=S->vtop[-1].r;
    S->vtop[-1].r=get_reg_ex(S, RC_FLOAT,two2mask(S, S->vtop[-1].r,c1));
    c1=fpr(S, S->vtop[-1].r);
  }
  S->vtop--;
  o(S, x|(r<<16)|(c1<<12)|r2);
}
#endif

/* convert integers to fp 't' type. Must handle 'int', 'unsigned int'
   and 'long long' cases. */
ST_FUNC void gen_cvt_itof(TCCState *S, int t)
{
  uint32_t r, r2;
  int bt;
  bt=S->vtop->type.t & VT_BTYPE;
  if(bt == VT_INT || bt == VT_SHORT || bt == VT_BYTE) {
#ifndef TCC_ARM_VFP
    uint32_t dsize = 0;
#endif
    r=intr(S, gv(S, RC_INT));
#ifdef TCC_ARM_VFP
    r2=vfpr(S, S->vtop->r=get_reg(S, RC_FLOAT));
    o(S, 0xEE000A10|(r<<12)|(r2<<16)); /* fmsr */
    r2|=r2<<12;
    if(!(S->vtop->type.t & VT_UNSIGNED))
      r2|=0x80;                /* fuitoX -> fsituX */
    o(S, 0xEEB80A40|r2|T2CPR(t)); /* fYitoX*/
#else
    r2=fpr(S, S->vtop->r=get_reg(S, RC_FLOAT));
    if((t & VT_BTYPE) != VT_FLOAT)
      dsize=0x80;    /* flts -> fltd */
    o(S, 0xEE000110|dsize|(r2<<16)|(r<<12)); /* flts */
    if((S->vtop->type.t & (VT_UNSIGNED|VT_BTYPE)) == (VT_UNSIGNED|VT_INT)) {
      uint32_t off = 0;
      o(S, 0xE3500000|(r<<12));        /* cmp */
      r=fpr(S, get_reg(S, RC_FLOAT));
      if(last_itod_magic) {
	off=S->ind+8-last_itod_magic;
	off/=4;
	if(off>255)
	  off=0;
      }
      o(S, 0xBD1F0100|(r<<12)|off);    /* ldflts */
      if(!off) {
        o(S, 0xEA000000);              /* b */
        last_itod_magic=S->ind;
        o(S, 0x4F800000);              /* 4294967296.0f */
      }
      o(S, 0xBE000100|dsize|(r2<<16)|(r2<<12)|r); /* adflt */
    }
#endif
    return;
  } else if(bt == VT_LLONG) {
    int func;
    CType *func_type = 0;
    if((t & VT_BTYPE) == VT_FLOAT) {
      func_type = &S->armgen_func_float_type;
      if(S->vtop->type.t & VT_UNSIGNED)
        func=TOK___floatundisf;
      else
        func=TOK___floatdisf;
#if LDOUBLE_SIZE != 8
    } else if((t & VT_BTYPE) == VT_LDOUBLE) {
      func_type = &S->armgen_func_ldouble_type;
      if(S->vtop->type.t & VT_UNSIGNED)
        func=TOK___floatundixf;
      else
        func=TOK___floatdixf;
    } else if((t & VT_BTYPE) == VT_DOUBLE) {
#else
    } else if((t & VT_BTYPE) == VT_DOUBLE || (t & VT_BTYPE) == VT_LDOUBLE) {
#endif
      func_type = &S->armgen_func_double_type;
      if(S->vtop->type.t & VT_UNSIGNED)
        func=TOK___floatundidf;
      else
        func=TOK___floatdidf;
    }
    if(func_type) {
      vpushsym(S, func_type, external_helper_sym(S, func));
      vswap(S);
      gfunc_call(S, 1);
      vpushi(S, 0);
      S->vtop->r=TREG_F0;
      return;
    }
  }
  tcc_error(S, "unimplemented gen_cvt_itof %x!",S->vtop->type.t);
}

/* convert fp to int 't' type */
void gen_cvt_ftoi(TCCState *S, int t)
{
  uint32_t r, r2;
  int u, func = 0;
  u=t&VT_UNSIGNED;
  t&=VT_BTYPE;
  r2=S->vtop->type.t & VT_BTYPE;
  if(t==VT_INT) {
#ifdef TCC_ARM_VFP
    r=vfpr(S, gv(S, RC_FLOAT));
    u=u?0:0x10000;
    o(S, 0xEEBC0AC0|(r<<12)|r|T2CPR(r2)|u); /* ftoXizY */
    r2=intr(S, S->vtop->r=get_reg(S, RC_INT));
    o(S, 0xEE100A10|(r<<16)|(r2<<12));
    return;
#else
    if(u) {
      if(r2 == VT_FLOAT)
        func=TOK___fixunssfsi;
#if LDOUBLE_SIZE != 8
      else if(r2 == VT_LDOUBLE)
	func=TOK___fixunsxfsi;
      else if(r2 == VT_DOUBLE)
#else
      else if(r2 == VT_LDOUBLE || r2 == VT_DOUBLE)
#endif
	func=TOK___fixunsdfsi;
    } else {
      r=fpr(S, gv(S, RC_FLOAT));
      r2=intr(S, S->vtop->r=get_reg(S, RC_INT));
      o(S, 0xEE100170|(r2<<12)|r);
      return;
    }
#endif
  } else if(t == VT_LLONG) { // unsigned handled in gen_cvt_ftoi1
    if(r2 == VT_FLOAT)
      func=TOK___fixsfdi;
#if LDOUBLE_SIZE != 8
    else if(r2 == VT_LDOUBLE)
      func=TOK___fixxfdi;
    else if(r2 == VT_DOUBLE)
#else
    else if(r2 == VT_LDOUBLE || r2 == VT_DOUBLE)
#endif
      func=TOK___fixdfdi;
  }
  if(func) {
    vpush_helper_func(S, func);
    vswap(S);
    gfunc_call(S, 1);
    vpushi(S, 0);
    if(t == VT_LLONG)
      S->vtop->r2 = REG_IRE2;
    S->vtop->r = REG_IRET;
    return;
  }
  tcc_error(S, "unimplemented gen_cvt_ftoi!");
}

/* convert from one floating point type to another */
void gen_cvt_ftof(TCCState *S, int t)
{
#ifdef TCC_ARM_VFP
  if(((S->vtop->type.t & VT_BTYPE) == VT_FLOAT) != ((t & VT_BTYPE) == VT_FLOAT)) {
    uint32_t r = vfpr(S, gv(S, RC_FLOAT));
    o(S, 0xEEB70AC0|(r<<12)|r|T2CPR(S->vtop->type.t));
  }
#else
  /* all we have to do on i386 and FPA ARM is to put the float in a register */
  gv(S, RC_FLOAT);
#endif
}

/* increment tcov counter */
ST_FUNC void gen_increment_tcov (TCCState *S, SValue *sv)
{
  int r1, r2;

  vpushv(S, sv);
  S->vtop->r = r1 = get_reg(S, RC_INT);
  r2 = get_reg(S, RC_INT);
  o(S, 0xE59F0000 | (intr(S, r1)<<12)); // ldr r1,[pc]
  o(S, 0xEA000000); // b $+4
  greloc(S, cur_text_section, sv->sym, S->ind, R_ARM_REL32);
  o(S, -12);
  o(S, 0xe080000f | (intr(S, r1)<<16) | (intr(S, r1)<<12)); // add r1,r1,pc
  o(S, 0xe5900000 | (intr(S, r1)<<16) | (intr(S, r2)<<12)); // ldr r2, [r1]
  o(S, 0xe2900001 | (intr(S, r2)<<16) | (intr(S, r2)<<12)); // adds r2, r2, #1
  o(S, 0xe5800000 | (intr(S, r1)<<16) | (intr(S, r2)<<12)); // str r2, [r1]
  o(S, 0xe2800004 | (intr(S, r1)<<16) | (intr(S, r1)<<12)); // add r1, r1, #4
  o(S, 0xe5900000 | (intr(S, r1)<<16) | (intr(S, r2)<<12)); // ldr r2, [r1]
  o(S, 0xe2a00000 | (intr(S, r2)<<16) | (intr(S, r2)<<12)); // adc r2, r2, #0
  o(S, 0xe5800000 | (intr(S, r1)<<16) | (intr(S, r2)<<12)); // str r2, [r1]
  vpop(S);
}

/* computed goto support */
void ggoto(TCCState *S)
{
  gcall_or_jmp(S, 1);
  S->vtop--;
}

/* Save the stack pointer onto the stack and return the location of its address */
ST_FUNC void gen_vla_sp_save(TCCState *S, int addr) {
    SValue v;
    v.type.t = VT_PTR;
    v.r = VT_LOCAL | VT_LVAL;
    v.c.i = addr;
    store(S, TREG_SP, &v);
}

/* Restore the SP from a location on the stack */
ST_FUNC void gen_vla_sp_restore(TCCState *S, int addr) {
    SValue v;
    v.type.t = VT_PTR;
    v.r = VT_LOCAL | VT_LVAL;
    v.c.i = addr;
    load(S, TREG_SP, &v);
}

/* Subtract from the stack pointer, and push the resulting value onto the stack */
ST_FUNC void gen_vla_alloc(TCCState *S, CType *type, int align) {
    int r;
#if defined(CONFIG_TCC_BCHECK)
    if (S->do_bounds_check)
        vpushv(S, S->vtop);
#endif
    r = intr(S, gv(S, RC_INT));
#if defined(CONFIG_TCC_BCHECK)
    if (S->do_bounds_check)
        o(S, 0xe2800001 | (r<<16)|(r<<12)); /* add r,r,#1 */
#endif
    o(S, 0xE04D0000|(r<<12)|r); /* sub r, sp, r */
#ifdef TCC_ARM_EABI
    if (align < 8)
        align = 8;
#else
    if (align < 4)
        align = 4;
#endif
    if (align & (align - 1))
        tcc_error(S, "alignment is not a power of 2: %i", align);
    o(S, stuff_const(0xE3C0D000|(r<<16), align - 1)); /* bic sp, r, #align-1 */
    vpop(S);
#if defined(CONFIG_TCC_BCHECK)
    if (S->do_bounds_check) {
        vpushi(S, 0);
        S->vtop->r = TREG_R0;
        o(S, 0xe1a0000d | (S->vtop->r << 12)); // mov r0,sp
        vswap(S);
        vpush_helper_func(S, TOK___bound_new_region);
        vrott(S, 3);
        gfunc_call(S, 2);
        S->func_bound_add_epilog = 1;
    }
#endif
}

/* end of ARM code generator */
/*************************************************************/
#endif
/*************************************************************/
