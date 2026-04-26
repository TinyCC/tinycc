#ifdef TARGET_DEFS_ONLY

#define EM_TCC_TARGET EM_RISCV

#define R_DATA_32  R_RISCV_32
#define R_DATA_PTR R_RISCV_32
#define R_JMP_SLOT R_RISCV_JUMP_SLOT
#define R_GLOB_DAT R_RISCV_32
#define R_COPY     R_RISCV_COPY
#define R_RELATIVE R_RISCV_RELATIVE

#define R_NUM      R_RISCV_NUM

#define ELF_START_ADDR 0x00010000
#define ELF_PAGE_SIZE 0x1000

#define PCRELATIVE_DLLPLT 1
#define RELOCATE_DLLPLT 1

#else /* !TARGET_DEFS_ONLY */

//#define DEBUG_RELOC
#include "tcc.h"

/* Returns 1 for a code relocation, 0 for a data relocation. For unknown
   relocations, returns -1. */
ST_FUNC int code_reloc (int reloc_type)
{
    switch (reloc_type) {

    case R_RISCV_BRANCH:
    case R_RISCV_CALL:
    case R_RISCV_JAL:
        return 1;

    case R_RISCV_GOT_HI20:
    case R_RISCV_PCREL_HI20:
    case R_RISCV_PCREL_LO12_I:
    case R_RISCV_PCREL_LO12_S:
    case R_RISCV_32_PCREL:
    case R_RISCV_SET6:
    case R_RISCV_SET8:
    case R_RISCV_SET16:
    case R_RISCV_SUB6:
    case R_RISCV_ADD16:
    case R_RISCV_ADD32:
    case R_RISCV_SUB8:
    case R_RISCV_SUB16:
    case R_RISCV_SUB32:
    case R_RISCV_32:
    case R_RISCV_SET_ULEB128:
    case R_RISCV_SUB_ULEB128:
        return 0;

    case R_RISCV_CALL_PLT:
        return 1;
    }
    return -1;
}

/* Returns an enumerator to describe whether and when the relocation needs a
   GOT and/or PLT entry to be created. See tcc.h for a description of the
   different values. */
ST_FUNC int gotplt_entry_type (int reloc_type)
{
    switch (reloc_type) {
    case R_RISCV_ALIGN:
    case R_RISCV_RELAX:
    case R_RISCV_RVC_BRANCH:
    case R_RISCV_RVC_JUMP:
    case R_RISCV_JUMP_SLOT:
    case R_RISCV_SET6:
    case R_RISCV_SET8:
    case R_RISCV_SET16:
    case R_RISCV_SUB6:
    case R_RISCV_ADD16:
    case R_RISCV_SUB8:
    case R_RISCV_SUB16:
    case R_RISCV_SET_ULEB128:
    case R_RISCV_SUB_ULEB128:
        return NO_GOTPLT_ENTRY;

    case R_RISCV_BRANCH:
    case R_RISCV_CALL:
    case R_RISCV_PCREL_HI20:
    case R_RISCV_PCREL_LO12_I:
    case R_RISCV_PCREL_LO12_S:
    case R_RISCV_32_PCREL:
    case R_RISCV_ADD32:
    case R_RISCV_SUB32:
    case R_RISCV_32:
    case R_RISCV_JAL:
    case R_RISCV_CALL_PLT:
        return AUTO_GOTPLT_ENTRY;

    case R_RISCV_GOT_HI20:
        return ALWAYS_GOTPLT_ENTRY;
    }
    return -1;
}

ST_FUNC unsigned create_plt_entry(TCCState *s1, unsigned got_offset, struct sym_attr *attr)
{
    Section *plt = s1->plt;
    uint8_t *p;
    unsigned plt_offset;

    if (plt->data_offset == 0)
        section_ptr_add(plt, 32);
    plt_offset = plt->data_offset;

    p = section_ptr_add(plt, 16);
    write32le(p, got_offset);
    return plt_offset;
}

