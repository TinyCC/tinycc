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

static void help(void)
{
    printf("tcc version " TCC_VERSION " - Tiny C Compiler - Copyright (C) 2001-2006 Fabrice Bellard\n"
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
           "Preprocessor options:\n"
           "  -E          preprocess only\n"
           "  -Idir       add include path 'dir'\n"
           "  -Dsym[=val] define 'sym' with value 'val'\n"
           "  -Usym       undefine 'sym'\n"
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
    int ret = spawnvp(P_NOWAIT, prog, (char const*const*)argv);
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
    if (( (s->output_type == TCC_OUTPUT_OBJ && !s->option_r) ||
          (s->output_type == TCC_OUTPUT_PREPROCESS) )
        && *ext)
        strcpy(ext, ".o");
    else
        strcpy(buf, "a.out");

    return tcc_strdup(buf);
}

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
# ifdef TCC_TARGET_PE
        " Win32"
# endif
#elif defined TCC_TARGET_X86_64
        "x86-64"
# ifdef TCC_TARGET_PE
        " Win64"
# endif
#elif defined TCC_TARGET_ARM
        "ARM"
# ifdef TCC_ARM_HARDFLOAT
        " Hard Float"
# endif
# ifdef TCC_TARGET_PE
        " WinCE"
# endif
#endif
#ifndef TCC_TARGET_PE
# ifdef __linux
        " Linux"
# endif
#endif
        ")\n", TCC_VERSION);
        break;
    case 1:
        printf("install: %s/\n", s->tcc_lib_path);
        /* print_paths("programs", NULL, 0); */
        print_paths("crt", s->crt_paths, s->nb_crt_paths);
        print_paths("libraries", s->library_paths, s->nb_library_paths);
        print_paths("include", s->sysinclude_paths, s->nb_sysinclude_paths);
        printf("elfinterp:\n  %s\n",  CONFIG_TCC_ELFINTERP);
        break;
    }
}

static int64_t getclock_us(void)
{
#ifdef _WIN32
    struct _timeb tb;
    _ftime(&tb);
    return (tb.time * 1000LL + tb.millitm) * 1000LL;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
#endif
}

int main(int argc, char **argv)
{
    TCCState *s;
    int ret, optind, i, bench;
    int64_t start_time = 0;
    const char *first_file = NULL;

    s = tcc_new();
    s->output_type = TCC_OUTPUT_EXE;

    optind = tcc_parse_args(s, argc - 1, argv + 1);

    if (optind == 0) {
        help();
        return 1;
    }

    if (s->option_m)
        exec_other_tcc(s, argv, s->option_m);

    if (s->verbose)
        display_info(s, 0);

    if (s->print_search_dirs || (s->verbose == 2 && optind == 1)) {
        tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
        display_info(s, 1);
        return 0;
    }

    if (s->verbose && optind == 1)
        return 0;

    if (s->nb_files == 0)
        tcc_error("no input files\n");

    /* check -c consistency : only single file handled. XXX: checks file type */
    if (s->output_type == TCC_OUTPUT_OBJ && !s->option_r) {
        if (s->nb_libraries != 0)
            tcc_error("cannot specify libraries with -c");
        /* accepts only a single input file */
        if (s->nb_files != 1)
            tcc_error("cannot specify multiple files with -c");
    }
    
    if (s->output_type == TCC_OUTPUT_PREPROCESS) {
        if (!s->outfile) {
            s->ppfp = stdout;
        } else {
            s->ppfp = fopen(s->outfile, "w");
            if (!s->ppfp)
                tcc_error("could not write '%s'", s->outfile);
        }
    }

    bench = s->do_bench;
    if (bench)
        start_time = getclock_us();

    tcc_set_output_type(s, s->output_type);

    /* compile or add each files or library */
    for(i = ret = 0; i < s->nb_files && ret == 0; i++) {
        const char *filename;

        filename = s->files[i];
        if (filename[0] == '-' && filename[1] == 'l') {
            if (tcc_add_library(s, filename + 2) < 0) {
                tcc_error_noabort("cannot find '%s'", filename);
                ret = 1;
            }
        } else {
            if (1 == s->verbose)
                printf("-> %s\n", filename);
            if (tcc_add_file(s, filename) < 0)
                ret = 1;
            if (!first_file)
                first_file = filename;
        }
    }

    if (0 == ret) {
        if (bench)
            tcc_print_stats(s, getclock_us() - start_time);

        if (s->output_type == TCC_OUTPUT_MEMORY) {
#ifdef TCC_IS_NATIVE
            ret = tcc_run(s, argc - 1 - optind, argv + 1 + optind);
#else
            tcc_error_noabort("-run is not available in a cross compiler");
            ret = 1;
#endif
        } else if (s->output_type == TCC_OUTPUT_PREPROCESS) {
             if (s->outfile)
                fclose(s->ppfp);
        } else {
            if (!s->outfile)
                s->outfile = default_outputfile(s, first_file);
            ret = !!tcc_output_file(s, s->outfile);
            /* dump collected dependencies */
            if (s->gen_deps && !ret)
                gen_makedeps(s, s->outfile, s->deps_outfile);
        }
    }

    tcc_delete(s);
    if (bench)
        tcc_memstats();
    return ret;
}
