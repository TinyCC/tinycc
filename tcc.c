/*
 *  TCC - Tiny C Compiler
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
#include "tcclib.h"

#define INCLUDE_PATH "/usr/include"

#define TEXT_SIZE           50000
#define DATA_SIZE           50000
#define SYM_TABLE_SIZE      10000
#define MACRO_STACK_SIZE    32
#define INCLUDE_STACK_SIZE  32
#define IFDEF_STACK_SIZE    64
#define VSTACK_SIZE         64

#define NB_REGS             3

/* symbol management */
typedef struct Sym {
    int v;    /* symbol token */
    int t;    /* associated type */
    int c;    /* associated number */
    struct Sym *next; /* next related symbol */
    struct Sym *prev; /* prev symbol in stack */
} Sym;

#define SYM_STRUCT 0x40000000 /* struct/union/enum symbol space */
#define SYM_FIELD  0x20000000 /* struct/union field symbol space */

#define FUNC_NEW       1 /* ansi function prototype */
#define FUNC_OLD       2 /* old function prototype */
#define FUNC_ELLIPSIS  3 /* ansi function prototype with ... */

typedef struct {
    FILE *file;
    char *filename;
    int line_num;
} IncludeFile;

/* loc : local variable index
   glo : global variable index
   ind : output code ptr
   rsym: return symbol
   prog: output code
   anon_sym: anonymous symbol index
*/
FILE *file;
int tok, tok1, tokc, rsym, anon_sym,
    prog, ind, loc, glo, vt, vc, line_num;
char *idtable, *idptr, *filename;
Sym *define_stack, *global_stack, *local_stack, *label_stack;

int vstack[VSTACK_SIZE], *vstack_ptr;
char *macro_stack[MACRO_STACK_SIZE], **macro_stack_ptr, *macro_ptr;
IncludeFile include_stack[INCLUDE_STACK_SIZE], *include_stack_ptr;
int ifdef_stack[IFDEF_STACK_SIZE], *ifdef_stack_ptr;

/* The current value can be: */
#define VT_VALMASK 0x000f
#define VT_CONST   0x000a  /* constant in vc 
                              (must be first non register value) */
#define VT_LLOCAL  0x000b  /* lvalue, offset on stack */
#define VT_LOCAL   0x000c  /* offset on stack */
#define VT_CMP     0x000d  /* the value is stored in processor flags (in vc) */
#define VT_JMP     0x000e  /* value is the consequence of jmp true */
#define VT_JMPI    0x000f  /* value is the consequence of jmp false */
#define VT_LVAL    0x0010  /* var is an lvalue */
#define VT_LVALN   -17         /* ~VT_LVAL */
#define VT_FORWARD 0x0020  /* value is forward reference 
                              (only used for functions) */

#define VT_VOID     0x00040
#define VT_BYTE     0x00080  /* byte type */
#define VT_PTR      0x00100  /* pointer increment */
#define VT_UNSIGNED 0x00200  /* unsigned type */
#define VT_ARRAY    0x00400  /* array type (only used in parsing) */
#define VT_ENUM     0x00800  /* enum definition */
#define VT_FUNC     0x01000  /* function type */
#define VT_STRUCT  0x002000  /* struct/union definition */
#define VT_TYPEDEF 0x004000  /* typedef definition */
#define VT_EXTERN  0x008000   /* extern definition */
#define VT_STATIC  0x010000   /* static variable */
#define VT_STRUCT_SHIFT 17   /* structure/enum name shift (12 bits lefts) */

#define VT_TYPE    0xffffffc0  /* type mask */
#define VT_TYPEN   0x0000003f  /* ~VT_TYPE */
#define VT_FUNCN   -4097       /* ~VT_FUNC */


/* Special infos */

/* token values */

/* warning: the following compare tokens depend on i386 asm code */
#define TOK_ULT 0x92
#define TOK_UGE 0x93
#define TOK_EQ  0x94
#define TOK_NE  0x95
#define TOK_ULE 0x96
#define TOK_UGT 0x97
#define TOK_LT  0x9c
#define TOK_GE  0x9d
#define TOK_LE  0x9e
#define TOK_GT  0x9f

#define TOK_LAND  0xa0
#define TOK_LOR   0xa1

#define TOK_DEC   0xa2
#define TOK_MID   0xa3 /* inc/dec, to void constant */
#define TOK_INC   0xa4
#define TOK_ARROW 0xa7 
#define TOK_DOTS  0xa8 /* three dots */
#define TOK_SHR   0xa9 /* unsigned shift right */
#define TOK_UDIV  0xb0 /* unsigned division */
#define TOK_UMOD  0xb1 /* unsigned modulo */
#define TOK_PDIV  0xb2 /* fast division with undefined rounding for pointers */
#define TOK_NUM   0xb3 /* number in tokc */

#define TOK_SHL   0x01 /* shift left */
#define TOK_SAR   0x02 /* signed shift right */
  
/* assignement operators : normal operator or 0x80 */
#define TOK_A_MOD 0xa5
#define TOK_A_AND 0xa6
#define TOK_A_MUL 0xaa
#define TOK_A_ADD 0xab
#define TOK_A_SUB 0xad
#define TOK_A_DIV 0xaf
#define TOK_A_XOR 0xde
#define TOK_A_OR  0xfc
#define TOK_A_SHL 0x81
#define TOK_A_SAR 0x82

enum {
    TOK_INT = 256,
    TOK_VOID,
    TOK_CHAR,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_BREAK,
    TOK_RETURN,
    TOK_FOR,
    TOK_EXTERN,
    TOK_STATIC,
    TOK_UNSIGNED,
    TOK_GOTO,
    TOK_DO,
    TOK_CONTINUE,
    TOK_SWITCH,
    TOK_CASE,

    /* ignored types Must have contiguous values */
    TOK_CONST,
    TOK_VOLATILE,
    TOK_LONG,
    TOK_REGISTER,
    TOK_SIGNED,
    TOK_AUTO,
    TOK_INLINE,

    /* unsupported type */
    TOK_FLOAT,
    TOK_DOUBLE,

    TOK_STRUCT,
    TOK_UNION,
    TOK_TYPEDEF,
    TOK_DEFAULT,
    TOK_ENUM,
    TOK_SIZEOF,

    /* preprocessor only */
    TOK_DEFINE,
    TOK_INCLUDE,
    TOK_IFDEF,
    TOK_IFNDEF,
    TOK_ELIF,
    TOK_ENDIF,
    TOK_DEFINED,

    /* special identifiers */
    TOK_MAIN,
};

void sum();
void next();
int expr_const();
void expr_eq();
void expr();
void decl();
int gv();
void move_reg();
void save_reg();

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

void printline()
{
    IncludeFile *f;
    for(f = include_stack; f < include_stack_ptr; f++)
        printf("In file included from %s:%d:\n", f->filename, f->line_num);
    printf("%s:%d: ", filename, line_num);
}

/* XXX: use stderr ? */
void error(char *msg)
{
    printline();
    printf("%s\n", msg);
    exit(1);
}

void expect(char *msg)
{
    printline();
    printf("%s expected\n", msg);
    exit(1);
}

