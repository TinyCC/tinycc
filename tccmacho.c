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
    sk_rw_data,
    sk_bss,
    sk_linkedit,
    sk_last
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
    Section *linkedit, *wdata;
};

#define SHT_LINKEDIT (SHT_LOOS + 42)

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

static int check_symbols(TCCState *s1)
{
    ElfW(Sym) *sym;
    int sym_index, sym_end;
    int ret = 0;

    sym_end = symtab_section->data_offset / sizeof(ElfW(Sym));
    for (sym_index = 1; sym_index < sym_end; ++sym_index) {

        sym = (ElfW(Sym) *)symtab_section->data + sym_index;
        if (sym->st_shndx == SHN_UNDEF) {
            const char *name = (char*)symtab_section->link->data + sym->st_name;
            //unsigned type = ELFW(ST_TYPE)(sym->st_info);

            if (ELFW(ST_BIND)(sym->st_info) == STB_WEAK)
                /* STB_WEAK undefined symbols are accepted */
                continue;
            tcc_error_noabort("undefined symbol '%s'", name);
            ret = -1;
        }
    }
    return ret;
}

struct {
    int seg;
    uint32_t flags;
    char *name;
} skinfo[sk_last] = {
    [sk_text] =         { 1, 0x80000400, "__text" },
    [sk_ro_data] =      { 1, 0, "__rodata" },
    [sk_rw_data] =      { 2, 0, "__data" },
    [sk_bss] =          { 2, 1, "__bss" },
    [sk_linkedit] =     { 3, 0, NULL },
};

static void collect_sections(TCCState *s1, struct macho *mo)
{
    int i, sk;
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
    mo->ep.entryoff = 4096; // XXX
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
            case SHT_NOBITS:   sk = sk_bss; break;
            case SHT_SYMTAB:   sk = sk_discard; break;
            case SHT_STRTAB:   sk = sk_discard; break;
            case SHT_RELX:     sk = sk_discard; break;
            case SHT_LINKEDIT: sk = sk_linkedit; break;
            case SHT_PROGBITS:
                if (flags & SHF_EXECINSTR)
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
    int ret = 0;
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

    resolve_common_syms(s1);
    ret = check_symbols(s1);
    if (!ret) {
        collect_sections(s1, &mo);
        macho_write(s1, &mo, fp);
    }

    fclose(fp);
    return ret;
}
