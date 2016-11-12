#include "tcc.h"
#define HAVE_SECTION_RELOC

void relocate_init(Section *sr) {}

void relocate(TCCState *s1, ElfW_Rel *rel, int type, char *ptr, addr_t addr, addr_t val)
{
    ElfW(Sym) *sym;
    int sym_index;

    sym_index = ELFW(R_SYM)(rel->r_info);
    sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];

    switch(type) {
        case R_ARM_PC24:
        case R_ARM_CALL:
        case R_ARM_JUMP24:
        case R_ARM_PLT32:
            {
                int x, is_thumb, is_call, h, blx_avail, is_bl, th_ko;
                x = (*(int *) ptr) & 0xffffff;
		if (sym->st_shndx == SHN_UNDEF ||
                    s1->output_type == TCC_OUTPUT_MEMORY)
	            val = s1->plt->sh_addr;
#ifdef DEBUG_RELOC
		printf ("reloc %d: x=0x%x val=0x%x ", type, x, val);
#endif
                (*(int *)ptr) &= 0xff000000;
                if (x & 0x800000)
                    x -= 0x1000000;
                x <<= 2;
                blx_avail = (TCC_ARM_VERSION >= 5);
                is_thumb = val & 1;
                is_bl = (*(unsigned *) ptr) >> 24 == 0xeb;
                is_call = (type == R_ARM_CALL || (type == R_ARM_PC24 && is_bl));
                x += val - addr;
#ifdef DEBUG_RELOC
		printf (" newx=0x%x name=%s\n", x,
			(char *) symtab_section->link->data + sym->st_name);
#endif
                h = x & 2;
                th_ko = (x & 3) && (!blx_avail || !is_call);
                if (th_ko || x >= 0x2000000 || x < -0x2000000)
                    tcc_error("can't relocate value at %x,%d",addr, type);
                x >>= 2;
                x &= 0xffffff;
                /* Only reached if blx is avail and it is a call */
                if (is_thumb) {
                    x |= h << 24;
                    (*(int *)ptr) = 0xfa << 24; /* bl -> blx */
                }
                (*(int *) ptr) |= x;
            }
            return;
        /* Since these relocations only concern Thumb-2 and blx instruction was
           introduced before Thumb-2, we can assume blx is available and not
           guard its use */
        case R_ARM_THM_PC22:
        case R_ARM_THM_JUMP24:
            {
                int x, hi, lo, s, j1, j2, i1, i2, imm10, imm11;
                int to_thumb, is_call, to_plt, blx_bit = 1 << 12;
                Section *plt;

                /* weak reference */
                if (sym->st_shndx == SHN_UNDEF &&
                    ELFW(ST_BIND)(sym->st_info) == STB_WEAK)
                    return;

                /* Get initial offset */
                hi = (*(uint16_t *)ptr);
                lo = (*(uint16_t *)(ptr+2));
                s = (hi >> 10) & 1;
                j1 = (lo >> 13) & 1;
                j2 = (lo >> 11) & 1;
                i1 = (j1 ^ s) ^ 1;
                i2 = (j2 ^ s) ^ 1;
                imm10 = hi & 0x3ff;
                imm11 = lo & 0x7ff;
                x = (s << 24) | (i1 << 23) | (i2 << 22) |
                    (imm10 << 12) | (imm11 << 1);
                if (x & 0x01000000)
                    x -= 0x02000000;

                /* Relocation infos */
                to_thumb = val & 1;
                plt = s1->plt;
                to_plt = (val >= plt->sh_addr) &&
                         (val < plt->sh_addr + plt->data_offset);
                is_call = (type == R_ARM_THM_PC22);

                /* Compute final offset */
                if (to_plt && !is_call) /* Point to 1st instr of Thumb stub */
                    x -= 4;
                x += val - addr;
                if (!to_thumb && is_call) {
                    blx_bit = 0; /* bl -> blx */
                    x = (x + 3) & -4; /* Compute offset from aligned PC */
                }

                /* Check that relocation is possible
                   * offset must not be out of range
                   * if target is to be entered in arm mode:
                     - bit 1 must not set
                     - instruction must be a call (bl) or a jump to PLT */
                if (!to_thumb || x >= 0x1000000 || x < -0x1000000)
                    if (to_thumb || (val & 2) || (!is_call && !to_plt))
                        tcc_error("can't relocate value at %x,%d",addr, type);

                /* Compute and store final offset */
                s = (x >> 24) & 1;
                i1 = (x >> 23) & 1;
                i2 = (x >> 22) & 1;
                j1 = s ^ (i1 ^ 1);
                j2 = s ^ (i2 ^ 1);
                imm10 = (x >> 12) & 0x3ff;
                imm11 = (x >> 1) & 0x7ff;
                (*(uint16_t *)ptr) = (uint16_t) ((hi & 0xf800) |
                                     (s << 10) | imm10);
                (*(uint16_t *)(ptr+2)) = (uint16_t) ((lo & 0xc000) |
                                (j1 << 13) | blx_bit | (j2 << 11) |
                                imm11);
            }
            return;
        case R_ARM_MOVT_ABS:
        case R_ARM_MOVW_ABS_NC:
            {
                int x, imm4, imm12;
                if (type == R_ARM_MOVT_ABS)
                    val >>= 16;
                imm12 = val & 0xfff;
                imm4 = (val >> 12) & 0xf;
                x = (imm4 << 16) | imm12;
                if (type == R_ARM_THM_MOVT_ABS)
                    *(int *)ptr |= x;
                else
                    *(int *)ptr += x;
            }
            return;
        case R_ARM_THM_MOVT_ABS:
        case R_ARM_THM_MOVW_ABS_NC:
            {
                int x, i, imm4, imm3, imm8;
                if (type == R_ARM_THM_MOVT_ABS)
                    val >>= 16;
                imm8 = val & 0xff;
                imm3 = (val >> 8) & 0x7;
                i = (val >> 11) & 1;
                imm4 = (val >> 12) & 0xf;
                x = (imm3 << 28) | (imm8 << 16) | (i << 10) | imm4;
                if (type == R_ARM_THM_MOVT_ABS)
                    *(int *)ptr |= x;
                else
                    *(int *)ptr += x;
            }
            return;
        case R_ARM_PREL31:
            {
                int x;
                x = (*(int *)ptr) & 0x7fffffff;
                (*(int *)ptr) &= 0x80000000;
                x = (x * 2) / 2;
                x += val - addr;
                if((x^(x>>1))&0x40000000)
                    tcc_error("can't relocate value at %x,%d",addr, type);
                (*(int *)ptr) |= x & 0x7fffffff;
            }
        case R_ARM_ABS32:
            *(int *)ptr += val;
            return;
        case R_ARM_REL32:
            *(int *)ptr += val - addr;
            return;
        case R_ARM_GOTPC:
            *(int *)ptr += s1->got->sh_addr - addr;
            return;
        case R_ARM_GOTOFF:
            *(int *)ptr += val - s1->got->sh_addr;
            return;
        case R_ARM_GOT32:
            /* we load the got offset */
            *(int *)ptr += s1->sym_attrs[sym_index].got_offset;
            return;
        case R_ARM_COPY:
            return;
        case R_ARM_V4BX:
            /* trade Thumb support for ARMv4 support */
            if ((0x0ffffff0 & *(int*)ptr) == 0x012FFF10)
                *(int*)ptr ^= 0xE12FFF10 ^ 0xE1A0F000; /* BX Rm -> MOV PC, Rm */
            return;
        case R_ARM_GLOB_DAT:
        case R_ARM_JUMP_SLOT:
            *(addr_t *)ptr = val;
            return;
        case R_ARM_NONE:
            /* Nothing to do.  Normally used to indicate a dependency
               on a certain symbol (like for exception handling under EABI).  */
            return;
        default:
            fprintf(stderr,"FIXME: handle reloc type %x at %x [%p] to %x\n",
                type, (unsigned)addr, ptr, (unsigned)val);
            return;
    }
}
