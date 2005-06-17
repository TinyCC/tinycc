/*
 *  TCCPE.C - PE file output for the TinyC Compiler
 * 
 *  Copyright (c) 2005 grischka
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

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
#define ST static

/* XXX: move that to TCC ? */
int verbose = 0;

/* definitions below are from winnt.h */

typedef struct _IMAGE_DOS_HEADER {	/* DOS .EXE header */
    WORD e_magic;		/* Magic number */
    WORD e_cblp;		/* Bytes on last page of file */
    WORD e_cp;			/* Pages in file */
    WORD e_crlc;		/* Relocations */
    WORD e_cparhdr;		/* Size of header in paragraphs */
    WORD e_minalloc;		/* Minimum extra paragraphs needed */
    WORD e_maxalloc;		/* Maximum extra paragraphs needed */
    WORD e_ss;			/* Initial (relative) SS value */
    WORD e_sp;			/* Initial SP value */
    WORD e_csum;		/* Checksum */
    WORD e_ip;			/* Initial IP value */
    WORD e_cs;			/* Initial (relative) CS value */
    WORD e_lfarlc;		/* File address of relocation table */
    WORD e_ovno;		/* Overlay number */
    WORD e_res[4];		/* Reserved words */
    WORD e_oemid;		/* OEM identifier (for e_oeminfo) */
    WORD e_oeminfo;		/* OEM information; e_oemid specific */
    WORD e_res2[10];		/* Reserved words */
    DWORD e_lfanew;		/* File address of new exe header */
    BYTE e_code[0x40];

} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

#define IMAGE_NT_SIGNATURE  0x00004550	/* PE00 */
#define SIZE_OF_NT_SIGNATURE 4

typedef struct _IMAGE_FILE_HEADER {
    WORD Machine;
    WORD NumberOfSections;
    DWORD TimeDateStamp;
    DWORD PointerToSymbolTable;
    DWORD NumberOfSymbols;
    WORD SizeOfOptionalHeader;
    WORD Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;


#define IMAGE_SIZEOF_FILE_HEADER 20

typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD VirtualAddress;
    DWORD Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;


typedef struct _IMAGE_OPTIONAL_HEADER {
    /* Standard fields. */
    WORD Magic;
    BYTE MajorLinkerVersion;
    BYTE MinorLinkerVersion;
    DWORD SizeOfCode;
    DWORD SizeOfInitializedData;
    DWORD SizeOfUninitializedData;
    DWORD AddressOfEntryPoint;
    DWORD BaseOfCode;
    DWORD BaseOfData;

    /* NT additional fields. */
    DWORD ImageBase;
    DWORD SectionAlignment;
    DWORD FileAlignment;
    WORD MajorOperatingSystemVersion;
    WORD MinorOperatingSystemVersion;
    WORD MajorImageVersion;
    WORD MinorImageVersion;
    WORD MajorSubsystemVersion;
    WORD MinorSubsystemVersion;
    DWORD Win32VersionValue;
    DWORD SizeOfImage;
    DWORD SizeOfHeaders;
    DWORD CheckSum;
    WORD Subsystem;
    WORD DllCharacteristics;
    DWORD SizeOfStackReserve;
    DWORD SizeOfStackCommit;
    DWORD SizeOfHeapReserve;
    DWORD SizeOfHeapCommit;
    DWORD LoaderFlags;
    DWORD NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];

} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;


#define IMAGE_DIRECTORY_ENTRY_EXPORT          0	/* Export Directory */
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1	/* Import Directory */
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2	/* Resource Directory */
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3	/* Exception Directory */
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4	/* Security Directory */
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5	/* Base Relocation Table */
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6	/* Debug Directory */
/*      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7      (X86 usage) */
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7	/* Architecture Specific Data */
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8	/* RVA of GP */
#define IMAGE_DIRECTORY_ENTRY_TLS             9	/* TLS Directory */
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10	/* Load Configuration Directory */
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11	/* Bound Import Directory in headers */
#define IMAGE_DIRECTORY_ENTRY_IAT            12	/* Import Address Table */
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13	/* Delay Load Import Descriptors */
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14	/* COM Runtime descriptor */

/* Section header format. */
#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct _IMAGE_SECTION_HEADER {
    BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
	DWORD PhysicalAddress;
	DWORD VirtualSize;
    } Misc;
    DWORD VirtualAddress;
    DWORD SizeOfRawData;
    DWORD PointerToRawData;
    DWORD PointerToRelocations;
    DWORD PointerToLinenumbers;
    WORD NumberOfRelocations;
    WORD NumberOfLinenumbers;
    DWORD Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SIZEOF_SECTION_HEADER          40

/* ----------------------------------------------------------- */
typedef struct _IMAGE_BASE_RELOCATION {
    DWORD VirtualAddress;
    DWORD SizeOfBlock;
//  WORD    TypeOffset[1];
} IMAGE_BASE_RELOCATION;

#define IMAGE_SIZEOF_BASE_RELOCATION         8

#define IMAGE_REL_BASED_ABSOLUTE              0
#define IMAGE_REL_BASED_HIGH                  1
#define IMAGE_REL_BASED_LOW                   2
#define IMAGE_REL_BASED_HIGHLOW               3
#define IMAGE_REL_BASED_HIGHADJ               4
#define IMAGE_REL_BASED_MIPS_JMPADDR          5
#define IMAGE_REL_BASED_SECTION               6
#define IMAGE_REL_BASED_REL32                 7

