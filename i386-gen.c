/*
 *  X86 code generator for TCC
 * 
 *  Copyright (c) 2001 Fabrice Bellard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* number of available registers */
#define NB_REGS             4

#define NB_REG_CLASSES      2

/* a register can belong to several classes */
#define REG_CLASS_INT    0x0001
#define REG_CLASS_FLOAT  0x0002

/* pretty names for the registers */
enum {
    REG_EAX = 0,
    REG_ECX,
    REG_EDX,
    REG_ST0,
};

int reg_classes[NB_REGS] = {
    REG_CLASS_INT,    /* eax */
    REG_CLASS_INT,    /* ecx */
    REG_CLASS_INT,    /* edx */
    REG_CLASS_FLOAT,  /* st0 */
};

/* integer return register for functions */
#define FUNC_RET_REG        0 
/* float return register for functions */
#define FUNC_RET_FREG       3

/* defined if function parameters must be evaluated in reverse order */
#define INVERT_FUNC_PARAMS

/* defined if structures are passed as pointers. Otherwise structures
   are directly pushed on stack. */
//#define FUNC_STRUCT_PARAM_AS_PTR

/* pointer size, in bytes */
#define PTR_SIZE 4

/* long double size and alignment, in bytes */
#define LDOUBLE_SIZE  12
#define LDOUBLE_ALIGN 4

/* function call context */
typedef struct GFuncContext {
    int args_size;
} GFuncContext;

/******************************************************/

void g(int c)
{
    *(char *)ind++ = c;
}

void o(int c)
{
    while (c) {
        g(c);
        c = c / 256;
    }
}

void gen_le32(int c)
{
    g(c);
    g(c >> 8);
    g(c >> 16);
    g(c >> 24);
}

/* add a new relocation entry to symbol 's' */
void greloc(Sym *s, int addr, int type)
{
    Reloc *p;
    p = malloc(sizeof(Reloc));
    if (!p)
        error("memory full");
    p->type = type;
    p->addr = addr;
    p->next = (Reloc *)s->c;
    s->c = (int)p;
}

/* patch each relocation entry with value 'val' */
void greloc_patch(Sym *s, int val)
{
    Reloc *p, *p1;

    p = (Reloc *)s->c;
    while (p != NULL) {
        p1 = p->next;
        switch(p->type) {
        case RELOC_ADDR32:
            *(int *)p->addr = val;
            break;
        case RELOC_REL32:
            *(int *)p->addr = val - p->addr - 4;
            break;
        }
        free(p);
        p = p1;
    }
    s->c = val;
    s->t &= ~VT_FORWARD;
}

/* output a symbol and patch all calls to it */
void gsym_addr(t, a)
{
    int n;
    while (t) {
        n = *(int *)t; /* next value */
        *(int *)t = a - t - 4;
        t = n;
    }
}

void gsym(t)
{
    gsym_addr(t, ind);
}

/* psym is used to put an instruction with a data field which is a
   reference to a symbol. It is in fact the same as oad ! */
#define psym oad

/* instruction + 4 bytes data. Return the address of the data */
int oad(int c, int s)
{
    o(c);
    *(int *)ind = s;
    s = ind;
    ind = ind + 4;
    return s;
}

/* output constant with relocation if 't & VT_FORWARD' is true */
void gen_addr32(int c, int t)
{
    if (!(t & VT_FORWARD)) {
        gen_le32(c);
    } else {
        greloc((Sym *)c, ind, RELOC_ADDR32);
        gen_le32(0);
    }
}