void warning(char *msg)
{
    printline();
    printf("warning: %s\n", msg);
}

void skip(c)
{
    if (tok != c) {
        printline();
        printf("'%c' expected\n", c);
        exit(1);
    }
    next();
}

void test_lvalue()
{
    if (!(vt & VT_LVAL))
        expect("lvalue");
}

#else

#define skip(c) next()
#define test_lvalue() 

#endif

char *get_tok_str(int v)
{
    int t;
    char *p;
    p = idtable;
    t = 256;
    while (t != v) {
        if (p >= idptr)
            return 0;
        while (*p++);
        t++;
    }
    return p;
}

/* find a symbol and return its associated structure. 's' is the top
   of the symbol stack */
Sym *sym_find1(Sym *s, int v)
{
    while (s) {
        if (s->v == v)
            return s;
        s = s->prev;
    }
    return 0;
}

Sym *sym_push1(Sym **ps, int v, int t, int c)
{
    Sym *s;
    s = malloc(sizeof(Sym));
    if (!s)
        error("memory full");
    s->v = v;
    s->t = t;
    s->c = c;
    s->next = 0;
    s->prev = *ps;
    *ps = s;
    return s;
}

/* find a symbol in the right symbol space */
Sym *sym_find(int v)
{
    Sym *s;
    s = sym_find1(local_stack, v);
    if (!s)
        s = sym_find1(global_stack, v);
    return s;
}

/* push a given symbol on the symbol stack */
Sym *sym_push(int v, int t, int c)
{
    //    printf("sym_push: %x %s type=%x\n", v, get_tok_str(v), t);
    if (local_stack)
        return sym_push1(&local_stack, v, t, c);
    else
        return sym_push1(&global_stack, v, t, c);
}

/* pop symbols until top reaches 'b' */
void sym_pop(Sym **ps, Sym *b)
{
    Sym *s, *ss;

    s = *ps;
    while(s != b) {
        ss = s->prev;
        //        printf("sym_pop: %x %s type=%x\n", s->v, get_tok_str(s->v), s->t);
        free(s);
        s = ss;
    }
    *ps = b;
}

int ch, ch1;

/* read next char from current input file */
void inp()
{
    int c;

 redo:
    ch1 = fgetc(file);
    if (ch1 == -1) {
        if (include_stack_ptr == include_stack)
            return;
        /* pop include stack */
        fclose(file);
        free(filename);
        include_stack_ptr--;
        file = include_stack_ptr->file;
        filename = include_stack_ptr->filename;
        line_num = include_stack_ptr->line_num;
        goto redo;
    }
    if (ch1 == '\n')
        line_num++;
    //    printf("ch1=%c 0x%x\n", ch1, ch1);
}

/* input with '\\n' handling and macro subtitution if in macro state */
void minp()
{
    if (macro_ptr != 0) {
        ch = *macro_ptr++;
        /* end of macro ? */
        if (ch == '\0') {
            macro_ptr = *--macro_stack_ptr;
            ch = (int)*--macro_stack_ptr;
        }
    } else {
    redo:
        ch = ch1;
        inp();
        if (ch == '\\' && ch1 == '\n') {
            inp();
            goto redo;
        }
    }
    //    printf("ch=%c 0x%x\n", ch, ch);
}

/* same as minp, but also skip comments */
/* XXX: skip strings & chars */
void cinp()
{
    int c;

    if (ch1 == '/') {
        inp();
        if (ch1 == '/') {
            /* single line C++ comments */
            inp();
            while (ch1 != '\n' && ch1 != -1)
                inp();
            inp();
            ch = ' '; /* return space */
        } else if (ch1 == '*') {
            /* C comments */
            inp();
            while (ch1 != -1) {
                c = ch1;
                inp();
                if (c == '*' && ch1 == '/') {
                    inp();
                    ch = ' '; /* return space */
                    break;
                }
            }
        } else {
            ch = '/';
        }
    } else {
        minp();
    }
}

void skip_spaces()
{
    while (ch == ' ' || ch == '\t')
        cinp();
}

/* skip block of text until #else, #elif or #endif. skip also pairs of
   #if/#endif */
void preprocess_skip()
{
    int a;
    a = 0;
    while (1) {
        while (ch != '\n') {
            if (ch == -1)
                expect("#endif");
            cinp();
        }
        cinp();
        skip_spaces();
        if (ch == '#') {
            cinp();
            next();
            if (a == 0 && 
                (tok == TOK_ELSE || tok == TOK_ELIF || tok == TOK_ENDIF))
                break;
            if (tok == TOK_IF || tok == TOK_IFDEF || tok == TOK_IFNDEF)
                a++;
            else if (tok == TOK_ENDIF)
                a--;
        }
    }
}

/* parse until eol and add given char */
char *get_str(c)
{
    char *str;
    int size, n;

    str = NULL;
    size = 0;
    n = 0;
    while (1) {
        if ((n + 1) >= size) {
            size += 128;
            str = realloc(str, size);
            if (!str)
                error("memory full");
        }
        if (ch == -1 || ch == '\n') {
            str[n++] = c; 
            str[n++] = '\0';
            break;
        }
        str[n++] = ch;
        cinp();
    }
    return str;
}

/* return next token without macro substitution */
void next_nomacro()
{
    Sym *s;
    /* hack to avoid replacing id by defined value */
    s = define_stack;
    define_stack = 0;
    next();
    define_stack = s;
}

/* XXX: not correct yet (need to ensure it is constant) */
int expr_preprocess()
{
    int c;

    if ((macro_stack_ptr - macro_stack) >= MACRO_STACK_SIZE)
        error("too many nested macros");
    *macro_stack_ptr++ = (char *)'\n';
    *macro_stack_ptr++ = macro_ptr;
    macro_ptr = get_str(';');
    cinp();
    next();
    c = expr_const();
    ch = '\n';
    return c != 0;
}

