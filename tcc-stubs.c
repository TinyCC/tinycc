/* Minimal stubs for syntax-only mode */

#define ONE_SOURCE 0
#include "tcc.h"
#define ONE_SOURCE 1

/* Code generation stubs (never called when nocode_wanted is set) */
void gen_opi(int op) {}
void gen_opf(int op) {}
void gen_opl(int op) {}
void gen_cvt_ftoi(int t) {}
void gen_cvt_ftof(int t) {}
void gen_cvt_itof(int t) {}
void gen_vla_sp_save(int addr) {}
void gen_vla_sp_restore(int addr) {}
void gen_vla_alloc(CType *type, int align) {}
void ggoto(void) {}
int gjmp(int t) { return 0; }
void gjmp_addr(int a) {}
int gtst(int inv, int t) { return 0; }
void gtst_addr(int inv, int a) {}
void gsym(int t) {}
void gsym_addr(int t, int a) {}
void load(int r, SValue *sv) {}
void store(int r, SValue *v) {}
void gfunc_call(int nb_args) {}
void gfunc_prolog(CType *func_type) {}
void gfunc_epilog(void) {}
int gfunc_sret(CType *vt, int variadic, CType *ret, int *align, int *regsize) { return 0; }
void o(unsigned int c) {}
void gen_le16(int c) {}
void gen_le32(int c) {}
int oad(int c, int s) { return 0; }
void g(int c) {}

/* x86_64 specific stubs */
int classify_x86_64_va_arg(CType *ty) { return 0; }

/* Assembly stubs */
void asm_instr(void) { tcc_error("inline assembly not supported in syntax-only mode"); }
void asm_global_instr(void) { tcc_error("inline assembly not supported in syntax-only mode"); }
int asm_parse_regvar(int t) { return -1; }
int tcc_assemble(TCCState *s1, int do_preprocess) { tcc_error("assembly not supported"); return -1; }

/* ELF/section management stubs */
void tccelf_begin_file(TCCState *s1) {}
void tccelf_end_file(TCCState *s1) {}
void tccelf_new(TCCState *s) {}
void tccelf_delete(TCCState *s1) {}
void tccelf_stab_new(TCCState *s) {}
int tcc_object_type(int fd, ElfW(Ehdr) *h) { return 0; }
int tcc_load_object_file(TCCState *s1, int fd, unsigned long file_offset) { return -1; }
int tcc_load_archive(TCCState *s1, int fd) { return -1; }
int tcc_load_dll(TCCState *s1, int fd, const char *filename, int level) { return -1; }
int tcc_load_ldscript(TCCState *s1) { return -1; }
int set_elf_sym(Section *s, addr_t value, unsigned long size, int info, int other, int shndx, const char *name) { return 0; }
int put_elf_sym(Section *s, addr_t value, unsigned long size, int info, int other, int shndx, const char *name) { return 0; }
void put_elf_reloca(Section *symtab, Section *s, unsigned long offset, int type, int symbol, addr_t addend) {}
void put_stabs(const char *str, int type, int other, int desc, unsigned long value) {}
void put_stabs_r(const char *str, int type, int other, int desc, unsigned long value, Section *sec, int sym_index) {}
void put_stabn(int type, int other, int desc, int value) {}
void put_stabd(int type, int other, int desc) {}
void squeeze_multi_relocs(Section *sec, size_t oldrelocoffset) {}
void section_realloc(Section *sec, unsigned long new_size) {}
size_t section_add(Section *sec, addr_t size, int align) { return 0; }
void section_reserve(Section *sec, unsigned long size) {}

/* Runtime stubs */
void tcc_run_free(TCCState *s1) {}
void tcc_set_num_callers(int n) {}

/* Section variables (dummy sections for syntax checking) */
static Section dummy_section = {0};
Section *text_section = &dummy_section;
Section *data_section = &dummy_section;
Section *bss_section = &dummy_section;
Section *common_section = &dummy_section;
Section *cur_text_section = &dummy_section;
Section *symtab_section = &dummy_section;

/* Backend data */
const int reg_classes[NB_REGS] = {0};

/* Additional backend functions */
Section *find_section(TCCState *s1, const char *name) { return &dummy_section; }
void *section_ptr_add(Section *sec, addr_t size) { return NULL; }

