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

/* only native compiler supports -run */
#ifdef TCC_IS_NATIVE

#ifndef _WIN32
# include <sys/mman.h>
#endif

#ifdef CONFIG_TCC_BACKTRACE
static void set_exception_handler(void);
#ifdef _WIN32
static DWORD s1_for_run_idx;
void set_s1_for_run(TCCState *s)
{
    if (!s1_for_run_idx)
        s1_for_run_idx = TlsAlloc();
    TlsSetValue(s1_for_run_idx, s);
}
#define get_s1_for_run() ((TCCState*)TlsGetValue(s1_for_run_idx))
#else
/* XXX: add tls support for linux */
static TCCState *s1_for_run;
#define set_s1_for_run(s) (s1_for_run = s)
#define get_s1_for_run() s1_for_run
#endif
#endif

static void set_pages_executable(TCCState *s1, void *ptr, unsigned long length);
static int tcc_relocate_ex(TCCState *s1, void *ptr, addr_t ptr_diff);

#ifdef _WIN64
static void *win64_add_function_table(TCCState *s1);
static void win64_del_function_table(void *);
#endif

/* ------------------------------------------------------------- */
/* Do all relocations (needed before using tcc_get_symbol())
   Returns -1 on error. */

LIBTCCAPI int tcc_relocate(TCCState *s1, void *ptr)
{
    int size;
    addr_t ptr_diff = 0;

    if (TCC_RELOCATE_AUTO != ptr)
        return tcc_relocate_ex(s1, ptr, 0);

    size = tcc_relocate_ex(s1, NULL, 0);
    if (size < 0)
        return -1;

#ifdef HAVE_SELINUX
{
    /* Using mmap instead of malloc */
    void *prx;
    char tmpfname[] = "/tmp/.tccrunXXXXXX";
    int fd = mkstemp(tmpfname);
    unlink(tmpfname);
    ftruncate(fd, size);

    ptr = mmap (NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    prx = mmap (NULL, size, PROT_READ|PROT_EXEC, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED || prx == MAP_FAILED)
	tcc_error("tccrun: could not map memory");
    dynarray_add(&s1->runtime_mem, &s1->nb_runtime_mem, (void*)(addr_t)size);
    dynarray_add(&s1->runtime_mem, &s1->nb_runtime_mem, prx);
    ptr_diff = (char*)prx - (char*)ptr;
}
#else
    ptr = tcc_malloc(size);
#endif
    tcc_relocate_ex(s1, ptr, ptr_diff); /* no more errors expected */
    dynarray_add(&s1->runtime_mem, &s1->nb_runtime_mem, ptr);
    return 0;
}

ST_FUNC void tcc_run_free(TCCState *s1)
{
    int i;

    for (i = 0; i < s1->nb_runtime_mem; ++i) {
#ifdef HAVE_SELINUX
        unsigned size = (unsigned)(addr_t)s1->runtime_mem[i++];
        munmap(s1->runtime_mem[i++], size);
        munmap(s1->runtime_mem[i], size);
#else
#ifdef _WIN64
        win64_del_function_table(*(void**)s1->runtime_mem[i]);
#endif
        tcc_free(s1->runtime_mem[i]);
#endif
    }
    tcc_free(s1->runtime_mem);
    if (get_s1_for_run() == s1)
        set_s1_for_run(NULL);
}

/* launch the compiled program with the given arguments */
LIBTCCAPI int tcc_run(TCCState *s1, int argc, char **argv)
{
    int (*prog_main)(int, char **);

    s1->runtime_main = s1->nostdlib ? "_start" : "main";
    if ((s1->dflag & 16) && !find_elf_sym(s1->symtab, s1->runtime_main))
        return 0;
    if (tcc_relocate(s1, TCC_RELOCATE_AUTO) < 0)
        return -1;
    prog_main = tcc_get_symbol_err(s1, s1->runtime_main);

#ifdef CONFIG_TCC_BACKTRACE
    if (s1->do_debug) {
        set_exception_handler();
        s1->rt_prog_main = prog_main;
        /* set global state pointer for exception handlers*/
        set_s1_for_run(s1);
    }
#endif

    errno = 0; /* clean errno value */

#ifdef CONFIG_TCC_BCHECK
    if (s1->do_bounds_check) {
        void (*bound_init)(void);
        void (*bound_exit)(void);
        void (*bounds_add_static_var)(size_t *p);
        size_t *bounds_start;
        int ret;

        /* set error function */
        s1->rt_bound_error_msg = tcc_get_symbol_err(s1, "__bound_error_msg");
        /* XXX: use .init section so that it also work in binary ? */
        bound_init = tcc_get_symbol_err(s1, "__bound_init");
        bound_exit = tcc_get_symbol_err(s1, "__bound_exit");
        bounds_add_static_var = tcc_get_symbol_err(s1, "__bounds_add_static_var");
        bounds_start = tcc_get_symbol_err(s1, "__bounds_start");

        bound_init();
        bounds_add_static_var (bounds_start);

        ret = (*prog_main)(argc, argv);

        bound_exit();
        return ret;
    }
#endif
    return (*prog_main)(argc, argv);
}

#if defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64
/* To avoid that x86 processors would reload cached instructions
   each time when data is written in the near, we need to make
   sure that code and data do not share the same 64 byte unit */
 #define RUN_SECTION_ALIGNMENT 63
#else
 #define RUN_SECTION_ALIGNMENT 0
#endif

/* relocate code. Return -1 on error, required size if ptr is NULL,
   otherwise copy code into buffer passed by the caller */
static int tcc_relocate_ex(TCCState *s1, void *ptr, addr_t ptr_diff)
{
    Section *s;
    unsigned offset, length, align, max_align, i, k, f;
    addr_t mem, addr;

    if (NULL == ptr) {
        s1->nb_errors = 0;
#ifdef TCC_TARGET_PE
        pe_output_file(s1, NULL);
#else
        tcc_add_runtime(s1);
	resolve_common_syms(s1);
        build_got_entries(s1);
#endif
        if (s1->nb_errors)
            return -1;
    }

    offset = max_align = 0, mem = (addr_t)ptr;
#ifdef _WIN64
    offset += sizeof (void*); /* space for function_table pointer */
#endif
    for (k = 0; k < 2; ++k) {
        f = 0, addr = k ? mem : mem + ptr_diff;
        for(i = 1; i < s1->nb_sections; i++) {
            s = s1->sections[i];
            if (0 == (s->sh_flags & SHF_ALLOC))
                continue;
            if (k != !(s->sh_flags & SHF_EXECINSTR))
                continue;
            align = s->sh_addralign - 1;
            if (++f == 1 && align < RUN_SECTION_ALIGNMENT)
                align = RUN_SECTION_ALIGNMENT;
            if (max_align < align)
                max_align = align;
            offset += -(addr + offset) & align;
            s->sh_addr = mem ? addr + offset : 0;
            offset += s->data_offset;
#if 0
            if (mem)
                printf("%-16s %p  len %04x  align %2d\n",
                    s->name, (void*)s->sh_addr, (unsigned)s->data_offset, align + 1);
#endif
        }
    }

    /* relocate symbols */
    relocate_syms(s1, s1->symtab, 1);
    if (s1->nb_errors)
        return -1;

    if (0 == mem)
        return offset + max_align;

#ifdef TCC_TARGET_PE
    s1->pe_imagebase = mem;
#endif

    /* relocate each section */
    for(i = 1; i < s1->nb_sections; i++) {
        s = s1->sections[i];
        if (s->reloc)
            relocate_section(s1, s);
    }
#ifndef TCC_TARGET_PE
    relocate_plt(s1);
#endif

    for(i = 1; i < s1->nb_sections; i++) {
        s = s1->sections[i];
        if (0 == (s->sh_flags & SHF_ALLOC))
            continue;
        length = s->data_offset;
        ptr = (void*)s->sh_addr;
        if (s->sh_flags & SHF_EXECINSTR)
            ptr = (char*)ptr - ptr_diff;
        if (NULL == s->data || s->sh_type == SHT_NOBITS)
            memset(ptr, 0, length);
        else
            memcpy(ptr, s->data, length);
        /* mark executable sections as executable in memory */
        if (s->sh_flags & SHF_EXECINSTR)
            set_pages_executable(s1, (char*)ptr + ptr_diff, length);
    }

#ifdef _WIN64
    *(void**)mem = win64_add_function_table(s1);
#endif

    return 0;
}

/* ------------------------------------------------------------- */
/* allow to run code in memory */

static void set_pages_executable(TCCState *s1, void *ptr, unsigned long length)
{
#ifdef _WIN32
    unsigned long old_protect;
    VirtualProtect(ptr, length, PAGE_EXECUTE_READWRITE, &old_protect);
#else
    void __clear_cache(void *beginning, void *end);
# ifndef HAVE_SELINUX
    addr_t start, end;
#  ifndef PAGESIZE
#   define PAGESIZE 4096
#  endif
    start = (addr_t)ptr & ~(PAGESIZE - 1);
    end = (addr_t)ptr + length;
    end = (end + PAGESIZE - 1) & ~(PAGESIZE - 1);
    if (mprotect((void *)start, end - start, PROT_READ | PROT_WRITE | PROT_EXEC))
        tcc_error("mprotect failed: did you mean to configure --with-selinux?");
# endif
# if defined TCC_TARGET_ARM || defined TCC_TARGET_ARM64
    __clear_cache(ptr, (char *)ptr + length);
# endif
#endif
}

#ifdef _WIN64
static void *win64_add_function_table(TCCState *s1)
{
    void *p = NULL;
    if (s1->uw_pdata) {
        p = (void*)s1->uw_pdata->sh_addr;
        RtlAddFunctionTable(
            (RUNTIME_FUNCTION*)p,
            s1->uw_pdata->data_offset / sizeof (RUNTIME_FUNCTION),
            s1->pe_imagebase
            );
        s1->uw_pdata = NULL;
    }
    return p;
}

static void win64_del_function_table(void *p)
{
    if (p) {
        RtlDeleteFunctionTable((RUNTIME_FUNCTION*)p);
    }
}
#endif

/* ------------------------------------------------------------- */
#ifdef CONFIG_TCC_BACKTRACE

#define INCLUDE_STACK_SIZE 32
static int rt_vprintf(const char *fmt, va_list ap)
{
    int ret = vfprintf(stderr, fmt, ap);
    fflush(stderr);
    return ret;
}

static int rt_printf(const char *fmt, ...)
{
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = rt_vprintf(fmt, ap);
    va_end(ap);
    return r;
}

/* print the position in the source file of PC value 'pc' by reading
   the stabs debug information */
static addr_t rt_printline(TCCState *s1, addr_t wanted_pc, const char *msg)
{
    char func_name[128];
    addr_t func_addr, last_pc, pc;
    const char *incl_files[INCLUDE_STACK_SIZE];
    int incl_index, last_incl_index, len, last_line_num, i;
    const char *str, *p;

    ElfW(Sym) *esym_start = NULL, *esym_end = NULL, *esym;
    Stab_Sym *stab_sym = NULL, *stab_sym_end = NULL, *sym;
    char *stab_str = NULL;
    char *elf_str = NULL;

    func_name[0] = '\0';
    func_addr = 0;
    incl_index = 0;
    last_pc = (addr_t)-1;
    last_line_num = 1;
    last_incl_index = 0;

    if (stab_section) {
        stab_sym = (Stab_Sym *)stab_section->data;
        stab_sym_end = (Stab_Sym *)(stab_section->data + stab_section->data_offset);
        stab_str = (char *) stab_section->link->data;
    }
    if (symtab_section) {
        esym_start = (ElfW(Sym) *)(symtab_section->data);
        esym_end = (ElfW(Sym) *)(symtab_section->data + symtab_section->data_offset);
        elf_str = symtab_section->link->data;
    }

    for (sym = stab_sym + 1; sym < stab_sym_end; ++sym) {
        str = stab_str + sym->n_strx;
        pc = sym->n_value;

        switch(sym->n_type) {
        case N_SLINE:
            if (func_addr)
                goto rel_pc;
        case N_SO:
        case N_SOL:
            goto abs_pc;
        case N_FUN:
            if (sym->n_strx == 0) /* end of function */
                goto rel_pc;
        abs_pc:
            if (sizeof pc == 8)
                /* Stab_Sym.n_value is only 32bits */
                pc |= wanted_pc & 0xffffffff00000000ULL;
            break;
        rel_pc:
            pc += func_addr;
            break;
        }

        if (pc > wanted_pc && wanted_pc > last_pc)
            goto found;

        switch(sym->n_type) {
            /* function start or end */
        case N_FUN:
            if (sym->n_strx == 0)
                goto reset_func;
            p = strchr(str, ':');
            if (0 == p || (len = p - str + 1, len > sizeof func_name))
                len = sizeof func_name;
            pstrcpy(func_name, len, str);
            func_addr = pc;
            break;
            /* line number info */
        case N_SLINE:
            last_pc = pc;
            last_line_num = sym->n_desc;
            last_incl_index = incl_index;
            if (pc == wanted_pc)
                goto found;
            break;
            /* include files */
        case N_BINCL:
            if (incl_index < INCLUDE_STACK_SIZE)
                incl_files[incl_index++] = str;
            break;
        case N_EINCL:
            if (incl_index > 1)
                incl_index--;
            break;
            /* start/end of translation unit */
        case N_SO:
            incl_index = 0;
            if (sym->n_strx) {
                /* do not add path */
                len = strlen(str);
                if (len > 0 && str[len - 1] != '/')
                    incl_files[incl_index++] = str;
            }
        reset_func:
            func_name[0] = '\0';
            func_addr = 0;
            last_pc = (addr_t)-1;
            break;
            /* alternative file name (from #line or #include directives) */
        case N_SOL:
            if (incl_index)
                incl_files[incl_index-1] = str;
            break;
        }
    }

    /* we try symtab symbols (no line number info) */
    for (esym = esym_start + 1; esym < esym_end; ++esym) {
        int type = ELFW(ST_TYPE)(esym->st_info);
        if (type == STT_FUNC || type == STT_GNU_IFUNC) {
            if (wanted_pc >= esym->st_value &&
                wanted_pc < esym->st_value + esym->st_size) {
                pstrcpy(func_name, sizeof(func_name),
                    elf_str + esym->st_name);
                func_addr = esym->st_value;
                last_incl_index = 0;
                goto found;
            }
        }
    }
    /* did not find any info: */
    rt_printf("%s %p ???", msg, (void*)wanted_pc);
    return 0;

 found:
    i = last_incl_index;
    if (i > 0)
        rt_printf("%s:%d: ", incl_files[--i], last_line_num);
    rt_printf("%s %p", msg, (void*)wanted_pc);
    if (func_name[0] != '\0')
        rt_printf(" %s()", func_name);
    if (--i >= 0) {
        rt_printf(" (included from ");
        for (;;) {
            rt_printf("%s", incl_files[i]);
            if (--i < 0)
                break;
            rt_printf(", ");
        }
        rt_printf(")");
    }
    return func_addr;
}

typedef struct rt_context {
    addr_t ip, fp, sp, pc;
} rt_context;

static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level);

