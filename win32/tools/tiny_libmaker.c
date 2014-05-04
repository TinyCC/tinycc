/*
 * This program is for making libtcc1.a without ar
 * tiny_libmaker - tiny elf lib maker
 * usage: tiny_libmaker [lib] files...
 * Copyright (c) 2007 Timppa
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../elf.h"

#ifdef TCC_TARGET_X86_64
# define ELFCLASSW ELFCLASS64
# define ElfW(type) Elf##64##_##type
# define ELFW(type) ELF##64##_##type
#else
# define ELFCLASSW ELFCLASS32
# define ElfW(type) Elf##32##_##type
# define ELFW(type) ELF##32##_##type
#endif

#define ARMAG  "!<arch>\n"
#define ARFMAG "`\n"

typedef struct ArHdr {
    char ar_name[16];
    char ar_date[12];
    char ar_uid[6];
    char ar_gid[6];
    char ar_mode[8];
    char ar_size[10];
    char ar_fmag[2];
} ArHdr;

unsigned long le2belong(unsigned long ul) {
    return ((ul & 0xFF0000)>>8)+((ul & 0xFF000000)>>24) +
        ((ul & 0xFF)<<24)+((ul & 0xFF00)<<8);
}

ArHdr arhdr = {
    "/               ",
    "            ",
    "0     ",
    "0     ",
    "0       ",
    "          ",
    ARFMAG
    };

ArHdr arhdro = {
    "                ",
    "            ",
    "0     ",
    "0     ",
    "0       ",
    "          ",
    ARFMAG
    };

int main(int argc, char **argv)
{
    FILE *fi, *fh = NULL, *fo = NULL;
    ElfW(Ehdr) *ehdr;
    ElfW(Shdr) *shdr;
    ElfW(Sym) *sym;
    int i, fsize, iarg;
    char *buf, *shstr, *symtab = NULL, *strtab = NULL;
    int symtabsize = 0;//, strtabsize = 0;
    char *anames = NULL;
    int *afpos = NULL;
    int istrlen, strpos = 0, fpos = 0, funccnt = 0, funcmax, hofs;
    char afile[260], tfile[260], stmp[20];
    char *file, *name;
    int ret = 2;


    strcpy(afile, "ar_test.a");
    iarg = 1;

    if (argc < 2)
    {
        printf("usage: tiny_libmaker [lib] file...\n");
        return 1;
    }
    for (i=1; i<argc; i++) {
        istrlen = strlen(argv[i]);
        if (argv[i][istrlen-2] == '.') {
            if(argv[i][istrlen-1] == 'a')
                strcpy(afile, argv[i]);
            else if(argv[i][istrlen-1] == 'o') {
                iarg = i;
                break;
            }
        }
    }

    if ((fh = fopen(afile, "wb")) == NULL)
    {
        fprintf(stderr, "Can't open file %s \n", afile);
        goto the_end;
    }

    sprintf(tfile, "%s.tmp", afile);
    if ((fo = fopen(tfile, "wb+")) == NULL)
    {
        fprintf(stderr, "Can't create temporary file %s\n", tfile);
        goto the_end;
    }

    funcmax = 250;
    afpos = realloc(NULL, funcmax * sizeof *afpos); // 250 func
    memcpy(&arhdro.ar_mode, "100666", 6);

    //iarg = 1;
    while (iarg < argc)
    {
        if (!strcmp(argv[iarg], "rcs")) {
            iarg++;
            continue;
        }
        if ((fi = fopen(argv[iarg], "rb")) == NULL)
        {
            fprintf(stderr, "Can't open file %s \n", argv[iarg]);
            goto the_end;
        }
        fseek(fi, 0, SEEK_END);
        fsize = ftell(fi);
        fseek(fi, 0, SEEK_SET);
        buf = malloc(fsize + 1);
        fread(buf, fsize, 1, fi);
        fclose(fi);

        //printf("%s:\n", argv[iarg]);

        // elf header
        ehdr = (ElfW(Ehdr) *)buf;
        if (ehdr->e_ident[4] != ELFCLASSW)
        {
            fprintf(stderr, "Unsupported Elf Class: %s\n", argv[iarg]);
            goto the_end;
        }

        shdr = (ElfW(Shdr) *) (buf + ehdr->e_shoff + ehdr->e_shstrndx * ehdr->e_shentsize);
        shstr = (char *)(buf + shdr->sh_offset);
        for (i = 0; i < ehdr->e_shnum; i++)
        {
            shdr = (ElfW(Shdr) *) (buf + ehdr->e_shoff + i * ehdr->e_shentsize);
            if (!shdr->sh_offset)
                continue;
            if (shdr->sh_type == SHT_SYMTAB)
            {
                symtab = (char *)(buf + shdr->sh_offset);
                symtabsize = shdr->sh_size;
            }
            if (shdr->sh_type == SHT_STRTAB)
            {
                if (!strcmp(shstr + shdr->sh_name, ".strtab"))
                {
                    strtab = (char *)(buf + shdr->sh_offset);
                    //strtabsize = shdr->sh_size;
                }
            }
        }

        if (symtab && symtabsize)
        {
            int nsym = symtabsize / sizeof(ElfW(Sym));
            //printf("symtab: info size shndx name\n");
            for (i = 1; i < nsym; i++)
            {
                sym = (ElfW(Sym) *) (symtab + i * sizeof(ElfW(Sym)));
                if (sym->st_shndx &&
                    (sym->st_info == 0x10
                    || sym->st_info == 0x11
                    || sym->st_info == 0x12
                    )) {
                    //printf("symtab: %2Xh %4Xh %2Xh %s\n", sym->st_info, sym->st_size, sym->st_shndx, strtab + sym->st_name);
                    istrlen = strlen(strtab + sym->st_name)+1;
                    anames = realloc(anames, strpos+istrlen);
                    strcpy(anames + strpos, strtab + sym->st_name);
                    strpos += istrlen;
                    if (++funccnt >= funcmax) {
                        funcmax += 250;
                        afpos = realloc(afpos, funcmax * sizeof *afpos); // 250 func more
                    }
                    afpos[funccnt] = fpos;
                }
            }
        }

        file = argv[iarg];
        for (name = strchr(file, 0);
             name > file && name[-1] != '/' && name[-1] != '\\';
             --name);
        istrlen = strlen(name);
        if (istrlen >= sizeof(arhdro.ar_name))
            istrlen = sizeof(arhdro.ar_name) - 1;
        memset(arhdro.ar_name, ' ', sizeof(arhdro.ar_name));
        memcpy(arhdro.ar_name, name, istrlen);
        arhdro.ar_name[istrlen] = '/';
        sprintf(stmp, "%-10d", fsize);
        memcpy(&arhdro.ar_size, stmp, 10);
        fwrite(&arhdro, sizeof(arhdro), 1, fo);
        fwrite(buf, fsize, 1, fo);
        free(buf);
        iarg++;
        fpos += (fsize + sizeof(arhdro));
    }
    hofs = 8 + sizeof(arhdr) + strpos + (funccnt+1) * sizeof(int);
    fpos = 0;
    if ((hofs & 1)) // align
        hofs++, fpos = 1;
    // write header
    fwrite("!<arch>\n", 8, 1, fh);
    sprintf(stmp, "%-10d", (int)(strpos + (funccnt+1) * sizeof(int)));
    memcpy(&arhdr.ar_size, stmp, 10);
    fwrite(&arhdr, sizeof(arhdr), 1, fh);
    afpos[0] = le2belong(funccnt);
    for (i=1; i<=funccnt; i++)
        afpos[i] = le2belong(afpos[i] + hofs);
    fwrite(afpos, (funccnt+1) * sizeof(int), 1, fh);
    fwrite(anames, strpos, 1, fh);
    if (fpos)
        fwrite("", 1, 1, fh);
    // write objects
    fseek(fo, 0, SEEK_END);
    fsize = ftell(fo);
    fseek(fo, 0, SEEK_SET);
    buf = malloc(fsize + 1);
    fread(buf, fsize, 1, fo);
    fwrite(buf, fsize, 1, fh);
    free(buf);
    ret = 0;
the_end:
    if (anames)
        free(anames);
    if (afpos)
        free(afpos);
    if (fh)
        fclose(fh);
    if (fo)
        fclose(fo), remove(tfile);
    return ret;
}
