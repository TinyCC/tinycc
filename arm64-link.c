#ifdef TARGET_DEFS_ONLY

#define EM_TCC_TARGET EM_AARCH64

#define R_DATA_32  R_AARCH64_ABS32
#define R_DATA_PTR R_AARCH64_ABS64
#define R_JMP_SLOT R_AARCH64_JUMP_SLOT
#define R_GLOB_DAT R_AARCH64_GLOB_DAT
#define R_COPY     R_AARCH64_COPY

#define R_NUM      R_AARCH64_NUM

#define ELF_START_ADDR 0x00400000
#define ELF_PAGE_SIZE 0x1000

#define HAVE_SECTION_RELOC
#define PCRELATIVE_DLLPLT 1

#else /* !TARGET_DEFS_ONLY */

#include "tcc.h"

ST_DATA struct reloc_info relocs_info[R_NUM] = {
    INIT_RELOC_INFO (R_AARCH64_ABS32, 0, NO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_ABS64, 0, NO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_MOVW_UABS_G0_NC, 0, NO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_MOVW_UABS_G1_NC, 0, NO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_MOVW_UABS_G2_NC, 0, NO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_MOVW_UABS_G3, 0, NO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_ADR_PREL_PG_HI21, 0, NO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_ADD_ABS_LO12_NC, 0, NO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_JUMP26, 1, AUTO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_CALL26, 1, AUTO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_ADR_GOT_PAGE, 0, ALWAYS_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_LD64_GOT_LO12_NC, 0, ALWAYS_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_GLOB_DAT, 0, NO_GOTPLT_ENTRY)
    INIT_RELOC_INFO (R_AARCH64_JUMP_SLOT, 1, NO_GOTPLT_ENTRY)
};

void relocate_init(Section *sr) {}

void relocate(TCCState *s1, ElfW_Rel *rel, int type, char *ptr, addr_t addr, addr_t val)
{
    int sym_index = ELFW(R_SYM)(rel->r_info);
#ifdef DEBUG_RELOC
    ElfW(Sym) *sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
#endif

    switch(type) {
        case R_AARCH64_ABS64:
            write64le(ptr, val);
            return;
        case R_AARCH64_ABS32:
            write32le(ptr, val);
            return;
        case R_AARCH64_MOVW_UABS_G0_NC:
            write32le(ptr, ((read32le(ptr) & 0xffe0001f) |
                            (val & 0xffff) << 5));
            return;
        case R_AARCH64_MOVW_UABS_G1_NC:
            write32le(ptr, ((read32le(ptr) & 0xffe0001f) |
                            (val >> 16 & 0xffff) << 5));
            return;
        case R_AARCH64_MOVW_UABS_G2_NC:
            write32le(ptr, ((read32le(ptr) & 0xffe0001f) |
                            (val >> 32 & 0xffff) << 5));
            return;
        case R_AARCH64_MOVW_UABS_G3:
            write32le(ptr, ((read32le(ptr) & 0xffe0001f) |
                            (val >> 48 & 0xffff) << 5));
            return;
        case R_AARCH64_ADR_PREL_PG_HI21: {
            uint64_t off = (val >> 12) - (addr >> 12);
            if ((off + ((uint64_t)1 << 20)) >> 21)
                tcc_error("R_AARCH64_ADR_PREL_PG_HI21 relocation failed");
            write32le(ptr, ((read32le(ptr) & 0x9f00001f) |
                            (off & 0x1ffffc) << 3 | (off & 3) << 29));
            return;
        }
        case R_AARCH64_ADD_ABS_LO12_NC:
            write32le(ptr, ((read32le(ptr) & 0xffc003ff) |
                            (val & 0xfff) << 10));
            return;
        case R_AARCH64_JUMP26:
        case R_AARCH64_CALL26:
#ifdef DEBUG_RELOC
	    printf ("reloc %d @ 0x%lx: val=0x%lx name=%s\n", type, addr, val,
		    (char *) symtab_section->link->data + sym->st_name);
#endif
            if (((val - addr) + ((uint64_t)1 << 27)) & ~(uint64_t)0xffffffc)
                tcc_error("R_AARCH64_(JUMP|CALL)26 relocation failed"
                          " (val=%lx, addr=%lx)", val, addr);
            write32le(ptr, (0x14000000 |
                            (uint32_t)(type == R_AARCH64_CALL26) << 31 |
                            ((val - addr) >> 2 & 0x3ffffff)));
            return;
        case R_AARCH64_ADR_GOT_PAGE: {
            uint64_t off =
                (((s1->got->sh_addr +
                   s1->sym_attrs[sym_index].got_offset) >> 12) - (addr >> 12));
            if ((off + ((uint64_t)1 << 20)) >> 21)
                tcc_error("R_AARCH64_ADR_GOT_PAGE relocation failed");
            write32le(ptr, ((read32le(ptr) & 0x9f00001f) |
                            (off & 0x1ffffc) << 3 | (off & 3) << 29));
            return;
        }
        case R_AARCH64_LD64_GOT_LO12_NC:
            write32le(ptr,
                      ((read32le(ptr) & 0xfff803ff) |
                       ((s1->got->sh_addr +
                         s1->sym_attrs[sym_index].got_offset) & 0xff8) << 7));
            return;
        case R_AARCH64_COPY:
            return;
        case R_AARCH64_GLOB_DAT:
        case R_AARCH64_JUMP_SLOT:
            /* They don't need addend */
#ifdef DEBUG_RELOC
	    printf ("reloc %d @ 0x%lx: val=0x%lx name=%s\n", type, addr,
		    val - rel->r_addend,
		    (char *) symtab_section->link->data + sym->st_name);
#endif
            write64le(ptr, val - rel->r_addend);
            return;
        default:
            fprintf(stderr, "FIXME: handle reloc type %x at %x [%p] to %x\n",
                    type, (unsigned)addr, ptr, (unsigned)val);
            return;
    }
}

#endif /* !TARGET_DEFS_ONLY */