/* ----------------------------------------------------------- */

/* ----------------------------------------------------------- */
IMAGE_DOS_HEADER pe_dos_hdr = {
    0x5A4D,			/*WORD e_magic;         Magic number */
    0x0090,			/*WORD e_cblp;          Bytes on last page of file */
    0x0003,			/*WORD e_cp;            Pages in file */
    0x0000,			/*WORD e_crlc;          Relocations */

    0x0004,			/*WORD e_cparhdr;       Size of header in paragraphs */
    0x0000,			/*WORD e_minalloc;      Minimum extra paragraphs needed */
    0xFFFF,			/*WORD e_maxalloc;      Maximum extra paragraphs needed */
    0x0000,			/*WORD e_ss;            Initial (relative) SS value */

    0x00B8,			/*WORD e_sp;            Initial SP value */
    0x0000,			/*WORD e_csum;          Checksum */
    0x0000,			/*WORD e_ip;            Initial IP value */
    0x0000,			/*WORD e_cs;            Initial (relative) CS value */
    0x0040,			/*WORD e_lfarlc;        File address of relocation table */
    0x0000,			/*WORD e_ovno;          Overlay number */
    {0, 0, 0, 0},		/*WORD e_res[4];     Reserved words */
    0x0000,			/*WORD e_oemid;         OEM identifier (for e_oeminfo) */
    0x0000,			/*WORD e_oeminfo;       OEM information; e_oemid specific */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/*WORD e_res2[10];      Reserved words */
    0x00000080,			/*DWORD   e_lfanew;        File address of new exe header */
    {				/* 14 code bytes + "This program cannot be run in DOS mode.\r\r\n$" + 6 * 0x00 */
     /*0040 */ 0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd, 0x21, 0xb8,
     0x01, 0x4c, 0xcd, 0x21, 0x54, 0x68,
     /*0050 */ 0x69, 0x73, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72, 0x61, 0x6d,
     0x20, 0x63, 0x61, 0x6e, 0x6e, 0x6f,
     /*0060 */ 0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6e, 0x20, 0x69,
     0x6e, 0x20, 0x44, 0x4f, 0x53, 0x20,
     /*0070 */ 0x6d, 0x6f, 0x64, 0x65, 0x2e, 0x0d, 0x0d, 0x0a, 0x24, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     /*0080 */
     }
};

DWORD pe_magic = IMAGE_NT_SIGNATURE;

IMAGE_FILE_HEADER pe_file_hdr = {
    0x014C,			/*WORD    Machine; */
    0x0003,			/*WORD    NumberOfSections; */
    0x00000000,			/*DWORD   TimeDateStamp; */
    0x00000000,			/*DWORD   PointerToSymbolTable; */
    0x00000000,			/*DWORD   NumberOfSymbols; */
    0x00E0,			/*WORD    SizeOfOptionalHeader; */
    0x030F			/*WORD    Characteristics; */
};

IMAGE_OPTIONAL_HEADER32 pe_opt_hdr = {
    /* Standard fields. */
    0x010B,			/*WORD    Magic; */
    0x06,			/*BYTE    MajorLinkerVersion; */
    0x00,			/*BYTE    MinorLinkerVersion; */
    0x00000000,			/*DWORD   SizeOfCode; */
    0x00000000,			/*DWORD   SizeOfInitializedData; */
    0x00000000,			/*DWORD   SizeOfUninitializedData; */
    0x00000000,			/*DWORD   AddressOfEntryPoint; */
    0x00000000,			/*DWORD   BaseOfCode; */
    0x00000000,			/*DWORD   BaseOfData; */

    /* NT additional fields. */
    0x00400000,			/*DWORD   ImageBase; */
    0x00001000,			/*DWORD   SectionAlignment; */
    0x00000200,			/*DWORD   FileAlignment; */
    0x0004,			/*WORD    MajorOperatingSystemVersion; */
    0x0000,			/*WORD    MinorOperatingSystemVersion; */
    0x0000,			/*WORD    MajorImageVersion; */
    0x0000,			/*WORD    MinorImageVersion; */
    0x0004,			/*WORD    MajorSubsystemVersion; */
    0x0000,			/*WORD    MinorSubsystemVersion; */
    0x00000000,			/*DWORD   Win32VersionValue; */
    0x00000000,			/*DWORD   SizeOfImage; */
    0x00000200,			/*DWORD   SizeOfHeaders; */
    0x00000000,			/*DWORD   CheckSum; */
    0x0002,			/*WORD    Subsystem; */
    0x0000,			/*WORD    DllCharacteristics; */
    0x00100000,			/*DWORD   SizeOfStackReserve; */
    0x00001000,			/*DWORD   SizeOfStackCommit; */
    0x00100000,			/*DWORD   SizeOfHeapReserve; */
    0x00001000,			/*DWORD   SizeOfHeapCommit; */
    0x00000000,			/*DWORD   LoaderFlags; */
    0x00000010,			/*DWORD   NumberOfRvaAndSizes; */

    /* IMAGE_DATA_DIRECTORY DataDirectory[16]; */
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
     {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}
};

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

struct pe_import_header {
    DWORD first_entry;
    DWORD time_date;
    DWORD forwarder;
    DWORD lib_name_offset;
    DWORD first_thunk;
};

