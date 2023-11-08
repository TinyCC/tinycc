/*
 * Mach-O file handling for TCC
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

/* In order to make life easy for us we are generating Mach-O files which
   don't make use of some modern features, but which aren't entirely classic
   either in that they do use some modern features.  We're also only
   generating 64bit Mach-O files, and only native endian at that.

   In particular we're generating executables that don't make use of
   DYLD_INFO for dynamic linking info, as that requires us building a
   trie of exported names.  We're simply using classic symbol tables which
   are still supported by modern dyld.

   But we do use LC_MAIN, which is a "modern" feature in order to not have
   to setup our own crt code.  We're not using lazy linking, so even function
   calls are resolved at startup.  */

#if !defined TCC_TARGET_X86_64 && !defined TCC_TARGET_ARM64
#error Platform not supported
#endif

/* XXX: this file uses tcc_error() to the effect of exit(1) */
#undef _tcc_error

#define DEBUG_MACHO 0
#define dprintf if (DEBUG_MACHO) printf

#define MH_EXECUTE              (0x2)
#define MH_DYLDLINK             (0x4)
#define MH_DYLIB                (0x6)
#define MH_PIE                  (0x200000)

#define CPU_SUBTYPE_LIB64       (0x80000000)
#define CPU_SUBTYPE_X86_ALL     (3)
#define CPU_SUBTYPE_ARM64_ALL   (0)

#define CPU_ARCH_ABI64          (0x01000000)

#define CPU_TYPE_X86            (7)
#define CPU_TYPE_X86_64         (CPU_TYPE_X86 | CPU_ARCH_ABI64)
#define CPU_TYPE_ARM            (12)
#define CPU_TYPE_ARM64          (CPU_TYPE_ARM | CPU_ARCH_ABI64)

struct fat_header {
    uint32_t        magic;          /* FAT_MAGIC or FAT_MAGIC_64 */
    uint32_t        nfat_arch;      /* number of structs that follow */
};

struct fat_arch {
    int             cputype;        /* cpu specifier (int) */
    int             cpusubtype;     /* machine specifier (int) */
    uint32_t        offset;         /* file offset to this object file */
    uint32_t        size;           /* size of this object file */
    uint32_t        align;          /* alignment as a power of 2 */
};

#define FAT_MAGIC       0xcafebabe
#define FAT_CIGAM       0xbebafeca
#define FAT_MAGIC_64    0xcafebabf
#define FAT_CIGAM_64    0xbfbafeca

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
    uint32_t        cmd;            /* type of load command */
    uint32_t        cmdsize;        /* total size of command in bytes */
};

#define LC_REQ_DYLD 0x80000000
#define LC_SYMTAB        0x2
#define LC_DYSYMTAB      0xb
#define LC_LOAD_DYLIB    0xc
#define LC_ID_DYLIB      0xd
#define LC_LOAD_DYLINKER 0xe
#define LC_SEGMENT_64    0x19
#define LC_RPATH (0x1c | LC_REQ_DYLD)
#define LC_REEXPORT_DYLIB (0x1f | LC_REQ_DYLD)
#define LC_DYLD_INFO_ONLY (0x22|LC_REQ_DYLD)
#define LC_MAIN (0x28|LC_REQ_DYLD)
#define LC_SOURCE_VERSION 0x2A
#define LC_BUILD_VERSION 0x32
#define LC_DYLD_EXPORTS_TRIE (0x33 | LC_REQ_DYLD)
#define LC_DYLD_CHAINED_FIXUPS (0x34 | LC_REQ_DYLD)

#define SG_READ_ONLY    0x10 /* This segment is made read-only after fixups */

typedef int vm_prot_t;

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

enum {
    DYLD_CHAINED_IMPORT          = 1,
};

struct dyld_chained_fixups_header {
    uint32_t    fixups_version; ///< 0
    uint32_t    starts_offset;  ///< Offset of dyld_chained_starts_in_image.
    uint32_t    imports_offset; ///< Offset of imports table in chain_data.
    uint32_t    symbols_offset; ///< Offset of symbol strings in chain_data.
    uint32_t    imports_count;  ///< Number of imported symbol names.
    uint32_t    imports_format; ///< DYLD_CHAINED_IMPORT*
    uint32_t    symbols_format; ///< 0 => uncompressed, 1 => zlib compressed
};

struct dyld_chained_starts_in_image
{
    uint32_t    seg_count;
    uint32_t    seg_info_offset[1];  // each entry is offset into this struct for that segment
    // followed by pool of dyld_chain_starts_in_segment data
};

enum {
    DYLD_CHAINED_PTR_64                     =  2,    // target is vmaddr
    DYLD_CHAINED_PTR_64_OFFSET              =  6,    // target is vm offset
};

enum {
    DYLD_CHAINED_PTR_START_NONE   = 0xFFFF, // used in page_start[] to denote a page with no fixups
};

#define SEG_PAGE_SIZE 16384

struct dyld_chained_starts_in_segment
{
    uint32_t    size;               // size of this (amount kernel needs to copy)
    uint16_t    page_size;          // 0x1000 or 0x4000
    uint16_t    pointer_format;     // DYLD_CHAINED_PTR_*
    uint64_t    segment_offset;     // offset in memory to start of segment
    uint32_t    max_valid_pointer;  // for 32-bit OS, any value beyond this is not a pointer
    uint16_t    page_count;         // how many pages are in array
    uint16_t    page_start[1];      // each entry is offset in each page of first element in chain
                                    // or DYLD_CHAINED_PTR_START_NONE if no fixups on page
};

enum BindSpecialDylib {
  BIND_SPECIAL_DYLIB_FLAT_LOOKUP = -2,
};

struct dyld_chained_import
{
    uint32_t    lib_ordinal :  8,
                weak_import :  1,
                name_offset : 23;
};

struct dyld_chained_ptr_64_rebase
{
    uint64_t    target    : 36,    // vmaddr, 64GB max image size
                high8     :  8,    // top 8 bits set to this after slide added
                reserved  :  7,    // all zeros
                next      : 12,    // 4-byte stride
                bind      :  1;    // == 0
};

struct dyld_chained_ptr_64_bind
{
    uint64_t    ordinal   : 24,
                addend    :  8,   // 0 thru 255
                reserved  : 19,   // all zeros
                next      : 12,   // 4-byte stride
                bind      :  1;   // == 1
};

#define S_REGULAR                       0x0
#define S_ZEROFILL                      0x1
#define S_NON_LAZY_SYMBOL_POINTERS      0x6
#define S_LAZY_SYMBOL_POINTERS          0x7
#define S_SYMBOL_STUBS                  0x8
#define S_MOD_INIT_FUNC_POINTERS        0x9
#define S_MOD_TERM_FUNC_POINTERS        0xa

#define S_ATTR_PURE_INSTRUCTIONS        0x80000000
#define S_ATTR_SOME_INSTRUCTIONS        0x00000400
#define S_ATTR_DEBUG             	0x02000000


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

struct rpath_command {
    uint32_t     cmd;           /* LC_RPATH */
    uint32_t     cmdsize;       /* includes string */
    lc_str       path;          /* path to add to run path */
};

struct dylinker_command {
    uint32_t        cmd;            /* LC_ID_DYLINKER, LC_LOAD_DYLINKER or
                                       LC_DYLD_ENVIRONMENT */
    uint32_t        cmdsize;        /* includes pathname string */
    lc_str          name;           /* dynamic linker's path name */
};

struct linkedit_data_command {
    uint32_t    cmd;            /* LC_CODE_SIGNATURE, LC_SEGMENT_SPLIT_INFO,
                                   LC_FUNCTION_STARTS, LC_DATA_IN_CODE,
                                   LC_DYLIB_CODE_SIGN_DRS,
                                   LC_LINKER_OPTIMIZATION_HINT,
                                   LC_DYLD_EXPORTS_TRIE, or
                                   LC_DYLD_CHAINED_FIXUPS. */
    uint32_t    cmdsize;        /* sizeof(struct linkedit_data_command) */
    uint32_t    dataoff;        /* file offset of data in __LINKEDIT segment */
    uint32_t    datasize;       /* file size of data in __LINKEDIT segment  */
};

#define PLATFORM_MACOS 1

struct build_version_command {
    uint32_t    cmd;            /* LC_BUILD_VERSION */
    uint32_t    cmdsize;        /* sizeof(struct build_version_command) plus */
                                /* ntools * sizeof(struct build_tool_version) */
    uint32_t    platform;       /* platform */
    uint32_t    minos;          /* X.Y.Z is encoded in nibbles xxxx.yy.zz */
    uint32_t    sdk;            /* X.Y.Z is encoded in nibbles xxxx.yy.zz */
    uint32_t    ntools;         /* number of tool entries following this */
};

struct source_version_command {
    uint32_t  cmd;      /* LC_SOURCE_VERSION */
    uint32_t  cmdsize;  /* 16 */
    uint64_t  version;  /* A.B.C.D.E packed as a24.b10.c10.d10.e10 */
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

    uint32_t extrefsymoff;  /* offset to referenced symbol table */
    uint32_t nextrefsyms;   /* number of referenced symbol table entries */

    uint32_t indirectsymoff;/* file offset to the indirect symbol table */
    uint32_t nindirectsyms; /* number of indirect symbol table entries */

    uint32_t extreloff; /* offset to external relocation entries */
    uint32_t nextrel;   /* number of external relocation entries */
    uint32_t locreloff; /* offset to local relocation entries */
    uint32_t nlocrel;   /* number of local relocation entries */
};

#define BIND_OPCODE_DONE                                        0x00
#define BIND_OPCODE_SET_DYLIB_SPECIAL_IMM                       0x30
#define BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM               0x40
#define BIND_OPCODE_SET_TYPE_IMM                                0x50
#define BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB                 0x70
#define BIND_OPCODE_DO_BIND                                     0x90

#define BIND_SYMBOL_FLAGS_WEAK_IMPORT                           0x1

#define BIND_TYPE_POINTER                                       1
#define BIND_SPECIAL_DYLIB_FLAT_LOOKUP                          -2

#define REBASE_OPCODE_DONE                                      0x00
#define REBASE_OPCODE_SET_TYPE_IMM                              0x10
#define REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB               0x20
#define REBASE_OPCODE_DO_REBASE_IMM_TIMES                       0x50

#define REBASE_TYPE_POINTER                                     1

#define EXPORT_SYMBOL_FLAGS_KIND_REGULAR                        0x00
#define EXPORT_SYMBOL_FLAGS_KIND_ABSOLUTE                       0x02
#define EXPORT_SYMBOL_FLAGS_WEAK_DEFINITION                     0x04

struct dyld_info_command {
    uint32_t cmd;             /* LC_DYLD_INFO or LC_DYLD_INFO_ONLY */
    uint32_t cmdsize;         /* sizeof(struct dyld_info_command) */
    uint32_t rebase_off;      /* file offset to rebase info  */
    uint32_t rebase_size;     /* size of rebase info   */
    uint32_t bind_off;        /* file offset to binding info   */
    uint32_t bind_size;       /* size of binding info  */
    uint32_t weak_bind_off;   /* file offset to weak binding info   */
    uint32_t weak_bind_size;  /* size of weak binding info  */
    uint32_t lazy_bind_off;   /* file offset to lazy binding info */
    uint32_t lazy_bind_size;  /* size of lazy binding infs */
    uint32_t export_off;      /* file offset to lazy binding info */
    uint32_t export_size;     /* size of lazy binding infs */
};

