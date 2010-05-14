/*
 *  TCC - Tiny C Compiler - Support for -run switch
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

#include "tcc.h"

#ifdef _WIN32
#define ucontext_t CONTEXT
#endif

static void set_pages_executable(void *ptr, unsigned long length);
static void set_exception_handler(void);
static int rt_get_caller_pc(uplong *paddr, ucontext_t *uc, int level);
static void rt_error(ucontext_t *uc, const char *fmt, ...);
static int tcc_relocate_ex(TCCState *s1, void *ptr);

/* ------------------------------------------------------------- */
/* Do all relocations (needed before using tcc_get_symbol())
   Returns -1 on error. */

int tcc_relocate(TCCState *s1)
{
    int ret;
#ifdef HAVE_SELINUX
    char tmpfname[] = "/tmp/.tccrunXXXXXX";
    int fd = mkstemp (tmpfname);
    unlink (tmpfname); ftruncate (fd, 1000);
    if ((ret= tcc_relocate_ex(s1,NULL)) < 0)return -1;
    s1->mem_size=ret;
     /* Use mmap instead of malloc for Selinux */
    s1->write_mem = mmap (NULL, ret, PROT_READ|PROT_WRITE,
        MAP_SHARED, fd, 0);
    if(s1->write_mem == MAP_FAILED){
        error("/tmp not writeable");
        return -1;
    }
    s1->runtime_mem = mmap (NULL, ret, PROT_READ|PROT_EXEC,
        MAP_SHARED, fd, 0);
    if(s1->runtime_mem == MAP_FAILED){
        error("/tmp not executable");
        return -1;
    }
    ret = tcc_relocate_ex(s1, s1->write_mem);
#else
    ret = tcc_relocate_ex(s1, NULL);
    if (-1 != ret) {
        s1->runtime_mem = tcc_malloc(ret);
        ret = tcc_relocate_ex(s1, s1->runtime_mem);
    }
#endif
    return ret;
}

/* launch the compiled program with the given arguments */
int tcc_run(TCCState *s1, int argc, char **argv)
{
    int (*prog_main)(int, char **);

    if (tcc_relocate(s1) < 0)
        return -1;

    prog_main = tcc_get_symbol_err(s1, "main");

#ifdef CONFIG_TCC_BACKTRACE
    if (s1->do_debug)
        set_exception_handler();
#endif

#ifdef CONFIG_TCC_BCHECK
    if (s1->do_bounds_check) {
        void (*bound_init)(void);
        void (*bound_exit)(void);
        int ret;
        /* set error function */
        rt_bound_error_msg = tcc_get_symbol_err(s1, "__bound_error_msg");
        rt_prog_main = prog_main;
        /* XXX: use .init section so that it also work in binary ? */
        bound_init = tcc_get_symbol_err(s1, "__bound_init");
        bound_exit = tcc_get_symbol_err(s1, "__bound_exit");
        bound_init();
        ret = (*prog_main)(argc, argv);
        bound_exit();
        return ret;
    }
#endif

#ifdef TCC_TARGET_PE
    {
      unsigned char *p = tcc_get_symbol(s1, "tinyc_no_getbp");
      if (p) *p = 0;
    }
#endif
    return (*prog_main)(argc, argv);
}


/* relocate code. Return -1 on error, required size if ptr is NULL,
   otherwise copy code into buffer passed by the caller */
