#include <stdio.h>

#define TEXT_SIZE       20000
#define DATA_SIZE       2000
#define SYM_TABLE_SIZE  10000
#define VAR_TABLE_SIZE  4096

/* vac: offset of variables 
   vat: type of variables
   loc : local variable index
   glo : global variable index
   parm : parameter variable index
   ind : output code ptr
   lsym: loop symbol stack
   rsym: return symbol
   prog: output code
   astk: arg position stack
*/
int tok, *vac, *vat, *lsym, rsym, 
    prog, ind, loc, glo, file, vt, 
    vc, *macro_stack, *macro_stack_ptr, line_num;
char *idtable, *idptr, *idlast, *filename;

/* The current value can be: */
#define VT_CONST   0x0002  /* constant in vc */
#define VT_VAR     0x0004  /* value is in eax */
#define VT_LOCAL   0x0008  /* offset on stack */

#define VT_LVAL    0x0010  /* const or var is an lvalue */
#define VT_CMP     0x0020  /* the value is stored in processor flags (in vc) */
#define VT_FORWARD 0x0040  /* value is forward reference (only used for functions) */
#define VT_JMP     0x0080  /* value is the consequence of jmp. bit 0 is set if inv */

#define VT_LVALN   -17         /* ~VT_LVAL */

/*
 *
 * VT_FUNC indicates a function. The return type is the stored type. A
 * function pointer is stored as a 'char' pointer.
 *
 * If VT_PTRMASK is non nul, then it indicates the number of pointer
 * iterations to reach the basic type.
 *   
 * Basic types:
 *
 * VT_BYTE indicate a char
 *
 *
 * otherwise integer type is assumed.
 *  */

#define VT_BYTE    0x00001  /* byte pointer. HARDCODED VALUE */
#define VT_PTRMASK 0x00f00  /* pointer mask */
#define VT_PTRINC  0x00100  /* pointer increment */
#define VT_FUNC    0x01000  /* function type */
#define VT_TYPE    0x01f01  /* type mask */

#define VT_TYPEN   0xffffe0fe  /* ~VT_TYPE */
#define VT_FUNCN   -4097

/* Special infos */
#define VT_DEFINE  0x02000  /* special value for #defined symbols */

/* token values */
#define TOK_INT     256
#define TOK_VOID    257
#define TOK_CHAR    258
#define TOK_IF      259
#define TOK_ELSE    260
#define TOK_WHILE   261
#define TOK_BREAK   262
#define TOK_RETURN  263
#define TOK_DEFINE  264
#define TOK_MAIN    265
#define TOK_FOR     266

#define TOK_EQ 0x94 /* warning: depend on asm code */
#define TOK_NE 0x95 /* warning: depend on asm code */
#define TOK_LT 0x9c /* warning: depend on asm code */
#define TOK_GE 0x9d /* warning: depend on asm code */
#define TOK_LE 0x9e /* warning: depend on asm code */
#define TOK_GT 0x9f /* warning: depend on asm code */

#define TOK_LAND 0xa0
#define TOK_LOR  0xa1

#define TOK_DEC  0xa2
#define TOK_MID  0xa3 /* inc/dec, to void constant */
#define TOK_INC  0xa4

#define TOK_SHL  0xe0 /* warning: depend on asm code */
#define TOK_SHR  0xf8 /* warning: depend on asm code */
  
#ifdef TINY
#define expr_eq() expr()
#endif

int inp()
{
#if 0
    int c;
    c = fgetc(file);
    printf("c=%c\n", c);
    return c;
#else
    return fgetc(file);
#endif
}

int isid(c)
{
    return (c >= 'a' & c <= 'z') |
        (c >= 'A' & c <= 'Z') |
        c == '_';
}

int isnum(c)
{
    return c >= '0' & c <= '9';
}

#ifndef TINY
/* XXX: use stderr ? */
void error(char *msg)
{
    printf("%s:%d: %s\n", filename, line_num, msg);
    exit(1);
}

void warning(char *msg)
{
    printf("%s:%d: warning: %s\n", filename, line_num, msg);
}

void skip(c)
{
    if (tok != c) {
        printf("%s:%d: '%c' expected\n", filename, line_num, c);
        exit(1);
    }
    next();
}

void test_lvalue()
{
    if (!(vt & VT_LVAL))
        error("lvalue expected\n");
}

#else

#define skip(c) next()
#define test_lvalue() 

#endif