/* XXX: generate correct pointer for forward references to functions */
/* r = (ft, fc) */
void load(int r, int ft, int fc)
{
    int v, t;

    v = ft & VT_VALMASK;
    if (ft & VT_LVAL) {
        if (v == VT_LLOCAL) {
            load(r, VT_LOCAL | VT_LVAL, fc);
            v = r;
        }
        if ((ft & VT_BTYPE) == VT_FLOAT) {
            o(0xd9); /* flds */
            r = 0;
        } else if ((ft & VT_BTYPE) == VT_DOUBLE) {
            o(0xdd); /* fldl */
            r = 0;
        } else if ((ft & VT_BTYPE) == VT_LDOUBLE) {
            o(0xdb); /* fldt */
            r = 5;
        } else if ((ft & VT_TYPE) == VT_BYTE)
            o(0xbe0f);   /* movsbl */
        else if ((ft & VT_TYPE) == (VT_BYTE | VT_UNSIGNED))
            o(0xb60f);   /* movzbl */
        else if ((ft & VT_TYPE) == VT_SHORT)
            o(0xbf0f);   /* movswl */
        else if ((ft & VT_TYPE) == (VT_SHORT | VT_UNSIGNED))
            o(0xb70f);   /* movzwl */
        else
            o(0x8b);     /* movl */

        if (v == VT_CONST) {
            o(0x05 + r * 8); /* 0xXX, r */
            gen_addr32(fc, ft);
        } else if (v == VT_LOCAL) {
            oad(0x85 + r * 8, fc); /* xx(%ebp), r */
        } else {
            g(0x00 + r * 8 + v); /* (v), r */
        }
    } else {
        if (v == VT_CONST) {
            o(0xb8 + r); /* mov $xx, r */
            gen_addr32(fc, ft);
        } else if (v == VT_LOCAL) {
            o(0x8d);
            oad(0x85 + r * 8, fc); /* lea xxx(%ebp), r */
        } else if (v == VT_CMP) {
            oad(0xb8 + r, 0); /* mov $0, r */
            o(0x0f); /* setxx %br */
            o(fc);
            o(0xc0 + r);
        } else if (v == VT_JMP || v == VT_JMPI) {
            t = v & 1;
            oad(0xb8 + r, t); /* mov $1, r */
            oad(0xe9, 5); /* jmp after */
            gsym(fc);
            oad(0xb8 + r, t ^ 1); /* mov $0, r */
        } else if (v != r) {
            o(0x89);
            o(0xc0 + r + v * 8); /* mov v, r */
        }
    }
}

/* (ft, fc) = r */
/* WARNING: r must not be allocated on the stack */
void store(r, ft, fc)
{
    int fr, bt;

    fr = ft & VT_VALMASK;
    bt = ft & VT_BTYPE;
    /* XXX: incorrect if reg to reg */
    /* XXX: should not flush float stack */
    if (bt == VT_FLOAT) {
        o(0xd9); /* fsts */
        r = 2;
    } else if (bt == VT_DOUBLE) {
        o(0xdd); /* fstpl */
        r = 2;
    } else if (bt == VT_LDOUBLE) {
        o(0xc0d9); /* fld %st(0) */
        o(0xdb); /* fstpt */
        r = 7;
    } else {
        if (bt == VT_SHORT)
            o(0x66);
        if (bt == VT_BYTE)
            o(0x88);
        else
            o(0x89);
    }
    if (fr == VT_CONST) {
        o(0x05 + r * 8); /* mov r,xxx */
        gen_addr32(fc, ft);
    } else if (fr == VT_LOCAL) {
        oad(0x85 + r * 8, fc); /* mov r,xxx(%ebp) */
    } else if (ft & VT_LVAL) {
        g(fr + r * 8); /* mov r, (fr) */
    } else if (fr != r) {
        o(0xc0 + fr + r * 8); /* mov r, fr */
    }
}

/* start function call and return function call context */
void gfunc_start(GFuncContext *c)
{
    c->args_size = 0;
}

/* push function parameter which is in (vtop->t, vtop->c). Stack entry
   is then popped. */
void gfunc_param(GFuncContext *c)
{
    int size, align, r;

    if ((vtop->t & (VT_BTYPE | VT_LVAL)) == (VT_STRUCT | VT_LVAL)) {
        size = type_size(vtop->t, &align);
        /* align to stack align size */
        size = (size + 3) & ~3;
        /* allocate the necessary size on stack */
        oad(0xec81, size); /* sub $xxx, %esp */
        /* generate structure store */
        r = get_reg(REG_CLASS_INT);
        o(0x89); /* mov %esp, r */
        o(0xe0 + r);
        vset(VT_INT | r, 0);
        vswap();
        vstore();
        c->args_size += size;
    } else if (is_float(vtop->t)) {
        gv(); /* only one float register */
        if ((vtop->t & VT_BTYPE) == VT_FLOAT)
            size = 4;
        else if ((vtop->t & VT_BTYPE) == VT_DOUBLE)
            size = 8;
        else
            size = 12;
        oad(0xec81, size); /* sub $xxx, %esp */
        if (size == 12)
            o(0x7cdb);
        else
            o(0x5cd9 + size - 4); /* fstp[s|l] 0(%esp) */
        g(0x24);
        g(0x00);
        c->args_size += size;
    } else {
        /* simple type (currently always same size) */
        /* XXX: implicit cast ? */
        r = gv();
        o(0x50 + r); /* push r */
        c->args_size += 4;
    }
    vtop--;
}