/* emit a run time error at position 'pc' */
static void rt_error(rt_context *rc, const char *fmt, ...)
{
    va_list ap;
    addr_t pc;
    int i;
    TCCState *s1;

    s1 = get_s1_for_run();
    if (*fmt == ' ') {
        if (s1 && s1->rt_bound_error_msg && *s1->rt_bound_error_msg)
            fmt = *s1->rt_bound_error_msg;
        else
            ++fmt;
    }

    rt_printf("Runtime error: ");
    va_start(ap, fmt);
    rt_vprintf(fmt, ap);
    va_end(ap);
    rt_printf("\n");

    if (!s1)
        return;

    for(i=0; i<s1->rt_num_callers; i++) {
        if (rt_get_caller_pc(&pc, rc, i) < 0)
            break;
        pc = rt_printline(s1, pc, i ? "by" : "at");
        rt_printf("\n");
        if (pc == (addr_t)s1->rt_prog_main && pc)
            break;
    }
}

/* ------------------------------------------------------------- */

#ifndef _WIN32
# include <signal.h>
# ifndef __OpenBSD__
#  include <sys/ucontext.h>
# endif
#else
# define ucontext_t CONTEXT
#endif

/* translate from ucontext_t* to internal rt_context * */
static void rt_getcontext(ucontext_t *uc, rt_context *rc)
{
#if defined _WIN64
    rc->ip = uc->Rip;
    rc->fp = uc->Rbp;
    rc->sp = uc->Rsp;
#elif defined _WIN32
    rc->ip = uc->Eip;
    rc->fp = uc->Ebp;
    rc->sp = uc->Esp;
#elif defined __i386__
# if defined(__APPLE__)
    rc->ip = uc->uc_mcontext->__ss.__eip;
    rc->fp = uc->uc_mcontext->__ss.__ebp;
# elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
    rc->ip = uc->uc_mcontext.mc_eip;
    rc->fp = uc->uc_mcontext.mc_ebp;
# elif defined(__dietlibc__)
    rc->ip = uc->uc_mcontext.eip;
    rc->fp = uc->uc_mcontext.ebp;
# elif defined(__NetBSD__)
    rc->ip = uc->uc_mcontext.__gregs[_REG_EIP];
    rc->fp = uc->uc_mcontext.__gregs[_REG_EBP];
# elif defined(__OpenBSD__)
    rc->ip = uc->sc_eip;
    rc->fp = uc->sc_ebp;
# elif !defined REG_EIP && defined EIP /* fix for glibc 2.1 */
    rc->ip = uc->uc_mcontext.gregs[EIP];
    rc->fp = uc->uc_mcontext.gregs[EBP];
# else
    rc->ip = uc->uc_mcontext.gregs[REG_EIP];
    rc->fp = uc->uc_mcontext.gregs[REG_EBP];
# endif
#elif defined(__x86_64__)
# if defined(__APPLE__)
    rc->ip = uc->uc_mcontext->__ss.__rip;
    rc->fp = uc->uc_mcontext->__ss.__rbp;
# elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
    rc->ip = uc->uc_mcontext.mc_rip;
    rc->fp = uc->uc_mcontext.mc_rbp;
# elif defined(__NetBSD__)
    rc->ip = uc->uc_mcontext.__gregs[_REG_RIP];
    rc->fp = uc->uc_mcontext.__gregs[_REG_RBP];
# else
    rc->ip = uc->uc_mcontext.gregs[REG_RIP];
    rc->fp = uc->uc_mcontext.gregs[REG_RBP];
# endif
#elif defined(__arm__)
    rc->ip = uc->uc_mcontext.arm_pc;
    rc->fp = uc->uc_mcontext.arm_fp;
    rc->sp = uc->uc_mcontext.arm_sp;
#elif defined(__aarch64__)
    rc->ip = uc->uc_mcontext.pc;
    rc->fp = (addr_t *)uc->uc_mcontext.regs[29];
#endif
}