void preprocess()
{
    char *str;
    int size, n, c, v;
    char buf[1024], *q, *p;
    char buf1[1024];
    FILE *f;

    cinp();
    next_nomacro();
 redo:
    if (tok == TOK_DEFINE) {
        next_nomacro(); /* XXX: should pass parameter to avoid macro subst */
        skip_spaces();
        /* now 'tok' is the macro symbol */
        /* a space is inserted after each macro */
        str = get_str(' ');
        //        printf("define %s '%s'\n", get_tok_str(tok), str);
        sym_push1(&define_stack, tok, 0, (int)str);
    } else if (tok == TOK_INCLUDE) {
        skip_spaces();
        if (ch == '<') {
            c = '>';
            goto read_name;
        } else if (ch == '\"') {
            c = ch;
        read_name:
            minp();
            q = buf;
            while (ch != c && ch != '\n' && ch != -1) {
                if ((q - buf) < sizeof(buf) - 1)
                    *q++ = ch;
                minp();
            }
            *q = '\0';
            if (include_stack_ptr >= include_stack + INCLUDE_STACK_SIZE)
                error("memory full");
            if (c == '\"') {
                /* first search in current dir if "header.h" */
                /* XXX: buffer overflow */
                size = 0;
                p = strrchr(filename, '/');
                if (p) 
                    size = p + 1 - filename;
                memcpy(buf1, filename, size);
                buf1[size] = '\0';
                strcat(buf1, buf);
                f = fopen(buf1, "r");
                if (f)
                    goto found;
            }
            /* now search in standard include path */
            strcpy(buf1, INCLUDE_PATH);
            strcat(buf1, "/");
            strcat(buf1, buf);
            f = fopen(buf1, "r");
            if (!f)
                error("include file not found");
        found:
            /* push current file in stack */
            /* XXX: fix current line init */
            include_stack_ptr->file = file;
            include_stack_ptr->filename = filename;
            include_stack_ptr->line_num = line_num;
            include_stack_ptr++;
            file = f;
            filename = strdup(buf1);
            line_num = 1;
        }
    } else if (tok == TOK_IFNDEF) {
        c = 1;
        goto do_ifdef;
    } else if (tok == TOK_IF) {
        c = expr_preprocess();
        goto do_if;
    } else if (tok == TOK_IFDEF) {
        c = 0;
    do_ifdef:
        next_nomacro();
        c = (sym_find1(define_stack, tok) != 0) ^ c;
    do_if:
        if (ifdef_stack_ptr >= ifdef_stack + IFDEF_STACK_SIZE)
            error("memory full");
        *ifdef_stack_ptr++ = c;
        goto test_skip;
    } else if (tok == TOK_ELSE) {
        if (ifdef_stack_ptr == ifdef_stack ||
            (ifdef_stack_ptr[-1] & 2))
            error("#else after #else");
        c = (ifdef_stack_ptr[-1] ^= 3);
        goto test_skip;
    } else if (tok == TOK_ELIF) {
        if (ifdef_stack_ptr == ifdef_stack ||
            ifdef_stack_ptr[-1] > 1)
            error("#elif after #else");
        c = expr_preprocess();
        ifdef_stack_ptr[-1] = c;
    test_skip:
        if (!(c & 1)) {
            preprocess_skip();
            goto redo;
        }
    } else if (tok == TOK_ENDIF) {
        if (ifdef_stack_ptr == ifdef_stack)
            expect("#if");
        ifdef_stack_ptr--;
    }
    /* ignore other preprocess commands or #! for C scripts */
    while (ch != '\n' && ch != -1)
        cinp();
}

/* read a number in base b */
int getn(b)
{
    int n, t;
    n = 0;
    while (1) {
        if (ch >= 'a' & ch <= 'f')
            t = ch - 'a' + 10;
        else if (ch >= 'A' & ch <= 'F')
            t = ch - 'A' + 10;
        else if (isnum(ch))
            t = ch - '0';
        else
            break;
        if (t < 0 | t >= b)
            break;
        n = n * b + t;
        cinp();
    }
    return n;
}

void next()
{
    int v, b;
    char *q, *p;
    Sym *s;

    /* special 'ungettok' case for label parsing */
    if (tok1) {
        tok = tok1;
        tok1 = 0;
        return;
    }
 redo:
    /* skip spaces */
    while(1) {
        while (ch == '\n') {
            cinp();
            while (ch == ' ' || ch == 9)
                cinp();
            if (ch == '#') {
                /* preprocessor command if # at start of line after
                   spaces */
                preprocess();
            }
        }
        if (ch != ' ' && ch != 9)
            break;
        cinp();
    }
    if (isid(ch)) {
        q = idptr;
        while(isid(ch) | isnum(ch)) {
            *q++ = ch;
            cinp();
        }
        *q++ = '\0';
        p = idtable;
        tok = 256;
        while (p < idptr) {
            if (strcmp(p, idptr) == 0)
                break;
            while (*p++);
            tok++;
        }
        //        printf("id=%s\n", idptr);
        /* if not found, add symbol */
        if (p == idptr)
            idptr = q;
        /* if symbol is a define, prepare substitution */
        if (s = sym_find1(define_stack, tok)) {
            if ((macro_stack_ptr - macro_stack) >= MACRO_STACK_SIZE)
                error("too many nested macros");
            *macro_stack_ptr++ = (char *)ch;
            *macro_stack_ptr++ = macro_ptr;
            macro_ptr = (char *)s->c;
            cinp();
            goto redo;
        }
    } else if (isnum(ch)) {
        /* number */
        b = 10;
        if (ch == '0') {
            cinp();
            b = 8;
            if (ch == 'x') {
                cinp();
                b = 16;
            }
        }
        tokc = getn(b);
        tok = TOK_NUM;
    } else {
#ifdef TINY
        q = "<=\236>=\235!=\225++\244--\242==\224";
#else
        q = "<=\236>=\235!=\225&&\240||\241++\244--\242==\224<<\1>>\2+=\253-=\255*=\252/=\257%=\245&=\246^=\336|=\374->\247..\250";
#endif
        /* two chars */
        tok = ch;
        cinp();
        while (*q) {
            if (*q == tok & q[1] == ch) {
                cinp();
                tok = q[2] & 0xff;
                /* three chars tests */
                if (tok == TOK_SHL | tok == TOK_SAR) {
                    if (ch == '=') {
                        tok = tok | 0x80;
                        cinp();
                    }
                } else if (tok == TOK_DOTS) {
                    if (ch != '.')
                        error("parse error");
                    cinp();
                }
                return;
            }
            q = q + 3;
        }
        /* single char substitutions */
        if (tok == '<')
            tok = TOK_LT;
        else if (tok == '>')
            tok = TOK_GT;
    }
    //    printf("tok=%x\n", tok);
}

void swap(int *p, int *q)
{
    int t;
    t = *p;
    *p = *q;
    *q = t;
}

void vset(t, v)
{
    vt = t;
    vc = v;
}

/******************************************************/
/* X86 code generator */

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
int oad(c, s)
{
    o(c);
    *(int *)ind = s;
    s = ind;
    ind = ind + 4;
    return s;
}

