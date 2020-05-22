/*
 *  Mach-O file handling for TCC
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

struct mach_header {
        uint32_t        magic;          /* mach magic number identifier */
        int             cputype;        /* cpu specifier */
        int             cpusubtype;     /* machine specifier */
        uint32_t        filetype;       /* type of file */
        uint32_t        ncmds;          /* number of load commands */
        uint32_t        sizeofcmds;     /* the size of all the load commands */
        uint32_t        flags;          /* flags */
};

struct mach_header_64 {
    struct mach_header  mh;
    uint32_t            reserved;       /* reserved, pad to 64bit */
};

/* Constant for the magic field of the mach_header (32-bit architectures) */
#define MH_MAGIC        0xfeedface      /* the mach magic number */
#define MH_CIGAM        0xcefaedfe      /* NXSwapInt(MH_MAGIC) */
#define MH_MAGIC_64     0xfeedfacf      /* the 64-bit mach magic number */
#define MH_CIGAM_64     0xcffaedfe      /* NXSwapInt(MH_MAGIC_64) */

struct load_command {
        uint32_t cmd;           /* type of load command */
        uint32_t cmdsize;       /* total size of command in bytes */
};

typedef int vm_prot_t;

struct segment_command { /* for 32-bit architectures */
        uint32_t        cmd;            /* LC_SEGMENT */
        uint32_t        cmdsize;        /* includes sizeof section structs */
        char            segname[16];    /* segment name */
        uint32_t        vmaddr;         /* memory address of this segment */
        uint32_t        vmsize;         /* memory size of this segment */
        uint32_t        fileoff;        /* file offset of this segment */
        uint32_t        filesize;       /* amount to map from the file */
        vm_prot_t       maxprot;        /* maximum VM protection */
        vm_prot_t       initprot;       /* initial VM protection */
        uint32_t        nsects;         /* number of sections in segment */
        uint32_t        flags;          /* flags */
};

struct segment_command_64 { /* for 64-bit architectures */
        uint32_t        cmd;            /* LC_SEGMENT_64 */
        uint32_t        cmdsize;        /* includes sizeof section_64 structs */
        char            segname[16];    /* segment name */
        uint64_t        vmaddr;         /* memory address of this segment */
        uint64_t        vmsize;         /* memory size of this segment */
        uint64_t        fileoff;        /* file offset of this segment */
        uint64_t        filesize;       /* amount to map from the file */
        vm_prot_t       maxprot;        /* maximum VM protection */
        vm_prot_t       initprot;       /* initial VM protection */
        uint32_t        nsects;         /* number of sections in segment */
        uint32_t        flags;          /* flags */
};

struct section { /* for 32-bit architectures */
        char            sectname[16];   /* name of this section */
        char            segname[16];    /* segment this section goes in */
        uint32_t        addr;           /* memory address of this section */
        uint32_t        size;           /* size in bytes of this section */
        uint32_t        offset;         /* file offset of this section */
        uint32_t        align;          /* section alignment (power of 2) */
        uint32_t        reloff;         /* file offset of relocation entries */
        uint32_t        nreloc;         /* number of relocation entries */
        uint32_t        flags;          /* flags (section type and attributes)*/
        uint32_t        reserved1;      /* reserved (for offset or index) */
        uint32_t        reserved2;      /* reserved (for count or sizeof) */
};

struct section_64 { /* for 64-bit architectures */
        char            sectname[16];   /* name of this section */
        char            segname[16];    /* segment this section goes in */
        uint64_t        addr;           /* memory address of this section */
        uint64_t        size;           /* size in bytes of this section */
        uint32_t        offset;         /* file offset of this section */
        uint32_t        align;          /* section alignment (power of 2) */
        uint32_t        reloff;         /* file offset of relocation entries */
        uint32_t        nreloc;         /* number of relocation entries */
        uint32_t        flags;          /* flags (section type and attributes)*/
        uint32_t        reserved1;      /* reserved (for offset or index) */
        uint32_t        reserved2;      /* reserved (for count or sizeof) */
        uint32_t        reserved3;      /* reserved */
};

typedef uint32_t lc_str;

struct dylib_command {
        uint32_t cmd;                   /* LC_ID_DYLIB, LC_LOAD_{,WEAK_}DYLIB,
                                           LC_REEXPORT_DYLIB */
        uint32_t cmdsize;               /* includes pathname string */
        lc_str   name;                  /* library's path name */
        uint32_t timestamp;             /* library's build time stamp */
        uint32_t current_version;       /* library's current version number */
        uint32_t compatibility_version; /* library's compatibility vers number*/
};