/* ------------------------------------------------------------- */
#ifndef _WIN32
/* signal handler for fatal errors */
static void sig_error(int signum, siginfo_t *siginf, void *puc)
{
    rt_context rc;

    rt_getcontext(puc, &rc);
    switch(signum) {
    case SIGFPE:
        switch(siginf->si_code) {
        case FPE_INTDIV:
        case FPE_FLTDIV:
            rt_error(&rc, "division by zero");
            break;
        default:
            rt_error(&rc, "floating point exception");
            break;
        }
        break;
    case SIGBUS:
    case SIGSEGV:
        rt_error(&rc, " dereferencing invalid pointer");
        break;
    case SIGILL:
        rt_error(&rc, "illegal instruction");
        break;
    case SIGABRT:
        rt_error(&rc, "abort() called");
        break;
    default:
        rt_error(&rc, "caught signal %d", signum);
        break;
    }
    exit(255);
}

#ifndef SA_SIGINFO
# define SA_SIGINFO 0x00000004u
#endif

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

#else /* WIN32 */
/* signal handler for fatal errors */
static long __stdcall cpu_exception_handler(EXCEPTION_POINTERS *ex_info)
{
    rt_context rc;
    unsigned code;

    rt_getcontext(ex_info->ContextRecord, &rc);
    switch (code = ex_info->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
	rt_error(&rc, " access violation");
        break;
    case EXCEPTION_STACK_OVERFLOW:
        rt_error(&rc, "stack overflow");
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        rt_error(&rc, "division by zero");
        break;
    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
        rc.ip = *(addr_t*)rc.sp;
        rt_error(&rc, "^breakpoint/single-step exception:");
        return EXCEPTION_CONTINUE_SEARCH;
    default:
        rt_error(&rc, "caught exception %08x", code);
        break;
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

/* Generate a stack backtrace when a CPU exception occurs. */
static void set_exception_handler(void)
{
    SetUnhandledExceptionFilter(cpu_exception_handler);
}

#endif

/* ------------------------------------------------------------- */
/* return the PC at frame level 'level'. Return negative if not found */
#if defined(__i386__) || defined(__x86_64__)
static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level)
{
    addr_t ip, fp;
    if (level == 0) {
        ip = rc->ip;
    } else {
        ip = 0;
        fp = rc->fp;
        while (--level) {
            /* XXX: check address validity with program info */
            if (fp <= 0x1000)
                break;
            fp = ((addr_t *)fp)[0];
        }
        if (fp > 0x1000)
            ip = ((addr_t *)fp)[1];
    }
    if (ip <= 0x1000)
        return -1;
    *paddr = ip;
    return 0;
}

#elif defined(__arm__)
static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level)
{
    /* XXX: only supports linux */
#if !defined(__linux__)
    return -1;
#else
    if (level == 0) {
        *paddr = rc->ip;
    } else {
        addr_t fp = rc->fp;
        addr_t sp = rc->sp;
        if (sp < 0x1000)
            sp = 0x1000;
        /* XXX: specific to tinycc stack frames */
        if (fp < sp + 12 || fp & 3)
            return -1;
        while (--level) {
            sp = ((addr_t *)fp)[-2];
            if (sp < fp || sp - fp > 16 || sp & 3)
                return -1;
            fp = ((addr_t *)fp)[-3];
            if (fp <= sp || fp - sp < 12 || fp & 3)
                return -1;
        }
        /* XXX: check address validity with program info */
        *paddr = ((addr_t *)fp)[-1];
    }
    return 0;
#endif
}