#define INDIRECT_SYMBOL_LOCAL   0x80000000

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
    sk_stub_helper,
    sk_ro_data,
    sk_uw_info,
    sk_nl_ptr,  // non-lazy pointers, aka GOT
    sk_debug_info,
    sk_debug_abbrev,
    sk_debug_line,
    sk_debug_aranges,
    sk_debug_str,
    sk_debug_line_str,
    sk_stab,
    sk_stab_str,
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

#define N_UNDF  0x0
#define N_ABS   0x2
#define N_EXT   0x1
#define N_SECT  0xe

#define N_WEAK_REF      0x0040
#define N_WEAK_DEF      0x0080

struct macho {
    struct mach_header_64 mh;
    int *seg2lc, nseg;
    struct load_command **lc;
    struct entry_point_command *ep;
    int nlc;
    struct {
        Section *s;
        int machosect;
    } sk_to_sect[sk_last];
    int *elfsectomacho;
    int *e2msym;
    Section *symtab, *strtab, *indirsyms, *stubs, *exports;
    uint32_t ilocal, iextdef, iundef;
    int stubsym, n_got, nr_plt;
    int segment[sk_last];
#ifdef CONFIG_NEW_MACHO
    Section *chained_fixups;
    int n_bind;
    int n_bind_rebase;
    struct bind_rebase {
	int section;
	int bind;
	ElfW_Rel rel;
    } *bind_rebase;
#else
    Section *rebase, *binding, *weak_binding, *lazy_binding;
    Section *stub_helper, *la_symbol_ptr;
    struct dyld_info_command *dyldinfo;
    int helpsym, lasym, dyld_private, dyld_stub_binder;
    int n_lazy_bind;    
    struct s_lazy_bind {
        int section;
        int bind_offset;
        int la_symbol_offset;
        ElfW_Rel rel;
    } *s_lazy_bind;
    int n_rebase;    
    struct s_rebase {
        int section;
        ElfW_Rel rel;
    } *s_rebase;
    int n_bind; 
    struct bind {
        int section;
        ElfW_Rel rel;
    } *bind;
#endif
};

#define SHT_LINKEDIT (SHT_LOOS + 42)
#define SHN_FROMDLL  (SHN_LOOS + 2)  /* Symbol is undefined, comes from a DLL */

static void * add_lc(struct macho *mo, uint32_t cmd, uint32_t cmdsize)
{
    struct load_command *lc = tcc_mallocz(cmdsize);
    lc->cmd = cmd;
    lc->cmdsize = cmdsize;
    mo->lc = tcc_realloc(mo->lc, sizeof(mo->lc[0]) * (mo->nlc + 1));
    mo->lc[mo->nlc++] = lc;
    return lc;
}

static struct segment_command_64 * add_segment(struct macho *mo, const char *name)
{
    struct segment_command_64 *sc = add_lc(mo, LC_SEGMENT_64, sizeof(*sc));
    strncpy(sc->segname, name, 16);
    mo->seg2lc = tcc_realloc(mo->seg2lc, sizeof(*mo->seg2lc) * (mo->nseg + 1));
    mo->seg2lc[mo->nseg++] = mo->nlc - 1;
    return sc;
}

static struct segment_command_64 * get_segment(struct macho *mo, int i)
{
    return (struct segment_command_64 *) (mo->lc[mo->seg2lc[i]]);
}

static int add_section(struct macho *mo, struct segment_command_64 **_seg, const char *name)
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
    return (struct section_64*)((char*)seg + sizeof(*seg)) + i;
}

static void * add_dylib(struct macho *mo, char *name)
{
    struct dylib_command *lc;
    int sz = (sizeof(*lc) + strlen(name) + 1 + 7) & -8;
    lc = add_lc(mo, LC_LOAD_DYLIB, sz);
    lc->name = sizeof(*lc);
    strcpy((char*)lc + lc->name, name);
    lc->timestamp = 2;
    lc->current_version = 1 << 16;
    lc->compatibility_version = 1 << 16;
    return lc;
}

static int uleb128_size (unsigned long long value)
{
    int size =  0;

    do {
        value >>= 7;
        size++;
    } while (value != 0);
    return size;
}

static void write_uleb128(Section *section, uint64_t value)
{
    do {
        unsigned char byte = value & 0x7f;
	uint8_t *ptr = section_ptr_add(section, 1);

        value >>= 7;
        *ptr = byte | (value ? 0x80 : 0);
    } while (value != 0);
}

static void tcc_macho_add_destructor(TCCState *s1)
{
    int init_sym, mh_execute_header, at_exit_sym;
    Section *s;
    ElfW_Rel *rel;
    uint8_t *ptr;

    mh_execute_header = put_elf_sym(s1->symtab, -4096, 0,
				    ELFW(ST_INFO)(STB_GLOBAL, STT_OBJECT), 0,
				    text_section->sh_num, "__mh_execute_header");
    s = find_section(s1, ".fini_array");
    if (s->data_offset == 0)
        return; 
    init_sym = put_elf_sym(s1->symtab, text_section->data_offset, 0,
                           ELFW(ST_INFO)(STB_LOCAL, STT_FUNC), 0,
                           text_section->sh_num, "___GLOBAL_init_65535");
    at_exit_sym = put_elf_sym(s1->symtab, 0, 0,
                              ELFW(ST_INFO)(STB_GLOBAL, STT_FUNC), 0,
                              SHN_UNDEF, "___cxa_atexit");
#ifdef TCC_TARGET_X86_64
    ptr = section_ptr_add(text_section, 4);
    ptr[0] = 0x55;  // pushq   %rbp
    ptr[1] = 0x48;  // movq    %rsp, %rbp
    ptr[2] = 0x89;
    ptr[3] = 0xe5;
    for_each_elem(s->reloc, 0, rel, ElfW_Rel) {
        int sym_index = ELFW(R_SYM)(rel->r_info);

        ptr = section_ptr_add(text_section, 26);
        ptr[0] = 0x48;  // lea destructor(%rip),%rax
        ptr[1] = 0x8d;
        ptr[2] = 0x05;
        put_elf_reloca(s1->symtab, text_section, 
		       text_section->data_offset - 23,
		       R_X86_64_PC32, sym_index, -4);
        ptr[7] = 0x48;  // mov %rax,%rdi
        ptr[8] = 0x89;
        ptr[9] = 0xc7;
	ptr[10] = 0x31; // xorl %ecx, %ecx
	ptr[11] = 0xc9;
	ptr[12] = 0x89; // movl %ecx, %esi
	ptr[13] = 0xce;
        ptr[14] = 0x48;  // lea mh_execute_header(%rip),%rdx
        ptr[15] = 0x8d;
        ptr[16] = 0x15;
        put_elf_reloca(s1->symtab, text_section,
		       text_section->data_offset - 9,
		       R_X86_64_PC32, mh_execute_header, -4);
	ptr[21] = 0xe8; // call __cxa_atexit
        put_elf_reloca(s1->symtab, text_section,
		       text_section->data_offset - 4,
		       R_X86_64_PLT32, at_exit_sym, -4);
    }
    ptr = section_ptr_add(text_section, 2);
    ptr[0] = 0x5d;  // pop   %rbp
    ptr[1] = 0xc3;  // ret
#elif defined TCC_TARGET_ARM64
    ptr = section_ptr_add(text_section, 8);
    write32le(ptr, 0xa9bf7bfd);     // stp     x29, x30, [sp, #-16]!
    write32le(ptr + 4, 0x910003fd); // mov     x29, sp
    for_each_elem(s->reloc, 0, rel, ElfW_Rel) {
        int sym_index = ELFW(R_SYM)(rel->r_info);

        ptr = section_ptr_add(text_section, 24);
        put_elf_reloc(s1->symtab, text_section, 
		      text_section->data_offset - 24,
		      R_AARCH64_ADR_PREL_PG_HI21, sym_index);
        write32le(ptr, 0x90000000);      // adrp x0, destructor@page
        put_elf_reloc(s1->symtab, text_section,
		      text_section->data_offset - 20,
		      R_AARCH64_LDST8_ABS_LO12_NC, sym_index);
        write32le(ptr + 4, 0x91000000);  // add x0,x0,destructor@pageoff
        write32le(ptr + 8, 0xd2800001);  // mov x1, #0
        put_elf_reloc(s1->symtab, text_section, 
		      text_section->data_offset - 12,
		      R_AARCH64_ADR_PREL_PG_HI21, mh_execute_header);
        write32le(ptr + 12, 0x90000002);      // adrp x2, mh_execute_header@page
        put_elf_reloc(s1->symtab, text_section,
		      text_section->data_offset - 8,
		      R_AARCH64_LDST8_ABS_LO12_NC, mh_execute_header);
        write32le(ptr + 16, 0x91000042);  // add x2,x2,mh_execute_header@pageoff
        put_elf_reloc(s1->symtab, text_section,
		      text_section->data_offset - 4,
		      R_AARCH64_CALL26, at_exit_sym);
	write32le(ptr + 20, 0x94000000); // bl __cxa_atexit
    }
    ptr = section_ptr_add(text_section, 8);
    write32le(ptr, 0xa8c17bfd);     // ldp     x29, x30, [sp], #16
    write32le(ptr + 4, 0xd65f03c0); // ret
#endif
    s->reloc->data_offset = s->data_offset = 0;
    s->sh_flags &= ~SHF_ALLOC;
    add_array (s1, ".init_array", init_sym);
}

#ifdef CONFIG_NEW_MACHO
static void bind_rebase_add(struct macho *mo, int bind, int sh_info,
			    ElfW_Rel *rel, struct sym_attr *attr)
{
    mo->bind_rebase = tcc_realloc(mo->bind_rebase, (mo->n_bind_rebase + 1) *
		                  sizeof(struct bind_rebase));
    mo->bind_rebase[mo->n_bind_rebase].section = sh_info;
    mo->bind_rebase[mo->n_bind_rebase].bind = bind;
    mo->bind_rebase[mo->n_bind_rebase].rel = *rel;
    if (attr)
        mo->bind_rebase[mo->n_bind_rebase].rel.r_offset = attr->got_offset;
    mo->n_bind_rebase++;
    mo->n_bind += bind;
}

