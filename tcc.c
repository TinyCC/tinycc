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

#include "tcc.h"
#if ONE_SOURCE
# include "libtcc.c"
#endif
#include "tcctools.c"

static const char help[] =
    "Tiny C Compiler "TCC_VERSION" - Copyright (C) 2001-2006 Fabrice Bellard\n"
    "Usage: tcc [options...] [-o outfile] [-c] infile(s)...\n"
    "       tcc [options...] -run infile [arguments...]\n"
    "General options:\n"
    "  -c           compile only - generate an object file\n"
    "  -o outfile   set output filename\n"
    "  -run         run compiled source\n"
    "  -fflag       set or reset (with 'no-' prefix) 'flag' (see tcc -hh)\n"
    "  -std=c99     Conform to the ISO 1999 C standard (default).\n"
    "  -std=c11     Conform to the ISO 2011 C standard.\n"
    "  -Wwarning    set or reset (with 'no-' prefix) 'warning' (see tcc -hh)\n"
    "  -w           disable all warnings\n"
    "  -v --version show version\n"
    "  -vv          show search paths or loaded files\n"
    "  -h -hh       show this, show more help\n"
    "  -bench       show compilation statistics\n"
    "  -            use stdin pipe as infile\n"
    "  @listfile    read arguments from listfile\n"
    "Preprocessor options:\n"
    "  -Idir        add include path 'dir'\n"
    "  -Dsym[=val]  define 'sym' with value 'val'\n"
    "  -Usym        undefine 'sym'\n"
    "  -E           preprocess only\n"
    "  -C           keep comments (not yet implemented)\n"
    "Linker options:\n"
    "  -Ldir        add library path 'dir'\n"
    "  -llib        link with dynamic or static library 'lib'\n"
    "  -r           generate (relocatable) object file\n"
    "  -shared      generate a shared library/dll\n"
    "  -rdynamic    export all global symbols to dynamic linker\n"
    "  -soname      set name for shared library to be used at runtime\n"
    "  -Wl,-opt[=val]  set linker option (see tcc -hh)\n"
    "Debugger options:\n"
    "  -g           generate runtime debug info\n"
#ifdef CONFIG_TCC_BCHECK
    "  -b           compile with built-in memory and bounds checker (implies -g)\n"
#endif
#ifdef CONFIG_TCC_BACKTRACE
    "  -bt[N]       link with backtrace (stack dump) support [show max N callers]\n"
#endif
    "Misc. options:\n"
    "  -x[c|a|b|n]  specify type of the next infile (C,ASM,BIN,NONE)\n"
    "  -nostdinc    do not use standard system include paths\n"
    "  -nostdlib    do not link with standard crt and libraries\n"
    "  -Bdir        set tcc's private include/library dir\n"
    "  -M[M]D       generate make dependency file [ignore system files]\n"
    "  -M[M]        as above but no other output\n"
    "  -MF file     specify dependency file name\n"
#if defined(TCC_TARGET_I386) || defined(TCC_TARGET_X86_64)
    "  -m32/64      defer to i386/x86_64 cross compiler\n"
#endif
    "Tools:\n"
    "  create library  : tcc -ar [rcsv] lib.a files\n"
#ifdef TCC_TARGET_PE
    "  create def file : tcc -impdef lib.dll [-v] [-o lib.def]\n"
#endif
    ;

static const char help2[] =
    "Tiny C Compiler "TCC_VERSION" - More Options\n"
    "Special options:\n"
    "  -P -P1                        with -E: no/alternative #line output\n"
    "  -dD -dM                       with -E: output #define directives\n"
    "  -pthread                      same as -D_REENTRANT and -lpthread\n"
    "  -On                           same as -D__OPTIMIZE__ for n > 0\n"
    "  -Wp,-opt                      same as -opt\n"
    "  -include file                 include 'file' above each input file\n"
    "  -isystem dir                  add 'dir' to system include path\n"
    "  -static                       link to static libraries (not recommended)\n"
    "  -dumpversion                  print version\n"
    "  -print-search-dirs            print search paths\n"
    "  -dt                           with -run/-E: auto-define 'test_...' macros\n"
    "Ignored options:\n"
    "  -arch -C --param -pedantic -pipe -s -traditional\n"
    "-W[no-]... warnings:\n"
    "  all                           turn on some (*) warnings\n"
    "  error[=warning]               stop after warning (any or specified)\n"
    "  write-strings                 strings are const\n"
    "  unsupported                   warn about ignored options, pragmas, etc.\n"
    "  implicit-function-declaration warn for missing prototype (*)\n"
    "  discarded-qualifiers          warn when const is dropped (*)\n"
    "-f[no-]... flags:\n"
    "  unsigned-char                 default char is unsigned\n"
    "  signed-char                   default char is signed\n"
    "  common                        use common section instead of bss\n"
    "  leading-underscore            decorate extern symbols\n"
    "  ms-extensions                 allow anonymous struct in struct\n"
    "  dollars-in-identifiers        allow '$' in C symbols\n"
    "  test-coverage                 create code coverage code\n"
    "-m... target specific options:\n"
    "  ms-bitfields                  use MSVC bitfield layout\n"
