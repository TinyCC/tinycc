#include "tcc.h"
#define HAVE_SECTION_RELOC

ST_DATA struct reloc_info relocs_info[] = {
    INIT_RELOC_INFO (R_AARCH64_ABS32, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_ABS64, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_MOVW_UABS_G0_NC, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_MOVW_UABS_G1_NC, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_MOVW_UABS_G2_NC, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_MOVW_UABS_G3, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_ADR_PREL_PG_HI21, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_ADD_ABS_LO12_NC, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_JUMP26, 1, AUTO_GOTPLT_ENTRY, 1)
    INIT_RELOC_INFO (R_AARCH64_CALL26, 1, AUTO_GOTPLT_ENTRY, 1)
    INIT_RELOC_INFO (R_AARCH64_ADR_GOT_PAGE, 0, ALWAYS_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_LD64_GOT_LO12_NC, 0, ALWAYS_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_GLOB_DAT, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_AARCH64_JUMP_SLOT, 1, NO_GOTPLT_ENTRY, 0)
};

void relocate_init(Section *sr) {}

void relocate(TCCState *s1, ElfW_Rel *rel, int type, char *ptr, addr_t addr, addr_t val)
{
    ElfW(Sym) *sym;
    int sym_index;

    sym_index = ELFW(R_SYM)(rel->r_info);
    sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];

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
	    /* This check must match the one in build_got_entries, testing
	       if we really need a PLT slot.  */
	    if (sym->st_shndx == SHN_UNDEF ||
                sym->st_shndx == SHN_ABS)
	        /* We've put the PLT slot offset into r_addend when generating
		   it, and that's what we must use as relocation value (adjusted
		   by section offset of course).  */
		val = s1->plt->sh_addr + rel->r_addend;
#ifdef DEBUG_RELOC
	    printf ("reloc %d @ 0x%lx: val=0x%lx name=%s\n", type, addr, val,
		    (char *) symtab_section->link->data + sym->st_name);
#endif
            if (((val - addr) + ((uint64_t)1 << 27)) & ~(uint64_t)0xffffffc)
	      {
                tcc_error("R_AARCH64_(JUMP|CALL)26 relocation failed (val=%lx, addr=%lx)", addr, val);
	      }
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
