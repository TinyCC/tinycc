#ifdef TARGET_DEFS_ONLY

#define EM_TCC_TARGET EM_C60

/* relocation type for 32 bit data relocation */
#define R_DATA_32   R_C60_32
#define R_DATA_PTR  R_C60_32
#define R_JMP_SLOT  R_C60_JMP_SLOT
#define R_GLOB_DAT  R_C60_GLOB_DAT
#define R_COPY      R_C60_COPY

#define R_NUM       R_C60_NUM

#define ELF_START_ADDR 0x00000400
#define ELF_PAGE_SIZE  0x1000

#define HAVE_SECTION_RELOC
#define PCRELATIVE_DLLPLT 0

#else /* !TARGET_DEFS_ONLY */

#include "tcc.h"

ST_DATA struct reloc_info relocs_info[R_NUM] = {
    INIT_RELOC_INFO (R_C60_32, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_C60LO16, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_C60HI16, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_C60_GOTOFF, 0, BUILD_GOT_ONLY, 0)
    INIT_RELOC_INFO (R_C60_GOTPC, 0, BUILD_GOT_ONLY, 0)
    INIT_RELOC_INFO (R_C60_GOT32, 0, ALWAYS_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_C60_PLT32, 1, ALWAYS_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_C60_GLOB_DAT, 0, NO_GOTPLT_ENTRY, 0)
    INIT_RELOC_INFO (R_C60_JMP_SLOT, 1, NO_GOTPLT_ENTRY, 0)
};

void relocate_init(Section *sr) {}

void relocate(TCCState *s1, ElfW_Rel *rel, int type, char *ptr, addr_t addr, addr_t val)
{
    switch(type) {
        case R_C60_32:
            *(int *)ptr += val;
            break;
        case R_C60LO16:
            {
                uint32_t orig;

                /* put the low 16 bits of the absolute address add to what is
                   already there */
                orig  =   ((*(int *)(ptr  )) >> 7) & 0xffff;
                orig |=  (((*(int *)(ptr+4)) >> 7) & 0xffff) << 16;

                /* patch both at once - assumes always in pairs Low - High */
                *(int *) ptr    = (*(int *) ptr    & (~(0xffff << 7)) ) |
                                   (((val+orig)      & 0xffff) << 7);
                *(int *)(ptr+4) = (*(int *)(ptr+4) & (~(0xffff << 7)) ) |
                                  ((((val+orig)>>16) & 0xffff) << 7);
            }
            break;
        case R_C60HI16:
            break;
        default:
            fprintf(stderr,"FIXME: handle reloc type %x at %x [%p] to %x\n",
                    type, (unsigned) addr, ptr, (unsigned) val);
            break;
    }
}

#endif /* !TARGET_DEFS_ONLY */