#ifdef TCC_TARGET_ARM
    "  float-abi                     hard/softfp on arm\n"
#endif
#ifdef TCC_TARGET_X86_64
    "  no-sse                        disable floats on x86_64\n"
#endif
    "-Wl,... linker options:\n"
    "  -nostdlib                     do not link with standard crt/libs\n"
    "  -[no-]whole-archive           load lib(s) fully/only as needed\n"
    "  -export-all-symbols           same as -rdynamic\n"
    "  -export-dynamic               same as -rdynamic\n"
    "  -image-base= -Ttext=          set base address of executable\n"
    "  -section-alignment=           set section alignment in executable\n"
#ifdef TCC_TARGET_PE
    "  -file-alignment=              set PE file alignment\n"
    "  -stack=                       set PE stack reserve\n"
    "  -large-address-aware          set related PE option\n"
    "  -subsystem=[console/windows]  set PE subsystem\n"
    "  -oformat=[pe-* binary]        set executable output format\n"
    "Predefined macros:\n"
    "  tcc -E -dM - < nul\n"
#else
    "  -rpath=                       set dynamic library search path\n"
    "  -enable-new-dtags             set DT_RUNPATH instead of DT_RPATH\n"
    "  -soname=                      set DT_SONAME elf tag\n"
    "  -Bsymbolic                    set DT_SYMBOLIC elf tag\n"
    "  -oformat=[elf32/64-* binary]  set executable output format\n"
    "  -init= -fini= -as-needed -O   (ignored)\n"
    "Predefined macros:\n"
    "  tcc -E -dM - < /dev/null\n"
#endif
    "See also the manual for more details.\n"
    ;

static const char version[] =
    "tcc version "TCC_VERSION
#ifdef TCC_GITHASH
    " "TCC_GITHASH
#endif
    " ("
#ifdef TCC_TARGET_I386
        "i386"
#elif defined TCC_TARGET_X86_64
        "x86_64"
#elif defined TCC_TARGET_C67
        "C67"
#elif defined TCC_TARGET_ARM
        "ARM"
# ifdef TCC_ARM_EABI
        " eabi"
#  ifdef TCC_ARM_HARDFLOAT
        "hf"
#  endif
# endif
#elif defined TCC_TARGET_ARM64
        "AArch64"
#elif defined TCC_TARGET_RISCV64
        "riscv64"
#endif
#ifdef TCC_TARGET_PE
        " Windows"
#elif defined(TCC_TARGET_MACHO)
        " Darwin"
#elif TARGETOS_FreeBSD || TARGETOS_FreeBSD_kernel
        " FreeBSD"
#elif TARGETOS_OpenBSD
        " OpenBSD"
#elif TARGETOS_NetBSD
        " NetBSD"
#else
        " Linux"
#endif
    ")\n"
    ;

static void print_dirs(const char *msg, char **paths, int nb_paths)
{
    int i;
    printf("%s:\n%s", msg, nb_paths ? "" : "  -\n");
    for(i = 0; i < nb_paths; i++)
        printf("  %s\n", paths[i]);
}

static void print_search_dirs(TCCState *S)
{
    printf("install: %s\n", S->tcc_lib_path);
    /* print_dirs("programs", NULL, 0); */
    print_dirs("include", S->sysinclude_paths, S->nb_sysinclude_paths);
    print_dirs("libraries", S->library_paths, S->nb_library_paths);
#ifdef TCC_TARGET_PE
    printf("libtcc1:\n  %s/lib/"TCC_LIBTCC1"\n", S->tcc_lib_path);
#else
    printf("libtcc1:\n  %s/"TCC_LIBTCC1"\n", S->tcc_lib_path);
    print_dirs("crt", S->crt_paths, S->nb_crt_paths);
    printf("elfinterp:\n  %s\n",  DEFAULT_ELFINTERP(S));
#endif
}