void next()
{
    int c, v;
    char *q, *p;

    while(1) {
        c = inp();
#ifndef TINY
        if (c == '/') {
            /* comments */
            c = inp();
            if (c == '/') {
                /* single line comments */
                while (c != '\n')
                    c = inp();
            } else if (c == '*') {
                /* comments */
                while ((c = inp()) >= 0) {
                    if (c == '*') {
                        c = inp();
                        if (c == '/') {
                            c = ' ';
                            break;
                        } else if (c == '*')
                            ungetc(c, file);
                    } else if (c == '\n')
                        line_num++;
                }
            } else {
                ungetc(c, file);
                c = '/';
                break;
            }
        } else
#endif
        if (c == 35) {
            /* preprocessor: we handle only define */
            next();
            if (tok == TOK_DEFINE) {
                next();
                /* now tok is the macro symbol */
                vat[tok] = VT_DEFINE;
                vac[tok] = ftell(file);
            }
            /* ignore preprocessor or shell */
            while (c != '\n')
                c = inp();
        }
        if (c == '\n') {
            /* end of line : check if we are in macro state. if so,
               pop new file position */
            if (macro_stack_ptr > macro_stack)
                fseek(file, *--macro_stack_ptr, 0);
            else
                line_num++;
        } else if (c != ' ' & c != 9)
            break;
    }
    if (isid(c)) {
        q = idptr;
        idlast = q;
        while(isid(c) | isnum(c)) {
            *q++ = c;
            c = inp();
        }
        *q++ = '\0';
        ungetc(c, file);
        p = idtable;
        tok = 256;
        while (p < idptr) {
            if (strcmp(p, idptr) == 0)
                break;
            while (*p++);
            tok++;
        }
        /* if not found, add symbol */
        if (p == idptr)
            idptr = q;
        /* eval defines */
        if (vat[tok] & VT_DEFINE) {
            *macro_stack_ptr++ = ftell(file);
            fseek(file, vac[tok], 0);
            next();
        }
    } else {
#ifdef TINY
        q = "<=\236>=\235!=\225++\244--\242==\224";
#else
        q = "<=\236>=\235!=\225&&\240||\241++\244--\242==\224<<\340>>\370";
#endif
        /* two chars */
        v = inp();
        while (*q) {
            if (*q == c & q[1] == v) {
                tok = q[2] & 0xff;
                return;
            }
            q = q + 3;
        }
        ungetc(v, file);
        /* single char substitutions */
        if (c == '<')
            tok = TOK_LT;
        else if (c == '>')
            tok = TOK_GT;
        else
            tok = c;
    }
}

void g(c)
{
    *(char *)ind++ = c;
}

void o(c)
{
    while (c) {
        g(c);
        c = c / 256;
    }
}

/* output a symbol and patch all calls to it */
void gsym(t)
{
    int n;
    while (t) {
        n = *(int *)t; /* next value */
        *(int *)t = ind - t - 4;
        t = n;
    }
}

/* psym is used to put an instruction with a data field which is a
   reference to a symbol. It is in fact the same as oad ! */
#define psym oad

/* instruction + 4 bytes data. Return the address of the data */
int oad(c, s)
{
    o(c);
    *(int *)ind = s;
    s = ind;
    ind = ind + 4;
    return s;
}

void vset(t, v)
{
    vt = t;
    vc = v;
}

/* generate a value in eax from vt and vc */
void gv()
{
#ifndef TINY
    int t;
#endif
    if (vt & VT_LVAL) {
        if ((vt & VT_TYPE) == VT_BYTE)
            o(0xbe0f);   /* movsbl x, %eax */
        else
            o(0x8b);     /* movl x,%eax */
        if (vt & VT_CONST)
            oad(0x05, vc);
        else if (vt & VT_LOCAL)
            oad(0x85, vc);
        else
            g(0x00);
    } else {
        if (vt & VT_CONST) {
            oad(0xb8, vc); /* mov $xx, %eax */
        } else if (vt & VT_LOCAL) {
            oad(0x858d, vc); /* lea xxx(%ebp), %eax */
        } else if (vt & VT_CMP) {
            oad(0xb8, 0); /* mov $0, %eax */
            o(0x0f); /* setxx %al */
            o(vc);
            o(0xc0);
        }
#ifndef TINY
        else if (vt & VT_JMP) {
            t = vt & 1;
            oad(0xb8, t); /* mov $1, %eax */
            oad(0xe9, 5); /* jmp after */
            gsym(vc);
            oad(0xb8, t ^ 1); /* mov $0, %eax */
        }
#endif
    }
    vt = (vt & VT_TYPE) | VT_VAR;
}

