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

#ifdef ONE_SOURCE
#include "libtcc.c"
#else
#include "tcc.h"
#endif

static void print_paths(const char *msg, char **paths, int nb_paths)
{
    int i;
    printf("%s:\n%s", msg, nb_paths ? "" : "  -\n");
    for(i = 0; i < nb_paths; i++)
        printf("  %s\n", paths[i]);
}

static void display_info(TCCState *s, int what)
{
    switch (what) {
    case 0:
        printf("tcc version %s ("
#ifdef TCC_TARGET_I386
        "i386"
#elif defined TCC_TARGET_X86_64
        "x86-64"
#elif defined TCC_TARGET_C67
        "C67"
#elif defined TCC_TARGET_ARM
        "ARM"
# ifdef TCC_ARM_HARDFLOAT
        " Hard Float"
# endif
#elif defined TCC_TARGET_ARM64
        "AArch64"
# ifdef TCC_ARM_HARDFLOAT
        " Hard Float"
# endif
#endif
#ifdef TCC_TARGET_PE
        " Windows"
#else
        " Linux"
#endif
        ")\n", TCC_VERSION);
        break;
    case 1:
        printf("install: %s\n", s->tcc_lib_path);
        /* print_paths("programs", NULL, 0); */
        print_paths("include", s->sysinclude_paths, s->nb_sysinclude_paths);
        print_paths("libraries", s->library_paths, s->nb_library_paths);
#ifndef TCC_TARGET_PE
        print_paths("crt", s->crt_paths, s->nb_crt_paths);
        printf("elfinterp:\n  %s\n",  DEFAULT_ELFINTERP(s));
#endif
        break;
    }
}

static void help(void)
{
    printf("Tiny C Compiler "TCC_VERSION" - Copyright (C) 2001-2006 Fabrice Bellard\n"
           "Usage: tcc [options...] [-o outfile] [-c] infile(s)...\n"
           "       tcc [options...] -run infile [arguments...]\n"
           "General options:\n"
           "  -c          compile only - generate an object file\n"
           "  -o outfile  set output filename\n"
           "  -run        run compiled source\n"
           "  -fflag      set or reset (with 'no-' prefix) 'flag' (see man page)\n"
           "  -Wwarning   set or reset (with 'no-' prefix) 'warning' (see man page)\n"
           "  -w          disable all warnings\n"
           "  -v          show version\n"
           "  -vv         show included files (as sole argument: show search paths)\n"
           "  -dumpversion\n"
           "  -bench      show compilation statistics\n"
           "  -xc -xa     specify type of the next infile\n"
           "  -           use stdin pipe as infile\n"
           "  @listfile   read arguments from listfile\n"
           "Preprocessor options:\n"
           "  -Idir       add include path 'dir'\n"
           "  -Dsym[=val] define 'sym' with value 'val'\n"
           "  -Usym       undefine 'sym'\n"
           "  -E          preprocess only\n"
           "  -P[1]       no / alternative #line output with -E\n"
           "  -dD -dM     output #define directives with -E\n"
           "Linker options:\n"
           "  -Ldir       add library path 'dir'\n"
           "  -llib       link with dynamic or static library 'lib'\n"
           "  -pthread    link with -lpthread and -D_REENTRANT (POSIX Linux)\n"
           "  -r          generate (relocatable) object file\n"
           "  -rdynamic   export all global symbols to dynamic linker\n"
           "  -shared     generate a shared library\n"
           "  -soname     set name for shared library to be used at runtime\n"
           "  -static     static linking\n"
           "  -Wl,-opt[=val]  set linker option (see manual)\n"
           "Debugger options:\n"
           "  -g          generate runtime debug info\n"
#ifdef CONFIG_TCC_BCHECK
           "  -b          compile with built-in memory and bounds checker (implies -g)\n"
#endif
#ifdef CONFIG_TCC_BACKTRACE
           "  -bt N       show N callers in stack traces\n"
#endif
           "Misc options:\n"
           "  -nostdinc   do not use standard system include paths\n"
           "  -nostdlib   do not link with standard crt and libraries\n"
           "  -Bdir       use 'dir' as tcc internal library and include path\n"
           "  -MD         generate target dependencies for make\n"
           "  -MF depfile put generated dependencies here\n"
           );
}

/* re-execute the i386/x86_64 cross-compilers with tcc -m32/-m64: */
#if defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64
#ifdef _WIN32
#include <process.h>
static int execvp_win32(const char *prog, char **argv)
{
    int ret = _spawnvp(P_NOWAIT, prog, (const char *const*)argv);
    if (-1 == ret)
        return ret;
    cwait(&ret, ret, WAIT_CHILD);
    exit(ret);
}
#define execvp execvp_win32
#endif
static void exec_other_tcc(TCCState *s, char **argv, const char *optarg)
{
    char child_path[4096], *child_name; const char *target;
    switch (atoi(optarg)) {
#ifdef TCC_TARGET_I386
        case 32: break;
        case 64: target = "x86_64";
#else
        case 64: break;
        case 32: target = "i386";
#endif
            pstrcpy(child_path, sizeof child_path - 40, argv[0]);
            child_name = tcc_basename(child_path);
            strcpy(child_name, target);
#ifdef TCC_TARGET_PE
            strcat(child_name, "-win32");
#endif
            strcat(child_name, "-tcc");
            if (strcmp(argv[0], child_path)) {
                if (s->verbose > 0)
                    printf("tcc: using '%s'\n", child_name), fflush(stdout);
                execvp(argv[0] = child_path, argv);
            }
            tcc_error("'%s' not found", child_name);
        case 0: /* ignore -march etc. */
            break;
        default:
            tcc_warning("unsupported option \"-m%s\"", optarg);
    }
}
#else
#define exec_other_tcc(s, argv, optarg)
#endif

