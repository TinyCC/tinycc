/*
 *  TCC - Tiny C Compiler
 * 
 *  Copyright (c) 2001, 2002 Fabrice Bellard
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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <sys/mman.h>
#include <elf.h>
#include <stab.h>
#include <malloc.h>
#ifndef CONFIG_TCC_STATIC
#include <dlfcn.h>
#endif

//#define DEBUG
/* preprocessor debug */
//#define PP_DEBUG

/* amount of virtual memory associate to a section (currently, we do
   not realloc them) */
#define SECTION_VSIZE       (1024 * 1024)

#define INCLUDE_STACK_SIZE  32
#define IFDEF_STACK_SIZE    64
#define VSTACK_SIZE         64
#define STRING_MAX_SIZE     1024
#define INCLUDE_PATHS_MAX   32

#define TOK_HASH_SIZE       521
#define TOK_ALLOC_INCR      256 /* must be a power of two */
#define SYM_HASH_SIZE       263

/* token symbol management */
typedef struct TokenSym {
    struct TokenSym *hash_next;
    int tok; /* token number */
    int len;
    char str[1];
} TokenSym;

/* constant value */
typedef union CValue {
    long double ld;
    double d;
    float f;
    int i;
    unsigned int ui;
    unsigned int ul; /* address (should be unsigned long on 64 bit cpu) */
    long long ll;
    unsigned long long ull;
    struct TokenSym *ts;
    struct Sym *sym;
    int tab[1];
} CValue;

/* value on stack */
typedef struct SValue {
    int t;      /* type */
    unsigned short r;      /* register + flags */
    unsigned short r2;     /* second register, used for 'long long'
                              type. If not used, set to VT_CONST */
    CValue c;   /* constant */
} SValue;

/* symbol management */
typedef struct Sym {
    int v;    /* symbol token */
    int t;    /* associated type */
    int r;    /* associated register */
    int c;    /* associated number */
    struct Sym *next; /* next related symbol */
    struct Sym *prev; /* prev symbol in stack */
    struct Sym *hash_next; /* next symbol in hash table */
} Sym;

typedef struct SymStack {
  struct Sym *top;
  struct Sym *hash[SYM_HASH_SIZE];
} SymStack;

/* relocation entry (currently only used for functions or variables */
typedef struct Reloc {
    int type;            /* type of relocation */
    int addr;            /* address of relocation */
    struct Reloc *next;  /* next relocation */
} Reloc;

#define RELOC_ADDR32 1  /* 32 bits relocation */
#define RELOC_REL32  2  /* 32 bits relative relocation */

/* section definition */
typedef struct Section {
    char name[64];           /* section name */
    unsigned char *data;     /* section data */
    unsigned char *data_ptr; /* current data pointer */
    int sh_num;              /* elf section number */
    int sh_type;             /* elf section type */
    int sh_flags;            /* elf section flags */
    int sh_entsize;          /* elf entry size */
    struct Section *link;    /* link to another section */
    struct Section *next;
} Section;

/* GNUC attribute definition */
typedef struct AttributeDef {
    int aligned;
    Section *section;
} AttributeDef;

#define SYM_STRUCT     0x40000000 /* struct/union/enum symbol space */
#define SYM_FIELD      0x20000000 /* struct/union field symbol space */
#define SYM_FIRST_ANOM (1 << (31 - VT_STRUCT_SHIFT)) /* first anonymous sym */

#define FUNC_NEW       1 /* ansi function prototype */
#define FUNC_OLD       2 /* old function prototype */
#define FUNC_ELLIPSIS  3 /* ansi function prototype with ... */

/* field 'Sym.t' for macros */
#define MACRO_OBJ      0 /* object like macro */
#define MACRO_FUNC     1 /* function like macro */

/* type_decl() types */
#define TYPE_ABSTRACT  1 /* type without variable */
#define TYPE_DIRECT    2 /* type with variable */

typedef struct {
    FILE *file;
    char *filename;
    int line_num;
} IncludeFile;

/* parser */
FILE *file;
int line_num;
int ch, ch1, tok, tok1;
CValue tokc, tok1c;

/* sections */
Section *first_section;
int section_num;
Section *text_section, *data_section, *bss_section; /* predefined sections */
Section *cur_text_section; /* current section where function code is
                              generated */
/* bound check related sections */
Section *bounds_section; /* contains global data bound description */
Section *lbounds_section; /* contains local data bound description */
/* debug sections */
Section *stab_section, *stabstr_section, *symtab_section, *strtab_section;

/* loc : local variable index
   ind : output code index
   rsym: return symbol
   anon_sym: anonymous symbol index
*/
int rsym, anon_sym,
    prog, ind, loc, const_wanted;
int global_expr; /* true if compound literals must be allocated
                    globally (used during initializers parsing */
int func_vt, func_vc; /* current function return type (used by
                         return instruction) */
int last_line_num, last_ind, func_ind; /* debug last line number and pc */
int tok_ident;
TokenSym **table_ident;
TokenSym *hash_ident[TOK_HASH_SIZE];
char token_buf[STRING_MAX_SIZE + 1];
char *filename, *funcname;
/* contains global symbols which remain between each translation unit */
SymStack extern_stack;
SymStack define_stack, global_stack, local_stack, label_stack;

SValue vstack[VSTACK_SIZE], *vtop;
int *macro_ptr, *macro_ptr_allocated;
IncludeFile include_stack[INCLUDE_STACK_SIZE], *include_stack_ptr;
int ifdef_stack[IFDEF_STACK_SIZE], *ifdef_stack_ptr;
char *include_paths[INCLUDE_PATHS_MAX];
int nb_include_paths;
int char_pointer_type;

/* compile with debug symbol (and use them if error during execution) */
int do_debug = 0;

/* compile with built-in memory and bounds checker */
int do_bounds_check = 0;

/* use GNU C extensions */
int gnu_ext = 1;

/* use Tiny C extensions */
int tcc_ext = 1;

/* The current value can be: */
#define VT_VALMASK   0x00ff
#define VT_CONST     0x00f0  /* constant in vc 
                              (must be first non register value) */
#define VT_LLOCAL    0x00f1  /* lvalue, offset on stack */
#define VT_LOCAL     0x00f2  /* offset on stack */
#define VT_CMP       0x00f3  /* the value is stored in processor flags (in vc) */
#define VT_JMP       0x00f4  /* value is the consequence of jmp true (even) */
#define VT_JMPI      0x00f5  /* value is the consequence of jmp false (odd) */
#define VT_LVAL      0x0100  /* var is an lvalue */
#define VT_FORWARD   0x0200  /* value is forward reference */
#define VT_MUSTCAST  0x0400  /* value must be casted to be correct (used for
                               char/short stored in integer registers) */
#define VT_MUSTBOUND 0x0800  /* bound checking must be done before
                                dereferencing value */

/* types */
#define VT_STRUCT_SHIFT 16   /* structure/enum name shift (16 bits left) */

#define VT_INT        0  /* integer type */
#define VT_BYTE       1  /* signed byte type */
#define VT_SHORT      2  /* short type */
#define VT_VOID       3  /* void type */
#define VT_PTR        4  /* pointer increment */
#define VT_ENUM       5  /* enum definition */
#define VT_FUNC       6  /* function type */
#define VT_STRUCT     7  /* struct/union definition */
#define VT_FLOAT      8  /* IEEE float */
#define VT_DOUBLE     9  /* IEEE double */
#define VT_LDOUBLE   10  /* IEEE long double */
#define VT_BOOL      11  /* ISOC99 boolean type */
#define VT_LLONG     12  /* 64 bit integer */
#define VT_LONG      13  /* long integer (NEVER USED as type, only
                            during parsing) */
#define VT_BTYPE      0x000f /* mask for basic type */
#define VT_UNSIGNED   0x0010  /* unsigned type */
#define VT_ARRAY      0x0020  /* array type (also has VT_PTR) */
#define VT_BITFIELD   0x0040  /* bitfield modifier */

/* storage */
#define VT_EXTERN  0x00000080  /* extern definition */
#define VT_STATIC  0x00000100  /* static variable */
#define VT_TYPEDEF 0x00000200  /* typedef definition */

/* type mask (except storage) */
#define VT_TYPE    (~(VT_EXTERN | VT_STATIC | VT_TYPEDEF))

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
#define TOK_UDIV  0xb0 /* unsigned division */
#define TOK_UMOD  0xb1 /* unsigned modulo */
#define TOK_PDIV  0xb2 /* fast division with undefined rounding for pointers */
#define TOK_CINT   0xb3 /* number in tokc */
#define TOK_CCHAR 0xb4 /* char constant in tokc */
#define TOK_STR   0xb5 /* pointer to string in tokc */
#define TOK_TWOSHARPS 0xb6 /* ## preprocessing token */
#define TOK_LCHAR 0xb7
#define TOK_LSTR  0xb8
#define TOK_CFLOAT   0xb9 /* float constant */
#define TOK_CDOUBLE  0xc0 /* double constant */
#define TOK_CLDOUBLE 0xc1 /* long double constant */
#define TOK_UMULL    0xc2 /* unsigned 32x32 -> 64 mul */
#define TOK_ADDC1    0xc3 /* add with carry generation */
#define TOK_ADDC2    0xc4 /* add with carry use */
#define TOK_SUBC1    0xc5 /* add with carry generation */
#define TOK_SUBC2    0xc6 /* add with carry use */
#define TOK_CUINT    0xc8 /* unsigned int constant */
#define TOK_CLLONG   0xc9 /* long long constant */
#define TOK_CULLONG  0xca /* unsigned long long constant */
#define TOK_ARROW    0xcb
#define TOK_DOTS     0xcc /* three dots */
#define TOK_SHR      0xcd /* unsigned shift right */

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

/* all identificators and strings have token above that */
#define TOK_IDENT 256

enum {
    TOK_INT = TOK_IDENT,
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
    TOK_RESTRICT,

    /* unsupported type */
    TOK_FLOAT,
    TOK_DOUBLE,
    TOK_BOOL,

    TOK_SHORT,
    TOK_STRUCT,
    TOK_UNION,
    TOK_TYPEDEF,
    TOK_DEFAULT,
    TOK_ENUM,
    TOK_SIZEOF,
    TOK___ATTRIBUTE__,
    
    /* preprocessor only */
    TOK_UIDENT, /* first "user" ident (not keyword) */
    TOK_DEFINE = TOK_UIDENT,
    TOK_INCLUDE,
    TOK_IFDEF,
    TOK_IFNDEF,
    TOK_ELIF,
    TOK_ENDIF,
    TOK_DEFINED,
    TOK_UNDEF,
    TOK_ERROR,
    TOK_LINE,
    TOK___LINE__,
    TOK___FILE__,
    TOK___DATE__,
    TOK___TIME__,
    TOK___VA_ARGS__,

    /* special identifiers */
    TOK___FUNC__,
    TOK_MAIN,
    /* attribute identifiers */
    TOK_SECTION,
    TOK___SECTION__,
    TOK_ALIGNED,
    TOK___ALIGNED__,
    TOK_UNUSED,
    TOK___UNUSED__,
};

/* XXX: need to define this to use them in non ISOC99 context */
extern float strtof (const char *__nptr, char **__endptr);
extern long double strtold (const char *__nptr, char **__endptr);

void sum(int l);
void next(void);
void next_nomacro(void);
int expr_const(void);
void expr_eq(void);
void gexpr(void);
void decl(int l);
void decl_initializer(int t, int r, int c, int first, int size_only);
int decl_initializer_alloc(int t, AttributeDef *ad, int r, int has_init);
int gv(int rc);
void gv2(int rc1, int rc2);
void move_reg(int r, int s);
void save_regs(void);
void save_reg(int r);
void vpop(void);
void vswap(void);
void vdup(void);
int get_reg(int rc);

void macro_subst(int **tok_str, int *tok_len, 
                 Sym **nested_list, int *macro_str);
int save_reg_forced(int r);
void gen_op(int op);
void force_charshort_cast(int t);
void gen_cast(int t);
void vstore(void);
Sym *sym_find(int v);
Sym *sym_push(int v, int t, int r, int c);

/* type handling */
int type_size(int t, int *a);
int pointed_type(int t);
int pointed_size(int t);
int is_compatible_types(int t1, int t2);
int parse_btype(int *type_ptr, AttributeDef *ad);
int type_decl(int *v, int t, int td);

void error(const char *fmt, ...);
void rt_error(unsigned long pc, const char *fmt, ...);
void vpushi(int v);
void vset(int t, int r, int v);
void type_to_str(char *buf, int buf_size, 
                 int t, const char *varstr);

/* section generation */
void greloc(Sym *s, int addr, int type);
static int put_elf_str(Section *s, const char *sym);
static void put_elf_sym(Section *s, 
                        unsigned long value, unsigned long size,
                        int info, int other, int shndx, const char *name);
static void put_stabs(const char *str, int type, int other, int desc, int value);
static void put_stabn(int type, int other, int desc, int value);
static void put_stabd(int type, int other, int desc);

/* true if float/double/long double type */
static inline int is_float(int t)
{
    int bt;
    bt = t & VT_BTYPE;
    return bt == VT_LDOUBLE || bt == VT_DOUBLE || bt == VT_FLOAT;
}

#include "bcheck.c"

#include "i386-gen.c"

#ifdef CONFIG_TCC_STATIC

#define RTLD_LAZY       0x001
#define RTLD_NOW        0x002
#define RTLD_GLOBAL     0x100

/* dummy function for profiling */
void *dlopen(const char *filename, int flag)
{
    return NULL;
}

const char *dlerror(void)
{
    return "error";
}

typedef struct TCCSyms {
    char *str;
    void *ptr;
} TCCSyms;

#define TCCSYM(a) { #a, &a, },

/* add the symbol you want here if no dynamic linking is done */
static TCCSyms tcc_syms[] = {
    TCCSYM(printf)
    TCCSYM(fprintf)
    TCCSYM(fopen)
    TCCSYM(fclose)
    { NULL, NULL },
};

void *dlsym(void *handle, char *symbol)
{
    TCCSyms *p;
    p = tcc_syms;
    while (p->str != NULL) {
        if (!strcmp(p->str, symbol))
            return p->ptr;
        p++;
    }
    return NULL;
}

#endif

/********************************************************/
/* runtime library is there */
/* XXX: we suppose that the host compiler handles 'long long'. It
   would not be difficult to suppress this assumption */

long long __divll(long long a, long long b)
{
    return a / b;
}

long long __modll(long long a, long long b)
{
    return a % b;
}

unsigned long long __divull(unsigned long long a, unsigned long long b)
{
    return a / b;
}

unsigned long long __modull(unsigned long long a, unsigned long long b)
{
    return a % b;
}

long long __sardi3(long long a, int b)
{
    return a >> b;
}

unsigned long long __shrdi3(unsigned long long a, int b)
{
    return a >> b;
}

long long __shldi3(long long a, int b)
{
    return a << b;
}

float __ulltof(unsigned long long a)
{
    return (float)a;
}

double __ulltod(unsigned long long a)
{
    return (double)a;
}

long double __ulltold(unsigned long long a)
{
    return (long double)a;
}

unsigned long long __ftoull(float a)
{
    return (unsigned long long)a;
}

unsigned long long __dtoull(double a)
{
    return (unsigned long long)a;
}

unsigned long long __ldtoull(long double a)
{
    return (unsigned long long)a;
}


/********************************************************/

/* copy a string and truncate it */
char *pstrcpy(char *buf, int buf_size, const char *s)
{
    char *q, *q_end;
    int c;

    q = buf;
    q_end = buf + buf_size - 1;
    while (q < q_end) {
        c = *s++;
        if (c == '\0')
            break;
        *q++ = c;
    }
    *q = '\0';
    return buf;
}

/* strcat and truncate */
char *pstrcat(char *buf, int buf_size, const char *s)
{
    int len;
    len = strlen(buf);
    if (len < buf_size) 
        pstrcpy(buf + len, buf_size - len, s);
    return buf;
}

Section *new_section(const char *name, int sh_type, int sh_flags)
{
    Section *sec, **psec;
    void *data;

    sec = malloc(sizeof(Section));
    if (!sec)
        error("memory full");
    memset(sec, 0, sizeof(Section));
    pstrcpy(sec->name, sizeof(sec->name), name);
    sec->link = NULL;
    sec->sh_num = ++section_num;
    sec->sh_type = sh_type;
    sec->sh_flags = sh_flags;
    data = mmap(NULL, SECTION_VSIZE, 
                PROT_EXEC | PROT_READ | PROT_WRITE, 
                MAP_PRIVATE | MAP_ANONYMOUS, 
                -1, 0);
    if (data == (void *)(-1))
        error("could not mmap section '%s'", name);
    sec->data = data;
    sec->data_ptr = data;
    psec = &first_section;
    while (*psec != NULL)
        psec = &(*psec)->next;
    sec->next = NULL;
    *psec = sec;
    return sec;
}

/* return a reference to a section, and create it if it does not
   exists */
Section *find_section(const char *name)
{
    Section *sec;
    
    for(sec = first_section; sec != NULL; sec = sec->next) {
        if (!strcmp(name, sec->name)) 
            return sec;
    }
    /* sections are created as PROGBITS */
    return new_section(name, SHT_PROGBITS, SHF_ALLOC);
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
        greloc_patch1(p, val);
        free(p);
        p = p1;
    }
    s->c = val;
    s->r &= ~VT_FORWARD;
}


static inline int isid(int c)
{
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

static inline int isnum(int c)
{
    return c >= '0' && c <= '9';
}

static inline int toup(int c)
{
    if (ch >= 'a' && ch <= 'z')
        return ch - 'a' + 'A';
    else
        return ch;
}

void printline(void)
{
    IncludeFile *f;
    for(f = include_stack; f < include_stack_ptr; f++)
        fprintf(stderr, "In file included from %s:%d:\n", 
                f->filename, f->line_num);
    fprintf(stderr, "%s:%d: ", filename, line_num);
}

void error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printline();
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
    va_end(ap);
}

void expect(const char *msg)
{
    error("%s expected", msg);
}

