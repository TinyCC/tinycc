/*
 *  TCC - Tiny C Compiler
 * 
 *  Copyright (c) 2001-2004 Fabrice Bellard
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

#define USING_GLOBALS
#include "tcc.h"

/********************************************************/
/* global variables */

/* ------------------------------------------------------------------------- */

static void tok_print(TCCState *S, const char *msg, const int *str);

static const char tcc_keywords[] = 
#define DEF(id, str) str "\0"
#include "tcctok.h"
#undef DEF
;

/* WARNING: the content of this string encodes token numbers */
static const unsigned char tok_two_chars[] =
/* outdated -- gr
    "<=\236>=\235!=\225&&\240||\241++\244--\242==\224<<\1>>\2+=\253"
    "-=\255*=\252/=\257%=\245&=\246^=\336|=\374->\313..\250##\266";
*/{
    '<','=', TOK_LE,
    '>','=', TOK_GE,
    '!','=', TOK_NE,
    '&','&', TOK_LAND,
    '|','|', TOK_LOR,
    '+','+', TOK_INC,
    '-','-', TOK_DEC,
    '=','=', TOK_EQ,
    '<','<', TOK_SHL,
    '>','>', TOK_SAR,
    '+','=', TOK_A_ADD,
    '-','=', TOK_A_SUB,
    '*','=', TOK_A_MUL,
    '/','=', TOK_A_DIV,
    '%','=', TOK_A_MOD,
    '&','=', TOK_A_AND,
    '^','=', TOK_A_XOR,
    '|','=', TOK_A_OR,
    '-','>', TOK_ARROW,
    '.','.', TOK_TWODOTS,
    '#','#', TOK_TWOSHARPS,
    '#','#', TOK_PPJOIN,
    0
};

static void next_nomacro(TCCState *S);

ST_FUNC void skip(TCCState *S, int c)
{
    if (S->tok != c)
        tcc_error(S, "'%c' expected (got \"%s\")", c, get_tok_str(S, S->tok, &S->tokc));
    next(S);
}

ST_FUNC void expect(TCCState *S, const char *msg)
{
    tcc_error(S, "%s expected", msg);
}

/* ------------------------------------------------------------------------- */
/* Custom allocator for tiny objects */

#define USE_TAL

#ifndef USE_TAL
#define tal_free(S, al, p) tcc_free(S, p)
#define tal_realloc(S, al, p, size) tcc_realloc(S, p, size)
#define tal_new(S, a, b, c)
#define tal_delete(S, a)
#else
#if !defined(MEM_DEBUG)
#define tal_free(S, al, p) tal_free_impl(S, al, p)
#define tal_realloc(S, al, p, size) tal_realloc_impl(S, &al, p, size)
#define TAL_DEBUG_PARAMS
#else
#define TAL_DEBUG 1
//#define TAL_INFO 1 /* collect and dump allocators stats */
#define tal_free(S, al, p) tal_free_impl(S, al, p, __FILE__, __LINE__)
#define tal_realloc(S, al, p, size) tal_realloc_impl(S, &al, p, size, __FILE__, __LINE__)
#define TAL_DEBUG_PARAMS , const char *file, int line
#define TAL_DEBUG_FILE_LEN 40
#endif

#define TOKSYM_TAL_SIZE     (768 * 1024) /* allocator for tiny TokenSym in table_ident */
#define TOKSTR_TAL_SIZE     (768 * 1024) /* allocator for tiny TokenString instances */
#define CSTR_TAL_SIZE       (256 * 1024) /* allocator for tiny CString instances */
#define TOKSYM_TAL_LIMIT    256 /* prefer unique limits to distinguish allocators debug msgs */
#define TOKSTR_TAL_LIMIT    128 /* 32 * sizeof(int) */
#define CSTR_TAL_LIMIT      1024

typedef struct tal_header_t {
    unsigned  size;
#ifdef TAL_DEBUG
    int     line_num; /* negative line_num used for double free check */
    char    file_name[TAL_DEBUG_FILE_LEN + 1];
#endif
} tal_header_t;

/* ------------------------------------------------------------------------- */

static TinyAlloc *tal_new(TCCState *S, TinyAlloc **pal, unsigned limit, unsigned size)
{
    TinyAlloc *al = tcc_mallocz(S, sizeof(TinyAlloc));
    al->p = al->buffer = tcc_malloc(S, size);
    al->limit = limit;
    al->size = size;
    if (pal) *pal = al;
    return al;
}

static void tal_delete(TCCState *S, TinyAlloc *al)
{
    TinyAlloc *next;

tail_call:
    if (!al)
        return;
#ifdef TAL_INFO
    fprintf(stderr, "limit=%5d, size=%5g MB, nb_peak=%6d, nb_total=%8d, nb_missed=%6d, usage=%5.1f%%\n",
            al->limit, al->size / 1024.0 / 1024.0, al->nb_peak, al->nb_total, al->nb_missed,
            (al->peak_p - al->buffer) * 100.0 / al->size);
#endif
#ifdef TAL_DEBUG
    if (al->nb_allocs > 0) {
        uint8_t *p;
        fprintf(stderr, "TAL_DEBUG: memory leak %d chunk(s) (limit= %d)\n",
                al->nb_allocs, al->limit);
        p = al->buffer;
        while (p < al->p) {
            tal_header_t *header = (tal_header_t *)p;
            if (header->line_num > 0) {
                fprintf(stderr, "%s:%d: chunk of %d bytes leaked\n",
                        header->file_name, header->line_num, header->size);
            }
            p += header->size + sizeof(tal_header_t);
        }
#if MEM_DEBUG-0 == 2
        exit(2);
#endif
    }
#endif
    next = al->next;
    tcc_free(S, al->buffer);
    tcc_free(S, al);
    al = next;
    goto tail_call;
}

static void tal_free_impl(TCCState *S, TinyAlloc *al, void *p TAL_DEBUG_PARAMS)
{
    if (!p)
        return;
tail_call:
    if (al->buffer <= (uint8_t *)p && (uint8_t *)p < al->buffer + al->size) {
#ifdef TAL_DEBUG
        tal_header_t *header = (((tal_header_t *)p) - 1);
        if (header->line_num < 0) {
            fprintf(stderr, "%s:%d: TAL_DEBUG: double frees chunk from\n",
                    file, line);
            fprintf(stderr, "%s:%d: %d bytes\n",
                    header->file_name, (int)-header->line_num, (int)header->size);
        } else
            header->line_num = -header->line_num;
#endif
        al->nb_allocs--;
        if (!al->nb_allocs)
            al->p = al->buffer;
    } else if (al->next) {
        al = al->next;
        goto tail_call;
    }
    else
        tcc_free(S, p);
}

static void *tal_realloc_impl(TCCState *S, TinyAlloc **pal, void *p, unsigned size TAL_DEBUG_PARAMS)
{
    tal_header_t *header;
    void *ret;
    int is_own;
    unsigned adj_size = (size + 3) & -4;
    TinyAlloc *al = *pal;

tail_call:
    is_own = (al->buffer <= (uint8_t *)p && (uint8_t *)p < al->buffer + al->size);
    if ((!p || is_own) && size <= al->limit) {
        if (al->p - al->buffer + adj_size + sizeof(tal_header_t) < al->size) {
            header = (tal_header_t *)al->p;
            header->size = adj_size;
#ifdef TAL_DEBUG
            { int ofs = strlen(file) - TAL_DEBUG_FILE_LEN;
            strncpy(header->file_name, file + (ofs > 0 ? ofs : 0), TAL_DEBUG_FILE_LEN);
            header->file_name[TAL_DEBUG_FILE_LEN] = 0;
            header->line_num = line; }
#endif
            ret = al->p + sizeof(tal_header_t);
            al->p += adj_size + sizeof(tal_header_t);
            if (is_own) {
                header = (((tal_header_t *)p) - 1);
                if (p) memcpy(ret, p, header->size);
#ifdef TAL_DEBUG
                header->line_num = -header->line_num;
#endif
            } else {
                al->nb_allocs++;
            }
#ifdef TAL_INFO
            if (al->nb_peak < al->nb_allocs)
                al->nb_peak = al->nb_allocs;
            if (al->peak_p < al->p)
                al->peak_p = al->p;
            al->nb_total++;
#endif
            return ret;
        } else if (is_own) {
            al->nb_allocs--;
            ret = tal_realloc(S, *pal, 0, size);
            header = (((tal_header_t *)p) - 1);
            if (p) memcpy(ret, p, header->size);
#ifdef TAL_DEBUG
            header->line_num = -header->line_num;
#endif
            return ret;
        }
        if (al->next) {
            al = al->next;
        } else {
            TinyAlloc *bottom = al, *next = al->top ? al->top : al;

            al = tal_new(S, pal, next->limit, next->size * 2);
            al->next = next;
            bottom->top = al;
        }
        goto tail_call;
    }
    if (is_own) {
        al->nb_allocs--;
        ret = tcc_malloc(S, size);
        header = (((tal_header_t *)p) - 1);
        if (p) memcpy(ret, p, header->size);
#ifdef TAL_DEBUG
        header->line_num = -header->line_num;
#endif
    } else if (al->next) {
        al = al->next;
        goto tail_call;
    } else
        ret = tcc_realloc(S, p, size);
#ifdef TAL_INFO
    al->nb_missed++;
#endif
    return ret;
}

#endif /* USE_TAL */

/* ------------------------------------------------------------------------- */
/* CString handling */
static void cstr_realloc(TCCState *S, CString *cstr, int new_size)
{
    int size;

    size = cstr->size_allocated;
    if (size < 8)
        size = 8; /* no need to allocate a too small first string */
    while (size < new_size)
        size = size * 2;
    cstr->data = tcc_realloc(S, cstr->data, size);
    cstr->size_allocated = size;
}

/* add a byte */
ST_INLN void cstr_ccat(TCCState *S, CString *cstr, int ch)
{
    int size;
    size = cstr->size + 1;
    if (size > cstr->size_allocated)
        cstr_realloc(S, cstr, size);
    ((unsigned char *)cstr->data)[size - 1] = ch;
    cstr->size = size;
}

ST_INLN char *unicode_to_utf8 (char *b, uint32_t Uc)
{
    if (Uc<0x80) *b++=Uc;
    else if (Uc<0x800) *b++=192+Uc/64, *b++=128+Uc%64;
    else if (Uc-0xd800u<0x800) return b;
    else if (Uc<0x10000) *b++=224+Uc/4096, *b++=128+Uc/64%64, *b++=128+Uc%64;
    else if (Uc<0x110000) *b++=240+Uc/262144, *b++=128+Uc/4096%64, *b++=128+Uc/64%64, *b++=128+Uc%64;
    return b;
}

/* add a unicode character expanded into utf8 */
ST_INLN void cstr_u8cat(TCCState *S, CString *cstr, int ch)
{
    char buf[4], *e;
    e = unicode_to_utf8(buf, (uint32_t)ch);
    cstr_cat(S, cstr, buf, e - buf);
}

ST_FUNC void cstr_cat(TCCState *S, CString *cstr, const char *str, int len)
{
    int size;
    if (len <= 0)
        len = strlen(str) + 1 + len;
    size = cstr->size + len;
    if (size > cstr->size_allocated)
        cstr_realloc(S, cstr, size);
    memmove(((unsigned char *)cstr->data) + cstr->size, str, len);
    cstr->size = size;
}

/* add a wide char */
ST_FUNC void cstr_wccat(TCCState *S, CString *cstr, int ch)
{
    int size;
    size = cstr->size + sizeof(nwchar_t);
    if (size > cstr->size_allocated)
        cstr_realloc(S, cstr, size);
    *(nwchar_t *)(((unsigned char *)cstr->data) + size - sizeof(nwchar_t)) = ch;
    cstr->size = size;
}

ST_FUNC void cstr_new(TCCState *S, CString *cstr)
{
    memset(cstr, 0, sizeof(CString));
}

/* free string and reset it to NULL */
ST_FUNC void cstr_free(TCCState *S, CString *cstr)
{
    tcc_free(S, cstr->data);
    cstr_new(S, cstr);
}

/* reset string to empty */
ST_FUNC void cstr_reset(CString *cstr)
{
    cstr->size = 0;
}

ST_FUNC int cstr_vprintf(TCCState *S, CString *cstr, const char *fmt, va_list ap)
{
    va_list v;
    int len, size = 80;
    for (;;) {
        size += cstr->size;
        if (size > cstr->size_allocated)
            cstr_realloc(S, cstr, size);
        size = cstr->size_allocated - cstr->size;
        va_copy(v, ap);
        len = vsnprintf((char*)cstr->data + cstr->size, size, fmt, v);
        va_end(v);
        if (len > 0 && len < size)
            break;
        size *= 2;
    }
    cstr->size += len;
    return len;
}

ST_FUNC int cstr_printf(TCCState *S, CString *cstr, const char *fmt, ...)
{
    va_list ap; int len;
    va_start(ap, fmt);
    len = cstr_vprintf(S, cstr, fmt, ap);
    va_end(ap);
    return len;
}

/* XXX: unicode ? */
static void add_char(TCCState *S, CString *cstr, int c)
{
    if (c == '\'' || c == '\"' || c == '\\') {
        /* XXX: could be more precise if char or string */
        cstr_ccat(S, cstr, '\\');
    }
    if (c >= 32 && c <= 126) {
        cstr_ccat(S, cstr, c);
    } else {
        cstr_ccat(S, cstr, '\\');
        if (c == '\n') {
            cstr_ccat(S, cstr, 'n');
        } else {
            cstr_ccat(S, cstr, '0' + ((c >> 6) & 7));
            cstr_ccat(S, cstr, '0' + ((c >> 3) & 7));
            cstr_ccat(S, cstr, '0' + (c & 7));
        }
    }
}

/* ------------------------------------------------------------------------- */
/* allocate a new token */
static TokenSym *tok_alloc_new(TCCState *S, TokenSym **pts, const char *str, int len)
{
    TokenSym *ts, **ptable;
    int i;

    if (S->tok_ident >= SYM_FIRST_ANOM) 
        tcc_error(S, "memory full (symbols)");

    /* expand token table if needed */
    i = S->tok_ident - TOK_IDENT;
    if ((i % TOK_ALLOC_INCR) == 0) {
        ptable = tcc_realloc(S, S->tccpp_table_ident, (i + TOK_ALLOC_INCR) * sizeof(TokenSym *));
        S->tccpp_table_ident = ptable;
    }

    ts = tal_realloc(S, S->toksym_alloc, 0, sizeof(TokenSym) + len);
    S->tccpp_table_ident[i] = ts;
    ts->tok = S->tok_ident++;
    ts->sym_define = NULL;
    ts->sym_label = NULL;
    ts->sym_struct = NULL;
    ts->sym_identifier = NULL;
    ts->len = len;
    ts->hash_next = NULL;
    memcpy(ts->str, str, len);
    ts->str[len] = '\0';
    *pts = ts;
    return ts;
}

#define TOK_HASH_INIT 1
#define TOK_HASH_FUNC(h, c) ((h) + ((h) << 5) + ((h) >> 27) + (c))


/* find a token and add it if not found */
ST_FUNC TokenSym *tok_alloc(TCCState *S, const char *str, int len)
{
    TokenSym *ts, **pts;
    int i;
    unsigned int h;
    
    h = TOK_HASH_INIT;
    for(i=0;i<len;i++)
        h = TOK_HASH_FUNC(h, ((unsigned char *)str)[i]);
    h &= (TOK_HASH_SIZE - 1);

    pts = &S->tccpp_hash_ident[h];
    for(;;) {
        ts = *pts;
        if (!ts)
            break;
        if (ts->len == len && !memcmp(ts->str, str, len))
            return ts;
        pts = &(ts->hash_next);
    }
    return tok_alloc_new(S, pts, str, len);
}

ST_FUNC int tok_alloc_const(TCCState *S, const char *str)
{
    return tok_alloc(S, str, strlen(str))->tok;
}


