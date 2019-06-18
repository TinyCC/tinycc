#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#if defined(_WIN32)
#endif

#if __linux__ || __APPLE__
#define SYS_WHICH_NM "which nm >/dev/null 2>&1"
#define TCC_COMPILER "../../tcc"
#define SYS_AWK

char c[]="/tmp/tcc-XXXXXX"; char o[]="/tmp/tcc-XXXXXX";
static int mktempfile(char *buf)
{
	return mkstemps(buf,0);
}
#elif defined(_WIN32)
#define SYS_WHICH_NM  "which nm >nul 2>&1"

#if defined(_WIN64)
#define TCC_COMPILER "..\\..\\win32\\x86_64-win32-tcc"
#else
#define TCC_COMPILER "..\\..\\win32\\i386-win32-tcc"
#endif

char c[1024]; char o[1024];
static int mktempfile(char *buf)
{
        /*
         * WARNING, this simplified 'mktemp' like function
         * create two temporary Windows files having always
         * the same name. It is enought for tcc test suite.
         */
        if (buf == c) {
                sprintf(c, "%s\\tcc-temp1", getenv("LOCALAPPDATA"));
        } else {
                sprintf(o, "%s\\tcc-temp2", getenv("LOCALAPPDATA"));
        }
}
#endif

void rmh(int Sig)
{
	remove(c);
	remove(o);
	signal(Sig,SIG_DFL);
	raise(Sig);
}
int str2file(char const *fnm, char const *str)
{
	FILE *f;
	if(0==(f=fopen(fnm,"w"))) return -1;
	if(0>fputs(str,f)) return -1;
	if(0>fclose(f)) return -1;
	return 0;
}
int main(int C, char **V)
{
	int r=0;
	if (system(SYS_WHICH_NM)){ return 0; }
   	signal(SIGINT,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	if(0>mktempfile(c)) return perror("mkstemps"),1;
	if(0>mktempfile(o)){
		if(0>remove(c)) perror("remove");
		return perror("mkstemps"),1;
	}
   	signal(SIGINT,rmh);
	signal(SIGTERM,rmh);
	extern char const cfileContents[];
	if(0>str2file(c, cfileContents)) { perror("write");r=1;goto out;}
	char buf[1024];
        sprintf(buf, "%s -c -xc %s -o %s", V[1]?V[1]:TCC_COMPILER, c, o); if(0!=system(buf)){ r=1;goto out;}
        sprintf(buf, "nm -Ptx %s > %s", o, c); if(system(buf)) {r=1;goto out;}
        sprintf(buf, "gawk '{ if($2 == \"T\") print $1 }' %s > %s", c, o); if(system(buf)) {r=1;goto out;}
        sprintf(buf, "sort %s", o); if(system(buf)) {r=1;goto out;}
out:
	remove(c);
	remove(o);
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