struct pe_export_header {
    DWORD Characteristics;
    DWORD TimeDateStamp;
    DWORD Version;
    DWORD Name;
    DWORD Base;
    DWORD NumberOfFunctions;
    DWORD NumberOfNames;
    DWORD AddressOfFunctions;
    DWORD AddressOfNames;
    DWORD AddressOfNameOrdinals;
};

struct pe_reloc_header {
    DWORD offset;
    DWORD size;
};

/* ------------------------------------------------------------- */
/* internal temporary structures */

ST const char *pe_sec_names[] = {
    ".text",
    ".data",
    ".bss",
    ".rsrc",
    ".reloc",
    ".stab",
    ".stabstr"
};

enum {
    sec_text = 0,
    sec_data,
    sec_bss,
    sec_rsrc,
    sec_reloc,
    sec_stab,
    sec_stabstr,
    pe_sec_number
};

ST DWORD pe_flags[] = {
    0x60000020,			/* ".text", */
    0xC0000040,			/* ".data", */
    0xC0000080,			/* ".bss", */
    0x40000040,			/* ".rsrc", */
    0x42000040,			/* ".reloc", */
    0x42000802,			/* ".stab", */
    0x42000802			/* ".stabstr", */
};

struct section_info {
    struct section_info *next;
    int id;
    DWORD sh_addr;
    DWORD sh_size;
    unsigned char *data;
    DWORD data_size;
};

struct import_symbol {
    int sym_index;
    int offset;
};

struct pe_import_info {
    int dll_index;
    int sym_count;
    struct import_symbol **symbols;
};

struct pe_info {
    const char *filename;
    DWORD sizeofheaders;
    DWORD imagebase;
    DWORD start_addr;
    DWORD imp_offs;
    DWORD imp_size;
    DWORD iat_offs;
    DWORD iat_size;
    DWORD exp_offs;
    DWORD exp_size;
    struct section_info sh_info[pe_sec_number];
    int sec_count;
    struct pe_import_info **imp_info;
    int imp_count;
    Section *reloc;
    Section *thunk;
    TCCState *s1;
};

/* ------------------------------------------------------------- */
#define PE_MERGE_DATA
// #define PE_PRINT_SECTIONS

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

void error_noabort(const char *, ...);

ST char pe_type;

#define PE_NUL 0
#define PE_DLL 1
#define PE_GUI 2
#define PE_EXE 3

ST int pe_find_import(TCCState * s1, const char *symbol, char *ret)
{
    int sym_index = find_elf_sym(s1->dynsymtab_section, symbol);
    if (0 == sym_index) {
	/* Hm, maybe it's '_symbol' instead of 'symbol' or '__imp__symbol' */
	char buffer[100];
	if (0 == memcmp(symbol, "__imp__", 7))
	    symbol += 6;
	else
	    buffer[0] = '_', strcpy(buffer + 1, symbol), symbol = buffer;
	sym_index = find_elf_sym(s1->dynsymtab_section, symbol);
    }
    if (ret)
	strcpy(ret, symbol);
    return sym_index;
}

#ifdef WIN32
ST void **pe_imp;
ST int nb_pe_imp;

void *resolve_sym(struct TCCState *s1, const char *symbol, int type)
{
    char buffer[100], *p = buffer;
    void *a = NULL;
    int sym_index = pe_find_import(s1, symbol, p);
    int dll_index;
    const char *dll_name;
    void *hm;

    if (sym_index) {
	dll_index = ((Elf32_Sym *) s1->dynsymtab_section->data)[sym_index].
	    st_other;
        dll_name = s1->loaded_dlls[dll_index]->name;
	hm = GetModuleHandleA(dll_name);
        if (NULL == hm)
            hm = LoadLibraryA(dll_name);
        if (hm) {
            a = GetProcAddress(hm, buffer);
            if (a && STT_OBJECT == type) {
                // need to return a pointer to the address for data objects
                dynarray_add(&pe_imp, &nb_pe_imp, a);
                a = &pe_imp[nb_pe_imp - 1];
            }
        }
    }
    return a;
}
#endif

#define for_sym_in_symtab(sym) \
for (sym = (Elf32_Sym *)symtab_section->data + 1; \
	 sym < (Elf32_Sym *)(symtab_section->data + \
		symtab_section->data_offset); \
	 ++sym)

#define pe_set_datadir(dir,addr,size) \
	pe_opt_hdr.DataDirectory[dir].VirtualAddress = addr, \
	pe_opt_hdr.DataDirectory[dir].Size = size

/*----------------------------------------------------------------------------*/
ST void dynarray_reset(void ***pp, int *n)
{
    int i;
    for (i = 0; i < *n; ++i)
	tcc_free((*pp)[i]);
    tcc_free(*pp);
    *pp = NULL;
    *n = 0;
}

ST int dynarray_assoc(void **pp, int n, int key)
{
    int i;
    for (i = 0; i < n; ++i, ++pp)
	if (key == **(int **) pp)
	    return i;
    return -1;
}

#if 0
ST DWORD umin(DWORD a, DWORD b)
{
    return a < b ? a : b;
}
#endif

ST DWORD umax(DWORD a, DWORD b)
{
    return a < b ? b : a;
}

ST void pe_fpad(FILE * fp, DWORD new_pos)
{
    DWORD pos = ftell(fp);
    while (++pos <= new_pos)
	fputc(0, fp);
}