static void gen_makedeps(TCCState *s, const char *target, const char *filename)
{
    FILE *depout;
    char buf[1024], *ext;
    int i;

    if (!filename) {
        /* compute filename automatically
         * dir/file.o -> dir/file.d             */
        pstrcpy(buf, sizeof(buf), target);
        ext = tcc_fileextension(buf);
        pstrcpy(ext, sizeof(buf) - (ext-buf), ".d");
        filename = buf;
    }

    if (s->verbose)
        printf("<- %s\n", filename);

    /* XXX return err codes instead of error() ? */
    depout = fopen(filename, "w");
    if (!depout)
        tcc_error("could not open '%s'", filename);

    fprintf(depout, "%s : \\\n", target);
    for (i=0; i<s->nb_target_deps; ++i)
        fprintf(depout, " %s \\\n", s->target_deps[i]);
    fprintf(depout, "\n");
    fclose(depout);
}

static char *default_outputfile(TCCState *s, const char *first_file)
{
    char buf[1024];
    char *ext;
    const char *name = "a";

    if (first_file && strcmp(first_file, "-"))
        name = tcc_basename(first_file);
    pstrcpy(buf, sizeof(buf), name);
    ext = tcc_fileextension(buf);
#ifdef TCC_TARGET_PE
    if (s->output_type == TCC_OUTPUT_DLL)
        strcpy(ext, ".dll");
    else
    if (s->output_type == TCC_OUTPUT_EXE)
        strcpy(ext, ".exe");
    else
#endif
    if (s->output_type == TCC_OUTPUT_OBJ && !s->option_r && *ext)
        strcpy(ext, ".o");
    else
        strcpy(buf, "a.out");

    return tcc_strdup(buf);
}

static int64_t getclock_us(void)
{
#ifdef _WIN32
    LARGE_INTEGER frequency, t1;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&t1);
    return t1.QuadPart * 1000000LL / frequency.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
#endif
}

int main(int argc, char **argv)
{
    TCCState *s;
    int ret, optind, i;
    int64_t start_time = 0;
    const char *first_file = NULL;

    s = tcc_new();

    optind = tcc_parse_args(s, argc - 1, argv + 1);

    tcc_set_environment(s);

    if (optind == 0) {
        help();
        return 1;
    }

    if (s->option_m)
        exec_other_tcc(s, argv, s->option_m);

    if (s->verbose)
        display_info(s, 0);

    if (s->nb_files == 0) {
        if (optind == 1) {
            if (s->print_search_dirs || s->verbose == 2) {
                tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
                display_info(s, 1);
                return 1;
            }
            if (s->verbose)
                return 1;
        }
        tcc_error("no input files\n");
    }

    /* check -c consistency : only single file handled. XXX: checks file type */
    if (s->output_type == TCC_OUTPUT_OBJ && !s->option_r) {
        if (s->nb_libraries != 0)
            tcc_error("cannot specify libraries with -c");
        /* accepts only a single input file */
        if (s->nb_files != 1)
            tcc_error("cannot specify multiple files with -c");
    }

    if (s->output_type == 0)
        s->output_type = TCC_OUTPUT_EXE;
    tcc_set_output_type(s, s->output_type);

    if (s->output_type == TCC_OUTPUT_PREPROCESS) {
	if (!s->outfile) {
	    s->ppfp = stdout;
	} else {
	    s->ppfp = fopen(s->outfile, "w");
	    if (!s->ppfp)
		tcc_error("could not write '%s'", s->outfile);
	}
    }

    if (s->do_bench)
        start_time = getclock_us();

    /* compile or add each files or library */
    for(i = ret = 0; i < s->nb_files && ret == 0; i++) {
        struct filespec *f = s->files[i];
        if (f->type == AFF_TYPE_LIB) {
            if (tcc_add_library_err(s, f->name) < 0)
                ret = 1;
        } else {
            if (1 == s->verbose)
                printf("-> %s\n", f->name);
            s->filetype = f->type;
            if (tcc_add_file(s, f->name) < 0)
                ret = 1;
            s->filetype = AFF_TYPE_NONE;
            if (!first_file)
                first_file = f->name;
        }
    }

    if (0 == ret) {
        if (s->do_bench)
            tcc_print_stats(s, getclock_us() - start_time);
        if (s->output_type == TCC_OUTPUT_MEMORY) {
#ifdef TCC_IS_NATIVE
            ret = tcc_run(s, argc - 1 - optind, argv + 1 + optind);
#endif
        } else {
            if (!s->outfile)
                s->outfile = default_outputfile(s, first_file);
            ret = !!tcc_output_file(s, s->outfile);
            if (s->gen_deps && !ret)
                gen_makedeps(s, s->outfile, s->deps_outfile);
        }
    }

    tcc_delete(s);
    return ret;
}
