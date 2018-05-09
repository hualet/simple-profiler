#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "elfutil.h"

#include <elf.h>
#include <unistd.h>
#include <sys/mman.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#include <iostream>

int ElfUtil::findSymNameElf(const char *elfFile, uintptr_t offset, char *symName)
{
    FILE *pyfile = fopen(elfFile, "r");
    if (pyfile == nullptr) {
        return 1;
    }
    if (fseek(pyfile, 0, SEEK_END)) {
        fclose(pyfile);
        return 1;
    }
    long pyfile_size = ftell(pyfile);

    void *pybytes = mmap(nullptr, (size_t)pyfile_size, PROT_READ, MAP_PRIVATE,
                         fileno(pyfile), 0);
    if (pybytes == nullptr) {
        fclose(pyfile);
        perror("mmap()");
        return 1;
    }
    fclose(pyfile);
//    printf("%p\n", pybytes);

    const unsigned char expected_magic[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};

    Elf64_Ehdr elf_hdr;
    memmove(&elf_hdr, pybytes, sizeof(elf_hdr));
    if (memcmp(elf_hdr.e_ident, expected_magic, sizeof(expected_magic)) != 0) {
        std::cerr << "Target is not an ELF executable\n";
        return 1;
    }
    if (elf_hdr.e_ident[EI_CLASS] != ELFCLASS64) {
        std::cerr << "Sorry, only ELF-64 is supported.\n";
        return 1;
    }
    if (elf_hdr.e_machine != EM_X86_64) {
        std::cerr << "Sorry, only x86-64 is supported.\n";
        return 1;
    }

    char *cbytes = (char *)pybytes;

    size_t dynstr_off = 0;
    size_t dynsym_off = 0;
    size_t dynsym_sz = 0;

    for (uint16_t i = 0; i < elf_hdr.e_shnum; i++) {
        size_t offset = elf_hdr.e_shoff + i * elf_hdr.e_shentsize;
        Elf64_Shdr shdr;
        memmove(&shdr, pybytes + offset, sizeof(shdr));
        switch (shdr.sh_type) {
        case SHT_SYMTAB:
        case SHT_STRTAB:
            // TODO: have to handle multiple string tables better
            if (!dynstr_off) {
                dynstr_off = shdr.sh_offset;
            }
            break;
        case SHT_DYNSYM:
            dynsym_off = shdr.sh_offset;
            dynsym_sz = shdr.sh_size;
            break;
        default:
            break;
        }
    }

    assert(dynstr_off);
    assert(dynsym_off);

    for (size_t j = 0; j * sizeof(Elf64_Sym) < dynsym_sz; j++) {
        Elf64_Sym sym;
        size_t absoffset = dynsym_off + j * sizeof(Elf64_Sym);
        memmove(&sym, cbytes + absoffset, sizeof(sym));

        if (sym.st_name != 0 && sym.st_value <= offset
                && offset <= sym.st_value + sym.st_size)
        {
            sprintf(symName, "%s", cbytes + dynstr_off + sym.st_name);
            return 0;
        }
    }

    return -1;
}