/* relocate the PLT: compute addresses and offsets in the PLT now that final
   address for PLT and GOT are known (see fill_program_header) */
ST_FUNC void relocate_plt(TCCState *s1)
{
    uint8_t *p, *p_end;

    if (!s1->plt)
      return;

    p = s1->plt->data;
    p_end = p + s1->plt->data_offset;

    if (p < p_end) {
        uint32_t plt = s1->plt->sh_addr;
        uint32_t got = s1->got->sh_addr;
        uint32_t off = (got - plt + 0x800) >> 12;
        if ((off + ((uint32_t)1 << 20)) >> 21)
            tcc_error_noabort("Failed relocating PLT (off=0x%lx, got=0x%lx, plt=0x%lx)", (long)off, (long)got, (long)plt);
        write32le(p, 0x397 | (off << 12)); // auipc t2, %pcrel_hi(got)
        write32le(p + 4, 0x41c30333); // sub t1, t1, t3
        write32le(p + 8, 0x0003ae03   // lw t3, %pcrel_lo(got)(t2)
                         | (((got - plt) & 0xfff) << 20));
        write32le(p + 12, 0xfd430313); // addi t1, t1, -(32+12)
        write32le(p + 16, 0x00038293   // addi t0, t2, %pcrel_lo(got)
                          | (((got - plt) & 0xfff) << 20));
        write32le(p + 20, 0x00235313); // srli t1, t1, log2(16/PTRSIZE) = 2
        write32le(p + 24, 0x0042a283); // lw t0, PTRSIZE(t0)
        write32le(p + 28, 0x000e0067); // jr t3
        p += 32;
        while (p < p_end) {
            uint32_t pc = plt + (p - s1->plt->data);
            uint32_t addr = got + read32le(p);
            uint32_t off = (addr - pc + 0x800) >> 12;
            if ((off + ((uint32_t)1 << 20)) >> 21)
                tcc_error_noabort("Failed relocating PLT (off=0x%lx, addr=0x%lx, pc=0x%lx)", (long)off, (long)addr, (long)pc);
            write32le(p, 0xe17 | (off << 12)); // auipc t3, %pcrel_hi(func@got)
            write32le(p + 4, 0x000e2e03 // lw t3, %pcrel_lo(func@got)(t3)
                             | (((addr - pc) & 0xfff) << 20));
            write32le(p + 8, 0x000e0367); // jalr t1, t3
            write32le(p + 12, 0x00000013); // nop
            p += 16;
        }
    }

    if (s1->plt->reloc) {
        ElfW_Rel *rel;
        p = s1->got->data;
        for_each_elem(s1->plt->reloc, 0, rel, ElfW_Rel) {
            write32le(p + rel->r_offset, s1->plt->sh_addr);
	}
    }
}

static void riscv32_record_pcrel_hi(TCCState *s1, addr_t addr, addr_t val)
{
    int n = s1->nb_pcrel_hi_entries;
    if (n >= s1->alloc_pcrel_hi_entries) {
        int new_alloc = s1->alloc_pcrel_hi_entries ? s1->alloc_pcrel_hi_entries * 2 : 64;
        s1->pcrel_hi_entries = tcc_realloc(s1->pcrel_hi_entries,
            new_alloc * sizeof(*s1->pcrel_hi_entries));
        s1->alloc_pcrel_hi_entries = new_alloc;
    }
    s1->pcrel_hi_entries[n].addr = addr;
    s1->pcrel_hi_entries[n].val = val;
    s1->nb_pcrel_hi_entries = n + 1;
    last_hi.addr = addr;
    last_hi.val = val;
}

static int riscv32_lookup_pcrel_hi(TCCState *s1, addr_t hi_addr, addr_t *hi_val)
{
    int i;
    struct pcrel_hi *entry;
    if (s1->nb_pcrel_hi_entries && hi_addr == last_hi.addr) {
        *hi_val = last_hi.val;
        return 1;
    }
    for (i = s1->nb_pcrel_hi_entries - 1; i >= 0; --i) {
        entry = &s1->pcrel_hi_entries[i];
        if (entry->addr == hi_addr) {
            last_hi = *entry;
            *hi_val = entry->val;
            return 1;
        }
    }
    return 0;
}