static void check_relocs(TCCState *s1, struct macho *mo)
{
    Section *s;
    ElfW_Rel *rel, save_rel;
    ElfW(Sym) *sym;
    int i, j, type, gotplt_entry, sym_index, for_code;
    uint32_t *pi, *goti;
    struct sym_attr *attr;

    goti = NULL;
    mo->nr_plt = mo->n_got = 0;
    for (i = 1; i < s1->nb_sections; i++) {
        s = s1->sections[i];
        if (s->sh_type != SHT_RELX ||
	    !strncmp(s1->sections[s->sh_info]->name, ".debug_", 7))
            continue;
        for_each_elem(s, 0, rel, ElfW_Rel) {
	    save_rel = *rel;
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
                    attr->got_offset = s1->got->data_offset;
                    attr->plt_offset = -1;
                    attr->dyn_index = 1; /* used as flag */
                    section_ptr_add(s1->got, PTR_SIZE);
                    put_elf_reloc(s1->symtab, s1->got, attr->got_offset,
                                  R_JMP_SLOT, sym_index);
	 	    goti = tcc_realloc(goti, (mo->n_got + 1) * sizeof(*goti));
                    if (ELFW(ST_BIND)(sym->st_info) == STB_LOCAL) {
                        if (sym->st_shndx == SHN_UNDEF)
                          tcc_error("undefined local symbo: '%s'",
				    (char *) symtab_section->link->data + sym->st_name);
			goti[mo->n_got++] = INDIRECT_SYMBOL_LOCAL;
                    } else {
                        goti[mo->n_got++] = mo->e2msym[sym_index];
                        if (sym->st_shndx == SHN_UNDEF
#ifdef TCC_TARGET_X86_64
                            && type == R_X86_64_GOTPCREL
#elif defined TCC_TARGET_ARM64
                            && type == R_AARCH64_ADR_GOT_PAGE
#endif
                            ) {
			    attr->plt_offset = -mo->n_bind_rebase - 2;
			    bind_rebase_add(mo, 1, s1->got->reloc->sh_info, &save_rel, attr);
			    s1->got->reloc->data_offset -= sizeof (ElfW_Rel);
			}
		        if (for_code && sym->st_shndx == SHN_UNDEF)
			    s1->got->reloc->data_offset -= sizeof (ElfW_Rel);
		    }
                }
                if (for_code && sym->st_shndx == SHN_UNDEF) {
		    if ((int)attr->plt_offset < -1) {
			/* remove above bind and replace with plt */
			mo->bind_rebase[-attr->plt_offset - 2].bind = 2;
			attr->plt_offset = -1;
		    }
                    if (attr->plt_offset == -1) {
                        uint8_t *jmp;

                        attr->plt_offset = mo->stubs->data_offset;
#ifdef TCC_TARGET_X86_64
                        if (type != R_X86_64_PLT32)
                             continue;
                        jmp = section_ptr_add(mo->stubs, 6);
                        jmp[0] = 0xff;  /* jmpq *ofs(%rip) */
                        jmp[1] = 0x25;
                        put_elf_reloc(s1->symtab, mo->stubs,
                                      attr->plt_offset + 2,
                                      R_X86_64_GOTPCREL, sym_index);
#elif defined TCC_TARGET_ARM64
                        if (type != R_AARCH64_CALL26)
                             continue;
                        jmp = section_ptr_add(mo->stubs, 12);
                        put_elf_reloc(s1->symtab, mo->stubs,
                                      attr->plt_offset,
                                      R_AARCH64_ADR_GOT_PAGE, sym_index);
                        write32le(jmp, // adrp x16, #sym
                                  0x90000010);
                        put_elf_reloc(s1->symtab, mo->stubs,
                                      attr->plt_offset + 4,
                                      R_AARCH64_LD64_GOT_LO12_NC, sym_index);
                        write32le(jmp + 4, // ld x16,[x16, #sym]
                                  0xf9400210);
                        write32le(jmp + 8, // br x16
                                  0xd61f0200);
#endif
			bind_rebase_add(mo, 1, s1->got->reloc->sh_info, &save_rel, attr);
                        pi = section_ptr_add(mo->indirsyms, sizeof(*pi));
                        *pi = mo->e2msym[sym_index];
                        mo->nr_plt++;
                    }
                    rel->r_info = ELFW(R_INFO)(mo->stubsym, type);
                    rel->r_addend += attr->plt_offset;
                }
            }
	    if (type == R_DATA_PTR || type == R_JMP_SLOT)
		bind_rebase_add(mo, sym->st_shndx == SHN_UNDEF ? 1 : 0,
				s->sh_info, &save_rel, NULL);
        }
    }
    /* remove deleted binds */
    for (i = 0, j = 0; i < mo->n_bind_rebase; i++)
	if (mo->bind_rebase[i].bind == 2)
	    mo->n_bind--;
	else
	    mo->bind_rebase[j++] = mo->bind_rebase[i];
    mo->n_bind_rebase = j;
    pi = section_ptr_add(mo->indirsyms, mo->n_got * sizeof(*pi));
    memcpy(pi, goti, mo->n_got * sizeof(*pi));
    tcc_free(goti);
}

#else

static void check_relocs(TCCState *s1, struct macho *mo)
{
    uint8_t *jmp;
    Section *s;
    ElfW_Rel *rel, save_rel;
    ElfW(Sym) *sym;
    int i, type, gotplt_entry, sym_index, for_code;
    int bind_offset, la_symbol_offset;
    uint32_t *pi, *goti;
    struct sym_attr *attr;

#ifdef TCC_TARGET_X86_64
    jmp = section_ptr_add(mo->stub_helper, 16);
    jmp[0] = 0x4c;  /* leaq _dyld_private(%rip), %r11 */
    jmp[1] = 0x8d;
    jmp[2] = 0x1d;
    put_elf_reloca(s1->symtab, mo->stub_helper, 3,
		   R_X86_64_PC32, mo->dyld_private, -4);
    jmp[7] = 0x41;  /* pushq %r11 */
    jmp[8] = 0x53;
    jmp[9] = 0xff;  /* jmpq    *dyld_stub_binder@GOT(%rip) */
    jmp[10] = 0x25;
    put_elf_reloca(s1->symtab, mo->stub_helper, 11,
		   R_X86_64_GOTPCREL, mo->dyld_stub_binder, -4);
    jmp[15] = 0x90; /* nop */
#elif defined TCC_TARGET_ARM64
    jmp = section_ptr_add(mo->stub_helper, 24);
    put_elf_reloc(s1->symtab, mo->stub_helper, 0,
		  R_AARCH64_ADR_PREL_PG_HI21, mo->dyld_private);
    write32le(jmp, 0x90000011); // adrp x17, _dyld_private@page
    put_elf_reloc(s1->symtab, mo->stub_helper, 4,
		  R_AARCH64_LDST64_ABS_LO12_NC, mo->dyld_private);
    write32le(jmp + 4, 0x91000231); // add x17,x17,_dyld_private@pageoff
    write32le(jmp + 8, 0xa9bf47f0); // stp x16/x17, [sp, #-16]!
    put_elf_reloc(s1->symtab, mo->stub_helper, 12,
		  R_AARCH64_ADR_GOT_PAGE, mo->dyld_stub_binder);
    write32le(jmp + 12, 0x90000010); // adrp x16, dyld_stub_binder@page
    put_elf_reloc(s1->symtab, mo->stub_helper, 16,
		  R_AARCH64_LD64_GOT_LO12_NC, mo->dyld_stub_binder);
    write32le(jmp + 16, 0xf9400210); // ldr x16,[x16,dyld_stub_binder@pageoff]
    write32le(jmp + 20, 0xd61f0200); // br x16
#endif
    
    goti = NULL;
    mo->nr_plt = mo->n_got = 0;
    for (i = 1; i < s1->nb_sections; i++) {
        s = s1->sections[i];
        if (s->sh_type != SHT_RELX ||
	    !strncmp(s1->sections[s->sh_info]->name, ".debug_", 7))
            continue;
        for_each_elem(s, 0, rel, ElfW_Rel) {
	    save_rel = *rel;
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
                    attr->got_offset = s1->got->data_offset;
                    attr->plt_offset = -1;
                    attr->dyn_index = 1; /* used as flag */
		    section_ptr_add(s1->got, PTR_SIZE);
                    put_elf_reloc(s1->symtab, s1->got, attr->got_offset,
                                  R_JMP_SLOT, sym_index);
		    goti = tcc_realloc(goti, (mo->n_got + 1) * sizeof(*goti));
                    if (ELFW(ST_BIND)(sym->st_info) == STB_LOCAL) {
                        if (sym->st_shndx == SHN_UNDEF)
                          tcc_error("undefined local symbo: '%s'",
				    (char *) symtab_section->link->data + sym->st_name);
			goti[mo->n_got++] = INDIRECT_SYMBOL_LOCAL;
                    } else {
			goti[mo->n_got++] = mo->e2msym[sym_index];
			if (sym->st_shndx == SHN_UNDEF
#ifdef TCC_TARGET_X86_64
			    && type == R_X86_64_GOTPCREL
#elif defined TCC_TARGET_ARM64
			    && type == R_AARCH64_ADR_GOT_PAGE
#endif
			    ) {
			    mo->bind =
			        tcc_realloc(mo->bind,
					    (mo->n_bind + 1) *
					    sizeof(struct bind));
			    mo->bind[mo->n_bind].section = s1->got->reloc->sh_info;
			    mo->bind[mo->n_bind].rel = save_rel;
                            mo->bind[mo->n_bind].rel.r_offset = attr->got_offset;
			    mo->n_bind++;
			    s1->got->reloc->data_offset -= sizeof (ElfW_Rel);
			}
		    }
                }
                if (for_code && sym->st_shndx == SHN_UNDEF) {
                    if (attr->plt_offset == -1) {
                        attr->plt_offset = mo->stubs->data_offset;
#ifdef TCC_TARGET_X86_64
			if (type != R_X86_64_PLT32)
			     continue;
			/* __stubs */
                        jmp = section_ptr_add(mo->stubs, 6);
                        jmp[0] = 0xff;  /* jmpq *__la_symbol_ptr(%rip) */
                        jmp[1] = 0x25;
                        put_elf_reloca(s1->symtab, mo->stubs,
                                       mo->stubs->data_offset - 4,
                                       R_X86_64_PC32, mo->lasym,
				       mo->la_symbol_ptr->data_offset - 4);

			/* __stub_helper */
			bind_offset = mo->stub_helper->data_offset + 1;
                        jmp = section_ptr_add(mo->stub_helper, 10);
                        jmp[0] = 0x68;  /* pushq $bind_offset */
                        jmp[5] = 0xe9;  /* jmpq __stub_helper */
                        write32le(jmp + 6, -mo->stub_helper->data_offset);

			/* __la_symbol_ptr */
			la_symbol_offset = mo->la_symbol_ptr->data_offset;
                        put_elf_reloca(s1->symtab, mo->la_symbol_ptr,
				       mo->la_symbol_ptr->data_offset,
				       R_DATA_PTR, mo->helpsym,
				       mo->stub_helper->data_offset - 10);
			section_ptr_add(mo->la_symbol_ptr, PTR_SIZE);
#elif defined TCC_TARGET_ARM64
			if (type != R_AARCH64_CALL26)
			     continue;
			/* __stubs */
                        jmp = section_ptr_add(mo->stubs, 12);
                        put_elf_reloca(s1->symtab, mo->stubs,
                                       mo->stubs->data_offset - 12,
                                       R_AARCH64_ADR_PREL_PG_HI21, mo->lasym,
				       mo->la_symbol_ptr->data_offset);
                        write32le(jmp, // adrp x16, __la_symbol_ptr@page
                                  0x90000010);
                        put_elf_reloca(s1->symtab, mo->stubs,
                                       mo->stubs->data_offset - 8,
                                       R_AARCH64_LDST64_ABS_LO12_NC, mo->lasym,
				       mo->la_symbol_ptr->data_offset);
                        write32le(jmp + 4, // ldr x16,[x16, __la_symbol_ptr@pageoff]
                                  0xf9400210);
                        write32le(jmp + 8, // br x16
                                  0xd61f0200);

			/* __stub_helper */
			bind_offset = mo->stub_helper->data_offset + 8;
                        jmp = section_ptr_add(mo->stub_helper, 12);
                        write32le(jmp + 0, // ldr  w16, l0
                                  0x18000050);
                        write32le(jmp + 4, // b stubHelperHeader
                                  0x14000000 +
				  ((-(mo->stub_helper->data_offset - 8) / 4) &
				   0x3ffffff));
                        write32le(jmp + 8, 0); // l0: .long bind_offset

			/* __la_symbol_ptr */
			la_symbol_offset = mo->la_symbol_ptr->data_offset;
                        put_elf_reloca(s1->symtab, mo->la_symbol_ptr,
				       mo->la_symbol_ptr->data_offset,
				       R_DATA_PTR, mo->helpsym,
				       mo->stub_helper->data_offset - 12);
			section_ptr_add(mo->la_symbol_ptr, PTR_SIZE);
#endif
                        mo->s_lazy_bind =
                            tcc_realloc(mo->s_lazy_bind, (mo->n_lazy_bind + 1) *
                                        sizeof(struct s_lazy_bind));
                        mo->s_lazy_bind[mo->n_lazy_bind].section =
                            mo->stub_helper->reloc->sh_info;
                        mo->s_lazy_bind[mo->n_lazy_bind].bind_offset =
			    bind_offset;
                        mo->s_lazy_bind[mo->n_lazy_bind].la_symbol_offset =
			    la_symbol_offset;
                        mo->s_lazy_bind[mo->n_lazy_bind].rel = save_rel;
                        mo->s_lazy_bind[mo->n_lazy_bind].rel.r_offset =
                            attr->plt_offset;
                        mo->n_lazy_bind++;
			pi = section_ptr_add(mo->indirsyms, sizeof(*pi));
			*pi = mo->e2msym[sym_index];
			mo->nr_plt++;
                    }
                    rel->r_info = ELFW(R_INFO)(mo->stubsym, type);
                    rel->r_addend += attr->plt_offset;
                }
            }
            if (type == R_DATA_PTR || type == R_JMP_SLOT) {
		if (sym->st_shndx == SHN_UNDEF) {
		    mo->bind = tcc_realloc(mo->bind,
					   (mo->n_bind + 1) *
					   sizeof(struct bind));
		    mo->bind[mo->n_bind].section = s->sh_info;
		    mo->bind[mo->n_bind].rel = save_rel;
		    mo->n_bind++;
		}
		else {
                    mo->s_rebase =
                        tcc_realloc(mo->s_rebase, (mo->n_rebase + 1) *
				    sizeof(struct s_rebase));
                    mo->s_rebase[mo->n_rebase].section = s->sh_info;
                    mo->s_rebase[mo->n_rebase].rel = save_rel;
                    mo->n_rebase++;
		}
	    }
        }
    }
    pi = section_ptr_add(mo->indirsyms, mo->n_got * sizeof(*pi));
    memcpy(pi, goti, mo->n_got * sizeof(*pi));
    pi = section_ptr_add(mo->indirsyms, mo->nr_plt * sizeof(*pi));
    memcpy(pi, mo->indirsyms->data, mo->nr_plt * sizeof(*pi));
    tcc_free(goti);
}
#endif

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

        dprintf("%4d (%4d): %09lx %4d %4d %4d %3d %s\n",
                sym_index, elf_index, (long)sym->st_value,
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
            if (ELFW(ST_BIND)(sym->st_info) == STB_WEAK
		|| s1->output_type != TCC_OUTPUT_EXE
                || find_elf_sym(s1->dynsymtab_section, name)) {
                /* Mark the symbol as coming from a dylib so that
                   relocate_syms doesn't complain.  Normally bind_exe_dynsyms
                   would do this check, and place the symbol into dynsym
                   which is checked by relocate_syms.  But Mach-O doesn't use
                   bind_exe_dynsyms.  */
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
        n.n_type = N_SECT;
        break;
    case STT_FILE:
        n.n_type = N_ABS;
        break;
    default:
        tcc_error("unhandled ELF symbol type %d %s",
                  ELFW(ST_TYPE)(sym->st_info), name);
    }
    if (sym->st_shndx == SHN_UNDEF)
      tcc_error("should have been rewritten to SHN_FROMDLL: %s", name);
    else if (sym->st_shndx == SHN_FROMDLL)
      n.n_type = N_UNDF, n.n_sect = 0;
    else if (sym->st_shndx == SHN_ABS)
      n.n_type = N_ABS, n.n_sect = 0;
    else if (sym->st_shndx >= SHN_LORESERVE)
      tcc_error("unhandled ELF symbol section %d %s", sym->st_shndx, name);
    else if (!mo->elfsectomacho[sym->st_shndx]) {
      if (strncmp(s1->sections[sym->st_shndx]->name, ".debug_", 7))
        tcc_error("ELF section %d(%s) not mapped into Mach-O for symbol %s",
                  sym->st_shndx, s1->sections[sym->st_shndx]->name, name);
    }
    else
      n.n_sect = mo->elfsectomacho[sym->st_shndx];
    if (ELFW(ST_BIND)(sym->st_info) == STB_GLOBAL)
      n.n_type |=  N_EXT;
    else if (ELFW(ST_BIND)(sym->st_info) == STB_WEAK)
      n.n_desc |= N_WEAK_REF | (n.n_type != N_UNDF ? N_WEAK_DEF : 0);
    n.n_strx = pn->n_strx;
    n.n_value = sym->st_value;
    *pn = n;
}

