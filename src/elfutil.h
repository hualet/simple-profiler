#ifndef ELFUTIL_H
#define ELFUTIL_H

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>

class ElfUtil
{
public:
    static int findSymNameElf(const char* elfFile, uintptr_t offset,
                              char* symName, uintptr_t* start, size_t* size, bool tryDebug = true);

    static uint8_t compareShared(std::vector<std::string> old_stack,
                                 std::vector<std::string> new_stack);

    static std::vector<std::string> gained(std::vector<std::string> old_stack,
                                           std::vector<std::string> new_stack);
};

#endif // ELFUTIL_H
