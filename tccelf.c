/*
 *  ELF file handling for TCC
 * 
 *  Copyright (c) 2001, 2002 Fabrice Bellard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

static int put_elf_str(Section *s, const char *sym)
{
    int offset, len;
    char *ptr;

    len = strlen(sym) + 1;
    offset = s->data_offset;
    ptr = section_ptr_add(s, len);
    memcpy(ptr, sym, len);
    return offset;
}

/* elf symbol hashing function */
static unsigned long elf_hash(const unsigned char *name)
{
    unsigned long h = 0, g;
    
    while (*name) {
        h = (h << 4) + *name++;
        g = h & 0xf0000000;
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

/* rebuild hash table of section s */
/* NOTE: we do factorize the hash table code to go faster */
static void rebuild_hash(Section *s, unsigned int nb_buckets)
{
    Elf32_Sym *sym;
    int *ptr, *hash, nb_syms, sym_index, h;
    char *strtab;

    strtab = s->link->data;
    nb_syms = s->data_offset / sizeof(Elf32_Sym);

    s->hash->data_offset = 0;
    ptr = section_ptr_add(s->hash, (2 + nb_buckets + nb_syms) * sizeof(int));
    ptr[0] = nb_buckets;
    ptr[1] = nb_syms;
    ptr += 2;
    hash = ptr;
    memset(hash, 0, (nb_buckets + 1) * sizeof(int));
    ptr += nb_buckets + 1;

    sym = (Elf32_Sym *)s->data + 1;
    for(sym_index = 1; sym_index < nb_syms; sym_index++) {
        if (ELF32_ST_BIND(sym->st_info) != STB_LOCAL) {
            h = elf_hash(strtab + sym->st_name) % nb_buckets;
            *ptr = hash[h];
            hash[h] = sym_index;
        } else {
            *ptr = 0;
        }
        ptr++;
        sym++;
    }
}

/* return the symbol number */
static int put_elf_sym(Section *s, 
                       unsigned long value, unsigned long size,
                       int info, int other, int shndx, const char *name)
{
    int name_offset, sym_index;
    int nbuckets, h;
    Elf32_Sym *sym;
    Section *hs;
    
    sym = section_ptr_add(s, sizeof(Elf32_Sym));
    if (name)
        name_offset = put_elf_str(s->link, name);
    else
        name_offset = 0;
    /* XXX: endianness */
    sym->st_name = name_offset;
    sym->st_value = value;
    sym->st_size = size;
    sym->st_info = info;
    sym->st_other = other;
    sym->st_shndx = shndx;
    sym_index = sym - (Elf32_Sym *)s->data;
    hs = s->hash;
    if (hs) {
        int *ptr, *base;
        ptr = section_ptr_add(hs, sizeof(int));
        base = (int *)hs->data;
        /* only add global or weak symbols */
        if (ELF32_ST_BIND(info) != STB_LOCAL) {
            /* add another hashing entry */
            nbuckets = base[0];
            h = elf_hash(name) % nbuckets;
            *ptr = base[2 + h];
            base[2 + h] = sym_index;
            base[1]++;
            /* we resize the hash table */
            hs->nb_hashed_syms++;
            if (hs->nb_hashed_syms > 2 * nbuckets) {
                rebuild_hash(s, 2 * nbuckets);
            }
        } else {
            *ptr = 0;
            base[1]++;
        }
    }
    return sym_index;
}

/* find global ELF symbol 'name' and return its index. Return 0 if not
   found. */
static int find_elf_sym(Section *s, const char *name)
{
    Elf32_Sym *sym;
    Section *hs;
    int nbuckets, sym_index, h;
    const char *name1;
    
    hs = s->hash;
    if (!hs)
        return 0;
    nbuckets = ((int *)hs->data)[0];
    h = elf_hash(name) % nbuckets;
    sym_index = ((int *)hs->data)[2 + h];
    while (sym_index != 0) {
        sym = &((Elf32_Sym *)s->data)[sym_index];
        name1 = s->link->data + sym->st_name;
        if (!strcmp(name, name1))
            return sym_index;
        sym_index = ((int *)hs->data)[2 + nbuckets + sym_index];
    }
    return 0;
}

/* return elf symbol value or error */
static unsigned long get_elf_sym_val(const char *name)
{
    int sym_index;
    Elf32_Sym *sym;
    
    sym_index = find_elf_sym(symtab_section, name);
    if (!sym_index)
        error("%s not defined", name);
    sym = &((Elf32_Sym *)symtab_section->data)[sym_index];
    return sym->st_value;
}

/* add an elf symbol : check if it is already defined and patch
   it. Return symbol index. NOTE that sh_num can be SHN_UNDEF. */
static int add_elf_sym(Section *s, unsigned long value, unsigned long size,
                       int info, int sh_num, const char *name)
{
    Elf32_Sym *esym;
    int sym_bind, sym_index, sym_type, esym_bind;

    sym_bind = ELF32_ST_BIND(info);
    sym_type = ELF32_ST_TYPE(info);
        
    if (sym_bind != STB_LOCAL) {
        /* we search global or weak symbols */
        sym_index = find_elf_sym(s, name);
        if (!sym_index)
            goto do_def;
        esym = &((Elf32_Sym *)s->data)[sym_index];
        if (esym->st_shndx != SHN_UNDEF) {
            esym_bind = ELF32_ST_BIND(esym->st_info);
            if (sh_num == SHN_UNDEF) {
                /* ignore adding of undefined symbol if the
                   corresponding symbol is already defined */
            } else if (sym_bind == STB_GLOBAL && esym_bind == STB_WEAK) {
                /* global overrides weak, so patch */
                goto do_patch;
            } else if (sym_bind == STB_WEAK && esym_bind == STB_GLOBAL) {
                /* weak is ignored if already global */
            } else {
#if 0
                printf("new_bind=%d new_shndx=%d last_bind=%d old_shndx=%d\n",
                       sym_bind, sh_num, esym_bind, esym->st_shndx);
#endif
                /* NOTE: we accept that two DLL define the same symbol */
                if (s != dynsymtab_section)
                    error("'%s' defined twice", name);
            }
        } else {
        do_patch:
            esym->st_info = ELF32_ST_INFO(sym_bind, sym_type);
            esym->st_shndx = sh_num;
            esym->st_value = value;
            esym->st_size = size;
        }
    } else {
    do_def:
        sym_index = put_elf_sym(s, value, size, 
                                ELF32_ST_INFO(sym_bind, sym_type), 0, 
                                sh_num, name);
    }
    return sym_index;
}

/* put relocation */
static void put_elf_reloc(Section *symtab, Section *s, unsigned long offset,
                          int type, int symbol)
{
    char buf[256];
    Section *sr;
    Elf32_Rel *rel;

    sr = s->reloc;
    if (!sr) {
        /* if no relocation section, create it */
        snprintf(buf, sizeof(buf), ".rel%s", s->name);
        /* if the symtab is allocated, then we consider the relocation
           are also */
        sr = new_section(buf, SHT_REL, symtab->sh_flags);
        sr->sh_entsize = sizeof(Elf32_Rel);
        sr->link = symtab;
        sr->sh_info = s->sh_num;
        s->reloc = sr;
    }
    rel = section_ptr_add(sr, sizeof(Elf32_Rel));
    rel->r_offset = offset;
    rel->r_info = ELF32_R_INFO(symbol, type);
}

/* put stab debug information */

typedef struct {
    unsigned long n_strx;         /* index into string table of name */
    unsigned char n_type;         /* type of symbol */
    unsigned char n_other;        /* misc info (usually empty) */
    unsigned short n_desc;        /* description field */
    unsigned long n_value;        /* value of symbol */
} Stab_Sym;

static void put_stabs(const char *str, int type, int other, int desc, 
                      unsigned long value)
{
    Stab_Sym *sym;

    sym = section_ptr_add(stab_section, sizeof(Stab_Sym));
    if (str) {
        sym->n_strx = put_elf_str(stabstr_section, str);
    } else {
        sym->n_strx = 0;
    }
    sym->n_type = type;
    sym->n_other = other;
    sym->n_desc = desc;
    sym->n_value = value;
}

static void put_stabs_r(const char *str, int type, int other, int desc, 
                        unsigned long value, Section *sec, int sym_index)
{
    put_stabs(str, type, other, desc, value);
    put_elf_reloc(symtab_section, stab_section, 
                  stab_section->data_offset - sizeof(unsigned long),
                  R_DATA_32, sym_index);
}

static void put_stabn(int type, int other, int desc, int value)
{
    put_stabs(NULL, type, other, desc, value);
}

static void put_stabd(int type, int other, int desc)
{
    put_stabs(NULL, type, other, desc, 0);
}

/* In an ELF file symbol table, the local symbols must appear below
   the global and weak ones. Since TCC cannot sort it while generating
   the code, we must do it after. All the relocation tables are also
   modified to take into account the symbol table sorting */
static void sort_syms(Section *s)
{
    int *old_to_new_syms;
    Elf32_Sym *new_syms;
    int nb_syms, i;
    Elf32_Sym *p, *q;
    Elf32_Rel *rel, *rel_end;
    Section *sr;
    int type, sym_index;

    nb_syms = s->data_offset / sizeof(Elf32_Sym);
    new_syms = tcc_malloc(nb_syms * sizeof(Elf32_Sym));
    old_to_new_syms = tcc_malloc(nb_syms * sizeof(int));

    /* first pass for local symbols */
    p = (Elf32_Sym *)s->data;
    q = new_syms;
    for(i = 0; i < nb_syms; i++) {
        if (ELF32_ST_BIND(p->st_info) == STB_LOCAL) {
            old_to_new_syms[i] = q - new_syms;
            *q++ = *p;
        }
        p++;
    }
    /* save the number of local symbols in section header */
    s->sh_info = q - new_syms;

    /* then second pass for non local symbols */
    p = (Elf32_Sym *)s->data;
    for(i = 0; i < nb_syms; i++) {
        if (ELF32_ST_BIND(p->st_info) != STB_LOCAL) {
            old_to_new_syms[i] = q - new_syms;
            *q++ = *p;
        }
        p++;
    }
    
    /* we copy the new symbols to the old */
    memcpy(s->data, new_syms, nb_syms * sizeof(Elf32_Sym));
    tcc_free(new_syms);

    /* now we modify all the relocations */
    for(i = 1; i < nb_sections; i++) {
        sr = sections[i];
        if (sr->sh_type == SHT_REL && sr->link == s) {
            rel_end = (Elf32_Rel *)(sr->data + sr->data_offset);
            for(rel = (Elf32_Rel *)sr->data;
                rel < rel_end;
                rel++) {
                sym_index = ELF32_R_SYM(rel->r_info);
                type = ELF32_R_TYPE(rel->r_info);
                sym_index = old_to_new_syms[sym_index];
                rel->r_info = ELF32_R_INFO(sym_index, type);
            }
        }
    }
    
    tcc_free(old_to_new_syms);
}

/* relocate common symbols in the .bss section */
static void relocate_common_syms(void)
{
    Elf32_Sym *sym, *sym_end;
    unsigned long offset, align;
    
    sym_end = (Elf32_Sym *)(symtab_section->data + symtab_section->data_offset);
    for(sym = (Elf32_Sym *)symtab_section->data + 1; 
        sym < sym_end;
        sym++) {
        if (sym->st_shndx == SHN_COMMON) {
            /* align symbol */
            align = sym->st_value;
            offset = bss_section->data_offset;
            offset = (offset + align - 1) & -align;
            sym->st_value = offset;
            sym->st_shndx = bss_section->sh_num;
            offset += sym->st_size;
            bss_section->data_offset = offset;
        }
    }
}

static void *resolve_sym(const char *sym)
{
    return dlsym(NULL, sym);
}

/* relocate symbol table, resolve undefined symbols if do_resolve is
   true and output error if undefined symbol. */
static void relocate_syms(int do_resolve)
{
    Elf32_Sym *sym, *esym, *sym_end;
    int sym_bind, sh_num, sym_index;
    const char *name;
    unsigned long addr;

    sym_end = (Elf32_Sym *)(symtab_section->data + symtab_section->data_offset);
    for(sym = (Elf32_Sym *)symtab_section->data + 1; 
        sym < sym_end;
        sym++) {
        sh_num = sym->st_shndx;
        if (sh_num == SHN_UNDEF) {
            name = strtab_section->data + sym->st_name;
            if (do_resolve) {
                name = symtab_section->link->data + sym->st_name;
                addr = (unsigned long)resolve_sym(name);
                if (addr) {
                    sym->st_value = addr;
                    goto found;
                }
            } else if (dynsym) {
                /* if dynamic symbol exist, then use it */
                sym_index = find_elf_sym(dynsym, name);
                if (sym_index) {
                    esym = &((Elf32_Sym *)dynsym->data)[sym_index];
                    sym->st_value = esym->st_value;
                    goto found;
                }
            }
            /* XXX: _fp_hw seems to be part of the ABI, so we ignore
               it */
            if (!strcmp(name, "_fp_hw"))
                goto found;
            /* only weak symbols are accepted to be undefined. Their
               value is zero */
            sym_bind = ELF32_ST_BIND(sym->st_info);
            if (sym_bind == STB_WEAK) {
                sym->st_value = 0;
            } else {
                error("undefined symbol '%s'", name);
            }
        } else if (sh_num < SHN_LORESERVE) {
            /* add section base */
            sym->st_value += sections[sym->st_shndx]->sh_addr;
        }
    found: ;
    }
}

/* relocate a given section (CPU dependant) */
static void relocate_section(TCCState *s1, Section *s)
{
    Section *sr;
    Elf32_Rel *rel, *rel_end, *qrel;
    Elf32_Sym *sym;
    int type, sym_index, esym_index;
    unsigned char *ptr;
    unsigned long val, addr;

    sr = s->reloc;
    rel_end = (Elf32_Rel *)(sr->data + sr->data_offset);
    qrel = (Elf32_Rel *)sr->data;
    for(rel = qrel;
        rel < rel_end;
        rel++) {
        ptr = s->data + rel->r_offset;

        sym_index = ELF32_R_SYM(rel->r_info);
        sym = &((Elf32_Sym *)symtab_section->data)[sym_index];
        val = sym->st_value;
        type = ELF32_R_TYPE(rel->r_info);
        addr = s->sh_addr + rel->r_offset;

        /* CPU specific */
        switch(type) {
        case R_386_32:
            if (s1->output_type == TCC_OUTPUT_DLL) {
                esym_index = symtab_to_dynsym[sym_index];
                qrel->r_offset = rel->r_offset;
                if (esym_index) {
                    qrel->r_info = ELF32_R_INFO(esym_index, R_386_32);
                    qrel++;
                    break;
                } else {
                    qrel->r_info = ELF32_R_INFO(0, R_386_RELATIVE);
                    qrel++;
                }
            }
            *(int *)ptr += val;
            break;
        case R_386_PC32:
            if (s1->output_type == TCC_OUTPUT_DLL) {
                /* DLL relocation */
                esym_index = symtab_to_dynsym[sym_index];
                if (esym_index) {
                    qrel->r_offset = rel->r_offset;
                    qrel->r_info = ELF32_R_INFO(esym_index, R_386_PC32);
                    qrel++;
                    break;
                }
            }
            *(int *)ptr += val - addr;
            break;
        case R_386_PLT32:
            *(int *)ptr += val - addr;
            break;
        case R_386_GLOB_DAT:
        case R_386_JMP_SLOT:
            *(int *)ptr = val;
            break;
        case R_386_GOTPC:
            *(int *)ptr += got->sh_addr - addr;
            break;
        case R_386_GOTOFF:
            *(int *)ptr += val - got->sh_addr;
            break;
        case R_386_GOT32:
            /* we load the got offset */
            *(int *)ptr += got_offsets[sym_index];
            break;
        }
    }
    /* if the relocation is allocated, we change its symbol table */
    if (sr->sh_flags & SHF_ALLOC)
        sr->link = dynsym;
}

/* relocate relocation table in 'sr' */
static void relocate_rel(Section *sr)
{
    Section *s;
    Elf32_Rel *rel, *rel_end;
    
    s = sections[sr->sh_info];
    rel_end = (Elf32_Rel *)(sr->data + sr->data_offset);
    for(rel = (Elf32_Rel *)sr->data;
        rel < rel_end;
        rel++) {
        rel->r_offset += s->sh_addr;
    }
}

/* count the number of dynamic relocations so that we can reserve
   their space */
static int prepare_dynamic_rel(Section *sr)
{
    Elf32_Rel *rel, *rel_end;
    int sym_index, esym_index, type, count;

    count = 0;
    rel_end = (Elf32_Rel *)(sr->data + sr->data_offset);
    for(rel = (Elf32_Rel *)sr->data; rel < rel_end; rel++) {
        sym_index = ELF32_R_SYM(rel->r_info);
        type = ELF32_R_TYPE(rel->r_info);
        switch(type) {
        case R_386_32:
            count++;
            break;
        case R_386_PC32:
            esym_index = symtab_to_dynsym[sym_index];
            if (esym_index)
                count++;
            break;
        default:
            break;
        }
    }
    if (count) {
        /* allocate the section */
        sr->sh_flags |= SHF_ALLOC;
        sr->sh_size = count * sizeof(Elf32_Rel);
    }
    return count;
}

static void put_got_offset(int index, unsigned long val)
{
    int n;
    unsigned long *tab;

    if (index >= nb_got_offsets) {
        /* find immediately bigger power of 2 and reallocate array */
        n = 1;
        while (index >= n)
            n *= 2;
        tab = tcc_realloc(got_offsets, n * sizeof(unsigned long));
        if (!tab)
            error("memory full");
        got_offsets = tab;
        memset(got_offsets + nb_got_offsets, 0,
               (n - nb_got_offsets) * sizeof(unsigned long));
        nb_got_offsets = n;
    }
    got_offsets[index] = val;
}

/* XXX: suppress that */
static void put32(unsigned char *p, unsigned int val)
{
    p[0] = val;
    p[1] = val >> 8;
    p[2] = val >> 16;
    p[3] = val >> 24;
}

static void build_got(void)
{
    unsigned char *ptr;

    /* if no got, then create it */
    got = new_section(".got", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
    got->sh_entsize = 4;
    add_elf_sym(symtab_section, 0, 4, ELF32_ST_INFO(STB_GLOBAL, STT_OBJECT), 
                got->sh_num, "_GLOBAL_OFFSET_TABLE_");
    ptr = section_ptr_add(got, 3 * sizeof(int));
    /* keep space for _DYNAMIC pointer, if present */
    put32(ptr, 0);
    /* two dummy got entries */
    put32(ptr + 4, 0);
    put32(ptr + 8, 0);
}

/* put a got entry corresponding to a symbol in symtab_section. 'size'
   and 'info' can be modifed if more precise info comes from the DLL */
static void put_got_entry(int reloc_type, unsigned long size, int info, 
                          int sym_index)
{
    int index;
    const char *name;
    Elf32_Sym *sym;
    unsigned long offset;
    int *ptr;

    if (!got)
        build_got();

    /* if a got entry already exists for that symbol, no need to add one */
    if (sym_index < nb_got_offsets &&
        got_offsets[sym_index] != 0)
        return;
    
    put_got_offset(sym_index, got->data_offset);

    if (dynsym) {
        sym = &((Elf32_Sym *)symtab_section->data)[sym_index];
        name = symtab_section->link->data + sym->st_name;
        offset = sym->st_value;
        /* NOTE: we put temporarily the got offset */
        if (reloc_type == R_386_JMP_SLOT) {
            nb_plt_entries++;
            offset = got->data_offset;
        }
        index = put_elf_sym(dynsym, offset, 
                            size, info, 0, sym->st_shndx, name);
        /* put a got entry */
        put_elf_reloc(dynsym, got, 
                      got->data_offset, 
                      reloc_type, index);
    }
    ptr = section_ptr_add(got, sizeof(int));
    *ptr = 0;
}

/* build GOT and PLT entries */
static void build_got_entries(void)
{
    Section *s, *symtab;
    Elf32_Rel *rel, *rel_end;
    Elf32_Sym *sym;
    int i, type, reloc_type, sym_index;

    for(i = 1; i < nb_sections; i++) {
        s = sections[i];
        if (s->sh_type != SHT_REL)
            continue;
        /* no need to handle got relocations */
        if (s->link != symtab_section)
            continue;
        symtab = s->link;
        rel_end = (Elf32_Rel *)(s->data + s->data_offset);
        for(rel = (Elf32_Rel *)s->data;
            rel < rel_end;
            rel++) {
            type = ELF32_R_TYPE(rel->r_info);
            switch(type) {
            case R_386_GOT32:
            case R_386_GOTOFF:
            case R_386_GOTPC:
            case R_386_PLT32:
                if (!got)
                    build_got();
                if (type == R_386_GOT32 || type == R_386_PLT32) {
                    sym_index = ELF32_R_SYM(rel->r_info);
                    sym = &((Elf32_Sym *)symtab_section->data)[sym_index];
                    /* look at the symbol got offset. If none, then add one */
                    if (type == R_386_GOT32)
                        reloc_type = R_386_GLOB_DAT;
                    else
                        reloc_type = R_386_JMP_SLOT;
                    put_got_entry(reloc_type, sym->st_size, sym->st_info, 
                                  sym_index);
                }
                break;
            default:
                break;
            }
        }
    }
}

static Section *new_symtab(const char *symtab_name, int sh_type, int sh_flags,
                           const char *strtab_name, 
                           const char *hash_name, int hash_sh_flags)
{
    Section *symtab, *strtab, *hash;
    int *ptr, nb_buckets;

    symtab = new_section(symtab_name, sh_type, sh_flags);
    symtab->sh_entsize = sizeof(Elf32_Sym);
    strtab = new_section(strtab_name, SHT_STRTAB, sh_flags);
    put_elf_str(strtab, "");
    symtab->link = strtab;
    put_elf_sym(symtab, 0, 0, 0, 0, 0, NULL);
    
    nb_buckets = 1;

    hash = new_section(hash_name, SHT_HASH, hash_sh_flags);
    hash->sh_entsize = sizeof(int);
    symtab->hash = hash;
    hash->link = symtab;

    ptr = section_ptr_add(hash, (2 + nb_buckets + 1) * sizeof(int));
    ptr[0] = nb_buckets;
    ptr[1] = 1;
    memset(ptr + 2, 0, (nb_buckets + 1) * sizeof(int));
    return symtab;
}

/* put dynamic tag */
static void put_dt(Section *dynamic, int dt, unsigned long val)
{
    Elf32_Dyn *dyn;
    dyn = section_ptr_add(dynamic, sizeof(Elf32_Dyn));
    dyn->d_tag = dt;
    dyn->d_un.d_val = val;
}

/* add tcc runtime libraries */
static void tcc_add_runtime(TCCState *s1)
{
    char buf[1024];
    int i;
    Section *s;

    snprintf(buf, sizeof(buf), "%s/%s", tcc_lib_path, "libtcc1.o");
    tcc_add_file(s1, buf);
#ifdef CONFIG_TCC_BCHECK
    if (do_bounds_check) {
        unsigned long *ptr;
        Section *init_section;
        unsigned char *pinit;
        int sym_index;

        /* XXX: add an object file to do that */
        ptr = section_ptr_add(bounds_section, sizeof(unsigned long));
        *ptr = 0;
        add_elf_sym(symtab_section, 0, 0, 
                    ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                    bounds_section->sh_num, "__bounds_start");
        /* add bound check code */
        snprintf(buf, sizeof(buf), "%s/%s", tcc_lib_path, "bcheck.o");
        tcc_add_file(s1, buf);
#ifdef TCC_TARGET_I386
        if (s1->output_type != TCC_OUTPUT_MEMORY) {
            /* add 'call __bound_init()' in .init section */
            init_section = find_section(".init");
            pinit = section_ptr_add(init_section, 5);
            pinit[0] = 0xe8;
            put32(pinit + 1, -4);
            sym_index = find_elf_sym(symtab_section, "__bound_init");
            put_elf_reloc(symtab_section, init_section, 
                          init_section->data_offset - 4, R_386_PC32, sym_index);
        }
#endif
    }
#endif
    /* add libc if not memory output */
    if (s1->output_type != TCC_OUTPUT_MEMORY) {
        tcc_add_library(s1, "c");
        tcc_add_file(s1, CONFIG_TCC_CRT_PREFIX "/crtn.o");
    }
    /* add various standard linker symbols */
    add_elf_sym(symtab_section, 
                text_section->data_offset, 0,
                ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                text_section->sh_num, "_etext");
    add_elf_sym(symtab_section, 
                data_section->data_offset, 0,
                ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                data_section->sh_num, "_edata");
    add_elf_sym(symtab_section, 
                bss_section->data_offset, 0,
                ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                bss_section->sh_num, "_end");
    /* add start and stop symbols for sections whose name can be
       expressed in C */
    for(i = 1; i < nb_sections; i++) {
        s = sections[i];
        if (s->sh_type == SHT_PROGBITS &&
            (s->sh_flags & SHF_ALLOC)) {
            const char *p;
            int ch;

            /* check if section name can be expressed in C */
            p = s->name;
            for(;;) {
                ch = *p;
                if (!ch)
                    break;
                if (!isid(ch) && !isnum(ch))
                    goto next_sec;
                p++;
            }
            snprintf(buf, sizeof(buf), "__start_%s", s->name);
            add_elf_sym(symtab_section, 
                        0, 0,
                        ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                        s->sh_num, buf);
            snprintf(buf, sizeof(buf), "__stop_%s", s->name);
            add_elf_sym(symtab_section,
                        s->data_offset, 0,
                        ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                        s->sh_num, buf);
        }
    next_sec: ;
    }
}

/* name of ELF interpreter */
static char elf_interp[] = "/lib/ld-linux.so.2";

#define ELF_START_ADDR 0x08048000
#define ELF_PAGE_SIZE  0x1000

/* output an ELF file */
/* XXX: suppress unneeded sections */
int tcc_output_file(TCCState *s1, const char *filename)
{
    Elf32_Ehdr ehdr;
    FILE *f;
    int fd, mode;
    int *section_order;
    int shnum, i, phnum, file_offset, offset, size, j, tmp, sh_order_index, k;
    unsigned long addr;
    Section *strsec, *s;
    Elf32_Shdr shdr, *sh;
    Elf32_Phdr *phdr, *ph;
    Section *interp, *plt, *dynamic, *dynstr;
    unsigned long saved_dynamic_data_offset;
    Elf32_Sym *sym;
    int type, file_type;
    unsigned long rel_addr, rel_size;
    
    file_type = s1->output_type;

    if (file_type != TCC_OUTPUT_OBJ)
        tcc_add_runtime(s1);

    interp = NULL;
    dynamic = NULL;
    dynsym = NULL;
    got = NULL;
    nb_plt_entries = 0;
    plt = NULL; /* avoid warning */
    dynstr = NULL; /* avoid warning */
    saved_dynamic_data_offset = 0; /* avoid warning */

    if (file_type != TCC_OUTPUT_OBJ) {

        relocate_common_syms();

        if (!static_link) {
            const char *name;
            int sym_index, index;
            Elf32_Sym *esym, *sym_end;
            
            if (file_type == TCC_OUTPUT_EXE) {
                char *ptr;
                /* add interpreter section only if executable */
                interp = new_section(".interp", SHT_PROGBITS, SHF_ALLOC);
                interp->sh_addralign = 1;
                ptr = section_ptr_add(interp, sizeof(elf_interp));
                strcpy(ptr, elf_interp);
            }
        
            /* add dynamic symbol table */
            dynsym = new_symtab(".dynsym", SHT_DYNSYM, SHF_ALLOC,
                                ".dynstr", 
                                ".hash", SHF_ALLOC);
            dynstr = dynsym->link;
            
            /* add dynamic section */
            dynamic = new_section(".dynamic", SHT_DYNAMIC, 
                                  SHF_ALLOC | SHF_WRITE);
            dynamic->link = dynstr;
            dynamic->sh_entsize = sizeof(Elf32_Dyn);
        
            /* add PLT */
            plt = new_section(".plt", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
            plt->sh_entsize = 4;

            build_got();

            /* scan for undefined symbols and see if they are in the
               dynamic symbols. If a symbol STT_FUNC is found, then we
               add it in the PLT. If a symbol STT_OBJECT is found, we
               add it in the .bss section with a suitable relocation */
            sym_end = (Elf32_Sym *)(symtab_section->data + 
                                    symtab_section->data_offset);
            if (file_type == TCC_OUTPUT_EXE) {
                for(sym = (Elf32_Sym *)symtab_section->data + 1; 
                    sym < sym_end;
                    sym++) {
                    if (sym->st_shndx == SHN_UNDEF) {
                        name = symtab_section->link->data + sym->st_name;
                        sym_index = find_elf_sym(dynsymtab_section, name);
                        if (sym_index) {
                            esym = &((Elf32_Sym *)dynsymtab_section->data)[sym_index];
                            type = ELF32_ST_TYPE(esym->st_info);
                            if (type == STT_FUNC) {
                                put_got_entry(R_386_JMP_SLOT, esym->st_size, 
                                              esym->st_info, 
                                              sym - (Elf32_Sym *)symtab_section->data);
                            } else if (type == STT_OBJECT) {
                                unsigned long offset;
                                offset = bss_section->data_offset;
                                /* XXX: which alignment ? */
                                offset = (offset + 8 - 1) & -8;
                                index = put_elf_sym(dynsym, offset, esym->st_size, 
                                                    esym->st_info, 0, 
                                                    bss_section->sh_num, name);
                                put_elf_reloc(dynsym, bss_section, 
                                              offset, R_386_COPY, index);
                                offset += esym->st_size;
                                bss_section->data_offset = offset;
                            }
                        } else {
                                /* STB_WEAK undefined symbols are accepted */
                                /* XXX: _fp_hw seems to be part of the ABI, so we ignore
                                   it */
                            if (ELF32_ST_BIND(sym->st_info) == STB_WEAK ||
                                !strcmp(name, "_fp_hw")) {
                            } else {
                                error("undefined symbol '%s'", name);
                            }
                        }
                    }
                }
            
                /* now look at unresolved dynamic symbols and export
                   corresponding symbol */
                sym_end = (Elf32_Sym *)(dynsymtab_section->data + 
                                        dynsymtab_section->data_offset);
                for(esym = (Elf32_Sym *)dynsymtab_section->data + 1; 
                    esym < sym_end;
                    esym++) {
                    if (esym->st_shndx == SHN_UNDEF) {
                        name = dynsymtab_section->link->data + esym->st_name;
                        sym_index = find_elf_sym(symtab_section, name);
                        if (sym_index) {
                            sym = &((Elf32_Sym *)symtab_section->data)[sym_index];
                            put_elf_sym(dynsym, sym->st_value, sym->st_size, 
                                        sym->st_info, 0, 
                                        sym->st_shndx, name);
                        } else {
                            if (ELF32_ST_BIND(esym->st_info) == STB_WEAK) {
                                /* weak symbols can stay undefined */
                            } else {
                                warning("undefined dynamic symbol '%s'", name);
                            }
                        }
                    }
                }
            } else {
                int nb_syms;
                /* shared library case : we simply export all the global symbols */
                nb_syms = symtab_section->data_offset / sizeof(Elf32_Sym);
                symtab_to_dynsym = tcc_mallocz(sizeof(int) * nb_syms);
                for(sym = (Elf32_Sym *)symtab_section->data + 1; 
                    sym < sym_end;
                    sym++) {
                    if (ELF32_ST_BIND(sym->st_info) != STB_LOCAL) {
                        name = symtab_section->link->data + sym->st_name;
                        index = put_elf_sym(dynsym, sym->st_value, sym->st_size, 
                                            sym->st_info, 0, 
                                            sym->st_shndx, name);
                        symtab_to_dynsym[sym - (Elf32_Sym *)symtab_section->data] = 
                            index;
                    }
                }
            }

            build_got_entries();
        
            /* update PLT/GOT sizes so that we can allocate their space */
            plt->data_offset += 16 * (nb_plt_entries + 1);

            /* add a list of needed dlls */
            for(i = 0; i < nb_loaded_dlls; i++) {
                DLLReference *dllref = loaded_dlls[i];
                if (dllref->level == 0)
                    put_dt(dynamic, DT_NEEDED, put_elf_str(dynstr, dllref->name));
            }
            /* XXX: currently, since we do not handle PIC code, we
               must relocate the readonly segments */
            if (file_type == TCC_OUTPUT_DLL)
                put_dt(dynamic, DT_TEXTREL, 0);

            /* add necessary space for other entries */
            saved_dynamic_data_offset = dynamic->data_offset;
            dynamic->data_offset += 8 * 9;
        } else {
            /* still need to build got entries in case of static link */
            build_got_entries();
        }
    }

    memset(&ehdr, 0, sizeof(ehdr));

    /* we add a section for symbols */
    strsec = new_section(".shstrtab", SHT_STRTAB, 0);
    put_elf_str(strsec, "");
    
    /* compute number of sections */
    shnum = nb_sections;

    /* this array is used to reorder sections in the output file */
    section_order = tcc_malloc(sizeof(int) * shnum);
    section_order[0] = 0;
    sh_order_index = 1;
    
    /* compute number of program headers */
    switch(file_type) {
    default:
    case TCC_OUTPUT_OBJ:
        phnum = 0;
        break;
    case TCC_OUTPUT_EXE:
        if (!static_link)
            phnum = 4;
        else
            phnum = 2;
        break;
    case TCC_OUTPUT_DLL:
        phnum = 3;
        break;
    }

    /* allocate strings for section names and decide if an unallocated
       section should be output */
    /* NOTE: the strsec section comes last, so its size is also
       correct ! */
    for(i = 1; i < nb_sections; i++) {
        s = sections[i];
        s->sh_name = put_elf_str(strsec, s->name);
        /* when generating a DLL, we include relocations but we may
           patch them */
        if (file_type == TCC_OUTPUT_DLL && 
            s->sh_type == SHT_REL && 
            !(s->sh_flags & SHF_ALLOC)) {
            prepare_dynamic_rel(s);
        } else if (do_debug || 
            file_type == TCC_OUTPUT_OBJ || 
            (s->sh_flags & SHF_ALLOC) ||
            i == (nb_sections - 1)) {
            /* we output all sections if debug or object file */
            s->sh_size = s->data_offset;
        }
    }

    /* allocate program segment headers */
    phdr = tcc_mallocz(phnum * sizeof(Elf32_Phdr));
        
    file_offset = sizeof(Elf32_Ehdr) + phnum * sizeof(Elf32_Phdr);
    if (phnum > 0) {
        /* compute section to program header mapping */
        if (file_type == TCC_OUTPUT_DLL)
            addr = 0;
        else
            addr = ELF_START_ADDR;

        /* dynamic relocation table information, for .dynamic section */
        rel_size = 0;
        rel_addr = 0;

        /* compute address after headers */
        addr += (file_offset & (ELF_PAGE_SIZE - 1));
        
        /* leave one program header for the program interpreter */
        ph = &phdr[0];
        if (interp)
            ph++;

        for(j = 0; j < 2; j++) {
            ph->p_type = PT_LOAD;
            if (j == 0)
                ph->p_flags = PF_R | PF_X;
            else
                ph->p_flags = PF_R | PF_W;
            ph->p_align = ELF_PAGE_SIZE;
            
            /* we do the following ordering: interp, symbol tables,
               relocations, progbits, nobits */
            /* XXX: do faster and simpler sorting */
            for(k = 0; k < 5; k++) {
                for(i = 1; i < nb_sections; i++) {
                    s = sections[i];
                    /* compute if section should be included */
                    if (j == 0) {
                        if ((s->sh_flags & (SHF_ALLOC | SHF_WRITE)) != 
                            SHF_ALLOC)
                            continue;
                    } else {
                        if ((s->sh_flags & (SHF_ALLOC | SHF_WRITE)) != 
                            (SHF_ALLOC | SHF_WRITE))
                            continue;
                    }
                    if (s == interp) {
                        if (k != 0)
                            continue;
                    } else if (s->sh_type == SHT_DYNSYM ||
                               s->sh_type == SHT_STRTAB ||
                               s->sh_type == SHT_HASH) {
                        if (k != 1)
                            continue;
                    } else if (s->sh_type == SHT_REL) {
                        if (k != 2)
                            continue;
                    } else if (s->sh_type == SHT_NOBITS) {
                        if (k != 4)
                            continue;
                    } else {
                        if (k != 3)
                            continue;
                    }
                    section_order[sh_order_index++] = i;

                    /* section matches: we align it and add its size */
                    tmp = file_offset;
                    file_offset = (file_offset + s->sh_addralign - 1) & 
                        ~(s->sh_addralign - 1);
                    s->sh_offset = file_offset;
                    addr += file_offset - tmp;
                    s->sh_addr = addr;
                    
                    /* update program header infos */
                    if (ph->p_offset == 0) {
                        ph->p_offset = file_offset;
                        ph->p_vaddr = addr;
                        ph->p_paddr = ph->p_vaddr;
                    }
                    /* update dynamic relocation infos */
                    if (s->sh_type == SHT_REL) {
                        if (rel_size == 0)
                            rel_addr = addr;
                        rel_size += s->sh_size;
                    }
                    addr += s->sh_size;
                    if (s->sh_type != SHT_NOBITS)
                        file_offset += s->sh_size;
                }
            }
            ph->p_filesz = file_offset - ph->p_offset;
            ph->p_memsz = addr - ph->p_vaddr;
            ph++;
        }

        /* if interpreter, then add corresponing program header */
        if (interp) {
            ph = &phdr[0];
            
            ph->p_type = PT_INTERP;
            ph->p_offset = interp->sh_offset;
            ph->p_vaddr = interp->sh_addr;
            ph->p_paddr = ph->p_vaddr;
            ph->p_filesz = interp->sh_size;
            ph->p_memsz = interp->sh_size;
            ph->p_flags = PF_R;
            ph->p_align = interp->sh_addralign;
        }
        
        /* if dynamic section, then add corresponing program header */
        if (dynamic) {
            int plt_offset;
            unsigned char *p;
            Elf32_Sym *sym_end;

            ph = &phdr[phnum - 1];
            
            ph->p_type = PT_DYNAMIC;
            ph->p_offset = dynamic->sh_offset;
            ph->p_vaddr = dynamic->sh_addr;
            ph->p_paddr = ph->p_vaddr;
            ph->p_filesz = dynamic->sh_size;
            ph->p_memsz = dynamic->sh_size;
            ph->p_flags = PF_R | PF_W;
            ph->p_align = dynamic->sh_addralign;

            /* put GOT dynamic section address */
            put32(got->data, dynamic->sh_addr);

            /* compute the PLT */
            plt->data_offset = 0;
            
            /* first plt entry */
            p = section_ptr_add(plt, 16);
            p[0] = 0xff; /* pushl got + 4 */
            p[1] = 0x35;
            put32(p + 2, got->sh_addr + 4);
            p[6] = 0xff; /* jmp *(got + 8) */
            p[7] = 0x25;
            put32(p + 8, got->sh_addr + 8);
            
            /* relocation symbols in .dynsym and build PLT. */
            plt_offset = 0;
            sym_end = (Elf32_Sym *)(dynsym->data + dynsym->data_offset);
            for(sym = (Elf32_Sym *)dynsym->data + 1; 
                sym < sym_end;
                sym++) {
                type = ELF32_ST_TYPE(sym->st_info);
                if (sym->st_shndx == SHN_UNDEF) {
                    if (type == STT_FUNC) {
                        /* one more entry in PLT */
                        p = section_ptr_add(plt, 16);
                        p[0] = 0xff; /* jmp *(got + x) */
                        p[1] = 0x25;
                        put32(p + 2, got->sh_addr + sym->st_value);
                        p[6] = 0x68; /* push $xxx */
                        put32(p + 7, plt_offset);
                        p[11] = 0xe9; /* jmp plt_start */
                        put32(p + 12, -(plt->data_offset));
                        
                        /* patch symbol value to point to plt */
                        sym->st_value = plt->sh_addr + p - plt->data;
                        
                        plt_offset += 8;
                    }
                } else if (sym->st_shndx < SHN_LORESERVE) {
                    /* do symbol relocation */
                    sym->st_value += sections[sym->st_shndx]->sh_addr;
                }
            }
            /* put dynamic section entries */

            dynamic->data_offset = saved_dynamic_data_offset;
            put_dt(dynamic, DT_HASH, dynsym->hash->sh_addr);
            put_dt(dynamic, DT_STRTAB, dynstr->sh_addr);
            put_dt(dynamic, DT_SYMTAB, dynsym->sh_addr);
            put_dt(dynamic, DT_STRSZ, dynstr->data_offset);
            put_dt(dynamic, DT_SYMENT, sizeof(Elf32_Sym));
            put_dt(dynamic, DT_REL, rel_addr);
            put_dt(dynamic, DT_RELSZ, rel_size);
            put_dt(dynamic, DT_RELENT, sizeof(Elf32_Rel));
            put_dt(dynamic, DT_NULL, 0);
        }

        ehdr.e_phentsize = sizeof(Elf32_Phdr);
        ehdr.e_phnum = phnum;
        ehdr.e_phoff = sizeof(Elf32_Ehdr);
    }

    /* all other sections come after */
    for(i = 1; i < nb_sections; i++) {
        s = sections[i];
        if (phnum > 0 && (s->sh_flags & SHF_ALLOC))
            continue;
        section_order[sh_order_index++] = i;
        
        file_offset = (file_offset + s->sh_addralign - 1) & 
            ~(s->sh_addralign - 1);
        s->sh_offset = file_offset;
        if (s->sh_type != SHT_NOBITS)
            file_offset += s->sh_size;
    }
    
    /* if building executable or DLL, then relocate each section
       except the GOT which is already relocated */
    if (file_type != TCC_OUTPUT_OBJ) {
        relocate_syms(0);

        /* relocate sections */
        /* XXX: ignore sections with allocated relocations ? */
        for(i = 1; i < nb_sections; i++) {
            s = sections[i];
            if (s->reloc && s != got)
                relocate_section(s1, s);
        }

        /* relocate relocation entries if the relocation tables are
           allocated in the executable */
        for(i = 1; i < nb_sections; i++) {
            s = sections[i];
            if ((s->sh_flags & SHF_ALLOC) &&
                s->sh_type == SHT_REL) {
                relocate_rel(s);
            }
        }

        /* get entry point address */
        if (file_type == TCC_OUTPUT_EXE)
            ehdr.e_entry = get_elf_sym_val("_start");
        else
            ehdr.e_entry = text_section->sh_addr; /* XXX: is it correct ? */
    }

    sort_syms(symtab_section);

    /* align to 4 */
    file_offset = (file_offset + 3) & -4;
    
    /* fill header */
    ehdr.e_ident[0] = ELFMAG0;
    ehdr.e_ident[1] = ELFMAG1;
    ehdr.e_ident[2] = ELFMAG2;
    ehdr.e_ident[3] = ELFMAG3;
    ehdr.e_ident[4] = ELFCLASS32;
    ehdr.e_ident[5] = ELFDATA2LSB;
    ehdr.e_ident[6] = EV_CURRENT;
    switch(file_type) {
    default:
    case TCC_OUTPUT_EXE:
        ehdr.e_type = ET_EXEC;
        break;
    case TCC_OUTPUT_DLL:
        ehdr.e_type = ET_DYN;
        break;
    case TCC_OUTPUT_OBJ:
        ehdr.e_type = ET_REL;
        break;
    }
    ehdr.e_machine = EM_386;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_shoff = file_offset;
    ehdr.e_ehsize = sizeof(Elf32_Ehdr);
    ehdr.e_shentsize = sizeof(Elf32_Shdr);
    ehdr.e_shnum = shnum;
    ehdr.e_shstrndx = shnum - 1;
    
    /* write elf file */
    if (file_type == TCC_OUTPUT_OBJ)
        mode = 0666;
    else
        mode = 0777;
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode); 
    if (fd < 0)
        error("could not write '%s'", filename);

    f = fdopen(fd, "w");
    fwrite(&ehdr, 1, sizeof(Elf32_Ehdr), f);
    fwrite(phdr, 1, phnum * sizeof(Elf32_Phdr), f);
    offset = sizeof(Elf32_Ehdr) + phnum * sizeof(Elf32_Phdr);
    for(i=1;i<nb_sections;i++) {
        s = sections[section_order[i]];
        if (s->sh_type != SHT_NOBITS) {
            while (offset < s->sh_offset) {
                fputc(0, f);
                offset++;
            }
            size = s->sh_size;
            fwrite(s->data, 1, size, f);
            offset += size;
        }
    }
    while (offset < ehdr.e_shoff) {
        fputc(0, f);
        offset++;
    }
    
    /* output section headers */
    for(i=0;i<nb_sections;i++) {
        sh = &shdr;
        memset(sh, 0, sizeof(Elf32_Shdr));
        s = sections[i];
        if (s) {
            sh->sh_name = s->sh_name;
            sh->sh_type = s->sh_type;
            sh->sh_flags = s->sh_flags;
            sh->sh_entsize = s->sh_entsize;
            sh->sh_info = s->sh_info;
            if (s->link)
                sh->sh_link = s->link->sh_num;
            sh->sh_addralign = s->sh_addralign;
            sh->sh_addr = s->sh_addr;
            sh->sh_offset = s->sh_offset;
            sh->sh_size = s->sh_size;
        }
        fwrite(sh, 1, sizeof(Elf32_Shdr), f);
    }
    fclose(f);

    tcc_free(section_order);
    tcc_free(phdr);
    return 0;
}

static void *load_data(int fd, unsigned long file_offset, unsigned long size)
{
    void *data;

    data = tcc_malloc(size);
    lseek(fd, file_offset, SEEK_SET);
    read(fd, data, size);
    return data;
}

typedef struct SectionMergeInfo {
    Section *s;            /* corresponding existing section */
    unsigned long offset;  /* offset of the new section in the existing section */
    int new_section;       /* true if section 's' was added */
} SectionMergeInfo;

/* load an object file and merge it with current files */
/* XXX: handle correctly stab (debug) info */
static int tcc_load_object_file(TCCState *s1, 
                                int fd, unsigned long file_offset)
{ 
    Elf32_Ehdr ehdr;
    Elf32_Shdr *shdr, *sh;
    int size, i, j, offset, offseti, nb_syms, sym_index;
    unsigned char *strsec, *strtab;
    int *old_to_new_syms;
    char *sh_name, *name;
    SectionMergeInfo *sm_table, *sm;
    Elf32_Sym *sym, *symtab;
    Elf32_Rel *rel, *rel_end;
    Section *s;

    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
        goto fail;
    if (ehdr.e_ident[0] != ELFMAG0 ||
        ehdr.e_ident[1] != ELFMAG1 ||
        ehdr.e_ident[2] != ELFMAG2 ||
        ehdr.e_ident[3] != ELFMAG3)
        goto fail;
    /* test if object file */
    if (ehdr.e_type != ET_REL)
        goto fail;
    /* test CPU specific stuff */
    if (ehdr.e_ident[5] != ELFDATA2LSB ||
        ehdr.e_machine != EM_386) {
    fail:
        error("invalid object file");
    }
    /* read sections */
    shdr = load_data(fd, file_offset + ehdr.e_shoff, 
                     sizeof(Elf32_Shdr) * ehdr.e_shnum);
    sm_table = tcc_mallocz(sizeof(SectionMergeInfo) * ehdr.e_shnum);
    
    /* load section names */
    sh = &shdr[ehdr.e_shstrndx];
    strsec = load_data(fd, file_offset + sh->sh_offset, sh->sh_size);

    /* load symtab and strtab */
    symtab = NULL;
    strtab = NULL;
    nb_syms = 0;
    for(i = 1; i < ehdr.e_shnum; i++) {
        sh = &shdr[i];
        if (sh->sh_type == SHT_SYMTAB) {
            if (symtab)
                error("object must contain only one symtab");
            nb_syms = sh->sh_size / sizeof(Elf32_Sym);
            symtab = load_data(fd, file_offset + sh->sh_offset, sh->sh_size);
            sm_table[i].s = symtab_section;

            /* now load strtab */
            sh = &shdr[sh->sh_link];
            strtab = load_data(fd, file_offset + sh->sh_offset, sh->sh_size);
        }
    }
        
    /* now examine each section and try to merge its content with the
       ones in memory */
    for(i = 1; i < ehdr.e_shnum; i++) {
        /* no need to examine section name strtab */
        if (i == ehdr.e_shstrndx)
            continue;
        sh = &shdr[i];
        sh_name = strsec + sh->sh_name;
        /* ignore sections types we do not handle */
        if (sh->sh_type != SHT_PROGBITS &&
            sh->sh_type != SHT_REL && 
            sh->sh_type != SHT_NOBITS)
            continue;
        if (sh->sh_addralign < 1)
            sh->sh_addralign = 1;
        /* find corresponding section, if any */
        for(j = 1; j < nb_sections;j++) {
            s = sections[j];
            if (!strcmp(s->name, sh_name))
                goto found;
        }
        /* not found: create new section */
        s = new_section(sh_name, sh->sh_type, sh->sh_flags);
        /* take as much info as possible from the section. sh_link and
           sh_info will be updated later */
        s->sh_addralign = sh->sh_addralign;
        s->sh_entsize = sh->sh_entsize;
        sm_table[i].new_section = 1;
    found:
        if (sh->sh_type != s->sh_type)
            goto fail;

        /* align start of section */
        offset = s->data_offset;
        size = sh->sh_addralign - 1;
        offset = (offset + size) & ~size;
        if (sh->sh_addralign > s->sh_addralign)
            s->sh_addralign = sh->sh_addralign;
        s->data_offset = offset;
        sm_table[i].offset = offset;
        sm_table[i].s = s;
        /* concatenate sections */
        size = sh->sh_size;
        if (sh->sh_type != SHT_NOBITS) {
            unsigned char *ptr;
            lseek(fd, file_offset + sh->sh_offset, SEEK_SET);
            ptr = section_ptr(s, size);
            read(fd, ptr, size);
        }
        s->data_offset += size;
    }

    /* second short pass to update sh_link and sh_info fields of new
       sections */
    sm = sm_table;
    for(i = 1; i < ehdr.e_shnum; i++) {
        s = sm_table[i].s;
        if (!s || !sm_table[i].new_section)
            continue;
        sh = &shdr[i];
        if (sh->sh_link > 0)
            s->link = sm_table[sh->sh_link].s;
        if (sh->sh_type == SHT_REL) {
            s->sh_info = sm_table[sh->sh_info].s->sh_num;
            /* update backward link */
            sections[s->sh_info]->reloc = s;
        }
    }

    /* resolve symbols */
    old_to_new_syms = tcc_mallocz(nb_syms * sizeof(int));

    sym = symtab + 1;
    for(i = 1; i < nb_syms; i++, sym++) {
        if (sym->st_shndx != SHN_UNDEF &&
            sym->st_shndx < SHN_LORESERVE) {
            sm = &sm_table[sym->st_shndx];
            /* if no corresponding section added, no need to add symbol */
            if (!sm->s)
                continue;
            /* convert section number */
            sym->st_shndx = sm->s->sh_num;
            /* offset value */
            sym->st_value += sm->offset;
        }
        /* add symbol */
        name = strtab + sym->st_name;
        sym_index = add_elf_sym(symtab_section, sym->st_value, sym->st_size, 
                                sym->st_info, sym->st_shndx, name);
        old_to_new_syms[i] = sym_index;
    }

    /* third pass to patch relocation entries */
    for(i = 1; i < ehdr.e_shnum; i++) {
        s = sm_table[i].s;
        if (!s)
            continue;
        sh = &shdr[i];
        offset = sm_table[i].offset;
        switch(s->sh_type) {
        case SHT_REL:
            /* take relocation offset information */
            offseti = sm_table[sh->sh_info].offset;
            rel_end = (Elf32_Rel *)(s->data + s->data_offset);
            for(rel = (Elf32_Rel *)(s->data + offset);
                rel < rel_end;
                rel++) {
                int type;
                unsigned sym_index;
                /* convert symbol index */
                type = ELF32_R_TYPE(rel->r_info);
                sym_index = ELF32_R_SYM(rel->r_info);
                /* NOTE: only one symtab assumed */
                if (sym_index >= nb_syms)
                    goto invalid_reloc;
                sym_index = old_to_new_syms[sym_index];
                if (!sym_index) {
                invalid_reloc:
                    error("Invalid relocation entry");
                }
                rel->r_info = ELF32_R_INFO(sym_index, type);
                /* offset the relocation offset */
                rel->r_offset += offseti;
            }
            break;
        default:
            break;
        }
    }
    tcc_free(symtab);
    tcc_free(strtab);
    tcc_free(old_to_new_syms);
    tcc_free(sm_table);
    tcc_free(shdr);
    return 0;
}

#define ARMAG  "!<arch>\012"	/* For COFF and a.out archives */

typedef struct ArchiveHeader {
    char ar_name[16];		/* name of this member */
    char ar_date[12];		/* file mtime */
    char ar_uid[6];		/* owner uid; printed as decimal */
    char ar_gid[6];		/* owner gid; printed as decimal */
    char ar_mode[8];		/* file mode, printed as octal   */
    char ar_size[10];		/* file size, printed as decimal */
    char ar_fmag[2];		/* should contain ARFMAG */
} ArchiveHeader;

/* load a '.a' file */
static int tcc_load_archive(TCCState *s1, int fd)
{
    ArchiveHeader hdr;
    char ar_size[11];
    char ar_name[17];
    char magic[8];
    int size, len, i;
    unsigned long file_offset;

    /* skip magic which was already checked */
    read(fd, magic, sizeof(magic));
    
    for(;;) {
        len = read(fd, &hdr, sizeof(hdr));
        if (len == 0)
            break;
        if (len != sizeof(hdr))
            error("invalid archive");
        memcpy(ar_size, hdr.ar_size, sizeof(hdr.ar_size));
        ar_size[sizeof(hdr.ar_size)] = '\0';
        size = strtol(ar_size, NULL, 0);
        memcpy(ar_name, hdr.ar_name, sizeof(hdr.ar_name));
        for(i = sizeof(hdr.ar_name) - 1; i >= 0; i--) {
            if (ar_name[i] != ' ')
                break;
        }
        ar_name[i + 1] = '\0';
        //        printf("name='%s' size=%d %s\n", ar_name, size, ar_size);
        file_offset = lseek(fd, 0, SEEK_CUR);
        if (!strcmp(ar_name, "/") ||
            !strcmp(ar_name, "//") ||
            !strcmp(ar_name, "__.SYMDEF") ||
            !strcmp(ar_name, "__.SYMDEF/") ||
            !strcmp(ar_name, "ARFILENAMES/")) {
            /* skip symbol table or archive names */
        } else {
            tcc_load_object_file(s1, fd, file_offset);
        }
        /* align to even */
        size = (size + 1) & ~1;
        lseek(fd, file_offset + size, SEEK_SET);
    }
    return 0;
}

/* load a DLL and all referenced DLLs. 'level = 0' means that the DLL
   is referenced by the user (so it should be added as DT_NEEDED in
   the generated ELF file) */
static int tcc_load_dll(TCCState *s1, int fd, const char *filename, int level)
{ 
    Elf32_Ehdr ehdr;
    Elf32_Shdr *shdr, *sh, *sh1;
    int i, nb_syms, nb_dts, sym_bind;
    Elf32_Sym *sym, *dynsym;
    Elf32_Dyn *dt, *dynamic;
    unsigned char *dynstr;
    const char *name, *soname, *p;
    DLLReference *dllref;
    
    read(fd, &ehdr, sizeof(ehdr));

    /* test CPU specific stuff */
    if (ehdr.e_ident[5] != ELFDATA2LSB ||
        ehdr.e_machine != EM_386)
        error("bad architecture");

    /* read sections */
    shdr = load_data(fd, ehdr.e_shoff, sizeof(Elf32_Shdr) * ehdr.e_shnum);

    /* load dynamic section and dynamic symbols */
    nb_syms = 0;
    nb_dts = 0;
    dynamic = NULL;
    dynsym = NULL; /* avoid warning */
    dynstr = NULL; /* avoid warning */
    for(i = 0, sh = shdr; i < ehdr.e_shnum; i++, sh++) {
        switch(sh->sh_type) {
        case SHT_DYNAMIC:
            nb_dts = sh->sh_size / sizeof(Elf32_Dyn);
            dynamic = load_data(fd, sh->sh_offset, sh->sh_size);
            break;
        case SHT_DYNSYM:
            nb_syms = sh->sh_size / sizeof(Elf32_Sym);
            dynsym = load_data(fd, sh->sh_offset, sh->sh_size);
            sh1 = &shdr[sh->sh_link];
            dynstr = load_data(fd, sh1->sh_offset, sh1->sh_size);
            break;
        default:
            break;
        }
    }
    
    /* compute the real library name */
    soname = filename;
    p = strrchr(soname, '/');
    if (p)
        soname = p + 1;
        
    for(i = 0, dt = dynamic; i < nb_dts; i++, dt++) {
        if (dt->d_tag == DT_SONAME) {
            soname = dynstr + dt->d_un.d_val;
        }
    }

    /* if the dll is already loaded, do not load it */
    for(i = 0; i < nb_loaded_dlls; i++) {
        dllref = loaded_dlls[i];
        if (!strcmp(soname, dllref->name)) {
            /* but update level if needed */
            if (level < dllref->level)
                dllref->level = level;
            goto the_end;
        }
    }
    
    //    printf("loading dll '%s'\n", soname);

    /* add the dll and its level */
    dllref = tcc_malloc(sizeof(DLLReference) + strlen(soname));
    dllref->level = level;
    strcpy(dllref->name, soname);
    dynarray_add((void ***)&loaded_dlls, &nb_loaded_dlls, dllref);

    /* add dynamic symbols in dynsym_section */
    for(i = 1, sym = dynsym + 1; i < nb_syms; i++, sym++) {
        sym_bind = ELF32_ST_BIND(sym->st_info);
        if (sym_bind == STB_LOCAL)
            continue;
        name = dynstr + sym->st_name;
        add_elf_sym(dynsymtab_section, sym->st_value, sym->st_size,
                    sym->st_info, sym->st_shndx, name);
    }

    /* load all referenced DLLs */
    for(i = 0, dt = dynamic; i < nb_dts; i++, dt++) {
        switch(dt->d_tag) {
        case DT_NEEDED:
            name = dynstr + dt->d_un.d_val;
            for(i = 0; i < nb_loaded_dlls; i++) {
                dllref = loaded_dlls[i];
                if (!strcmp(name, dllref->name))
                    goto already_loaded;
            }
            if (tcc_add_dll(s1, name, AFF_REFERENCED_DLL) < 0)
                error("referenced dll '%s' not found", name);
        already_loaded:
            break;
        }
    }
 the_end:
    tcc_free(shdr);
    return 0;
}

/* return -2 if error and CH_EOF if eof */
static void ld_skipspaces(void)
{
    while (ch == ' ' || ch == '\t' || ch == '\n')
        cinp();
}

static int ld_get_cmd(char *cmd, int cmd_size)
{
    char *q;

    ld_skipspaces();
    if (ch == CH_EOF)
        return -1;
    q = cmd;
    for(;;) {
        if (!((ch >= 'a' && ch <= 'z') ||
              (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') ||
              strchr("/.-_+=$:\\,~?*", ch)))
            break;
        if ((q - cmd) >= (cmd_size - 1))
            return -2;
        *q++ = ch;
        cinp();
    }
    *q = '\0';
    return 0;
}
                     
/* interpret a subset of GNU ldscripts to handle the dummy libc.so
   files */
static int tcc_load_ldscript(TCCState *s1)
{
    char cmd[64];
    char filename[1024];
    int ret;
    
    inp();
    cinp();
    for(;;) {
        ret = ld_get_cmd(cmd, sizeof(cmd));
        if (ret == CH_EOF)
            return 0;
        else if (ret < 0)
            return -1;
        //        printf("cmd='%s'\n", cmd);
        if (!strcmp(cmd, "INPUT") ||
            !strcmp(cmd, "GROUP")) {
            ld_skipspaces();
            if (ch != '(')
                expect("(");
            cinp();
            for(;;) {
                ld_get_cmd(filename, sizeof(filename));
                tcc_add_file(s1, filename);
                ld_skipspaces();
                if (ch == ',') {
                    cinp();
                } else if (ch == ')') {
                    cinp();
                    break;
                } else if (ch == CH_EOF) {
                    error("unexpected end of file");
                }
            }
        } else {
            return -1;
        }
    }
    return 0;
}

