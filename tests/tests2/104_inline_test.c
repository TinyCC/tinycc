#include <stdio.h>
#include <stdlib.h>

int str2file(const char *str, const char *fname)
{
	FILE *f = fopen(fname, "wb");
	return !(f && fputs(str, f) >= 0 && fclose(f) == 0);
}

/*
 * Wrap this sh script, which is compared to .expect (x.c is cFileContents):
 * tcc -c x.c && nm -Ptx x.o | awk '{ if($2 == "T") print $1 }' | LC_ALL=C sort
 * FIXME: Makefile should do it directly without this wrapper.
 *
 * Uses: sh, nm, awk, sort.
 * Good enough: we use fixed temp files at CWD to avoid abs path conversions
 * between *nix/Windows representations (shell, FILE APIs, tcc CLI arguments).
 */
#define TMP_SCRIPT "tmp-t104.sh"
#define TMP_C      "tmp-t104.c"
#define TMP_O      "tmp-t104.o"

int main(int argc, char **argv)
{
	extern char const cfileContents[];
	const char *tcc;
	char script[1024];
	int r = 1;
	/* errors at system(..) or the script show at the diff to .expect */

	/* TESTED_TCC is path/to/tcc + arguments */
	if (!(tcc = argc > 1 ? argv[1] : getenv("TESTED_TCC")))
		return fputs("unknown tcc executable", stderr), 1;

	sprintf(script,
	        "%s -c "TMP_C" -o "TMP_O" || exit 1\n"
	        "nm -Ptx "TMP_O" | awk '{ if($2 == \"T\") print $1 }' | LC_ALL=C sort\n"
	        , tcc);

	if (str2file(cfileContents, TMP_C) || str2file(script, TMP_SCRIPT))
		perror("create "TMP_C"/"TMP_SCRIPT);
	else
		r = system("sh "TMP_SCRIPT);

	remove(TMP_SCRIPT);
	remove(TMP_C);
	remove(TMP_O);
	return r;
}