/* XXX: buffer overflow */
/* XXX: float tokens */
ST_FUNC const char *get_tok_str(TCCState *S, int v, CValue *cv)
{
    char *p;
    int i, len;

    cstr_reset(&S->tccpp_cstr_buf);
    p = S->tccpp_cstr_buf.data;

    switch(v) {
    case TOK_CINT:
    case TOK_CUINT:
    case TOK_CLONG:
    case TOK_CULONG:
    case TOK_CLLONG:
    case TOK_CULLONG:
        /* XXX: not quite exact, but only useful for testing  */
#ifdef _WIN32
        sprintf(p, "%u", (unsigned)cv->i);
#else
        sprintf(p, "%llu", (unsigned long long)cv->i);
#endif
        break;
    case TOK_LCHAR:
        cstr_ccat(S, &S->tccpp_cstr_buf, 'L');
    case TOK_CCHAR:
        cstr_ccat(S, &S->tccpp_cstr_buf, '\'');
        add_char(S, &S->tccpp_cstr_buf, cv->i);
        cstr_ccat(S, &S->tccpp_cstr_buf, '\'');
        cstr_ccat(S, &S->tccpp_cstr_buf, '\0');
        break;
    case TOK_PPNUM:
    case TOK_PPSTR:
        return (char*)cv->str.data;
    case TOK_LSTR:
        cstr_ccat(S, &S->tccpp_cstr_buf, 'L');
    case TOK_STR:
        cstr_ccat(S, &S->tccpp_cstr_buf, '\"');
        if (v == TOK_STR) {
            len = cv->str.size - 1;
            for(i=0;i<len;i++)
                add_char(S, &S->tccpp_cstr_buf, ((unsigned char *)cv->str.data)[i]);
        } else {
            len = (cv->str.size / sizeof(nwchar_t)) - 1;
            for(i=0;i<len;i++)
                add_char(S, &S->tccpp_cstr_buf, ((nwchar_t *)cv->str.data)[i]);
        }
        cstr_ccat(S, &S->tccpp_cstr_buf, '\"');
        cstr_ccat(S, &S->tccpp_cstr_buf, '\0');
        break;

    case TOK_CFLOAT:
        cstr_cat(S, &S->tccpp_cstr_buf, "<float>", 0);
        break;
    case TOK_CDOUBLE:
	cstr_cat(S, &S->tccpp_cstr_buf, "<double>", 0);
	break;
    case TOK_CLDOUBLE:
	cstr_cat(S, &S->tccpp_cstr_buf, "<long double>", 0);
	break;
    case TOK_LINENUM:
	cstr_cat(S, &S->tccpp_cstr_buf, "<linenumber>", 0);
	break;

    /* above tokens have value, the ones below don't */
    case TOK_LT:
        v = '<';
        goto addv;
    case TOK_GT:
        v = '>';
        goto addv;
    case TOK_DOTS:
        return strcpy(p, "...");
    case TOK_A_SHL:
        return strcpy(p, "<<=");
    case TOK_A_SAR:
        return strcpy(p, ">>=");
    case TOK_EOF:
        return strcpy(p, "<eof>");
    default:
        if (v < TOK_IDENT) {
            /* search in two bytes table */
            const unsigned char *q = tok_two_chars;
            while (*q) {
                if (q[2] == v) {
                    *p++ = q[0];
                    *p++ = q[1];
                    *p = '\0';
                    return S->tccpp_cstr_buf.data;
                }
                q += 3;
            }
        if (v >= 127) {
            sprintf(S->tccpp_cstr_buf.data, "<%02x>", v);
            return S->tccpp_cstr_buf.data;
        }
        addv:
            *p++ = v;
    case 0: /* nameless anonymous symbol */
            *p = '\0';
        } else if (v < S->tok_ident) {
            return S->tccpp_table_ident[v - TOK_IDENT]->str;
        } else if (v >= SYM_FIRST_ANOM) {
            /* special name for anonymous symbol */
            sprintf(p, "L.%u", v - SYM_FIRST_ANOM);
        } else {
            /* should never happen */
            return NULL;
        }
        break;
    }
    return S->tccpp_cstr_buf.data;
}

/* return the current character, handling end of block if necessary
   (but not stray) */
static int handle_eob(TCCState *S)
{
    BufferedFile *bf = S->tccpp_file;
    int len;

    /* only tries to read if really end of buffer */
    if (bf->buf_ptr >= bf->buf_end) {
        if (bf->fd >= 0) {
#if defined(PARSE_DEBUG)
            len = 1;
#else
            len = IO_BUF_SIZE;
#endif
            len = read(bf->fd, bf->buffer, len);
            if (len < 0)
                len = 0;
        } else {
            len = 0;
        }
        total_bytes += len;
        bf->buf_ptr = bf->buffer;
        bf->buf_end = bf->buffer + len;
        *bf->buf_end = CH_EOB;
    }
    if (bf->buf_ptr < bf->buf_end) {
        return bf->buf_ptr[0];
    } else {
        bf->buf_ptr = bf->buf_end;
        return CH_EOF;
    }
}

/* read next char from current input file and handle end of input buffer */
static inline void inp(TCCState *S)
{
    S->tccpp_ch = *(++(S->tccpp_file->buf_ptr));
    /* end of buffer/file handling */
    if (S->tccpp_ch == CH_EOB)
        S->tccpp_ch = handle_eob(S);
}

/* handle '\[\r]\n' */
static int handle_stray_noerror(TCCState *S)
{
    while (S->tccpp_ch == '\\') {
        inp(S);
        if (S->tccpp_ch == '\n') {
            S->tccpp_file->line_num++;
            inp(S);
        } else if (S->tccpp_ch == '\r') {
            inp(S);
            if (S->tccpp_ch != '\n')
                goto fail;
            S->tccpp_file->line_num++;
            inp(S);
        } else {
        fail:
            return 1;
        }
    }
    return 0;
}

static void handle_stray(TCCState *S)
{
    if (handle_stray_noerror(S))
        tcc_error(S, "stray '\\' in program");
}

/* skip the stray and handle the \\n case. Output an error if
   incorrect char after the stray */
static int handle_stray1(TCCState *S, uint8_t *p)
{
    int c;

    S->tccpp_file->buf_ptr = p;
    if (p >= S->tccpp_file->buf_end) {
        c = handle_eob(S);
        if (c != '\\')
            return c;
        p = S->tccpp_file->buf_ptr;
    }
    S->tccpp_ch = *p;
    if (handle_stray_noerror(S)) {
        if (!(S->tccpp_parse_flags & PARSE_FLAG_ACCEPT_STRAYS))
            tcc_error(S, "stray '\\' in program");
        *--S->tccpp_file->buf_ptr = '\\';
    }
    p = S->tccpp_file->buf_ptr;
    c = *p;
    return c;
}

/* handle just the EOB case, but not stray */
#define PEEKC_EOB(c, p)\
{\
    p++;\
    c = *p;\
    if (c == '\\') {\
        S->tccpp_file->buf_ptr = p;\
        c = handle_eob(S);\
        p = S->tccpp_file->buf_ptr;\
    }\
}

/* handle the complicated stray case */
#define PEEKC(c, p)\
{\
    p++;\
    c = *p;\
    if (c == '\\') {\
        c = handle_stray1(S, p);\
        p = S->tccpp_file->buf_ptr;\
    }\
}

/* input with '\[\r]\n' handling. Note that this function cannot
   handle other characters after '\', so you cannot call it inside
   strings or comments */
static void minp(TCCState *S)
{
    inp(S);
    if (S->tccpp_ch == '\\') 
        handle_stray(S);
}

/* single line C++ comments */
static uint8_t *parse_line_comment(TCCState *S, uint8_t *p)
{
    int c;

    p++;
    for(;;) {
        c = *p;
    redo:
        if (c == '\n' || c == CH_EOF) {
            break;
        } else if (c == '\\') {
            S->tccpp_file->buf_ptr = p;
            c = handle_eob(S);
            p = S->tccpp_file->buf_ptr;
            if (c == '\\') {
                PEEKC_EOB(c, p);
                if (c == '\n') {
                    S->tccpp_file->line_num++;
                    PEEKC_EOB(c, p);
                } else if (c == '\r') {
                    PEEKC_EOB(c, p);
                    if (c == '\n') {
                        S->tccpp_file->line_num++;
                        PEEKC_EOB(c, p);
                    }
                }
            } else {
                goto redo;
            }
        } else {
            p++;
        }
    }
    return p;
}

/* C comments */
static uint8_t *parse_comment(TCCState *S, uint8_t *p)
{
    int c;

    p++;
    for(;;) {
        /* fast skip loop */
        for(;;) {
            c = *p;
            if (c == '\n' || c == '*' || c == '\\')
                break;
            p++;
            c = *p;
            if (c == '\n' || c == '*' || c == '\\')
                break;
            p++;
        }
        /* now we can handle all the cases */
        if (c == '\n') {
            S->tccpp_file->line_num++;
            p++;
        } else if (c == '*') {
            p++;
            for(;;) {
                c = *p;
                if (c == '*') {
                    p++;
                } else if (c == '/') {
                    goto end_of_comment;
                } else if (c == '\\') {
                    S->tccpp_file->buf_ptr = p;
                    c = handle_eob(S);
                    p = S->tccpp_file->buf_ptr;
                    if (c == CH_EOF)
                        tcc_error(S, "unexpected end of file in comment");
                    if (c == '\\') {
                        /* skip '\[\r]\n', otherwise just skip the stray */
                        while (c == '\\') {
                            PEEKC_EOB(c, p);
                            if (c == '\n') {
                                S->tccpp_file->line_num++;
                                PEEKC_EOB(c, p);
                            } else if (c == '\r') {
                                PEEKC_EOB(c, p);
                                if (c == '\n') {
                                    S->tccpp_file->line_num++;
                                    PEEKC_EOB(c, p);
                                }
                            } else {
                                goto after_star;
                            }
                        }
                    }
                } else {
                    break;
                }
            }
        after_star: ;
        } else {
            /* stray, eob or eof */
            S->tccpp_file->buf_ptr = p;
            c = handle_eob(S);
            p = S->tccpp_file->buf_ptr;
            if (c == CH_EOF) {
                tcc_error(S, "unexpected end of file in comment");
            } else if (c == '\\') {
                p++;
            }
        }
    }
 end_of_comment:
    p++;
    return p;
}

ST_FUNC int set_idnum(TCCState *S, int c, int val)
{
    int prev = S->tccpp_isidnum_table[c - CH_EOF];
    S->tccpp_isidnum_table[c - CH_EOF] = val;
    return prev;
}

#define cinp minp

static inline void skip_spaces(TCCState *S)
{
    while (S->tccpp_isidnum_table[S->tccpp_ch - CH_EOF] & IS_SPC)
        cinp(S);
}

static inline int check_space(TCCState *S, int t, int *spc) 
{
    if (t < 256 && (S->tccpp_isidnum_table[t - CH_EOF] & IS_SPC)) {
        if (*spc) 
            return 1;
        *spc = 1;
    } else 
        *spc = 0;
    return 0;
}

/* parse a string without interpreting escapes */
static uint8_t *parse_pp_string(TCCState *S, uint8_t *p,
                                int sep, CString *str)
{
    int c;
    p++;
    for(;;) {
        c = *p;
        if (c == sep) {
            break;
        } else if (c == '\\') {
            S->tccpp_file->buf_ptr = p;
            c = handle_eob(S);
            p = S->tccpp_file->buf_ptr;
            if (c == CH_EOF) {
            unterminated_string:
                /* XXX: indicate line number of start of string */
                tcc_error(S, "missing terminating %c character", sep);
            } else if (c == '\\') {
                /* escape : just skip \[\r]\n */
                PEEKC_EOB(c, p);
                if (c == '\n') {
                    S->tccpp_file->line_num++;
                    p++;
                } else if (c == '\r') {
                    PEEKC_EOB(c, p);
                    if (c != '\n')
                        expect(S, "'\n' after '\r'");
                    S->tccpp_file->line_num++;
                    p++;
                } else if (c == CH_EOF) {
                    goto unterminated_string;
                } else {
                    if (str) {
                        cstr_ccat(S, str, '\\');
                        cstr_ccat(S, str, c);
                    }
                    p++;
                }
            }
        } else if (c == '\n') {
            S->tccpp_file->line_num++;
            goto add_char;
        } else if (c == '\r') {
            PEEKC_EOB(c, p);
            if (c != '\n') {
                if (str)
                    cstr_ccat(S, str, '\r');
            } else {
                S->tccpp_file->line_num++;
                goto add_char;
            }
        } else {
        add_char:
            if (str)
                cstr_ccat(S, str, c);
            p++;
        }
    }
    p++;
    return p;
}

/* skip block of text until #else, #elif or #endif. skip also pairs of
   #if/#endif */
static void preprocess_skip(TCCState *S)
{
    int a, start_of_line, c, in_warn_or_error;
    uint8_t *p;

    p = S->tccpp_file->buf_ptr;
    a = 0;
redo_start:
    start_of_line = 1;
    in_warn_or_error = 0;
    for(;;) {
    redo_no_start:
        c = *p;
        switch(c) {
        case ' ':
        case '\t':
        case '\f':
        case '\v':
        case '\r':
            p++;
            goto redo_no_start;
        case '\n':
            S->tccpp_file->line_num++;
            p++;
            goto redo_start;
        case '\\':
            S->tccpp_file->buf_ptr = p;
            c = handle_eob(S);
            if (c == CH_EOF) {
                expect(S, "#endif");
            } else if (c == '\\') {
                S->tccpp_ch = S->tccpp_file->buf_ptr[0];
                handle_stray_noerror(S);
            }
            p = S->tccpp_file->buf_ptr;
            goto redo_no_start;
        /* skip strings */
        case '\"':
        case '\'':
            if (in_warn_or_error)
                goto _default;
            p = parse_pp_string(S, p, c, NULL);
            break;
        /* skip comments */
        case '/':
            if (in_warn_or_error)
                goto _default;
            S->tccpp_file->buf_ptr = p;
            S->tccpp_ch = *p;
            minp(S);
            p = S->tccpp_file->buf_ptr;
            if (S->tccpp_ch == '*') {
                p = parse_comment(S, p);
            } else if (S->tccpp_ch == '/') {
                p = parse_line_comment(S, p);
            }
            break;
        case '#':
            p++;
            if (start_of_line) {
                S->tccpp_file->buf_ptr = p;
                next_nomacro(S);
                p = S->tccpp_file->buf_ptr;
                if (a == 0 && 
                    (S->tok == TOK_ELSE || S->tok == TOK_ELIF || S->tok == TOK_ENDIF))
                    goto the_end;
                if (S->tok == TOK_IF || S->tok == TOK_IFDEF || S->tok == TOK_IFNDEF)
                    a++;
                else if (S->tok == TOK_ENDIF)
                    a--;
                else if( S->tok == TOK_ERROR || S->tok == TOK_WARNING)
                    in_warn_or_error = 1;
                else if (S->tok == TOK_LINEFEED)
                    goto redo_start;
                else if (S->tccpp_parse_flags & PARSE_FLAG_ASM_FILE)
                    p = parse_line_comment(S, p - 1);
            }
#if !defined(TCC_TARGET_ARM)
            else if (S->tccpp_parse_flags & PARSE_FLAG_ASM_FILE)
                p = parse_line_comment(S, p - 1);
#else
            /* ARM assembly uses '#' for constants */
#endif
            break;
_default:
        default:
            p++;
            break;
        }
        start_of_line = 0;
    }
 the_end: ;
    S->tccpp_file->buf_ptr = p;
}

#if 0
/* return the number of additional 'ints' necessary to store the
   token */