static void set_environment(TCCState *S)
{
    char * path;

    path = getenv("C_INCLUDE_PATH");
    if(path != NULL) {
        tcc_add_sysinclude_path(S, path);
    }
    path = getenv("CPATH");
    if(path != NULL) {
        tcc_add_include_path(S, path);
    }
    path = getenv("LIBRARY_PATH");
    if(path != NULL) {
        tcc_add_library_path(S, path);
    }
}

static char *default_outputfile(TCCState *S, const char *first_file)
{
    char buf[1024];
    char *ext;
    const char *name = "a";

    if (first_file && strcmp(first_file, "-"))
        name = tcc_basename(first_file);
    snprintf(buf, sizeof(buf), "%s", name);
    ext = tcc_fileextension(buf);
#ifdef TCC_TARGET_PE
    if (S->output_type == TCC_OUTPUT_DLL)
        strcpy(ext, ".dll");
    else
    if (S->output_type == TCC_OUTPUT_EXE)
        strcpy(ext, ".exe");
    else
#endif
    if ((S->just_deps || S->output_type == TCC_OUTPUT_OBJ) && !S->option_r && *ext)
        strcpy(ext, ".o");
    else
        strcpy(buf, "a.out");
    return tcc_strdup(S, buf);
}

static unsigned getclock_ms(void)
{
#ifdef _WIN32
    return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000 + (tv.tv_usec+500)/1000;
#endif
}

#ifdef WITH_ATTACHMENTS
#include "tcc_attachments.h"
#define ATTACH_PREFIX "/_attach_"

static vio_module_t vio_module;

typedef struct vio_memfile_t {
    off_t size;
    off_t pos;
    const unsigned char *mem;
} vio_memfile_t;

static int vio_mem_open(vio_fd *fd, const char *fn, int oflag) {
    //printf("%d:%s\n", fd->fd, fn);
    if(fd->vio_module && strncmp(ATTACH_PREFIX, fn, sizeof(ATTACH_PREFIX)-1) == 0){
        int i, count = sizeof(bin2c_filesAttached)/sizeof(bin2c_filesAttached_st);
        for(i=0; i < count; ++i) {
            //printf("%s:%s\n", fn, bin2c_filesAttached[i].file_name);
            if(strcmp(fn, bin2c_filesAttached[i].file_name) == 0) {
                vio_memfile_t *mf = (vio_memfile_t*)tcc_malloc(S, fd->vio_module->tcc_state);
                mf->mem = bin2c_filesAttached[i].sym_name;
                mf->size = bin2c_filesAttached[i].size;
                mf->pos = 0;
                fd->fd = 1;
                fd->vio_udata = mf;
		//printf("%d:%s\n", fd->fd, fn);
                return fd->fd;
            }
        }
    }
    return -1;
}

static off_t vio_mem_lseek(vio_fd fd, off_t offset, int whence) {
    if(fd.vio_udata) {
        off_t loffset = 0;
        vio_memfile_t *mf = (vio_memfile_t*)fd.vio_udata;
        if (whence == SEEK_CUR)
            loffset = mf->pos + offset;
        else if (whence == SEEK_SET)
            loffset = offset;
        else if (whence == SEEK_END)
            loffset = ((off_t)mf->size) + offset;

        if (loffset < 0 && loffset > mf->size)
            return -1;

        mf->pos = loffset;

        return mf->pos;
    }
    return lseek(fd.fd, offset, whence);
}

static size_t vio_mem_read(vio_fd fd, void *buf, size_t bytes) {
    if(fd.vio_udata) {
        vio_memfile_t *mf = (vio_memfile_t*)fd.vio_udata;
        if( (mf->pos + bytes) > mf->size) {
            long bc = mf->size - mf->pos;
            if(bc > 0) {
                memcpy(buf, mf->mem + mf->pos, bc);
                mf->pos = mf->size;
                return bc;
            }
            return 0;
        }
        memcpy(buf, mf->mem + mf->pos, bytes);
        mf->pos += bytes;
        return bytes;
    }
    return 0;
}

static int vio_mem_close(vio_fd *fd) {
    if(fd->vio_udata){
        tcc_free(S, fd->vio_udata);
    }
    return 0;
}

void set_vio_module(TCCState *S){
    vio_module.user_data = NULL;
    vio_module.call_vio_open_flags = CALL_VIO_OPEN_FIRST;
    vio_module.vio_open = &vio_mem_open;
    vio_module.vio_lseek = &vio_mem_lseek;
    vio_module.vio_read = &vio_mem_read;
    vio_module.vio_close = &vio_mem_close;
    tcc_set_vio_module(s, &vio_module);
}

#endif