ST_FUNC void relocate(TCCState *s1, ElfW_Rel *rel, int type, unsigned char *ptr,
              addr_t addr, addr_t val)
{
    uint32_t off32;
    int sym_index = ELFW(R_SYM)(rel->r_info), esym_index;

    switch(type) {
    case R_RISCV_ALIGN:
    case R_RISCV_RELAX:
        return;

    case R_RISCV_BRANCH:
        off32 = val - addr;
        if ((off32 + (1 << 12)) & ~(uint32_t)0x1ffe)
          tcc_error_noabort("R_RISCV_BRANCH relocation failed"
                    " (val=%lx, addr=%lx)", (long)val, (long)addr);
        off32 >>= 1;
        write32le(ptr, (read32le(ptr) & ~0xfe000f80)
                       | ((off32 & 0x800) << 20)
                       | ((off32 & 0x3f0) << 21)
                       | ((off32 & 0x00f) << 8)
                       | ((off32 & 0x400) >> 3));
        return;
    case R_RISCV_JAL:
        off32 = val - addr;
        if ((off32 + (1 << 21)) & ~(((uint32_t)1 << 22) - 2))
          tcc_error_noabort("R_RISCV_JAL relocation failed"
                    " (val=%lx, addr=%lx)", (long)val, (long)addr);
        write32le(ptr, (read32le(ptr) & 0xfff)
                       | (((off32 >> 12) &  0xff) << 12)
                       | (((off32 >> 11) &     1) << 20)
                       | (((off32 >>  1) & 0x3ff) << 21)
                       | (((off32 >> 20) &     1) << 31));
        return;
    case R_RISCV_CALL:
    case R_RISCV_CALL_PLT:
        write32le(ptr, (read32le(ptr) & 0xfff)
                       | ((val - addr + 0x800) & ~0xfff));
        write32le(ptr + 4, (read32le(ptr + 4) & 0xfffff)
                           | (((val - addr) & 0xfff) << 20));
        return;
    case R_RISCV_PCREL_HI20:
#ifdef DEBUG_RELOC
        printf("PCREL_HI20: val=%lx addr=%lx\n", (long)val, (long)addr);
#endif
        off32 = (int32_t)(val - addr + 0x800) >> 12;
        write32le(ptr, (read32le(ptr) & 0xfff)
                       | ((off32 & 0xfffff) << 12));
        riscv32_record_pcrel_hi(s1, addr, val);
        return;
    case R_RISCV_GOT_HI20:
        val = s1->got->sh_addr + get_sym_attr(s1, sym_index, 0)->got_offset;
        off32 = (int32_t)(val - addr + 0x800) >> 12;
        write32le(ptr, (read32le(ptr) & 0xfff)
                       | ((off32 & 0xfffff) << 12));
        riscv32_record_pcrel_hi(s1, addr, val);
        return;
    case R_RISCV_PCREL_LO12_I:
#ifdef DEBUG_RELOC
        printf("PCREL_LO12_I: val=%lx addr=%lx\n", (long)val, (long)addr);
#endif
        addr = val;
        if (!riscv32_lookup_pcrel_hi(s1, addr, &val))
          tcc_error_noabort("unsupported hi/lo pcrel reloc scheme");
        write32le(ptr, (read32le(ptr) & 0xfffff)
                       | (((val - addr) & 0xfff) << 20));
        return;
    case R_RISCV_PCREL_LO12_S:
        addr = val;
        if (!riscv32_lookup_pcrel_hi(s1, addr, &val))
          tcc_error_noabort("unsupported hi/lo pcrel reloc scheme");
        off32 = val - addr;
        write32le(ptr, (read32le(ptr) & ~0xfe000f80)
                       | ((off32 & 0xfe0) << 20)
                       | ((off32 & 0x01f) << 7));
        return;

    case R_RISCV_RVC_BRANCH:
        off32 = (val - addr);
        if ((off32 + (1 << 8)) & ~(uint32_t)0x1fe)
          tcc_error_noabort("R_RISCV_RVC_BRANCH relocation failed"
                    " (val=%lx, addr=%lx)", (long)val, (long)addr);
        write16le(ptr, (read16le(ptr) & 0xe383)
                       | (((off32 >> 5) & 1) << 2)
                       | (((off32 >> 1) & 3) << 3)
                       | (((off32 >> 6) & 3) << 5)
                       | (((off32 >> 3) & 3) << 10)
                       | (((off32 >> 8) & 1) << 12));
        return;
    case R_RISCV_RVC_JUMP:
        off32 = (val - addr);
        if ((off32 + (1 << 11)) & ~(uint32_t)0xffe)
          tcc_error_noabort("R_RISCV_RVC_BRANCH relocation failed"
                    " (val=%lx, addr=%lx)", (long)val, (long)addr);
        write16le(ptr, (read16le(ptr) & 0xe003)
                       | (((off32 >>  5) & 1) << 2)
                       | (((off32 >>  1) & 7) << 3)
                       | (((off32 >>  7) & 1) << 6)
                       | (((off32 >>  6) & 1) << 7)
                       | (((off32 >> 10) & 1) << 8)
                       | (((off32 >>  8) & 3) << 9)
                       | (((off32 >>  4) & 1) << 11)
                       | (((off32 >> 11) & 1) << 12));
        return;

    case R_RISCV_32:
        if (s1->output_type & TCC_OUTPUT_DYN) {
            qrel->r_offset = rel->r_offset;
            qrel->r_info = ELFW(R_INFO)(0, R_RISCV_RELATIVE);
            qrel->r_addend = (int)read32le(ptr) + val;
            qrel++;
        }
        add32le(ptr, val);
        return;
    case R_RISCV_JUMP_SLOT:
        add32le(ptr, val);
        return;
    case R_RISCV_ADD32:
        write32le(ptr, read32le(ptr) + val);
        return;
    case R_RISCV_SUB32:
        write32le(ptr, read32le(ptr) - val);
        return;
    case R_RISCV_ADD16:
        write16le(ptr, read16le(ptr) + val);
        return;
    case R_RISCV_SUB8:
        *ptr -= val;
        return;
    case R_RISCV_SUB16:
        write16le(ptr, read16le(ptr) - val);
        return;
    case R_RISCV_SET6:
        *ptr = (*ptr & ~0x3f) | (val & 0x3f);
        return;
    case R_RISCV_SET8:
        *ptr = (*ptr & ~0xff) | (val & 0xff);
        return;
    case R_RISCV_SET16:
        write16le(ptr, val);
        return;
    case R_RISCV_SUB6:
        *ptr = (*ptr & ~0x3f) | ((*ptr - val) & 0x3f);
        return;
    case R_RISCV_32_PCREL:
        if (s1->output_type & TCC_OUTPUT_DYN) {
	    /* DLL relocation */
	    esym_index = get_sym_attr(s1, sym_index, 0)->dyn_index;
	    if (esym_index) {
                qrel->r_offset = rel->r_offset;
                qrel->r_info = ELFW(R_INFO)(esym_index, R_RISCV_32_PCREL);
                qrel->r_addend = (int)read32le(ptr) + rel->r_addend;
                qrel++;
		break;
	    }
        }
	add32le(ptr, val - addr);
        return;
    case R_RISCV_SET_ULEB128:
    case R_RISCV_SUB_ULEB128:
	/* ignore. used in section .debug_loclists */
        return;
    case R_RISCV_COPY:
        /* XXX */
        return;

    default:
        fprintf(stderr, "FIXME: handle reloc type %x at %x [%p] to %x\n",
                type, (unsigned)addr, ptr, (unsigned)val);
        return;
    }
}
#endif