static inline int tok_size(const int *p)
{
    switch(*p) {
        /* 4 bytes */
    case TOK_CINT:
    case TOK_CUINT:
    case TOK_CCHAR:
    case TOK_LCHAR:
    case TOK_CFLOAT:
    case TOK_LINENUM:
        return 1 + 1;
    case TOK_STR:
    case TOK_LSTR:
    case TOK_PPNUM:
    case TOK_PPSTR:
        return 1 + ((sizeof(CString) + ((CString *)(p+1))->size + 3) >> 2);
    case TOK_CLONG:
    case TOK_CULONG:
	return 1 + LONG_SIZE / 4;
    case TOK_CDOUBLE:
    case TOK_CLLONG:
    case TOK_CULLONG:
        return 1 + 2;
    case TOK_CLDOUBLE:
        return 1 + LDOUBLE_SIZE / 4;
    default:
        return 1 + 0;
    }
}
#endif

/* token string handling */
ST_INLN void tok_str_new(TokenString *s)
{
    s->str = NULL;
    s->len = s->lastlen = 0;
    s->allocated_len = 0;
    s->last_line_num = -1;
}

ST_FUNC TokenString *tok_str_alloc(TCCState *S)
{
    TokenString *str = tal_realloc(S, S->tokstr_alloc, 0, sizeof *str);
    tok_str_new(str);
    return str;
}

ST_FUNC int *tok_str_dup(TCCState *S, TokenString *s)
{
    int *str;

    str = tal_realloc(S, S->tokstr_alloc, 0, s->len * sizeof(int));
    memcpy(str, s->str, s->len * sizeof(int));
    return str;
}

ST_FUNC void tok_str_free_str(TCCState *S, int *str)
{
    tal_free(S, S->tokstr_alloc, str);
}

ST_FUNC void tok_str_free(TCCState *S, TokenString *str)
{
    tok_str_free_str(S, str->str);
    tal_free(S, S->tokstr_alloc, str);
}

ST_FUNC int *tok_str_realloc(TCCState *S, TokenString *s, int new_size)
{
    int *str, size;

    size = s->allocated_len;
    if (size < 16)
        size = 16;
    while (size < new_size)
        size = size * 2;
    if (size > s->allocated_len) {
        str = tal_realloc(S, S->tokstr_alloc, s->str, size * sizeof(int));
        s->allocated_len = size;
        s->str = str;
    }
    return s->str;
}

ST_FUNC void tok_str_add(TCCState *S, TokenString *s, int t)
{
    int len, *str;

    len = s->len;
    str = s->str;
    if (len >= s->allocated_len)
        str = tok_str_realloc(S, s, len + 1);
    str[len++] = t;
    s->len = len;
}

ST_FUNC void begin_macro(TCCState *S, TokenString *str, int alloc)
{
    str->alloc = alloc;
    str->prev = S->tccpp_macro_stack;
    str->prev_ptr = S->tccpp_macro_ptr;
    str->save_line_num = S->tccpp_file->line_num;
    S->tccpp_macro_ptr = str->str;
    S->tccpp_macro_stack = str;
}

ST_FUNC void end_macro(TCCState *S)
{
    TokenString *str = S->tccpp_macro_stack;
    S->tccpp_macro_stack = str->prev;
    S->tccpp_macro_ptr = str->prev_ptr;
    S->tccpp_file->line_num = str->save_line_num;
    if (str->alloc != 0) {
        if (str->alloc == 2)
            str->str = NULL; /* don't free */
        tok_str_free(S, str);
    }
}

static void tok_str_add2(TCCState *S, TokenString *s, int t, CValue *cv)
{
    int len, *str;

    len = s->lastlen = s->len;
    str = s->str;

    /* allocate space for worst case */
    if (len + TOK_MAX_SIZE >= s->allocated_len)
        str = tok_str_realloc(S, s, len + TOK_MAX_SIZE + 1);
    str[len++] = t;
    switch(t) {
    case TOK_CINT:
    case TOK_CUINT:
    case TOK_CCHAR:
    case TOK_LCHAR:
    case TOK_CFLOAT:
    case TOK_LINENUM:
#if LONG_SIZE == 4
    case TOK_CLONG:
    case TOK_CULONG:
#endif
        str[len++] = cv->tab[0];
        break;
    case TOK_PPNUM:
    case TOK_PPSTR:
    case TOK_STR:
    case TOK_LSTR:
        {
            /* Insert the string into the int array. */
            size_t nb_words =
                1 + (cv->str.size + sizeof(int) - 1) / sizeof(int);
            if (len + nb_words >= s->allocated_len)
                str = tok_str_realloc(S, s, len + nb_words + 1);
            str[len] = cv->str.size;
            memcpy(&str[len + 1], cv->str.data, cv->str.size);
            len += nb_words;
        }
        break;
    case TOK_CDOUBLE:
    case TOK_CLLONG:
    case TOK_CULLONG:
#if LONG_SIZE == 8
    case TOK_CLONG:
    case TOK_CULONG:
#endif
#if LDOUBLE_SIZE == 8
    case TOK_CLDOUBLE:
#endif
        str[len++] = cv->tab[0];
        str[len++] = cv->tab[1];
        break;
#if LDOUBLE_SIZE == 12
    case TOK_CLDOUBLE:
        str[len++] = cv->tab[0];
        str[len++] = cv->tab[1];
        str[len++] = cv->tab[2];
#elif LDOUBLE_SIZE == 16
    case TOK_CLDOUBLE:
        str[len++] = cv->tab[0];
        str[len++] = cv->tab[1];
        str[len++] = cv->tab[2];
        str[len++] = cv->tab[3];
#elif LDOUBLE_SIZE != 8
#error add long double size support
#endif
        break;
    default:
        break;
    }
    s->len = len;
}

/* add the current parse token in token string 's' */
ST_FUNC void tok_str_add_tok(TCCState *S, TokenString *s)
{
    CValue cval;

    /* save line number info */
    if (S->tccpp_file->line_num != s->last_line_num) {
        s->last_line_num = S->tccpp_file->line_num;
        cval.i = s->last_line_num;
        tok_str_add2(S, s, TOK_LINENUM, &cval);
    }
    tok_str_add2(S, s, S->tok, &S->tokc);
}

/* get a token from an integer array and increment pointer. */
static inline void tok_get(int *t, const int **pp, CValue *cv)
{
    const int *p = *pp;
    int n, *tab;

    tab = cv->tab;
    switch(*t = *p++) {
#if LONG_SIZE == 4
    case TOK_CLONG:
#endif
    case TOK_CINT:
    case TOK_CCHAR:
    case TOK_LCHAR:
    case TOK_LINENUM:
        cv->i = *p++;
        break;
#if LONG_SIZE == 4
    case TOK_CULONG:
#endif
    case TOK_CUINT:
        cv->i = (unsigned)*p++;
        break;
    case TOK_CFLOAT:
	tab[0] = *p++;
	break;
    case TOK_STR:
    case TOK_LSTR:
    case TOK_PPNUM:
    case TOK_PPSTR:
        cv->str.size = *p++;
        cv->str.data = p;
        p += (cv->str.size + sizeof(int) - 1) / sizeof(int);
        break;
    case TOK_CDOUBLE:
    case TOK_CLLONG:
    case TOK_CULLONG:
#if LONG_SIZE == 8
    case TOK_CLONG:
    case TOK_CULONG:
#endif
        n = 2;
        goto copy;
    case TOK_CLDOUBLE:
#if LDOUBLE_SIZE == 16
        n = 4;
#elif LDOUBLE_SIZE == 12
        n = 3;
#elif LDOUBLE_SIZE == 8
        n = 2;
#else
# error add long double size support
#endif
    copy:
        do
            *tab++ = *p++;
        while (--n);
        break;
    default:
        break;
    }
    *pp = p;
}

#if 0
# define TOK_GET(t,p,c) tok_get(t,p,c)
#else
# define TOK_GET(t,p,c) do { \
    int _t = **(p); \
    if (TOK_HAS_VALUE(_t)) \
        tok_get(t, p, c); \
    else \
        *(t) = _t, ++*(p); \
    } while (0)
#endif

static int macro_is_equal(TCCState *S, const int *a, const int *b)
{
    CValue cv;
    int t;

    if (!a || !b)
        return 1;

    while (*a && *b) {
        /* first time preallocate macro_equal_buf, next time only reset position to start */
        cstr_reset(&S->tccpp_macro_equal_buf);
        TOK_GET(&t, &a, &cv);
        cstr_cat(S, &S->tccpp_macro_equal_buf, get_tok_str(S, t, &cv), 0);
        TOK_GET(&t, &b, &cv);
        if (strcmp(S->tccpp_macro_equal_buf.data, get_tok_str(S, t, &cv)))
            return 0;
    }
    return !(*a || *b);
}

/* defines handling */
ST_INLN void define_push(TCCState *S, int v, int macro_type, int *str, Sym *first_arg)
{
    Sym *s, *o;

    o = define_find(S, v);
    s = sym_push2(S, &S->tccgen_define_stack, v, macro_type, 0);
    s->d = str;
    s->next = first_arg;
    S->tccpp_table_ident[v - TOK_IDENT]->sym_define = s;

    if (o && !macro_is_equal(S, o->d, s->d))
	tcc_warning(S, "%s redefined", get_tok_str(S, v, NULL));
}

/* undefined a define symbol. Its name is just set to zero */
ST_FUNC void define_undef(TCCState *S, Sym *s)
{
    int v = s->v;
    if (v >= TOK_IDENT && v < S->tok_ident)
        S->tccpp_table_ident[v - TOK_IDENT]->sym_define = NULL;
}

ST_INLN Sym *define_find(TCCState *S, int v)
{
    v -= TOK_IDENT;
    if ((unsigned)v >= (unsigned)(S->tok_ident - TOK_IDENT))
        return NULL;
    return S->tccpp_table_ident[v]->sym_define;
}

/* free define stack until top reaches 'b' */
ST_FUNC void free_defines(TCCState *S, Sym *b)
{
    while (S->tccgen_define_stack != b) {
        Sym *top = S->tccgen_define_stack;
        S->tccgen_define_stack = top->prev;
        tok_str_free_str(S, top->d);
        define_undef(S, top);
        sym_free(S, top);
    }
}

/* label lookup */
ST_FUNC Sym *label_find(TCCState *S, int v)
{
    v -= TOK_IDENT;
    if ((unsigned)v >= (unsigned)(S->tok_ident - TOK_IDENT))
        return NULL;
    return S->tccpp_table_ident[v]->sym_label;
}

ST_FUNC Sym *label_push(TCCState *S, Sym **ptop, int v, int flags)
{
    Sym *s, **ps;
    s = sym_push2(S, ptop, v, 0, 0);
    s->r = flags;
    ps = &S->tccpp_table_ident[v - TOK_IDENT]->sym_label;
    if (ptop == &S->tccgen_global_label_stack) {
        /* modify the top most local identifier, so that
           sym_identifier will point to 's' when popped */
        while (*ps != NULL)
            ps = &(*ps)->prev_tok;
    }
    s->prev_tok = *ps;
    *ps = s;
    return s;
}

/* pop labels until element last is reached. Look if any labels are
   undefined. Define symbols if '&&label' was used. */
ST_FUNC void label_pop(TCCState *S, Sym **ptop, Sym *slast, int keep)
{
    Sym *s, *s1;
    for(s = *ptop; s != slast; s = s1) {
        s1 = s->prev;
        if (s->r == LABEL_DECLARED) {
            tcc_warning_c(warn_all)(S, "label '%s' declared but not used", get_tok_str(S, s->v, NULL));
        } else if (s->r == LABEL_FORWARD) {
                tcc_error(S, "label '%s' used but not defined",
                      get_tok_str(S, s->v, NULL));
        } else {
            if (s->c) {
                /* define corresponding symbol. A size of
                   1 is put. */
                put_extern_sym(S, s, cur_text_section, s->jnext, 1);
            }
        }
        /* remove label */
        if (s->r != LABEL_GONE)
            S->tccpp_table_ident[s->v - TOK_IDENT]->sym_label = s->prev_tok;
        if (!keep)
            sym_free(S, s);
        else
            s->r = LABEL_GONE;
    }
    if (!keep)
        *ptop = slast;
}

/* fake the nth "#if defined test_..." for tcc -dt -run */
static void maybe_run_test(TCCState *S)
{
    const char *p;
    if (S->include_stack_ptr != S->include_stack)
        return;
    p = get_tok_str(S, S->tok, NULL);
    if (0 != memcmp(p, "test_", 5))
        return;
    if (0 != --S->run_test)
        return;
    fprintf(S->ppfp, &"\n[%s]\n"[!(S->dflag & TCC_OPTION_d_32)], p), fflush(S->ppfp);
    define_push(S, S->tok, MACRO_OBJ, NULL, NULL);
}

/* eval an expression for #if/#elif */
static int expr_preprocess(TCCState *S)
{
    int c, t;
    TokenString *str;
    
    str = tok_str_alloc(S);
    S->tccpp_pp_expr = 1;
    while (S->tok != TOK_LINEFEED && S->tok != TOK_EOF) {
        next(S); /* do macro subst */
      redo:
        if (S->tok == TOK_DEFINED) {
            next_nomacro(S);
            t = S->tok;
            if (t == '(') 
                next_nomacro(S);
            if (S->tok < TOK_IDENT)
                expect(S, "identifier");
            if (S->run_test)
                maybe_run_test(S);
            c = define_find(S, S->tok) != 0;
            if (t == '(') {
                next_nomacro(S);
                if (S->tok != ')')
                    expect(S, "')'");
            }
            S->tok = TOK_CINT;
            S->tokc.i = c;
        } else if (1 && S->tok == TOK___HAS_INCLUDE) {
            next(S);  /* XXX check if correct to use expansion */
            skip(S, '(');
            while (S->tok != ')' && S->tok != TOK_EOF)
              next(S);
            if (S->tok != ')')
              expect(S, "')'");
            S->tok = TOK_CINT;
            S->tokc.i = 0;
        } else if (S->tok >= TOK_IDENT) {
            /* if undefined macro, replace with zero, check for func-like */
            t = S->tok;
            S->tok = TOK_CINT;
            S->tokc.i = 0;
            tok_str_add_tok(S, str);
            next(S);
            if (S->tok == '(')
                tcc_error(S, "function-like macro '%s' is not defined",
                          get_tok_str(S, t, NULL));
            goto redo;
        }
        tok_str_add_tok(S, str);
    }
    S->tccpp_pp_expr = 0;
    tok_str_add(S, str, -1); /* simulate end of file */
    tok_str_add(S, str, 0);
    /* now evaluate C constant expression */
    begin_macro(S, str, 1);
    next(S);
    c = expr_const(S);
    end_macro(S);
    return c != 0;
}


