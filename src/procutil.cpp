#include "procutil.h"

#include <fstream>
#include <sstream>

uintptr_t parseAddress(std::string &str)
{
    uintptr_t ret;

    std::stringstream ss;
    ss << std::hex << str;
    ss >> ret;

    return ret;
}

int ProcUtil::findTargetElf(uintptr_t pc, char* elf, int* offset)
{
    std::ifstream input("/proc/self/maps");
    std::string line;

    while( std::getline( input, line )) {
        size_t addrRangePos = line.find_first_of(' ');
        std::string addrRange = line.substr(0, addrRangePos);

        size_t delPos = addrRange.find_first_of('-');
        std::string addrFromStr =  addrRange.substr(0, delPos);
        std::string addrToStr = addrRange.substr(delPos + 1, addrRangePos);

        uintptr_t addrFrom = parseAddress(addrFromStr);
        uintptr_t addrTo = parseAddress(addrToStr);

        if (addrFrom <= pc && pc <= addrTo) {
            size_t elfEnd = line.find_last_not_of(' ');
            size_t elfStart = line.find_last_of(' ', elfEnd - 1);
            std::string elfStr = line.substr(elfStart + 1, elfEnd - elfStart);

            sprintf(elf, "%s", elfStr.c_str());
            *offset = pc - addrFrom;

            return 0;
        }
    }

    return -1;
}