/* XXX: generate correct pointer for forward references to functions */
/* r = (ft, fc) */
void load(r, ft, fc)
{
    int v, t;

    v = ft & VT_VALMASK;
    if (ft & VT_LVAL) {
        if (v == VT_LLOCAL) {
            load(r, VT_LOCAL | VT_LVAL, fc);
            v = r;
        }
        if ((ft & VT_TYPE) == VT_BYTE)
            o(0xbe0f);   /* movsbl */
        else
            o(0x8b);     /* movl */
        if (v == VT_CONST) {
            oad(0x05 + r * 8, fc); /* 0xXX, r */
        } else if (v == VT_LOCAL) {
            oad(0x85 + r * 8, fc); /* xx(%ebp), r */
        } else {
            g(0x00 + r * 8 + v); /* (v), r */
        }
    } else {
        if (v == VT_CONST) {
            oad(0xb8 + r, fc); /* mov $xx, r */
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
    int fr, b;

    fr = ft & VT_VALMASK;
    b = (ft & VT_TYPE) == VT_BYTE;
    o(0x89 - b);
    if (fr == VT_CONST) {
        oad(0x05 + r * 8, fc); /* mov r,xxx */
    } else if (fr == VT_LOCAL) {
        oad(0x85 + r * 8, fc); /* mov r,xxx(%ebp) */
    } else if (ft & VT_LVAL) {
        g(fr + r * 8); /* mov r, (fr) */
    } else if (fr != r) {
        o(0xc0 + fr + r * 8); /* mov r, fr */
    }
}

int gjmp(t)
{
    return psym(0xe9, t);
}

/* generate a test. set 'inv' to invert test */
int gtst(inv, t)
{
    int v, *p;
    v = vt & VT_VALMASK;
    if (v == VT_CMP) {
        /* fast case : can jump directly since flags are set */
        g(0x0f);
        t = psym((vc - 16) ^ inv, t);
    } else if (v == VT_JMP || v == VT_JMPI) {
        /* && or || optimization */
        if ((v & 1) == inv) {
            /* insert vc jump list in t */
            p = &vc;
            while (*p != 0)
                p = (int *)*p;
            *p = t;
            t = vc;
        } else {
            t = gjmp(t);
            gsym(vc);
        }
    } else if ((vt & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
        /* constant jmp optimization */
        if ((vc != 0) != inv) 
            t = gjmp(t);
    } else {
        v = gv();
        o(0x85);
        o(0xc0 + v * 9);
        g(0x0f);
        t = psym(0x85 ^ inv, t);
    }
    return t;
}

/* generate a binary operation 'v = r op fr' instruction and modifies
   (vt,vc) if needed */
void gen_op1(op, r, fr)
{
    int t;
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
        vt = (vt & VT_TYPE) | r;
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
        vt = (vt & VT_TYPE) | r;
    } else {
        o(0x39);
        o(0xc0 + r + fr * 8); /* cmp fr, r */
        vset(VT_CMP, op);
    }
}

/* end of X86 code generator */
/*************************************************************/

int save_reg_forced(r)
{
    int i, l, *p, t;
    /* store register */
    loc = (loc - 4) & -3;
    store(r, VT_LOCAL, loc);
    l = loc;

    /* modify all stack values */
    for(p=vstack;p<vstack_ptr;p+=2) {
        i = p[0] & VT_VALMASK;
        if (i == r) {
            if (p[0] & VT_LVAL)
                t = VT_LLOCAL;
            else
                t = VT_LOCAL;
            p[0] = (p[0] & VT_TYPE) | VT_LVAL | t;
            p[1] = l;
        }
    }
    return l;
}

/* save r to memory. and mark it as being free */
void save_reg(r)
{
    int i, *p;

    /* modify all stack values */
    for(p=vstack;p<vstack_ptr;p+=2) {
        i = p[0] & VT_VALMASK;
        if (i == r) {
            save_reg_forced(r);
            break;
        }
    }
}

/* find a free register. If none, save one register */
int get_reg()
{
    int r, i, *p;

    /* find a free register */
    for(r=0;r<NB_REGS;r++) {
        for(p=vstack;p<vstack_ptr;p+=2) {
            i = p[0] & VT_VALMASK;
            if (i == r)
                goto notfound;
        }
        return r;
    notfound: ;
    }
    
    /* no register left : free the first one on the stack (very
       important to start from the bottom to ensure that we don't
       spill registers used in gen_op()) */
    for(p=vstack;p<vstack_ptr;p+=2) {
        r = p[0] & VT_VALMASK;
        if (r < VT_CONST) {
            save_reg(r);
            break;
        }
    }
    return r;
}

void save_regs()
{
    int r, *p;
    for(p=vstack;p<vstack_ptr;p+=2) {
        r = p[0] & VT_VALMASK;
        if (r < VT_CONST) {
            save_reg(r);
        }
    }
}

/* move register 's' to 'r', and flush previous value of r to memory
   if needed */
void move_reg(r, s)
{
    if (r != s) {
        save_reg(r);
        load(r, s, 0);
    }
}

/* convert a stack entry in register */
int gvp(int *p)
{
    int r;
    r = p[0] & VT_VALMASK;
    if (r >= VT_CONST || (p[0] & VT_LVAL))
        r = get_reg();
    load(r, p[0], p[1]);
    p[0] = (p[0] & VT_TYPE) | r;
    return r;
}

void vpush()
{
    if (vstack_ptr >= vstack + VSTACK_SIZE)
        error("memory full");
    *vstack_ptr++ = vt;
    *vstack_ptr++ = vc;
    /* cannot let cpu flags if other instruction are generated */
    if ((vt & VT_VALMASK) == VT_CMP)
        gvp(vstack_ptr - 2);
}

void vpop(int *ft, int *fc)
{
    *fc = *--vstack_ptr;
    *ft = *--vstack_ptr;
}

/* generate a value in a register from vt and vc */
int gv()
{
    int r;
    vpush();
    r = gvp(vstack_ptr - 2);
    vpop(&vt, &vc);
    return r;
}

/* handle constant optimizations and various machine independant opt */
void gen_opc(op)
{
    int fr, ft, fc, r, c1, c2, n;

    vpop(&ft, &fc);
    vpop(&vt, &vc);
    c1 = (vt & (VT_VALMASK | VT_LVAL)) == VT_CONST;
    c2 = (ft & (VT_VALMASK | VT_LVAL)) == VT_CONST;
    if (c1 && c2) {
        switch(op) {
        case '+': vc += fc; break;
        case '-': vc -= fc; break;
        case '&': vc &= fc; break;
        case '^': vc ^= fc; break;
        case '|': vc |= fc; break;
        case '*': vc *= fc; break;
        case TOK_PDIV:
        case '/': vc /= fc; break; /* XXX: zero case ? */
        case '%': vc %= fc; break; /* XXX: zero case ? */
        case TOK_UDIV: vc = (unsigned)vc / fc; break; /* XXX: zero case ? */
        case TOK_UMOD: vc = (unsigned)vc % fc; break; /* XXX: zero case ? */
        case TOK_SHL: vc <<= fc; break;
        case TOK_SHR: vc = (unsigned)vc >> fc; break;
        case TOK_SAR: vc >>= fc; break;
        default:
            goto general_case;
        }
    } else {
        /* if commutative ops, put c2 as constant */
        if (c1 && (op == '+' || op == '&' || op == '^' || 
                   op == '|' || op == '*')) {
            swap(&vt, &ft);
            swap(&vc, &fc);
            swap(&c1, &c2);
        }
        if (c2 && (((op == '*' || op == '/' || op == TOK_UDIV || 
                     op == TOK_PDIV) && 
                    fc == 1) ||
                   ((op == '+' || op == '-' || op == '|' || op == '^' || 
                     op == TOK_SHL || op == TOK_SHR || op == TOK_SAR) && 
                    fc == 0) ||
                   (op == '&' && 
                    fc == -1))) {
        } else if (c2 && (op == '*' || op == TOK_PDIV || op == TOK_UDIV)) {
            /* try to use shifts instead of muls or divs */
            if (fc > 0 && (fc & (fc - 1)) == 0) {
                n = -1;
                while (fc) {
                    fc >>= 1;
                    n++;
                }
                fc = n;
                if (op == '*')
                    op = TOK_SHL;
                else if (op == TOK_PDIV)
                    op = TOK_SAR;
                else
                    op = TOK_SHR;
            }
            goto general_case;
        } else {
        general_case:
            vpush();
            vt = ft;
            vc = fc;
            vpush();
            r = gvp(vstack_ptr - 4);
            fr = gvp(vstack_ptr - 2);
            vpop(&ft, &fc);
            vpop(&vt, &vc);
            /* call low level op generator */
            gen_op1(op, r, fr);
        }
    }
}

int pointed_size(t)
{
    return type_size(pointed_type(t), &t);
}

/* generic gen_op: handles types problems */
void gen_op(op)
{
    int u, t1, t2;

    vpush();
    t1 = vstack_ptr[-4];
    t2 = vstack_ptr[-2];
    if (op == '+' | op == '-') {
        if ((t1 & VT_PTR) && (t2 & VT_PTR)) {
            if (op != '-')
                error("invalid type");
            /* XXX: check that types are compatible */
            u = pointed_size(t1);
            gen_opc(op);
            vpush();
            vstack_ptr[-2] &= ~VT_TYPE; /* set to integer */
            vset(VT_CONST, u);
            gen_op(TOK_PDIV);
        } else if ((t1 | t2) & VT_PTR) {
            if (t2 & VT_PTR) {
                swap(vstack_ptr - 4, vstack_ptr - 2);
                swap(vstack_ptr - 3, vstack_ptr - 1);
                swap(&t1, &t2);
            }
            /* stack-4 contains pointer, stack-2 value to add */
            vset(VT_CONST, pointed_size(vstack_ptr[-4]));
            gen_op('*');
            vpush();
            gen_opc(op);
            /* put again type if gen_opc() swaped operands */
            vt = (vt & VT_TYPEN) | (t1 & VT_TYPE);
        } else {
            gen_opc(op);
        }
    } else {
        /* XXX: test types and compute returned value */
        if ((t1 | t2) & (VT_UNSIGNED | VT_PTR)) {
            if (op == TOK_SAR)
                op = TOK_SHR;
            else if (op == '/')
                op = TOK_UDIV;
            else if (op == '%')
                op = TOK_UMOD;
            else if (op == TOK_LT)
                op = TOK_ULT;
            else if (op == TOK_GT)
                op = TOK_UGT;
            else if (op == TOK_LE)
                op = TOK_ULE;
            else if (op == TOK_GE)
                op = TOK_UGE;
        }
        gen_opc(op);
    }
}

/* return type size. Put alignment at 'a' */
int type_size(int t, int *a)
{
    Sym *s;

    /* int, enum or pointer */
    if (t & VT_STRUCT) {
        /* struct/union */
        s = sym_find(((unsigned)t >> VT_STRUCT_SHIFT) | SYM_STRUCT);
        *a = 4; /* XXX: cannot store it yet. Doing that is safe */
        return s->c;
    } else if (t & VT_ARRAY) {
        s = sym_find(((unsigned)t >> VT_STRUCT_SHIFT));
        return type_size(s->t, a) * s->c;
    } else if ((t & VT_PTR) | 
               (t & VT_TYPE) == 0 |
               (t & VT_ENUM)) {
        *a = 4;
        return 4;
    } else {
        *a = 1;
        return 1;
    }
}

/* return the pointed type of t */
int pointed_type(int t)
{
    Sym *s;
    s = sym_find(((unsigned)t >> VT_STRUCT_SHIFT));
    return s->t | (t & VT_TYPEN);
}

int mk_pointer(int t)
{
    int p;
    p = anon_sym++;
    sym_push(p, t, -1);
    return VT_PTR | (p << VT_STRUCT_SHIFT) | (t & VT_TYPEN);
}

/* store value in lvalue pushed on stack */
void vstore()
{
    int ft, fc, r, t;

    r = gv();  /* generate value */
    vpush();
    ft = vstack_ptr[-4];
    fc = vstack_ptr[-3];
    if ((ft & VT_VALMASK) == VT_LLOCAL) {
        t = get_reg();
        load(t, VT_LOCAL | VT_LVAL, fc);
        ft = (ft & ~VT_VALMASK) | t;
    }
    store(r, ft, fc);
    vstack_ptr -= 4;
}

/* post defines POST/PRE add. c is the token ++ or -- */
void inc(post, c)
{
    int r, r1;
    test_lvalue();
    if (post)
        vpush(); /* room for returned value */
    vpush(); /* save lvalue */
    r = gv();
    vpush(); /* save value */
    if (post) {
        /* duplicate value */
        r1 = get_reg();
        load(r1, r, 0); /* move r to r1 */
        vstack_ptr[-6] = (vt & VT_TYPE) | r1;
        vstack_ptr[-5] = 0;
    }
    /* add constant */
    vset(VT_CONST, c - TOK_MID); 
    gen_op('+');
    vstore(); /* store value */
    if (post)
        vpop(&vt, &vc);
}

int expr_const()
{
    expr_eq();
    if ((vt & (VT_CONST | VT_LVAL)) != VT_CONST)
        expect("constant");
    return vc;
}

/* enum/struct/union declaration */
int struct_decl(u)
{
    int a, t, b, v, size, align, maxalign, c;
    Sym *slast, *s, *ss;

    a = tok; /* save decl type */
    next();
    if (tok != '{') {
        v = tok;
        next();
        /* struct already defined ? return it */
        /* XXX: check consistency */
        if (s = sym_find(v | SYM_STRUCT)) {
            if (s->t != a)
                error("invalid type");
            u = u | (v << VT_STRUCT_SHIFT);
            return u;
        }
    } else {
        v = anon_sym++;
    }
    s = sym_push(v | SYM_STRUCT, a, 0);
    /* put struct/union/enum name in type */
    u = u | (v << VT_STRUCT_SHIFT);
    
    if (tok == '{') {
        next();
        /* cannot be empty */
        c = 0;
        maxalign = 0;
        slast = 0;
        while (1) {
            if (a == TOK_ENUM) {
                v = tok;
                next();
                if (tok == '=') {
                    next();
                    c = expr_const();
                }
                sym_push(v, VT_CONST, c);
                if (tok == ',')
                    next();
                c++;
            } else {
                b = ist();
                while (1) {
                    t = typ(&v, b);
                    if (t & (VT_FUNC | VT_TYPEDEF))
                        error("invalid type");
                    /* XXX: align & correct type size */
                    v |= SYM_FIELD;
                    size = type_size(t, &align);
                    if (a == TOK_STRUCT) {
                        c = (c + align - 1) & -align;
                        ss = sym_push(v, t, c);
                        c += size;
                    } else {
                        ss = sym_push(v, t, 0);
                        if (size > c)
                            c = size;
                    }
                    if (align > maxalign)
                        maxalign = align;
                    ss->next = slast;
                    slast = ss;
                    if (tok == ';' || tok == -1)
                        break;
                    skip(',');
                }
                skip(';');
            }
            if (tok == '}')
                break;
        }
        skip('}');
        s->next = slast;
        /* size for struct/union, dummy for enum */
        s->c = (c + maxalign - 1) & -maxalign; 
    }
    return u;
}

/* return 0 if no type declaration. otherwise, return the basic type
   and skip it. 
   XXX: A '2' is ored to ensure non zero return if int type.
 */
int ist()
{
    int t, n, v;
    Sym *s;

    t = 0;
    while(1) {
#ifndef TINY
        if (tok == TOK_ENUM) {
            t |= struct_decl(VT_ENUM);
        } else if (tok == TOK_STRUCT || tok == TOK_UNION) {
            t |= struct_decl(VT_STRUCT);
        } else
#endif
        {
            if (tok == TOK_CHAR) {
                t |= VT_BYTE;
            } else if (tok == TOK_VOID) {
                t |= VT_VOID;
            } else if (tok == TOK_INT |
                       (tok >= TOK_CONST & tok <= TOK_INLINE)) {
                /* ignored types */
            } else if (tok == TOK_FLOAT & tok == TOK_DOUBLE) {
                error("floats not supported");
            } else if (tok == TOK_EXTERN) {
                t |= VT_EXTERN;
            } else if (tok == TOK_STATIC) {
                t |= VT_STATIC;
            } else if (tok == TOK_UNSIGNED) {
                t |= VT_UNSIGNED;
            } else if (tok == TOK_TYPEDEF) {
                t |= VT_TYPEDEF;
            } else {
                s = sym_find(tok);
                if (!s || !(s->t & VT_TYPEDEF))
                    break;
                t = s->t & ~VT_TYPEDEF;
            }
            next();
        }
        t |= 2;
    }
    return t;
}

int post_type(t)
{
    int p, n, pt, l, a;
    Sym *last, *s;

    if (tok == '(') {
        /* function declaration */
        next();
        a = 4; 
        l = 0;
        last = NULL;
        while (tok != ')') {
            /* read param name and compute offset */
            if (l != FUNC_OLD) {
                if (!(pt = ist())) {
                    if (l) {
                        error("invalid type");
                    } else {
                        l = FUNC_OLD;
                        goto old_proto;
                    }
                }
                if (pt & VT_VOID && tok == ')')
                    break;
                l = FUNC_NEW;
                pt = typ(&n, pt); /* XXX: should accept 
                                     both arg/non arg if v == 0 */
            } else {
            old_proto:
                n = tok;
                pt = 0; /* int type */
                next();
            }
            /* array must be transformed to pointer according to ANSI C */
            pt &= ~VT_ARRAY;
            /* XXX: size will be different someday */
            a = a + 4;
            s = sym_push(n | SYM_FIELD, VT_LOCAL | VT_LVAL | pt, a);
            s->next = last;
            last = s;
            if (tok == ',') {
                next();
                if (l == FUNC_NEW && tok == TOK_DOTS) {
                    l = FUNC_ELLIPSIS;
                    next();
                    break;
                }
            }
        }
        skip(')');
        t = post_type(t);
        /* we push a anonymous symbol which will contain the function prototype */
        p = anon_sym++;
        s = sym_push(p, t, l);
        s->next = last;
        t = VT_FUNC | (p << VT_STRUCT_SHIFT);
    } else if (tok == '[') {
        /* array definition */
        next();
        n = -1;
        if (tok != ']') {
            n = expr_const();
            if (n < 0)
                error("invalid array size");    
        }
        skip(']');
        /* parse next post type */
        t = post_type(t);
        
        /* we push a anonymous symbol which will contain the array
           element type */
        p = anon_sym++;
        sym_push(p, t, n);
        t = VT_ARRAY | VT_PTR | (p << VT_STRUCT_SHIFT);
    }
    return t;
}

/* Read a type declaration (except basic type), and return the
   type. If v is true, then also put variable name in 'vc' */
int typ(int *v, int t)
{
    int u, p;
    Sym *s;

    t = t & -3; /* suppress the ored '2' */
    while (tok == '*') {
        next();
        t = mk_pointer(t);
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
            *v = tok;
            next();
        }
    }
    /* append t at the end of u */
    t = post_type(t);
    if (!u) 
        return t;
    p = u;
    while(1) {
        s = sym_find((unsigned)p >> VT_STRUCT_SHIFT);
        p = s->t;
        if (!p) {
            s->t = t;
            break;
        }
    }
    return u;
}

/* define a new external reference to a function 'v' of type 'u' */
Sym *external_func(v, u)
{
    int t, n, p;
    Sym *s;
    s = sym_find(v);
    if (!s) {
        n = dlsym(0, get_tok_str(v));
        if (n == 0) {
            /* used to generate symbol list */
            s = sym_push1(&global_stack, 
                          v, u | VT_CONST | VT_LVAL | VT_FORWARD, 0);
        } else {
            /* int f() */
            s = sym_push1(&global_stack,
                          v, u | VT_CONST | VT_LVAL, n);
        }
    }
    return s;
}

/* read a character for string or char constant and eval escape codes */
int getq()
{
    int c;

    c = ch;
    minp();
    if (c == '\\') {
        if (isnum(ch)) {
            return getn(8);
        } else {
            if (ch == 'n')
                c = '\n';
            else if (ch == 'r')
                c = '\r';
            else if (ch == 't')
                c = '\t';
            else
                c = ch;
            minp();
        }
    }
    return c;
}

void indir()
{
    if (vt & VT_LVAL)
        gv();
    if (!(vt & VT_PTR))
        expect("pointer");
    vt = pointed_type(vt);
    if (!(vt & VT_ARRAY)) /* an array is never an lvalue */
        vt |= VT_LVAL;
}

void unary()
{
    int n, t, ft, fc, p, r;
    Sym *s;

    if (tok == TOK_NUM) {
        vset(VT_CONST, tokc);
        next();
    } else 
    if (tok == '\'') {
        vset(VT_CONST, getq());
        next(); /* skip char */
        skip('\''); 
    } else if (tok == '\"') {
        /* generate (char *) type */
        vset(VT_CONST | mk_pointer(VT_TYPE), glo);
        while (tok == '\"') {
            while (ch != '\"') {
                if (ch == -1)
                    error("unterminated string");
                *(char *)glo++ = getq();
            }
            minp();
            next();
        }
        *(char *)glo++ = 0;
    } else if (tok == TOK_DEFINED) {
        /* XXX: should only be used in preprocess expr parsing */
        next_nomacro();
        t = tok;
        if (t == '(') 
            next_nomacro();
        vset(VT_CONST, sym_find1(define_stack, tok) != 0);
        next();
        if (t == '(')
            skip(')');
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
            indir();
        } else if (t == '&') {
            unary();
            test_lvalue();
            vt = mk_pointer(vt & VT_LVALN);
        } else
        if (t == '!') {
            unary();
            if ((vt & VT_VALMASK) == VT_CMP)
                vc = vc ^ 1;
            else
                vset(VT_JMP, gtst(1, 0));
        } else 
        if (t == '~') {
            unary();
            vpush();
            vset(VT_CONST, -1);
            gen_op('^');
        } else 
        if (t == '+') {
            unary();
        } else 
        if (t == TOK_SIZEOF) {
            /* XXX: some code can be generated */
            if (tok == '(') {
                next();
                if (t = ist())
                    vt = typ(0, t);
                else
                    expr();
                skip(')');
            } else {
                unary();
            }
            vset(VT_CONST, type_size(vt, &t));
        } else
        if (t == TOK_INC | t == TOK_DEC) {
            unary();
            inc(0, t);
        } else if (t == '-') {
            vset(VT_CONST, 0);
            vpush();
            unary();
            gen_op('-');
        } else 
        {
            s = sym_find(t);
            if (!s) {
                if (tok != '(')
                    error("undefined symbol");
                /* for simple function calls, we tolerate undeclared
                   external reference */
                p = anon_sym++;        
                sym_push1(&global_stack, p, 0, FUNC_OLD);
                /* int() function */
                s = external_func(t, VT_FUNC | (p << VT_STRUCT_SHIFT)); 
            }
            vset(s->t, s->c);
            /* if forward reference, we must point to s->c */
            if (vt & VT_FORWARD)
                vc = (int)&s->c;
        }
    }
    
    /* post operations */
    while (1) {
        if (tok == TOK_INC | tok == TOK_DEC) {
            inc(1, tok);
            next();
        } else if (tok == '.' | tok == TOK_ARROW) {
            /* field */ 
            if (tok == TOK_ARROW) 
                indir();
            test_lvalue();
            vt &= VT_LVALN;
            next();
            /* expect pointer on structure */
            if (!(vt & VT_STRUCT))
                expect("struct or union");
            s = sym_find(((unsigned)vt >> VT_STRUCT_SHIFT) | SYM_STRUCT);
            /* find field */
            tok |= SYM_FIELD;
            while (s = s->next) {
                if (s->v == tok)
                    break;
            }
            if (!s)
                error("field not found");
            /* add field offset to pointer */
            vt = vt & VT_TYPEN; /* change type to int */
            vpush();
            vset(VT_CONST, s->c);
            gen_op('+');
            /* change type to field type, and set to lvalue */
            vt = (vt & VT_TYPEN) | VT_LVAL | s->t;
            next();
        } else if (tok == '[') {
            next();
            vpush();
            expr();
            gen_op('+');
            indir();
            skip(']');
        } else if (tok == '(') {
            /* function call  */
            save_regs(); /* save used temporary registers */
            /* lvalue is implied */
            vt = vt & VT_LVALN;
            if ((vt & VT_VALMASK) != VT_CONST) {
                /* evaluate function address */
                r = gv();
                o(0x50 + r); /* push r */
            }
            ft = vt;
            fc = vc;
            next();
            t = 0;
            while (tok != ')') {
                t = t + 4;
                expr_eq();
                r = gv();
                o(0x50 + r); /* push r */
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
            if ((ft & VT_VALMASK) == VT_CONST) {
                /* forward reference */
                if (ft & VT_FORWARD) {
                    *(int *)fc = psym(0xe8, *(int *)fc);
                } else
                    oad(0xe8, fc - ind - 5);
            } else {
                oad(0x2494ff, t); /* call *xxx(%esp) */
                t = t + 4;
            }
            if (t)
                oad(0xc481, t);
            /* get return type */
            s = sym_find((unsigned)ft >> VT_STRUCT_SHIFT);
            vt = s->t | 0; /* return register is eax */
        } else {
            break;
        }
    }
}

void uneq()
{
    int t;
    
    unary();
    if (tok == '=' | 
        (tok >= TOK_A_MOD & tok <= TOK_A_DIV) |
        tok == TOK_A_XOR | tok == TOK_A_OR | 
        tok == TOK_A_SHL | tok == TOK_A_SAR) {
        test_lvalue();
        vpush();
        t = tok;
        next();
        if (t == '=') {
            expr_eq();
            /* XXX: be more precise */
            if ((vt & VT_PTR) != (vstack_ptr[-2] & VT_PTR))
                warning("incompatible type");
        } else {
            vpush();
            expr_eq();
            gen_op(t & 0x7f);
        }
        vstore();
    }
}

void sum(l)
{
    int ft, fc, t;

    if (l == 0)
        uneq();
    else {
        sum(--l);
        while ((l == 0 & (tok == '*' | tok == '/' | tok == '%')) |
               (l == 1 & (tok == '+' | tok == '-')) |
               (l == 2 & (tok == TOK_SHL | tok == TOK_SAR)) |
               (l == 3 & ((tok >= TOK_ULE & tok <= TOK_GT) |
                          tok == TOK_ULT | tok == TOK_UGE)) |
               (l == 4 & (tok == TOK_EQ | tok == TOK_NE)) |
               (l == 5 & tok == '&') |
               (l == 6 & tok == '^') |
               (l == 7 & tok == '|')) {
            vpush();
            t = tok;
            next();
            sum(l);
            gen_op(t);
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
                vset(VT_JMPI, t);
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
        u = gjmp(0);
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

void block(int *bsym, int *csym, int *case_sym, int *def_sym, int case_reg)
{
    int a, b, c, d;
    Sym *s;

    if (tok == TOK_IF) {
        /* if test */
        next();
        skip('(');
        expr();
        skip(')');
        a = gtst(1, 0);
        block(bsym, csym, case_sym, def_sym, case_reg);
        c = tok;
        if (c == TOK_ELSE) {
            next();
            d = gjmp(0);
            gsym(a);
            block(bsym, csym, case_sym, def_sym, case_reg);
            gsym(d); /* patch else jmp */
        } else
            gsym(a);
    } else if (tok == TOK_WHILE) {
        next();
        d = ind;
        skip('(');
        expr();
        skip(')');
        a = gtst(1, 0);
        b = 0;
        block(&a, &b, case_sym, def_sym, case_reg);
        oad(0xe9, d - ind - 5); /* jmp */
        gsym(a);
        gsym_addr(b, d);
    } else if (tok == '{') {
        next();
        /* declarations */
        s = local_stack;
        decl(VT_LOCAL);
        while (tok != '}')
            block(bsym, csym, case_sym, def_sym, case_reg);
        /* pop locally defined symbols */
        sym_pop(&local_stack, s);
        next();
    } else if (tok == TOK_RETURN) {
        next();
        if (tok != ';') {
            expr();
            move_reg(0, gv());
        }
        skip(';');
        rsym = gjmp(rsym); /* jmp */
    } else if (tok == TOK_BREAK) {
        /* compute jump */
        if (!bsym)
            error("cannot break");
        *bsym = gjmp(*bsym);
        next();
        skip(';');
    } else if (tok == TOK_CONTINUE) {
        /* compute jump */
        if (!csym)
            error("cannot continue");
        *csym = gjmp(*csym);
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
        b = 0;
        if (tok != ';') {
            expr();
            a = gtst(1, 0);
        }
        skip(';');
        if (tok != ')') {
            e = gjmp(0);
            c = ind;
            expr();
            oad(0xe9, d - ind - 5); /* jmp */
            gsym(e);
        }
        skip(')');
        block(&a, &b, case_sym, def_sym, case_reg);
        oad(0xe9, c - ind - 5); /* jmp */
        gsym(a);
        gsym_addr(b, c);
    } else 
    if (tok == TOK_DO) {
        next();
        a = 0;
        b = 0;
        d = ind;
        block(&a, &b, case_sym, def_sym, case_reg);
        skip(TOK_WHILE);
        skip('(');
        gsym(b);
        expr();
        c = gtst(0, 0);
        gsym_addr(c, d);
        skip(')');
        gsym(a);
    } else
    if (tok == TOK_SWITCH) {
        next();
        skip('(');
        expr();
        case_reg = gv();
        skip(')');
        a = 0;
        b = 0;
        c = 0;
        block(&a, csym, &b, &c, case_reg);
        /* if no default, jmp after switch */
        if (c == 0)
            c = ind;
        /* default label */
        gsym_addr(b, c);
        /* break label */
        gsym(a);
    } else
    if (tok == TOK_CASE) {
        next();
        a = expr_const();
        if (!case_sym)
            expect("switch");
        gsym(*case_sym);
        vset(case_reg, 0);
        vpush();
        vset(VT_CONST, a);
        gen_op(TOK_EQ);
        *case_sym = gtst(1, 0);
        skip(':');
        block(bsym, csym, case_sym, def_sym, case_reg);
    } else 
    if (tok == TOK_DEFAULT) {
        next();
        skip(':');
        if (!def_sym)
            expect("switch");
        if (*def_sym)
            error("too many 'default'");
        *def_sym = ind;
        block(bsym, csym, case_sym, def_sym, case_reg);
    } else
    if (tok == TOK_GOTO) {
        next();
        s = sym_find1(label_stack, tok);
        /* put forward definition if needed */
        if (!s)
            s = sym_push1(&label_stack, tok, VT_FORWARD, 0);
        /* label already defined */
        if (s->t & VT_FORWARD) 
            s->c = gjmp(s->c); /* jmp xxx */
        else
            oad(0xe9, s->c - ind - 5); /* jmp xxx */
        next();
        skip(';');
    } else
#endif
    {
        b = tok;
        next();
        if (tok == ':') {
            next();
            /* label case */
            s = sym_find1(label_stack, b);
            if (s) {
                if (!(s->t & VT_FORWARD))
                    error("multiple defined label");
                gsym(s->c);
                s->c = ind;
                s->t = 0;
            } else {
                sym_push1(&label_stack, b, 0, ind);
            }
            block(bsym, csym, case_sym, def_sym, case_reg);
        } else {
            /* expression case: go backward of one token */
            /* XXX: currently incorrect if number/string/char */
            tok1 = tok;
            tok = b;
            if (tok != ';') {
                expr();
            }
            skip(';');
        }
    }
}

/* 'l' is VT_LOCAL or VT_CONST to define default storage type */
void decl(l)
{
    int *a, t, b, size, align, v, u, n;
    Sym *sym;

    while (b = ist()) {
        if ((b & (VT_ENUM | VT_STRUCT)) && tok == ';') {
            /* we accept no variable after */
            next();
            continue;
        }
        while (1) { /* iterate thru each declaration */
            t = typ(&v, b);
            if (tok == '{') {
                if (!(t & VT_FUNC))
                    expect("function defintion");
                /* patch forward references */
                if ((sym = sym_find(v)) && (sym->t & VT_FORWARD)) {
                    gsym(sym->c);
                    sym->c = ind;
                    sym->t = VT_CONST | VT_LVAL | t;
                } else {
                    /* put function address */
                    sym_push1(&global_stack, v, VT_CONST | VT_LVAL | t, ind);
                }
                /* push a dummy symbol to enable local sym storage */
                sym_push1(&local_stack, 0, 0, 0);
                /* define parameters */
                sym = sym_find((unsigned)t >> VT_STRUCT_SHIFT);
                while (sym = sym->next)
                    sym_push(sym->v & ~SYM_FIELD, sym->t, sym->c);
                loc = 0;
                o(0xe58955); /* push   %ebp, mov    %esp, %ebp */
                a = (int *)oad(0xec81, 0); /* sub $xxx, %esp */
                rsym = 0;
                block(0, 0, 0, 0, 0);
                gsym(rsym);
                o(0xc3c9); /* leave, ret */
                *a = (-loc + 3) & -4; /* align local size to word & 
                                         save local variables */
                sym_pop(&label_stack, 0); /* reset label stack */
                sym_pop(&local_stack, 0); /* reset local stack */
                break;
            } else {
                if (b & VT_TYPEDEF) {
                    /* save typedefed type */
                    sym_push(v, t | VT_TYPEDEF, 0);
                } else if (t & VT_FUNC) {
                    /* XXX: incorrect to flush, but needed while
                       waiting for function prototypes */
                    /* external function definition */
                    external_func(v, t);
                } else {
                    /* not lvalue if array */
                    if (!(t & VT_ARRAY))
                        t |= VT_LVAL;
                    if (t & VT_EXTERN) {
                        /* external variable */
                        /* XXX: factorize with external function def */
                        n = dlsym(NULL, get_tok_str(v));
                        if (!n)
                            error("unknown external variable");
                        sym_push(v, VT_CONST | t, n);
                    } else {
                        u = l;
                        if (t & VT_STATIC)
                            u = VT_CONST;
                        u |= t;
                        size = type_size(t, &align);
                        if (size < 0)
                            error("invalid size");
                        if ((u & VT_VALMASK) == VT_LOCAL) {
                            /* allocate space down on the stack */
                            loc = (loc - size) & -align;
                            sym_push(v, u, loc);
                        } else {
                            /* allocate space up in the data space */
                            glo = (glo + align - 1) & -align;
                            sym_push(v, u, glo);
                            glo += size;
                        }
                    }
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
    Sym *s;
    int (*t)();
    if (c < 2) {
        printf("usage: tcc source ...\n");
        return 1;
    }
    v++;
    filename = *v;
    line_num = 1;
    file = fopen(filename, "r");
    if (!file) {
        perror(filename);
        exit(1);
    }
    include_stack_ptr = include_stack;
    ifdef_stack_ptr = ifdef_stack;

    idtable = malloc(SYM_TABLE_SIZE);
    memcpy(idtable, 
           "int\0void\0char\0if\0else\0while\0break\0return\0for\0extern\0static\0unsigned\0goto\0do\0continue\0switch\0case\0const\0volatile\0long\0register\0signed\0auto\0inline\0float\0double\0struct\0union\0typedef\0default\0enum\0sizeof\0define\0include\0ifdef\0ifndef\0elif\0endif\0defined\0main", 251);
    idptr = idtable + 251;

    glo = malloc(DATA_SIZE);
    memset((void *)glo, 0, DATA_SIZE);
    prog = malloc(TEXT_SIZE);
    vstack_ptr = vstack;
    macro_stack_ptr = macro_stack;
    anon_sym = 1 << (31 - VT_STRUCT_SHIFT); 
    ind = prog;
    inp();
    ch = '\n'; /* needed to parse correctly first preprocessor command */
    next();
    decl(VT_CONST);
    if (tok != -1)
        expect("declaration");
#ifdef TEST
    { 
        FILE *f;
        f = fopen(v[1], "w");
        fwrite((void *)prog, 1, ind - prog, f);
        fclose(f);
        return 0;
    }
#else
    s = sym_find(TOK_MAIN);
    if (!s)
        error("main() not defined");
    t = s->c;
    return (*t)(c - 1, v);
#endif
}