/* parse after #define */
ST_FUNC void parse_define(TCCState *S)
{
    Sym *s, *first, **ps;
    int v, t, varg, is_vaargs, spc;
    int saved_parse_flags = S->tccpp_parse_flags;

    v = S->tok;
    if (v < TOK_IDENT || v == TOK_DEFINED)
        tcc_error(S, "invalid macro name '%s'", get_tok_str(S, S->tok, &S->tokc));
    /* XXX: should check if same macro (ANSI) */
    first = NULL;
    t = MACRO_OBJ;
    /* We have to parse the whole define as if not in asm mode, in particular
       no line comment with '#' must be ignored.  Also for function
       macros the argument list must be parsed without '.' being an ID
       character.  */
    S->tccpp_parse_flags = ((S->tccpp_parse_flags & ~PARSE_FLAG_ASM_FILE) | PARSE_FLAG_SPACES);
    /* '(' must be just after macro definition for MACRO_FUNC */
    next_nomacro(S);
    S->tccpp_parse_flags &= ~PARSE_FLAG_SPACES;
    if (S->tok == '(') {
        int dotid = set_idnum(S, '.', 0);
        next_nomacro(S);
        ps = &first;
        if (S->tok != ')') for (;;) {
            varg = S->tok;
            next_nomacro(S);
            is_vaargs = 0;
            if (varg == TOK_DOTS) {
                varg = TOK___VA_ARGS__;
                is_vaargs = 1;
            } else if (S->tok == TOK_DOTS && gnu_ext) {
                is_vaargs = 1;
                next_nomacro(S);
            }
            if (varg < TOK_IDENT)
        bad_list:
                tcc_error(S, "bad macro parameter list");
            s = sym_push2(S, &S->tccgen_define_stack, varg | SYM_FIELD, is_vaargs, 0);
            *ps = s;
            ps = &s->next;
            if (S->tok == ')')
                break;
            if (S->tok != ',' || is_vaargs)
                goto bad_list;
            next_nomacro(S);
        }
        S->tccpp_parse_flags |= PARSE_FLAG_SPACES;
        next_nomacro(S);
        t = MACRO_FUNC;
        set_idnum(S, '.', dotid);
    }

    S->tokstr_buf.len = 0;
    spc = 2;
    S->tccpp_parse_flags |= PARSE_FLAG_ACCEPT_STRAYS | PARSE_FLAG_SPACES | PARSE_FLAG_LINEFEED;
    /* The body of a macro definition should be parsed such that identifiers
       are parsed like the file mode determines (i.e. with '.' being an
       ID character in asm mode).  But '#' should be retained instead of
       regarded as line comment leader, so still don't set ASM_FILE
       in parse_flags. */
    while (S->tok != TOK_LINEFEED && S->tok != TOK_EOF) {
        /* remove spaces around ## and after '#' */
        if (TOK_TWOSHARPS == S->tok) {
            if (2 == spc)
                goto bad_twosharp;
            if (1 == spc)
                --S->tokstr_buf.len;
            spc = 3;
	    S->tok = TOK_PPJOIN;
        } else if ('#' == S->tok) {
            spc = 4;
        } else if (check_space(S, S->tok, &spc)) {
            goto skip;
        }
        tok_str_add2(S, &S->tokstr_buf, S->tok, &S->tokc);
    skip:
        next_nomacro(S);
    }

    S->tccpp_parse_flags = saved_parse_flags;
    if (spc == 1)
        --S->tokstr_buf.len; /* remove trailing space */
    tok_str_add(S, &S->tokstr_buf, 0);
    if (3 == spc)
bad_twosharp:
        tcc_error(S, "'##' cannot appear at either end of macro");
    define_push(S, v, t, tok_str_dup(S, &S->tokstr_buf), first);
}

static CachedInclude *search_cached_include(TCCState *S, const char *filename, int add)
{
    const unsigned char *s;
    unsigned int h;
    CachedInclude *e;
    int i;

    h = TOK_HASH_INIT;
    s = (unsigned char *) filename;
    while (*s) {
#ifdef _WIN32
        h = TOK_HASH_FUNC(h, toup(*s));
#else
        h = TOK_HASH_FUNC(h, *s);
#endif
        s++;
    }
    h &= (CACHED_INCLUDES_HASH_SIZE - 1);

    i = S->cached_includes_hash[h];
    for(;;) {
        if (i == 0)
            break;
        e = S->cached_includes[i - 1];
        if (0 == PATHCMP(e->filename, filename))
            return e;
        i = e->hash_next;
    }
    if (!add)
        return NULL;

    e = tcc_malloc(S, sizeof(CachedInclude) + strlen(filename));
    strcpy(e->filename, filename);
    e->ifndef_macro = e->once = 0;
    dynarray_add(S, &S->cached_includes, &S->nb_cached_includes, e);
    /* add in hash table */
    e->hash_next = S->cached_includes_hash[h];
    S->cached_includes_hash[h] = S->nb_cached_includes;
#ifdef INC_DEBUG
    printf("adding cached '%s'\n", filename);
#endif
    return e;
}

static void pragma_parse(TCCState *S)
{
    next_nomacro(S);
    if (S->tok == TOK_push_macro || S->tok == TOK_pop_macro) {
        int t = S->tok, v;
        Sym *s;

        if (next(S), S->tok != '(')
            goto pragma_err;
        if (next(S), S->tok != TOK_STR)
            goto pragma_err;
        v = tok_alloc(S, S->tokc.str.data, S->tokc.str.size - 1)->tok;
        if (next(S), S->tok != ')')
            goto pragma_err;
        if (t == TOK_push_macro) {
            while (NULL == (s = define_find(S, v)))
                define_push(S, v, 0, NULL, NULL);
            s->type.ref = s; /* set push boundary */
        } else {
            for (s = S->tccgen_define_stack; s; s = s->prev)
                if (s->v == v && s->type.ref == s) {
                    s->type.ref = NULL;
                    break;
                }
        }
        if (s)
            S->tccpp_table_ident[v - TOK_IDENT]->sym_define = s->d ? s : NULL;
        else
            tcc_warning(S, "unbalanced #pragma pop_macro");
        S->tccpp_pp_debug_tok = t, S->tccpp_pp_debug_symv = v;

    } else if (S->tok == TOK_once) {
        search_cached_include(S, S->tccpp_file->filename, 1)->once = S->tccpp_pp_once;

    } else if (S->output_type == TCC_OUTPUT_PREPROCESS) {
        /* tcc -E: keep pragmas below unchanged */
        unget_tok(S, ' ');
        unget_tok(S, TOK_PRAGMA);
        unget_tok(S, '#');
        unget_tok(S, TOK_LINEFEED);

    } else if (S->tok == TOK_pack) {
        /* This may be:
           #pragma pack(1) // set
           #pragma pack() // reset to default
           #pragma pack(push) // push current
           #pragma pack(push,1) // push & set
           #pragma pack(pop) // restore previous */
        next(S);
        skip(S, '(');
        if (S->tok == TOK_ASM_pop) {
            next(S);
            if (S->pack_stack_ptr <= S->pack_stack) {
            stk_error:
                tcc_error(S, "out of pack stack");
            }
            S->pack_stack_ptr--;
        } else {
            int val = 0;
            if (S->tok != ')') {
                if (S->tok == TOK_ASM_push) {
                    next(S);
                    if (S->pack_stack_ptr >= S->pack_stack + PACK_STACK_SIZE - 1)
                        goto stk_error;
                    val = *S->pack_stack_ptr++;
                    if (S->tok != ',')
                        goto pack_set;
                    next(S);
                }
                if (S->tok != TOK_CINT)
                    goto pragma_err;
                val = S->tokc.i;
                if (val < 1 || val > 16 || (val & (val - 1)) != 0)
                    goto pragma_err;
                next(S);
            }
        pack_set:
            *S->pack_stack_ptr = val;
        }
        if (S->tok != ')')
            goto pragma_err;

    } else if (S->tok == TOK_comment) {
        char *p; int t;
        next(S);
        skip(S, '(');
        t = S->tok;
        next(S);
        skip(S, ',');
        if (S->tok != TOK_STR)
            goto pragma_err;
        p = tcc_strdup(S, (char *)S->tokc.str.data);
        next(S);
        if (S->tok != ')')
            goto pragma_err;
        if (t == TOK_lib) {
            dynarray_add(S, &S->pragma_libs, &S->nb_pragma_libs, p);
        } else {
            if (t == TOK_option)
                tcc_set_options(S, p);
            tcc_free(S, p);
        }

    } else
        tcc_warning_c(warn_unsupported)(S, "#pragma %s ignored", get_tok_str(S, S->tok, &S->tokc));
    return;

pragma_err:
    tcc_error(S, "malformed #pragma directive");
    return;
}

/* is_bof is true if first non space token at beginning of file */
ST_FUNC void preprocess(TCCState *S, int is_bof)
{
    int i, c, n, saved_parse_flags;
    char buf[1024], *q;
    Sym *s;

    saved_parse_flags = S->tccpp_parse_flags;
    S->tccpp_parse_flags = PARSE_FLAG_PREPROCESS
        | PARSE_FLAG_TOK_NUM
        | PARSE_FLAG_TOK_STR
        | PARSE_FLAG_LINEFEED
        | (S->tccpp_parse_flags & PARSE_FLAG_ASM_FILE)
        ;

    next_nomacro(S);
 redo:
    switch(S->tok) {
    case TOK_DEFINE:
        S->tccpp_pp_debug_tok = S->tok;
        next_nomacro(S);
        S->tccpp_pp_debug_symv = S->tok;
        parse_define(S);
        break;
    case TOK_UNDEF:
        S->tccpp_pp_debug_tok = S->tok;
        next_nomacro(S);
        S->tccpp_pp_debug_symv = S->tok;
        s = define_find(S, S->tok);
        /* undefine symbol by putting an invalid name */
        if (s)
            define_undef(S, s);
        break;
    case TOK_INCLUDE:
    case TOK_INCLUDE_NEXT:
        S->tccpp_ch = S->tccpp_file->buf_ptr[0];
        /* XXX: incorrect if comments : use next_nomacro with a special mode */
        skip_spaces(S);
        if (S->tccpp_ch == '<') {
            c = '>';
            goto read_name;
        } else if (S->tccpp_ch == '\"') {
            c = S->tccpp_ch;
        read_name:
            inp(S);
            q = buf;
            while (S->tccpp_ch != c && S->tccpp_ch != '\n' && S->tccpp_ch != CH_EOF) {
                if ((q - buf) < sizeof(buf) - 1)
                    *q++ = S->tccpp_ch;
                if (S->tccpp_ch == '\\') {
                    if (handle_stray_noerror(S) == 0)
                        --q;
                } else
                    inp(S);
            }
            *q = '\0';
            minp(S);
#if 0
            /* eat all spaces and comments after include */
            /* XXX: slightly incorrect */
            while (ch1 != '\n' && ch1 != CH_EOF)
                inp(S);
#endif
        } else {
	    int len;
            /* computed #include : concatenate everything up to linefeed,
	       the result must be one of the two accepted forms.
	       Don't convert pp-tokens to tokens here.  */
	    S->tccpp_parse_flags = (PARSE_FLAG_PREPROCESS
			   | PARSE_FLAG_LINEFEED
			   | (S->tccpp_parse_flags & PARSE_FLAG_ASM_FILE));
            next(S);
            buf[0] = '\0';
	    while (S->tok != TOK_LINEFEED) {
		pstrcat(buf, sizeof(buf), get_tok_str(S, S->tok, &S->tokc));
		next(S);
	    }
	    len = strlen(buf);
	    /* check syntax and remove '<>|""' */
	    if ((len < 2 || ((buf[0] != '"' || buf[len-1] != '"') &&
			     (buf[0] != '<' || buf[len-1] != '>'))))
	        tcc_error(S, "'#include' expects \"FILENAME\" or <FILENAME>");
	    c = buf[len-1];
	    memmove(buf, buf + 1, len - 2);
	    buf[len - 2] = '\0';
        }

        if (S->include_stack_ptr >= S->include_stack + INCLUDE_STACK_SIZE)
            tcc_error(S,"#include recursion too deep");
        i = S->tok == TOK_INCLUDE_NEXT ? S->tccpp_file->include_next_index + 1 : 0;
        n = 2 + S->nb_include_paths + S->nb_sysinclude_paths;
        for (; i < n; ++i) {
            char buf1[sizeof S->tccpp_file->filename];
            CachedInclude *e;
            const char *path;

            if (i == 0) {
                /* check absolute include path */
                if (!IS_ABSPATH(buf))
                    continue;
                buf1[0] = 0;

            } else if (i == 1) {
                /* search in file's dir if "header.h" */
                if (c != '\"')
                    continue;
                /* https://savannah.nongnu.org/bugs/index.php?50847 */
                path = S->tccpp_file->true_filename;
                pstrncpy(buf1, path, tcc_basename(path) - path);

            } else {
                /* search in all the include paths */
                int j = i - 2, k = j - S->nb_include_paths;
                path = k < 0 ? S->include_paths[j] : S->sysinclude_paths[k];
                pstrcpy(buf1, sizeof(buf1), path);
                pstrcat(buf1, sizeof(buf1), "/");
            }

            pstrcat(buf1, sizeof(buf1), buf);
            e = search_cached_include(S, buf1, 0);
            if (e && (define_find(S, e->ifndef_macro) || e->once == S->tccpp_pp_once)) {
                /* no need to parse the include because the 'ifndef macro'
                   is defined (or had #pragma once) */
#ifdef INC_DEBUG
                printf("%s: skipping cached %s\n", S->tccpp_file->filename, buf1);
#endif
                goto include_done;
            }

            if (tcc_open(S, buf1) < 0)
                continue;
            /* push previous file on stack */
            *S->include_stack_ptr++ = S->tccpp_file->prev;
            S->tccpp_file->include_next_index = i;
#ifdef INC_DEBUG
            printf("%s: including %s\n", S->tccpp_file->prev->filename, S->tccpp_file->filename);
#endif
            /* update target deps */
            if (S->gen_deps) {
                BufferedFile *bf = S->tccpp_file;
                while (i == 1 && (bf = bf->prev))
                    i = bf->include_next_index;
                /* skip system include files */
                if (S->include_sys_deps || n - i > S->nb_sysinclude_paths)
                    dynarray_add(S, &S->target_deps, &S->nb_target_deps,
                        tcc_strdup(S, buf1));
            }
            /* add include file debug info */
            tcc_debug_bincl(S);
            S->tok_flags |= TOK_FLAG_BOF | TOK_FLAG_BOL;
            S->tccpp_ch = S->tccpp_file->buf_ptr[0];
            goto the_end;
        }
        tcc_error(S,"include file '%s' not found", buf);
include_done:
        break;
    case TOK_IFNDEF:
        c = 1;
        goto do_ifdef;
    case TOK_IF:
        c = expr_preprocess(S);
        goto do_if;
    case TOK_IFDEF:
        c = 0;
    do_ifdef:
        next_nomacro(S);
        if (S->tok < TOK_IDENT)
            tcc_error(S,"invalid argument for '#if%sdef'", c ? "n" : "");
        if (is_bof) {
            if (c) {
#ifdef INC_DEBUG
                printf("#ifndef %s\n", get_tok_str(S, S->tok, NULL));
#endif
                S->tccpp_file->ifndef_macro = S->tok;
            }
        }
        c = (define_find(S, S->tok) != 0) ^ c;
    do_if:
        if (S->ifdef_stack_ptr >= S->ifdef_stack + IFDEF_STACK_SIZE)
            tcc_error(S,"memory full (ifdef)");
        *S->ifdef_stack_ptr++ = c;
        goto test_skip;
    case TOK_ELSE:
        if (S->ifdef_stack_ptr == S->ifdef_stack)
            tcc_error(S,"#else without matching #if");
        if (S->ifdef_stack_ptr[-1] & 2)
            tcc_error(S,"#else after #else");
        c = (S->ifdef_stack_ptr[-1] ^= 3);
        goto test_else;
    case TOK_ELIF:
        if (S->ifdef_stack_ptr == S->ifdef_stack)
            tcc_error(S,"#elif without matching #if");
        c = S->ifdef_stack_ptr[-1];
        if (c > 1)
            tcc_error(S,"#elif after #else");
        /* last #if/#elif expression was true: we skip */
        if (c == 1) {
            c = 0;
        } else {
            c = expr_preprocess(S);
            S->ifdef_stack_ptr[-1] = c;
        }
    test_else:
        if (S->ifdef_stack_ptr == S->tccpp_file->ifdef_stack_ptr + 1)
            S->tccpp_file->ifndef_macro = 0;
    test_skip:
        if (!(c & 1)) {
            preprocess_skip(S);
            is_bof = 0;
            goto redo;
        }
        break;
    case TOK_ENDIF:
        if (S->ifdef_stack_ptr <= S->tccpp_file->ifdef_stack_ptr)
            tcc_error(S,"#endif without matching #if");
        S->ifdef_stack_ptr--;
        /* '#ifndef macro' was at the start of file. Now we check if
           an '#endif' is exactly at the end of file */
        if (S->tccpp_file->ifndef_macro &&
            S->ifdef_stack_ptr == S->tccpp_file->ifdef_stack_ptr) {
            S->tccpp_file->ifndef_macro_saved = S->tccpp_file->ifndef_macro;
            /* need to set to zero to avoid false matches if another
               #ifndef at middle of file */
            S->tccpp_file->ifndef_macro = 0;
            while (S->tok != TOK_LINEFEED)
                next_nomacro(S);
            S->tok_flags |= TOK_FLAG_ENDIF;
            goto the_end;
        }
        break;
    case TOK_PPNUM:
        n = strtoul((char*)S->tokc.str.data, &q, 10);
        goto _line_num;
    case TOK_LINE:
        next(S);
        if (S->tok != TOK_CINT)
    _line_err:
            tcc_error(S,"wrong #line format");
        n = S->tokc.i;
    _line_num:
        next(S);
        if (S->tok != TOK_LINEFEED) {
            if (S->tok == TOK_STR) {
                if (S->tccpp_file->true_filename == S->tccpp_file->filename)
                    S->tccpp_file->true_filename = tcc_strdup(S, S->tccpp_file->filename);
                /* prepend directory from real file */
                pstrcpy(buf, sizeof buf, S->tccpp_file->true_filename);
                *tcc_basename(buf) = 0;
                pstrcat(buf, sizeof buf, (char *)S->tokc.str.data);
                tcc_debug_putfile(S, buf);
            } else if (S->tccpp_parse_flags & PARSE_FLAG_ASM_FILE)
                break;
            else
                goto _line_err;
            --n;
        }
        if (S->tccpp_file->fd > 0)
            total_lines += S->tccpp_file->line_num - n;
        S->tccpp_file->line_num = n;
        break;
    case TOK_ERROR:
    case TOK_WARNING:
        c = S->tok;
        S->tccpp_ch = S->tccpp_file->buf_ptr[0];
        skip_spaces(S);
        q = buf;
        while (S->tccpp_ch != '\n' && S->tccpp_ch != CH_EOF) {
            if ((q - buf) < sizeof(buf) - 1)
                *q++ = S->tccpp_ch;
            if (S->tccpp_ch == '\\') {
                if (handle_stray_noerror(S) == 0)
                    --q;
            } else
                inp(S);
        }
        *q = '\0';
        if (c == TOK_ERROR)
            tcc_error(S,"#error %s", buf);
        else
            tcc_warning(S,"#warning %s", buf);
        break;
    case TOK_PRAGMA:
        pragma_parse(S);
        break;
    case TOK_LINEFEED:
        goto the_end;
    default:
        /* ignore gas line comment in an 'S' file. */
        if (saved_parse_flags & PARSE_FLAG_ASM_FILE)
            goto ignore;
        if (S->tok == '!' && is_bof)
            /* '!' is ignored at beginning to allow C scripts. */
            goto ignore;
        tcc_warning(S,"Ignoring unknown preprocessing directive #%s", get_tok_str(S, S->tok, &S->tokc));
    ignore:
        S->tccpp_file->buf_ptr = parse_line_comment(S, S->tccpp_file->buf_ptr - 1);
        goto the_end;
    }
    /* ignore other preprocess commands or #! for C scripts */
    while (S->tok != TOK_LINEFEED)
        next_nomacro(S);
 the_end:
    S->tccpp_parse_flags = saved_parse_flags;
}