char const cfileContents[]=
"inline void inline_inline_2decl_only(void);\n"
"inline void inline_inline_2decl_only(void);\n"
"\n"
"inline void inline_inline_undeclared(void){}\n"
"\n"
"inline void inline_inline_predeclared(void);\n"
"inline void inline_inline_predeclared(void){}\n"
"\n"
"inline void inline_inline_postdeclared(void){}\n"
"inline void inline_inline_postdeclared(void);\n"
"\n"
"inline void inline_inline_prepostdeclared(void);\n"
"inline void inline_inline_prepostdeclared(void){}\n"
"inline void inline_inline_prepostdeclared(void);\n"
"\n"
"inline void inline_inline_undeclared2(void){}\n"
"\n"
"inline void inline_inline_predeclared2(void);\n"
"inline void inline_inline_predeclared2(void);\n"
"inline void inline_inline_predeclared2(void){}\n"
"\n"
"inline void inline_inline_postdeclared2(void){}\n"
"inline void inline_inline_postdeclared2(void);\n"
"inline void inline_inline_postdeclared2(void);\n"
"\n"
"inline void inline_inline_prepostdeclared2(void);\n"
"inline void inline_inline_prepostdeclared2(void);\n"
"inline void inline_inline_prepostdeclared2(void){}\n"
"inline void inline_inline_prepostdeclared2(void);\n"
"inline void inline_inline_prepostdeclared2(void);\n"
"\n"
"extern void extern_extern_undeclared(void){}\n"
"\n"
"extern void extern_extern_predeclared(void);\n"
"extern void extern_extern_predeclared(void){}\n"
"\n"
"extern void extern_extern_postdeclared(void){}\n"
"extern void extern_extern_postdeclared(void);\n"
"\n"
"extern void extern_extern_prepostdeclared(void);\n"
"extern void extern_extern_prepostdeclared(void){}\n"
"extern void extern_extern_prepostdeclared(void);\n"
"\n"
"extern void extern_extern_undeclared2(void){}\n"
"\n"
"extern void extern_extern_predeclared2(void);\n"
"extern void extern_extern_predeclared2(void);\n"
"extern void extern_extern_predeclared2(void){}\n"
"\n"
"extern void extern_extern_postdeclared2(void){}\n"
"extern void extern_extern_postdeclared2(void);\n"
"extern void extern_extern_postdeclared2(void);\n"
"\n"
"extern void extern_extern_prepostdeclared2(void);\n"
"extern void extern_extern_prepostdeclared2(void);\n"
"extern void extern_extern_prepostdeclared2(void){}\n"
"extern void extern_extern_prepostdeclared2(void);\n"
"extern void extern_extern_prepostdeclared2(void);\n"
"\n"
"void extern_undeclared(void){}\n"
"\n"
"void extern_predeclared(void);\n"
"void extern_predeclared(void){}\n"
"\n"
"void extern_postdeclared(void){}\n"
"void extern_postdeclared(void);\n"
"\n"
"void extern_prepostdeclared(void);\n"
"void extern_prepostdeclared(void){}\n"
"void extern_prepostdeclared(void);\n"
"\n"
"void extern_undeclared2(void){}\n"
"\n"
"void extern_predeclared2(void);\n"
"void extern_predeclared2(void);\n"
"void extern_predeclared2(void){}\n"
"\n"
"void extern_postdeclared2(void){}\n"
"void extern_postdeclared2(void);\n"
"void extern_postdeclared2(void);\n"
"\n"
"\n"
"extern inline void noinst_extern_inline_undeclared(void){}\n"
"\n"
"extern inline void noinst_extern_inline_postdeclared(void){}\n"
"inline void noinst_extern_inline_postdeclared(void);\n"
"\n"
"extern inline void noinst_extern_inline_postdeclared2(void){}\n"
"inline void noinst_extern_inline_postdeclared2(void);\n"
"inline void noinst_extern_inline_postdeclared2(void);\n"
"\n"
"extern inline void inst_extern_inline_postdeclared(void){}\n"
"extern inline void inst_extern_inline_postdeclared(void);\n"
"inline void inst2_extern_inline_postdeclared(void){}\n"
"void inst2_extern_inline_postdeclared(void);\n"
"\n"
"void inst_extern_inline_predeclared(void);\n"
"extern inline void inst_extern_inline_predeclared(void){}\n"
"void inst2_extern_inline_predeclared(void);\n"
"inline void inst2_extern_inline_predeclared(void){}\n"
"extern inline void inst3_extern_inline_predeclared(void);\n"
"inline void inst3_extern_inline_predeclared(void){}\n"
"\n"
"static inline void noinst_static_inline_postdeclared(void){}\n"
"static inline void noinst_static_inline_postdeclared(void);\n"
"static inline void noinst2_static_inline_postdeclared(void){}\n"
"static void noinst2_static_inline_postdeclared(void);\n"
"\n"
"static void noinst_static_inline_predeclared(void);\n"
"static inline void noinst_static_inline_predeclared(void){}\n"
"static void noinst2_static_inline_predeclared(void);\n"
"static inline void noinst2_static_inline_predeclared(void){}\n"
"\n"
"static void static_func(void);\n"
"void static_func(void) { }\n"
"\n"
"inline void noinst_extern_inline_func(void);\n"
"void noinst_extern_inline_func(void) { }\n"
"int main()\n"
"{\n"
"	inline_inline_undeclared(); inline_inline_predeclared(); inline_inline_postdeclared();\n"
"	inline_inline_undeclared2(); inline_inline_predeclared2(); inline_inline_postdeclared2();\n"
"	noinst_static_inline_predeclared();\n"
"	noinst2_static_inline_predeclared();\n"
"	noinst_static_inline_predeclared();\n"
"	noinst2_static_inline_predeclared();\n"
"}\n"
"\n"
;