/* generate a test. set 'inv' to invert test */
/* XXX: handle constant */
int gtst(inv, t)
{
    if (vt & VT_CMP) {
        /* fast case : can jump directly since flags are set */
        g(0x0f);
        t = psym((vc - 16) ^ inv, t);
    } else 
#ifndef TINY
    if (vt & VT_JMP) {
        /* && or || optimization */
        if ((vt & 1) == inv)
            t = vc;
        else {
            t = psym(0xe9, t);
            gsym(vc);
        }
    } else 
    if ((vt & (VT_CONST | VT_LVAL)) == VT_CONST) {
        /* constant jmp optimization */
        if ((vc != 0) != inv) 
            t = psym(0xe9, t);
    } else
#endif
    {
        gv();
        o(0xc085); /* test %eax, %eax */
        g(0x0f);
        t = psym(0x85 ^ inv, t);
    }
    return t;
}

/* return the size in bytes of a given type */
int type_size(t)
{
    if ((t & VT_PTRMASK) > VT_PTRINC | (t & VT_TYPE) == VT_PTRINC)
        return 4;
    else
        return 1;
}

#define POST_ADD 0x1000
#define PRE_ADD  0

/* a defines POST/PRE add. c is the token ++ or -- */
void inc(a, c)
{
    test_lvalue();
    vt = vt & VT_LVALN;
    gv();
    o(0x018bc189); /* movl %eax, %ecx ; mov (%ecx), %eax */
    o(0x408d | a); /* leal x(%eax), %eax/%edx */
    g((c - TOK_MID) * type_size(vt));
    o(0x0189 | a); /* mov %eax/%edx, (%ecx) */
}

/* XXX: handle ptr sub and 'int + ptr' case (only 'ptr + int' handled) */
/* XXX: handle constant propagation (need to track live eax) */
void gen_op(op, l)
{
    int t;
    gv();
    t = vt;
    o(0x50); /* push %eax */
    next();
    if (l < 0)
        expr();
    else
        sum(l);
    gv();
    o(0x59); /* pop %ecx */
    if (op == '+' | op == '-') {
        /* XXX: incorrect for short (futur!) */
        if (type_size(t) == 4)
            o(0x02e0c1); /* shl $2, %eax */
        if (op == '-') 
            o(0xd8f7); /* neg %eax */
        o(0xc801); /* add %ecx, %eax */
        vt = t;
    } else if (op == '&')
        o(0xc821);
    else if (op == '^')
        o(0xc831);
    else if (op == '|')
        o(0xc809);
    else if (op == '*')
        o(0xc1af0f); /* imul %ecx, %eax */
#ifndef TINY
    else if (op == TOK_SHL | op == TOK_SHR) {
        o(0xd391); /* xchg %ecx, %eax, shl/sar %cl, %eax */
        o(op);
    }
#endif
    else if (op == '/' | op == '%') {
        o(0x91);   /* xchg %ecx, %eax */
        o(0xf9f799); /* cltd, idiv %ecx, %eax */
        if (op == '%')
            o(0x92); /* xchg %edx, %eax */
    } else {
        o(0xc139); /* cmp %eax,%ecx */
        vset(VT_CMP, op);
    }
}

/* return 0 if no type declaration. otherwise, return the basic type
   and skip it. 
   XXX: A '2' is ored to ensure non zero return if int type.
 */
int ist()
{
    int t;

    if (tok == TOK_INT | tok == TOK_CHAR | tok == TOK_VOID) {
        t = tok;
        next();
        return (t != TOK_INT) | 2;
    } else {
        return 0;
    }
}

/* Read a type declaration (except basic type), and return the
   type. If v is true, then also put variable name in 'vc' */
int typ(v,t)
{
    int u, p, n;

    t = t & -3; /* suppress the ored '2' */
    while (tok == '*') {
        next();
        t = t + VT_PTRINC;
    } 
    
    /* recursive type */
    /* XXX: incorrect if abstract type for functions (e.g. 'int ()') */
    if (tok == '(') {
        next();
        u = typ(v, 0);
        skip(')');
    } else {
        u = 0;
        /* type identifier */
        if (v) {
            vc = tok;
            next();
        }
    }
    /* function declaration */
    if (tok == '(') {
        next();
        p = 4; 
        n = vc; /* must save vc there */
        while (tok != ')') {
            /* read param name and compute offset */
            if (t = ist())
                t = typ(1, t); /* XXX: should accept both arg/non arg if v == 0 */
            else {
                vc = tok;
                t = 0;
                next();
            }
            p = p + 4;
            vat[vc] = VT_LOCAL | VT_LVAL | t;
            vac[vc] = p;
            if (tok == ',')
                next();
        }
        next(); /* skip ')' */
        vc = n;
        if (u)
            t = u + VT_BYTE;
        else
            t = t | VT_FUNC;
    }
    return t;
}