/* evaluate escape codes in a string. */
static void parse_escape_string(TCCState *S, CString *outstr, const uint8_t *buf, int is_long)
{
    int c, n, i;
    const uint8_t *p;

    p = buf;
    for(;;) {
        c = *p;
        if (c == '\0')
            break;
        if (c == '\\') {
            p++;
            /* escape */
            c = *p;
            switch(c) {
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
                /* at most three octal digits */
                n = c - '0';
                p++;
                c = *p;
                if (isoct(c)) {
                    n = n * 8 + c - '0';
                    p++;
                    c = *p;
                    if (isoct(c)) {
                        n = n * 8 + c - '0';
                        p++;
                    }
                }
                c = n;
                goto add_char_nonext;
            case 'x': i = 0; goto parse_hex_or_ucn;
            case 'u': i = 4; goto parse_hex_or_ucn;
            case 'U': i = 8; goto parse_hex_or_ucn;
    parse_hex_or_ucn:
                p++;
                n = 0;
                do {
                    c = *p;
                    if (c >= 'a' && c <= 'f')
                        c = c - 'a' + 10;
                    else if (c >= 'A' && c <= 'F')
                        c = c - 'A' + 10;
                    else if (isnum(c))
                        c = c - '0';
                    else if (i > 0)
                        expect(S, "more hex digits in universal-character-name");
                    else {
                        c = n;
                        goto add_char_nonext;
                    }
                    n = n * 16 + c;
                    p++;
                } while (--i);
                cstr_u8cat(S, outstr, n);
                continue;
            case 'a':
                c = '\a';
                break;
            case 'b':
                c = '\b';
                break;
            case 'f':
                c = '\f';
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case 'v':
                c = '\v';
                break;
            case 'e':
                if (!gnu_ext)
                    goto invalid_escape;
                c = 27;
                break;
            case '\'':
            case '\"':
            case '\\': 
            case '?':
                break;
            default:
            invalid_escape:
                if (c >= '!' && c <= '~')
                    tcc_warning(S,"unknown escape sequence: \'\\%c\'", c);
                else
                    tcc_warning(S,"unknown escape sequence: \'\\x%x\'", c);
                break;
            }
        } else if (is_long && c >= 0x80) {
            /* assume we are processing UTF-8 sequence */
            /* reference: The Unicode Standard, Version 10.0, ch3.9 */

            int cont; /* count of continuation bytes */
            int skip; /* how many bytes should skip when error occurred */
            int i;

            /* decode leading byte */
            if (c < 0xC2) {
	            skip = 1; goto invalid_utf8_sequence;
            } else if (c <= 0xDF) {
	            cont = 1; n = c & 0x1f;
            } else if (c <= 0xEF) {
	            cont = 2; n = c & 0xf;
            } else if (c <= 0xF4) {
	            cont = 3; n = c & 0x7;
            } else {
	            skip = 1; goto invalid_utf8_sequence;
            }

            /* decode continuation bytes */
            for (i = 1; i <= cont; i++) {
                int l = 0x80, h = 0xBF;

                /* adjust limit for second byte */
                if (i == 1) {
                    switch (c) {
                    case 0xE0: l = 0xA0; break;
                    case 0xED: h = 0x9F; break;
                    case 0xF0: l = 0x90; break;
                    case 0xF4: h = 0x8F; break;
                    }
                }

                if (p[i] < l || p[i] > h) {
                    skip = i; goto invalid_utf8_sequence;
                }

                n = (n << 6) | (p[i] & 0x3f);
            }

            /* advance pointer */
            p += 1 + cont;
            c = n;
            goto add_char_nonext;

            /* error handling */
        invalid_utf8_sequence:
            tcc_warning(S,"ill-formed UTF-8 subsequence starting with: \'\\x%x\'", c);
            c = 0xFFFD;
            p += skip;
            goto add_char_nonext;

        }
        p++;
    add_char_nonext:
        if (!is_long)
            cstr_ccat(S, outstr, c);
        else {
#ifdef TCC_TARGET_PE
            /* store as UTF-16 */
            if (c < 0x10000) {
                cstr_wccat(S, outstr, c);
            } else {
                c -= 0x10000;
                cstr_wccat(S, outstr, (c >> 10) + 0xD800);
                cstr_wccat(S, outstr, (c & 0x3FF) + 0xDC00);
            }
#else
            cstr_wccat(S, outstr, c);
#endif
        }
    }
    /* add a trailing '\0' */
    if (!is_long)
        cstr_ccat(S, outstr, '\0');
    else
        cstr_wccat(S, outstr, '\0');
}

static void parse_string(TCCState *S, const char *s, int len)
{
    uint8_t buf[1000], *p = buf;
    int is_long, sep;

    if ((is_long = *s == 'L'))
        ++s, --len;
    sep = *s++;
    len -= 2;
    if (len >= sizeof buf)
        p = tcc_malloc(S, len + 1);
    memcpy(p, s, len);
    p[len] = 0;

    cstr_reset(&S->tokcstr);
    parse_escape_string(S, &S->tokcstr, p, is_long);
    if (p != buf)
        tcc_free(S, p);

    if (sep == '\'') {
        int char_size, i, n, c;
        /* XXX: make it portable */
        if (!is_long)
            S->tok = TOK_CCHAR, char_size = 1;
        else
            S->tok = TOK_LCHAR, char_size = sizeof(nwchar_t);
        n = S->tokcstr.size / char_size - 1;
        if (n < 1)
            tcc_error(S,"empty character constant");
        if (n > 1)
            tcc_warning_c(warn_all)(S,"multi-character character constant");
        for (c = i = 0; i < n; ++i) {
            if (is_long)
                c = ((nwchar_t *)S->tokcstr.data)[i];
            else
                c = (c << 8) | ((char *)S->tokcstr.data)[i];
        }
        S->tokc.i = c;
    } else {
        S->tokc.str.size = S->tokcstr.size;
        S->tokc.str.data = S->tokcstr.data;
        if (!is_long)
            S->tok = TOK_STR;
        else
            S->tok = TOK_LSTR;
    }
}

/* we use 64 bit numbers */
#define BN_SIZE 2

/* bn = (bn << shift) | or_val */
static void bn_lshift(unsigned int *bn, int shift, int or_val)
{
    int i;
    unsigned int v;
    for(i=0;i<BN_SIZE;i++) {
        v = bn[i];
        bn[i] = (v << shift) | or_val;
        or_val = v >> (32 - shift);
    }
}

static void bn_zero(unsigned int *bn)
{
    int i;
    for(i=0;i<BN_SIZE;i++) {
        bn[i] = 0;
    }
}

/* parse number in null terminated string 'p' and return it in the
   current token */
static void parse_number(TCCState *S, const char *p)
{
    int b, t, shift, frac_bits, s, exp_val, ch;
    char *q;
    unsigned int bn[BN_SIZE];
    double d;

    /* number */
    q = S->token_buf;
    ch = *p++;
    t = ch;
    ch = *p++;
    *q++ = t;
    b = 10;
    if (t == '.') {
        goto float_frac_parse;
    } else if (t == '0') {
        if (ch == 'x' || ch == 'X') {
            q--;
            ch = *p++;
            b = 16;
        } else if (S->tcc_ext && (ch == 'b' || ch == 'B')) {
            q--;
            ch = *p++;
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
        if (q >= S->token_buf + STRING_MAX_SIZE) {
        num_too_long:
            tcc_error(S,"number too long");
        }
        *q++ = ch;
        ch = *p++;
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
                shift = 1;
            bn_zero(bn);
            q = S->token_buf;
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
                ch = *p++;
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
                        tcc_error(S,"invalid digit");
                    bn_lshift(bn, shift, t);
                    frac_bits += shift;
                    ch = *p++;
                }
            }
            if (ch != 'p' && ch != 'P')
                expect(S, "exponent");
            ch = *p++;
            s = 1;
            exp_val = 0;
            if (ch == '+') {
                ch = *p++;
            } else if (ch == '-') {
                s = -1;
                ch = *p++;
            }
            if (ch < '0' || ch > '9')
                expect(S, "exponent digits");
            while (ch >= '0' && ch <= '9') {
                exp_val = exp_val * 10 + ch - '0';
                ch = *p++;
            }
            exp_val = exp_val * s;
            
            /* now we can generate the number */
            /* XXX: should patch directly float number */
            d = (double)bn[1] * 4294967296.0 + (double)bn[0];
            d = ldexp(d, exp_val - frac_bits);
            t = toup(ch);
            if (t == 'F') {
                ch = *p++;
                S->tok = TOK_CFLOAT;
                /* float : should handle overflow */
                S->tokc.f = (float)d;
            } else if (t == 'L') {
                ch = *p++;
#ifdef TCC_USING_DOUBLE_FOR_LDOUBLE
                S->tok = TOK_CDOUBLE;
                S->tokc.d = d;
#else
                S->tok = TOK_CLDOUBLE;
                /* XXX: not large enough */
                S->tokc.ld = (long double)d;
#endif
            } else {
                S->tok = TOK_CDOUBLE;
                S->tokc.d = d;
            }
        } else {
            /* decimal floats */
            if (ch == '.') {
                if (q >= S->token_buf + STRING_MAX_SIZE)
                    goto num_too_long;
                *q++ = ch;
                ch = *p++;
            float_frac_parse:
                while (ch >= '0' && ch <= '9') {
                    if (q >= S->token_buf + STRING_MAX_SIZE)
                        goto num_too_long;
                    *q++ = ch;
                    ch = *p++;
                }
            }
            if (ch == 'e' || ch == 'E') {
                if (q >= S->token_buf + STRING_MAX_SIZE)
                    goto num_too_long;
                *q++ = ch;
                ch = *p++;
                if (ch == '-' || ch == '+') {
                    if (q >= S->token_buf + STRING_MAX_SIZE)
                        goto num_too_long;
                    *q++ = ch;
                    ch = *p++;
                }
                if (ch < '0' || ch > '9')
                    expect(S, "exponent digits");
                while (ch >= '0' && ch <= '9') {
                    if (q >= S->token_buf + STRING_MAX_SIZE)
                        goto num_too_long;
                    *q++ = ch;
                    ch = *p++;
                }
            }
            *q = '\0';
            t = toup(ch);
            errno = 0;
            if (t == 'F') {
                ch = *p++;
                S->tok = TOK_CFLOAT;
                S->tokc.f = strtof(S->token_buf, NULL);
            } else if (t == 'L') {
                ch = *p++;
#ifdef TCC_USING_DOUBLE_FOR_LDOUBLE
                S->tok = TOK_CDOUBLE;
                S->tokc.d = strtod(S->token_buf, NULL);
#else
                S->tok = TOK_CLDOUBLE;
                S->tokc.ld = strtold(S->token_buf, NULL);
#endif
            } else {
                S->tok = TOK_CDOUBLE;
                S->tokc.d = strtod(S->token_buf, NULL);
            }
        }
    } else {
        unsigned long long n, n1;
        int lcount, ucount, ov = 0;
        const char *p1;

        /* integer number */
        *q = '\0';
        q = S->token_buf;
        if (b == 10 && *q == '0') {
            b = 8;
            q++;
        }
        n = 0;
        while(1) {
            t = *q++;
            /* no need for checks except for base 10 / 8 errors */
            if (t == '\0')
                break;
            else if (t >= 'a')
                t = t - 'a' + 10;
            else if (t >= 'A')
                t = t - 'A' + 10;
            else
                t = t - '0';
            if (t >= b)
                tcc_error(S,"invalid digit");
            n1 = n;
            n = n * b + t;
            /* detect overflow */
            if (n1 >= 0x1000000000000000ULL && n / b != n1)
                ov = 1;
        }

        /* Determine the characteristics (unsigned and/or 64bit) the type of
           the constant must have according to the constant suffix(es) */
        lcount = ucount = 0;
        p1 = p;
        for(;;) {
            t = toup(ch);
            if (t == 'L') {
                if (lcount >= 2)
                    tcc_error(S,"three 'l's in integer constant");
                if (lcount && *(p - 1) != ch)
                    tcc_error(S,"incorrect integer suffix: %s", p1);
                lcount++;
                ch = *p++;
            } else if (t == 'U') {
                if (ucount >= 1)
                    tcc_error(S,"two 'u's in integer constant");
                ucount++;
                ch = *p++;
            } else {
                break;
            }
        }

        /* Determine if it needs 64 bits and/or unsigned in order to fit */
        if (ucount == 0 && b == 10) {
            if (lcount <= (LONG_SIZE == 4)) {
                if (n >= 0x80000000U)
                    lcount = (LONG_SIZE == 4) + 1;
            }
            if (n >= 0x8000000000000000ULL)
                ov = 1, ucount = 1;
        } else {
            if (lcount <= (LONG_SIZE == 4)) {
                if (n >= 0x100000000ULL)
                    lcount = (LONG_SIZE == 4) + 1;
                else if (n >= 0x80000000U)
                    ucount = 1;
            }
            if (n >= 0x8000000000000000ULL)
                ucount = 1;
        }

        if (ov)
            tcc_warning(S, "integer constant overflow");

        S->tok = TOK_CINT;
	if (lcount) {
            S->tok = TOK_CLONG;
            if (lcount == 2)
                S->tok = TOK_CLLONG;
	}
	if (ucount)
	    ++S->tok; /* TOK_CU... */
        S->tokc.i = n;
    }
    if (ch)
        tcc_error(S,"invalid number");
}