ST DWORD pe_file_align(DWORD n)
{
    return (n + (0x200 - 1)) & ~(0x200 - 1);
}

ST DWORD pe_virtual_align(DWORD n)
{
    return (n + (0x1000 - 1)) & ~(0x1000 - 1);
}

ST void pe_align_section(Section * s, int a)
{
    int i = s->data_offset & (a - 1);
    if (i)
	section_ptr_add(s, a - i);
}


/*----------------------------------------------------------------------------*/
ST int pe_write_pe(struct pe_info *pe)
{
    int i;
    FILE *op;
    DWORD file_offset;
    IMAGE_SECTION_HEADER ish[pe_sec_number], *psh;
    int sec_index = 0;

    op = fopen(pe->filename, "wb");
    if (NULL == op) {
	error_noabort("could not create file: %s", pe->filename);
	return 1;
    }

    memset(&ish, 0, sizeof ish);

    pe->sizeofheaders = pe_file_align(sizeof pe_dos_hdr
				      + sizeof pe_magic
				      + sizeof pe_file_hdr
				      + sizeof pe_opt_hdr
				      +
				      pe->sec_count *
				      sizeof(IMAGE_SECTION_HEADER)
	);

    file_offset = pe->sizeofheaders;
    pe_fpad(op, file_offset);

    if (2 == verbose)
	printf("-------------------------------"
	       "\n  virt   file   size  section" "\n");

    for (i = 0; i < pe->sec_count; ++i) {
	struct section_info *si = pe->sh_info + i;
	const char *sh_name = pe_sec_names[si->id];
	unsigned long addr = si->sh_addr - pe->imagebase;
	unsigned long size = si->sh_size;

	if (2 == verbose)
	    printf("%6lx %6lx %6lx  %s\n",
		   addr, file_offset, size, sh_name);

	switch (si->id) {
	case sec_text:
	    pe_opt_hdr.BaseOfCode = addr;
	    pe_opt_hdr.AddressOfEntryPoint = addr + pe->start_addr;
	    break;

	case sec_data:
	    pe_opt_hdr.BaseOfData = addr;
	    if (pe->imp_size) {
		pe_set_datadir(IMAGE_DIRECTORY_ENTRY_IMPORT,
			       pe->imp_offs + addr, pe->imp_size);
		pe_set_datadir(IMAGE_DIRECTORY_ENTRY_IAT,
			       pe->iat_offs + addr, pe->iat_size);
	    }
	    if (pe->exp_size) {
		pe_set_datadir(IMAGE_DIRECTORY_ENTRY_EXPORT,
			       pe->exp_offs + addr, pe->exp_size);
	    }
	    break;

	case sec_bss:
	    break;

	case sec_reloc:
	    pe_set_datadir(IMAGE_DIRECTORY_ENTRY_BASERELOC, addr, size);
	    break;

	case sec_rsrc:
	    pe_set_datadir(IMAGE_DIRECTORY_ENTRY_RESOURCE, addr, size);
	    break;

	case sec_stab:
	    break;

	case sec_stabstr:
	    break;
	}

	psh = &ish[sec_index++];
	strcpy((char *) psh->Name, sh_name);

	psh->Characteristics = pe_flags[si->id];
	psh->VirtualAddress = addr;
	psh->Misc.VirtualSize = size;
	pe_opt_hdr.SizeOfImage =
	    umax(psh->VirtualAddress + psh->Misc.VirtualSize,
		 pe_opt_hdr.SizeOfImage);

	if (si->data_size) {
	    psh->PointerToRawData = file_offset;
	    fwrite(si->data, 1, si->data_size, op);
	    file_offset = pe_file_align(file_offset + si->data_size);
	    psh->SizeOfRawData = file_offset - psh->PointerToRawData;
	    pe_fpad(op, file_offset);
	}
    }

	/*----------------------------------------------------- */

    pe_file_hdr.NumberOfSections = sec_index;
    pe_opt_hdr.SizeOfHeaders = pe->sizeofheaders;
    pe_opt_hdr.ImageBase = pe->imagebase;
    if (PE_DLL == pe_type)
	pe_file_hdr.Characteristics = 0x230E;
    else if (PE_GUI != pe_type)
	pe_opt_hdr.Subsystem = 3;

    fseek(op, SEEK_SET, 0);
    fwrite(&pe_dos_hdr, 1, sizeof pe_dos_hdr, op);
    fwrite(&pe_magic, 1, sizeof pe_magic, op);
    fwrite(&pe_file_hdr, 1, sizeof pe_file_hdr, op);
    fwrite(&pe_opt_hdr, 1, sizeof pe_opt_hdr, op);
    for (i = 0; i < sec_index; ++i)
	fwrite(&ish[i], 1, sizeof(IMAGE_SECTION_HEADER), op);
    fclose(op);

    if (2 == verbose)
	printf("-------------------------------\n");
    if (verbose)
	printf("<-- %s (%lu bytes)\n", pe->filename, file_offset);

    return 0;
}