struct dylinker_command {
        uint32_t        cmd;            /* LC_ID_DYLINKER, LC_LOAD_DYLINKER or
                                           LC_DYLD_ENVIRONMENT */
        uint32_t        cmdsize;        /* includes pathname string */
        lc_str          name;           /* dynamic linker's path name */
};

struct symtab_command {
        uint32_t        cmd;            /* LC_SYMTAB */
        uint32_t        cmdsize;        /* sizeof(struct symtab_command) */
        uint32_t        symoff;         /* symbol table offset */
        uint32_t        nsyms;          /* number of symbol table entries */
        uint32_t        stroff;         /* string table offset */
        uint32_t        strsize;        /* string table size in bytes */
};

struct dysymtab_command {
    uint32_t cmd;       /* LC_DYSYMTAB */
    uint32_t cmdsize;   /* sizeof(struct dysymtab_command) */

    uint32_t ilocalsym; /* index to local symbols */
    uint32_t nlocalsym; /* number of local symbols */

    uint32_t iextdefsym;/* index to externally defined symbols */
    uint32_t nextdefsym;/* number of externally defined symbols */

    uint32_t iundefsym; /* index to undefined symbols */
    uint32_t nundefsym; /* number of undefined symbols */

    uint32_t tocoff;    /* file offset to table of contents */
    uint32_t ntoc;      /* number of entries in table of contents */

    uint32_t modtaboff; /* file offset to module table */
    uint32_t nmodtab;   /* number of module table entries */

    uint32_t extrefsymoff;      /* offset to referenced symbol table */
    uint32_t nextrefsyms;       /* number of referenced symbol table entries */

    uint32_t indirectsymoff; /* file offset to the indirect symbol table */
    uint32_t nindirectsyms;  /* number of indirect symbol table entries */

    uint32_t extreloff; /* offset to external relocation entries */
    uint32_t nextrel;   /* number of external relocation entries */
    uint32_t locreloff; /* offset to local relocation entries */
    uint32_t nlocrel;   /* number of local relocation entries */

};

struct linkedit_data_command {
    uint32_t    cmd;            /* LC_CODE_SIGNATURE, LC_SEGMENT_SPLIT_INFO,
                                   LC_FUNCTION_STARTS, LC_DATA_IN_CODE,
                                   LC_DYLIB_CODE_SIGN_DRS or
                                   LC_LINKER_OPTIMIZATION_HINT. */
    uint32_t    cmdsize;        /* sizeof(struct linkedit_data_command) */
    uint32_t    dataoff;        /* file offset of data in __LINKEDIT segment */
    uint32_t    datasize;       /* file size of data in __LINKEDIT segment  */
};

struct entry_point_command {
    uint32_t  cmd;      /* LC_MAIN only used in MH_EXECUTE filetypes */
    uint32_t  cmdsize;  /* 24 */
    uint64_t  entryoff; /* file (__TEXT) offset of main() */
    uint64_t  stacksize;/* if not zero, initial stack size */
};

enum skind {
    sk_unknown = 0,
    sk_discard,
    sk_text,
    sk_stubs,
    sk_ro_data,
    sk_uw_info,
    sk_nl_ptr,  // non-lazy pointers, aka GOT
    sk_la_ptr,  // lazy pointers
    sk_init,
    sk_fini,
    sk_rw_data,
    sk_bss,
    sk_linkedit,
    sk_last
};

struct nlist_64 {
    uint32_t  n_strx;      /* index into the string table */
    uint8_t n_type;        /* type flag, see below */
    uint8_t n_sect;        /* section number or NO_SECT */
    uint16_t n_desc;       /* see <mach-o/stab.h> */
    uint64_t n_value;      /* value of this symbol (or stab offset) */
};

struct macho {
    struct mach_header_64 mh;
    struct segment_command_64 *seg[4];
    int nseg;
    struct load_command **lc;
    int nlc;
    struct entry_point_command ep;
    struct {
        Section *s;
        int machosect;
    } sk_to_sect[sk_last];
    int *elfsectomacho;
    int *e2msym;
    Section *linkedit, *symtab, *strtab, *wdata, *indirsyms, *stubs;
    int stubsym;
    uint32_t ilocal, iextdef, iundef;
};

#define SHT_LINKEDIT (SHT_LOOS + 42)
#define SHN_FROMDLL  (SHN_LOOS + 2)  /* Symbol is undefined, comes from a DLL */