#define PARSE2(c1, tok1, c2, tok2)              \
    case c1:                                    \
        PEEKC(c, p);                            \
        if (c == c2) {                          \
            p++;                                \
            S->tok = tok2;                         \
        } else {                                \
            S->tok = tok1;                         \
        }                                       \
        break;

/* return next token without macro substitution */
static inline void next_nomacro1(TCCState *S)
{
    int t, c, is_long, len;
    TokenSym *ts;
    uint8_t *p, *p1;
    unsigned int h;

    p = S->tccpp_file->buf_ptr;
 redo_no_start:
    c = *p;
    switch(c) {
    case ' ':
    case '\t':
        S->tok = c;
        p++;
 maybe_space:
        if (S->tccpp_parse_flags & PARSE_FLAG_SPACES)
            goto keep_tok_flags;
        while (S->tccpp_isidnum_table[*p - CH_EOF] & IS_SPC)
            ++p;
        goto redo_no_start;
    case '\f':
    case '\v':
    case '\r':
        p++;
        goto redo_no_start;
    case '\\':
        /* first look if it is in fact an end of buffer */
        c = handle_stray1(S, p);
        p = S->tccpp_file->buf_ptr;
        if (c == '\\')
            goto parse_simple;
        if (c != CH_EOF)
            goto redo_no_start;
        {
            if ((S->tccpp_parse_flags & PARSE_FLAG_LINEFEED)
                && !(S->tok_flags & TOK_FLAG_EOF)) {
                S->tok_flags |= TOK_FLAG_EOF;
                S->tok = TOK_LINEFEED;
                goto keep_tok_flags;
            } else if (!(S->tccpp_parse_flags & PARSE_FLAG_PREPROCESS)) {
                S->tok = TOK_EOF;
            } else if (S->ifdef_stack_ptr != S->tccpp_file->ifdef_stack_ptr) {
                tcc_error(S,"missing #endif");
            } else if (S->include_stack_ptr == S->include_stack) {
                /* no include left : end of file. */
                S->tok = TOK_EOF;
            } else {
                S->tok_flags &= ~TOK_FLAG_EOF;
                /* pop include file */
                
                /* test if previous '#endif' was after a #ifdef at
                   start of file */
                if (S->tok_flags & TOK_FLAG_ENDIF) {
#ifdef INC_DEBUG
                    printf("#endif %s\n", get_tok_str(S, S->tccpp_file->ifndef_macro_saved, NULL));
#endif
                    search_cached_include(S, S->tccpp_file->filename, 1)
                        ->ifndef_macro = S->tccpp_file->ifndef_macro_saved;
                    S->tok_flags &= ~TOK_FLAG_ENDIF;
                }

                /* add end of include file debug info */
                tcc_debug_eincl(S);
                /* pop include stack */
                tcc_close(S);
                S->include_stack_ptr--;
                p = S->tccpp_file->buf_ptr;
                if (p == S->tccpp_file->buffer)
                    S->tok_flags = TOK_FLAG_BOF|TOK_FLAG_BOL;
                goto redo_no_start;
            }
        }
        break;

    case '\n':
        S->tccpp_file->line_num++;
        S->tok_flags |= TOK_FLAG_BOL;
        p++;
maybe_newline:
        if (0 == (S->tccpp_parse_flags & PARSE_FLAG_LINEFEED))
            goto redo_no_start;
        S->tok = TOK_LINEFEED;
        goto keep_tok_flags;

    case '#':
        /* XXX: simplify */
        PEEKC(c, p);
        if ((S->tok_flags & TOK_FLAG_BOL) && 
            (S->tccpp_parse_flags & PARSE_FLAG_PREPROCESS)) {
            S->tccpp_file->buf_ptr = p;
            preprocess(S, S->tok_flags & TOK_FLAG_BOF);
            p = S->tccpp_file->buf_ptr;
            goto maybe_newline;
        } else {
            if (c == '#') {
                p++;
                S->tok = TOK_TWOSHARPS;
            } else {
#if !defined(TCC_TARGET_ARM)
                if (S->tccpp_parse_flags & PARSE_FLAG_ASM_FILE) {
                    p = parse_line_comment(S, p - 1);
                    goto redo_no_start;
                } else
#endif
                {
                    S->tok = '#';
                }
            }
        }
        break;
    
    /* dollar is allowed to start identifiers when not parsing asm */
    case '$':
        if (!(S->tccpp_isidnum_table[c - CH_EOF] & IS_ID)
         || (S->tccpp_parse_flags & PARSE_FLAG_ASM_FILE))
            goto parse_simple;

    case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': 
    case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': 
    case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': 
    case '_':
    parse_ident_fast:
        p1 = p;
        h = TOK_HASH_INIT;
        h = TOK_HASH_FUNC(h, c);
        while (c = *++p, S->tccpp_isidnum_table[c - CH_EOF] & (IS_ID|IS_NUM))
            h = TOK_HASH_FUNC(h, c);
        len = p - p1;
        if (c != '\\') {
            TokenSym **pts;

            /* fast case : no stray found, so we have the full token
               and we have already hashed it */
            h &= (TOK_HASH_SIZE - 1);
            pts = &S->tccpp_hash_ident[h];
            for(;;) {
                ts = *pts;
                if (!ts)
                    break;
                if (ts->len == len && !memcmp(ts->str, p1, len))
                    goto token_found;
                pts = &(ts->hash_next);
            }
            ts = tok_alloc_new(S, pts, (char *) p1, len);
        token_found: ;
        } else {
            /* slower case */
            cstr_reset(&S->tokcstr);
            cstr_cat(S, &S->tokcstr, (char *) p1, len);
            p--;
            PEEKC(c, p);
        parse_ident_slow:
            while (S->tccpp_isidnum_table[c - CH_EOF] & (IS_ID|IS_NUM))
            {
                cstr_ccat(S, &S->tokcstr, c);
                PEEKC(c, p);
            }
            ts = tok_alloc(S, S->tokcstr.data, S->tokcstr.size);
        }
        S->tok = ts->tok;
        break;
    case 'L':
        t = p[1];
        if (t != '\\' && t != '\'' && t != '\"') {
            /* fast case */
            goto parse_ident_fast;
        } else {
            PEEKC(c, p);
            if (c == '\'' || c == '\"') {
                is_long = 1;
                goto str_const;
            } else {
                cstr_reset(&S->tokcstr);
                cstr_ccat(S, &S->tokcstr, 'L');
                goto parse_ident_slow;
            }
        }
        break;

    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
    case '8': case '9':
        t = c;
        PEEKC(c, p);
        /* after the first digit, accept digits, alpha, '.' or sign if
           prefixed by 'eEpP' */
    parse_num:
        cstr_reset(&S->tokcstr);
        for(;;) {
            cstr_ccat(S, &S->tokcstr, t);
            if (!((S->tccpp_isidnum_table[c - CH_EOF] & (IS_ID|IS_NUM))
                  || c == '.'
                  || ((c == '+' || c == '-')
                      && (((t == 'e' || t == 'E')
                            && !(S->tccpp_parse_flags & PARSE_FLAG_ASM_FILE
                                /* 0xe+1 is 3 tokens in asm */
                                && ((char*)S->tokcstr.data)[0] == '0'
                                && toup(((char*)S->tokcstr.data)[1]) == 'X'))
                          || t == 'p' || t == 'P'))))
                break;
            t = c;
            PEEKC(c, p);
        }
        /* We add a trailing '\0' to ease parsing */
        cstr_ccat(S, &S->tokcstr, '\0');
        S->tokc.str.size = S->tokcstr.size;
        S->tokc.str.data = S->tokcstr.data;
        S->tok = TOK_PPNUM;
        break;

    case '.':
        /* special dot handling because it can also start a number */
        PEEKC(c, p);
        if (isnum(c)) {
            t = '.';
            goto parse_num;
        } else if ((S->tccpp_isidnum_table['.' - CH_EOF] & IS_ID)
                   && (S->tccpp_isidnum_table[c - CH_EOF] & (IS_ID|IS_NUM))) {
            *--p = c = '.';
            goto parse_ident_fast;
        } else if (c == '.') {
            PEEKC(c, p);
            if (c == '.') {
                p++;
                S->tok = TOK_DOTS;
            } else {
                *--p = '.'; /* may underflow into file->unget[] */
                S->tok = '.';
            }
        } else {
            S->tok = '.';
        }
        break;
    case '\'':
    case '\"':
        is_long = 0;
    str_const:
        cstr_reset(&S->tokcstr);
        if (is_long)
            cstr_ccat(S, &S->tokcstr, 'L');
        cstr_ccat(S, &S->tokcstr, c);
        p = parse_pp_string(S, p, c, &S->tokcstr);
        cstr_ccat(S, &S->tokcstr, c);
        cstr_ccat(S, &S->tokcstr, '\0');
        S->tokc.str.size = S->tokcstr.size;
        S->tokc.str.data = S->tokcstr.data;
        S->tok = TOK_PPSTR;
        break;

    case '<':
        PEEKC(c, p);
        if (c == '=') {
            p++;
            S->tok = TOK_LE;
        } else if (c == '<') {
            PEEKC(c, p);
            if (c == '=') {
                p++;
                S->tok = TOK_A_SHL;
            } else {
                S->tok = TOK_SHL;
            }
        } else {
            S->tok = TOK_LT;
        }
        break;
    case '>':
        PEEKC(c, p);
        if (c == '=') {
            p++;
            S->tok = TOK_GE;
        } else if (c == '>') {
            PEEKC(c, p);
            if (c == '=') {
                p++;
                S->tok = TOK_A_SAR;
            } else {
                S->tok = TOK_SAR;
            }
        } else {
            S->tok = TOK_GT;
        }
        break;
        
    case '&':
        PEEKC(c, p);
        if (c == '&') {
            p++;
            S->tok = TOK_LAND;
        } else if (c == '=') {
            p++;
            S->tok = TOK_A_AND;
        } else {
            S->tok = '&';
        }
        break;
        
    case '|':
        PEEKC(c, p);
        if (c == '|') {
            p++;
            S->tok = TOK_LOR;
        } else if (c == '=') {
            p++;
            S->tok = TOK_A_OR;
        } else {
            S->tok = '|';
        }
        break;

    case '+':
        PEEKC(c, p);
        if (c == '+') {
            p++;
            S->tok = TOK_INC;
        } else if (c == '=') {
            p++;
            S->tok = TOK_A_ADD;
        } else {
            S->tok = '+';
        }
        break;
        
    case '-':
        PEEKC(c, p);
        if (c == '-') {
            p++;
            S->tok = TOK_DEC;
        } else if (c == '=') {
            p++;
            S->tok = TOK_A_SUB;
        } else if (c == '>') {
            p++;
            S->tok = TOK_ARROW;
        } else {
            S->tok = '-';
        }
        break;

    PARSE2('!', '!', '=', TOK_NE)
    PARSE2('=', '=', '=', TOK_EQ)
    PARSE2('*', '*', '=', TOK_A_MUL)
    PARSE2('%', '%', '=', TOK_A_MOD)
    PARSE2('^', '^', '=', TOK_A_XOR)
        
        /* comments or operator */
    case '/':
        PEEKC(c, p);
        if (c == '*') {
            p = parse_comment(S, p);
            /* comments replaced by a blank */
            S->tok = ' ';
            goto maybe_space;
        } else if (c == '/') {
            p = parse_line_comment(S, p);
            S->tok = ' ';
            goto maybe_space;
        } else if (c == '=') {
            p++;
            S->tok = TOK_A_DIV;
        } else {
            S->tok = '/';
        }
        break;
        
        /* simple tokens */
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
    case ',':
    case ';':
    case ':':
    case '?':
    case '~':
    case '@': /* only used in assembler */
    parse_simple:
        S->tok = c;
        p++;
        break;
    default:
        if (c >= 0x80 && c <= 0xFF) /* utf8 identifiers */
	    goto parse_ident_fast;
        if (S->tccpp_parse_flags & PARSE_FLAG_ASM_FILE)
            goto parse_simple;
        tcc_error(S,"unrecognized character \\x%02x", c);
        break;
    }
    S->tok_flags = 0;
keep_tok_flags:
    S->tccpp_file->buf_ptr = p;
#if defined(PARSE_DEBUG)
    printf("token = %d %s\n", S->tok, get_tok_str(S, S->tok, &S->tokc));
#endif
}

static void macro_subst(TCCState *S,
    TokenString *tok_str,
    Sym **nested_list,
    const int *macro_str
    );

/* substitute arguments in replacement lists in macro_str by the values in
   args (field d) and return allocated string */
static int *macro_arg_subst(TCCState *S, Sym **nested_list, const int *macro_str, Sym *args)
{
    int t, t0, t1, spc;
    const int *st;
    Sym *s;
    CValue cval;
    TokenString str;
    CString cstr;

    tok_str_new(&str);
    t0 = t1 = 0;
    while(1) {
        TOK_GET(&t, &macro_str, &cval);
        if (!t)
            break;
        if (t == '#') {
            /* stringize */
            TOK_GET(&t, &macro_str, &cval);
            if (!t)
                goto bad_stringy;
            s = sym_find2(args, t);
            if (s) {
                cstr_new(S, &cstr);
                cstr_ccat(S, &cstr, '\"');
                st = s->d;
                spc = 0;
                while (*st >= 0) {
                    TOK_GET(&t, &st, &cval);
                    if (t != TOK_PLCHLDR
                     && t != TOK_NOSUBST
                     && 0 == check_space(S, t, &spc)) {
                        const char *s = get_tok_str(S, t, &cval);
                        while (*s) {
                            if (t == TOK_PPSTR && *s != '\'')
                                add_char(S, &cstr, *s);
                            else
                                cstr_ccat(S, &cstr, *s);
                            ++s;
                        }
                    }
                }
                cstr.size -= spc;
                cstr_ccat(S, &cstr, '\"');
                cstr_ccat(S, &cstr, '\0');
#ifdef PP_DEBUG
                printf("\nstringize: <%s>\n", (char *)cstr.data);
#endif
                /* add string */
                cval.str.size = cstr.size;
                cval.str.data = cstr.data;
                tok_str_add2(S, &str, TOK_PPSTR, &cval);
                cstr_free(S, &cstr);
            } else {
        bad_stringy:
                expect(S, "macro parameter after '#'");
            }
        } else if (t >= TOK_IDENT) {
            s = sym_find2(args, t);
            if (s) {
                int l0 = str.len;
                st = s->d;
                /* if '##' is present before or after, no arg substitution */
                if (*macro_str == TOK_PPJOIN || t1 == TOK_PPJOIN) {
                    /* special case for var arg macros : ## eats the ','
                       if empty VA_ARGS variable. */
                    if (t1 == TOK_PPJOIN && t0 == ',' && gnu_ext && s->type.t) {
                        if (*st <= 0) {
                            /* suppress ',' '##' */
                            str.len -= 2;
                        } else {
                            /* suppress '##' and add variable */
                            str.len--;
                            goto add_var;
                        }
                    }
                } else {
            add_var:
		    if (!s->next) {
			/* Expand arguments tokens and store them.  In most
			   cases we could also re-expand each argument if
			   used multiple times, but not if the argument
			   contains the __COUNTER__ macro.  */
			TokenString str2;
			sym_push2(S, &s->next, s->v, s->type.t, 0);
			tok_str_new(&str2);
			macro_subst(S, &str2, nested_list, st);
			tok_str_add(S, &str2, 0);
			s->next->d = str2.str;
		    }
		    st = s->next->d;
                }
                for(;;) {
                    int t2;
                    TOK_GET(&t2, &st, &cval);
                    if (t2 <= 0)
                        break;
                    tok_str_add2(S, &str, t2, &cval);
                }
                if (str.len == l0) /* expanded to empty string */
                    tok_str_add(S, &str, TOK_PLCHLDR);
            } else {
                tok_str_add(S, &str, t);
            }
        } else {
            tok_str_add2(S, &str, t, &cval);
        }
        t0 = t1, t1 = t;
    }
    tok_str_add(S, &str, 0);
    return str.str;
}