static int tcc_relocate_ex(TCCState *s1, void *ptr)
{
    Section *s;
    unsigned long offset, length;
    uplong mem;
    int i;

    if (0 == s1->runtime_added) {
        s1->runtime_added = 1;
        s1->nb_errors = 0;
#ifdef TCC_TARGET_PE
        pe_output_file(s1, NULL);
#else
        tcc_add_runtime(s1);
        relocate_common_syms();
        tcc_add_linker_symbols(s1);
        build_got_entries(s1);
#endif
        if (s1->nb_errors)
            return -1;
    }

    offset = 0, mem = (uplong)ptr;
    for(i = 1; i < s1->nb_sections; i++) {
        s = s1->sections[i];
        if (0 == (s->sh_flags & SHF_ALLOC))
            continue;
        length = s->data_offset;
        s->sh_addr = mem ? (mem + offset + 15) & ~15 : 0;
        offset = (offset + length + 15) & ~15;
    }
    offset += 16;

    /* relocate symbols */
    relocate_syms(s1, 1);
    if (s1->nb_errors)
        return -1;

#if (defined TCC_TARGET_X86_64 || defined TCC_TARGET_ARM) && !defined TCC_TARGET_PE
    s1->runtime_plt_and_got_offset = 0;
    s1->runtime_plt_and_got = (char *)(mem + offset);
    /* double the size of the buffer for got and plt entries
       XXX: calculate exact size for them? */
    offset *= 2;
#endif

    if (0 == mem)
        return offset;

    /* relocate each section */
    for(i = 1; i < s1->nb_sections; i++) {
        s = s1->sections[i];
        if (s->reloc)
            relocate_section(s1, s);
    }

    for(i = 1; i < s1->nb_sections; i++) {
        s = s1->sections[i];
        if (0 == (s->sh_flags & SHF_ALLOC))
            continue;
        length = s->data_offset;
        // printf("%-12s %08x %04x\n", s->name, s->sh_addr, length);
        ptr = (void*)(uplong)s->sh_addr;
        if (NULL == s->data || s->sh_type == SHT_NOBITS)
            memset(ptr, 0, length);
        else
            memcpy(ptr, s->data, length);
        /* mark executable sections as executable in memory */
        if (s->sh_flags & SHF_EXECINSTR)
            set_pages_executable(ptr, length);
    }

#if (defined TCC_TARGET_X86_64 || defined TCC_TARGET_ARM) && !defined TCC_TARGET_PE
    set_pages_executable(s1->runtime_plt_and_got,
                         s1->runtime_plt_and_got_offset);
#endif
    return 0;
}

/* ------------------------------------------------------------- */
/* allow to run code in memory */

