#ifdef TARGET_DEFS_ONLY

#define EM_TCC_TARGET EM_386

/* relocation type for 32 bit data relocation */
#define R_DATA_32   R_386_32
#define R_DATA_PTR  R_386_32
#define R_JMP_SLOT  R_386_JMP_SLOT
#define R_GLOB_DAT  R_386_GLOB_DAT
#define R_COPY      R_386_COPY

#define R_NUM       R_386_NUM

#define ELF_START_ADDR 0x08048000
#define ELF_PAGE_SIZE  0x1000

#define HAVE_SECTION_RELOC
#define PCRELATIVE_DLLPLT 0

#else /* !TARGET_DEFS_ONLY */

#include "tcc.h"

static ElfW_Rel *qrel; /* ptr to next reloc entry reused */
ST_DATA struct reloc_info relocs_info[R_NUM] = {
    INIT_RELOC_INFO (R_386_32, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_386_PC32, 1, AUTO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_386_PLT32, 1, ALWAYS_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_386_GLOB_DAT, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_386_JMP_SLOT, 1, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_386_GOTPC, 0, BUILD_GOT_ONLY, 0)
    INIT_RELOC_INFO (R_386_GOTOFF, 0, BUILD_GOT_ONLY, 0)
    INIT_RELOC_INFO (R_386_GOT32, 0, ALWAYS_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_386_GOT32X, 0, ALWAYS_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_386_16, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_386_PC16, 1, AUTO_GOTPLT_ENTRY, 0)
};

void relocate_init(Section *sr)
{
    qrel = (ElfW_Rel *) sr->data;
}

void relocate(TCCState *s1, ElfW_Rel *rel, int type, char *ptr, addr_t addr, addr_t val)
{
    int sym_index, esym_index;

    sym_index = ELFW(R_SYM)(rel->r_info);

    switch (type) {
        case R_386_32:
            if (s1->output_type == TCC_OUTPUT_DLL) {
                esym_index = s1->symtab_to_dynsym[sym_index];
                qrel->r_offset = rel->r_offset;
                if (esym_index) {
                    qrel->r_info = ELFW(R_INFO)(esym_index, R_386_32);
                    qrel++;
                    return;
                } else {
                    qrel->r_info = ELFW(R_INFO)(0, R_386_RELATIVE);
                    qrel++;
                }
            }
            add32le(ptr, val);
            return;
        case R_386_PC32:
            if (s1->output_type == TCC_OUTPUT_DLL) {
                /* DLL relocation */
                esym_index = s1->symtab_to_dynsym[sym_index];
                if (esym_index) {
                    qrel->r_offset = rel->r_offset;
                    qrel->r_info = ELFW(R_INFO)(esym_index, R_386_PC32);
                    qrel++;
                    return;
                }
            }
            add32le(ptr, val - addr);
            return;
        case R_386_PLT32:
            add32le(ptr, val - addr);
            return;
        case R_386_GLOB_DAT:
        case R_386_JMP_SLOT:
            write32le(ptr, val);
            return;
        case R_386_GOTPC:
            add32le(ptr, s1->got->sh_addr - addr);
            return;
        case R_386_GOTOFF:
            add32le(ptr, val - s1->got->sh_addr);
            return;
        case R_386_GOT32:
        case R_386_GOT32X:
            /* we load the got offset */
            add32le(ptr, s1->sym_attrs[sym_index].got_offset);
            return;
        case R_386_16:
            if (s1->output_format != TCC_OUTPUT_FORMAT_BINARY) {
            output_file:
                tcc_error("can only produce 16-bit binary files");
            }
            write16le(ptr, read16le(ptr) + val);
            return;
        case R_386_PC16:
            if (s1->output_format != TCC_OUTPUT_FORMAT_BINARY)
                goto output_file;
            write16le(ptr, read16le(ptr) + val - addr);
            return;
        case R_386_RELATIVE:
            /* do nothing */
            return;
        case R_386_COPY:
            /* This reloction must copy initialized data from the library
            to the program .bss segment. Currently made like for ARM
            (to remove noise of defaukt case). Is this true? 
            */
            return;
        default:
            fprintf(stderr,"FIXME: handle reloc type %d at %x [%p] to %x\n",
                type, (unsigned)addr, ptr, (unsigned)val);
            return;
    }
}

#endif /* !TARGET_DEFS_ONLY */