static char const ab_month_name[12][4] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int paste_tokens(TCCState *S, int t1, CValue *v1, int t2, CValue *v2)
{
    CString cstr;
    int n, ret = 1;

    cstr_new(S, &cstr);
    if (t1 != TOK_PLCHLDR)
        cstr_cat(S, &cstr, get_tok_str(S, t1, v1), -1);
    n = cstr.size;
    if (t2 != TOK_PLCHLDR)
        cstr_cat(S, &cstr, get_tok_str(S, t2, v2), -1);
    cstr_ccat(S, &cstr, '\0');

    tcc_open_bf(S, ":paste:", cstr.size);
    memcpy(S->tccpp_file->buffer, cstr.data, cstr.size);
    S->tok_flags = 0;
    for (;;) {
        next_nomacro1(S);
        if (0 == *S->tccpp_file->buf_ptr)
            break;
        if (is_space(S->tok))
            continue;
        tcc_warning(S, "pasting \"%.*s\" and \"%s\" does not give a valid"
            " preprocessing token", n, (char *)cstr.data, (char*)cstr.data + n);
        ret = 0;
        break;
    }
    tcc_close(S);
    //printf("paste <%s>\n", (char*)cstr.data);
    cstr_free(S, &cstr);
    return ret;
}

/* handle the '##' operator. Return NULL if no '##' seen. Otherwise
   return the resulting string (which must be freed). */
static inline int *macro_twosharps(TCCState *S, const int *ptr0)
{
    int t;
    CValue cval;
    TokenString macro_str1;
    int start_of_nosubsts = -1;
    const int *ptr;

    /* we search the first '##' */
    for (ptr = ptr0;;) {
        TOK_GET(&t, &ptr, &cval);
        if (t == TOK_PPJOIN)
            break;
        if (t == 0)
            return NULL;
    }

    tok_str_new(&macro_str1);

    //tok_print(" $$$", ptr0);
    for (ptr = ptr0;;) {
        TOK_GET(&t, &ptr, &cval);
        if (t == 0)
            break;
        if (t == TOK_PPJOIN)
            continue;
        while (*ptr == TOK_PPJOIN) {
            int t1; CValue cv1;
            /* given 'a##b', remove nosubsts preceding 'a' */
            if (start_of_nosubsts >= 0)
                macro_str1.len = start_of_nosubsts;
            /* given 'a##b', remove nosubsts preceding 'b' */
            while ((t1 = *++ptr) == TOK_NOSUBST)
                ;
            if (t1 && t1 != TOK_PPJOIN) {
                TOK_GET(&t1, &ptr, &cv1);
                if (t != TOK_PLCHLDR || t1 != TOK_PLCHLDR) {
                    if (paste_tokens(S, t, &cval, t1, &cv1)) {
                        t = S->tok, cval = S->tokc;
                    } else {
                        tok_str_add2(S, &macro_str1, t, &cval);
                        t = t1, cval = cv1;
                    }
                }
            }
        }
        if (t == TOK_NOSUBST) {
            if (start_of_nosubsts < 0)
                start_of_nosubsts = macro_str1.len;
        } else {
            start_of_nosubsts = -1;
        }
        tok_str_add2(S, &macro_str1, t, &cval);
    }
    tok_str_add(S, &macro_str1, 0);
    //tok_print(" ###", macro_str1.str);
    return macro_str1.str;
}

/* peek or read [ws_str == NULL] next token from function macro call,
   walking up macro levels up to the file if necessary */
static int next_argstream(TCCState *S, Sym **nested_list, TokenString *ws_str)
{
    int t;
    const int *p;
    Sym *sa;

    for (;;) {
        if (S->tccpp_macro_ptr) {
            p = S->tccpp_macro_ptr, t = *p;
            if (ws_str) {
                while (is_space(t) || TOK_LINEFEED == t || TOK_PLCHLDR == t)
                    tok_str_add(S, ws_str, t), t = *++p;
            }
            if (t == 0) {
                end_macro(S);
                /* also, end of scope for nested defined symbol */
                sa = *nested_list;
                while (sa && sa->v == 0)
                    sa = sa->prev;
                if (sa)
                    sa->v = 0;
                continue;
            }
        } else {
            S->tccpp_ch = handle_eob(S);
            if (ws_str) {
                while (is_space(S->tccpp_ch) || S->tccpp_ch == '\n' || S->tccpp_ch == '/') {
                    if (S->tccpp_ch == '/') {
                        int c;
                        uint8_t *p = S->tccpp_file->buf_ptr;
                        PEEKC(c, p);
                        if (c == '*') {
                            p = parse_comment(S, p);
                            S->tccpp_file->buf_ptr = p - 1;
                        } else if (c == '/') {
                            p = parse_line_comment(S, p);
                            S->tccpp_file->buf_ptr = p - 1;
                        } else
                            break;
                        S->tccpp_ch = ' ';
                    }
                    if (S->tccpp_ch == '\n')
                        S->tccpp_file->line_num++;
                    if (!(S->tccpp_ch == '\f' || S->tccpp_ch == '\v' || S->tccpp_ch == '\r'))
                        tok_str_add(S, ws_str, S->tccpp_ch);
                    cinp(S);
                }
            }
            t = S->tccpp_ch;
        }

        if (ws_str)
            return t;
        next_nomacro(S);
        return S->tok;
    }
}

/* do macro substitution of current token with macro 's' and add
   result to (tok_str,tok_len). 'nested_list' is the list of all
   macros we got inside to avoid recursing. Return non zero if no
   substitution needs to be done */