/*----------------------------------------------------------------------------*/
ST int pe_add_import(struct pe_info *pe, int sym_index, DWORD offset)
{
    int i;
    int dll_index;
    struct pe_import_info *p;
    struct import_symbol *s;

    dll_index =
	((Elf32_Sym *) pe->s1->dynsymtab_section->data)[sym_index].
	st_other;
    i = dynarray_assoc((void **) pe->imp_info, pe->imp_count, dll_index);
    if (-1 != i) {
	p = pe->imp_info[i];
	goto found_dll;
    }
    p = tcc_mallocz(sizeof *p);
    p->dll_index = dll_index;
    dynarray_add((void ***) &pe->imp_info, &pe->imp_count, p);

  found_dll:
    i = dynarray_assoc((void **) p->symbols, p->sym_count, sym_index);
    if (-1 != i)
	goto found_sym;
    s = tcc_mallocz(sizeof *s);
    s->sym_index = sym_index;
    s->offset = offset;
    dynarray_add((void ***) &p->symbols, &p->sym_count, s);

  found_sym:
    return 1;
}

/*----------------------------------------------------------------------------*/
ST void pe_build_imports(struct pe_info *pe)
{
    int thk_ptr, ent_ptr, dll_ptr, sym_cnt, i;
    DWORD voffset = pe->thunk->sh_addr - pe->imagebase;
    int ndlls = pe->imp_count;

    for (sym_cnt = i = 0; i < ndlls; ++i)
	sym_cnt += pe->imp_info[i]->sym_count;

    if (0 == sym_cnt)
	return;

    pe_align_section(pe->thunk, 16);

    pe->imp_offs = dll_ptr = pe->thunk->data_offset;
    pe->imp_size = (ndlls + 1) * sizeof(struct pe_import_header);
    pe->iat_offs = dll_ptr + pe->imp_size;
    pe->iat_size = (sym_cnt + ndlls) * sizeof(DWORD);
    section_ptr_add(pe->thunk, pe->imp_size + 2 * pe->iat_size);

    thk_ptr = pe->iat_offs;
    ent_ptr = pe->iat_offs + pe->iat_size;
    for (i = 0; i < pe->imp_count; ++i) {
	struct pe_import_header *hdr;
	int k, n, v;
	struct pe_import_info *p = pe->imp_info[i];
	const char *name = pe->s1->loaded_dlls[p->dll_index]->name;

	/* put the dll name into the import header */
	if (0 == strncmp(name, "lib", 3))
	    name += 3;
	v = put_elf_str(pe->thunk, name);

	hdr = (struct pe_import_header *) (pe->thunk->data + dll_ptr);
	hdr->first_thunk = thk_ptr + voffset;
	hdr->first_entry = ent_ptr + voffset;
	hdr->lib_name_offset = v + voffset;

	for (k = 0, n = p->sym_count; k <= n; ++k) {
	    if (k < n) {
		DWORD offset = p->symbols[k]->offset;
		int sym_index = p->symbols[k]->sym_index;
		Elf32_Sym *sym =
		    (Elf32_Sym *) pe->s1->dynsymtab_section->data +
		    sym_index;
		const char *name =
		    pe->s1->dynsymtab_section->link->data + sym->st_name;

		if (offset & 0x80000000) {	/* ref to data */
		    Elf32_Sym *sym =
			&((Elf32_Sym *) symtab_section->
			  data)[offset & 0x7FFFFFFF];
		    sym->st_value = thk_ptr;
		    sym->st_shndx = pe->thunk->sh_num;
		} else {	/* ref to function */

		    char buffer[100];
		    sprintf(buffer, "IAT.%s", name);
		    sym_index =
			put_elf_sym(symtab_section, thk_ptr, sizeof(DWORD),
				    ELF32_ST_INFO(STB_GLOBAL, STT_OBJECT),
				    0, pe->thunk->sh_num, buffer);

		    put_elf_reloc(symtab_section, text_section, offset, R_386_32,	/*R_JMP_SLOT, */
				  sym_index);
		}
		v = pe->thunk->data_offset + voffset;
		section_ptr_add(pe->thunk, sizeof(WORD));	/* hint, not used */
		put_elf_str(pe->thunk, name);
	    } else {
		v = 0;		// last entry is zero
	    }
	    *(DWORD *) (pe->thunk->data + thk_ptr) =
		*(DWORD *) (pe->thunk->data + ent_ptr) = v;
	    thk_ptr += sizeof(DWORD);
	    ent_ptr += sizeof(DWORD);
	}
	dll_ptr += sizeof(struct pe_import_header);
	dynarray_reset((void ***) &p->symbols, &p->sym_count);
    }
    dynarray_reset((void ***) &pe->imp_info, &pe->imp_count);
}

/* ------------------------------------------------------------- */
ST int sym_cmp(const void *va, const void *vb)
{
    Elf32_Sym *sa = (Elf32_Sym *)symtab_section->data + *(int*)va;
    Elf32_Sym *sb = (Elf32_Sym *)symtab_section->data + *(int*)vb;
    const char *ca = symtab_section->link->data + sa->st_name;
    const char *cb = symtab_section->link->data + sb->st_name;
    return strcmp(ca, cb);
}

