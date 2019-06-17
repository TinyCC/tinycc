#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

extern char const cfileContents[];
char c[]="/tmp/tcc-XXXXXX.c";
char o[]="/tmp/tcc-XXXXXX.o";
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
	if (system("which nm >/dev/null 2>&1")){ return 0; }
   	signal(SIGINT,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	if(0>mkstemps(c,2)) return perror("mkstemps"),1;
	if(0>mkstemps(o,2)){
		if(0>remove(c)) perror("remove");
		return perror("mkstemps"),1;
	}
   	signal(SIGINT,rmh);
	signal(SIGTERM,rmh);
	if(0>str2file(c, cfileContents)) { perror("write");r=1;goto out;}
	char buf[1024];
	sprintf(buf, "%s -c %s -o %s", V[1]?V[1]:"../../tcc", c, o); if(0!=system(buf)){ r=1;goto out;}
	sprintf(buf, "nm -Ptx %s > %s", o, c); if(system(buf)) {r=1;goto out;}
	sprintf(buf, "awk '{ if($2 == \"T\") print $1 }' %s > %s", c, o); if(system(buf)) {r=1;goto out;}
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

