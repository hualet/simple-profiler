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
#include <vector>

#include <elf/elf++.hh>
#include <dwarf/dwarf++.hh>
#include <fcntl.h>

using namespace std;

void getElfBuildID(const char* file, Elf64_Off offset, Elf64_Xword size, char* buildID)
{
    const char* data = file + offset;

   int32_t name_size;
   int32_t desc_size;
   memcpy(&name_size, data, 4);
   memcpy(&desc_size, data + 4, 4);

   // http://www.netbsd.org/docs/kernel/elf-notes.html
   // name_size: 4 bytes
   // desc_size: 4 bytes
   // type: 4 bytes
   // name: name_size
   // desc: desc_size
   if (4 + 4 + 4 + name_size + desc_size != size) {
       printf("something's wrong with the build-id section, size not correct");
       return;
   }

   char* name = (char*)malloc(name_size);
   char* desc = (char*)malloc(desc_size);
   memcpy(name, data + 12, name_size);
   memcpy(desc, data + 12 + name_size, desc_size);

   for (int i = 0; i < desc_size; i++) {
       // found from readelf source code.
       sprintf(&buildID[i * 2], "%02x", desc[i] & 0xff);
   }
}

int ElfUtil::findSymNameElf(const char *elfFile, uintptr_t offset, char *symName, uintptr_t *start, size_t *size, bool tryDebug)
{
    FILE *pyfile = fopen(elfFile, "r");
    if (pyfile == nullptr) {
        return 1;
    }
    if (fseek(pyfile, 0, SEEK_END)) {
        fclose(pyfile);
        perror("file seems to be corrupted");
        return 1;
    }
    long pyfile_size = ftell(pyfile);

    void *pybytes = mmap(nullptr, (size_t)pyfile_size, PROT_READ, MAP_PRIVATE,
                         fileno(pyfile), 0);
    if (pybytes == nullptr) {
        fclose(pyfile);
        perror("mmap failed");
        return 1;
    }
    fclose(pyfile);

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

    size_t strtab_off = 0;
    size_t symtab_off = 0;
    size_t symtab_sz = 0;

    char build_id[256];
    memset(&build_id, '\0', sizeof(build_id));

    Elf64_Shdr *shdr = (Elf64_Shdr*)(pybytes + elf_hdr.e_shoff);
    Elf64_Shdr *sh_strtab = &shdr[elf_hdr.e_shstrndx];
    const char *const sh_strtab_p = (char*)pybytes + sh_strtab->sh_offset;

    for (uint16_t i = 0; i < elf_hdr.e_shnum; i++) {
        char* hdr_name = NULL;
        size_t offset = elf_hdr.e_shoff + i * elf_hdr.e_shentsize;

        Elf64_Shdr shdr;
        memmove(&shdr, pybytes + offset, sizeof(shdr));

        hdr_name = (char*)(sh_strtab_p + shdr.sh_name);

        switch (shdr.sh_type) {
        case SHT_STRTAB: // .strtab .dynstr .shstrtab
            if (strcmp(".strtab", hdr_name) == 0) {
                strtab_off = shdr.sh_offset;
            } else if (strcmp(".dynstr", hdr_name) == 0) {
                dynstr_off = shdr.sh_offset;
            }
            break;
        case SHT_SYMTAB:
            symtab_off = shdr.sh_offset;
            symtab_sz = shdr.sh_size;
            break;
        case SHT_DYNSYM:
            dynsym_off = shdr.sh_offset;
            dynsym_sz = shdr.sh_size;
            break;
        case SHT_NOTE:
            if (strcmp(".note.gnu.build-id", hdr_name) == 0) {
                getElfBuildID((const char*)pybytes, shdr.sh_offset, shdr.sh_size, build_id);
            }
        default:
            break;
        }
    }

    // TODO(hualet): merge with bellow sample piece of code.
    if (dynstr_off > 0 && dynsym_off > 0) {
        for (size_t j = 0; j * sizeof(Elf64_Sym) < dynsym_sz; j++) {
            Elf64_Sym sym;
            size_t absoffset = dynsym_off + j * sizeof(Elf64_Sym);
            memmove(&sym, cbytes + absoffset, sizeof(sym));

            if (sym.st_name != 0 && sym.st_value <= offset
                    && offset <= sym.st_value + sym.st_size)
            {
                sprintf(symName, "%s", cbytes + dynstr_off + sym.st_name);
                *start = sym.st_value;
                *size = sym.st_size;
                return 0;
            }
        }
    }

    if (strtab_off > 0 && symtab_off > 0) {
        for (size_t j = 0; j * sizeof(Elf64_Sym) < symtab_sz; j++) {
            Elf64_Sym sym;
            size_t absoffset = symtab_off + j * sizeof(Elf64_Sym);
            memmove(&sym, cbytes + absoffset, sizeof(sym));

            if (sym.st_name != 0 && sym.st_value <= offset
                    && offset <= sym.st_value + sym.st_size)
            {
                sprintf(symName, "%s", cbytes + strtab_off + sym.st_name);
                *start = sym.st_value;
                *size = sym.st_size;
                return 0;
            }
        }
    }

    if (tryDebug) {
        char prefix[3];
        char suffix[256 - 2];
        char elfDebugFile[256];

        memcpy(prefix, build_id, 2);
        memcpy(suffix, &build_id[2], 256 - 2);
        sprintf(elfDebugFile, "/usr/lib/debug/.build-id/%s/%s.debug", prefix, suffix);

        return findSymNameElf(elfDebugFile, offset, symName, start, size, false);
    }

    return -1;
}

uint8_t ElfUtil::compareShared(std::vector<string> old_stack, std::vector<string> new_stack)
{
    if (old_stack.size() == 0 || new_stack.size() == 0) {
        return 0;
    }

    int i = 1;
    for (; i <= old_stack.size() && i <= new_stack.size(); i++) {
        if (old_stack.at(old_stack.size() - i) != new_stack.at(new_stack.size() - i))
            break;
    }

    return i - 1;
}

std::vector<std::string> ElfUtil::gained(std::vector<string> old_stack, std::vector<string> new_stack)
{
    std::vector<std::string> ret;

    uint8_t shared = compareShared(old_stack, new_stack);

    if (shared <= new_stack.size()) {
        uint z = 0;
        do {
            std::string symname = new_stack.at(z);
            ret.push_back(symname);
            z++;
        } while(z < new_stack.size() - shared);
    }

    return ret;
}