static int macro_subst_tok(TCCState *S,
    TokenString *tok_str,
    Sym **nested_list,
    Sym *s)
{
    Sym *args, *sa, *sa1;
    int parlevel, t, t1, spc;
    TokenString str;
    char *cstrval;
    CValue cval;
    CString cstr;
    char buf[32];

    /* if symbol is a macro, prepare substitution */
    /* special macros */
    if (S->tok == TOK___LINE__ || S->tok == TOK___COUNTER__) {
        t = S->tok == TOK___LINE__ ? S->tccpp_file->line_num : S->tccpp_pp_counter++;
        snprintf(buf, sizeof(buf), "%d", t);
        cstrval = buf;
        t1 = TOK_PPNUM;
        goto add_cstr1;
    } else if (S->tok == TOK___FILE__) {
        cstrval = S->tccpp_file->filename;
        goto add_cstr;
    } else if (S->tok == TOK___DATE__ || S->tok == TOK___TIME__) {
        time_t ti;
        struct tm *tm;

        time(&ti);
        tm = localtime(&ti);
        if (S->tok == TOK___DATE__) {
            snprintf(buf, sizeof(buf), "%s %2d %d", 
                     ab_month_name[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
        } else {
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", 
                     tm->tm_hour, tm->tm_min, tm->tm_sec);
        }
        cstrval = buf;
    add_cstr:
        t1 = TOK_STR;
    add_cstr1:
        cstr_new(S, &cstr);
        cstr_cat(S, &cstr, cstrval, 0);
        cval.str.size = cstr.size;
        cval.str.data = cstr.data;
        tok_str_add2(S, tok_str, t1, &cval);
        cstr_free(S, &cstr);
    } else if (s->d) {
        int saved_parse_flags = S->tccpp_parse_flags;
	int *joined_str = NULL;
        int *mstr = s->d;

        if (s->type.t == MACRO_FUNC) {
            /* whitespace between macro name and argument list */
            TokenString ws_str;
            tok_str_new(&ws_str);

            spc = 0;
            S->tccpp_parse_flags |= PARSE_FLAG_SPACES | PARSE_FLAG_LINEFEED
                | PARSE_FLAG_ACCEPT_STRAYS;

            /* get next token from argument stream */
            t = next_argstream(S, nested_list, &ws_str);
            if (t != '(') {
                /* not a macro substitution after all, restore the
                 * macro token plus all whitespace we've read.
                 * whitespace is intentionally not merged to preserve
                 * newlines. */
                S->tccpp_parse_flags = saved_parse_flags;
                tok_str_add(S, tok_str, S->tok);
                if (S->tccpp_parse_flags & PARSE_FLAG_SPACES) {
                    int i;
                    for (i = 0; i < ws_str.len; i++)
                        tok_str_add(S, tok_str, ws_str.str[i]);
                }
                tok_str_free_str(S, ws_str.str);
                return 0;
            } else {
                tok_str_free_str(S, ws_str.str);
            }
	    do {
		next_nomacro(S); /* eat '(' */
	    } while (S->tok == TOK_PLCHLDR || is_space(S->tok));

            /* argument macro */
            args = NULL;
            sa = s->next;
            /* NOTE: empty args are allowed, except if no args */
            for(;;) {
                do {
                    next_argstream(S, nested_list, NULL);
                } while (S->tok == TOK_PLCHLDR || is_space(S->tok) ||
			 TOK_LINEFEED == S->tok);
    empty_arg:
                /* handle '()' case */
                if (!args && !sa && S->tok == ')')
                    break;
                if (!sa)
                    tcc_error(S,"macro '%s' used with too many args",
                          get_tok_str(S, s->v, 0));
                tok_str_new(&str);
                parlevel = spc = 0;
                /* NOTE: non zero sa->t indicates VA_ARGS */
                while ((parlevel > 0 || 
                        (S->tok != ')' && 
                         (S->tok != ',' || sa->type.t)))) {
                    if (S->tok == TOK_EOF || S->tok == 0)
                        break;
                    if (S->tok == '(')
                        parlevel++;
                    else if (S->tok == ')')
                        parlevel--;
                    if (S->tok == TOK_LINEFEED)
                        S->tok = ' ';
                    if (!check_space(S, S->tok, &spc))
                        tok_str_add2(S, &str, S->tok, &S->tokc);
                    next_argstream(S, nested_list, NULL);
                }
                if (parlevel)
                    expect(S, ")");
                str.len -= spc;
                tok_str_add(S, &str, -1);
                tok_str_add(S, &str, 0);
                sa1 = sym_push2(S, &args, sa->v & ~SYM_FIELD, sa->type.t, 0);
                sa1->d = str.str;
                sa = sa->next;
                if (S->tok == ')') {
                    /* special case for gcc var args: add an empty
                       var arg argument if it is omitted */
                    if (sa && sa->type.t && gnu_ext)
                        goto empty_arg;
                    break;
                }
                if (S->tok != ',')
                    expect(S, ",");
            }
            if (sa) {
                tcc_error(S,"macro '%s' used with too few args",
                      get_tok_str(S, s->v, 0));
            }

            /* now subst each arg */
            mstr = macro_arg_subst(S, nested_list, mstr, args);
            /* free memory */
            sa = args;
            while (sa) {
                sa1 = sa->prev;
                tok_str_free_str(S, sa->d);
                if (sa->next) {
                    tok_str_free_str(S, sa->next->d);
                    sym_free(S, sa->next);
                }
                sym_free(S, sa);
                sa = sa1;
            }
            S->tccpp_parse_flags = saved_parse_flags;
        }

        sym_push2(S, nested_list, s->v, 0, 0);
        S->tccpp_parse_flags = saved_parse_flags;
        joined_str = macro_twosharps(S, mstr);
        macro_subst(S, tok_str, nested_list, joined_str ? joined_str : mstr);

        /* pop nested defined symbol */
        sa1 = *nested_list;
        *nested_list = sa1->prev;
        sym_free(S, sa1);
	if (joined_str)
	    tok_str_free_str(S, joined_str);
        if (mstr != s->d)
            tok_str_free_str(S, mstr);
    }
    return 0;
}

/* do macro substitution of macro_str and add result to
   (tok_str,tok_len). 'nested_list' is the list of all macros we got
   inside to avoid recursing. */
static void macro_subst(TCCState *S,
    TokenString *tok_str,
    Sym **nested_list,
    const int *macro_str
    )
{
    Sym *s;
    int t, spc, nosubst;
    CValue cval;
    
    spc = nosubst = 0;

    while (1) {
        TOK_GET(&t, &macro_str, &cval);
        if (t <= 0)
            break;

        if (t >= TOK_IDENT && 0 == nosubst) {
            s = define_find(S, t);
            if (s == NULL)
                goto no_subst;

            /* if nested substitution, do nothing */
            if (sym_find2(*nested_list, t)) {
                /* and mark it as TOK_NOSUBST, so it doesn't get subst'd again */
                tok_str_add2(S, tok_str, TOK_NOSUBST, NULL);
                goto no_subst;
            }

            {
                TokenString *str = tok_str_alloc(S);
                str->str = (int*)macro_str;
                begin_macro(S, str, 2);

                S->tok = t;
                macro_subst_tok(S, tok_str, nested_list, s);

                if (S->tccpp_macro_stack != str) {
                    /* already finished by reading function macro arguments */
                    break;
                }

                macro_str = S->tccpp_macro_ptr;
                end_macro (S);
            }
            if (tok_str->len)
                spc = is_space(t = tok_str->str[tok_str->lastlen]);
        } else {
no_subst:
            if (!check_space(S, t, &spc))
                tok_str_add2(S, tok_str, t, &cval);

            if (nosubst) {
                if (nosubst > 1 && (spc || (++nosubst == 3 && t == '(')))
                    continue;
                nosubst = 0;
            }
            if (t == TOK_NOSUBST)
                nosubst = 1;
        }
        /* GCC supports 'defined' as result of a macro substitution */
        if (t == TOK_DEFINED && S->tccpp_pp_expr)
            nosubst = 2;
    }
}

/* return next token without macro substitution. Can read input from
   macro_ptr buffer */
static void next_nomacro(TCCState *S)
{
    int t;
    if (S->tccpp_macro_ptr) {
 redo:
        t = *S->tccpp_macro_ptr;
        if (TOK_HAS_VALUE(t)) {
            tok_get(&S->tok, &S->tccpp_macro_ptr, &S->tokc);
            if (t == TOK_LINENUM) {
                S->tccpp_file->line_num = S->tokc.i;
                goto redo;
            }
        } else {
            S->tccpp_macro_ptr++;
            if (t < TOK_IDENT) {
                if (!(S->tccpp_parse_flags & PARSE_FLAG_SPACES)
                    && (S->tccpp_isidnum_table[t - CH_EOF] & IS_SPC))
                    goto redo;
            }
            S->tok = t;
        }
    } else {
        next_nomacro1(S);
    }
}

/* return next token with macro substitution */
ST_FUNC void next(TCCState *S)
{
    int t;
 redo:
    next_nomacro(S);
    t = S->tok;
    if (S->tccpp_macro_ptr) {
        if (!TOK_HAS_VALUE(t)) {
            if (t == TOK_NOSUBST || t == TOK_PLCHLDR) {
                /* discard preprocessor markers */
                goto redo;
            } else if (t == 0) {
                /* end of macro or unget token string */
                end_macro(S);
                goto redo;
            } else if (t == '\\') {
                if (!(S->tccpp_parse_flags & PARSE_FLAG_ACCEPT_STRAYS))
                    tcc_error(S, "stray '\\' in program");
            }
            return;
        }
    } else if (t >= TOK_IDENT && (S->tccpp_parse_flags & PARSE_FLAG_PREPROCESS)) {
        /* if reading from file, try to substitute macros */
        Sym *s = define_find(S, t);
        if (s) {
            Sym *nested_list = NULL;
            S->tokstr_buf.len = 0;
            macro_subst_tok(S, &S->tokstr_buf, &nested_list, s);
            tok_str_add(S, &S->tokstr_buf, 0);
            begin_macro(S, &S->tokstr_buf, 0);
            goto redo;
        }
        return;
    }
    /* convert preprocessor tokens into C tokens */
    if (t == TOK_PPNUM) {
        if  (S->tccpp_parse_flags & PARSE_FLAG_TOK_NUM)
            parse_number(S, (char *)S->tokc.str.data);
    } else if (t == TOK_PPSTR) {
        if (S->tccpp_parse_flags & PARSE_FLAG_TOK_STR)
            parse_string(S, (char *)S->tokc.str.data, S->tokc.str.size - 1);
    }
}

/* push back current token and set current token to 'last_tok'. Only
   identifier case handled for labels. */
ST_INLN void unget_tok(TCCState *S, int last_tok)
{

    TokenString *str = tok_str_alloc(S);
    tok_str_add2(S, str, S->tok, &S->tokc);
    tok_str_add(S, str, 0);
    begin_macro(S, str, 1);
    S->tok = last_tok;
}

/* ------------------------------------------------------------------------- */
/* init preprocessor */

static const char * const target_os_defs =
#ifdef TCC_TARGET_PE
    "_WIN32\0"
# if PTR_SIZE == 8
    "_WIN64\0"
# endif
#else
# if defined TCC_TARGET_MACHO
    "__APPLE__\0"
# elif TARGETOS_FreeBSD
    "__FreeBSD__ 12\0"
# elif TARGETOS_FreeBSD_kernel
    "__FreeBSD_kernel__\0"
# elif TARGETOS_NetBSD
    "__NetBSD__\0"
# elif TARGETOS_OpenBSD
    "__OpenBSD__\0"
# else
    "__linux__\0"
    "__linux\0"
# endif
    "__unix__\0"
    "__unix\0"
#endif
    ;

static void putdef(TCCState *S, CString *cs, const char *p)
{
    cstr_printf(S, cs, "#define %s%s\n", p, &" 1"[!!strchr(p, ' ')*2]);
}

static void tcc_predefs(TCCState *S, CString *cs, int is_asm)
{
    int a, b, c;
    const char *defs[] = { target_machine_defs, target_os_defs, NULL };
    const char *p;

    sscanf(TCC_VERSION, "%d.%d.%d", &a, &b, &c);
    cstr_printf(S, cs, "#define __TINYC__ %d\n", a*10000 + b*100 + c);
    for (a = 0; defs[a]; ++a)
        for (p = defs[a]; *p; p = strchr(p, 0) + 1)
            putdef(S, cs, p);
#ifdef TCC_TARGET_ARM
    if (S->float_abi == ARM_HARD_FLOAT)
      putdef(S, cs, "__ARM_PCS_VFP");
#endif
    if (is_asm)
      putdef(S, cs, "__ASSEMBLER__");
    if (S->output_type == TCC_OUTPUT_PREPROCESS)
      putdef(S, cs, "__TCC_PP__");
    if (S->output_type == TCC_OUTPUT_MEMORY)
      putdef(S, cs, "__TCC_RUN__");
    if (S->char_is_unsigned)
      putdef(S, cs, "__CHAR_UNSIGNED__");
    if (S->optimize > 0)
      putdef(S, cs, "__OPTIMIZE__");
    if (S->option_pthread)
      putdef(S, cs, "_REENTRANT");
    if (S->leading_underscore)
      putdef(S, cs, "__leading_underscore");
#ifdef CONFIG_TCC_BCHECK
    if (S->do_bounds_check)
      putdef(S, cs, "__BOUNDS_CHECKING_ON");
#endif
    cstr_printf(S, cs, "#define __SIZEOF_POINTER__ %d\n", PTR_SIZE);
    cstr_printf(S, cs, "#define __SIZEOF_LONG__ %d\n", LONG_SIZE);
    if (!is_asm) {
      putdef(S, cs, "__STDC__");
      cstr_printf(S, cs, "#define __STDC_VERSION__ %dL\n", S->cversion);
      cstr_cat(S, cs,
        /* load more predefs and __builtins */
#if CONFIG_TCC_PREDEFS
        #include "tccdefs_.h" /* include as strings */
#else
        "#include <tccdefs.h>\n" /* load at runtime */
#endif
        , -1);
    }
    cstr_printf(S, cs, "#define __BASE_FILE__ \"%s\"\n", S->tccpp_file->filename);
}

ST_FUNC void preprocess_start(TCCState *S, int filetype)
{
    int is_asm = !!(filetype & (AFF_TYPE_ASM|AFF_TYPE_ASMPP));
    CString cstr;

    tccpp_new(S);

    S->include_stack_ptr = S->include_stack;
    S->ifdef_stack_ptr = S->ifdef_stack;
    S->tccpp_file->ifdef_stack_ptr = S->ifdef_stack_ptr;
    S->tccpp_pp_expr = 0;
    S->tccpp_pp_counter = 0;
    S->tccpp_pp_debug_tok = S->tccpp_pp_debug_symv = 0;
    S->tccpp_pp_once++;
    S->pack_stack[0] = 0;
    S->pack_stack_ptr = S->pack_stack;

    set_idnum(S, '$', !is_asm && S->dollars_in_identifiers ? IS_ID : 0);
    set_idnum(S, '.', is_asm ? IS_ID : 0);

    if (!(filetype & AFF_TYPE_ASM)) {
        cstr_new(S, &cstr);
        tcc_predefs(S, &cstr, is_asm);
        if (S->cmdline_defs.size)
          cstr_cat(S, &cstr, S->cmdline_defs.data, S->cmdline_defs.size);
        if (S->cmdline_incl.size)
          cstr_cat(S, &cstr, S->cmdline_incl.data, S->cmdline_incl.size);
        //printf("%s\n", (char*)cstr.data);
        *S->include_stack_ptr++ = S->tccpp_file;
        tcc_open_bf(S, "<command line>", cstr.size);
        memcpy(S->tccpp_file->buffer, cstr.data, cstr.size);
        cstr_free(S, &cstr);
    }

    S->tccpp_parse_flags = is_asm ? PARSE_FLAG_ASM_FILE : 0;
    S->tok_flags = TOK_FLAG_BOL | TOK_FLAG_BOF;
}

/* cleanup from error/setjmp */
ST_FUNC void preprocess_end(TCCState *S)
{
    while (S->tccpp_macro_stack)
        end_macro(S);
    S->tccpp_macro_ptr = NULL;
    while (S->tccpp_file)
        tcc_close(S);
    tccpp_delete(S);
}

ST_FUNC void tccpp_new(TCCState *S)
{
    int i, c;
    const char *p, *r;

    /* init isid table */
    for(i = CH_EOF; i<128; i++)
        set_idnum(S, i,
            is_space(i) ? IS_SPC
            : isid(i) ? IS_ID
            : isnum(i) ? IS_NUM
            : 0);

    for(i = 128; i<256; i++)
        set_idnum(S, i, IS_ID);

    /* init allocators */
    tal_new(S, &S->toksym_alloc, TOKSYM_TAL_LIMIT, TOKSYM_TAL_SIZE);
    tal_new(S, &S->tokstr_alloc, TOKSTR_TAL_LIMIT, TOKSTR_TAL_SIZE);

    memset(S->tccpp_hash_ident, 0, TOK_HASH_SIZE * sizeof(TokenSym *));
    memset(S->cached_includes_hash, 0, sizeof S->cached_includes_hash);

    cstr_new(S, &S->tccpp_cstr_buf);
    cstr_realloc(S, &S->tccpp_cstr_buf, STRING_MAX_SIZE);
    tok_str_new(&S->tokstr_buf);
    tok_str_realloc(S, &S->tokstr_buf, TOKSTR_MAX_SIZE);

    S->tok_ident = TOK_IDENT;
    p = tcc_keywords;
    while (*p) {
        r = p;
        for(;;) {
            c = *r++;
            if (c == '\0')
                break;
        }
        tok_alloc(S, p, r - p - 1);
        p = r;
    }

    /* we add dummy defines for some special macros to speed up tests
       and to have working defined() */
    define_push(S, TOK___LINE__, MACRO_OBJ, NULL, NULL);
    define_push(S, TOK___FILE__, MACRO_OBJ, NULL, NULL);
    define_push(S, TOK___DATE__, MACRO_OBJ, NULL, NULL);
    define_push(S, TOK___TIME__, MACRO_OBJ, NULL, NULL);
    define_push(S, TOK___COUNTER__, MACRO_OBJ, NULL, NULL);
}

ST_FUNC void tccpp_delete(TCCState *S)
{
    int i, n;

    dynarray_reset(S, &S->cached_includes, &S->nb_cached_includes);

    /* free tokens */
    n = S->tok_ident - TOK_IDENT;
    if (n > total_idents)
        total_idents = n;
    for(i = 0; i < n; i++)
        tal_free(S, S->toksym_alloc, S->tccpp_table_ident[i]);
    tcc_free(S, S->tccpp_table_ident);
    S->tccpp_table_ident = NULL;

    /* free static buffers */
    cstr_free(S, &S->tokcstr);
    cstr_free(S, &S->tccpp_cstr_buf);
    cstr_free(S, &S->tccpp_macro_equal_buf);
    tok_str_free_str(S, S->tokstr_buf.str);

    /* free allocators */
    tal_delete(S, S->toksym_alloc);
    S->toksym_alloc = NULL;
    tal_delete(S, S->tokstr_alloc);
    S->tokstr_alloc = NULL;
}

/* ------------------------------------------------------------------------- */
/* tcc -E [-P[1]] [-dD} support */

static void tok_print(TCCState *S, const char *msg, const int *str)
{
    FILE *fp;
    int t, s = 0;
    CValue cval;

    fp = S->ppfp;
    fprintf(fp, "%s", msg);
    while (str) {
	TOK_GET(&t, &str, &cval);
	if (!t)
	    break;
	fprintf(fp, &" %s"[s], get_tok_str(S, t, &cval)), s = 1;
    }
    fprintf(fp, "\n");
}

static void pp_line(TCCState *S, BufferedFile *f, int level)
{
    int d = f->line_num - f->line_ref;

    if (S->dflag & TCC_OPTION_d_4)
	return;

    if (S->Pflag == LINE_MACRO_OUTPUT_FORMAT_NONE) {
        ;
    } else if (level == 0 && f->line_ref && d < 8) {
	while (d > 0)
	    fputs("\n", S->ppfp), --d;
    } else if (S->Pflag == LINE_MACRO_OUTPUT_FORMAT_STD) {
	fprintf(S->ppfp, "#line %d \"%s\"\n", f->line_num, f->filename);
    } else {
	fprintf(S->ppfp, "# %d \"%s\"%s\n", f->line_num, f->filename,
	    level > 0 ? " 1" : level < 0 ? " 2" : "");
    }
    f->line_ref = f->line_num;
}

static void define_print(TCCState *S, int v)
{
    FILE *fp;
    Sym *s;

    s = define_find(S, v);
    if (NULL == s || NULL == s->d)
        return;

    fp = S->ppfp;
    fprintf(fp, "#define %s", get_tok_str(S, v, NULL));
    if (s->type.t == MACRO_FUNC) {
        Sym *a = s->next;
        fprintf(fp,"(");
        if (a)
            for (;;) {
                fprintf(fp,"%s", get_tok_str(S, a->v & ~SYM_FIELD, NULL));
                if (!(a = a->next))
                    break;
                fprintf(fp,",");
            }
        fprintf(fp,")");
    }
    tok_print(S, "", s->d);
}

static void pp_debug_defines(TCCState *S)
{
    int v, t;
    const char *vs;
    FILE *fp;

    t = S->tccpp_pp_debug_tok;
    if (t == 0)
        return;

    S->tccpp_file->line_num--;
    pp_line(S, S->tccpp_file, 0);
    S->tccpp_file->line_ref = ++S->tccpp_file->line_num;

    fp = S->ppfp;
    v = S->tccpp_pp_debug_symv;
    vs = get_tok_str(S, v, NULL);
    if (t == TOK_DEFINE) {
        define_print(S, v);
    } else if (t == TOK_UNDEF) {
        fprintf(fp, "#undef %s\n", vs);
    } else if (t == TOK_push_macro) {
        fprintf(fp, "#pragma push_macro(\"%s\")\n", vs);
    } else if (t == TOK_pop_macro) {
        fprintf(fp, "#pragma pop_macro(\"%s\")\n", vs);
    }
    S->tccpp_pp_debug_tok = 0;
}

static void pp_debug_builtins(TCCState *S)
{
    int v;
    for (v = TOK_IDENT; v < S->tok_ident; ++v)
        define_print(S, v);
}

/* Add a space between tokens a and b to avoid unwanted textual pasting */
static int pp_need_space(int a, int b)
{
    return 'E' == a ? '+' == b || '-' == b
        : '+' == a ? TOK_INC == b || '+' == b
        : '-' == a ? TOK_DEC == b || '-' == b
        : a >= TOK_IDENT ? b >= TOK_IDENT
	: a == TOK_PPNUM ? b >= TOK_IDENT
        : 0;
}

/* maybe hex like 0x1e */
static int pp_check_he0xE(int t, const char *p)
{
    if (t == TOK_PPNUM && toup(strchr(p, 0)[-1]) == 'E')
        return 'E';
    return t;
}

/* Preprocess the current file */
ST_FUNC int tcc_preprocess(TCCState *S)
{
    BufferedFile **iptr;
    int token_seen, spcs, level;
    const char *p;
    char white[400];

    S->tccpp_parse_flags = PARSE_FLAG_PREPROCESS
                | (S->tccpp_parse_flags & PARSE_FLAG_ASM_FILE)
                | PARSE_FLAG_LINEFEED
                | PARSE_FLAG_SPACES
                | PARSE_FLAG_ACCEPT_STRAYS
                ;
    /* Credits to Fabrice Bellard's initial revision to demonstrate its
       capability to compile and run itself, provided all numbers are
       given as decimals. tcc -E -P10 will do. */
    if (S->Pflag == LINE_MACRO_OUTPUT_FORMAT_P10)
        S->tccpp_parse_flags |= PARSE_FLAG_TOK_NUM, S->Pflag = 1;

    if (S->do_bench) {
	/* for PP benchmarks */
	do next(S); while (S->tok != TOK_EOF);
	return 0;
    }

    if (S->dflag & TCC_OPTION_d_BI) {
        pp_debug_builtins(S);
        S->dflag &= ~TCC_OPTION_d_BI;
    }

    token_seen = TOK_LINEFEED, spcs = 0, level = 0;
    if (S->tccpp_file->prev)
        pp_line(S, S->tccpp_file->prev, level++);
    pp_line(S, S->tccpp_file, level);
    for (;;) {
        iptr = S->include_stack_ptr;
        next(S);
        if (S->tok == TOK_EOF)
            break;

        level = S->include_stack_ptr - iptr;
        if (level) {
            if (level > 0)
                pp_line(S, *iptr, 0);
            pp_line(S, S->tccpp_file, level);
        }
        if (S->dflag & TCC_OPTION_d_M) {
            pp_debug_defines(S);
            if (S->dflag & TCC_OPTION_d_4)
                continue;
        }

        if (is_space(S->tok)) {
            if (spcs < sizeof white - 1)
                white[spcs++] = S->tok;
            continue;
        } else if (S->tok == TOK_LINEFEED) {
            spcs = 0;
            if (token_seen == TOK_LINEFEED)
                continue;
            ++S->tccpp_file->line_ref;
        } else if (token_seen == TOK_LINEFEED) {
            pp_line(S, S->tccpp_file, 0);
        } else if (spcs == 0 && pp_need_space(token_seen, S->tok)) {
            white[spcs++] = ' ';
        }

        white[spcs] = 0, fputs(white, S->ppfp), spcs = 0;
        fputs(p = get_tok_str(S, S->tok, &S->tokc), S->ppfp);
        token_seen = pp_check_he0xE(S->tok, p);
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