#define LC_REQ_DYLD 0x80000000
#define LC_SYMTAB        0x2
#define LC_DYSYMTAB      0xb
#define LC_LOAD_DYLIB    0xc
#define LC_LOAD_DYLINKER 0xe
#define LC_MAIN (0x28|LC_REQ_DYLD)

/* Hack for now, 46_grep.c needs fopen, but due to aliasing games
   in darwin headers it's searching for _fopen also via dlsym.  */
FILE *_fopen(const char*, const char*);
FILE *_fopen(const char * filename, const char *mode)
{
    return fopen(filename, mode);
}

static struct segment_command_64 * add_segment(struct macho *mo, char *name)
{
    struct segment_command_64 *sc = tcc_mallocz(sizeof *sc);
    strncpy(sc->segname, name, 16);
    sc->cmd = 0x19;  // LC_SEGMENT_64
    sc->cmdsize = sizeof(*sc);
    mo->seg[mo->nseg++] = sc;
    return sc;
}

static int add_section(struct macho *mo, struct segment_command_64 **_seg, char *name)
{
    struct segment_command_64 *seg = *_seg;
    int ret = seg->nsects;
    struct section_64 *sec;
    seg->nsects++;
    seg->cmdsize += sizeof(*sec);
    seg = tcc_realloc(seg, sizeof(*seg) + seg->nsects * sizeof(*sec));
    sec = (struct section_64*)((char*)seg + sizeof(*seg)) + ret;
    memset(sec, 0, sizeof(*sec));
    strncpy(sec->sectname, name, 16);
    strncpy(sec->segname, seg->segname, 16);
    *_seg = seg;
    return ret;
}

static struct section_64 *get_section(struct segment_command_64 *seg, int i)
{
    struct section_64 *sec;
    sec = (struct section_64*)((char*)seg + sizeof(*seg)) + i;
    return sec;
}

static void * add_lc(struct macho *mo, void *lc)
{
    mo->lc = tcc_realloc(mo->lc, sizeof(mo->lc[0]) * (mo->nlc + 1));
    mo->lc[mo->nlc++] = lc;
    return lc;
}

static void * add_dylib(struct macho *mo, char *name)
{
    struct dylib_command *lc;
    int sz = (sizeof(*lc) + strlen(name) + 1 + 7) & -8;
    lc = tcc_mallocz(sz);
    lc->cmd = LC_LOAD_DYLIB;
    lc->cmdsize = sz;
    lc->name = sizeof(*lc);
    strcpy((char*)lc + lc->name, name);
    lc->timestamp = 2;
    lc->current_version = 1 << 16;
    lc->compatibility_version = 1 << 16;
    return add_lc(mo, lc);
}