/* read a number in base b */
int getn(c, b)
{
    int n, t;
    n = 0;
#ifndef TINY
    while (1) {
        if (c >= 'a')
            t = c - 'a' + 10;
        else if (c >= 'A')
            t = c - 'A' + 10;
        else
            t = c - '0';
        if (t < 0 | t >= b)
            break;
        n = n * b + t;
        c = inp();
    }
#else
    while (isnum(c)) {
        n = n * b + c - '0';
        c = inp();
    }
#endif
    ungetc(c, file);
    return n;
}

int getq(n)
{
    if (n == '\\') {
        n = inp();
        if (n == 'n')
            n = '\n';
#ifndef TINY
        else if (n == 'r')
            n = '\r';
        else if (n == 't')
            n = '\t';
#endif
        else if (isnum(n))
            n = getn(n, 8);
    }
    return n;
}

void unary()
{
    int n, t, ft, fc, p;

    if (isnum(tok)) {
        /* number */
#ifndef TINY
        t = 10;
        if (tok == '0') {
            t = 8;
            tok = inp();
            if (tok == 'x') {
                t = 16;
                tok = inp();
            }
        }
        vset(VT_CONST, getn(tok, t));
#else
        vset(VT_CONST, getn(tok, 10));
#endif
        next();
    } else 
#ifndef TINY
    if (tok == '\'') {
        vset(VT_CONST, getq(inp()));
        next(); /* skip char */
        skip('\''); 
    } else 
#endif
    if (tok == '\"') {
        vset(VT_CONST | VT_PTRINC | VT_BYTE, glo);
        while (tok == '\"') {
            while((n = inp()) != 34) {
                *(char *)glo = getq(n);
                glo++;
            }
            next();
        }
        *(char *)glo = 0;
        glo = (glo + 4) & -4; /* align heap */
    } else {
        t = tok;
        next();
        if (t == '(') {
            /* cast ? */
            if (t = ist()) {
                ft = typ(0, t);
                skip(')');
                unary();
                vt = (vt & VT_TYPEN) | ft;
            } else {
                expr();
                skip(')');
            }
        } else if (t == '*') {
            unary();
            if (vt & VT_LVAL)
                gv();
#ifndef TINY
            if (!(vt & VT_PTRMASK))
                error("pointer expected");
#endif
            vt = (vt - VT_PTRINC) | VT_LVAL;
        } else if (t == '&') {
            unary();
            test_lvalue();
            vt = (vt & VT_LVALN) + VT_PTRINC;
        } else
#ifndef TINY
        if (t == '!') {
            unary();
            if (vt & VT_CMP)
                vc = vc ^ 1;
            else
                vset(VT_JMP, gtst(1, 0));
        } else 
        if (t == '~') {
            unary();
            if ((vt & (VT_CONST | VT_LVAL)) == VT_CONST)
                vc = ~vc;
            else {
                gv();
                o(0xd0f7);
            }
        } else 
        if (t == '+') {
            unary();
        } else 
#endif
        if (t == TOK_INC | t == TOK_DEC) {
            unary();
            inc(PRE_ADD, t);
        } else if (t == '-') {
            unary();
            if ((vt & (VT_CONST | VT_LVAL)) == VT_CONST)
                vc = -vc;
            else {
                gv();
                o(0xd8f7); /* neg %eax */
            }
        } else 
        {
            vset(vat[t], vac[t]);
            /* forward reference or external reference ? */
            if (vt == 0) {
                n = dlsym(0, idlast);
                if (n == 0)
                    vset(VT_CONST | VT_FORWARD | VT_LVAL, vac + t);
                else
                    vset(VT_CONST | VT_LVAL, n);
            }
        }
    }
    
    /* post operations */
    if (tok == TOK_INC | tok == TOK_DEC) {
        inc(POST_ADD, tok);
        next();
    } else 
    if (tok == '[') {
#ifndef TINY
        if (!(vt & VT_PTRMASK))
            error("pointer expected");
#endif
        gen_op('+', -1);
        /* dereference pointer */
        vt = (vt - VT_PTRINC) | VT_LVAL;
        skip(']');
    } else
    if (tok == '(') {
        /* function call  */
        /* lvalue is implied */
        vt = vt & VT_LVALN;
        if ((vt & VT_CONST) == 0) {
            /* evaluate function address */
            gv();
            o(0x50); /* push %eax */
        }
        ft = vt;
        fc = vc;

        next();
        t = 0;
        while (tok != ')') {
            t = t + 4;
            expr_eq();
            gv();
            o(0x50); /* push %eax */
            if (tok == ',')
                next();
        }
        skip(')');
        /* horrible, but needed : convert to native ordering (could
           parse parameters in reverse order, but would cost more
           code) */
        n = 0;
        p = t - 4;
        while (n < p) {
            oad(0x24848b, p); /* mov x(%esp,1), %eax */
            oad(0x248487, n); /* xchg   x(%esp,1), %eax */
            oad(0x248489, p); /* mov %eax, x(%esp,1) */
            n = n + 4;
            p = p - 4;
        }
        if (ft & VT_CONST) {
            /* forward reference */
            if (ft & VT_FORWARD)
                *(int *)fc = psym(0xe8, *(int *)fc); 
            else
                oad(0xe8, fc - ind - 5);
        } else {
            oad(0x2494ff, t); /* call *xxx(%esp) */
            t = t + 4;
        }
        if (t)
            oad(0xc481, t);
        /* return value is variable, int */
        vt = VT_VAR;
    }
}