#elif defined(__aarch64__)
static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level)
{
   if (level == 0) {
        *paddr = rc->ip;
    } else {
        addr_t *fp = rc->fp;
        while (--level)
            fp = (addr_t *)fp[0];
        *paddr = fp[1];
    }
    return 0;
}

#else
#warning add arch specific rt_get_caller_pc()
static int rt_get_caller_pc(addr_t *paddr, rt_context *rc, int level)
{
    return -1;
}

#endif
#endif /* CONFIG_TCC_BACKTRACE */

/* ------------------------------------------------------------- */
#ifdef CONFIG_TCC_STATIC

/* dummy function for profiling */
ST_FUNC void *dlopen(const char *filename, int flag)
{
    return NULL;
}

ST_FUNC void dlclose(void *p)
{
}

ST_FUNC const char *dlerror(void)
{
    return "error";
}

typedef struct TCCSyms {
    char *str;
    void *ptr;
} TCCSyms;


/* add the symbol you want here if no dynamic linking is done */
static TCCSyms tcc_syms[] = {
#if !defined(CONFIG_TCCBOOT)
#define TCCSYM(a) { #a, &a, },
    TCCSYM(printf)
    TCCSYM(fprintf)
    TCCSYM(fopen)
    TCCSYM(fclose)
#undef TCCSYM
#endif
    { NULL, NULL },
};

ST_FUNC void *dlsym(void *handle, const char *symbol)
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

#endif /* CONFIG_TCC_STATIC */
#endif /* TCC_IS_NATIVE */
/* ------------------------------------------------------------- */