static void check_relocs(TCCState *s1, struct macho *mo)
{
    Section *s;
    ElfW_Rel *rel;
    ElfW(Sym) *sym;
    int i, type, gotplt_entry, sym_index, for_code;
    struct sym_attr *attr;

    s1->got = new_section(s1, ".got", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
    mo->indirsyms = new_section(s1, "LEINDIR", SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);
    for (i = 1; i < s1->nb_sections; i++) {
        s = s1->sections[i];
        if (s->sh_type != SHT_RELX)
            continue;
        for_each_elem(s, 0, rel, ElfW_Rel) {
            type = ELFW(R_TYPE)(rel->r_info);
            gotplt_entry = gotplt_entry_type(type);
            for_code = code_reloc(type);
            /* We generate a non-lazy pointer for used undefined symbols
               and for defined symbols that must have a place for their
               address due to codegen (i.e. a reloc requiring a got slot).  */
            sym_index = ELFW(R_SYM)(rel->r_info);
            sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
            if (sym->st_shndx == SHN_UNDEF
                || gotplt_entry == ALWAYS_GOTPLT_ENTRY) {
                attr = get_sym_attr(s1, sym_index, 1);
                if (!attr->dyn_index) {
                    uint32_t *pi = section_ptr_add(mo->indirsyms, sizeof(*pi));
                    attr->got_offset = s1->got->data_offset;
                    attr->plt_offset = -1;
                    attr->dyn_index = 1; /* used as flag */
                    section_ptr_add(s1->got, PTR_SIZE);
                    if (ELFW(ST_BIND)(sym->st_info) == STB_LOCAL) {
                        if (sym->st_shndx == SHN_UNDEF)
                          tcc_error("undefined local symbol???");
                        *pi = 0x80000000; /* INDIRECT_SYMBOL_LOCAL */
                        /* The pointer slot we generated must point to the
                           symbol, whose address is only known after layout,
                           so register a simply relocation for that.  */
                        put_elf_reloc(s1->symtab, s1->got, attr->got_offset,
                                      R_DATA_PTR, sym_index);
                    } else
                      *pi = mo->e2msym[sym_index];
                }
                if (for_code) {
                    if (attr->plt_offset == -1) {
                        uint8_t *jmp;
                        attr->plt_offset = mo->stubs->data_offset;
                        jmp = section_ptr_add(mo->stubs, 6);
                        jmp[0] = 0xff;  /* jmpq *ofs(%rip) */
                        jmp[1] = 0x25;
                        put_elf_reloc(s1->symtab, mo->stubs,
                                      attr->plt_offset + 2,
                                      R_X86_64_GOTPCREL, sym_index);
                    }
                    rel->r_info = ELFW(R_INFO)(mo->stubsym, type);
                    rel->r_addend += attr->plt_offset;
                }
            }
        }
    }
}

static int check_symbols(TCCState *s1, struct macho *mo)
{
    int sym_index, sym_end;
    int ret = 0;

    mo->ilocal = mo->iextdef = mo->iundef = -1;
    sym_end = symtab_section->data_offset / sizeof(ElfW(Sym));
    for (sym_index = 1; sym_index < sym_end; ++sym_index) {
        int elf_index = ((struct nlist_64 *)mo->symtab->data + sym_index - 1)->n_value;
        ElfW(Sym) *sym = (ElfW(Sym) *)symtab_section->data + elf_index;
        const char *name = (char*)symtab_section->link->data + sym->st_name;
        unsigned type = ELFW(ST_TYPE)(sym->st_info);
        unsigned bind = ELFW(ST_BIND)(sym->st_info);
        unsigned vis  = ELFW(ST_VISIBILITY)(sym->st_other);

        printf("%4d (%4d): %09llx %4d %4d %4d %3d %s\n",
               sym_index, elf_index, sym->st_value,
               type, bind, vis, sym->st_shndx, name);
        if (bind == STB_LOCAL) {
            if (mo->ilocal == -1)
              mo->ilocal = sym_index - 1;
            if (mo->iextdef != -1 || mo->iundef != -1)
              tcc_error("local syms after global ones");
        } else if (sym->st_shndx != SHN_UNDEF) {
            if (mo->iextdef == -1)
              mo->iextdef = sym_index - 1;
            if (mo->iundef != -1)
              tcc_error("external defined symbol after undefined");
        } else if (sym->st_shndx == SHN_UNDEF) {
            if (mo->iundef == -1)
              mo->iundef = sym_index - 1;
            if (ELFW(ST_BIND)(sym->st_info) == STB_WEAK)
                continue;
            if (get_sym_attr(s1, elf_index, 0)) {
                /* Mark the symbol as coming from a dylib so that
                   relocate_syms doesn't complain.  Normally we would have
                   checked that any dylib in fact exports this one, and listed
                   those in s1->dynsymtab, which would cause us to enter this
                   symbol into s1->dynsym, which is checked by relocate_syms.
                   But for now we fake this and regard all undefined as
                   coming from a dylib, and we don't use the dynsymtab/dynsym
                   scheme.  */
                sym->st_shndx = SHN_FROMDLL;
                continue;
            }
            tcc_error_noabort("undefined symbol '%s'", name);
            ret = -1;
        }
    }
    return ret;
}

static void convert_symbol(TCCState *s1, struct macho *mo, struct nlist_64 *pn)
{
    struct nlist_64 n = *pn;
    ElfSym *sym = (ElfW(Sym) *)symtab_section->data + pn->n_value;
    const char *name = (char*)symtab_section->link->data + sym->st_name;
    switch(ELFW(ST_TYPE)(sym->st_info)) {
    case STT_NOTYPE:
    case STT_OBJECT:
    case STT_FUNC:
    case STT_SECTION:
        n.n_type = 0xe;   /* default type is N_SECT */
        break;
    case STT_FILE:
        n.n_type = 2;     /* N_ABS */
        break;
    default:
        tcc_error("unhandled ELF symbol type %d %s",
                  ELFW(ST_TYPE)(sym->st_info), name);
    }
    if (sym->st_shndx == SHN_UNDEF)
      tcc_error("should have been rewritten to SHN_FROMDLL");
    else if (sym->st_shndx == SHN_FROMDLL)
      n.n_type = 0 /* N_UNDF */, n.n_sect = 0;
    else if (sym->st_shndx == SHN_ABS)
      n.n_type = 2 /* N_ABS */, n.n_sect = 0;
    else if (sym->st_shndx >= SHN_LORESERVE)
      tcc_error("unhandled ELF symbol section %d %s", sym->st_shndx, name);
    else if (!mo->elfsectomacho[sym->st_shndx])
      tcc_error("ELF section %d not mapped into Mach-O for symbol %s",
                sym->st_shndx, name);
    else
      n.n_sect = mo->elfsectomacho[sym->st_shndx];
    if (ELFW(ST_BIND)(sym->st_info) == STB_GLOBAL)
      n.n_type |= 1; /* N_EXT */
    else if (ELFW(ST_BIND)(sym->st_info) == STB_WEAK)
      tcc_error("weak symbol %s unhandled", name);
    n.n_strx = pn->n_strx;
    n.n_value = sym->st_value;
    *pn = n;
}

static void convert_symbols(TCCState *s1, struct macho *mo)
{
    int sym_index, sym_end;
    struct nlist_64 *pn = (struct nlist_64 *)mo->symtab->data;

    sym_end = symtab_section->data_offset / sizeof(ElfW(Sym));
    for (sym_index = 1; sym_index < sym_end; ++sym_index) {
        convert_symbol(s1, mo, pn + sym_index - 1);
    }
}

static int machosymcmp(const void *_a, const void *_b)
{
    TCCState *s1 = tcc_state;
    int ea = ((struct nlist_64 *)_a)->n_value;
    int eb = ((struct nlist_64 *)_b)->n_value;
    ElfSym *sa = (ElfSym *)symtab_section->data + ea;
    ElfSym *sb = (ElfSym *)symtab_section->data + eb;
    int r;
    r = (ELFW(ST_BIND)(sb->st_info) == STB_LOCAL)
        - (ELFW(ST_BIND)(sa->st_info) == STB_LOCAL);
    if (r)
      return r;
    r = (sa->st_shndx == SHN_UNDEF) - (sb->st_shndx == SHN_UNDEF);
    if (r)
      return r;
    if (ELFW(ST_BIND)(sa->st_info) != STB_LOCAL) {
        const char * na = (char*)symtab_section->link->data + sa->st_name;
        const char * nb = (char*)symtab_section->link->data + sb->st_name;
        r = strcmp(na, nb);
        if (r)
          return r;
    }
    return ea - eb;
}

static void create_symtab(TCCState *s1, struct macho *mo)
{
    int sym_index, sym_end;
    struct nlist_64 *pn;

    /* Stub creation belongs to check_relocs, but we need to create
       the symbol now, so its included in the sorting.  */
    mo->stubs = new_section(s1, "__stubs", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
    mo->stubsym = put_elf_sym(s1->symtab, 0, 0,
                              ELFW(ST_INFO)(STB_LOCAL, STT_NOTYPE), 0,
                              mo->stubs->sh_num, ".__stubs");

    mo->symtab = new_section(s1, "LESYMTAB", SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);
    mo->strtab = new_section(s1, "LESTRTAB", SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);
    put_elf_str(mo->strtab, " "); /* Mach-O starts strtab with a space */
    sym_end = symtab_section->data_offset / sizeof(ElfW(Sym));
    pn = section_ptr_add(mo->symtab, sizeof(*pn) * (sym_end - 1));
    for (sym_index = 1; sym_index < sym_end; ++sym_index) {
        ElfW(Sym) *sym = (ElfW(Sym) *)symtab_section->data + sym_index;
        const char *name = (char*)symtab_section->link->data + sym->st_name;
        pn[sym_index - 1].n_strx = put_elf_str(mo->strtab, name);
        pn[sym_index - 1].n_value = sym_index;
    }
    tcc_enter_state(s1);  /* qsort needs global state */
    qsort(pn, sym_end - 1, sizeof(*pn), machosymcmp);
    tcc_exit_state();
    mo->e2msym = tcc_malloc(sym_end * sizeof(*mo->e2msym));
    mo->e2msym[0] = -1;
    for (sym_index = 1; sym_index < sym_end; ++sym_index) {
        mo->e2msym[pn[sym_index - 1].n_value] = sym_index - 1;
    }
}

struct {
    int seg;
    uint32_t flags;
    char *name;
} skinfo[sk_last] = {
    [sk_text] =         { 1, 0x80000400, "__text" },
    [sk_ro_data] =      { 1, 0x0, "__rodata" },
    [sk_nl_ptr] =       { 2, 0x6, "__got" }, /* S_NON_LAZY_SYMBOL_POINTERS */
    [sk_init] =         { 2, 0x9, "__mod_init_func" }, /* S_MOD_INIT_FUNC_POINTERS */
    [sk_fini] =         { 2, 0xa, "__mod_term_func" }, /* S_MOD_TERM_FUNC_POINTERS */
    [sk_rw_data] =      { 2, 0x0, "__data" },
    [sk_bss] =          { 2, 0x1, "__bss" }, /* S_ZEROFILL */
    [sk_linkedit] =     { 3, 0x0, NULL },
};

static void collect_sections(TCCState *s1, struct macho *mo)
{
    int i, sk, numsec;
    uint64_t curaddr, fileofs;
    Section *s;
    struct segment_command_64 *seg = NULL;
    struct dylinker_command *dyldlc;
    struct symtab_command *symlc;
    struct dysymtab_command *dysymlc;
    char *str;
    //struct segment_command_64 *zc, *tc, *sc, *lc;
    add_segment(mo, "__PAGEZERO");
    add_segment(mo, "__TEXT");
    add_segment(mo, "__DATA");
    add_segment(mo, "__LINKEDIT");
    mo->seg[0]->vmsize = (uint64_t)1 << 32;
    mo->seg[1]->vmaddr = (uint64_t)1 << 32;
    mo->seg[1]->maxprot = 7;  // rwx
    mo->seg[1]->initprot = 5; // r-x
    mo->seg[2]->vmaddr = -1;
    mo->seg[2]->maxprot = 7;  // rwx
    mo->seg[2]->initprot = 3; // rw-
    mo->seg[3]->vmaddr = -1;
    mo->seg[3]->maxprot = 7;  // rwx
    mo->seg[3]->initprot = 1; // r--

    mo->ep.cmd = LC_MAIN;
    mo->ep.cmdsize = sizeof(mo->ep);
    mo->ep.entryoff = 4096;
    mo->ep.stacksize = 0;
    add_lc(mo, &mo->ep);

    i = (sizeof(*dyldlc) + strlen("/usr/lib/dyld") + 1 + 7) &-8;
    dyldlc = tcc_mallocz(i);
    dyldlc->cmd = LC_LOAD_DYLINKER;
    dyldlc->cmdsize = i;
    dyldlc->name = sizeof(*dyldlc);
    str = (char*)dyldlc + dyldlc->name;
    strcpy(str, "/usr/lib/dyld");
    add_lc(mo, dyldlc);

    symlc = tcc_mallocz(sizeof(*symlc));
    symlc->cmd = LC_SYMTAB;
    symlc->cmdsize = sizeof(*symlc);
    add_lc(mo, symlc);

    dysymlc = tcc_mallocz(sizeof(*dysymlc));
    dysymlc->cmd = LC_DYSYMTAB;
    dysymlc->cmdsize = sizeof(*dysymlc);
    add_lc(mo, dysymlc);

    add_dylib(mo, "/usr/lib/libSystem.B.dylib");

    mo->linkedit = new_section(s1, "LINKEDIT", SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);
    /* LINKEDIT can't be empty (XXX remove once we have symbol table) */
    section_ptr_add(mo->linkedit, 256);

    /* dyld requires a writable segment, but ignores zero-sized segments
       for this, so force to have some data.  */
    mo->wdata = new_section(s1, " wdata", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
    section_ptr_add(mo->wdata, 64);
    memset (mo->sk_to_sect, 0, sizeof(mo->sk_to_sect));
    for (i = s1->nb_sections; i-- > 1;) {
        int type, flags;
        s = s1->sections[i];
        type = s->sh_type;
        flags = s->sh_flags;
        sk = sk_unknown;
        if (flags & SHF_ALLOC) {
            switch (type) {
            default:           sk = sk_unknown; break;
            case SHT_INIT_ARRAY: sk = sk_init; break;
            case SHT_FINI_ARRAY: sk = sk_fini; break;
            case SHT_NOBITS:   sk = sk_bss; break;
            case SHT_SYMTAB:   sk = sk_discard; break;
            case SHT_STRTAB:   sk = s == stabstr_section ? sk_ro_data : sk_discard; break;
            case SHT_RELX:     sk = sk_discard; break;
            case SHT_LINKEDIT: sk = sk_linkedit; break;
            case SHT_PROGBITS:
                if (s == s1->got)
                  sk = sk_nl_ptr;
                else if (flags & SHF_EXECINSTR)
                  sk = sk_text;
                else if (flags & SHF_WRITE)
                  sk = sk_rw_data;
                else
                  sk = sk_ro_data;
                break;
            }
        } else
          sk = sk_discard;
        s->prev = mo->sk_to_sect[sk].s;
        mo->sk_to_sect[sk].s = s;
    }
    fileofs = 4096;  /* leave space for mach-o headers */
    curaddr = mo->seg[1]->vmaddr;
    curaddr += 4096;
    seg = NULL;
    numsec = 0;
    mo->elfsectomacho = tcc_mallocz(sizeof(*mo->elfsectomacho) * s1->nb_sections);
    for (sk = sk_unknown; sk < sk_last; sk++) {
        struct section_64 *sec = NULL;
        if (seg) {
            seg->vmsize = curaddr - seg->vmaddr;
            seg->filesize = fileofs - seg->fileoff;
        }
        if (skinfo[sk].seg && mo->sk_to_sect[sk].s) {
            uint64_t al = 0;
            int si;
            seg = mo->seg[skinfo[sk].seg];
            if (skinfo[sk].name) {
                si = add_section(mo, &seg, skinfo[sk].name);
                numsec++;
                mo->seg[skinfo[sk].seg] = seg;
                mo->sk_to_sect[sk].machosect = si;
                sec = get_section(seg, si);
                sec->flags = skinfo[sk].flags;
            }
            if (seg->vmaddr == -1) {
                curaddr = (curaddr + 4095) & -4096;
                seg->vmaddr = curaddr;
                fileofs = (fileofs + 4095) & -4096;
                seg->fileoff = fileofs;
            }

            for (s = mo->sk_to_sect[sk].s; s; s = s->prev) {
                int a = exact_log2p1(s->sh_addralign);
                if (a && al < (a - 1))
                  al = a - 1;
                s->sh_size = s->data_offset;
            }
            if (sec)
              sec->align = al;
            al = 1U << al;
            if (al > 4096)
              tcc_warning("alignment > 4096"), sec->align = 12, al = 4096;
            curaddr = (curaddr + al - 1) & -al;
            fileofs = (fileofs + al - 1) & -al;
            if (sec) {
                sec->addr = curaddr;
                sec->offset = fileofs;
            }
            for (s = mo->sk_to_sect[sk].s; s; s = s->prev) {
                al = s->sh_addralign;
                curaddr = (curaddr + al - 1) & -al;
                tcc_warning("curaddr now 0x%llx", curaddr);
                s->sh_addr = curaddr;
                curaddr += s->sh_size;
                if (s->sh_type != SHT_NOBITS) {
                    fileofs = (fileofs + al - 1) & -al;
                    s->sh_offset = fileofs;
                    fileofs += s->sh_size;
                    tcc_warning("fileofs now %lld", fileofs);
                }
                if (sec)
                  mo->elfsectomacho[s->sh_num] = numsec;
            }
            if (sec)
              sec->size = curaddr - sec->addr;
        }
        for (s = mo->sk_to_sect[sk].s; s; s = s->prev) {
          int type = s->sh_type;
          int flags = s->sh_flags;
          printf("%d section %-16s %-10s %09llx %04x %02d %s,%s,%s\n",
            sk,
            s->name,
            type == SHT_PROGBITS ? "progbits" :
            type == SHT_NOBITS ? "nobits" :
            type == SHT_SYMTAB ? "symtab" :
            type == SHT_STRTAB ? "strtab" :
            type == SHT_INIT_ARRAY ? "init" :
            type == SHT_FINI_ARRAY ? "fini" :
            type == SHT_RELX ? "rel" : "???",
            s->sh_addr,
            (unsigned)s->data_offset,
            s->sh_addralign,
            flags & SHF_ALLOC ? "alloc" : "",
            flags & SHF_WRITE ? "write" : "",
            flags & SHF_EXECINSTR ? "exec" : ""
            );
        }
    }
    if (seg) {
        seg->vmsize = curaddr - seg->vmaddr;
        seg->filesize = fileofs - seg->fileoff;
    }

    /* Fill symtab info */
    symlc->symoff = mo->symtab->sh_offset;
    symlc->nsyms = mo->symtab->data_offset / sizeof(struct nlist_64);
    symlc->stroff = mo->strtab->sh_offset;
    symlc->strsize = mo->strtab->data_offset;

    dysymlc->iundefsym = mo->iundef == -1 ? symlc->nsyms : mo->iundef;
    dysymlc->iextdefsym = mo->iextdef == -1 ? dysymlc->iundefsym : mo->iextdef;
    dysymlc->ilocalsym = mo->ilocal == -1 ? dysymlc->iextdefsym : mo->ilocal;
    dysymlc->nlocalsym = dysymlc->iextdefsym - dysymlc->ilocalsym;
    dysymlc->nextdefsym = dysymlc->iundefsym - dysymlc->iextdefsym;
    dysymlc->nundefsym = symlc->nsyms - dysymlc->iundefsym;
    dysymlc->indirectsymoff = mo->indirsyms->sh_offset;
    dysymlc->nindirectsyms = mo->indirsyms->data_offset / sizeof(uint32_t);
}

static void macho_write(TCCState *s1, struct macho *mo, FILE *fp)
{
    int i, sk;
    uint64_t fileofs = 0;
    Section *s;
    mo->mh.mh.magic = MH_MAGIC_64;
    mo->mh.mh.cputype = 0x1000007;    // x86_64
    mo->mh.mh.cpusubtype = 0x80000003;// all | CPU_SUBTYPE_LIB64
    mo->mh.mh.filetype = 2;           // MH_EXECUTE
    mo->mh.mh.flags = 4;              // DYLDLINK
    mo->mh.mh.ncmds = mo->nseg + mo->nlc;
    mo->mh.mh.sizeofcmds = 0;
    for (i = 0; i < mo->nseg; i++)
      mo->mh.mh.sizeofcmds += mo->seg[i]->cmdsize;
    for (i = 0; i < mo->nlc; i++)
      mo->mh.mh.sizeofcmds += mo->lc[i]->cmdsize;

    fwrite(&mo->mh, 1, sizeof(mo->mh), fp);
    fileofs += sizeof(mo->mh);
    for (i = 0; i < mo->nseg; i++) {
        fwrite(mo->seg[i], 1, mo->seg[i]->cmdsize, fp);
        fileofs += mo->seg[i]->cmdsize;
    }
    for (i = 0; i < mo->nlc; i++) {
        fwrite(mo->lc[i], 1, mo->lc[i]->cmdsize, fp);
        fileofs += mo->lc[i]->cmdsize;
    }

    for (sk = sk_unknown; sk < sk_last; sk++) {
        struct segment_command_64 *seg;
        //struct section_64 *sec;
        if (!skinfo[sk].seg || !mo->sk_to_sect[sk].s)
          continue;
        seg = mo->seg[skinfo[sk].seg];
        /*sec = get_section(seg, mo->sk_to_sect[sk].machosect);
        while (fileofs < sec->offset)
          fputc(0, fp), fileofs++;*/
        for (s = mo->sk_to_sect[sk].s; s; s = s->prev) {
            if (s->sh_type != SHT_NOBITS) {
                while (fileofs < s->sh_offset)
                  fputc(0, fp), fileofs++;
                if (s->sh_size) {
                    fwrite(s->data, 1, s->sh_size, fp);
                    fileofs += s->sh_size;
                }
            }
        }
    }
}

ST_FUNC int macho_output_file(TCCState *s1, const char *filename)
{
    int fd, mode, file_type;
    FILE *fp;
    int ret = -1;
    struct macho mo = {0,};

    file_type = s1->output_type;
    if (file_type == TCC_OUTPUT_OBJ)
        mode = 0666;
    else
        mode = 0777;
    unlink(filename);
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, mode);
    if (fd < 0) {
        tcc_error_noabort("could not write '%s: %s'", filename, strerror(errno));
        return -1;
    }
    fp = fdopen(fd, "wb");
    if (s1->verbose)
        printf("<- %s\n", filename);

    tcc_add_runtime(s1);
    resolve_common_syms(s1);
    create_symtab(s1, &mo);
    check_relocs(s1, &mo);
    ret = check_symbols(s1, &mo);
    if (!ret) {
        int i;
        Section *s;
        collect_sections(s1, &mo);
        relocate_syms(s1, s1->symtab, 0);
        mo.ep.entryoff = get_sym_addr(s1, "_main", 1, 0) - mo.seg[1]->vmaddr;
        if (s1->nb_errors)
          goto do_ret;

        for(i = 1; i < s1->nb_sections; i++) {
            s = s1->sections[i];
            if (s->reloc)
              relocate_section(s1, s);
        }
        convert_symbols(s1, &mo);

        macho_write(s1, &mo, fp);
    }
    ret = 0;

 do_ret:
    fclose(fp);
    return ret;
}
