#ifndef ELFUTIL_H
#define ELFUTIL_H

#include <stdint.h>

class ElfUtil
{
public:
    static int findSymNameElf(const char* elfFile, uintptr_t offset, char* symName);
};

#endif // ELFUTIL_H