static void convert_symbols(TCCState *s1, struct macho *mo)
{
    struct nlist_64 *pn;
    for_each_elem(mo->symtab, 0, pn, struct nlist_64)
        convert_symbol(s1, mo, pn);
}

static int machosymcmp(const void *_a, const void *_b, void *arg)
{
    TCCState *s1 = arg;
    int ea = ((struct nlist_64 *)_a)->n_value;
    int eb = ((struct nlist_64 *)_b)->n_value;
    ElfSym *sa = (ElfSym *)symtab_section->data + ea;
    ElfSym *sb = (ElfSym *)symtab_section->data + eb;
    int r;
    /* locals, then defined externals, then undefined externals, the
       last two sections also by name, otherwise stable sort */
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

/* cannot use qsort because code has to be reentrant */
static void tcc_qsort (void  *base, size_t nel, size_t width,
                       int (*comp)(const void *, const void *, void *), void *arg)
{
    size_t wnel, gap, wgap, i, j, k;
    char *a, *b, tmp;

    wnel = width * nel;
    for (gap = 0; ++gap < nel;)
        gap *= 3;
    while ( gap /= 3 ) {
        wgap = width * gap;
        for (i = wgap; i < wnel; i += width) {
            for (j = i - wgap; ;j -= wgap) {
                a = j + (char *)base;
                b = a + wgap;
                if ( (*comp)(a, b, arg) <= 0 )
                    break;
                k = width;
                do {
                    tmp = *a;
                    *a++ = *b;
                    *b++ = tmp;
                } while ( --k );
                if (j < wgap)
                    break;
            }
        }
    }
}

static void create_symtab(TCCState *s1, struct macho *mo)
{
    int sym_index, sym_end;
    struct nlist_64 *pn;

    /* Stub creation belongs to check_relocs, but we need to create
       the symbol now, so its included in the sorting.  */
    mo->stubs = new_section(s1, "__stubs", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
    s1->got = new_section(s1, ".got", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
    mo->stubsym = put_elf_sym(s1->symtab, 0, 0,
                              ELFW(ST_INFO)(STB_LOCAL, STT_SECTION), 0,
                              mo->stubs->sh_num, ".__stubs");
#ifdef CONFIG_NEW_MACHO
    mo->chained_fixups = new_section(s1, "CHAINED_FIXUPS",
				     SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);
#else
    mo->stub_helper = new_section(s1, "__stub_helper", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
    mo->la_symbol_ptr = new_section(s1, "__la_symbol_ptr", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
    mo->helpsym = put_elf_sym(s1->symtab, 0, 0,
                              ELFW(ST_INFO)(STB_LOCAL, STT_SECTION), 0,
                              mo->stub_helper->sh_num, ".__stub_helper");
    mo->lasym = put_elf_sym(s1->symtab, 0, 0,
                            ELFW(ST_INFO)(STB_LOCAL, STT_SECTION), 0,
                            mo->la_symbol_ptr->sh_num, ".__la_symbol_ptr");
    section_ptr_add(data_section, -data_section->data_offset & (PTR_SIZE - 1));
    mo->dyld_private = put_elf_sym(s1->symtab, data_section->data_offset, PTR_SIZE,
                                   ELFW(ST_INFO)(STB_LOCAL, STT_OBJECT), 0,
                                   data_section->sh_num, ".__dyld_private");
    section_ptr_add(data_section, PTR_SIZE);
    mo->dyld_stub_binder = put_elf_sym(s1->symtab, 0, 0,
                		       ELFW(ST_INFO)(STB_GLOBAL, STT_OBJECT), 0,
				       SHN_UNDEF, "dyld_stub_binder");
    mo->rebase = new_section(s1, "REBASE", SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);
    mo->binding = new_section(s1, "BINDING", SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);
    mo->weak_binding = new_section(s1, "WEAK_BINDING", SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);
    mo->lazy_binding = new_section(s1, "LAZY_BINDING", SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);
#endif
    mo->exports = new_section(s1, "EXPORT", SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);
    mo->indirsyms = new_section(s1, "LEINDIR", SHT_LINKEDIT, SHF_ALLOC | SHF_WRITE);

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
    section_ptr_add(mo->strtab, -mo->strtab->data_offset & (PTR_SIZE - 1));
    tcc_qsort(pn, sym_end - 1, sizeof(*pn), machosymcmp, s1);
    mo->e2msym = tcc_malloc(sym_end * sizeof(*mo->e2msym));
    mo->e2msym[0] = -1;
    for (sym_index = 1; sym_index < sym_end; ++sym_index) {
        mo->e2msym[pn[sym_index - 1].n_value] = sym_index - 1;
    }
}

const struct {
    int seg_initial;
    uint32_t flags;
    const char *name;
} skinfo[sk_last] = {
    /*[sk_unknown] =*/        { 0 },
    /*[sk_discard] =*/        { 0 },
    /*[sk_text] =*/           { 1, S_REGULAR | S_ATTR_PURE_INSTRUCTIONS
                                   | S_ATTR_SOME_INSTRUCTIONS, "__text" },
    /*[sk_stubs] =*/          { 1, S_REGULAR | S_ATTR_PURE_INSTRUCTIONS | S_SYMBOL_STUBS
                                   | S_ATTR_SOME_INSTRUCTIONS , "__stubs" },
    /*[sk_stub_helper] =*/    { 1, S_REGULAR | S_ATTR_PURE_INSTRUCTIONS
                                   | S_ATTR_SOME_INSTRUCTIONS , "__stub_helper" },
    /*[sk_ro_data] =*/        { 2, S_REGULAR, "__rodata" },
    /*[sk_uw_info] =*/        { 0 },
    /*[sk_nl_ptr] =*/         { 2, S_NON_LAZY_SYMBOL_POINTERS, "__got" },
    /*[sk_debug_info] =*/     { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_info" },
    /*[sk_debug_abbrev] =*/   { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_abbrev" },
    /*[sk_debug_line] =*/     { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_line" },
    /*[sk_debug_aranges] =*/  { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_aranges" },
    /*[sk_debug_str] =*/      { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_str" },
    /*[sk_debug_line_str] =*/ { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_line_str" },
    /*[sk_stab] =*/           { 4, S_REGULAR, "__stab" },
    /*[sk_stab_str] =*/       { 4, S_REGULAR, "__stab_str" },
    /*[sk_la_ptr] =*/         { 4, S_LAZY_SYMBOL_POINTERS, "__la_symbol_ptr" },
    /*[sk_init] =*/           { 4, S_MOD_INIT_FUNC_POINTERS, "__mod_init_func" },
    /*[sk_fini] =*/           { 4, S_MOD_TERM_FUNC_POINTERS, "__mod_term_func" },
    /*[sk_rw_data] =*/        { 4, S_REGULAR, "__data" },
    /*[sk_bss] =*/            { 4, S_ZEROFILL, "__bss" },
    /*[sk_linkedit] =*/       { 5, S_REGULAR, NULL },
};

#define	START	((uint64_t)1 << 32)

const struct {
    int used;
    const char *name;
    uint64_t vmaddr;
    uint64_t vmsize;
    vm_prot_t maxprot;
    vm_prot_t initprot;
    uint32_t flags;
} all_segment[] = {
    { 1, "__PAGEZERO",       0, START, 0, 0,            0 },
    { 0, "__TEXT",       START,     0, 5, 5,            0 },
    { 0, "__DATA_CONST",    -1,     0, 3, 3, SG_READ_ONLY },
    { 0, "__DWARF",         -1,     0, 7, 3,            0 },
    { 0, "__DATA",          -1,     0, 3, 3,            0 },
    { 1, "__LINKEDIT",      -1,     0, 1, 1,            0 },
};

#define	N_SEGMENT	(sizeof(all_segment)/sizeof(all_segment[0]))

#ifdef CONFIG_NEW_MACHO
static void calc_fixup_size(TCCState *s1, struct macho *mo)
{
    int i, size;

    size = (sizeof(struct dyld_chained_fixups_header) + 7) & -8;
    size += (sizeof(struct dyld_chained_starts_in_image) + (mo->nseg - 1) * sizeof(uint32_t) + 7) & -8;
    for (i = (s1->output_type == TCC_OUTPUT_EXE); i < mo->nseg - 1; i++) {
	int page_count = (get_segment(mo, i)->vmsize + SEG_PAGE_SIZE - 1) / SEG_PAGE_SIZE;
	size += (sizeof(struct dyld_chained_starts_in_segment) + (page_count - 1) * sizeof(uint16_t) + 7) & -8;
    }
    size += mo->n_bind * sizeof (struct dyld_chained_import) + 1;
    for (i = 0; i < mo->n_bind_rebase; i++) {
	if (mo->bind_rebase[i].bind) {
	    int sym_index = ELFW(R_SYM)(mo->bind_rebase[i].rel.r_info);
	    ElfW(Sym) *sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
	    const char *name = (char *) symtab_section->link->data + sym->st_name;
	    size += strlen(name) + 1;
	}
    }
    size = (size + 7) & -8;
    section_ptr_add(mo->chained_fixups, size);
}

#else

static void set_segment_and_offset(TCCState *s1, struct macho *mo, addr_t addr,
				   uint8_t *ptr, int opcode,
				   Section *sec, addr_t offset)
{
    int i;
    struct segment_command_64 *seg = NULL;

    for (i = (s1->output_type == TCC_OUTPUT_EXE); i < mo->nseg - 1; i++) {
	seg = get_segment(mo, i);
	if (addr >= seg->vmaddr && addr < (seg->vmaddr + seg->vmsize))
	    break;
    }
    *ptr = opcode | i;
    write_uleb128(sec, offset - seg->vmaddr);
}

static void bind_rebase(TCCState *s1, struct macho *mo)
{
    int i;
    uint8_t *ptr;
    ElfW(Sym) *sym;
    const char *name;

    for (i = 0; i < mo->n_lazy_bind; i++) {
	int sym_index = ELFW(R_SYM)(mo->s_lazy_bind[i].rel.r_info);

	sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
	name = (char *) symtab_section->link->data + sym->st_name;
	write32le(mo->stub_helper->data +
		  mo->s_lazy_bind[i].bind_offset,
		  mo->lazy_binding->data_offset);
	ptr = section_ptr_add(mo->lazy_binding, 1);
	set_segment_and_offset(s1, mo, mo->la_symbol_ptr->sh_addr, ptr,
			       BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB,
			       mo->lazy_binding,
			       mo->s_lazy_bind[i].la_symbol_offset +
			       mo->la_symbol_ptr->sh_addr);
	ptr = section_ptr_add(mo->lazy_binding, 5 + strlen(name));
	*ptr++ = BIND_OPCODE_SET_DYLIB_SPECIAL_IMM |
		 (BIND_SPECIAL_DYLIB_FLAT_LOOKUP & 0xf);
	*ptr++ = BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM | 0;
	strcpy((char *)ptr, name);
	ptr += strlen(name) + 1;
	*ptr++ = BIND_OPCODE_DO_BIND;
	*ptr = BIND_OPCODE_DONE;
    }
    for (i = 0; i < mo->n_rebase; i++) {
	Section *s = s1->sections[mo->s_rebase[i].section];

	ptr = section_ptr_add(mo->rebase, 2);
	*ptr++ = REBASE_OPCODE_SET_TYPE_IMM | REBASE_TYPE_POINTER;
	set_segment_and_offset(s1, mo, s->sh_addr, ptr,
			       REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB,
			       mo->rebase,
			       mo->s_rebase[i].rel.r_offset +
			       s->sh_addr);
	ptr = section_ptr_add(mo->rebase, 1);
	*ptr = REBASE_OPCODE_DO_REBASE_IMM_TIMES | 1;
    }
    for (i = 0; i < mo->n_bind; i++) {
	int sym_index = ELFW(R_SYM)(mo->bind[i].rel.r_info);
	Section *s = s1->sections[mo->bind[i].section];
	Section *binding;

	sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
	name = (char *) symtab_section->link->data + sym->st_name;
	binding = ELFW(ST_BIND)(sym->st_info) == STB_WEAK
	    ? mo->weak_binding : mo->binding;
        ptr = section_ptr_add(binding, 4 + (binding == mo->binding) +
			     	       strlen(name));
	if (binding == mo->binding)
            *ptr++ = BIND_OPCODE_SET_DYLIB_SPECIAL_IMM |
	             (BIND_SPECIAL_DYLIB_FLAT_LOOKUP & 0xf);
        *ptr++ = BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM |
		 (binding == mo->weak_binding
		  ? BIND_SYMBOL_FLAGS_WEAK_IMPORT : 0);
        strcpy((char *)ptr, name);
        ptr += strlen(name) + 1;
        *ptr++ = BIND_OPCODE_SET_TYPE_IMM | BIND_TYPE_POINTER;
	set_segment_and_offset(s1, mo, s->sh_addr, ptr,
			       BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB,
			       binding,
			       mo->bind[i].rel.r_offset + s->sh_addr);
        ptr = section_ptr_add(binding, 1);
        *ptr++ = BIND_OPCODE_DO_BIND;
    }
    if (mo->rebase->data_offset) {
        ptr = section_ptr_add(mo->rebase, 1);
        *ptr = REBASE_OPCODE_DONE;
    }
    if (mo->binding->data_offset) {
        ptr = section_ptr_add(mo->binding, 1);
        *ptr = BIND_OPCODE_DONE;
    }
    if (mo->weak_binding->data_offset) {
        ptr = section_ptr_add(mo->weak_binding, 1);
        *ptr = BIND_OPCODE_DONE;
    }
    tcc_free(mo->s_lazy_bind);
    tcc_free(mo->s_rebase);
    tcc_free(mo->bind);
}
#endif

struct trie_info {
    const char *name;
    int flag;
    addr_t addr;
    int str_size;
    int term_size;
};

struct trie_node {
    int start;
    int end;
    int index_start;
    int index_end;
    int n_child;
    struct trie_node *child;
};

struct trie_seq {
    int n_child;
    struct trie_node *node;
    int offset;
    int nest_offset;
};

static void create_trie(struct trie_node *node,
                        int from, int to, int index_start,
                        int n_trie, struct trie_info *trie)
{
    int i;
    int start, end, index_end;
    char cur;
    struct trie_node *child;

    for (i = from; i < to; i = end) {
        cur = trie[i].name[index_start];
        start = i++;
        for (; i < to; i++)
            if (cur != trie[i].name[index_start])
                break;
        end = i;
        if (start == end - 1 ||
            (trie[start].name[index_start] &&
             trie[start].name[index_start + 1] == 0))
            index_end = trie[start].str_size - 1;
        else {
            index_end = index_start + 1;
            for (;;) {
                cur = trie[start].name[index_end];
                for (i = start + 1; i < end; i++)
                    if (cur != trie[i].name[index_end])
                        break;
                if (trie[start].name[index_end] &&
                    trie[start].name[index_end + 1] == 0) {
                    end = start + 1;
                    index_end = trie[start].str_size - 1;
                    break;
                }
                if (i != end)
                    break;
                index_end++;
            }
        }
        node->child = tcc_realloc(node->child,
                                  (node->n_child + 1) *
                                  sizeof(struct trie_node));
        child = &node->child[node->n_child];
        child->start = start;
        child->end = end;
        child->index_start = index_start;
        child->index_end = index_end;
        child->n_child = 0;
        child->child = NULL;
        node->n_child++;
        if (start != end - 1)
            create_trie(child, start, end, index_end, n_trie, trie);
    }
}

static int create_seq(int *offset, int *n_seq, struct trie_seq **seq,
                        struct trie_node *node,
                        int n_trie, struct trie_info *trie)
{
    int i, nest_offset, last_seq = *n_seq, retval = *offset;
    struct trie_seq *p_seq;
    struct trie_node *p_nest;

    for (i = 0; i < node->n_child; i++) {
        p_nest = &node->child[i];
        *seq = tcc_realloc(*seq, (*n_seq + 1) * sizeof(struct trie_seq));
        p_seq = &(*seq)[(*n_seq)++];
        p_seq->n_child = i == 0 ? node->n_child : -1;
        p_seq->node = p_nest;
        p_seq->offset = *offset;
        p_seq->nest_offset = 0;
        *offset += (i == 0 ? 1 + 1 : 0) +
                   p_nest->index_end - p_nest->index_start + 1 + 3;
    }
    for (i = 0; i < node->n_child; i++) {
        nest_offset =
            create_seq(offset, n_seq, seq, &node->child[i], n_trie, trie);
        p_seq = &(*seq)[last_seq + i];
        p_seq->nest_offset = nest_offset;
    }
    return retval;
}

static void node_free(struct trie_node *node)
{
    int i;

    for (i = 0; i < node->n_child; i++)
	node_free(&node->child[i]);
    tcc_free(node->child);
}

static int triecmp(const void *_a, const void *_b, void *arg)
{
    struct trie_info *a = (struct trie_info *) _a;
    struct trie_info *b = (struct trie_info *) _b;
    int len_a = strlen(a->name);
    int len_b = strlen(b->name);

    /* strange sorting needed. Name 'xx' should be after 'xx1' */
    if (!strncmp(a->name, b->name, len_a < len_b ? len_a : len_b))
	return len_a < len_b ? 1 : (len_a > len_b ? -1 : 0);
    return strcmp(a->name, b->name);
}

static void export_trie(TCCState *s1, struct macho *mo)
{
    int i, size, offset = 0, save_offset;
    uint8_t *ptr;
    int sym_index;
    int sym_end = symtab_section->data_offset / sizeof(ElfW(Sym));
    int n_trie = 0, n_seq = 0;
    struct trie_info *trie = NULL, *p_trie;
    struct trie_node node, *p_node;
    struct trie_seq *seq = NULL;
    addr_t vm_addr = get_segment(mo, s1->output_type == TCC_OUTPUT_EXE)->vmaddr;

    for (sym_index = 1; sym_index < sym_end; ++sym_index) {
	ElfW(Sym) *sym = (ElfW(Sym) *)symtab_section->data + sym_index;
	const char *name = (char*)symtab_section->link->data + sym->st_name;

	if (sym->st_shndx != SHN_UNDEF && sym->st_shndx < SHN_LORESERVE &&
            (ELFW(ST_BIND)(sym->st_info) == STB_GLOBAL ||
	     ELFW(ST_BIND)(sym->st_info) == STB_WEAK)) {
	    int flag = EXPORT_SYMBOL_FLAGS_KIND_REGULAR;
	    addr_t addr =
		sym->st_value + s1->sections[sym->st_shndx]->sh_addr - vm_addr;

	    if (ELFW(ST_BIND)(sym->st_info) == STB_WEAK)
		flag |= EXPORT_SYMBOL_FLAGS_WEAK_DEFINITION;
	    dprintf ("%s %d %llx\n", name, flag, (long long)addr + vm_addr);
	    trie = tcc_realloc(trie, (n_trie + 1) * sizeof(struct trie_info));
	    trie[n_trie].name = name;
	    trie[n_trie].flag = flag;
	    trie[n_trie].addr = addr;
	    trie[n_trie].str_size = strlen(name) + 1;
	    trie[n_trie].term_size = uleb128_size(flag) + uleb128_size(addr);
	    n_trie++;
	}
    }
    if (n_trie) {
        tcc_qsort(trie, n_trie, sizeof(struct trie_info), triecmp, NULL);
	memset(&node, 0, sizeof(node));
        create_trie(&node, 0, n_trie, 0, n_trie, trie);
	create_seq(&offset, &n_seq, &seq, &node, n_trie, trie);
        save_offset = offset;
        for (i = 0; i < n_seq; i++) {
            p_node = seq[i].node;
            if (p_node->n_child == 0) {
                p_trie = &trie[p_node->start];
                seq[i].nest_offset = offset;
                offset += 1 + p_trie->term_size + 1;
            }
        }
        for (i = 0; i < n_seq; i++) {
            p_node = seq[i].node;
            p_trie = &trie[p_node->start];
            if (seq[i].n_child >= 0) {
                section_ptr_add(mo->exports,
				seq[i].offset - mo->exports->data_offset);
                ptr = section_ptr_add(mo->exports, 2);
                *ptr++ = 0;
                *ptr = seq[i].n_child;
            }
            size = p_node->index_end - p_node->index_start;
            ptr = section_ptr_add(mo->exports, size + 1);
            memcpy(ptr, &p_trie->name[p_node->index_start], size);
            ptr[size] = 0;
            write_uleb128(mo->exports, seq[i].nest_offset);
        }
        section_ptr_add(mo->exports, save_offset - mo->exports->data_offset);
        for (i = 0; i < n_seq; i++) {
            p_node = seq[i].node;
            if (p_node->n_child == 0) {
                p_trie = &trie[p_node->start];
                write_uleb128(mo->exports, p_trie->term_size);
                write_uleb128(mo->exports, p_trie->flag);
                write_uleb128(mo->exports, p_trie->addr);
                ptr = section_ptr_add(mo->exports, 1);
                *ptr = 0;
            }
        }
        section_ptr_add(mo->exports, -mo->exports->data_offset & 7);
	node_free(&node);
        tcc_free(seq);
    }
    tcc_free(trie);
}

static void collect_sections(TCCState *s1, struct macho *mo, const char *filename)
{
    int i, sk, numsec;
    int used_segment[N_SEGMENT];
    uint64_t curaddr, fileofs;
    Section *s;
    struct segment_command_64 *seg;
    struct dylib_command *dylib;
#ifdef CONFIG_NEW_MACHO
    struct linkedit_data_command *chained_fixups_lc;
    struct linkedit_data_command *export_trie_lc;
#endif
    struct build_version_command *dyldbv;
    struct source_version_command *dyldsv;
    struct rpath_command *rpath;
    struct dylinker_command *dyldlc;
    struct symtab_command *symlc;
    struct dysymtab_command *dysymlc;
    char *str;

    for (i = 0; i < N_SEGMENT; i++)
	used_segment[i] = all_segment[i].used;

    memset (mo->sk_to_sect, 0, sizeof(mo->sk_to_sect));
    for (i = s1->nb_sections; i-- > 1;) {
        int type, flags;
        s = s1->sections[i];
        type = s->sh_type;
        flags = s->sh_flags;
        sk = sk_unknown;
	/* debug sections have sometimes no SHF_ALLOC */
        if ((flags & SHF_ALLOC) || !strncmp(s->name, ".debug_", 7)) {
            switch (type) {
            default:           sk = sk_unknown; break;
            case SHT_INIT_ARRAY: sk = sk_init; break;
            case SHT_FINI_ARRAY: sk = sk_fini; break;
            case SHT_NOBITS:   sk = sk_bss; break;
            case SHT_SYMTAB:   sk = sk_discard; break;
            case SHT_STRTAB:
		if (s == stabstr_section)
		  sk = sk_stab_str;
		else
		  sk = sk_discard;
		break;
            case SHT_RELX:     sk = sk_discard; break;
            case SHT_LINKEDIT: sk = sk_linkedit; break;
            case SHT_PROGBITS:
                if (s == mo->stubs)
                  sk = sk_stubs;
#ifndef CONFIG_NEW_MACHO
                else if (s == mo->stub_helper)
                  sk = sk_stub_helper;
                else if (s == mo->la_symbol_ptr)
                  sk = sk_la_ptr;
#endif
                else if (s == rodata_section)
                  sk = sk_ro_data;
                else if (s == s1->got)
                  sk = sk_nl_ptr;
                else if (s == stab_section)
                  sk = sk_stab;
                else if (s == dwarf_info_section)
                  sk = sk_debug_info;
                else if (s == dwarf_abbrev_section)
                  sk = sk_debug_abbrev;
                else if (s == dwarf_line_section)
                  sk = sk_debug_line;
                else if (s == dwarf_aranges_section)
                  sk = sk_debug_aranges;
                else if (s == dwarf_str_section)
                  sk = sk_debug_str;
                else if (s == dwarf_line_str_section)
                  sk = sk_debug_line_str;
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
	used_segment[skinfo[sk].seg_initial] = 1;
    }

    if (s1->output_type != TCC_OUTPUT_EXE)
	used_segment[0] = 0;

    for (i = 0; i < N_SEGMENT; i++)
	if (used_segment[i]) {
	    seg = add_segment(mo, all_segment[i].name);
	    if (i == 1 && s1->output_type != TCC_OUTPUT_EXE)
	        seg->vmaddr = 0;
	    else
	        seg->vmaddr = all_segment[i].vmaddr;
	    seg->vmsize = all_segment[i].vmsize;
	    seg->maxprot = all_segment[i].maxprot;
	    seg->initprot = all_segment[i].initprot;
	    seg->flags = all_segment[i].flags;
            for (sk = sk_unknown; sk < sk_last; sk++)
		if (skinfo[sk].seg_initial == i)
	            mo->segment[sk] = mo->nseg - 1;
	}

    if (s1->output_type != TCC_OUTPUT_EXE) {
	const char *name = s1->install_name ? s1->install_name : filename;
        i = (sizeof(*dylib) + strlen(name) + 1 + 7) &-8;
        dylib = add_lc(mo, LC_ID_DYLIB, i);
	dylib->name = sizeof(*dylib);
	dylib->timestamp = 1;
	dylib->current_version =
	    s1->current_version ? s1->current_version : 1 << 16;
	dylib->compatibility_version =
	    s1->compatibility_version ? s1->compatibility_version : 1 << 16;
        str = (char*)dylib + dylib->name;
        strcpy(str, name);
    }

#ifdef CONFIG_NEW_MACHO
    chained_fixups_lc = add_lc(mo, LC_DYLD_CHAINED_FIXUPS,
			       sizeof(struct linkedit_data_command));
    export_trie_lc = add_lc(mo, LC_DYLD_EXPORTS_TRIE,
			    sizeof(struct linkedit_data_command));
#else
    mo->dyldinfo = add_lc(mo, LC_DYLD_INFO_ONLY, sizeof(*mo->dyldinfo));
#endif

    symlc = add_lc(mo, LC_SYMTAB, sizeof(*symlc));
    dysymlc = add_lc(mo, LC_DYSYMTAB, sizeof(*dysymlc));

    if (s1->output_type == TCC_OUTPUT_EXE) {
        i = (sizeof(*dyldlc) + strlen("/usr/lib/dyld") + 1 + 7) &-8;
        dyldlc = add_lc(mo, LC_LOAD_DYLINKER, i);
        dyldlc->name = sizeof(*dyldlc);
        str = (char*)dyldlc + dyldlc->name;
        strcpy(str, "/usr/lib/dyld");
    }

    dyldbv = add_lc(mo, LC_BUILD_VERSION, sizeof(*dyldbv));
    dyldbv->platform = PLATFORM_MACOS;
    dyldbv->minos = (10 << 16) + (6 << 8);
    dyldbv->sdk = (10 << 16) + (6 << 8);
    dyldbv->ntools = 0;

    dyldsv = add_lc(mo, LC_SOURCE_VERSION, sizeof(*dyldsv));
    dyldsv->version = 0;

    if (s1->output_type == TCC_OUTPUT_EXE) {
        mo->ep = add_lc(mo, LC_MAIN, sizeof(*mo->ep));
        mo->ep->entryoff = 4096;
    }

    for(i = 0; i < s1->nb_loaded_dlls; i++) {
        DLLReference *dllref = s1->loaded_dlls[i];
        if (dllref->level == 0)
          add_dylib(mo, dllref->name);
    }

    if (s1->rpath) {
	char *path = s1->rpath, *end;
	do {
	    end = strchr(path, ':');
	    if (!end)
		end = strchr(path, 0);
            i = (sizeof(*rpath) + (end - path) + 1 + 7) &-8;
            rpath = add_lc(mo, LC_RPATH, i);
            rpath->path = sizeof(*rpath);
            str = (char*)rpath + rpath->path;
            memcpy(str, path, end - path);
	    str[end - path] = 0;
	    path = end + 1;
	} while (*end);
    }

    fileofs = 4096;  /* leave space for mach-o headers */
    curaddr = get_segment(mo, s1->output_type == TCC_OUTPUT_EXE)->vmaddr;
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
#ifdef CONFIG_NEW_MACHO
	if (sk == sk_linkedit) {
	    calc_fixup_size(s1, mo);
	    export_trie(s1, mo);
	}
#else
	if (sk == sk_linkedit) {
	    bind_rebase(s1, mo);
	    export_trie(s1, mo);
	}
#endif
        if (skinfo[sk].seg_initial &&
	    (s1->output_type != TCC_OUTPUT_EXE || mo->segment[sk]) &&
	    mo->sk_to_sect[sk].s) {
            uint64_t al = 0;
            int si;
            seg = get_segment(mo, mo->segment[sk]);
            if (skinfo[sk].name) {
                si = add_section(mo, &seg, skinfo[sk].name);
                numsec++;
                mo->lc[mo->seg2lc[mo->segment[sk]]] = (struct load_command*)seg;
                mo->sk_to_sect[sk].machosect = si;
                sec = get_section(seg, si);
                sec->flags = skinfo[sk].flags;
		if (sk == sk_stubs)
#ifdef TCC_TARGET_X86_64
	    	    sec->reserved2 = 6;
#elif defined TCC_TARGET_ARM64
	    	    sec->reserved2 = 12;
#endif
		if (sk == sk_nl_ptr)
	    	    sec->reserved1 = mo->nr_plt;
#ifndef CONFIG_NEW_MACHO
		if (sk == sk_la_ptr)
	    	    sec->reserved1 = mo->nr_plt + mo->n_got;
#endif
            }
            if (seg->vmaddr == -1) {
                curaddr = (curaddr + SEG_PAGE_SIZE - 1) & -SEG_PAGE_SIZE;
                seg->vmaddr = curaddr;
                fileofs = (fileofs + SEG_PAGE_SIZE - 1) & -SEG_PAGE_SIZE;
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
            al = 1ULL << al;
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
                dprintf("%s: curaddr now 0x%lx\n", s->name, (long)curaddr);
                s->sh_addr = curaddr;
                curaddr += s->sh_size;
                if (s->sh_type != SHT_NOBITS) {
                    fileofs = (fileofs + al - 1) & -al;
                    s->sh_offset = fileofs;
                    fileofs += s->sh_size;
                    dprintf("%s: fileofs now %ld\n", s->name, (long)fileofs);
                }
                if (sec)
                  mo->elfsectomacho[s->sh_num] = numsec;
            }
            if (sec)
              sec->size = curaddr - sec->addr;
        }
        if (DEBUG_MACHO)
          for (s = mo->sk_to_sect[sk].s; s; s = s->prev) {
              int type = s->sh_type;
              int flags = s->sh_flags;
              printf("%d section %-16s %-10s %09lx %04x %02d %s,%s,%s\n",
                     sk,
                     s->name,
                     type == SHT_PROGBITS ? "progbits" :
                     type == SHT_NOBITS ? "nobits" :
                     type == SHT_SYMTAB ? "symtab" :
                     type == SHT_STRTAB ? "strtab" :
                     type == SHT_INIT_ARRAY ? "init" :
                     type == SHT_FINI_ARRAY ? "fini" :
                     type == SHT_RELX ? "rel" : "???",
                     (long)s->sh_addr,
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

#ifdef CONFIG_NEW_MACHO
    if (mo->chained_fixups->data_offset) {
        chained_fixups_lc->dataoff = mo->chained_fixups->sh_offset;
        chained_fixups_lc->datasize = mo->chained_fixups->data_offset;
    }
    if (mo->exports->data_offset) {
        export_trie_lc->dataoff = mo->exports->sh_offset;
        export_trie_lc->datasize = mo->exports->data_offset;
    }
#else
    if (mo->rebase->data_offset) {
        mo->dyldinfo->rebase_off = mo->rebase->sh_offset;
        mo->dyldinfo->rebase_size = mo->rebase->data_offset;
    }
    if (mo->binding->data_offset) {
        mo->dyldinfo->bind_off = mo->binding->sh_offset;
        mo->dyldinfo->bind_size = mo->binding->data_offset;
    }
    if (mo->weak_binding->data_offset) {
        mo->dyldinfo->weak_bind_off = mo->weak_binding->sh_offset;
        mo->dyldinfo->weak_bind_size = mo->weak_binding->data_offset;
    }
    if (mo->lazy_binding->data_offset) {
        mo->dyldinfo->lazy_bind_off = mo->lazy_binding->sh_offset;
        mo->dyldinfo->lazy_bind_size = mo->lazy_binding->data_offset;
    }
    if (mo->exports->data_offset) {
        mo->dyldinfo->export_off = mo->exports->sh_offset;
        mo->dyldinfo->export_size = mo->exports->data_offset;
    }
#endif
}

static void macho_write(TCCState *s1, struct macho *mo, FILE *fp)
{
    int i, sk;
    uint64_t fileofs = 0;
    Section *s;
    mo->mh.mh.magic = MH_MAGIC_64;
#ifdef TCC_TARGET_X86_64
    mo->mh.mh.cputype = CPU_TYPE_X86_64;
    mo->mh.mh.cpusubtype = CPU_SUBTYPE_LIB64 | CPU_SUBTYPE_X86_ALL;
#elif defined TCC_TARGET_ARM64
    mo->mh.mh.cputype = CPU_TYPE_ARM64;
    mo->mh.mh.cpusubtype = CPU_SUBTYPE_ARM64_ALL;
#endif
    if (s1->output_type == TCC_OUTPUT_EXE) {
        mo->mh.mh.filetype = MH_EXECUTE;
        mo->mh.mh.flags = MH_DYLDLINK | MH_PIE;
    }
    else {
        mo->mh.mh.filetype = MH_DYLIB;
        mo->mh.mh.flags = MH_DYLDLINK;
    }
    mo->mh.mh.ncmds = mo->nlc;
    mo->mh.mh.sizeofcmds = 0;
    for (i = 0; i < mo->nlc; i++)
      mo->mh.mh.sizeofcmds += mo->lc[i]->cmdsize;

    fwrite(&mo->mh, 1, sizeof(mo->mh), fp);
    fileofs += sizeof(mo->mh);
    for (i = 0; i < mo->nlc; i++) {
        fwrite(mo->lc[i], 1, mo->lc[i]->cmdsize, fp);
        fileofs += mo->lc[i]->cmdsize;
    }

    for (sk = sk_unknown; sk < sk_last; sk++) {
        //struct segment_command_64 *seg;
        if (skinfo[sk].seg_initial == 0 ||
	    (s1->output_type == TCC_OUTPUT_EXE && !mo->segment[sk]) ||
	    !mo->sk_to_sect[sk].s)
          continue;
        /*seg =*/ get_segment(mo, mo->segment[sk]);
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

#ifdef CONFIG_NEW_MACHO
static int bind_rebase_cmp(const void *_a, const void *_b, void *arg)
{
    TCCState *s1 = arg;
    struct bind_rebase *a = (struct bind_rebase *) _a;
    struct bind_rebase *b = (struct bind_rebase *) _b;
    addr_t aa = s1->sections[a->section]->sh_addr + a->rel.r_offset;
    addr_t ab = s1->sections[b->section]->sh_addr + b->rel.r_offset;

    return aa > ab ? 1 : aa < ab ? -1 : 0;
}

ST_FUNC void bind_rebase_import(TCCState *s1, struct macho *mo)
{
    int i, j, k, bind_index, size, page_count, sym_index;
    const char *name;
    ElfW(Sym) *sym;
    unsigned char *data = mo->chained_fixups->data;
    struct segment_command_64 *seg;
    struct dyld_chained_fixups_header *header;
    struct dyld_chained_starts_in_image *image;
    struct dyld_chained_starts_in_segment *segment;
    struct dyld_chained_import *import;

    tcc_qsort(mo->bind_rebase, mo->n_bind_rebase, sizeof(struct bind_rebase),
	      bind_rebase_cmp, s1);
    for (i = 0; i < mo->n_bind_rebase - 1; i++)
	if (mo->bind_rebase[i].section == mo->bind_rebase[i + 1].section &&
	    mo->bind_rebase[i].rel.r_offset == mo->bind_rebase[i + 1].rel.r_offset) {
	    sym_index = ELFW(R_SYM)(mo->bind_rebase[i].rel.r_info);
            sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
	    name = (char *) symtab_section->link->data + sym->st_name;
	    tcc_error("Overlap %s/%s %s:%s",
		      mo->bind_rebase[i].bind ? "bind" : "rebase",
		      mo->bind_rebase[i + 1].bind ? "bind" : "rebase",
		      s1->sections[mo->bind_rebase[i].section]->name, name);
	}
    header = (struct dyld_chained_fixups_header *) data;
    data += (sizeof(struct dyld_chained_fixups_header) + 7) & -8;
    header->starts_offset = data - mo->chained_fixups->data;
    header->imports_count = mo->n_bind;
    header->imports_format = DYLD_CHAINED_IMPORT;
    header->symbols_format = 0;
    size = sizeof(struct dyld_chained_starts_in_image) +
	   (mo->nseg - 1) * sizeof(uint32_t);
    image = (struct dyld_chained_starts_in_image *) data;
    data += (size + 7) & -8;
    image->seg_count = mo->nseg;
    for (i = (s1->output_type == TCC_OUTPUT_EXE); i < mo->nseg - 1; i++) {
        image->seg_info_offset[i] = (data - mo->chained_fixups->data) -
				    header->starts_offset;
	seg = get_segment(mo, i);
	page_count = (seg->vmsize + SEG_PAGE_SIZE - 1) / SEG_PAGE_SIZE;
	size = sizeof(struct dyld_chained_starts_in_segment) +
		      (page_count - 1) * sizeof(uint16_t);
        segment = (struct dyld_chained_starts_in_segment *) data;
        data += (size + 7) & -8;
        segment->size = size;
        segment->page_size = SEG_PAGE_SIZE;
#if 1
#define	PTR_64_OFFSET 0
#define	PTR_64_MASK   0x7FFFFFFFFFFULL
        segment->pointer_format = DYLD_CHAINED_PTR_64;
#else
#define	PTR_64_OFFSET 0x100000000ULL
#define	PTR_64_MASK   0xFFFFFFFFFFFFFFULL
        segment->pointer_format = DYLD_CHAINED_PTR_64_OFFSET;
#endif
        segment->segment_offset = seg->fileoff;
        segment->max_valid_pointer = 0;
        segment->page_count = page_count;
	// add bind/rebase
	bind_index = 0;
	k = 0;
	for (j = 0; j < page_count; j++) {
	    addr_t start = seg->vmaddr + j * SEG_PAGE_SIZE;
	    addr_t end = start + SEG_PAGE_SIZE;
	    void *last = NULL;
	    addr_t last_o = 0;
	    addr_t cur_o, cur;
	    struct dyld_chained_ptr_64_rebase *rebase;
	    struct dyld_chained_ptr_64_bind *bind;

	    segment->page_start[j] = DYLD_CHAINED_PTR_START_NONE;
	    for (; k < mo->n_bind_rebase; k++) {
	        Section *s = s1->sections[mo->bind_rebase[k].section];
		addr_t r_offset = mo->bind_rebase[k].rel.r_offset;
		addr_t addr = s->sh_addr + r_offset;

		if ((addr & 3) ||
		    (addr & (SEG_PAGE_SIZE - 1)) > SEG_PAGE_SIZE - PTR_SIZE)
		    tcc_error("Illegal rel_offset %s %lld",
			      s->name, (long long)r_offset);
		if (addr >= end)
		    break;
		if (addr >= start) {
		    cur_o = addr - start;
	            if (mo->bind_rebase[k].bind) {
		        if (segment->page_start[j] == DYLD_CHAINED_PTR_START_NONE)
			    segment->page_start[j] = cur_o;
		        else {
			    bind = (struct dyld_chained_ptr_64_bind *) last;
			    bind->next = (cur_o - last_o) / 4;
		        }
		        bind = (struct dyld_chained_ptr_64_bind *)
				    (s->data + r_offset);
		        last = bind;
		        last_o = cur_o;
		        bind->ordinal = bind_index;
		        bind->addend = 0;
		        bind->reserved = 0;
		        bind->next = 0;
		        bind->bind = 1;
		    }
		    else {
		        if (segment->page_start[j] == DYLD_CHAINED_PTR_START_NONE)
			    segment->page_start[j] = cur_o;
		        else {
			    rebase = (struct dyld_chained_ptr_64_rebase *) last;
			    rebase->next = (cur_o - last_o) / 4;
		        }
		        rebase = (struct dyld_chained_ptr_64_rebase *)
				    (s->data + r_offset);
		        last = rebase;
		        last_o = cur_o;
		        cur = (*(uint64_t *) (s->data + r_offset)) -
			      PTR_64_OFFSET;
		        rebase->target = cur & PTR_64_MASK;
		        rebase->high8 = cur >> (64 - 8);
			if (cur != ((uint64_t)rebase->high8 << (64 - 8)) + rebase->target)
			    tcc_error("rebase error");
		        rebase->reserved = 0;
		        rebase->next = 0;
		        rebase->bind = 0;
		    }
		}
		bind_index += mo->bind_rebase[k].bind;
	    }
	}
    }
    // add imports
    header->imports_offset = data - mo->chained_fixups->data;
    import = (struct dyld_chained_import *) data;
    data += mo->n_bind * sizeof (struct dyld_chained_import);
    header->symbols_offset = data - mo->chained_fixups->data;
    data++;
    for (i = 0, bind_index = 0; i < mo->n_bind_rebase; i++) {
	if (mo->bind_rebase[i].bind) {
	    import[bind_index].lib_ordinal =
		BIND_SPECIAL_DYLIB_FLAT_LOOKUP & 0xffu;
	    import[bind_index].name_offset =
		(data - mo->chained_fixups->data) - header->symbols_offset;
	    sym_index = ELFW(R_SYM)(mo->bind_rebase[i].rel.r_info);
	    sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
	    import[bind_index].weak_import =
		ELFW(ST_BIND)(sym->st_info) == STB_WEAK;
	    name = (char *) symtab_section->link->data + sym->st_name;
            strcpy((char *) data, name);
	    data += strlen(name) + 1;
	    bind_index++;
	}
    }
    tcc_free(mo->bind_rebase);
}
#endif

ST_FUNC int macho_output_file(TCCState *s1, const char *filename)
{
    int fd, mode, file_type;
    FILE *fp;
    int i, ret = -1;
    struct macho mo;

    (void)memset(&mo, 0, sizeof(mo));

    file_type = s1->output_type;
    if (file_type == TCC_OUTPUT_OBJ)
        mode = 0666;
    else
        mode = 0777;
    unlink(filename);
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, mode);
    if (fd < 0 || (fp = fdopen(fd, "wb")) == NULL) {
        tcc_error_noabort("could not write '%s: %s'", filename, strerror(errno));
        return -1;
    }
    if (s1->verbose)
        printf("<- %s\n", filename);

    tcc_add_runtime(s1);
    tcc_macho_add_destructor(s1);
    resolve_common_syms(s1);
    create_symtab(s1, &mo);
    check_relocs(s1, &mo);
    ret = check_symbols(s1, &mo);
    if (!ret) {
	int save_output = s1->output_type;

        collect_sections(s1, &mo, filename);
        relocate_syms(s1, s1->symtab, 0);
	if (s1->output_type == TCC_OUTPUT_EXE)
            mo.ep->entryoff = get_sym_addr(s1, "main", 1, 1)
                            -     get_segment(&mo, 1)->vmaddr;
        if (s1->nb_errors)
          goto do_ret;
	// Macho uses bind/rebase instead of dynsym
	s1->output_type = TCC_OUTPUT_EXE;
        relocate_sections(s1);
	s1->output_type = save_output;
#ifdef CONFIG_NEW_MACHO
	bind_rebase_import(s1, &mo);
#endif
        convert_symbols(s1, &mo);
        macho_write(s1, &mo, fp);
    }

 do_ret:
    for (i = 0; i < mo.nlc; i++)
      tcc_free(mo.lc[i]);
    tcc_free(mo.seg2lc);
    tcc_free(mo.lc);
    tcc_free(mo.elfsectomacho);
    tcc_free(mo.e2msym);

    fclose(fp);
#ifdef CONFIG_CODESIGN
    if (!ret) {
	char command[1024];
	int retval;

	snprintf(command, sizeof(command), "codesign -f -s - %s", filename);
	retval = system (command);
	if (retval == -1 || !(WIFEXITED(retval) && WEXITSTATUS(retval) == 0))
	    tcc_error ("command failed '%s'", command);
    }
#endif
    return ret;
}

static uint32_t macho_swap32(uint32_t x)
{
  return (x >> 24) | (x << 24) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8);
}
#define SWAP(x) (swap ? macho_swap32(x) : (x))
#define tbd_parse_movepast(s) \
    (pos = (pos = strstr(pos, s)) ? pos + strlen(s) : NULL)
#define tbd_parse_movetoany(cs) (pos = strpbrk(pos, cs))
#define tbd_parse_skipws while (*pos && (*pos==' '||*pos=='\n')) ++pos
#define tbd_parse_tramplequote if(*pos=='\''||*pos=='"') tbd_parse_trample
#define tbd_parse_tramplespace if(*pos==' ') tbd_parse_trample
#define tbd_parse_trample *pos++=0

#ifdef TCC_IS_NATIVE
/* Looks for the active developer SDK set by xcode-select (or the default
   one set during installation.) */
ST_FUNC void tcc_add_macos_sdkpath(TCCState* s)
{
    char *sdkroot = NULL, *pos = NULL;
    void* xcs = dlopen("libxcselect.dylib", RTLD_GLOBAL | RTLD_LAZY);
    CString path;
    int (*f)(unsigned int, char**) = dlsym(xcs, "xcselect_host_sdk_path");
    cstr_new(&path);
    if (f) f(1, &sdkroot);
    if (sdkroot)
        pos = strstr(sdkroot,"SDKs/MacOSX");
    if (pos)
        cstr_printf(&path, "%.*s.sdk/usr/lib", (int)(pos - sdkroot + 11), sdkroot);
    /* must use free from libc directly */
#pragma push_macro("free")
#undef free
    free(sdkroot);
#pragma pop_macro("free")
    if (path.size)
        tcc_add_library_path(s, (char*)path.data);
    else
        tcc_add_library_path(s,
            "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib"
            ":" "/Applications/Xcode.app/Developer/SDKs/MacOSX.sdk/usr/lib"
            );
    cstr_free(&path);
}

ST_FUNC const char* macho_tbd_soname(const char* filename) {
    char *soname, *data, *pos;
    const char *ret = filename;

    int fd = open(filename,O_RDONLY);
    if (fd<0) return ret;
    pos = data = tcc_load_text(fd);
    if (!tbd_parse_movepast("install-name: ")) goto the_end;
    tbd_parse_skipws;
    tbd_parse_tramplequote;
    soname = pos;
    if (!tbd_parse_movetoany("\n \"'")) goto the_end;
    tbd_parse_trample;
    ret = tcc_strdup(soname);
the_end:
    tcc_free(data);
    return ret;
}
#endif /* TCC_IS_NATIVE */

ST_FUNC int macho_load_tbd(TCCState* s1, int fd, const char* filename, int lev)
{
    char *soname, *data, *pos;
    int ret = -1;

    pos = data = tcc_load_text(fd);
    if (!tbd_parse_movepast("install-name: ")) goto the_end;
    tbd_parse_skipws;
    tbd_parse_tramplequote;
    soname = pos;
    if (!tbd_parse_movetoany("\n \"'")) goto the_end;
    tbd_parse_trample;
    ret = 0;
    if (tcc_add_dllref(s1, soname, lev)->found)
        goto the_end;
    while(pos) {
        char* sym = NULL;
        int cont = 1;
        if (!tbd_parse_movepast("symbols: ")) break;
        if (!tbd_parse_movepast("[")) break;
        while (cont) {
            tbd_parse_skipws;
            tbd_parse_tramplequote;
            sym = pos;
            if (!tbd_parse_movetoany(",] \"'")) break;
            tbd_parse_tramplequote;
            tbd_parse_tramplespace;
            tbd_parse_skipws;
            if (*pos==0||*pos==']') cont=0;
            tbd_parse_trample;
            set_elf_sym(s1->dynsymtab_section, 0, 0,
                ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE), 0, SHN_UNDEF, sym);
        }
    }

the_end:
    tcc_free(data);
    return ret;
}

ST_FUNC int macho_load_dll(TCCState * s1, int fd, const char* filename, int lev)
{
    unsigned char buf[sizeof(struct mach_header_64)];
    void *buf2;
    uint32_t machofs = 0;
    struct fat_header fh;
    struct mach_header mh;
    struct load_command *lc;
    int i, swap = 0;
    const char *soname = filename;
    struct nlist_64 *symtab = 0;
    uint32_t nsyms = 0;
    char *strtab = 0;
    uint32_t strsize = 0;
    uint32_t iextdef = 0;
    uint32_t nextdef = 0;

  again:
    if (full_read(fd, buf, sizeof(buf)) != sizeof(buf))
      return -1;
    memcpy(&fh, buf, sizeof(fh));
    if (fh.magic == FAT_MAGIC || fh.magic == FAT_CIGAM) {
        struct fat_arch *fa = load_data(fd, sizeof(fh),
                                        fh.nfat_arch * sizeof(*fa));
        swap = fh.magic == FAT_CIGAM;
        for (i = 0; i < SWAP(fh.nfat_arch); i++)
#ifdef TCC_TARGET_X86_64
          if (SWAP(fa[i].cputype) == CPU_TYPE_X86_64
              && SWAP(fa[i].cpusubtype) == CPU_SUBTYPE_X86_ALL)
#elif defined TCC_TARGET_ARM64
          if (SWAP(fa[i].cputype) == CPU_TYPE_ARM64
              && SWAP(fa[i].cpusubtype) == CPU_SUBTYPE_ARM64_ALL)
#endif
            break;
        if (i == SWAP(fh.nfat_arch)) {
            tcc_free(fa);
            return -1;
        }
        machofs = SWAP(fa[i].offset);
        tcc_free(fa);
        lseek(fd, machofs, SEEK_SET);
        goto again;
    } else if (fh.magic == FAT_MAGIC_64 || fh.magic == FAT_CIGAM_64) {
        tcc_warning("%s: Mach-O fat 64bit files of type 0x%x not handled",
                    filename, fh.magic);
        return -1;
    }

    memcpy(&mh, buf, sizeof(mh));
    if (mh.magic != MH_MAGIC_64)
      return -1;
    dprintf("found Mach-O at %d\n", machofs);
    buf2 = load_data(fd, machofs + sizeof(struct mach_header_64), mh.sizeofcmds);
    for (i = 0, lc = buf2; i < mh.ncmds; i++) {
        dprintf("lc %2d: 0x%08x\n", i, lc->cmd);
        switch (lc->cmd) {
        case LC_SYMTAB:
        {
            struct symtab_command *sc = (struct symtab_command*)lc;
            nsyms = sc->nsyms;
            symtab = load_data(fd, machofs + sc->symoff, nsyms * sizeof(*symtab));
            strsize = sc->strsize;
            strtab = load_data(fd, machofs + sc->stroff, strsize);
            break;
        }
        case LC_ID_DYLIB:
        {
            struct dylib_command *dc = (struct dylib_command*)lc;
            soname = (char*)lc + dc->name;
            dprintf(" ID_DYLIB %d 0x%x 0x%x %s\n",
                    dc->timestamp, dc->current_version,
                    dc->compatibility_version, soname);
            break;
        }
        case LC_REEXPORT_DYLIB:
        {
            struct dylib_command *dc = (struct dylib_command*)lc;
            char *name = (char*)lc + dc->name;
            int subfd = open(name, O_RDONLY | O_BINARY);
            dprintf(" REEXPORT %s\n", name);
            if (subfd < 0)
              tcc_warning("can't open %s (reexported from %s)", name, filename);
            else {
                /* Hopefully the REEXPORTs never form a cycle, we don't check
                   for that!  */
                macho_load_dll(s1, subfd, name, lev + 1);
                close(subfd);
            }
            break;
        }
        case LC_DYSYMTAB:
        {
            struct dysymtab_command *dc = (struct dysymtab_command*)lc;
            iextdef = dc->iextdefsym;
            nextdef = dc->nextdefsym;
            break;
        }
        }
        lc = (struct load_command*) ((char*)lc + lc->cmdsize);
    }

    if (tcc_add_dllref(s1, soname, lev)->found)
        goto the_end;

    if (!nsyms || !nextdef)
      tcc_warning("%s doesn't export any symbols?", filename);

    //dprintf("symbols (all):\n");
    dprintf("symbols (exported):\n");
    dprintf("    n: typ sec   desc              value name\n");
    //for (i = 0; i < nsyms; i++) {
    for (i = iextdef; i < iextdef + nextdef; i++) {
        struct nlist_64 *sym = symtab + i;
        dprintf("%5d: %3d %3d 0x%04x 0x%016lx %s\n",
                i, sym->n_type, sym->n_sect, sym->n_desc, (long)sym->n_value,
                strtab + sym->n_strx);
        set_elf_sym(s1->dynsymtab_section, 0, 0,
                    ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE),
                    0, SHN_UNDEF, strtab + sym->n_strx);
    }

  the_end:
    tcc_free(strtab);
    tcc_free(symtab);
    tcc_free(buf2);
    return 0;
}