static void set_pages_executable(void *ptr, unsigned long length)
{
#ifdef _WIN32
    unsigned long old_protect;
    VirtualProtect(ptr, length, PAGE_EXECUTE_READWRITE, &old_protect);
#else
    unsigned long start, end;
    start = (uplong)ptr & ~(PAGESIZE - 1);
    end = (uplong)ptr + length;
    end = (end + PAGESIZE - 1) & ~(PAGESIZE - 1);
    mprotect((void *)start, end - start, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif
}

/* ------------------------------------------------------------- */
#ifdef CONFIG_TCC_BACKTRACE

/* print the position in the source file of PC value 'pc' by reading
   the stabs debug information */
static uplong rt_printline(uplong wanted_pc)
{
    Stab_Sym *sym, *sym_end;
    char func_name[128], last_func_name[128];
    unsigned long func_addr, last_pc, pc;
    const char *incl_files[INCLUDE_STACK_SIZE];
    int incl_index, len, last_line_num, i;
    const char *str, *p;

    fprintf(stderr, "0x%08lx:", (unsigned long)wanted_pc);

    func_name[0] = '\0';
    func_addr = 0;
    incl_index = 0;
    last_func_name[0] = '\0';
    last_pc = 0xffffffff;
    last_line_num = 1;
    sym = (Stab_Sym *)stab_section->data + 1;
    sym_end = (Stab_Sym *)(stab_section->data + stab_section->data_offset);
    while (sym < sym_end) {
        switch(sym->n_type) {
            /* function start or end */
        case N_FUN:
            if (sym->n_strx == 0) {
                /* we test if between last line and end of function */
                pc = sym->n_value + func_addr;
                if (wanted_pc >= last_pc && wanted_pc < pc)
                    goto found;
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
            /* XXX: slow! */
            strcpy(last_func_name, func_name);
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

    /* second pass: we try symtab symbols (no line number info) */
    incl_index = 0;
    {
        ElfW(Sym) *sym, *sym_end;
        int type;

        sym_end = (ElfW(Sym) *)(symtab_section->data + symtab_section->data_offset);
        for(sym = (ElfW(Sym) *)symtab_section->data + 1;
            sym < sym_end;
            sym++) {
            type = ELFW(ST_TYPE)(sym->st_info);
            if (type == STT_FUNC) {
                if (wanted_pc >= sym->st_value &&
                    wanted_pc < sym->st_value + sym->st_size) {
                    pstrcpy(last_func_name, sizeof(last_func_name),
                            strtab_section->data + sym->st_name);
                    func_addr = sym->st_value;
                    goto found;
                }
            }
        }
    }
    /* did not find any info: */
    fprintf(stderr, " ???\n");
    return 0;
 found:
    if (last_func_name[0] != '\0') {
        fprintf(stderr, " %s()", last_func_name);
    }
    if (incl_index > 0) {
        fprintf(stderr, " (%s:%d",
                incl_files[incl_index - 1], last_line_num);
        for(i = incl_index - 2; i >= 0; i--)
            fprintf(stderr, ", included from %s", incl_files[i]);
        fprintf(stderr, ")");
    }
    fprintf(stderr, "\n");
    return func_addr;
}

/* emit a run time error at position 'pc' */
static void rt_error(ucontext_t *uc, const char *fmt, ...)
{
    va_list ap;
    uplong pc;
    int i;

    va_start(ap, fmt);
    fprintf(stderr, "Runtime error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    for(i=0;i<num_callers;i++) {
        if (rt_get_caller_pc(&pc, uc, i) < 0)
            break;
        if (i == 0)
            fprintf(stderr, "at ");
        else
            fprintf(stderr, "by ");
        pc = rt_printline(pc);
        if (pc == (uplong)rt_prog_main && pc)
            break;
    }
    exit(255);
    va_end(ap);
}

/* ------------------------------------------------------------- */
#ifndef _WIN32

/* signal handler for fatal errors */
static void sig_error(int signum, siginfo_t *siginf, void *puc)
{
    ucontext_t *uc = puc;

    switch(signum) {
    case SIGFPE:
        switch(siginf->si_code) {
        case FPE_INTDIV:
        case FPE_FLTDIV:
            rt_error(uc, "division by zero");
            break;
        default:
            rt_error(uc, "floating point exception");
            break;
        }
        break;
    case SIGBUS:
    case SIGSEGV:
        if (rt_bound_error_msg && *rt_bound_error_msg)
            rt_error(uc, *rt_bound_error_msg);
        else
            rt_error(uc, "dereferencing invalid pointer");
        break;
    case SIGILL:
        rt_error(uc, "illegal instruction");
        break;
    case SIGABRT:
        rt_error(uc, "abort() called");
        break;
    default:
        rt_error(uc, "caught signal %d", signum);
        break;
    }
    exit(255);
}

/* Generate a stack backtrace when a CPU exception occurs. */
static void set_exception_handler(void)
{
    struct sigaction sigact;
    /* install TCC signal handlers to print debug info on fatal
       runtime errors */
    sigact.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigact.sa_sigaction = sig_error;
    sigemptyset(&sigact.sa_mask);
    sigaction(SIGFPE, &sigact, NULL);
    sigaction(SIGILL, &sigact, NULL);
    sigaction(SIGSEGV, &sigact, NULL);
    sigaction(SIGBUS, &sigact, NULL);
    sigaction(SIGABRT, &sigact, NULL);
}

/* ------------------------------------------------------------- */
#ifdef __i386__

/* fix for glibc 2.1 */
#ifndef REG_EIP
#define REG_EIP EIP
#define REG_EBP EBP
#endif

/* return the PC at frame level 'level'. Return non zero if not found */
static int rt_get_caller_pc(unsigned long *paddr, ucontext_t *uc, int level)
{
    unsigned long fp;
    int i;

    if (level == 0) {
#if defined(__FreeBSD__)
        *paddr = uc->uc_mcontext.mc_eip;
#elif defined(__dietlibc__)
        *paddr = uc->uc_mcontext.eip;
#else
        *paddr = uc->uc_mcontext.gregs[REG_EIP];
#endif
        return 0;
    } else {
#if defined(__FreeBSD__)
        fp = uc->uc_mcontext.mc_ebp;
#elif defined(__dietlibc__)
        fp = uc->uc_mcontext.ebp;
#else
        fp = uc->uc_mcontext.gregs[REG_EBP];
#endif
        for(i=1;i<level;i++) {
            /* XXX: check address validity with program info */
            if (fp <= 0x1000 || fp >= 0xc0000000)
                return -1;
            fp = ((unsigned long *)fp)[0];
        }
        *paddr = ((unsigned long *)fp)[1];
        return 0;
    }
}

/* ------------------------------------------------------------- */
#elif defined(__x86_64__)

/* return the PC at frame level 'level'. Return non zero if not found */
static int rt_get_caller_pc(unsigned long *paddr,
                            ucontext_t *uc, int level)
{
    unsigned long fp;
    int i;

    if (level == 0) {
        /* XXX: only support linux */
#if defined(__FreeBSD__)
        *paddr = uc->uc_mcontext.mc_rip;
#else
        *paddr = uc->uc_mcontext.gregs[REG_RIP];
#endif
        return 0;
    } else {
#if defined(__FreeBSD__)
        fp = uc->uc_mcontext.mc_rbp;
#else
        fp = uc->uc_mcontext.gregs[REG_RBP];
#endif
        for(i=1;i<level;i++) {
            /* XXX: check address validity with program info */
            if (fp <= 0x1000)
                return -1;
            fp = ((unsigned long *)fp)[0];
        }
        *paddr = ((unsigned long *)fp)[1];
        return 0;
    }
}

/* ------------------------------------------------------------- */
#elif defined(__arm__)

/* return the PC at frame level 'level'. Return negative if not found */
static int rt_get_caller_pc(unsigned long *paddr,
                            ucontext_t *uc, int level)
{
    uint32_t fp, sp;
    int i;

    if (level == 0) {
        /* XXX: only supports linux */
#if defined(__linux__)
        *paddr = uc->uc_mcontext.arm_pc;
#else
        return -1;
#endif
        return 0;
    } else {
#if defined(__linux__)
        fp = uc->uc_mcontext.arm_fp;
        sp = uc->uc_mcontext.arm_sp;
        if (sp < 0x1000)
            sp = 0x1000;
#else
        return -1;
#endif
        /* XXX: specific to tinycc stack frames */
        if (fp < sp + 12 || fp & 3)
            return -1;
        for(i = 1; i < level; i++) {
            sp = ((uint32_t *)fp)[-2];
            if (sp < fp || sp - fp > 16 || sp & 3)
                return -1;
            fp = ((uint32_t *)fp)[-3];
            if (fp <= sp || fp - sp < 12 || fp & 3)
                return -1;
        }
        /* XXX: check address validity with program info */
        *paddr = ((uint32_t *)fp)[-1];
        return 0;
    }
}

/* ------------------------------------------------------------- */
#else

#warning add arch specific rt_get_caller_pc()
static int rt_get_caller_pc(unsigned long *paddr,
                            ucontext_t *uc, int level)
{
    return -1;
}

#endif /* !__i386__ */

/* ------------------------------------------------------------- */
#else /* WIN32 */

static long __stdcall cpu_exception_handler(EXCEPTION_POINTERS *ex_info)
{
    CONTEXT *uc = ex_info->ContextRecord;
/*
    EXCEPTION_RECORD *er = ex_info->ExceptionRecord;
    printf("CPU exception: code=%08lx addr=%p\n",
	er->ExceptionCode, er->ExceptionAddress);
*/
    if (rt_bound_error_msg && *rt_bound_error_msg)
	rt_error(uc, *rt_bound_error_msg);
    else
	rt_error(uc, "dereferencing invalid pointer");
    exit(255);
    //return EXCEPTION_CONTINUE_SEARCH;
}

/* Generate a stack backtrace when a CPU exception occurs. */
static void set_exception_handler(void)
{
    SetUnhandledExceptionFilter(cpu_exception_handler);
}

#ifdef _WIN64
#define Eip Rip
#define Ebp Rbp
#endif

/* return the PC at frame level 'level'. Return non zero if not found */
static int rt_get_caller_pc(uplong *paddr, CONTEXT *uc, int level)
{
    uplong fp;
    int i;

    if (level == 0) {
	*paddr = uc->Eip;
	return 0;
    } else {
	fp = uc->Ebp;
	for(i=1;i<level;i++) {
	    /* XXX: check address validity with program info */
	    if (fp <= 0x1000 || fp >= 0xc0000000)
		return -1;
	    fp = ((uplong*)fp)[0];
	}
	*paddr = ((uplong*)fp)[1];
	return 0;
    }
}

#undef Eip
#undef Ebp

#endif /* _WIN32 */
#endif /* CONFIG_TCC_BACKTRACE */
/* ------------------------------------------------------------- */

#ifdef CONFIG_TCC_STATIC

#define RTLD_LAZY       0x001
#define RTLD_NOW        0x002
#define RTLD_GLOBAL     0x100
#define RTLD_DEFAULT    NULL

/* dummy function for profiling */
void *dlopen(const char *filename, int flag)
{
    return NULL;
}

void dlclose(void *p)
{
}
/*
const char *dlerror(void)
{
    return "error";
}
*/
typedef struct TCCSyms {
    char *str;
    void *ptr;
} TCCSyms;

#define TCCSYM(a) { #a, &a, },

/* add the symbol you want here if no dynamic linking is done */
static TCCSyms tcc_syms[] = {
#if !defined(CONFIG_TCCBOOT)
    TCCSYM(printf)
    TCCSYM(fprintf)
    TCCSYM(fopen)
    TCCSYM(fclose)
#endif
    { NULL, NULL },
};

void *resolve_sym(TCCState *s1, const char *symbol)
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

#elif !defined(_WIN32)

void *resolve_sym(TCCState *s1, const char *sym)
{
    return dlsym(RTLD_DEFAULT, sym);
}

#endif /* CONFIG_TCC_STATIC */

/* ------------------------------------------------------------- */