/* generate function call with address in (vtop->t, vtop->c) and free function
   context. Stack entry is popped */
void gfunc_call(GFuncContext *c)
{
    int r;
    if ((vtop->t & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
        /* constant case */
        /* forward reference */
        if (vtop->t & VT_FORWARD) {
            greloc(vtop->c.sym, ind + 1, RELOC_REL32);
            oad(0xe8, 0);
        } else {
            oad(0xe8, vtop->c.ul - ind - 5);
        }
    } else {
        /* otherwise, indirect call */
        r = gv();
        o(0xff); /* call *r */
        o(0xd0 + r);
    }
    if (c->args_size)
        oad(0xc481, c->args_size); /* add $xxx, %esp */
    vtop--;
}

int gjmp(int t)
{
    return psym(0xe9, t);
}

/* generate a test. set 'inv' to invert test. Stack entry is popped */
int gtst(int inv, int t)
{
    int v, *p;
    v = vtop->t & VT_VALMASK;
    if (v == VT_CMP) {
        /* fast case : can jump directly since flags are set */
        g(0x0f);
        t = psym((vtop->c.i - 16) ^ inv, t);
    } else if (v == VT_JMP || v == VT_JMPI) {
        /* && or || optimization */
        if ((v & 1) == inv) {
            /* insert vtop->c jump list in t */
            p = &vtop->c.i;
            while (*p != 0)
                p = (int *)*p;
            *p = t;
            t = vtop->c.i;
        } else {
            t = gjmp(t);
            gsym(vtop->c.i);
        }
    } else if ((vtop->t & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
        /* constant jmp optimization */
        if ((vtop->c.i != 0) != inv) 
            t = gjmp(t);
    } else {
        /* XXX: floats */
        v = gv();
        o(0x85);
        o(0xc0 + v * 9);
        g(0x0f);
        t = psym(0x85 ^ inv, t);
    }
    vtop--;
    return t;
}

/* generate an integer binary operation */
void gen_opi(int op)
{
    int t, r, fr;

    vswap();
    r = gv();
    vswap();
    fr = gv();
    vtop--;

    if (op == '+') {
        o(0x01);
        o(0xc0 + r + fr * 8); 
    } else if (op == '-') {
        o(0x29);
        o(0xc0 + r + fr * 8); 
    } else if (op == '&') {
        o(0x21);
        o(0xc0 + r + fr * 8); 
    } else if (op == '^') {
        o(0x31);
        o(0xc0 + r + fr * 8); 
    } else if (op == '|') {
        o(0x09);
        o(0xc0 + r + fr * 8); 
    } else if (op == '*') {
        o(0xaf0f); /* imul fr, r */
        o(0xc0 + fr + r * 8);
    } else if (op == TOK_SHL | op == TOK_SHR | op == TOK_SAR) {
        /* op2 is %ecx */
        if (fr != 1) {
            if (r == 1) {
                r = fr;
                fr = 1;
                o(0x87); /* xchg r, %ecx */
                o(0xc1 + r * 8);
            } else
                move_reg(1, fr);
        }
        o(0xd3); /* shl/shr/sar %cl, r */
        if (op == TOK_SHL) 
            o(0xe0 + r);
        else if (op == TOK_SHR)
            o(0xe8 + r);
        else
            o(0xf8 + r);
        vtop->t = (vtop->t & VT_TYPE) | r;
    } else if (op == '/' | op == TOK_UDIV | op == TOK_PDIV | 
               op == '%' | op == TOK_UMOD) {
        save_reg(2); /* save edx */
        t = save_reg_forced(fr); /* save fr and get op2 location */
        move_reg(0, r); /* op1 is %eax */
        if (op == TOK_UDIV | op == TOK_UMOD) {
            o(0xf7d231); /* xor %edx, %edx, div t(%ebp), %eax */
            oad(0xb5, t);
        } else {
            o(0xf799); /* cltd, idiv t(%ebp), %eax */
            oad(0xbd, t);
        }
        if (op == '%' | op == TOK_UMOD)
            r = 2;
        else
            r = 0;
        vtop->t = (vtop->t & VT_TYPE) | r;
    } else {
        vtop--;
        o(0x39);
        o(0xc0 + r + fr * 8); /* cmp fr, r */
        vset(VT_CMP, op);
    }
}

/* generate a floating point operation 'v = t1 op t2' instruction. The
   two operands are guaranted to have the same floating point type */
/* NOTE: currently floats can only be lvalues */
void gen_opf(int op)
{
    int a, ft, fc, swapped, r;

    /* convert constants to memory references */
    if ((vtop[-1].t & (VT_CONST | VT_LVAL)) == VT_CONST) {
        vswap();
        gv();
        vswap();
    }
    if ((vtop[0].t & (VT_CONST | VT_LVAL)) == VT_CONST)
        gv();

    /* must put at least one value in the floating point register */
    if ((vtop[-1].t & VT_LVAL) &&
        (vtop[0].t & VT_LVAL)) {
        vswap();
        gv();
        vswap();
    }
    if (op >= TOK_EQ && op <= TOK_GT) {
        /* load on stack second operand */
        load(REG_ST0, vtop->t, vtop->c.ul);
        if (op == TOK_GE || op == TOK_GT)
            o(0xc9d9); /* fxch %st(1) */
        o(0xe9da); /* fucompp */
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
        vtop->t = (vtop->t & VT_TYPE) | VT_CMP;
        vtop->c.i = op;
    } else {
        /* swap the stack if needed so that t1 is the register and t2 is
           the memory reference */
        swapped = 0;
        if (vtop[-1].t & VT_LVAL) {
            vswap();
            swapped = 1;
        }
        
        switch(op) {
        default:
        case '+':
            a = 0;
            break;
        case '-':
            a = 0x20;
            if (swapped)
                a += 8;
            break;
        case '*':
            a = 0x08;
            break;
        case '/':
            a = 0x30;
            if (swapped)
                a += 8;
            break;
        }
        ft = vtop->t;
        fc = vtop->c.ul;
        if ((ft & VT_BTYPE) == VT_DOUBLE)
            o(0xdc);
        else
            o(0xd8);
        
        r = ft & VT_VALMASK;
        if (r == VT_CONST) {
            o(0x05 + a);
            gen_addr32(fc, ft);
        } else if (r == VT_LOCAL) {
            oad(0x85 + a, fc);
        } else {
            g(0x00 + a + r);
        }
        vtop--;
    }
}

/* convert integers to fp 't' type */
void gen_cvt_itof(int t)
{
    gv();
    if ((vtop->t & (VT_BTYPE | VT_UNSIGNED)) == (VT_INT | VT_UNSIGNED)) {
        /* unsigned int to float/double/long double */
        o(0x6a); /* push $0 */
        g(0x00);
        o(0x50 + (vtop->t & VT_VALMASK)); /* push r */
        o(0x242cdf); /* fildll (%esp) */
        o(0x08c483); /* add $8, %esp */
    } else {
        /* int to float/double/long double */
        o(0x50 + (vtop->t & VT_VALMASK)); /* push r */
        o(0x2404db); /* fildl (%esp) */
        o(0x04c483); /* add $4, %esp */
    }
    vtop->t = t | REG_ST0;
}

/* FPU control word for rounding to nearest mode */
/* XXX: should move that into tcc lib support code ! */
static unsigned short __tcc_fpu_control = 0x137f;
/* FPU control word for round to zero mode for int convertion */
static unsigned short __tcc_int_fpu_control = 0x137f | 0x0c00;

/* convert fp to int 't' type */
/* XXX: handle long long case */
void gen_cvt_ftoi(int t)
{
    int r, size;

    gv();
    if (t == VT_INT | VT_UNSIGNED &&
        t == VT_LLONG | VT_UNSIGNED &&
        t == VT_LLONG)
        size = 8;
    else 
        size = 4;

    r = get_reg(REG_CLASS_INT);
    oad(0x2dd9, (int)&__tcc_int_fpu_control); /* ldcw xxx */
    oad(0xec81, size); /* sub $xxx, %esp */
    if (size == 4)
        o(0x1cdb); /* fistpl */
    else
        o(0x3cdb); /* fistpll */
    o(0x24);
    oad(0x2dd9, (int)&__tcc_fpu_control); /* ldcw xxx */
    o(0x58 + r); /* pop r */
    if (size == 8) 
        o(0x04c483); /* add $4, %esp */
    vtop->t = t | r;
}

/* convert from one floating point type to another */
void gen_cvt_ftof(int t)
{
    /* all we have to do on i386 is to put the float in a register */
    gv();
}

/* pop stack value */
void vpop(void)
{
    /* for x86, we need to pop the FP stack */
    if ((vtop->t & VT_VALMASK) == REG_ST0) {
        o(0xd9dd); /* fstp %st(1) */
    }
    vtop--;
}



/* end of X86 code generator */
/*************************************************************/