void uneq()
{
    int ft, fc, b;
    
    unary();
    if (tok == '=') {
        test_lvalue();
        next();
        fc = vc;
        ft = vt;
        b = (vt & VT_TYPE) == VT_BYTE;
        if (ft & VT_VAR)
            o(0x50); /* push %eax */
        expr_eq();
#ifndef TINY
        if ((vt & VT_PTRMASK) != (ft & VT_PTRMASK))
            warning("incompatible type");
#endif
        gv();  /* generate value */
        
        if (ft & VT_VAR) {
            o(0x59); /* pop %ecx */
            o(0x0189 - b); /* mov %eax/%al, (%ecx) */
        } else if (ft & VT_LOCAL)
            oad(0x8589 - b, fc); /* mov %eax/%al,xxx(%ebp) */
        else
            oad(0xa3 - b, fc); /* mov %eax/%al,xxx */
    }
}

void sum(l)
{
#ifndef TINY
    int t;
#endif
    if (l == 0)
        uneq();
    else {
        sum(--l);
        while ((l == 0 & (tok == '*' | tok == '/' | tok == '%')) |
               (l == 1 & (tok == '+' | tok == '-')) |
#ifndef TINY
               (l == 2 & (tok == TOK_SHL | tok == TOK_SHR)) |
#endif
               (l == 3 & (tok >= TOK_LT & tok <= TOK_GT)) |
               (l == 4 & (tok == TOK_EQ | tok == TOK_NE)) |
               (l == 5 & tok == '&') |
               (l == 6 & tok == '^') |
               (l == 7 & tok == '|')) {
            gen_op(tok, l);
       }
    }
}

#ifdef TINY 
void expr()
{
    sum(8);
}
#else
void eand()
{
    int t;

    sum(8);
    t = 0;
    while (1) {
        if (tok != TOK_LAND) {
            if (t) {
                t = gtst(1, t);
                vset(VT_JMP | 1, t);
            }
            break;
        }
        t = gtst(1, t);
        next();
        sum(8);
    }
}

void eor()
{
    int t, u;

    eand();
    t = 0;
    while (1) {
        if (tok != TOK_LOR) {
            if (t) {
                t = gtst(0, t);
                vset(VT_JMP, t);
            }
            break;
        }
        t = gtst(0, t);
        next();
        eand();
    }
}

void expr_eq()
{
    int t, u;
    
    eor();
    if (tok == '?') {
        next();
        t = gtst(1, 0);
        expr();
        gv();
        skip(':');
        u = psym(0xe9, 0);
        gsym(t);
        expr_eq();
        gv();
        gsym(u);
    }
}

void expr()
{
    while (1) {
        expr_eq();
        if (tok != ',')
            break;
        next();
    }
}

#endif