int main(int argc0, char **argv0)
{
    TCCState *S, *s1;
    int ret, opt, n = 0, t = 0, done;
    unsigned start_time = 0, end_time = 0;
    const char *first_file;
    int argc; char **argv;
    FILE *ppfp = stdout;

redo:
    argc = argc0, argv = argv0;
    S = s1 = tcc_new();
    opt = tcc_parse_args(S, &argc, &argv, 1);

#ifdef WITH_ATTACHMENTS
    tcc_set_lib_path(S, ATTACH_PREFIX);
    tcc_add_include_path(S, ATTACH_PREFIX);
    set_vio_module(S);
#endif

    if (n == 0) {
        if (opt == OPT_HELP) {
            fputs(help, stdout);
            if (!S->verbose)
                return 0;
            ++opt;
        }
        if (opt == OPT_HELP2) {
            fputs(help2, stdout);
            return 0;
        }
        if (opt == OPT_M32 || opt == OPT_M64)
            tcc_tool_cross(S, argv, opt); /* never returns */
        if (S->verbose)
            printf(version);
        if (opt == OPT_AR)
            return tcc_tool_ar(S, argc, argv);
#ifdef TCC_TARGET_PE
        if (opt == OPT_IMPDEF)
            return tcc_tool_impdef(S, argc, argv);
#endif
        if (opt == OPT_V)
            return 0;
        if (opt == OPT_PRINT_DIRS) {
            /* initialize search dirs */
            set_environment(S);
            tcc_set_output_type(S, TCC_OUTPUT_MEMORY);
            print_search_dirs(S);
            return 0;
        }

        if (S->nb_files == 0)
            tcc_error(S, "no input files");

        if (S->output_type == TCC_OUTPUT_PREPROCESS) {
            if (S->outfile && 0!=strcmp("-",S->outfile)) {
                ppfp = fopen(S->outfile, "w");
                if (!ppfp)
                    tcc_error(S, "could not write '%s'", S->outfile);
            }
        } else if (S->output_type == TCC_OUTPUT_OBJ && !S->option_r) {
            if (S->nb_libraries)
                tcc_error(S, "cannot specify libraries with -c");
            if (S->nb_files > 1 && S->outfile)
                tcc_error(S, "cannot specify output file with -c many files");
        }

        if (S->do_bench)
            start_time = getclock_ms();
    }

    set_environment(S);
    if (S->output_type == 0)
        S->output_type = TCC_OUTPUT_EXE;
    tcc_set_output_type(S, S->output_type);
    S->ppfp = ppfp;

    if ((S->output_type == TCC_OUTPUT_MEMORY
      || S->output_type == TCC_OUTPUT_PREPROCESS)
        && (S->dflag & TCC_OPTION_d_t)) { /* -dt option */
        if (t)
            S->dflag |= TCC_OPTION_d_32;
        S->run_test = ++t;
        if (n)
            --n;
    }

    /* compile or add each files or library */
    first_file = NULL, ret = 0;
    do {
        struct filespec *f = S->files[n];
        S->filetype = f->type;
        if (f->type & AFF_TYPE_LIB) {
            if (tcc_add_library_err(S, f->name) < 0)
                ret = 1;
        } else {
            if (1 == S->verbose)
                printf("-> %s\n", f->name);
            if (!first_file)
                first_file = f->name;
            if (tcc_add_file(S, f->name) < 0)
                ret = 1;
        }
        done = ret || ++n >= S->nb_files;
    } while (!done && (S->output_type != TCC_OUTPUT_OBJ || S->option_r));

    if (S->do_bench)
        end_time = getclock_ms();

    if (S->run_test) {
        t = 0;
    } else if (S->output_type == TCC_OUTPUT_PREPROCESS) {
        ;
    } else if (0 == ret) {
        if (S->output_type == TCC_OUTPUT_MEMORY) {
#ifdef TCC_IS_NATIVE
            ret = tcc_run(S, argc, argv);
#endif
        } else {
            if (!S->outfile)
                S->outfile = default_outputfile(S, first_file);
            if (!S->just_deps && tcc_output_file(S, S->outfile))
                ret = 1;
            else if (S->gen_deps)
                gen_makedeps(S, S->outfile, S->deps_outfile);
        }
    }

    if (done && 0 == t && 0 == ret && S->do_bench)
        tcc_print_stats(S, end_time - start_time);

    tcc_delete(S);
    if (!done)
        goto redo; /* compile more files with -c */
    if (t)
        goto redo; /* run more tests with -dt -run */

    if (ppfp && ppfp != stdout)
        fclose(ppfp);
    return ret;
}