ST void pe_build_exports(struct pe_info *pe)
{
    Elf32_Sym *sym;
    DWORD func_offset, voffset;
    struct pe_export_header *hdr;
    int sym_count, n, ord, *sorted;

    voffset = pe->thunk->sh_addr - pe->imagebase;
    sym_count = 0, n = 1, sorted = NULL;

    // for simplicity only functions are exported
    for_sym_in_symtab(sym)
    {
        if ((sym->st_other & 1)
            && sym->st_shndx == text_section->sh_num)
            dynarray_add((void***)&sorted, &sym_count, (void*)n);
        ++n;
    }

    if (0 == sym_count)
	return;

    qsort (sorted, sym_count, sizeof sorted[0], sym_cmp);
    pe_align_section(pe->thunk, 16);

    pe->exp_offs = pe->thunk->data_offset;
    hdr = section_ptr_add(pe->thunk,
			  sizeof(struct pe_export_header) +
			  sym_count * (2 * sizeof(DWORD) + sizeof(WORD)));

    func_offset = pe->exp_offs + sizeof(struct pe_export_header);

    hdr->Characteristics = 0;
    hdr->Base = 1;
    hdr->NumberOfFunctions = sym_count;
    hdr->NumberOfNames = sym_count;
    hdr->AddressOfFunctions = func_offset + voffset;
    hdr->AddressOfNames         = hdr->AddressOfFunctions + sym_count * sizeof(DWORD);
    hdr->AddressOfNameOrdinals  = hdr->AddressOfNames + sym_count * sizeof(DWORD);
    hdr->Name = pe->thunk->data_offset + voffset;
    put_elf_str(pe->thunk, tcc_basename(pe->filename));

    for (ord = 0; ord < sym_count; ++ord)
    {
        char *name; DWORD *p, *pfunc, *pname; WORD *pord;
        sym = (Elf32_Sym *)symtab_section->data + sorted[ord];
        name = symtab_section->link->data + sym->st_name;
        p = (DWORD*)(pe->thunk->data + func_offset);
        pfunc = p + ord;
        pname = p + sym_count + ord;
        pord = (WORD *)(p + 2*sym_count) + ord;
        *pfunc = sym->st_value + pe->s1->sections[sym->st_shndx]->sh_addr - pe->imagebase;
	*pname = pe->thunk->data_offset + voffset;
        *pord  = ord;
	put_elf_str(pe->thunk, name);
	/* printf("export: %s\n", name); */
    }
    pe->exp_size = pe->thunk->data_offset - pe->exp_offs;
    tcc_free(sorted);
}

/* ------------------------------------------------------------- */
ST void pe_build_reloc(struct pe_info *pe, int *section_order,
		       int section_count)
{
    DWORD offset, block_ptr, addr;
    int count, i;
    Elf32_Rel *rel, *rel_end;
    Section *s = NULL, *sr;
    offset = addr = block_ptr = count = i = 0;
    rel = rel_end = NULL;
    for (;;) {
	if (rel < rel_end) {
	    int type = ELF32_R_TYPE(rel->r_info);
	    addr = rel->r_offset + s->sh_addr;
	    ++rel;
	    if (type != R_386_32)
		continue;
	    if (count == 0) {	/* new block */
		block_ptr = pe->reloc->data_offset;
		section_ptr_add(pe->reloc, sizeof(struct pe_reloc_header));
		offset = addr & 0xFFFFFFFF << 12;
	    }
	    if ((addr -= offset) < (1 << 12)) {	/* one block spans 4k addresses */
		WORD *wp = section_ptr_add(pe->reloc, sizeof(WORD));
		*wp = addr | IMAGE_REL_BASED_HIGHLOW << 12;
		++count;
		continue;
	    }
	    --rel;
	} else if (i < section_count) {
	    sr = (s = pe->s1->sections[section_order[i++]])->reloc;
	    if (sr) {
		rel = (Elf32_Rel *) sr->data;
		rel_end = (Elf32_Rel *) (sr->data + sr->data_offset);
	    }
	    continue;
	}

	if (count) {		/* store the last block and ready for a new one */
	    struct pe_reloc_header *hdr;
	    if (count & 1)
		section_ptr_add(pe->reloc, 2), ++count;
	    hdr = (struct pe_reloc_header *) (pe->reloc->data + block_ptr);
	    hdr->offset = offset - pe->imagebase;
	    hdr->size =
		count * sizeof(WORD) + sizeof(struct pe_reloc_header);
	    count = 0;
	}
	if (rel >= rel_end)
	    break;
    }
}

