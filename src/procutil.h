#ifndef PROCUTIL_H
#define PROCUTIL_H

#include <stdint.h>

class ProcUtil
{
public:
    static char* getApplicationPath();
    static int findTargetElf(uintptr_t pc, char* elf, int* offset);
};

#endif // PROCUTIL_H