void block()
{
    int a, c, d;

    if (tok == TOK_IF) {
        /* if test */
        next();
        skip('(');
        expr();
        skip(')');
        a = gtst(1, 0);
        block();
        c = tok;
        if (c == TOK_ELSE) {
            next();
            d = psym(0xe9, 0); /* jmp */
            gsym(a);
            block();
            gsym(d); /* patch else jmp */
        } else
            gsym(a);
    } else if (tok == TOK_WHILE) {
        next();
        d = ind;
        skip('(');
        expr();
        skip(')');
        *++lsym = gtst(1, 0);
        block();
        oad(0xe9, d - ind - 5); /* jmp */
        gsym(*lsym--);
    } else if (tok == '{') {
        next();
        /* declarations */
        decl(VT_LOCAL);
        while (tok != '}')
            block();
        next();
    } else if (tok == TOK_RETURN) {
        next();
        if (tok != ';') {
            expr();
            gv();
        }
        skip(';');
        rsym = psym(0xe9, rsym); /* jmp */
    } else if (tok == TOK_BREAK) {
        /* compute jump */
        *lsym = psym(0xe9, *lsym);
        next();
        skip(';');
    } else 
#ifndef TINY
    if (tok == TOK_FOR) {
        int e;
        next();
        skip('(');
        if (tok != ';')
            expr();
        skip(';');
        d = ind;
        c = ind;
        a = 0;
        if (tok != ';') {
            expr();
            a = gtst(1, 0);
        }
        *++lsym = a;
        skip(';');
        if (tok != ')') {
            e = psym(0xe9, 0);
            c = ind;
            expr();
            oad(0xe9, d - ind - 5); /* jmp */
            gsym(e);
        }
        skip(')');
        block();
        oad(0xe9, c - ind - 5); /* jmp */
        gsym(*lsym--);
    } else 
#endif
    {
        if (tok != ';')
            expr();
        skip(';');
    }
}

/* 'l' is true if local declarations */
void decl(l)
{
    int *a, t, b;

    while (b = ist()) {
        while (1) { /* iterate thru each declaration */
            vt = typ(1, b);
            if (tok == '{') {
                /* patch forward references (XXX: does not work for
                   function pointers) */
                if (vat[vc] == 0)
                    gsym(vac[vc]);
                /* put function address */
                vat[vc] = VT_CONST | VT_LVAL | vt;
                vac[vc] = ind;
                loc = 0;
                o(0xe58955); /* push   %ebp, mov    %esp, %ebp */
                a = oad(0xec81, 0); /* sub $xxx, %esp */
                rsym = 0;
                block();
                gsym(rsym);
                o(0xc3c9); /* leave, ret */
                *a = loc; /* save local variables */
                break;
            } else {
                /* variable */
                vat[vc] = l | VT_LVAL | vt;
                if (l == VT_LOCAL) {
                    loc = loc + 4;
                    vac[vc] = -loc;
                } else {
                    vac[vc] = glo;
                    glo = glo + 4;
                }
                if (tok != ',') {
                    skip(';');
                    break;
                }
                next();
            }
        }
    }
}

int main(int c, char **v)
{
    int (*t)();
    if (c < 2) {
        printf("usage: tc src\n");
        return 1;
    }
    v++;
    filename = *v;
    file = fopen(filename, "r");
#ifndef TINY
    if (!file) {
        perror(filename);
        exit(1);
    }
#endif

    idtable = malloc(SYM_TABLE_SIZE);
#ifdef TINY
    memcpy(idtable, 
           "int\0void\0char\0if\0else\0while\0break\0return\0define\0main", 53);
    idptr = idtable + 53;
#else
    memcpy(idtable, 
           "int\0void\0char\0if\0else\0while\0break\0return\0define\0main\0for", 57);
    idptr = idtable + 57;
#endif
    glo = malloc(DATA_SIZE);
    prog = malloc(TEXT_SIZE);
    vac = malloc(VAR_TABLE_SIZE);
    vat = malloc(VAR_TABLE_SIZE);
    lsym = malloc(256);
    macro_stack = malloc(256);
    macro_stack_ptr = macro_stack;
    ind = prog;
    line_num = 1;
    next();
    decl(VT_CONST);
#ifdef TEST
    { 
        FILE *f;
        f = fopen(v[1], "w");
        fwrite((void *)prog, 1, ind - prog, f);
        fclose(f);
        return 0;
    }
#else
    t = vac[TOK_MAIN];
    if (!t)
        error("main() not defined");
    return (*t)(c - 1, v);
#endif
}