/* ------------------------------------------------------------- */
ST int pe_assign_addresses(struct pe_info *pe)
{
    int i, k, n;
    DWORD addr;
    int section_order[pe_sec_number];
    struct section_info *si_data = NULL;

    pe->imagebase = PE_DLL == pe_type ? 0x10000000 : 0x00400000;
    addr = pe->imagebase + 1;

    if (PE_DLL == pe_type)
	pe->reloc = new_section(pe->s1, ".reloc", SHT_DYNAMIC, SHF_ALLOC);

    for (n = k = 0; n < pe_sec_number; ++n) {
	for (i = 1; i < pe->s1->nb_sections; ++i) {
	    Section *s = pe->s1->sections[i];
	    if (0 == strcmp(s->name, pe_sec_names[n])) {
		struct section_info *si = &pe->sh_info[pe->sec_count];
#ifdef PE_MERGE_DATA
		if (n == sec_bss && si_data) {
		    /* append .bss to .data */
		    s->sh_addr = addr = ((addr - 1) | 15) + 1;
		    addr += s->data_offset;
		    si_data->sh_size = addr - si_data->sh_addr;
		} else
#endif
		{
		    si->sh_addr = s->sh_addr = addr =
			pe_virtual_align(addr);
		    si->id = n;

		    if (n == sec_data) {
			pe->thunk = s;
			si_data = si;
			pe_build_imports(pe);
			pe_build_exports(pe);
		    } else if (n == sec_reloc) {
			pe_build_reloc(pe, section_order, k);
		    }

		    if (s->data_offset) {
			if (n != sec_bss) {
			    si->data = s->data;
			    si->data_size = s->data_offset;
			}

			addr += s->data_offset;
			si->sh_size = s->data_offset;
			++pe->sec_count;
		    }
		    //printf("Section %08X %04X %s\n", si->sh_addr, si->data_size, s->name);
		}
		section_order[k] = i, ++k;
	    }
	}
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
ST int pe_check_symbols(struct pe_info *pe)
{
    Elf32_Sym *sym;
    int ret = 0;

    pe_align_section(text_section, 8);

    for_sym_in_symtab(sym) {
	if (sym->st_shndx == SHN_UNDEF) {
	    const char *symbol = symtab_section->link->data + sym->st_name;
	    unsigned type = ELF32_ST_TYPE(sym->st_info);
	    int sym_index = pe_find_import(pe->s1, symbol, NULL);
	    if (sym_index) {
		if (type == STT_FUNC) {
		    unsigned long offset = text_section->data_offset;
		    if (pe_add_import(pe, sym_index, offset + 2)) {
			/* add the 'jmp IAT[x]' instruction */
			*(WORD *) section_ptr_add(text_section, 8) =
			    0x25FF;
			/* patch the symbol */
			sym->st_shndx = text_section->sh_num;
			sym->st_value = offset;
			continue;
		    }
		} else if (type == STT_OBJECT) {	/* data, ptr to that should be */
		    if (pe_add_import(pe, sym_index,
				      (sym -
				      (Elf32_Sym *) symtab_section->data) | 
                                      0x80000000))
			continue;
		}
	    }
	    error_noabort("undefined symbol '%s'", symbol);
	    ret = 1;
	} else
	    if (pe->s1->rdynamic
		&& ELF32_ST_BIND(sym->st_info) != STB_LOCAL) {
	    /* if -rdynamic option, then export all non local symbols */
	    sym->st_other |= 1;
	}
    }
    return ret;
}

/*----------------------------------------------------------------------------*/
#ifdef PE_PRINT_SECTIONS
ST void pe_print_section(FILE * f, Section * s)
{				/* just if you'r curious */
    BYTE *p, *e, b;
    int i, n, l, m;
    p = s->data;
    e = s->data + s->data_offset;
    l = e - p;

    fprintf(f, "section  \"%s\"", s->name);
    if (s->link)
	fprintf(f, "\nlink     \"%s\"", s->link->name);
    if (s->reloc)
	fprintf(f, "\nreloc    \"%s\"", s->reloc->name);
    fprintf(f, "\nv_addr   %08X", s->sh_addr);
    fprintf(f, "\ncontents %08X", l);
    fprintf(f, "\n\n");

    if (s->sh_type == SHT_NOBITS)
	return;

    if (s->sh_type == SHT_SYMTAB)
	m = sizeof(Elf32_Sym);
    if (s->sh_type == SHT_REL)
	m = sizeof(Elf32_Rel);
    else
	m = 16;

    for (i = 0; i < l;) {
	fprintf(f, "%08X", i);
	for (n = 0; n < m; ++n) {
	    if (n + i < l)
		fprintf(f, " %02X", p[i + n]);
	    else
		fprintf(f, "   ");
	}

	if (s->sh_type == SHT_SYMTAB) {
	    Elf32_Sym *sym = (Elf32_Sym *) (p + i);
	    const char *name = s->link->data + sym->st_name;
	    fprintf(f,
		    "  name:%04X"
		    "  value:%04X"
		    "  size:%04X"
		    "  bind:%02X"
		    "  type:%02X"
		    "  other:%02X"
		    "  shndx:%04X"
		    "  \"%s\"",
		    sym->st_name,
		    sym->st_value,
		    sym->st_size,
		    ELF32_ST_BIND(sym->st_info),
		    ELF32_ST_TYPE(sym->st_info),
		    sym->st_other, sym->st_shndx, name);
	} else if (s->sh_type == SHT_REL) {
	    Elf32_Rel *rel = (Elf32_Rel *) (p + i);
	    Elf32_Sym *sym =
		(Elf32_Sym *) s->link->data + ELF32_R_SYM(rel->r_info);
	    const char *name = s->link->link->data + sym->st_name;
	    fprintf(f,
		    "  offset:%04X"
		    "  type:%02X"
		    "  symbol:%04X"
		    "  \"%s\"",
		    rel->r_offset,
		    ELF32_R_TYPE(rel->r_info),
		    ELF32_R_SYM(rel->r_info), name);
	} else {
	    fprintf(f, "   ");
	    for (n = 0; n < m; ++n) {
		if (n + i < l) {
		    b = p[i + n];
		    if (b < 32 || b >= 127)
			b = '.';
		    fprintf(f, "%c", b);
		}
	    }
	}
	i += m;
	fprintf(f, "\n");
    }
    fprintf(f, "\n\n");
}
#endif

static int pe_test_cmd(const char **pp, const char *cmd)
{
    const char *p;
    char *q, buf[16];
    int ret;

    p = *pp;
    q = buf;
    while (*p != '\0' && !is_space(*p)) {
        if ((q - buf) < sizeof(buf) - 1)
            *q++ = toup(*p);
        p++;
    }
    *q = '\0';
    ret = !strcmp(buf, cmd);
    *pp = p;
    return ret;
}

/* ------------------------------------------------------------- */
int pe_load_def_file(TCCState * s1, FILE * fp)
{
    DLLReference *dllref;
    int f = 0, sym_index;
    char *p, line[120], dllname[40];
    while (fgets(line, sizeof line, fp)) {
	p = strchr(line, 0);
	while (p > line && p[-1] <= ' ')
	    --p;
	*p = 0;
	p = line;
	while (*p && *p <= ' ')
	    ++p;

	if (*p && ';' != *p)
	    switch (f) {
	    case 0:
                if (!pe_test_cmd((const char **)&p, "LIBRARY"))
		    return -1;
                while (is_space(*p))
                    p++;
		pstrcpy(dllname, sizeof(dllname), p);
		++f;
		continue;

	    case 1:
                if (!pe_test_cmd((const char **)&p, "EXPORTS"))
		    return -1;
		++f;
		continue;

	    case 2:
		dllref =
		    tcc_malloc(sizeof(DLLReference) + strlen(dllname));
		strcpy(dllref->name, dllname);
		dllref->level = 0;
		dynarray_add((void ***) &s1->loaded_dlls,
			     &s1->nb_loaded_dlls, dllref);
		++f;

	    default:
		/* tccpe needs to know from what dll it should import
                   the sym */
		sym_index = add_elf_sym(s1->dynsymtab_section,
					0, 0, ELF32_ST_INFO(STB_GLOBAL,
							    STT_FUNC),
                                        s1->nb_loaded_dlls - 1,
					text_section->sh_num, p);
		continue;
	    }
    }
    return 0;
}

/* ------------------------------------------------------------- */
void pe_guess_outfile(char *objfilename, int output_type)
{
    char *ext = strrchr(objfilename, '.');
    if (NULL == ext)
        ext = strchr(objfilename, 0);
    if (output_type == TCC_OUTPUT_DLL)
        strcpy(ext, ".dll");
    else
    if (output_type == TCC_OUTPUT_EXE)
        strcpy(ext, ".exe");
    else
    if (output_type == TCC_OUTPUT_OBJ && strcmp(ext, ".o"))
        strcpy(ext, ".o");
    else
        error("no outputfile given");
}

/* ------------------------------------------------------------- */
unsigned long pe_add_runtime(TCCState * s1)
{
    const char *start_symbol;
    unsigned long addr;

    if (find_elf_sym(symtab_section, "WinMain"))
	pe_type = PE_GUI;
    else
    if (TCC_OUTPUT_DLL == s1->output_type)
    {
        pe_type = PE_DLL;
        // need this for 'tccelf.c:relocate_section()'
        s1->output_type = TCC_OUTPUT_EXE;
    }

    start_symbol =
	TCC_OUTPUT_MEMORY == s1->output_type
	? PE_GUI == pe_type ? "_runwinmain" : NULL
	: PE_DLL == pe_type ? "_dllstart"
	: PE_GUI == pe_type ? "_winstart" : "_start";

    /* grab the startup code from libtcc1 */
    if (start_symbol)
	add_elf_sym(symtab_section,
		    0, 0,
		    ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), 0,
		    SHN_UNDEF, start_symbol);

    if (0 == s1->nostdlib) {
	tcc_add_library(s1, "tcc1");
	tcc_add_library(s1, "msvcrt");
	if (PE_DLL == pe_type || PE_GUI == pe_type) {
	    tcc_add_library(s1, "kernel32");
	    tcc_add_library(s1, "user32");
	    tcc_add_library(s1, "gdi32");
	}
    }

    addr = start_symbol ?
	(unsigned long) tcc_get_symbol_err(s1, start_symbol) : 0;

    if (s1->output_type == TCC_OUTPUT_MEMORY && addr) {
	/* for -run GUI's, put '_runwinmain' instead of 'main' */
	add_elf_sym(symtab_section,
		    addr, 0,
		    ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), 0,
		    text_section->sh_num, "main");

	/* FreeConsole(); */
    }
    return addr;
}

int tcc_output_pe(TCCState * s1, const char *filename)
{
    int ret;
    struct pe_info pe;
    int i;
    memset(&pe, 0, sizeof pe);
    pe.filename = filename;
    pe.s1 = s1;
    pe.start_addr = pe_add_runtime(s1);

    relocate_common_syms();	/* assign bss adresses */
    ret = pe_check_symbols(&pe);
    if (0 == ret) {
	pe_assign_addresses(&pe);
	relocate_syms(s1, 0);
	for (i = 1; i < s1->nb_sections; ++i) {
	    Section *s = s1->sections[i];
	    if (s->reloc)
		relocate_section(s1, s);
	}
	ret = pe_write_pe(&pe);
    }
#ifdef PE_PRINT_SECTIONS
    {
	Section *s;
	FILE *f;
        f = fopen("tccpe.log", "wt");
	for (i = 1; i < s1->nb_sections; ++i) {
	    s = s1->sections[i];
	    pe_print_section(f, s);
	}
	pe_print_section(f, s1->dynsymtab_section);
	fclose(f);
    }
#endif
    return ret;
}

/*----------------------------------------------------------------------------*/
