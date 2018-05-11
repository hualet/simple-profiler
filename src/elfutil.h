#ifndef ELFUTIL_H
#define ELFUTIL_H

#include <stdint.h>

class ElfUtil
{
public:
    static int findSymNameElf(const char* elfFile, uintptr_t offset,
                              char* symName, bool tryDebug = true);

    static uint8_t compareShared(const uintptr_t* lastStack, int lastStackDepth,
                                 const uintptr_t* stack, int stackDepth);
};

#endif // ELFUTIL_H