void warning(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    printline();
    fprintf(stderr, "warning: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void skip(int c)
{
    if (tok != c)
        error("'%c' expected", c);
    next();
}

void test_lvalue(void)
{
    if (!(vtop->r & VT_LVAL))
        expect("lvalue");
}

TokenSym *tok_alloc(const char *str, int len)
{
    TokenSym *ts, **pts, **ptable;
    int h, i;
    
    if (len <= 0)
        len = strlen(str);
    h = 1;
    for(i=0;i<len;i++)
        h = ((h << 8) | (str[i] & 0xff)) % TOK_HASH_SIZE;

    pts = &hash_ident[h];
    while (1) {
        ts = *pts;
        if (!ts)
            break;
        if (ts->len == len && !memcmp(ts->str, str, len))
            return ts;
        pts = &(ts->hash_next);
    }

    if (tok_ident >= SYM_FIRST_ANOM) 
        error("memory full");

    /* expand token table if needed */
    i = tok_ident - TOK_IDENT;
    if ((i % TOK_ALLOC_INCR) == 0) {
        ptable = realloc(table_ident, (i + TOK_ALLOC_INCR) * sizeof(TokenSym *));
        if (!ptable)
            error("memory full");
        table_ident = ptable;
    }

    ts = malloc(sizeof(TokenSym) + len);
    if (!ts)
        error("memory full");
    table_ident[i] = ts;
    ts->tok = tok_ident++;
    ts->len = len;
    ts->hash_next = NULL;
    memcpy(ts->str, str, len + 1);
    *pts = ts;
    return ts;
}

void add_char(char **pp, int c)
{
    char *p;
    p = *pp;
    if (c == '\'' || c == '\"' || c == '\\') {
        /* XXX: could be more precise if char or string */
        *p++ = '\\';
    }
    if (c >= 32 && c <= 126) {
        *p++ = c;
    } else {
        *p++ = '\\';
        if (c == '\n') {
            *p++ = 'n';
        } else {
            *p++ = '0' + ((c >> 6) & 7);
            *p++ = '0' + ((c >> 3) & 7);
            *p++ = '0' + (c & 7);
        }
    }
    *pp = p;
}

/* XXX: buffer overflow */
char *get_tok_str(int v, CValue *cv)
{
    static char buf[STRING_MAX_SIZE + 1];
    TokenSym *ts;
    char *p;
    int i;

    if (v == TOK_CINT || v == TOK_CUINT) {
        sprintf(buf, "%u", cv->ui);
        return buf;
    } else if (v == TOK_CCHAR || v == TOK_LCHAR) {
        p = buf;
        *p++ = '\'';
        add_char(&p, cv->i);
        *p++ = '\'';
        *p = '\0';
        return buf;
    } else if (v == TOK_STR || v == TOK_LSTR) {
        ts = cv->ts;
        p = buf;
        *p++ = '\"';
        for(i=0;i<ts->len;i++)
            add_char(&p, ts->str[i]);
        *p++ = '\"';
        *p = '\0';
        return buf;
    } else if (v < TOK_IDENT) {
        p = buf;
        *p++ = v;
        *p = '\0';
        return buf;
    } else if (v < tok_ident) {
        return table_ident[v - TOK_IDENT]->str;
    } else {
        /* should never happen */
        return NULL;
    }
}

/* push, without hashing */
Sym *sym_push2(Sym **ps, int v, int t, int c)
{
    Sym *s;
    s = malloc(sizeof(Sym));
    if (!s)
        error("memory full");
    s->v = v;
    s->t = t;
    s->c = c;
    s->next = NULL;
    /* add in stack */
    s->prev = *ps;
    *ps = s;
    return s;
}

/* find a symbol and return its associated structure. 's' is the top
   of the symbol stack */
Sym *sym_find2(Sym *s, int v)
{
    while (s) {
        if (s->v == v)
            return s;
        s = s->prev;
    }
    return NULL;
}

#define HASH_SYM(v) ((unsigned)(v) % SYM_HASH_SIZE)

/* find a symbol and return its associated structure. 'st' is the
   symbol stack */
Sym *sym_find1(SymStack *st, int v)
{
    Sym *s;

    s = st->hash[HASH_SYM(v)];
    while (s) {
        if (s->v == v)
            return s;
        s = s->hash_next;
    }
    return NULL;
}

Sym *sym_push1(SymStack *st, int v, int t, int c)
{
    Sym *s, **ps;
    s = sym_push2(&st->top, v, t, c);
    /* add in hash table */
    if (v) {
        ps = &st->hash[HASH_SYM(v)];
        s->hash_next = *ps;
        *ps = s;
    }
    return s;
}

/* find a symbol in the right symbol space */
Sym *sym_find(int v)
{
    Sym *s;
    s = sym_find1(&local_stack, v);
    if (!s)
        s = sym_find1(&global_stack, v);
    return s;
}

/* push a given symbol on the symbol stack */
Sym *sym_push(int v, int t, int r, int c)
{
    Sym *s;
    if (local_stack.top)
        s = sym_push1(&local_stack, v, t, c);
    else
        s = sym_push1(&global_stack, v, t, c);
    s->r = r;
    return s;
}

/* pop symbols until top reaches 'b' */
void sym_pop(SymStack *st, Sym *b)
{
    Sym *s, *ss;

    s = st->top;
    while(s != b) {
        ss = s->prev;
        /* free hash table entry, except if symbol was freed (only
           used for #undef symbols) */
        if (s->v)
            st->hash[HASH_SYM(s->v)] = s->hash_next;
        free(s);
        s = ss;
    }
    st->top = b;
}

/* undefined a hashed symbol (used for #undef). Its name is set to
   zero */
void sym_undef(SymStack *st, Sym *s)
{
    Sym **ss;
    ss = &st->hash[HASH_SYM(s->v)];
    while (*ss != NULL) {
        if (*ss == s)
            break;
        ss = &(*ss)->hash_next;
    }
    *ss = s->hash_next;
    s->v = 0;
}

/* no need to put that inline */
int handle_eof(void)
{
    if (include_stack_ptr == include_stack)
        return -1;
    /* add end of include file debug info */
    if (do_debug) {
        put_stabd(N_EINCL, 0, 0);
    }
    /* pop include stack */
    fclose(file);
    free(filename);
    include_stack_ptr--;
    file = include_stack_ptr->file;
    filename = include_stack_ptr->filename;
    line_num = include_stack_ptr->line_num;
    return 0;
}

/* read next char from current input file */
static inline void inp(void)
{
 redo:
    /* faster than fgetc */
    ch1 = getc_unlocked(file);
    if (ch1 == -1) {
        if (handle_eof() < 0)
            return;
        else
            goto redo;
    }
    if (ch1 == '\n')
        line_num++;
    //    printf("ch1=%c 0x%x\n", ch1, ch1);
}

/* input with '\\n' handling */
static inline void minp(void)
{
 redo:
    ch = ch1;
    inp();
    if (ch == '\\' && ch1 == '\n') {
        inp();
        goto redo;
    }
    //printf("ch=%c 0x%x\n", ch, ch);
}


/* same as minp, but also skip comments */
void cinp(void)
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

void skip_spaces(void)
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
            next_nomacro();
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

/* return the number of additionnal 'ints' necessary to store the
   token */
static inline int tok_ext_size(int t)
{
    switch(t) {
        /* 4 bytes */
    case TOK_CINT:
    case TOK_CUINT:
    case TOK_CCHAR:
    case TOK_LCHAR:
    case TOK_STR:
    case TOK_LSTR:
    case TOK_CFLOAT:
        return 1;
    case TOK_CDOUBLE:
    case TOK_CLLONG:
    case TOK_CULLONG:
        return 2;
    case TOK_CLDOUBLE:
        return LDOUBLE_SIZE / 4;
    default:
        return 0;
    }
}

void tok_add(int **tok_str, int *tok_len, int t)
{
    int len, *str;
    len = *tok_len;
    str = *tok_str;
    if ((len & 63) == 0) {
        str = realloc(str, (len + 64) * sizeof(int));
        if (!str)
            return;
        *tok_str = str;
    }
    str[len++] = t;
    *tok_len = len;
}

void tok_add2(int **tok_str, int *tok_len, int t, CValue *cv)
{
    int n, i;

    tok_add(tok_str, tok_len, t);
    n = tok_ext_size(t);
    for(i=0;i<n;i++)
        tok_add(tok_str, tok_len, cv->tab[i]);
}

/* get a token from an integer array and increment pointer accordingly */
int tok_get(int **tok_str, CValue *cv)
{
    int *p, t, n, i;

    p = *tok_str;
    t = *p++;
    n = tok_ext_size(t);
    for(i=0;i<n;i++)
        cv->tab[i] = *p++;
    *tok_str = p;
    return t;
}

/* eval an expression for #if/#elif */
int expr_preprocess(void)
{
    int *str, len, c, t;
    
    str = NULL;
    len = 0;
    while (1) {
        skip_spaces();
        if (ch == '\n')
            break;
        next(); /* do macro subst */
        if (tok == TOK_DEFINED) {
            next_nomacro();
            t = tok;
            if (t == '(') 
                next_nomacro();
            c = sym_find1(&define_stack, tok) != 0;
            if (t == '(')
                next_nomacro();
            tok = TOK_CINT;
            tokc.i = c;
        } else if (tok >= TOK_IDENT) {
            /* if undefined macro */
            tok = TOK_CINT;
            tokc.i = 0;
        }
        tok_add2(&str, &len, tok, &tokc);
    }
    tok_add(&str, &len, -1); /* simulate end of file */
    tok_add(&str, &len, 0);
    /* now evaluate C constant expression */
    macro_ptr = str;
    next();
    c = expr_const();
    macro_ptr = NULL;
    free(str);
    return c != 0;
}

#if defined(DEBUG)
void tok_print(int *str)
{
    int t;
    CValue cval;

    while (1) {
        t = tok_get(&str, &cval);
        if (!t)
            break;
        printf(" %s", get_tok_str(t, &cval));
    }
    printf("\n");
}
#endif

void preprocess(void)
{
    int size, i, c, v, t, *str, len;
    char buf[1024], *q, *p;
    char buf1[1024];
    FILE *f;
    Sym **ps, *first, *s;

    cinp();
    next_nomacro();
 redo:
    if (tok == TOK_DEFINE) {
        next_nomacro();
        v = tok;
        /* XXX: should check if same macro (ANSI) */
        first = NULL;
        t = MACRO_OBJ;
        /* '(' must be just after macro definition for MACRO_FUNC */
        if (ch == '(') {
            next_nomacro();
            next_nomacro();
            ps = &first;
            while (tok != ')') {
                if (tok == TOK_DOTS) 
                    tok = TOK___VA_ARGS__;
                s = sym_push1(&define_stack, tok | SYM_FIELD, 0, 0);
                *ps = s;
                ps = &s->next;
                next_nomacro();
                if (tok != ',')
                    break;
                next_nomacro();
            }
            t = MACRO_FUNC;
        }
        str = NULL;
        len = 0;
        while (1) {
            skip_spaces();
            if (ch == '\n' || ch == -1)
                break;
            next_nomacro();
            tok_add2(&str, &len, tok, &tokc);
        }
        tok_add(&str, &len, 0);
#ifdef PP_DEBUG
        printf("define %s %d: ", get_tok_str(v, NULL), t);
        tok_print(str);
#endif
        s = sym_push1(&define_stack, v, t, (int)str);
        s->next = first;
    } else if (tok == TOK_UNDEF) {
        next_nomacro();
        s = sym_find1(&define_stack, tok);
        /* undefine symbol by putting an invalid name */
        if (s)
            sym_undef(&define_stack, s);
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
        } else {
            next();
            if (tok != TOK_STR)
                error("#include syntax error");
            pstrcpy(buf, sizeof(buf), get_tok_str(tok, &tokc));
            c = '\"';
        }
        /* eat all spaces and comments after include */
        /* XXX: slightly incorrect */
        while (ch1 != '\n' && ch1 != -1)
            inp();

        if (include_stack_ptr >= include_stack + INCLUDE_STACK_SIZE)
            error("memory full");
        if (c == '\"') {
            /* first search in current dir if "header.h" */
            size = 0;
            p = strrchr(filename, '/');
            if (p) 
                size = p + 1 - filename;
            if (size > sizeof(buf1) - 1)
                size = sizeof(buf1) - 1;
            memcpy(buf1, filename, size);
            buf1[size] = '\0';
            pstrcat(buf1, sizeof(buf1), buf);
            f = fopen(buf1, "r");
            if (f)
                goto found;
        }
        /* now search in standard include path */
        for(i=nb_include_paths - 1;i>=0;i--) {
            strcpy(buf1, include_paths[i]);
            strcat(buf1, "/");
            strcat(buf1, buf);
            f = fopen(buf1, "r");
            if (f)
                goto found;
        }
        error("include file '%s' not found", buf1);
        f = NULL;
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
        /* add include file debug info */
        if (do_debug) {
            put_stabs(filename, N_BINCL, 0, 0, 0);
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
        c = (sym_find1(&define_stack, tok) != 0) ^ c;
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
    } else if (tok == TOK_LINE) {
        next();
        if (tok != TOK_CINT)
            error("#line");
        line_num = tokc.i;
        skip_spaces();
        if (ch != '\n') {
            next();
            if (tok != TOK_STR)
                error("#line");
            /* XXX: potential memory leak */
            filename = strdup(get_tok_str(tok, &tokc));
        }
    } else if (tok == TOK_ERROR) {
        error("#error");
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
        if (ch >= 'a' && ch <= 'f')
            t = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F')
            t = ch - 'A' + 10;
        else if (isnum(ch))
            t = ch - '0';
        else
            break;
        if (t < 0 || t >= b)
            break;
        n = n * b + t;
        cinp();
    }
    return n;
}

/* read a character for string or char constant and eval escape codes */
int getq()
{
    int c;

    c = ch;
    minp();
    if (c == '\\') {
        if (isnum(ch)) {
            /* at most three octal digits */
            c = ch - '0';
            minp();
            if (isnum(ch)) {
                c = c * 8 + ch - '0';
                minp();
                if (isnum(ch)) {
                    c = c * 8 + ch - '0';
                    minp();
                }
            }
            return c;
        } else if (ch == 'x') {
            minp();
            return getn(16);
        } else {
            if (ch == 'a')
                c = '\a';
            else if (ch == 'b')
                c = '\b';
            else if (ch == 'f')
                c = '\f';
            else if (ch == 'n')
                c = '\n';
            else if (ch == 'r')
                c = '\r';
            else if (ch == 't')
                c = '\t';
            else if (ch == 'v')
                c = '\v';
            else if (ch == 'e' && gnu_ext)
                c = 27;
            else if (ch == '\'' || ch == '\"' || ch == '\\' || ch == '?')
                c = ch;
            else
                error("invalid escaped char");
            minp();
        }
    }
    return c;
}

/* we use 64 bit numbers */
#define BN_SIZE 2

/* bn = (bn << shift) | or_val */
void bn_lshift(unsigned int *bn, int shift, int or_val)
{
    int i;
    unsigned int v;
    for(i=0;i<BN_SIZE;i++) {
        v = bn[i];
        bn[i] = (v << shift) | or_val;
        or_val = v >> (32 - shift);
    }
}

void bn_zero(unsigned int *bn)
{
    int i;
    for(i=0;i<BN_SIZE;i++) {
        bn[i] = 0;
    }
}

void parse_number(void)
{
    int b, t, shift, frac_bits, s, exp_val;
    char *q;
    unsigned int bn[BN_SIZE];
    double d;

    /* number */
    q = token_buf;
    t = ch;
    cinp();
    *q++ = t;
    b = 10;
    if (t == '.') {
        /* special dot handling */
        if (ch >= '0' && ch <= '9') {
            goto float_frac_parse;
        } else if (ch == '.') {
            cinp();
            if (ch != '.')
                expect("'.'");
            cinp();
            tok = TOK_DOTS;
        } else {
            /* dots */
            tok = t;
        }
        return;
    } else if (t == '0') {
        if (ch == 'x' || ch == 'X') {
            q--;
            cinp();
            b = 16;
        } else if (tcc_ext && (ch == 'b' || ch == 'B')) {
            q--;
            cinp();
            b = 2;
        }
    }
    /* parse all digits. cannot check octal numbers at this stage
       because of floating point constants */
    while (1) {
        if (ch >= 'a' && ch <= 'f')
            t = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F')
            t = ch - 'A' + 10;
        else if (isnum(ch))
            t = ch - '0';
        else
            break;
        if (t >= b)
            break;
        if (q >= token_buf + STRING_MAX_SIZE) {
        num_too_long:
            error("number too long");
        }
        *q++ = ch;
        cinp();
    }
    if (ch == '.' ||
        ((ch == 'e' || ch == 'E') && b == 10) ||
        ((ch == 'p' || ch == 'P') && (b == 16 || b == 2))) {
        if (b != 10) {
            /* NOTE: strtox should support that for hexa numbers, but
               non ISOC99 libcs do not support it, so we prefer to do
               it by hand */
            /* hexadecimal or binary floats */
            /* XXX: handle overflows */
            *q = '\0';
            if (b == 16)
                shift = 4;
            else 
                shift = 2;
            bn_zero(bn);
            q = token_buf;
            while (1) {
                t = *q++;
                if (t == '\0') {
                    break;
                } else if (t >= 'a') {
                    t = t - 'a' + 10;
                } else if (t >= 'A') {
                    t = t - 'A' + 10;
                } else {
                    t = t - '0';
                }
                bn_lshift(bn, shift, t);
            }
            frac_bits = 0;
            if (ch == '.') {
                cinp();
                while (1) {
                    t = ch;
                    if (t >= 'a' && t <= 'f') {
                        t = t - 'a' + 10;
                    } else if (t >= 'A' && t <= 'F') {
                        t = t - 'A' + 10;
                    } else if (t >= '0' && t <= '9') {
                        t = t - '0';
                    } else {
                        break;
                    }
                    if (t >= b)
                        error("invalid digit");
                    bn_lshift(bn, shift, t);
                    frac_bits += shift;
                    cinp();
                }
            }
            if (ch != 'p' && ch != 'P')
                error("exponent expected");
            cinp();
            s = 1;
            exp_val = 0;
            if (ch == '+') {
                cinp();
            } else if (ch == '-') {
                s = -1;
                cinp();
            }
            if (ch < '0' || ch > '9')
                error("exponent digits expected");
            while (ch >= '0' && ch <= '9') {
                exp_val = exp_val * 10 + ch - '0';
                cinp();
            }
            exp_val = exp_val * s;
            
            /* now we can generate the number */
            /* XXX: should patch directly float number */
            d = (double)bn[1] * 4294967296.0 + (double)bn[0];
            d = ldexp(d, exp_val - frac_bits);
            t = toup(ch);
            if (t == 'F') {
                cinp();
                tok = TOK_CFLOAT;
                /* float : should handle overflow */
                tokc.f = (float)d;
            } else if (t == 'L') {
                cinp();
                tok = TOK_CLDOUBLE;
                /* XXX: not large enough */
                tokc.ld = (long double)d;
            } else {
                tok = TOK_CDOUBLE;
                tokc.d = d;
            }
        } else {
            /* decimal floats */
            if (ch == '.') {
                if (q >= token_buf + STRING_MAX_SIZE)
                    goto num_too_long;
                *q++ = ch;
                cinp();
            float_frac_parse:
                while (ch >= '0' && ch <= '9') {
                    if (q >= token_buf + STRING_MAX_SIZE)
                        goto num_too_long;
                    *q++ = ch;
                    cinp();
                }
            }
            if (ch == 'e' || ch == 'E') {
                if (q >= token_buf + STRING_MAX_SIZE)
                    goto num_too_long;
                *q++ = ch;
                cinp();
                if (ch == '-' || ch == '+') {
                    if (q >= token_buf + STRING_MAX_SIZE)
                        goto num_too_long;
                    *q++ = ch;
                    cinp();
                }
                if (ch < '0' || ch > '9')
                    error("exponent digits expected");
                while (ch >= '0' && ch <= '9') {
                    if (q >= token_buf + STRING_MAX_SIZE)
                        goto num_too_long;
                    *q++ = ch;
                    cinp();
                }
            }
            *q = '\0';
            t = toup(ch);
            errno = 0;
            if (t == 'F') {
                cinp();
                tok = TOK_CFLOAT;
                tokc.f = strtof(token_buf, NULL);
            } else if (t == 'L') {
                cinp();
                tok = TOK_CLDOUBLE;
                tokc.ld = strtold(token_buf, NULL);
            } else {
                tok = TOK_CDOUBLE;
                tokc.d = strtod(token_buf, NULL);
            }
        }
    } else {
        unsigned long long n, n1;
        int lcount;

        /* integer number */
        *q = '\0';
        q = token_buf;
        if (b == 10 && *q == '0') {
            b = 8;
            q++;
        }
        n = 0;
        while(1) {
            t = *q++;
            /* no need for checks except for base 10 / 8 errors */
            if (t == '\0') {
                break;
            } else if (t >= 'a') {
                t = t - 'a' + 10;
            } else if (t >= 'A') {
                t = t - 'A' + 10;
            } else {
                t = t - '0';
                if (t >= b)
                    error("invalid digit");
            }
            n1 = n;
            n = n * b + t;
            /* detect overflow */
            if (n < n1)
                error("integer constant overflow");
        }
        
        /* XXX: not exactly ANSI compliant */
        if ((n & 0xffffffff00000000LL) != 0) {
            if ((n >> 63) != 0)
                tok = TOK_CULLONG;
            else
                tok = TOK_CLLONG;
        } else if (n > 0x7fffffff) {
            tok = TOK_CUINT;
        } else {
            tok = TOK_CINT;
        }
        lcount = 0;
        for(;;) {
            t = toup(ch);
            if (t == 'L') {
                if (lcount >= 2)
                    error("three 'l' in integer constant");
                lcount++;
                if (lcount == 2) {
                    if (tok == TOK_CINT)
                        tok = TOK_CLLONG;
                    else if (tok == TOK_CUINT)
                        tok = TOK_CULLONG;
                }
                cinp();
            } else if (t == 'U') {
                if (tok == TOK_CINT)
                    tok = TOK_CUINT;
                else if (tok == TOK_CLLONG)
                    tok = TOK_CULLONG;
                cinp();
            } else {
                break;
            }
        }
        if (tok == TOK_CINT || tok == TOK_CUINT)
            tokc.ui = n;
        else
            tokc.ull = n;
    }
}


/* return next token without macro substitution */
void next_nomacro1(void)
{
    int b;
    char *q;
    TokenSym *ts;

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
        if (ch != ' ' && ch != '\t' && ch != '\f')
            break;
        cinp();
    }
    if (isid(ch)) {
        q = token_buf;
        *q++ = ch;
        cinp();
        if (q[-1] == 'L') {
            if (ch == '\'') {
                tok = TOK_LCHAR;
                goto char_const;
            }
            if (ch == '\"') {
                tok = TOK_LSTR;
                goto str_const;
            }
        }
        while (isid(ch) || isnum(ch)) {
            if (q >= token_buf + STRING_MAX_SIZE)
                error("ident too long");
            *q++ = ch;
            cinp();
        }
        *q = '\0';
        ts = tok_alloc(token_buf, q - token_buf);
        tok = ts->tok;
    } else if (isnum(ch) || ch == '.') {
        parse_number();
    } else if (ch == '\'') {
        tok = TOK_CCHAR;
    char_const:
        minp();
        tokc.i = getq();
        if (ch != '\'')
            expect("\'");
        minp();
    } else if (ch == '\"') {
        tok = TOK_STR;
    str_const:
        minp();
        q = token_buf;
        while (ch != '\"') {
            b = getq();
            if (ch == -1)
                error("unterminated string");
            if (q >= token_buf + STRING_MAX_SIZE)
                error("string too long");
            *q++ = b;
        }
        *q = '\0';
        tokc.ts = tok_alloc(token_buf, q - token_buf);
        minp();
    } else {
        q = "<=\236>=\235!=\225&&\240||\241++\244--\242==\224<<\1>>\2+=\253-=\255*=\252/=\257%=\245&=\246^=\336|=\374->\313..\250##\266";
        /* two chars */
        tok = ch;
        cinp();
        while (*q) {
            if (*q == tok && q[1] == ch) {
                cinp();
                tok = q[2] & 0xff;
                /* three chars tests */
                if (tok == TOK_SHL || tok == TOK_SAR) {
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
}

/* return next token without macro substitution. Can read input from
   macro_ptr buffer */
void next_nomacro()
{
    if (macro_ptr) {
        tok = *macro_ptr;
        if (tok)
            tok = tok_get(&macro_ptr, &tokc);
    } else {
        next_nomacro1();
    }
}

/* substitute args in macro_str and return allocated string */
int *macro_arg_subst(Sym **nested_list, int *macro_str, Sym *args)
{
    int *st, last_tok, t, notfirst, *str, len;
    Sym *s;
    TokenSym *ts;
    CValue cval;

    str = NULL;
    len = 0;
    last_tok = 0;
    while(1) {
        t = tok_get(&macro_str, &cval);
        if (!t)
            break;
        if (t == '#') {
            /* stringize */
            t = tok_get(&macro_str, &cval);
            if (!t)
                break;
            s = sym_find2(args, t);
            if (s) {
                token_buf[0] = '\0';
                st = (int *)s->c;
                notfirst = 0;
                while (*st) {
                    if (notfirst)
                        pstrcat(token_buf, sizeof(token_buf), " ");
                    t = tok_get(&st, &cval);
                    pstrcat(token_buf, sizeof(token_buf), get_tok_str(t, &cval));
                    notfirst = 1;
                }
#ifdef PP_DEBUG
                printf("stringize: %s\n", token_buf);
#endif
                /* add string */
                ts = tok_alloc(token_buf, 0);
                cval.ts = ts;
                tok_add2(&str, &len, TOK_STR, &cval);
            } else {
                tok_add2(&str, &len, t, &cval);
            }
        } else if (t >= TOK_IDENT) {
            s = sym_find2(args, t);
            if (s) {
                st = (int *)s->c;
                /* if '##' is present before or after , no arg substitution */
                if (*macro_str == TOK_TWOSHARPS || last_tok == TOK_TWOSHARPS) {
                    while (*st)
                        tok_add(&str, &len, *st++);
                } else {
                    macro_subst(&str, &len, nested_list, st);
                }
            } else {
                tok_add(&str, &len, t);
            }
        } else {
            tok_add2(&str, &len, t, &cval);
        }
        last_tok = t;
    }
    tok_add(&str, &len, 0);
    return str;
}

/* handle the '##' operator */
int *macro_twosharps(int *macro_str)
{
    TokenSym *ts;
    int *macro_str1, macro_str1_len, *macro_ptr1;
    int t;
    char *p;
    CValue cval;

    macro_str1 = NULL;
    macro_str1_len = 0;
    tok = 0;
    while (1) {
        next_nomacro();
        if (tok == 0)
            break;
        while (*macro_ptr == TOK_TWOSHARPS) {
            macro_ptr++;
            macro_ptr1 = macro_ptr;
            t = *macro_ptr;
            if (t) {
                t = tok_get(&macro_ptr, &cval);
                /* XXX: we handle only most common cases: 
                   ident + ident or ident + number */
                if (tok >= TOK_IDENT && 
                    (t >= TOK_IDENT || t == TOK_CINT)) {
                    p = get_tok_str(tok, &tokc);
                    pstrcpy(token_buf, sizeof(token_buf), p);
                    p = get_tok_str(t, &cval);
                    pstrcat(token_buf, sizeof(token_buf), p);
                    ts = tok_alloc(token_buf, 0);
                    tok = ts->tok; /* modify current token */
                } else {
                    /* cannot merge tokens: skip '##' */
                    macro_ptr = macro_ptr1;
                    break;
                }
            }
        }
        tok_add2(&macro_str1, &macro_str1_len, tok, &tokc);
    }
    tok_add(&macro_str1, &macro_str1_len, 0);
    return macro_str1;
}



/* do macro substitution of macro_str and add result to
   (tok_str,tok_len). If macro_str is NULL, then input stream token is
   substituted. 'nested_list' is the list of all macros we got inside
   to avoid recursing. */
void macro_subst(int **tok_str, int *tok_len, 
                 Sym **nested_list, int *macro_str)
{
    Sym *s, *args, *sa, *sa1;
    int *str, parlevel, len, *mstr, t, *saved_macro_ptr;
    int mstr_allocated, *macro_str1;
    CValue cval;

    saved_macro_ptr = macro_ptr;
    macro_ptr = macro_str;
    macro_str1 = NULL;
    if (macro_str) {
        /* first scan for '##' operator handling */
        macro_str1 = macro_twosharps(macro_str);
        macro_ptr = macro_str1;
    }

    while (1) {
        next_nomacro();
        if (tok == 0)
            break;
        /* special macros */
        if (tok == TOK___LINE__) {
            cval.i = line_num;
            tok_add2(tok_str, tok_len, TOK_CINT, &cval);
        } else if (tok == TOK___FILE__) {
            cval.ts = tok_alloc(filename, 0);
            tok_add2(tok_str, tok_len, TOK_STR, &cval);
        } else if (tok == TOK___DATE__) {
            cval.ts = tok_alloc("Jan  1 1970", 0);
            tok_add2(tok_str, tok_len, TOK_STR, &cval);
        } else if (tok == TOK___TIME__) {
            cval.ts = tok_alloc("00:00:00", 0);
            tok_add2(tok_str, tok_len, TOK_STR, &cval);
        } else if ((s = sym_find1(&define_stack, tok)) != NULL) {
            /* if symbol is a macro, prepare substitution */
            /* if nested substitution, do nothing */
            if (sym_find2(*nested_list, tok))
                goto no_subst;
            mstr = (int *)s->c;
            mstr_allocated = 0;
            if (s->t == MACRO_FUNC) {
                /* NOTE: we do not use next_nomacro to avoid eating the
                   next token. XXX: find better solution */
                if (macro_ptr) {
                    t = *macro_ptr;
                } else {
                    while (ch == ' ' || ch == '\t' || ch == '\n')
                        cinp();
                    t = ch;
                }
                if (t != '(') /* no macro subst */
                    goto no_subst;
                    
                /* argument macro */
                next_nomacro();
                next_nomacro();
                args = NULL;
                sa = s->next;
                while (tok != ')' && sa) {
                    len = 0;
                    str = NULL;
                    parlevel = 0;
                    while ((parlevel > 0 || 
                            (tok != ')' && 
                             (tok != ',' || 
                              sa->v == (TOK___VA_ARGS__ | SYM_FIELD)))) && 
                           tok != -1) {
                        if (tok == '(')
                            parlevel++;
                        else if (tok == ')')
                            parlevel--;
                        tok_add2(&str, &len, tok, &tokc);
                        next_nomacro();
                    }
                    tok_add(&str, &len, 0);
                    sym_push2(&args, sa->v & ~SYM_FIELD, 0, (int)str);
                    if (tok != ',')
                        break;
                    next_nomacro();
                    sa = sa->next;
                }
                if (tok != ')')
                    expect(")");
                /* now subst each arg */
                mstr = macro_arg_subst(nested_list, mstr, args);
                /* free memory */
                sa = args;
                while (sa) {
                    sa1 = sa->prev;
                    free((int *)sa->c);
                    free(sa);
                    sa = sa1;
                }
                mstr_allocated = 1;
            }
            sym_push2(nested_list, s->v, 0, 0);
            macro_subst(tok_str, tok_len, nested_list, mstr);
            /* pop nested defined symbol */
            sa1 = *nested_list;
            *nested_list = sa1->prev;
            free(sa1);
            if (mstr_allocated)
                free(mstr);
        } else {
        no_subst:
            /* no need to add if reading input stream */
            if (!macro_str)
                return;
            tok_add2(tok_str, tok_len, tok, &tokc);
        }
        /* only replace one macro while parsing input stream */
        if (!macro_str)
            return;
    }
    macro_ptr = saved_macro_ptr;
    if (macro_str1)
        free(macro_str1);
}

/* return next token with macro substitution */
void next(void)
{
    int len, *ptr;
    Sym *nested_list;

    /* special 'ungettok' case for label parsing */
    if (tok1) {
        tok = tok1;
        tokc = tok1c;
        tok1 = 0;
    } else {
    redo:
        if (!macro_ptr) {
            /* if not reading from macro substuted string, then try to substitute */
            len = 0;
            ptr = NULL;
            nested_list = NULL;
            macro_subst(&ptr, &len, &nested_list, NULL);
            if (ptr) {
                tok_add(&ptr, &len, 0);
                macro_ptr = ptr;
                macro_ptr_allocated = ptr;
                goto redo;
            }
            if (tok == 0)
                goto redo;
        } else {
            next_nomacro();
            if (tok == 0) {
                /* end of macro string: free it */
                free(macro_ptr_allocated);
                macro_ptr = NULL;
                goto redo;
            }
        }
    }
#if defined(DEBUG)
    printf("token = %s\n", get_tok_str(tok, tokc));
#endif
}

void swap(int *p, int *q)
{
    int t;
    t = *p;
    *p = *q;
    *q = t;
}

void vsetc(int t, int r, CValue *vc)
{
    if (vtop >= vstack + VSTACK_SIZE)
        error("memory full");
    /* cannot let cpu flags if other instruction are generated */
    /* XXX: VT_JMP test too ? */
    if ((vtop->r & VT_VALMASK) == VT_CMP)
        gv(RC_INT);
    vtop++;
    vtop->t = t;
    vtop->r = r;
    vtop->r2 = VT_CONST;
    vtop->c = *vc;
}

/* push integer constant */
void vpushi(int v)
{
    CValue cval;
    cval.i = v;
    vsetc(VT_INT, VT_CONST, &cval);
}

void vset(int t, int r, int v)
{
    CValue cval;

    cval.i = v;
    vsetc(t, r, &cval);
}

void vswap(void)
{
    SValue tmp;

    tmp = vtop[0];
    vtop[0] = vtop[-1];
    vtop[-1] = tmp;
}

void vpushv(SValue *v)
{
    if (vtop >= vstack + VSTACK_SIZE)
        error("memory full");
    vtop++;
    *vtop = *v;
}

void vdup(void)
{
    vpushv(vtop);
}

/* save r to the memory stack, and mark it as being free */
void save_reg(int r)
{
    int l, i, saved, t, size, align;
    SValue *p, sv;

    /* modify all stack values */
    saved = 0;
    l = 0;
    for(p=vstack;p<=vtop;p++) {
        i = p->r & VT_VALMASK;
        if ((p->r & VT_VALMASK) == r ||
            (p->r2 & VT_VALMASK) == r) {
            /* must save value on stack if not already done */
            if (!saved) {
                /* store register in the stack */
                t = p->t;
                if (!is_float(t) && (t & VT_BTYPE) != VT_LLONG)
                    t = VT_INT;
                size = type_size(t, &align);
                loc = (loc - size) & -align;
                sv.t = t;
                sv.r = VT_LOCAL | VT_LVAL;
                sv.c.ul = loc;
                store(r, &sv);
                /* XXX: x86 specific: need to pop fp register ST0 if saved */
                if (r == REG_ST0) {
                    o(0xd9dd); /* fstp %st(1) */
                }
                /* special long long case */
                if ((p->t & VT_BTYPE) == VT_LLONG) {
                    sv.c.ul += 4;
                    store(p->r2, &sv);
                }
                l = loc;
                saved = 1;
            }
            /* mark that stack entry as being saved on the stack */
            if (p->r & VT_LVAL)
                t = VT_LLOCAL;
            else
                t = VT_LOCAL;
            p->r = VT_LVAL | t;
            p->r2 = VT_CONST;
            p->c.ul = l;
        }
    }
}

/* find a free register of class 'rc'. If none, save one register */
int get_reg(int rc)
{
    int r;
    SValue *p;

    /* find a free register */
    for(r=0;r<NB_REGS;r++) {
        if (reg_classes[r] & rc) {
            for(p=vstack;p<=vtop;p++) {
                if ((p->r & VT_VALMASK) == r ||
                    (p->r2 & VT_VALMASK) == r)
                    goto notfound;
            }
            return r;
        }
    notfound: ;
    }
    
    /* no register left : free the first one on the stack (VERY
       IMPORTANT to start from the bottom to ensure that we don't
       spill registers used in gen_opi()) */
    for(p=vstack;p<=vtop;p++) {
        r = p->r & VT_VALMASK;
        if (r < VT_CONST && (reg_classes[r] & rc)) {
            save_reg(r);
            break;
        }
    }
    return r;
}

void save_regs(void)
{
    int r;
    SValue *p;

    for(p=vstack;p<=vtop;p++) {
        r = p->r & VT_VALMASK;
        if (r < VT_CONST) {
            save_reg(r);
        }
    }
}

/* move register 's' to 'r', and flush previous value of r to memory
   if needed */
void move_reg(int r, int s)
{
    SValue sv;

    if (r != s) {
        save_reg(r);
        sv.t = VT_INT;
        sv.r = s;
        sv.c.ul = 0;
        load(r, &sv);
    }
}

/* get address of vtop (vtop MUST BE an lvalue) */
void gaddrof(void)
{
    vtop->r &= ~VT_LVAL;
    /* tricky: if saved lvalue, then we can go back to lvalue */
    if ((vtop->r & VT_VALMASK) == VT_LLOCAL)
        vtop->r = (vtop->r & ~VT_VALMASK) | VT_LOCAL | VT_LVAL;
}

/* generate lvalue bound code */
void gbound(void)
{
    vtop->r &= ~VT_MUSTBOUND;
    /* if lvalue, then use checking code before dereferencing */
    if (vtop->r & VT_LVAL) {
        gaddrof();
        vpushi(0);
        gen_bounded_ptr_add1();
        gen_bounded_ptr_add2(1);
        vtop->r |= VT_LVAL;
    }
}

/* store vtop a register belonging to class 'rc'. lvalues are
   converted to values. Cannot be used if cannot be converted to
   register value (such as structures). */
int gv(int rc)
{
    int r, r2, rc2, bit_pos, bit_size, size, align, i, data_offset;
    unsigned long long ll;

    /* NOTE: get_reg can modify vstack[] */
    if (vtop->t & VT_BITFIELD) {
        bit_pos = (vtop->t >> VT_STRUCT_SHIFT) & 0x3f;
        bit_size = (vtop->t >> (VT_STRUCT_SHIFT + 6)) & 0x3f;
        /* remove bit field info to avoid loops */
        vtop->t &= ~(VT_BITFIELD | (-1 << VT_STRUCT_SHIFT));
        /* generate shifts */
        vpushi(32 - (bit_pos + bit_size));
        gen_op(TOK_SHL);
        vpushi(32 - bit_size);
        /* NOTE: transformed to SHR if unsigned */
        gen_op(TOK_SAR);
        r = gv(rc);
    } else {
        if (is_float(vtop->t) && 
            (vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
            /* CPUs usually cannot use float constants, so we store them
               generically in data segment */
            size = type_size(vtop->t, &align);
            data_offset = (int)data_section->data_ptr;
            data_offset = (data_offset + align - 1) & -align;
            /* XXX: not portable yet */
            size = size >> 2;
            for(i=0;i<size;i++)
                ((int *)data_offset)[i] = vtop->c.tab[i];
            vtop->r |= VT_LVAL;
            vtop->c.ul = data_offset;
            data_offset += size << 2;
            data_section->data_ptr = (unsigned char *)data_offset;
        }
        if (vtop->r & VT_MUSTBOUND) 
            gbound();

        r = vtop->r & VT_VALMASK;
        /* need to reload if:
           - constant
           - lvalue (need to dereference pointer)
           - already a register, but not in the right class */
        if (r >= VT_CONST || 
            (vtop->r & VT_LVAL) ||
            !(reg_classes[r] & rc) ||
            ((vtop->t & VT_BTYPE) == VT_LLONG && 
             !(reg_classes[vtop->r2] & rc))) {
            r = get_reg(rc);
            if ((vtop->t & VT_BTYPE) == VT_LLONG) {
                /* two register type load : expand to two words
                   temporarily */
                if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
                    /* load constant */
                    ll = vtop->c.ull;
                    vtop->c.ui = ll; /* first word */
                    load(r, vtop);
                    vtop->r = r; /* save register value */
                    vpushi(ll >> 32); /* second word */
                } else if (r >= VT_CONST || 
                           (vtop->r & VT_LVAL)) {
                    /* load from memory */
                    load(r, vtop);
                    vdup();
                    vtop[-1].r = r; /* save register value */
                    /* increment pointer to get second word */
                    vtop->t = VT_INT;
                    gaddrof();
                    vpushi(4);
                    gen_op('+');
                    vtop->r |= VT_LVAL;
                } else {
                    /* move registers */
                    load(r, vtop);
                    vdup();
                    vtop[-1].r = r; /* save register value */
                    vtop->r = vtop[-1].r2;
                }
                /* allocate second register */
                rc2 = RC_INT;
                if (rc == RC_IRET)
                    rc2 = RC_LRET;
                r2 = get_reg(rc2);
                load(r2, vtop);
                vpop();
                /* write second register */
                vtop->r2 = r2;
            } else {
                /* one register type load */
                load(r, vtop);
            }
        }
        vtop->r = r;
    }
    return r;
}

/* generate vtop[-1] and vtop[0] in resp. classes rc1 and rc2 */
void gv2(int rc1, int rc2)
{
    /* generate more generic register first */
    if (rc1 <= rc2) {
        vswap();
        gv(rc1);
        vswap();
        gv(rc2);
        /* test if reload is needed for first register */
        if ((vtop[-1].r & VT_VALMASK) >= VT_CONST) {
            vswap();
            gv(rc1);
            vswap();
        }
    } else {
        gv(rc2);
        vswap();
        gv(rc1);
        vswap();
        /* test if reload is needed for first register */
        if ((vtop[0].r & VT_VALMASK) >= VT_CONST) {
            gv(rc2);
        }
    }
}

/* expand long long on stack in two int registers */
void lexpand(void)
{
    int u;

    u = vtop->t & VT_UNSIGNED;
    gv(RC_INT);
    vdup();
    vtop[0].r = vtop[-1].r2;
    vtop[0].r2 = VT_CONST;
    vtop[-1].r2 = VT_CONST;
    vtop[0].t = VT_INT | u;
    vtop[-1].t = VT_INT | u;
}

/* build a long long from two ints */
void lbuild(int t)
{
    gv2(RC_INT, RC_INT);
    vtop[-1].r2 = vtop[0].r;
    vtop[-1].t = t;
    vpop();
}

/* rotate n first stack elements to the bottom */
void vrotb(int n)
{
    int i;
    SValue tmp;

    tmp = vtop[-n + 1];
    for(i=-n+1;i!=0;i++)
        vtop[i] = vtop[i+1];
    vtop[0] = tmp;
}

/* pop stack value */
void vpop(void)
{
    /* for x86, we need to pop the FP stack */
    if ((vtop->r & VT_VALMASK) == REG_ST0) {
        o(0xd9dd); /* fstp %st(1) */
    }
    vtop--;
}

/* convert stack entry to register and duplicate its value in another
   register */
void gv_dup(void)
{
    int rc, t, r, r1;
    SValue sv;

    t = vtop->t;
    if ((t & VT_BTYPE) == VT_LLONG) {
        lexpand();
        gv_dup();
        vswap();
        vrotb(3);
        gv_dup();
        vrotb(4);
        /* stack: H L L1 H1 */
        lbuild(t);
        vrotb(3);
        vrotb(3);
        vswap();
        lbuild(t);
        vswap();
    } else {
        /* duplicate value */
        rc = RC_INT;
        sv.t = VT_INT;
        if (is_float(t)) {
            rc = RC_FLOAT;
            sv.t = t;
        }
        r = gv(rc);
        r1 = get_reg(rc);
        sv.r = r;
        sv.c.ul = 0;
        load(r1, &sv); /* move r to r1 */
        vdup();
        /* duplicates value */
        vtop->r = r1;
    }
}

/* generate CPU independent (unsigned) long long operations */
void gen_opl(int op)
{
    int t, a, b, op1, c, i;
    void *func;
    GFuncContext gf;
    SValue tmp;

    switch(op) {
    case '/':
    case TOK_PDIV:
        func = __divll;
        goto gen_func;
    case TOK_UDIV:
        func = __divull;
        goto gen_func;
    case '%':
        func = __modll;
        goto gen_func;
    case TOK_UMOD:
        func = __modull;
    gen_func:
        /* call generic long long function */
        gfunc_start(&gf);
        gfunc_param(&gf);
        gfunc_param(&gf);
        vpushi((int)func);
        gfunc_call(&gf);
        vpushi(0);
        vtop->r = REG_IRET;
        vtop->r2 = REG_LRET;
        break;
    case '^':
    case '&':
    case '|':
    case '*':
    case '+':
    case '-':
        t = vtop->t;
        vswap();
        lexpand();
        vrotb(3);
        lexpand();
        /* stack: L1 H1 L2 H2 */
        tmp = vtop[0];
        vtop[0] = vtop[-3];
        vtop[-3] = tmp;
        tmp = vtop[-2];
        vtop[-2] = vtop[-3];
        vtop[-3] = tmp;
        vswap();
        /* stack: H1 H2 L1 L2 */
        if (op == '*') {
            vpushv(vtop - 1);
            vpushv(vtop - 1);
            gen_op(TOK_UMULL);
            lexpand();
            /* stack: H1 H2 L1 L2 ML MH */
            for(i=0;i<4;i++)
                vrotb(6);
            /* stack: ML MH H1 H2 L1 L2 */
            tmp = vtop[0];
            vtop[0] = vtop[-2];
            vtop[-2] = tmp;
            /* stack: ML MH H1 L2 H2 L1 */
            gen_op('*');
            vrotb(3);
            vrotb(3);
            gen_op('*');
            /* stack: ML MH M1 M2 */
            gen_op('+');
            gen_op('+');
        } else if (op == '+' || op == '-') {
            /* XXX: add non carry method too (for MIPS or alpha) */
            if (op == '+')
                op1 = TOK_ADDC1;
            else
                op1 = TOK_SUBC1;
            gen_op(op1);
            /* stack: H1 H2 (L1 op L2) */
            vrotb(3);
            vrotb(3);
            gen_op(op1 + 1); /* TOK_xxxC2 */
        } else {
            gen_op(op);
            /* stack: H1 H2 (L1 op L2) */
            vrotb(3);
            vrotb(3);
            /* stack: (L1 op L2) H1 H2 */
            gen_op(op);
            /* stack: (L1 op L2) (H1 op H2) */
        }
        /* stack: L H */
        lbuild(t);
        break;
    case TOK_SAR:
    case TOK_SHR:
    case TOK_SHL:
        if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_FORWARD)) == VT_CONST) {
            t = vtop[-1].t;
            vswap();
            lexpand();
            vrotb(3);
        /* stack: L H shift */
            c = (int)vtop->c.i;
            /* constant: simpler */
            /* NOTE: all comments are for SHL. the other cases are
               done by swaping words */
            vpop();
            if (op != TOK_SHL)
                vswap();
            if (c >= 32) {
                /* stack: L H */
                vpop();
                if (c > 32) {
                    vpushi(c - 32);
                    gen_op(op);
                }
                if (op != TOK_SAR) {
                    vpushi(0);
                } else {
                    gv_dup();
                    vpushi(31);
                    gen_op(TOK_SAR);
                }
                vswap();
            } else {
                vswap();
                gv_dup();
                /* stack: H L L */
                vpushi(c);
                gen_op(op);
                vswap();
                vpushi(32 - c);
                if (op == TOK_SHL)
                    gen_op(TOK_SHR);
                else
                    gen_op(TOK_SHL);
                vrotb(3);
                /* stack: L L H */
                vpushi(c);
                gen_op(op);
                gen_op('|');
            }
            if (op != TOK_SHL)
                vswap();
            lbuild(t);
        } else {
            /* XXX: should provide a faster fallback on x86 ? */
            switch(op) {
            case TOK_SAR:
                func = __sardi3;
                goto gen_func;
            case TOK_SHR:
                func = __shrdi3;
                goto gen_func;
            case TOK_SHL:
                func = __shldi3;
                goto gen_func;
            }
        }
        break;
    default:
        /* compare operations */
        t = vtop->t;
        vswap();
        lexpand();
        vrotb(3);
        lexpand();
        /* stack: L1 H1 L2 H2 */
        tmp = vtop[-1];
        vtop[-1] = vtop[-2];
        vtop[-2] = tmp;
        /* stack: L1 L2 H1 H2 */
        /* compare high */
        op1 = op;
        /* when values are equal, we need to compare low words. since
           the jump is inverted, we invert the test too. */
        if (op1 == TOK_LT)
            op1 = TOK_LE;
        else if (op1 == TOK_GT)
            op1 = TOK_GE;
        else if (op1 == TOK_ULT)
            op1 = TOK_ULE;
        else if (op1 == TOK_UGT)
            op1 = TOK_UGE;
        a = 0;
        b = 0;
        gen_op(op1);
        if (op1 != TOK_NE) {
            a = gtst(1, 0);
        }
        if (op != TOK_EQ) {
            /* generate non equal test */
            /* XXX: NOT PORTABLE yet */
            if (a == 0)
                b = gtst(0, 0);
            else
                b = psym(0x850f, 0);
        }
        /* compare low */
        gen_op(op);
        a = gtst(1, a);
        gsym(b);
        vset(VT_INT, VT_JMPI, a);
        break;
    }
}

/* handle constant optimizations and various machine independant opt */
void gen_opc(int op)
{
    int fc, c1, c2, n;
    SValue *v1, *v2;

    v1 = vtop - 1;
    v2 = vtop;
    /* currently, we cannot do computations with forward symbols */
    c1 = (v1->r & (VT_VALMASK | VT_LVAL | VT_FORWARD)) == VT_CONST;
    c2 = (v2->r & (VT_VALMASK | VT_LVAL | VT_FORWARD)) == VT_CONST;
    if (c1 && c2) {
        fc = v2->c.i;
        switch(op) {
        case '+': v1->c.i += fc; break;
        case '-': v1->c.i -= fc; break;
        case '&': v1->c.i &= fc; break;
        case '^': v1->c.i ^= fc; break;
        case '|': v1->c.i |= fc; break;
        case '*': v1->c.i *= fc; break;

        case TOK_PDIV:
        case '/':
        case '%':
        case TOK_UDIV:
        case TOK_UMOD:
            /* if division by zero, generate explicit division */
            if (fc == 0) {
                if (const_wanted)
                    error("division by zero in constant");
                goto general_case;
            }
            switch(op) {
            default: v1->c.i /= fc; break;
            case '%': v1->c.i %= fc; break;
            case TOK_UDIV: v1->c.i = (unsigned)v1->c.i / fc; break;
            case TOK_UMOD: v1->c.i = (unsigned)v1->c.i % fc; break;
            }
            break;
        case TOK_SHL: v1->c.i <<= fc; break;
        case TOK_SHR: v1->c.i = (unsigned)v1->c.i >> fc; break;
        case TOK_SAR: v1->c.i >>= fc; break;
            /* tests */
        case TOK_ULT: v1->c.i = (unsigned)v1->c.i < (unsigned)fc; break;
        case TOK_UGE: v1->c.i = (unsigned)v1->c.i >= (unsigned)fc; break;
        case TOK_EQ: v1->c.i = v1->c.i == fc; break;
        case TOK_NE: v1->c.i = v1->c.i != fc; break;
        case TOK_ULE: v1->c.i = (unsigned)v1->c.i <= (unsigned)fc; break;
        case TOK_UGT: v1->c.i = (unsigned)v1->c.i > (unsigned)fc; break;
        case TOK_LT: v1->c.i = v1->c.i < fc; break;
        case TOK_GE: v1->c.i = v1->c.i >= fc; break;
        case TOK_LE: v1->c.i = v1->c.i <= fc; break;
        case TOK_GT: v1->c.i = v1->c.i > fc; break;
            /* logical */
        case TOK_LAND: v1->c.i = v1->c.i && fc; break;
        case TOK_LOR: v1->c.i = v1->c.i || fc; break;
        default:
            goto general_case;
        }
        vtop--;
    } else {
        /* if commutative ops, put c2 as constant */
        if (c1 && (op == '+' || op == '&' || op == '^' || 
                   op == '|' || op == '*')) {
            vswap();
            swap(&c1, &c2);
        }
        fc = vtop->c.i;
        if (c2 && (((op == '*' || op == '/' || op == TOK_UDIV || 
                     op == TOK_PDIV) && 
                    fc == 1) ||
                   ((op == '+' || op == '-' || op == '|' || op == '^' || 
                     op == TOK_SHL || op == TOK_SHR || op == TOK_SAR) && 
                    fc == 0) ||
                   (op == '&' && 
                    fc == -1))) {
            /* nothing to do */
            vtop--;
        } else if (c2 && (op == '*' || op == TOK_PDIV || op == TOK_UDIV)) {
            /* try to use shifts instead of muls or divs */
            if (fc > 0 && (fc & (fc - 1)) == 0) {
                n = -1;
                while (fc) {
                    fc >>= 1;
                    n++;
                }
                vtop->c.i = n;
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
            /* call low level op generator */
            gen_opi(op);
        }
    }
}

int pointed_size(int t)
{
    return type_size(pointed_type(t), &t);
}

#if 0
void check_pointer_types(SValue *p1, SValue *p2)
{
    char buf1[256], buf2[256];
    int t1, t2;
    t1 = p1->t;
    t2 = p2->t;
    if (!is_compatible_types(t1, t2)) {
        type_to_str(buf1, sizeof(buf1), t1, NULL);
        type_to_str(buf2, sizeof(buf2), t2, NULL);
        error("incompatible pointers '%s' and '%s'", buf1, buf2);
    }
}
#endif

/* generic gen_op: handles types problems */
void gen_op(int op)
{
    int u, t1, t2, bt1, bt2, t;

    t1 = vtop[-1].t;
    t2 = vtop[0].t;
    bt1 = t1 & VT_BTYPE;
    bt2 = t2 & VT_BTYPE;
        
    if (bt1 == VT_PTR || bt2 == VT_PTR) {
        /* at least one operand is a pointer */
        /* relationnal op: must be both pointers */
        if (op >= TOK_ULT && op <= TOK_GT) {
            //            check_pointer_types(vtop, vtop - 1);
            /* pointers are handled are unsigned */
            t = VT_INT | VT_UNSIGNED;
            goto std_op;
        }
        /* if both pointers, then it must be the '-' op */
        if ((t1 & VT_BTYPE) == VT_PTR && 
            (t2 & VT_BTYPE) == VT_PTR) {
            if (op != '-')
                error("cannot use pointers here");
            //            check_pointer_types(vtop - 1, vtop);
            /* XXX: check that types are compatible */
            u = pointed_size(t1);
            gen_opc(op);
            /* set to integer type */
            vtop->t = VT_INT; 
            vpushi(u);
            gen_op(TOK_PDIV);
        } else {
            /* exactly one pointer : must be '+' or '-'. */
            if (op != '-' && op != '+')
                error("cannot use pointers here");
            /* Put pointer as first operand */
            if ((t2 & VT_BTYPE) == VT_PTR) {
                vswap();
                swap(&t1, &t2);
            }
            /* XXX: cast to int ? (long long case) */
            vpushi(pointed_size(vtop[-1].t));
            gen_op('*');
            if (do_bounds_check) {
                /* if bounded pointers, we generate a special code to
                   test bounds */
                if (op == '-') {
                    vpushi(0);
                    vswap();
                    gen_op('-');
                }
                gen_bounded_ptr_add1();
                gen_bounded_ptr_add2(0);
            } else {
                gen_opc(op);
            }
            /* put again type if gen_opc() swaped operands */
            vtop->t = t1;
        }
    } else if (is_float(bt1) || is_float(bt2)) {
        /* compute bigger type and do implicit casts */
        if (bt1 == VT_LDOUBLE || bt2 == VT_LDOUBLE) {
            t = VT_LDOUBLE;
        } else if (bt1 == VT_DOUBLE || bt2 == VT_DOUBLE) {
            t = VT_DOUBLE;
        } else {
            t = VT_FLOAT;
        }
        /* floats can only be used for a few operations */
        if (op != '+' && op != '-' && op != '*' && op != '/' &&
            (op < TOK_ULT || op > TOK_GT))
            error("invalid operands for binary operation");
        goto std_op;
    } else if (bt1 == VT_LLONG || bt2 == VT_LLONG) {
        /* cast to biggest op */
        t = VT_LLONG;
        /* convert to unsigned if it does not fit in a long long */
        if ((t1 & (VT_BTYPE | VT_UNSIGNED)) == (VT_LLONG | VT_UNSIGNED) ||
            (t2 & (VT_BTYPE | VT_UNSIGNED)) == (VT_LLONG | VT_UNSIGNED))
            t |= VT_UNSIGNED;
        goto std_op;
    } else {
        /* integer operations */
        t = VT_INT;
        /* convert to unsigned if it does not fit in an integer */
        if ((t1 & (VT_BTYPE | VT_UNSIGNED)) == (VT_INT | VT_UNSIGNED) ||
            (t2 & (VT_BTYPE | VT_UNSIGNED)) == (VT_INT | VT_UNSIGNED))
            t |= VT_UNSIGNED;
    std_op:
        /* XXX: currently, some unsigned operations are explicit, so
           we modify them here */
        if (t & VT_UNSIGNED) {
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
        vswap();
        gen_cast(t);
        vswap();
        /* special case for shifts and long long: we keep the shift as
           an integer */
        if (op == TOK_SHR || op == TOK_SAR || op == TOK_SHL)
            gen_cast(VT_INT);
        else
            gen_cast(t);
        if (is_float(t))
            gen_opf(op);
        else if ((t & VT_BTYPE) == VT_LLONG)
            gen_opl(op);
        else
            gen_opc(op);
        if (op >= TOK_ULT && op <= TOK_GT) {
            /* relationnal op: the result is an int */
            vtop->t = VT_INT;
        } else {
            vtop->t = t;
        }
    }
}

/* generic itof for unsigned long long case */
void gen_cvt_itof1(int t)
{
    GFuncContext gf;

    if ((vtop->t & (VT_BTYPE | VT_UNSIGNED)) == 
        (VT_LLONG | VT_UNSIGNED)) {

        gfunc_start(&gf);
        gfunc_param(&gf);
        if (t == VT_FLOAT)
            vpushi((int)&__ulltof);
        else if (t == VT_DOUBLE)
            vpushi((int)&__ulltod);
        else
            vpushi((int)&__ulltold);
        gfunc_call(&gf);
        vpushi(0);
        vtop->r = REG_FRET;
    } else {
        gen_cvt_itof(t);
    }
}

/* generic ftoi for unsigned long long case */
void gen_cvt_ftoi1(int t)
{
    GFuncContext gf;
    int st;

    if (t == (VT_LLONG | VT_UNSIGNED)) {
        /* not handled natively */
        gfunc_start(&gf);
        st = vtop->t & VT_BTYPE;
        gfunc_param(&gf);
        if (st == VT_FLOAT)
            vpushi((int)&__ftoull);
        else if (st == VT_DOUBLE)
            vpushi((int)&__dtoull);
        else
            vpushi((int)&__ldtoull);
        gfunc_call(&gf);
        vpushi(0);
        vtop->r = REG_IRET;
        vtop->r2 = REG_LRET;
    } else {
        gen_cvt_ftoi(t);
    }
}

/* force char or short cast */
void force_charshort_cast(int t)
{
    int bits, dbt;
    dbt = t & VT_BTYPE;
    /* XXX: add optimization if lvalue : just change type and offset */
    if (dbt == VT_BYTE)
        bits = 8;
    else
        bits = 16;
    if (t & VT_UNSIGNED) {
        vpushi((1 << bits) - 1);
        gen_op('&');
    } else {
        bits = 32 - bits;
        vpushi(bits);
        gen_op(TOK_SHL);
        vpushi(bits);
        gen_op(TOK_SAR);
    }
}

/* cast 'vtop' to 't' type */
void gen_cast(int t)
{
    int sbt, dbt, sf, df, c, st1, dt1;

    /* special delayed cast for char/short */
    /* XXX: in some cases (multiple cascaded casts), it may still
       be incorrect */
    if (vtop->r & VT_MUSTCAST) {
        vtop->r &= ~VT_MUSTCAST;
        force_charshort_cast(vtop->t);
    }

    dbt = t & VT_BTYPE;
    sbt = vtop->t & VT_BTYPE;

    if (sbt != dbt) {
        sf = is_float(sbt);
        df = is_float(dbt);
        c = (vtop->r & (VT_VALMASK | VT_LVAL | VT_FORWARD)) == VT_CONST;
        if (sf && df) {
            /* convert from fp to fp */
            if (c) {
                /* constant case: we can do it now */
                /* XXX: in ISOC, cannot do it if error in convert */
                if (dbt == VT_FLOAT && sbt == VT_DOUBLE) 
                    vtop->c.f = (float)vtop->c.d;
                else if (dbt == VT_FLOAT && sbt == VT_LDOUBLE) 
                    vtop->c.f = (float)vtop->c.ld;
                else if (dbt == VT_DOUBLE && sbt == VT_FLOAT) 
                    vtop->c.d = (double)vtop->c.f;
                else if (dbt == VT_DOUBLE && sbt == VT_LDOUBLE) 
                    vtop->c.d = (double)vtop->c.ld;
                else if (dbt == VT_LDOUBLE && sbt == VT_FLOAT) 
                    vtop->c.ld = (long double)vtop->c.f;
                else if (dbt == VT_LDOUBLE && sbt == VT_DOUBLE) 
                    vtop->c.ld = (long double)vtop->c.d;
            } else {
                /* non constant case: generate code */
                gen_cvt_ftof(dbt);
            }
        } else if (df) {
            /* convert int to fp */
            st1 = vtop->t & (VT_BTYPE | VT_UNSIGNED);
            if (c) {
                switch(st1) {
                case VT_LLONG | VT_UNSIGNED:
                case VT_LLONG:
                    /* XXX: add const cases for long long */
                    goto do_itof;
                case VT_INT | VT_UNSIGNED:
                    switch(dbt) {
                    case VT_FLOAT: vtop->c.f = (float)vtop->c.ui; break;
                    case VT_DOUBLE: vtop->c.d = (double)vtop->c.ui; break;
                    case VT_LDOUBLE: vtop->c.ld = (long double)vtop->c.ui; break;
                    }
                    break;
                default:
                    switch(dbt) {
                    case VT_FLOAT: vtop->c.f = (float)vtop->c.i; break;
                    case VT_DOUBLE: vtop->c.d = (double)vtop->c.i; break;
                    case VT_LDOUBLE: vtop->c.ld = (long double)vtop->c.i; break;
                    }
                    break;
                }
            } else {
            do_itof:
                gen_cvt_itof1(dbt);
            }
        } else if (sf) {
            /* convert fp to int */
            dt1 = t & (VT_BTYPE | VT_UNSIGNED);
            /* we handle char/short/etc... with generic code */
            if (dt1 != (VT_INT | VT_UNSIGNED) &&
                dt1 != (VT_LLONG | VT_UNSIGNED) &&
                dt1 != VT_LLONG)
                dt1 = VT_INT;
            if (c) {
                switch(dt1) {
                case VT_LLONG | VT_UNSIGNED:
                case VT_LLONG:
                    /* XXX: add const cases for long long */
                    goto do_ftoi;
                case VT_INT | VT_UNSIGNED:
                    switch(sbt) {
                    case VT_FLOAT: vtop->c.ui = (unsigned int)vtop->c.d; break;
                    case VT_DOUBLE: vtop->c.ui = (unsigned int)vtop->c.d; break;
                    case VT_LDOUBLE: vtop->c.ui = (unsigned int)vtop->c.d; break;
                    }
                    break;
                default:
                    /* int case */
                    switch(sbt) {
                    case VT_FLOAT: vtop->c.i = (int)vtop->c.d; break;
                    case VT_DOUBLE: vtop->c.i = (int)vtop->c.d; break;
                    case VT_LDOUBLE: vtop->c.i = (int)vtop->c.d; break;
                    }
                    break;
                }
            } else {
            do_ftoi:
                gen_cvt_ftoi1(dt1);
            }
            if (dt1 == VT_INT && (t & (VT_BTYPE | VT_UNSIGNED)) != dt1) {
                /* additionnal cast for char/short/bool... */
                vtop->t = dt1;
                gen_cast(t);
            }
        } else if (dbt == VT_LLONG) {
            /* scalar to long long */
            if (c) {
                if ((vtop->t & (VT_BTYPE | VT_UNSIGNED)) == (VT_INT | VT_UNSIGNED))
                    vtop->c.ll = vtop->c.ui;
                else
                    vtop->c.ll = vtop->c.i;
            } else {
                /* machine independant conversion */
                gv(RC_INT);
                /* generate high word */
                if ((vtop->t & (VT_BTYPE | VT_UNSIGNED)) == (VT_INT | VT_UNSIGNED)) {
                    vpushi(0);
                    gv(RC_INT);
                } else {
                    gv_dup();
                    vpushi(31);
                    gen_op(TOK_SAR);
                }
                /* patch second register */
                vtop[-1].r2 = vtop->r;
                vpop();
            }
        } else if (dbt == VT_BOOL) {
            /* scalar to bool */
            vpushi(0);
            gen_op(TOK_NE);
        } else if (dbt == VT_BYTE || dbt == VT_SHORT) {
            force_charshort_cast(t);
        } else if (dbt == VT_INT) {
            /* scalar to int */
            if (sbt == VT_LLONG) {
                /* from long long: just take low order word */
                lexpand();
                vpop();
            } else if (vtop->r & VT_LVAL) {
                /* if lvalue and single word type, nothing to do (XXX:
                   maybe incorrect for sizeof op) */
                goto no_cast;
            }
        }
    }
    vtop->t = t;
 no_cast: ;
}

/* return type size. Put alignment at 'a' */
int type_size(int t, int *a)
{
    Sym *s;
    int bt;

    bt = t & VT_BTYPE;
    if (bt == VT_STRUCT) {
        /* struct/union */
        s = sym_find(((unsigned)t >> VT_STRUCT_SHIFT) | SYM_STRUCT);
        *a = 4; /* XXX: cannot store it yet. Doing that is safe */
        return s->c;
    } else if (bt == VT_PTR) {
        if (t & VT_ARRAY) {
            s = sym_find(((unsigned)t >> VT_STRUCT_SHIFT));
            return type_size(s->t, a) * s->c;
        } else {
            *a = PTR_SIZE;
            return PTR_SIZE;
        }
    } else if (bt == VT_LDOUBLE) {
        *a = LDOUBLE_ALIGN;
        return LDOUBLE_SIZE;
    } else if (bt == VT_DOUBLE || bt == VT_LLONG) {
        *a = 8;
        return 8;
    } else if (bt == VT_INT || bt == VT_ENUM || bt == VT_FLOAT) {
        *a = 4;
        return 4;
    } else if (bt == VT_SHORT) {
        *a = 2;
        return 2;
    } else {
        /* char, void, function, _Bool */
        *a = 1;
        return 1;
    }
}

/* return the pointed type of t */
int pointed_type(int t)
{
    Sym *s;
    s = sym_find(((unsigned)t >> VT_STRUCT_SHIFT));
    return s->t | (t & ~VT_TYPE);
}

int mk_pointer(int t)
{
    int p;
    p = anon_sym++;
    sym_push(p, t, 0, -1);
    return VT_PTR | (p << VT_STRUCT_SHIFT) | (t & ~VT_TYPE);
}

int is_compatible_types(int t1, int t2)
{
    Sym *s1, *s2;
    int bt1, bt2;

    t1 &= VT_TYPE;
    t2 &= VT_TYPE;
    bt1 = t1 & VT_BTYPE;
    bt2 = t2 & VT_BTYPE;
    if (bt1 == VT_PTR) {
        t1 = pointed_type(t1);
        /* if function, then convert implicitely to function pointer */
        if (bt2 != VT_FUNC) {
            if (bt2 != VT_PTR)
                return 0;
            t2 = pointed_type(t2);
        }
        /* void matches everything */
        t1 &= VT_TYPE;
        t2 &= VT_TYPE;
        if (t1 == VT_VOID || t2 == VT_VOID)
            return 1;
        return is_compatible_types(t1, t2);
    } else if (bt1 == VT_STRUCT) {
        return (t2 == t1);
    } else if (bt1 == VT_FUNC) {
        if (bt2 != VT_FUNC)
            return 0;
        s1 = sym_find(((unsigned)t1 >> VT_STRUCT_SHIFT));
        s2 = sym_find(((unsigned)t2 >> VT_STRUCT_SHIFT));
        if (!is_compatible_types(s1->t, s2->t))
            return 0;
        /* XXX: not complete */
        if (s1->c == FUNC_OLD || s2->c == FUNC_OLD)
            return 1;
        if (s1->c != s2->c)
            return 0;
        while (s1 != NULL) {
            if (s2 == NULL)
                return 0;
            if (!is_compatible_types(s1->t, s2->t))
                return 0;
            s1 = s1->next;
            s2 = s2->next;
        }
        if (s2)
            return 0;
        return 1;
    } else {
        /* XXX: not complete */
        return 1;
    }
}

/* print a type. If 'varstr' is not NULL, then the variable is also
   printed in the type */
/* XXX: union */
/* XXX: add array and function pointers */
void type_to_str(char *buf, int buf_size, 
                 int t, const char *varstr)
{
    int bt, v;
    Sym *s, *sa;
    char buf1[256];
    const char *tstr;

    t = t & VT_TYPE;
    bt = t & VT_BTYPE;
    buf[0] = '\0';
    if (t & VT_UNSIGNED)
        pstrcat(buf, buf_size, "unsigned ");
    switch(bt) {
    case VT_VOID:
        tstr = "void";
        goto add_tstr;
    case VT_BOOL:
        tstr = "_Bool";
        goto add_tstr;
    case VT_BYTE:
        tstr = "char";
        goto add_tstr;
    case VT_SHORT:
        tstr = "short";
        goto add_tstr;
    case VT_INT:
        tstr = "int";
        goto add_tstr;
    case VT_LONG:
        tstr = "long";
        goto add_tstr;
    case VT_LLONG:
        tstr = "long long";
        goto add_tstr;
    case VT_FLOAT:
        tstr = "float";
        goto add_tstr;
    case VT_DOUBLE:
        tstr = "double";
        goto add_tstr;
    case VT_LDOUBLE:
        tstr = "long double";
    add_tstr:
        pstrcat(buf, buf_size, tstr);
        break;
    case VT_ENUM:
    case VT_STRUCT:
        if (bt == VT_STRUCT)
            tstr = "struct ";
        else
            tstr = "enum ";
        pstrcat(buf, buf_size, tstr);
        v = (unsigned)t >> VT_STRUCT_SHIFT;
        if (v >= SYM_FIRST_ANOM)
            pstrcat(buf, buf_size, "<anonymous>");
        else
            pstrcat(buf, buf_size, get_tok_str(v, NULL));
        break;
    case VT_FUNC:
        s = sym_find((unsigned)t >> VT_STRUCT_SHIFT);
        type_to_str(buf, buf_size, s->t, varstr);
        pstrcat(buf, buf_size, "(");
        sa = s->next;
        while (sa != NULL) {
            type_to_str(buf1, sizeof(buf1), sa->t, NULL);
            pstrcat(buf, buf_size, buf1);
            sa = sa->next;
            if (sa)
                pstrcat(buf, buf_size, ", ");
        }
        pstrcat(buf, buf_size, ")");
        goto no_var;
    case VT_PTR:
        s = sym_find((unsigned)t >> VT_STRUCT_SHIFT);
        pstrcpy(buf1, sizeof(buf1), "*");
        if (varstr)
            pstrcat(buf1, sizeof(buf1), varstr);
        type_to_str(buf, buf_size, s->t, buf1);
        goto no_var;
    }
    if (varstr) {
        pstrcat(buf, buf_size, " ");
        pstrcat(buf, buf_size, varstr);
    }
 no_var: ;
}

/* verify type compatibility to store vtop in 'dt' type, and generate
   casts if needed. */
void gen_assign_cast(int dt)
{
    int st;
    char buf1[256], buf2[256];

    st = vtop->t; /* source type */
    if ((dt & VT_BTYPE) == VT_PTR) {
        /* special cases for pointers */
        /* a function is implicitely a function pointer */
        if ((st & VT_BTYPE) == VT_FUNC) {
            if (!is_compatible_types(pointed_type(dt), st))
                goto error;
            else
                goto type_ok;
        }
        /* '0' can also be a pointer */
        if ((st & VT_BTYPE) == VT_INT &&
            ((vtop->r & (VT_VALMASK | VT_LVAL | VT_FORWARD)) == VT_CONST) &&
            vtop->c.i == 0)
            goto type_ok;
    }
    if (!is_compatible_types(dt, st)) {
    error:
        type_to_str(buf1, sizeof(buf1), st, NULL);
        type_to_str(buf2, sizeof(buf2), dt, NULL);
        error("cannot cast '%s' to '%s'", buf1, buf2);
    }
 type_ok:
    gen_cast(dt);
}

/* store vtop in lvalue pushed on stack */
void vstore(void)
{
    int sbt, dbt, ft, r, t, size, align, bit_size, bit_pos, rc, delayed_cast;
    GFuncContext gf;

    ft = vtop[-1].t;
    sbt = vtop->t & VT_BTYPE;
    dbt = ft & VT_BTYPE;
    if (((sbt == VT_INT || sbt == VT_SHORT) && dbt == VT_BYTE) ||
        (sbt == VT_INT && dbt == VT_SHORT)) {
        /* optimize char/short casts */
        delayed_cast = VT_MUSTCAST;
        vtop->t = ft & VT_TYPE;
    } else {
        delayed_cast = 0;
        gen_assign_cast(ft & VT_TYPE);
    }

    if (sbt == VT_STRUCT) {
        /* if structure, only generate pointer */
        /* structure assignment : generate memcpy */
        /* XXX: optimize if small size */

        vdup();
        gfunc_start(&gf);
        /* type size */
        size = type_size(vtop->t, &align);
        vpushi(size);
        gfunc_param(&gf);
        /* source */
        vtop->t = VT_INT;
        gaddrof();
        gfunc_param(&gf);
        /* destination */
        vswap();
        vtop->t = VT_INT;
        gaddrof();
        gfunc_param(&gf);

        save_regs();
        vpushi((int)&memcpy);
        gfunc_call(&gf);
        /* leave source on stack */
    } else if (ft & VT_BITFIELD) {
        /* bitfield store handling */
        bit_pos = (ft >> VT_STRUCT_SHIFT) & 0x3f;
        bit_size = (ft >> (VT_STRUCT_SHIFT + 6)) & 0x3f;
        /* remove bit field info to avoid loops */
        vtop[-1].t = ft & ~(VT_BITFIELD | (-1 << VT_STRUCT_SHIFT));

        /* duplicate destination */
        vdup();
        vtop[-1] = vtop[-2];

        /* mask and shift source */
        vpushi((1 << bit_size) - 1);
        gen_op('&');
        vpushi(bit_pos);
        gen_op(TOK_SHL);
        /* load destination, mask and or with source */
        vswap();
        vpushi(~(((1 << bit_size) - 1) << bit_pos));
        gen_op('&');
        gen_op('|');
        /* store result */
        vstore();
    } else {
        /* bound check case */
        if (vtop[-1].r & VT_MUSTBOUND) {
            vswap();
            gbound();
            vswap();
        }
        rc = RC_INT;
        if (is_float(ft))
            rc = RC_FLOAT;
        r = gv(rc);  /* generate value */
        /* if lvalue was saved on stack, must read it */
        if ((vtop[-1].r & VT_VALMASK) == VT_LLOCAL) {
            SValue sv;
            t = get_reg(RC_INT);
            sv.t = VT_INT;
            sv.r = VT_LOCAL | VT_LVAL;
            sv.c.ul = vtop[-1].c.ul;
            load(t, &sv);
            vtop[-1].r = t | VT_LVAL;
        }
        store(r, vtop - 1);
        /* two word case handling : store second register at word + 4 */
        if ((ft & VT_BTYPE) == VT_LLONG) {
            vswap();
            /* convert to int to increment easily */
            vtop->t = VT_INT;
            gaddrof();
            vpushi(4);
            gen_op('+');
            vtop->r |= VT_LVAL;
            vswap();
            /* XXX: it works because r2 is spilled last ! */
            store(vtop->r2, vtop - 1);
        }
        vswap();
        vtop--; /* NOT vpop() because on x86 it would flush the fp stack */
        vtop->r |= delayed_cast;
    }
}

/* post defines POST/PRE add. c is the token ++ or -- */
void inc(int post, int c)
{
    test_lvalue();
    vdup(); /* save lvalue */
    if (post) {
        gv_dup(); /* duplicate value */
        vrotb(3);
        vrotb(3);
    }
    /* add constant */
    vpushi(c - TOK_MID); 
    gen_op('+');
    vstore(); /* store value */
    if (post)
        vpop(); /* if post op, return saved value */
}

/* Parse GNUC __attribute__ extension. Currently, the following
   extensions are recognized:
   - aligned(n) : set data/function alignment.
   - section(x) : generate data/code in this section.
   - unused : currently ignored, but may be used someday.
 */
void parse_attribute(AttributeDef *ad)
{
    int t, n;

    next();
    skip('(');
    skip('(');
    while (tok != ')') {
        if (tok < TOK_IDENT)
            expect("attribute name");
        t = tok;
        next();
        switch(t) {
        case TOK_SECTION:
        case TOK___SECTION__:
            skip('(');
            if (tok != TOK_STR)
                expect("section name");
            ad->section = find_section(tokc.ts->str);
            next();
            skip(')');
            break;
        case TOK_ALIGNED:
        case TOK___ALIGNED__:
            skip('(');
            n = expr_const();
            if (n <= 0 || (n & (n - 1)) != 0) 
                error("alignment must be a positive power of two");
            ad->aligned = n;
            skip(')');
            break;
        case TOK_UNUSED:
        case TOK___UNUSED__:
            /* currently, no need to handle it because tcc does not
               track unused objects */
            break;
        default:
            warning("'%s' attribute ignored", get_tok_str(t, NULL));
            /* skip parameters */
            /* XXX: skip parenthesis too */
            if (tok == '(') {
                next();
                while (tok != ')' && tok != -1)
                    next();
                next();
            }
            break;
        }
        if (tok != ',')
            break;
        next();
    }
    skip(')');
    skip(')');
}

/* enum/struct/union declaration */
int struct_decl(int u)
{
    int a, t, b, v, size, align, maxalign, c, offset;
    int bit_size, bit_pos, bsize, bt, lbit_pos;
    Sym *s, *ss, **ps;
    AttributeDef ad;

    a = tok; /* save decl type */
    next();
    if (tok != '{') {
        v = tok;
        next();
        /* struct already defined ? return it */
        /* XXX: check consistency */
        s = sym_find(v | SYM_STRUCT);
        if (s) {
            if (s->t != a)
                error("invalid type");
            goto do_decl;
        }
    } else {
        v = anon_sym++;
    }
    s = sym_push(v | SYM_STRUCT, a, 0, 0);
    /* put struct/union/enum name in type */
 do_decl:
    u = u | (v << VT_STRUCT_SHIFT);
    
    if (tok == '{') {
        next();
        if (s->c)
            error("struct/union/enum already defined");
        /* cannot be empty */
        c = 0;
        maxalign = 0;
        ps = &s->next;
        bit_pos = 0;
        offset = 0;
        while (1) {
            if (a == TOK_ENUM) {
                v = tok;
                next();
                if (tok == '=') {
                    next();
                    c = expr_const();
                }
                /* enum symbols have static storage */
                sym_push(v, VT_STATIC | VT_INT, VT_CONST, c);
                if (tok == ',')
                    next();
                c++;
            } else {
                parse_btype(&b, &ad);
                while (1) {
                    bit_size = -1;
                    v = 0;
                    if (tok != ':') {
                        t = type_decl(&v, b, TYPE_DIRECT);
                        if ((t & VT_BTYPE) == VT_FUNC ||
                            (t & (VT_TYPEDEF | VT_STATIC | VT_EXTERN)))
                            error("invalid type for '%s'", 
                                  get_tok_str(v, NULL));
                    } else {
                        t = b;
                    }
                    if (tok == ':') {
                        next();
                        bit_size = expr_const();
                        /* XXX: handle v = 0 case for messages */
                        if (bit_size < 0)
                            error("negative width in bit-field '%s'", 
                                  get_tok_str(v, NULL));
                        if (v && bit_size == 0)
                            error("zero width for bit-field '%s'", 
                                  get_tok_str(v, NULL));
                    }
                    size = type_size(t, &align);
                    lbit_pos = 0;
                    if (bit_size >= 0) {
                        bt = t & VT_BTYPE;
                        if (bt != VT_INT && 
                            bt != VT_BYTE && 
                            bt != VT_SHORT)
                            error("bitfields must have scalar type");
                        bsize = size * 8;
                        if (bit_size > bsize) {
                            error("width of '%s' exceeds its type",
                                  get_tok_str(v, NULL));
                        } else if (bit_size == bsize) {
                            /* no need for bit fields */
                            bit_pos = 0;
                        } else if (bit_size == 0) {
                            /* XXX: what to do if only padding in a
                               structure ? */
                            /* zero size: means to pad */
                            if (bit_pos > 0)
                                bit_pos = bsize;
                        } else {
                            /* we do not have enough room ? */
                            if ((bit_pos + bit_size) > bsize)
                                bit_pos = 0;
                            lbit_pos = bit_pos;
                            /* XXX: handle LSB first */
                            t |= VT_BITFIELD | 
                                (bit_pos << VT_STRUCT_SHIFT) |
                                (bit_size << (VT_STRUCT_SHIFT + 6));
                            bit_pos += bit_size;
                        }
                    } else {
                        bit_pos = 0;
                    }
                    if (v) {
                        /* add new memory data only if starting
                           bit field */
                        if (lbit_pos == 0) {
                            if (a == TOK_STRUCT) {
                                c = (c + align - 1) & -align;
                                offset = c;
                                c += size;
                            } else {
                                offset = 0;
                                if (size > c)
                                    c = size;
                            }
                            if (align > maxalign)
                                maxalign = align;
                        }
#if 0
                        printf("add field %s offset=%d", 
                               get_tok_str(v, NULL), offset);
                        if (t & VT_BITFIELD) {
                            printf(" pos=%d size=%d", 
                                   (t >> VT_STRUCT_SHIFT) & 0x3f,
                                   (t >> (VT_STRUCT_SHIFT + 6)) & 0x3f);
                        }
                        printf("\n");
#endif
                        ss = sym_push(v | SYM_FIELD, t, 0, offset);
                        *ps = ss;
                        ps = &ss->next;
                    }
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
        /* size for struct/union, dummy for enum */
        s->c = (c + maxalign - 1) & -maxalign; 
    }
    return u;
}

/* return 0 if no type declaration. otherwise, return the basic type
   and skip it. 
 */
int parse_btype(int *type_ptr, AttributeDef *ad)
{
    int t, u, type_found;
    Sym *s;

    memset(ad, 0, sizeof(AttributeDef));
    type_found = 0;
    t = 0;
    while(1) {
        switch(tok) {
            /* basic types */
        case TOK_CHAR:
            u = VT_BYTE;
        basic_type:
            next();
        basic_type1:
            if ((t & VT_BTYPE) != 0)
                error("too many basic types");
            t |= u;
            break;
        case TOK_VOID:
            u = VT_VOID;
            goto basic_type;
        case TOK_SHORT:
            u = VT_SHORT;
            goto basic_type;
        case TOK_INT:
            next();
            break;
        case TOK_LONG:
            next();
            if ((t & VT_BTYPE) == VT_DOUBLE) {
                t = (t & ~VT_BTYPE) | VT_LDOUBLE;
            } else if ((t & VT_BTYPE) == VT_LONG) {
                t = (t & ~VT_BTYPE) | VT_LLONG;
            } else {
                u = VT_LONG;
                goto basic_type1;
            }
            break;
        case TOK_BOOL:
            u = VT_BOOL;
            goto basic_type;
        case TOK_FLOAT:
            u = VT_FLOAT;
            goto basic_type;
        case TOK_DOUBLE:
            next();
            if ((t & VT_BTYPE) == VT_LONG) {
                t = (t & ~VT_BTYPE) | VT_LDOUBLE;
            } else {
                u = VT_DOUBLE;
                goto basic_type1;
            }
            break;
        case TOK_ENUM:
            u = struct_decl(VT_ENUM);
            goto basic_type1;
        case TOK_STRUCT:
        case TOK_UNION:
            u = struct_decl(VT_STRUCT);
            goto basic_type1;

            /* type modifiers */
        case TOK_CONST:
        case TOK_VOLATILE:
        case TOK_REGISTER:
        case TOK_SIGNED:
        case TOK_AUTO:
        case TOK_INLINE:
        case TOK_RESTRICT:
            next();
            break;
        case TOK_UNSIGNED:
            t |= VT_UNSIGNED;
            next();
            break;

            /* storage */
        case TOK_EXTERN:
            t |= VT_EXTERN;
            next();
            break;
        case TOK_STATIC:
            t |= VT_STATIC;
            next();
            break;
        case TOK_TYPEDEF:
            t |= VT_TYPEDEF;
            next();
            break;
            /* GNUC attribute */
        case TOK___ATTRIBUTE__:
            parse_attribute(ad);
            break;
        default:
            s = sym_find(tok);
            if (!s || !(s->t & VT_TYPEDEF))
                goto the_end;
            t |= (s->t & ~VT_TYPEDEF);
            next();
            break;
        }
        type_found = 1;
    }
the_end:
    /* long is never used as type */
    if ((t & VT_BTYPE) == VT_LONG)
        t = (t & ~VT_BTYPE) | VT_INT;
    *type_ptr = t;
    return type_found;
}

int post_type(int t)
{
    int p, n, pt, l, t1;
    Sym **plast, *s, *first;
    AttributeDef ad;

    if (tok == '(') {
        /* function declaration */
        next();
        l = 0;
        first = NULL;
        plast = &first;
        while (tok != ')') {
            /* read param name and compute offset */
            if (l != FUNC_OLD) {
                if (!parse_btype(&pt, &ad)) {
                    if (l) {
                        error("invalid type");
                    } else {
                        l = FUNC_OLD;
                        goto old_proto;
                    }
                }
                l = FUNC_NEW;
                if ((pt & VT_BTYPE) == VT_VOID && tok == ')')
                    break;
                pt = type_decl(&n, pt, TYPE_DIRECT | TYPE_ABSTRACT);
                if ((pt & VT_BTYPE) == VT_VOID)
                    error("parameter declared as void");
            } else {
            old_proto:
                n = tok;
                pt = VT_INT;
                next();
            }
            /* array must be transformed to pointer according to ANSI C */
            pt &= ~VT_ARRAY;
            s = sym_push(n | SYM_FIELD, pt, 0, 0);
            *plast = s;
            plast = &s->next;
            if (tok == ',') {
                next();
                if (l == FUNC_NEW && tok == TOK_DOTS) {
                    l = FUNC_ELLIPSIS;
                    next();
                    break;
                }
            }
        }
        /* if no parameters, then old type prototype */
        if (l == 0)
            l = FUNC_OLD;
        skip(')');
        t1 = t & (VT_TYPEDEF | VT_STATIC | VT_EXTERN);
        t = post_type(t & ~(VT_TYPEDEF | VT_STATIC | VT_EXTERN));
        /* we push a anonymous symbol which will contain the function prototype */
        p = anon_sym++;
        s = sym_push(p, t, 0, l);
        s->next = first;
        t = t1 | VT_FUNC | (p << VT_STRUCT_SHIFT);
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
        t1 = t & (VT_TYPEDEF | VT_STATIC | VT_EXTERN);
        t = post_type(t & ~(VT_TYPEDEF | VT_STATIC | VT_EXTERN));
        
        /* we push a anonymous symbol which will contain the array
           element type */
        p = anon_sym++;
        sym_push(p, t, 0, n);
        t = t1 | VT_ARRAY | VT_PTR | (p << VT_STRUCT_SHIFT);
    }
    return t;
}

/* Read a type declaration (except basic type), and return the
   type. If v is true, then also put variable name in 'vtop->c' */
int type_decl(int *v, int t, int td)
{
    int u, p;
    Sym *s;

    while (tok == '*') {
        next();
        while (tok == TOK_CONST || tok == TOK_VOLATILE || tok == TOK_RESTRICT)
            next();
        t = mk_pointer(t);
    }
    
    /* recursive type */
    /* XXX: incorrect if abstract type for functions (e.g. 'int ()') */
    if (tok == '(') {
        next();
        u = type_decl(v, 0, td);
        skip(')');
    } else {
        u = 0;
        /* type identifier */
        if (tok >= TOK_IDENT && (td & TYPE_DIRECT)) {
            *v = tok;
            next();
        } else {
            if (!(td & TYPE_ABSTRACT))
                expect("identifier");
            *v = 0;
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
Sym *external_sym(int v, int u, int r)
{
    Sym *s;
    s = sym_find(v);
    if (!s) {
        /* push forward reference */
        s = sym_push1(&global_stack, 
                      v, u, 0);
        s->r = r | VT_CONST | VT_FORWARD;
    }
    return s;
}

void indir(void)
{
    if ((vtop->t & VT_BTYPE) != VT_PTR)
        expect("pointer");
    if (vtop->r & VT_LVAL)
        gv(RC_INT);
    vtop->t = pointed_type(vtop->t);
    /* an array is never an lvalue */
    if (!(vtop->t & VT_ARRAY)) {
        vtop->r |= VT_LVAL;
        /* if bound checking, the referenced pointer must be checked */
        if (do_bounds_check) 
            vtop->r |= VT_MUSTBOUND;
    }
}

/* pass a parameter to a function and do type checking and casting */
void gfunc_param_typed(GFuncContext *gf, Sym *func, Sym *arg)
{
    int func_type;
    func_type = func->c;
    if (func_type == FUNC_OLD ||
        (func_type == FUNC_ELLIPSIS && arg == NULL)) {
        /* default casting : only need to convert float to double */
        if ((vtop->t & VT_BTYPE) == VT_FLOAT)
            gen_cast(VT_DOUBLE);
    } else if (arg == NULL) {
        error("too many arguments to function");
    } else {
        gen_assign_cast(arg->t);
    }
    gfunc_param(gf);
}

void unary(void)
{
    int n, t, ft, fc, p, align, size, r, data_offset;
    Sym *s;
    GFuncContext gf;
    AttributeDef ad;

    if (tok == TOK_CINT || tok == TOK_CCHAR || tok == TOK_LCHAR) {
        vpushi(tokc.i);
        next();
    } else if (tok == TOK_CUINT) {
        vsetc(VT_INT | VT_UNSIGNED, VT_CONST, &tokc);
        next();
    } else if (tok == TOK_CLLONG) {
        vsetc(VT_LLONG, VT_CONST, &tokc);
        next();
    } else if (tok == TOK_CULLONG) {
        vsetc(VT_LLONG | VT_UNSIGNED, VT_CONST, &tokc);
        next();
    } else if (tok == TOK_CFLOAT) {
        vsetc(VT_FLOAT, VT_CONST, &tokc);
        next();
    } else if (tok == TOK_CDOUBLE) {
        vsetc(VT_DOUBLE, VT_CONST, &tokc);
        next();
    } else if (tok == TOK_CLDOUBLE) {
        vsetc(VT_LDOUBLE, VT_CONST, &tokc);
        next();
    } else if (tok == TOK___FUNC__) {
        /* special function name identifier */
        /* generate (char *) type */
        data_offset = (int)data_section->data_ptr;
        vset(mk_pointer(VT_BYTE), VT_CONST, data_offset);
        strcpy((void *)data_offset, funcname);
        data_offset += strlen(funcname) + 1;
        data_section->data_ptr = (unsigned char *)data_offset;
        next();
    } else if (tok == TOK_LSTR) {
        t = VT_INT;
        goto str_init;
    } else if (tok == TOK_STR) {
        /* string parsing */
        t = VT_BYTE;
    str_init:
        type_size(t, &align);
        data_offset = (int)data_section->data_ptr;
        data_offset = (data_offset + align - 1) & -align;
        fc = data_offset;
        /* we must declare it as an array first to use initializer parser */
        t = VT_ARRAY | mk_pointer(t);
        decl_initializer(t, VT_CONST, data_offset, 1, 0);
        data_offset += type_size(t, &align);
        /* put it as pointer */
        vset(t & ~VT_ARRAY, VT_CONST, fc);
        data_section->data_ptr = (unsigned char *)data_offset;
    } else {
        t = tok;
        next();
        if (t == '(') {
            /* cast ? */
            if (parse_btype(&t, &ad)) {
                ft = type_decl(&n, t, TYPE_ABSTRACT);
                skip(')');
                /* check ISOC99 compound literal */
                if (tok == '{') {
                    /* data is allocated locally by default */
                    if (global_expr)
                        r = VT_CONST;
                    else
                        r = VT_LOCAL;
                    /* all except arrays are lvalues */
                    if (!(ft & VT_ARRAY))
                        r |= VT_LVAL;
                    memset(&ad, 0, sizeof(AttributeDef));
                    fc = decl_initializer_alloc(ft, &ad, r, 1);
                    vset(ft, r, fc);
                } else {
                    unary();
                    gen_cast(ft);
                }
            } else {
                gexpr();
                skip(')');
            }
        } else if (t == '*') {
            unary();
            indir();
        } else if (t == '&') {
            unary();
            /* functions names must be treated as function pointers,
               except for unary '&' and sizeof. Since we consider that
               functions are not lvalues, we only have to handle it
               there and in function calls. */
            if ((vtop->t & VT_BTYPE) != VT_FUNC)
                test_lvalue();
            vtop->t = mk_pointer(vtop->t);
            gaddrof();
        } else
        if (t == '!') {
            unary();
            if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST) 
                vtop->c.i = !vtop->c.i;
            else if ((vtop->r & VT_VALMASK) == VT_CMP)
                vtop->c.i = vtop->c.i ^ 1;
            else
                vset(VT_INT, VT_JMP, gtst(1, 0));
        } else
        if (t == '~') {
            unary();
            vpushi(-1);
            gen_op('^');
        } else 
        if (t == '+') {
            unary();
        } else 
        if (t == TOK_SIZEOF) {
            if (tok == '(') {
                next();
                if (parse_btype(&t, &ad)) {
                    t = type_decl(&n, t, TYPE_ABSTRACT);
                } else {
                    /* XXX: some code could be generated: add eval
                       flag */
                    gexpr();
                    t = vtop->t;
                    vpop();
                }
                skip(')');
            } else {
                unary();
                t = vtop->t;
                vpop();
            }
            vpushi(type_size(t, &t));
        } else
        if (t == TOK_INC || t == TOK_DEC) {
            unary();
            inc(0, t);
        } else if (t == '-') {
            vpushi(0);
            unary();
            gen_op('-');
        } else 
        {
            s = sym_find(t);
            if (!s) {
                if (tok != '(')
                    error("'%s' undeclared", get_tok_str(t, NULL));
                /* for simple function calls, we tolerate undeclared
                   external reference */
                p = anon_sym++;        
                sym_push1(&global_stack, p, 0, FUNC_OLD);
                /* int() function */
                s = external_sym(t, VT_FUNC | (p << VT_STRUCT_SHIFT), 0); 
            }
            vset(s->t, s->r, s->c);
            /* if forward reference, we must point to s */
            if (vtop->r & VT_FORWARD)
                vtop->c.sym = s;
        }
    }
    
    /* post operations */
    while (1) {
        if (tok == TOK_INC || tok == TOK_DEC) {
            inc(1, tok);
            next();
        } else if (tok == '.' || tok == TOK_ARROW) {
            /* field */ 
            if (tok == TOK_ARROW) 
                indir();
            test_lvalue();
            gaddrof();
            next();
            /* expect pointer on structure */
            if ((vtop->t & VT_BTYPE) != VT_STRUCT)
                expect("struct or union");
            s = sym_find(((unsigned)vtop->t >> VT_STRUCT_SHIFT) | SYM_STRUCT);
            /* find field */
            tok |= SYM_FIELD;
            while ((s = s->next) != NULL) {
                if (s->v == tok)
                    break;
            }
            if (!s)
                error("field not found");
            /* add field offset to pointer */
            vtop->t = char_pointer_type; /* change type to 'char *' */
            vpushi(s->c);
            gen_op('+');
            /* change type to field type, and set to lvalue */
            vtop->t = s->t;
            /* an array is never an lvalue */
            if (!(vtop->t & VT_ARRAY))
                vtop->r |= VT_LVAL;
            next();
        } else if (tok == '[') {
            next();
            gexpr();
            gen_op('+');
            indir();
            skip(']');
        } else if (tok == '(') {
            SValue ret;
            Sym *sa;

            /* function call  */
            if ((vtop->t & VT_BTYPE) != VT_FUNC) {
                /* pointer test (no array accepted) */
                if ((vtop->t & (VT_BTYPE | VT_ARRAY)) == VT_PTR) {
                    vtop->t = pointed_type(vtop->t);
                    if ((vtop->t & VT_BTYPE) != VT_FUNC)
                        goto error_func;
                } else {
                error_func:
                    expect("function pointer");
                }
            } else {
                vtop->r &= ~VT_LVAL; /* no lvalue */
            }
            /* get return type */
            s = sym_find((unsigned)vtop->t >> VT_STRUCT_SHIFT);
            save_regs(); /* save used temporary registers */
            gfunc_start(&gf);
            next();
            sa = s->next; /* first parameter */
#ifdef INVERT_FUNC_PARAMS
            {
                int *str, len, parlevel, *saved_macro_ptr;
                Sym *args, *s1;

                /* read each argument and store it on a stack */
                /* XXX: merge it with macro args ? */
                args = NULL;
                while (tok != ')') {
                    len = 0;
                    str = NULL;
                    parlevel = 0;
                    while ((parlevel > 0 || (tok != ')' && tok != ',')) && 
                           tok != -1) {
                        if (tok == '(')
                            parlevel++;
                        else if (tok == ')')
                            parlevel--;
                        tok_add2(&str, &len, tok, &tokc);
                        next();
                    }
                    tok_add(&str, &len, -1); /* end of file added */
                    tok_add(&str, &len, 0);
                    s1 = sym_push2(&args, 0, 0, (int)str);
                    s1->next = sa; /* add reference to argument */
                    if (sa)
                        sa = sa->next;
                    if (tok != ',')
                        break;
                    next();
                }
                if (tok != ')')
                    expect(")");
                
                /* now generate code in reverse order by reading the stack */
                saved_macro_ptr = macro_ptr;
                while (args) {
                    macro_ptr = (int *)args->c;
                    next();
                    expr_eq();
                    if (tok != -1)
                        expect("',' or ')'");
                    gfunc_param_typed(&gf, s, args->next);
                    s1 = args->prev;
                    free((int *)args->c);
                    free(args);
                    args = s1;
                }
                macro_ptr = saved_macro_ptr;
                /* restore token */
                tok = ')';
            }
#endif
            /* compute first implicit argument if a structure is returned */
            if ((s->t & VT_BTYPE) == VT_STRUCT) {
                /* get some space for the returned structure */
                size = type_size(s->t, &align);
                loc = (loc - size) & -align;
                ret.t = s->t;
                ret.r = VT_LOCAL | VT_LVAL;
                /* pass it as 'int' to avoid structure arg passing
                   problems */
                vset(VT_INT, VT_LOCAL, loc);
                ret.c = vtop->c;
                gfunc_param(&gf);
            } else {
                ret.t = s->t; 
                ret.r2 = VT_CONST;
                /* return in register */
                if (is_float(ret.t)) {
                    ret.r = REG_FRET; 
                } else {
                    if ((ret.t & VT_BTYPE) == VT_LLONG)
                        ret.r2 = REG_LRET;
                    ret.r = REG_IRET;
                }
                ret.c.i = 0;
            }
#ifndef INVERT_FUNC_PARAMS
            while (tok != ')') {
                expr_eq();
                gfunc_param_typed(&gf, s, sa);
                if (sa)
                    sa = sa->next;
                if (tok == ',')
                    next();
            }
#endif
            if (sa)
                error("too few arguments to function");
            skip(')');
            gfunc_call(&gf);
            /* return value */
            vsetc(ret.t, ret.r, &ret.c);
            vtop->r2 = ret.r2;
        } else {
            break;
        }
    }
}

void uneq(void)
{
    int t;
    
    unary();
    if (tok == '=' ||
        (tok >= TOK_A_MOD && tok <= TOK_A_DIV) ||
        tok == TOK_A_XOR || tok == TOK_A_OR ||
        tok == TOK_A_SHL || tok == TOK_A_SAR) {
        test_lvalue();
        t = tok;
        next();
        if (t == '=') {
            expr_eq();
        } else {
            vdup();
            expr_eq();
            gen_op(t & 0x7f);
        }
        vstore();
    }
}

void sum(int l)
{
    int t;

    if (l == 0)
        uneq();
    else {
        sum(--l);
        while ((l == 0 && (tok == '*' || tok == '/' || tok == '%')) ||
               (l == 1 && (tok == '+' || tok == '-')) ||
               (l == 2 && (tok == TOK_SHL || tok == TOK_SAR)) ||
               (l == 3 && ((tok >= TOK_ULE && tok <= TOK_GT) ||
                          tok == TOK_ULT || tok == TOK_UGE)) ||
               (l == 4 && (tok == TOK_EQ || tok == TOK_NE)) ||
               (l == 5 && tok == '&') ||
               (l == 6 && tok == '^') ||
               (l == 7 && tok == '|') ||
               (l == 8 && tok == TOK_LAND) ||
               (l == 9 && tok == TOK_LOR)) {
            t = tok;
            next();
            sum(l);
            gen_op(t);
       }
    }
}

/* only used if non constant */
void eand(void)
{
    int t;

    sum(8);
    t = 0;
    while (1) {
        if (tok != TOK_LAND) {
            if (t) {
                t = gtst(1, t);
                vset(VT_INT, VT_JMPI, t);
            }
            break;
        }
        t = gtst(1, t);
        next();
        sum(8);
    }
}

void eor(void)
{
    int t;

    eand();
    t = 0;
    while (1) {
        if (tok != TOK_LOR) {
            if (t) {
                t = gtst(0, t);
                vset(VT_INT, VT_JMP, t);
            }
            break;
        }
        t = gtst(0, t);
        next();
        eand();
    }
}

/* XXX: better constant handling */
void expr_eq(void)
{
    int t, u, c, r1, r2, rc;

    if (const_wanted) {
        sum(10);
        if (tok == '?') {
            c = vtop->c.i;
            vpop();
            next();
            gexpr();
            t = vtop->c.i;
            vpop();
            skip(':');
            expr_eq();
            if (c)
                vtop->c.i = t;
        }
    } else {
        eor();
        if (tok == '?') {
            next();
            t = gtst(1, 0);
            gexpr();
            /* XXX: long long handling ? */
            rc = RC_INT;
            if (is_float(vtop->t))
                rc = RC_FLOAT;
            r1 = gv(rc);
            vpop();
            skip(':');
            u = gjmp(0);

            gsym(t);
            expr_eq();
            r2 = gv(rc);
            move_reg(r1, r2);
            vtop->r = r1;
            gsym(u);
        }
    }
}

void gexpr(void)
{
    while (1) {
        expr_eq();
        if (tok != ',')
            break;
        vpop();
        next();
    }
}

/* parse a constant expression and return value in vtop */
void expr_const1(void)
{
    int a;
    a = const_wanted;
    const_wanted = 1;
    expr_eq();
    if ((vtop->r & (VT_VALMASK | VT_LVAL)) != VT_CONST)
        expect("constant");
    const_wanted = a;
}

/* parse an integer constant and return its value */
int expr_const(void)
{
    int c;
    expr_const1();
    c = vtop->c.i;
    vpop();
    return c;
}

/* return the label token if current token is a label, otherwise
   return zero */
int is_label(void)
{
    int t;
    CValue c;

    /* fast test first */
    if (tok < TOK_UIDENT)
        return 0;
    /* no need to save tokc since we expect an identifier */
    t = tok;
    c = tokc;
    next();
    if (tok == ':') {
        next();
        return t;
    } else {
        /* XXX: may not work in all cases (macros ?) */
        tok1 = tok;
        tok1c = tokc;
        tok = t;
        tokc = c;
        return 0;
    }
}

void block(int *bsym, int *csym, int *case_sym, int *def_sym, int case_reg)
{
    int a, b, c, d;
    Sym *s;

    /* generate line number info */
    if (do_debug && 
        (last_line_num != line_num || last_ind != ind)) {
        put_stabn(N_SLINE, 0, line_num, ind - func_ind);
        last_ind = ind;
        last_line_num = line_num;
    }

    if (tok == TOK_IF) {
        /* if test */
        next();
        skip('(');
        gexpr();
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
        gexpr();
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
        s = local_stack.top;
        while (tok != '}') {
            decl(VT_LOCAL);
            if (tok != '}')
                block(bsym, csym, case_sym, def_sym, case_reg);
        }
        /* pop locally defined symbols */
        sym_pop(&local_stack, s);
        next();
    } else if (tok == TOK_RETURN) {
        next();
        if (tok != ';') {
            gexpr();
            gen_assign_cast(func_vt);
            if ((func_vt & VT_BTYPE) == VT_STRUCT) {
                /* if returning structure, must copy it to implicit
                   first pointer arg location */
                vset(mk_pointer(func_vt), VT_LOCAL | VT_LVAL, func_vc);
                indir();
                vswap();
                /* copy structure value to pointer */
                vstore();
            } else if (is_float(func_vt)) {
                gv(RC_FRET);
            } else {
                gv(RC_IRET);
            }
            vtop--; /* NOT vpop() because on x86 it would flush the fp stack */
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
    } else if (tok == TOK_FOR) {
        int e;
        next();
        skip('(');
        if (tok != ';') {
            gexpr();
            vpop();
        }
        skip(';');
        d = ind;
        c = ind;
        a = 0;
        b = 0;
        if (tok != ';') {
            gexpr();
            a = gtst(1, 0);
        }
        skip(';');
        if (tok != ')') {
            e = gjmp(0);
            c = ind;
            gexpr();
            vpop();
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
        gexpr();
        c = gtst(0, 0);
        gsym_addr(c, d);
        skip(')');
        gsym(a);
        skip(';');
    } else
    if (tok == TOK_SWITCH) {
        next();
        skip('(');
        gexpr();
        /* XXX: other types than integer */
        case_reg = gv(RC_INT);
        vpop();
        skip(')');
        a = 0;
        b = gjmp(0); /* jump to first case */
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
        int v1, v2;
        if (!case_sym)
            expect("switch");
        next();
        v1 = expr_const();
        v2 = v1;
        if (gnu_ext && tok == TOK_DOTS) {
            next();
            v2 = expr_const();
            if (v2 < v1)
                warning("empty case range");
        }
        /* since a case is like a label, we must skip it with a jmp */
        b = gjmp(0);
        gsym(*case_sym);
        vset(VT_INT, case_reg, 0);
        vpushi(v1);
        if (v1 == v2) {
            gen_op(TOK_EQ);
            *case_sym = gtst(1, 0);
        } else {
            gen_op(TOK_GE);
            *case_sym = gtst(1, 0);
            vset(VT_INT, case_reg, 0);
            vpushi(v2);
            gen_op(TOK_LE);
            *case_sym = gtst(1, *case_sym);
        }
        gsym(b);
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
        s = sym_find1(&label_stack, tok);
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
    } else {
        b = is_label();
        if (b) {
            /* label case */
            s = sym_find1(&label_stack, b);
            if (s) {
                if (!(s->t & VT_FORWARD))
                    error("multiple defined label");
                gsym(s->c);
                s->c = ind;
                s->t = 0;
            } else {
                sym_push1(&label_stack, b, 0, ind);
            }
            /* we accept this, but it is a mistake */
            if (tok == '}') 
                warning("deprecated use of label at end of compound statement");
            else
                block(bsym, csym, case_sym, def_sym, case_reg);
        } else {
            /* expression case */
            if (tok != ';') {
                gexpr();
                vpop();
            }
            skip(';');
        }
    }
}

/* t is the array or struct type. c is the array or struct
   address. cur_index/cur_field is the pointer to the current
   value. 'size_only' is true if only size info is needed (only used
   in arrays) */
void decl_designator(int t, int r, int c, 
                     int *cur_index, Sym **cur_field, 
                     int size_only)
{
    Sym *s, *f;
    int notfirst, index, align, l;

    notfirst = 0;
    if (gnu_ext && (l = is_label()) != 0)
        goto struct_field;

    while (tok == '[' || tok == '.') {
        if (tok == '[') {
            if (!(t & VT_ARRAY))
                expect("array type");
            s = sym_find(((unsigned)t >> VT_STRUCT_SHIFT));
            next();
            index = expr_const();
            if (index < 0 || (s->c >= 0 && index >= s->c))
                expect("invalid index");
            skip(']');
            if (!notfirst)
                *cur_index = index;
            t = pointed_type(t);
            c += index * type_size(t, &align);
        } else {
            next();
            l = tok;
            next();
        struct_field:
            if ((t & VT_BTYPE) != VT_STRUCT)
                expect("struct/union type");
            s = sym_find(((unsigned)t >> VT_STRUCT_SHIFT) | SYM_STRUCT);
            l |= SYM_FIELD;
            f = s->next;
            while (f) {
                if (f->v == l)
                    break;
                f = f->next;
            }
            if (!f)
                expect("field");
            if (!notfirst)
                *cur_field = f;
            t = f->t | (t & ~VT_TYPE);
            c += f->c;
        }
        notfirst = 1;
    }
    if (notfirst) {
        if (tok == '=') {
            next();
        } else {
            if (!gnu_ext)
                expect("=");
        }
    } else {
        if (t & VT_ARRAY) {
            index = *cur_index;
            t = pointed_type(t);
            c += index * type_size(t, &align);
        } else {
            f = *cur_field;
            if (!f)
                error("too many field init");
            t = f->t | (t & ~VT_TYPE);
            c += f->c;
        }
    }
    decl_initializer(t, r, c, 0, size_only);
}

#define EXPR_VAL   0
#define EXPR_CONST 1
#define EXPR_ANY   2

/* store a value or an expression directly in global data or in local array */
void init_putv(int t, int r, int c, 
               int v, int expr_type)
{
    int saved_global_expr, bt;

    switch(expr_type) {
    case EXPR_VAL:
        vpushi(v);
        break;
    case EXPR_CONST:
        /* compound literals must be allocated globally in this case */
        saved_global_expr = global_expr;
        global_expr = 1;
        expr_const1();
        global_expr = saved_global_expr;
        break;
    case EXPR_ANY:
        expr_eq();
        break;
    }
    
    if ((r & VT_VALMASK) == VT_CONST) {
        /* XXX: not portable */
        gen_assign_cast(t);
        bt = t & VT_BTYPE;
        switch(bt) {
        case VT_BYTE:
            *(char *)c = vtop->c.i;
            break;
        case VT_SHORT:
            *(short *)c = vtop->c.i;
            break;
        case VT_DOUBLE:
            *(double *)c = vtop->c.d;
            break;
        case VT_LDOUBLE:
            *(long double *)c = vtop->c.ld;
            break;
        case VT_LLONG:
            *(long long *)c = vtop->c.ll;
            break;
        default:
            *(int *)c = vtop->c.i;
            break;
        }
        vtop--;
    } else {
        vset(t, r, c);
        vswap();
        vstore();
        vpop();
    }
}

/* put zeros for variable based init */
void init_putz(int t, int r, int c, int size)
{
    GFuncContext gf;

    if ((r & VT_VALMASK) == VT_CONST) {
        /* nothing to do because globals are already set to zero */
    } else {
        gfunc_start(&gf);
        vpushi(size);
        gfunc_param(&gf);
        vpushi(0);
        gfunc_param(&gf);
        vset(VT_INT, VT_LOCAL, c);
        gfunc_param(&gf);
        vpushi((int)&memset);
        gfunc_call(&gf);
    }
}

/* 't' contains the type and storage info. c is the address of the
   object. 'first' is true if array '{' must be read (multi dimension
   implicit array init handling). 'size_only' is true if size only
   evaluation is wanted (only for arrays). */
void decl_initializer(int t, int r, int c, int first, int size_only)
{
    int index, array_length, n, no_oblock, nb, parlevel, i;
    int t1, size1, align1, expr_type;
    Sym *s, *f;
    TokenSym *ts;

    if (t & VT_ARRAY) {
        s = sym_find(((unsigned)t >> VT_STRUCT_SHIFT));
        n = s->c;
        array_length = 0;
        t1 = pointed_type(t);
        size1 = type_size(t1, &align1);

        no_oblock = 1;
        if ((first && tok != TOK_LSTR && tok != TOK_STR) || 
            tok == '{') {
            skip('{');
            no_oblock = 0;
        }

        /* only parse strings here if correct type (otherwise: handle
           them as ((w)char *) expressions */
        if ((tok == TOK_LSTR && 
             (t1 & VT_BTYPE) == VT_INT) ||
            (tok == TOK_STR &&
             (t1 & VT_BTYPE) == VT_BYTE)) {
            /* XXX: move multiple string parsing in parser ? */
            while (tok == TOK_STR || tok == TOK_LSTR) {
                ts = tokc.ts;
                /* compute maximum number of chars wanted */
                nb = ts->len;
                if (n >= 0 && nb > (n - array_length))
                    nb = n - array_length;
                if (!size_only) {
                    if (ts->len > nb)
                        warning("initializer-string for array is too long");
                    for(i=0;i<nb;i++) {
                        init_putv(t1, r, c + (array_length + i) * size1, 
                                  ts->str[i], EXPR_VAL);
                    }
                }
                array_length += nb;
                next();
            }
            /* only add trailing zero if enough storage (no
               warning in this case since it is standard) */
            if (n < 0 || array_length < n) {
                if (!size_only) {
                    init_putv(t1, r, c + (array_length * size1), 0, EXPR_VAL);
                }
                array_length++;
            }
        } else {
            index = 0;
            while (tok != '}') {
                decl_designator(t, r, c, &index, NULL, size_only);
                if (n >= 0 && index >= n)
                    error("index too large");
                /* must put zero in holes (note that doing it that way
                   ensures that it even works with designators) */
                if (!size_only && array_length < index) {
                    init_putz(t1, r, c + array_length * size1, 
                              (index - array_length) * size1);
                }
                index++;
                if (index > array_length)
                    array_length = index;
                /* special test for multi dimensional arrays (may not
                   be strictly correct if designators are used at the
                   same time) */
                if (index >= n && no_oblock)
                    break;
                if (tok == '}')
                    break;
                skip(',');
            }
        }
        if (!no_oblock)
            skip('}');
        /* put zeros at the end */
        if (!size_only && n >= 0 && array_length < n) {
            init_putz(t1, r, c + array_length * size1, 
                      (n - array_length) * size1);
        }
        /* patch type size if needed */
        if (n < 0)
            s->c = array_length;
    } else if ((t & VT_BTYPE) == VT_STRUCT && tok == '{') {
        /* XXX: union needs only one init */
        next();
        s = sym_find(((unsigned)t >> VT_STRUCT_SHIFT) | SYM_STRUCT);
        f = s->next;
        array_length = 0;
        index = 0;
        n = s->c;
        while (tok != '}') {
            decl_designator(t, r, c, NULL, &f, size_only);
            /* fill with zero between fields */
            index = f->c;
            if (!size_only && array_length < index) {
                init_putz(t, r, c + array_length, 
                          index - array_length);
            }
            index = index + type_size(f->t, &align1);
            if (index > array_length)
                array_length = index;
            if (tok == '}')
                break;
            skip(',');
            f = f->next;
        }
        /* put zeros at the end */
        if (!size_only && array_length < n) {
            init_putz(t, r, c + array_length, 
                      n - array_length);
        }
        skip('}');
    } else if (tok == '{') {
        next();
        decl_initializer(t, r, c, first, size_only);
        skip('}');
    } else if (size_only) {
        /* just skip expression */
        parlevel = 0;
        while ((parlevel > 0 || (tok != '}' && tok != ',')) && 
               tok != -1) {
            if (tok == '(')
                parlevel++;
            else if (tok == ')')
                parlevel--;
            next();
        }
    } else {
        /* currently, we always use constant expression for globals
           (may change for scripting case) */
        expr_type = EXPR_CONST;
        if ((r & VT_VALMASK) == VT_LOCAL)
            expr_type = EXPR_ANY;
        init_putv(t, r, c, 0, expr_type);
    }
}

/* parse an initializer for type 't' if 'has_init' is true, and
   allocate space in local or global data space ('r' is either
   VT_LOCAL or VT_CONST). The allocated address in returned */
int decl_initializer_alloc(int t, AttributeDef *ad, int r, int has_init)
{
    int size, align, addr, tok1, data_offset;
    int *init_str, init_len, level, *saved_macro_ptr;
    Section *sec;

    size = type_size(t, &align);
    /* If unknown size, we must evaluate it before
       evaluating initializers because
       initializers can generate global data too
       (e.g. string pointers or ISOC99 compound
       literals). It also simplifies local
       initializers handling */
    init_len = 0;
    init_str = NULL;
    saved_macro_ptr = NULL; /* avoid warning */
    tok1 = 0;
    if (size < 0) {
        if (!has_init) 
            error("unknown type size");
        /* get all init string */
        level = 0;
        while (level > 0 || (tok != ',' && tok != ';')) {
            if (tok < 0)
                error("unexpected end of file in initializer");
            tok_add2(&init_str, &init_len, tok, &tokc);
            if (tok == '{')
                level++;
            else if (tok == '}') {
                if (level == 0)
                    break;
                level--;
            }
            next();
        }
        tok1 = tok;
        tok_add(&init_str, &init_len, -1);
        tok_add(&init_str, &init_len, 0);
        
        /* compute size */
        saved_macro_ptr = macro_ptr;
        macro_ptr = init_str;
        next();
        decl_initializer(t, r, 0, 1, 1);
        /* prepare second initializer parsing */
        macro_ptr = init_str;
        next();
        
        /* if still unknown size, error */
        size = type_size(t, &align);
        if (size < 0) 
            error("unknown type size");
    }
    /* take into account specified alignment if bigger */
    if (ad->aligned > align)
        align = ad->aligned;
    if ((r & VT_VALMASK) == VT_LOCAL) {
        if (do_bounds_check && (t & VT_ARRAY)) 
            loc--;
        loc = (loc - size) & -align;
        addr = loc;
        /* handles bounds */
        /* XXX: currently, since we do only one pass, we cannot track
           '&' operators, so we add only arrays */
        if (do_bounds_check && (t & VT_ARRAY)) {
            int *bounds_ptr;
            /* add padding between regions */
            loc--;
            /* then add local bound info */
            bounds_ptr = (int *)lbounds_section->data_ptr;
            *bounds_ptr++ = addr;
            *bounds_ptr++ = size;
            lbounds_section->data_ptr = (unsigned char *)bounds_ptr;
        }
    } else {
        /* compute section */
        sec = ad->section;
        if (!sec) {
            if (has_init)
                sec = data_section;
            else
                sec = bss_section;
        }
        data_offset = (int)sec->data_ptr;
        data_offset = (data_offset + align - 1) & -align;
        addr = data_offset;
        /* very important to increment global
           pointer at this time because
           initializers themselves can create new
           initializers */
        data_offset += size;
        /* handles bounds */
        if (do_bounds_check) {
            int *bounds_ptr;
            /* first, we need to add at least one byte between each region */
            data_offset++;
            /* then add global bound info */
            bounds_ptr = (int *)bounds_section->data_ptr;
            *bounds_ptr++ = addr;
            *bounds_ptr++ = size;
            bounds_section->data_ptr = (unsigned char *)bounds_ptr;
        }
        sec->data_ptr = (unsigned char *)data_offset;
    }
    if (has_init) {
        decl_initializer(t, r, addr, 1, 0);
        /* restore parse state if needed */
        if (init_str) {
            free(init_str);
            macro_ptr = saved_macro_ptr;
            tok = tok1;
        }
    }
    return addr;
}

void put_func_debug(int t)
{
    int bind;
    char buf[512];

    if (t & VT_STATIC)
        bind = STB_LOCAL;
    else
        bind = STB_GLOBAL;
    put_elf_sym(symtab_section, ind, 0, 
                ELF32_ST_INFO(bind, STT_FUNC), 0, 
                cur_text_section->sh_num, funcname);
    /* stabs info */
    /* XXX: we put here a dummy type */
    snprintf(buf, sizeof(buf), "%s:%c1", 
             funcname, t & VT_STATIC ? 'f' : 'F');
    put_stabs(buf, N_FUN, 0, line_num, ind);
    func_ind = ind;
    last_ind = 0;
    last_line_num = 0;
}

/* 'l' is VT_LOCAL or VT_CONST to define default storage type */
void decl(int l)
{
    int t, b, v, addr, has_init, r;
    Sym *sym;
    AttributeDef ad;

    while (1) {
        if (!parse_btype(&b, &ad)) {
            /* skip redundant ';' */
            /* XXX: find more elegant solution */
            if (tok == ';') {
                next();
                continue;
            }
            /* special test for old K&R protos without explicit int
               type. Only accepted when defining global data */
            if (l == VT_LOCAL || tok < TOK_DEFINE)
                break;
            b = VT_INT;
        }
        if (((b & VT_BTYPE) == VT_ENUM ||
             (b & VT_BTYPE) == VT_STRUCT) && 
            tok == ';') {
            /* we accept no variable after */
            next();
            continue;
        }
        while (1) { /* iterate thru each declaration */
            t = type_decl(&v, b, TYPE_DIRECT);
            /* currently, we do not parse attribute in
               type_decl(). May change if needed */
            if (tok == TOK___ATTRIBUTE__)
                parse_attribute(&ad);
#if 0
            {
                char buf[500];
                type_to_str(buf, sizeof(buf), t, get_tok_str(v, NULL));
                printf("type = '%s'\n", buf);
            }
#endif
            if (tok == '{') {
                if (l == VT_LOCAL)
                    error("cannot use local functions");
                if (!(t & VT_FUNC))
                    expect("function definition");
                /* compute text section */
                cur_text_section = ad.section;
                if (!cur_text_section)
                    cur_text_section = text_section;
                ind = (int)cur_text_section->data_ptr;
                /* patch forward references */
                if ((sym = sym_find(v)) && (sym->r & VT_FORWARD)) {
                    greloc_patch(sym, ind);
                    sym->t = t;
                } else {
                    /* put function address */
                    sym = sym_push1(&global_stack, v, t, ind);
                }
                sym->r = VT_CONST;
                funcname = get_tok_str(v, NULL);
                /* put debug symbol */
                if (do_debug)
                    put_func_debug(t);
                /* push a dummy symbol to enable local sym storage */
                sym_push1(&local_stack, 0, 0, 0);
                gfunc_prolog(t);
                loc = 0;
                rsym = 0;
                block(NULL, NULL, NULL, NULL, 0);
                gsym(rsym);
                gfunc_epilog();
                cur_text_section->data_ptr = (unsigned char *)ind;
                sym_pop(&label_stack, NULL); /* reset label stack */
                sym_pop(&local_stack, NULL); /* reset local stack */
                /* end of function */
                if (do_debug) {
                    put_stabn(N_FUN, 0, 0, ind - func_ind);
                }
                funcname = ""; /* for safety */
                func_vt = VT_VOID; /* for safety */
                ind = 0; /* for safety */
                break;
            } else {
                if (b & VT_TYPEDEF) {
                    /* save typedefed type  */
                    /* XXX: test storage specifiers ? */
                    sym_push(v, t | VT_TYPEDEF, 0, 0);
                } else if ((t & VT_BTYPE) == VT_FUNC) {
                    /* external function definition */
                    external_sym(v, t, 0);
                } else {
                    /* not lvalue if array */
                    r = 0;
                    if (!(t & VT_ARRAY))
                        r |= VT_LVAL;
                    if (b & VT_EXTERN) {
                        /* external variable */
                        external_sym(v, t, r);
                    } else {
                        if (t & VT_STATIC)
                            r |= VT_CONST;
                        else
                            r |= l;
                        has_init = (tok == '=');
                        if (has_init)
                            next();
                        addr = decl_initializer_alloc(t, &ad, r, 
                                                      has_init);
                        if (l == VT_CONST) {
                            /* global scope: see if already defined */
                            sym = sym_find(v);
                            if (!sym)
                                goto do_def;
                            if (!is_compatible_types(sym->t, t))
                                error("incompatible types for redefinition of '%s'", 
                                      get_tok_str(v, NULL));
                            if (!(sym->r & VT_FORWARD))
                                error("redefinition of '%s'", get_tok_str(v, NULL));
                            greloc_patch(sym, addr);
                        } else {
                        do_def:
                            sym_push(v, t, r, addr);
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

/* put all global symbols in the extern stack and do all the
   resolving which can be done without using external symbols from DLLs */
/* XXX: could try to verify types, but would not to save them in
   extern_stack too */
void resolve_global_syms(void)
{
    Sym *s, *s1, *ext_sym;
    Reloc **p;

    s = global_stack.top;
    while (s != NULL) {
        s1 = s->prev;
        /* do not save static or typedefed symbols or types */
        if (!(s->t & (VT_STATIC | VT_TYPEDEF)) && 
            !(s->v & (SYM_FIELD | SYM_STRUCT)) &&
            (s->v < SYM_FIRST_ANOM)) {
            ext_sym = sym_find1(&extern_stack, s->v);
            if (!ext_sym) {
                /* if the symbol do not exist, we simply save it */
                ext_sym = sym_push1(&extern_stack, s->v, s->t, s->c);
                ext_sym->r = s->r;
            } else if (ext_sym->r & VT_FORWARD) {
                /* external symbol already exists, but only as forward
                   definition */
                if (!(s->r & VT_FORWARD)) {
                    /* s is not forward, so we can relocate all symbols */
                    greloc_patch(ext_sym, s->c);
                } else {
                    /* the two symbols are forward: merge them */
                    p = (Reloc **)&ext_sym->c;
                    while (*p != NULL)
                        p = &(*p)->next;
                    *p = (Reloc *)s->c;
                }
            } else {
                /* external symbol already exists and is defined :
                   patch all references to it */
                if (!(s->r & VT_FORWARD))
                    error("'%s' defined twice", get_tok_str(s->v, NULL));
                greloc_patch(s, ext_sym->c);
            }
        } 
        s = s1;
    }
}

/* compile a C file. Return non zero if errors. */
int tcc_compile_file(const char *filename1)
{
    Sym *define_start;
    char buf[512];
    
    line_num = 1;
    funcname = "";
    filename = (char *)filename1;

    file = fopen(filename, "r");
    if (!file)
        error("file '%s' not found", filename);
    include_stack_ptr = include_stack;
    ifdef_stack_ptr = ifdef_stack;

    vtop = vstack - 1;
    anon_sym = SYM_FIRST_ANOM; 

    /* file info: full path + filename */
    if (do_debug) {
        getcwd(buf, sizeof(buf));
        pstrcat(buf, sizeof(buf), "/");
        put_stabs(buf, N_SO, 0, 0, (unsigned long)text_section->data_ptr);
        put_stabs(filename, N_SO, 0, 0, (unsigned long)text_section->data_ptr);
    }
    /* define common 'char *' type because it is often used internally
       for arrays and struct dereference */
    char_pointer_type = mk_pointer(VT_BYTE);

    define_start = define_stack.top;
    inp();
    ch = '\n'; /* needed to parse correctly first preprocessor command */
    next();
    decl(VT_CONST);
    if (tok != -1)
        expect("declaration");
    fclose(file);

    /* end of translation unit info */
    if (do_debug) {
        put_stabn(N_SO, 0, 0, (unsigned long)text_section->data_ptr);
    }

    /* reset define stack, but leave -Dsymbols (may be incorrect if
       they are undefined) */
    sym_pop(&define_stack, define_start); 
    
    resolve_global_syms();
    
    sym_pop(&global_stack, NULL);
    
    return 0;
}

/* define a symbol. A value can also be provided with the '=' operator */
/* XXX: currently only handles integers and string defines. should use
   tcc parser, but would need a custom 'FILE *' */
void define_symbol(const char *sym)
{
    TokenSym *ts;
    int *str, len;
    CValue cval;
    const char *p;
    char buf[256];

    p = strchr(sym, '=');
    if (!p) {
        pstrcpy(buf, sizeof(buf), sym);
        p = "1";
    } else {
        len = p - sym;
        if (len > sizeof(buf) - 1)
            len = sizeof(buf) - 1;
        memcpy(buf, sym, len);
        buf[len] = '\0';
        p++;
    }
    
    ts = tok_alloc(buf, 0);
    str = NULL;
    len = 0;
    if (isnum(*p)) {
        /* integer case */
        cval.i = atoi(p);
        tok_add2(&str, &len, TOK_CINT, &cval);
    } else {
        /* string case */
        cval.ts = tok_alloc(p, 0);
        tok_add2(&str, &len, TOK_STR, &cval);
    }
    tok_add(&str, &len, 0);
    sym_push1(&define_stack, ts->tok, MACRO_OBJ, (int)str);
}

void undef_symbol(const char *sym)
{
    TokenSym *ts;
    Sym *s;
    printf("undef %s\n", sym);
    ts = tok_alloc(sym, 0);
    s = sym_find1(&define_stack, tok);
    /* undefine symbol by putting an invalid name */
    if (s)
        sym_undef(&define_stack, s);
}

/* open a dynamic library so that its symbol are available for
   compiled programs */
void open_dll(char *libname)
{
    char buf[1024];
    void *h;

    snprintf(buf, sizeof(buf), "lib%s.so", libname);
    h = dlopen(buf, RTLD_GLOBAL | RTLD_LAZY);
    if (!h)
        error((char *)dlerror());
}

static void *resolve_sym(const char *sym)
{
    void *ptr;
    if (do_bounds_check) {
        ptr = bound_resolve_sym(sym);
        if (ptr)
            return ptr;
    }
    return dlsym(NULL, sym);
}

void resolve_extern_syms(void)
{
    Sym *s, *s1;
    char *str;
    int addr;

    s = extern_stack.top;
    while (s != NULL) {
        s1 = s->prev;
        if (s->r & VT_FORWARD) {
            /* if there is at least one relocation to do, then find it
               and patch it */
            if (s->c) {
                str = get_tok_str(s->v, NULL);
                addr = (int)resolve_sym(str);
                if (!addr)
                    error("unresolved external reference '%s'", str);
                greloc_patch(s, addr);
            }
        }
        s = s1;
    }
}

static int put_elf_str(Section *s, const char *sym)
{
    int c, offset;
    offset = s->data_ptr - s->data;
    for(;;) {
        c = *sym++;
        *s->data_ptr++ = c;
        if (c == '\0')
            break;
    }
    return offset;
}

static void put_elf_sym(Section *s, 
                       unsigned long value, unsigned long size,
                       int info, int other, int shndx, const char *name)
{
    int name_offset;
    Elf32_Sym *sym;

    sym = (Elf32_Sym *)s->data_ptr;
    if (name)
        name_offset = put_elf_str(s->link, name);
    else
        name_offset = 0;
    sym->st_name = name_offset;
    sym->st_value = value;
    sym->st_size = size;
    sym->st_info = info;
    sym->st_other = other;
    sym->st_shndx = shndx;
    s->data_ptr += sizeof(Elf32_Sym);
}

/* put stab debug information */

typedef struct {
    unsigned long n_strx;         /* index into string table of name */
    unsigned char n_type;         /* type of symbol */
    unsigned char n_other;        /* misc info (usually empty) */
    unsigned short n_desc;        /* description field */
    unsigned long n_value;        /* value of symbol */
} Stab_Sym;

static void put_stabs(const char *str, int type, int other, int desc, int value)
{
    Stab_Sym *sym;

    sym = (Stab_Sym *)stab_section->data_ptr;
    if (str) {
        sym->n_strx = put_elf_str(stabstr_section, str);
    } else {
        sym->n_strx = 0;
    }
    sym->n_type = type;
    sym->n_other = other;
    sym->n_desc = desc;
    sym->n_value = value;

    stab_section->data_ptr += sizeof(Stab_Sym);
}

static void put_stabn(int type, int other, int desc, int value)
{
    put_stabs(NULL, type, other, desc, value);
}

static void put_stabd(int type, int other, int desc)
{
    put_stabs(NULL, type, other, desc, 0);
}

/* output an ELF file (currently, only for testing) */
/* XXX: generate dynamic reloc info + DLL tables */
/* XXX: generate startup code */
/* XXX: better program header generation */
/* XXX: handle realloc'ed sections (instead of mmaping them) */
void build_exe(char *filename)
{ 
    Elf32_Ehdr ehdr;
    FILE *f;
    int shnum, i, phnum, file_offset, offset, size, j;
    Section *sec, *strsec;
    Elf32_Shdr *shdr, *sh;
    Elf32_Phdr *phdr, *ph;

    memset(&ehdr, 0, sizeof(ehdr));

    /* we add a section for symbols */
    strsec = new_section(".shstrtab", SHT_STRTAB, 0);
    put_elf_str(strsec, "");
    
    /* count number of sections and compute number of program segments */
    shnum = 1; /* section index zero is reserved */
    phnum = 0;
    for(sec = first_section; sec != NULL; sec = sec->next) {
        shnum++;
        if (sec->sh_flags & SHF_ALLOC)
            phnum++;
    }
    /* allocate section headers */
    shdr = malloc(shnum * sizeof(Elf32_Shdr));
    if (!shdr)
        error("memory full");
    memset(shdr, 0, shnum * sizeof(Elf32_Shdr));
    /* allocate program segment headers */
    phdr = malloc(phnum * sizeof(Elf32_Phdr));
    if (!phdr)
        error("memory full");
    memset(phdr, 0, phnum * sizeof(Elf32_Phdr));
        
    /* XXX: find correct load order */
    file_offset = sizeof(Elf32_Ehdr) + phnum * sizeof(Elf32_Phdr);
    for(sec = first_section, i = 1; sec != NULL; sec = sec->next, i++) {
        sh = &shdr[i];
        sh->sh_name = put_elf_str(strsec, sec->name);
        sh->sh_type = sec->sh_type;
        sh->sh_flags = sec->sh_flags;
        sh->sh_entsize = sec->sh_entsize;
        if (sec->link)
            sh->sh_link = sec->link->sh_num;
        if (sh->sh_type == SHT_STRTAB) {
            sh->sh_addralign = 1;
        } else if (sh->sh_type == SHT_SYMTAB ||
                   (sh->sh_flags & SHF_ALLOC) == 0) {
            sh->sh_addralign = 4;
        } else {
            sh->sh_addr = (Elf32_Word)sec->data;
            sh->sh_addralign = 4096;
        }
        sh->sh_size = (Elf32_Word)sec->data_ptr - (Elf32_Word)sec->data;
        /* align to section start */
        file_offset = (file_offset + sh->sh_addralign - 1) & 
            ~(sh->sh_addralign - 1);
        sh->sh_offset = file_offset;
        file_offset += sh->sh_size;
    }
    /* build program headers (simplistic - not fully correct) */
    j = 0;
    for(i=1;i<shnum;i++) {
        sh = &shdr[i];
        if (sh->sh_type == SHT_PROGBITS &&
            (sh->sh_flags & SHF_ALLOC) != 0) {
            ph = &phdr[j++];
            ph->p_type = PT_LOAD;
            ph->p_offset = sh->sh_offset;
            ph->p_vaddr = sh->sh_addr;
            ph->p_paddr = ph->p_vaddr;
            ph->p_filesz = sh->sh_size;
            ph->p_memsz = sh->sh_size;
            ph->p_flags = PF_R;
            if (sh->sh_flags & SHF_WRITE)
                ph->p_flags |= PF_W;
            if (sh->sh_flags & SHF_EXECINSTR)
                ph->p_flags |= PF_X;
            ph->p_align = sh->sh_addralign;
        }
    }

    /* align to 4 */
    file_offset = (file_offset + 3) & -4;
    
    /* fill header */
    ehdr.e_ident[0] = ELFMAG0;
    ehdr.e_ident[1] = ELFMAG1;
    ehdr.e_ident[2] = ELFMAG2;
    ehdr.e_ident[3] = ELFMAG3;
    ehdr.e_ident[4] = ELFCLASS32;
    ehdr.e_ident[5] = ELFDATA2LSB;
    ehdr.e_ident[6] = EV_CURRENT;
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_386;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = 0; /* XXX: patch it */
    ehdr.e_phoff = sizeof(Elf32_Ehdr);
    ehdr.e_shoff = file_offset;
    ehdr.e_ehsize = sizeof(Elf32_Ehdr);
    ehdr.e_phentsize = sizeof(Elf32_Phdr);
    ehdr.e_phnum = phnum;
    ehdr.e_shentsize = sizeof(Elf32_Shdr);
    ehdr.e_shnum = shnum;
    ehdr.e_shstrndx = shnum - 1;
    
    /* write elf file */
    f = fopen(filename, "w");
    if (!f)
        error("could not write '%s'", filename);
    fwrite(&ehdr, 1, sizeof(Elf32_Ehdr), f);
    fwrite(phdr, 1, phnum * sizeof(Elf32_Phdr), f);
    offset = sizeof(Elf32_Ehdr) + phnum * sizeof(Elf32_Phdr);
    for(sec = first_section, i = 1; sec != NULL; sec = sec->next, i++) {
        sh = &shdr[i];
        while (offset < sh->sh_offset) {
            fputc(0, f);
            offset++;
        }
        size = sec->data_ptr - sec->data;
        fwrite(sec->data, 1, size, f);
        offset += size;
    }
    while (offset < ehdr.e_shoff) {
        fputc(0, f);
        offset++;
    }
    fwrite(shdr, 1, shnum * sizeof(Elf32_Shdr), f);
    fclose(f);
}

/* print the position in the source file of PC value 'pc' by reading
   the stabs debug information */
static void rt_printline(unsigned long wanted_pc)
{
    Stab_Sym *sym, *sym_end;
    char func_name[128];
    unsigned long func_addr, last_pc, pc;
    const char *incl_files[INCLUDE_STACK_SIZE];
    int incl_index, len, last_line_num, i;
    const char *str, *p;

    func_name[0] = '\0';
    func_addr = 0;
    incl_index = 0;
    last_pc = 0xffffffff;
    last_line_num = 1;
    sym = (Stab_Sym *)stab_section->data + 1;
    sym_end = (Stab_Sym *)stab_section->data_ptr;
    while (sym < sym_end) {
        switch(sym->n_type) {
            /* function start or end */
        case N_FUN:
            if (sym->n_strx == 0) {
                func_name[0] = '\0';
                func_addr = 0;
            } else {
                str = stabstr_section->data + sym->n_strx;
                p = strchr(str, ':');
                if (!p) {
                    pstrcpy(func_name, sizeof(func_name), str);
                } else {
                    len = p - str;
                    if (len > sizeof(func_name) - 1)
                        len = sizeof(func_name) - 1;
                    memcpy(func_name, str, len);
                    func_name[len] = '\0';
                }
                func_addr = sym->n_value;
            }
            break;
            /* line number info */
        case N_SLINE:
            pc = sym->n_value + func_addr;
            if (wanted_pc >= last_pc && wanted_pc < pc)
                goto found;
            last_pc = pc;
            last_line_num = sym->n_desc;
            break;
            /* include files */
        case N_BINCL:
            str = stabstr_section->data + sym->n_strx;
        add_incl:
            if (incl_index < INCLUDE_STACK_SIZE) {
                incl_files[incl_index++] = str;
            }
            break;
        case N_EINCL:
            if (incl_index > 1)
                incl_index--;
            break;
        case N_SO:
            if (sym->n_strx == 0) {
                incl_index = 0; /* end of translation unit */
            } else {
                str = stabstr_section->data + sym->n_strx;
                /* do not add path */
                len = strlen(str);
                if (len > 0 && str[len - 1] != '/')
                    goto add_incl;
            }
            break;
        }
        sym++;
    }
    /* did not find line number info: */
    fprintf(stderr, "(no debug info, pc=0x%08lx): ", wanted_pc);
    return;
 found:
    for(i = 0; i < incl_index - 1; i++)
        fprintf(stderr, "In file included from %s\n", 
                incl_files[i]);
    if (incl_index > 0) {
        fprintf(stderr, "%s:%d: ", 
                incl_files[incl_index - 1], last_line_num);
    }
    if (func_name[0] != '\0') {
        fprintf(stderr, "in function '%s()': ", func_name);
    }
}

/* emit a run time error at position 'pc' */
void rt_error(unsigned long pc, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    rt_printline(pc);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(255);
    va_end(ap);
}

/* signal handler for fatal errors */
static void sig_error(int signum, siginfo_t *siginf, void *puc)
{
    struct ucontext *uc = puc;
    unsigned long pc;

#ifdef __i386__
    pc = uc->uc_mcontext.gregs[14];
#else
#error please put the right sigcontext field    
#endif

    switch(signum) {
    case SIGFPE:
        switch(siginf->si_code) {
        case FPE_INTDIV:
        case FPE_FLTDIV:
            rt_error(pc, "division by zero");
            break;
        default:
            rt_error(pc, "floating point exception");
            break;
        }
        break;
    case SIGBUS:
    case SIGSEGV:
        rt_error(pc, "dereferencing invalid pointer");
        break;
    case SIGILL:
        rt_error(pc, "illegal instruction");
        break;
    case SIGABRT:
        rt_error(pc, "abort() called");
        break;
    default:
        rt_error(pc, "caught signal %d", signum);
        break;
    }
    exit(255);
}

/* launch the compiled program with the given arguments */
int launch_exe(int argc, char **argv)
{
    Sym *s;
    int (*t)();
    struct sigaction sigact;

    s = sym_find1(&extern_stack, TOK_MAIN);
    if (!s || (s->r & VT_FORWARD))
        error("main() not defined");
    
    if (do_debug) {
        /* install TCC signal handlers to print debug info on fatal
           runtime errors */
        sigact.sa_flags = SA_SIGINFO | SA_ONESHOT;
        sigact.sa_sigaction = sig_error;
        sigemptyset(&sigact.sa_mask);
        sigaction(SIGFPE, &sigact, NULL);
        sigaction(SIGILL, &sigact, NULL);
        sigaction(SIGSEGV, &sigact, NULL);
        sigaction(SIGBUS, &sigact, NULL);
        sigaction(SIGABRT, &sigact, NULL);
    }

    if (do_bounds_check) {
        int *p, *p_end;
        __bound_init();
        /* add all known static regions */
        p = (int *)bounds_section->data;
        p_end = (int *)bounds_section->data_ptr;
        while (p < p_end) {
            __bound_new_region((void *)p[0], p[1]);
            p += 2;
        }
    }

    t = (int (*)())s->c;
    return (*t)(argc, argv);
}


void help(void)
{
    printf("tcc version 0.9.3 - Tiny C Compiler - Copyright (C) 2001, 2002 Fabrice Bellard\n" 
           "usage: tcc [-Idir] [-Dsym[=val]] [-Usym] [-llib] [-g] [-b]\n"
           "           [-i infile] infile [infile_args...]\n"
           "\n"
           "-Idir        : add include path 'dir'\n"
           "-Dsym[=val]  : define 'sym' with value 'val'\n"
           "-Usym        : undefine 'sym'\n"
           "-llib        : link with dynamic library 'lib'\n"
           "-g           : generate runtime debug info\n"
           "-b           : compile with built-in memory and bounds checker (implies -g)\n"
           "-i infile    : compile infile\n"
           );
}

int main(int argc, char **argv)
{
    char *p, *r, *outfile;
    int optind;

    include_paths[0] = "/usr/include";
    include_paths[1] = "/usr/lib/tcc";
    include_paths[2] = "/usr/local/lib/tcc";
    nb_include_paths = 3;

    /* add all tokens */
    tok_ident = TOK_IDENT;
    p = "int\0void\0char\0if\0else\0while\0break\0return\0for\0extern\0static\0unsigned\0goto\0do\0continue\0switch\0case\0const\0volatile\0long\0register\0signed\0auto\0inline\0restrict\0float\0double\0_Bool\0short\0struct\0union\0typedef\0default\0enum\0sizeof\0__attribute__\0define\0include\0ifdef\0ifndef\0elif\0endif\0defined\0undef\0error\0line\0__LINE__\0__FILE__\0__DATE__\0__TIME__\0__VA_ARGS__\0__func__\0main\0section\0__section__\0aligned\0__aligned__\0unused\0__unused__\0";
    while (*p) {
        r = p;
        while (*r++);
        tok_alloc(p, r - p - 1);
        p = r;
    }

    /* standard defines */
    define_symbol("__STDC__");
#ifdef __i386__
    define_symbol("__i386__");
#endif
    /* tiny C specific defines */
    define_symbol("__TINYC__");
    
    /* create standard sections */
    text_section = new_section(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
    data_section = new_section(".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
    /* XXX: should change type to SHT_NOBITS */
    bss_section = new_section(".bss", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);

    optind = 1;
    outfile = NULL;
    while (1) {
        if (optind >= argc) {
        show_help:
            help();
            return 1;
        }
        r = argv[optind];
        if (r[0] != '-')
            break;
        optind++;
        if (r[1] == 'I') {
            if (nb_include_paths >= INCLUDE_PATHS_MAX)
                error("too many include paths");
            include_paths[nb_include_paths++] = r + 2;
        } else if (r[1] == 'D') {
            define_symbol(r + 2);
        } else if (r[1] == 'U') {
            undef_symbol(r + 2);
        } else if (r[1] == 'l') {
            open_dll(r + 2);
        } else if (r[1] == 'i') {
            if (optind >= argc)
                goto show_help;
            tcc_compile_file(argv[optind++]);
        } else if (r[1] == 'b') {
            if (!do_bounds_check) {
                do_bounds_check = 1;
                /* define symbol */
                define_symbol("__BOUNDS_CHECKING_ON");
                /* create bounds sections */
                bounds_section = new_section(".bounds", 
                                             SHT_PROGBITS, SHF_ALLOC);
                lbounds_section = new_section(".lbounds", 
                                              SHT_PROGBITS, SHF_ALLOC);
                /* debug is implied */
                goto debug_opt;
            }
        } else if (r[1] == 'g') {
        debug_opt:
            if (!do_debug) {
                do_debug = 1;

                /* stab symbols */
                stab_section = new_section(".stab", SHT_PROGBITS, 0);
                stab_section->sh_entsize = sizeof(Stab_Sym);
                stabstr_section = new_section(".stabstr", SHT_STRTAB, 0);
                put_elf_str(stabstr_section, "");
                stab_section->link = stabstr_section;
                /* put first entry */
                put_stabs("", 0, 0, 0, 0);
                
                /* elf symbols */
                symtab_section = new_section(".symtab", SHT_SYMTAB, 0);
                symtab_section->sh_entsize = sizeof(Elf32_Sym);
                strtab_section = new_section(".strtab", SHT_STRTAB, 0);
                put_elf_str(strtab_section, "");
                symtab_section->link = strtab_section;
                put_elf_sym(symtab_section, 0, 0, 0, 0, 0, NULL);
            }
        } else if (r[1] == 'o') {
            /* currently, only for testing, so not documented */
            if (optind >= argc)
                goto show_help;
            outfile = argv[optind++];
        } else {
            fprintf(stderr, "invalid option -- '%s'\n", r);
            exit(1);
        }
    }
    
    tcc_compile_file(argv[optind]);

    resolve_extern_syms();

    if (outfile) {
        build_exe(outfile);
        return 0;
    } else {
        return launch_exe(argc - optind, argv + optind);
    }
}
