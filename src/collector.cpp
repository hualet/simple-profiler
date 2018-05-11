#include "collector.h"

#include <stdio.h>
#include <memory.h>
#include <map>
#include <vector>
#include <algorithm>
#include <cxxabi.h>
#include <sstream>

#include "procutil.h"
#include "elfutil.h"
#include "constants.h"

static Collector* _instance = nullptr;

Collector::~Collector()
{
    dumpOne();
}

Collector *Collector::instance()
{
    if (!_instance)
        _instance = new Collector;

    return _instance;
}

Collector::Collector()
{

}

void Collector::collectSample(pid_t threadID, uint8_t depth, uintptr_t* stacktrace)
{
    ThreadBucket *thread = nullptr;

    int idx = findThreadIndex(threadID);
    if (idx < 0) {
        thread = &m_threads[m_threadCount];
        m_threadCount++;
    } else {
        thread = &m_threads[idx];
    }

    Sample *sample = &thread->samples[thread->sampleCount];

    thread->tid = threadID;
    sample->depth = depth;
    memcpy(sample->stacktrace, stacktrace, sizeof(uintptr_t)*depth);
    thread->sampleCount++;
}

void Collector::dumpOne()
{
    for (int i = 0; i < m_threadCount; i++) {
        const ThreadBucket *buk = &m_threads[i];

        std::map<std::string, uint16_t> execCount;

        std::vector<std::string> oldStack;
        for (int j = 0; j < buk->sampleCount; j++) {
            Sample *samp = const_cast<Sample*>(&buk->samples[j]);

            std::vector<std::string> symbolizedStack;
            for (int z = 0; z < samp->depth; z++) {
                char symname[kMaxSymNameLength];
                uintptr_t pc = samp->stacktrace[z];

                int ret = findSymName(pc, symname);
                if (ret == 0) {
                    symbolizedStack.push_back(std::string(symname));
                } else {
                    std::stringstream stream;
                    stream << "0x" << std::hex << pc;
                    symbolizedStack.push_back(std::string(stream.str()));
                }
            }

            std::vector<std::string> gained = ElfUtil::gained(oldStack, symbolizedStack);
            for (std::vector<std::string>::iterator iter = gained.begin();
                 iter != gained.end(); iter++)
            {
                execCount[*iter] = ++execCount[*iter];
            }

            oldStack = symbolizedStack;
        }

        auto printPC = [&execCount, this](uintptr_t pc) {
            char symName[kMaxSymNameLength];
            int ret = findSymName(pc, symName);
            uint16_t execTime = execCount[std::string(symName)];
            if (ret == 0) {
                printf("(0x%lx) %.30s x %d\n", pc, symName, execTime);
            } else {
                printf("(0x%lx) anonymouse x %d \n", pc, execTime);
            }
        };

        auto printSym = [&execCount](std::string sym) {
            uint16_t execTime = execCount[sym];
            printf("%.50s x %d\n",sym.c_str(), execTime);
        };

        printf("========================== Thread: %d =======================\n\n", buk->tid);
        printf("Current stack: \n");
        printf("--------------------------------\n");
        const Sample *samp = &buk->samples[buk->sampleCount - 1];
        for (int z = 0; z < samp->depth; z++) {
            printPC(samp->stacktrace[z]);
        }
        printf("\n");

        std::vector<std::pair<std::string, uint16_t>> pairs;
        for (std::map<std::string, uint16_t>::iterator iter = execCount.begin();
             iter != execCount.end(); ++iter)
        {
            pairs.push_back(*iter);
        }
        std::sort(pairs.begin(), pairs.end(),
                  [=](std::pair<std::string, uint16_t>& a,
                  std::pair<std::string, uint16_t>& b) { return a.second > b.second; });

        printf("Max executed function(top  10): \n");
        printf("--------------------------------\n");
        int c = 0;
        for (std::vector<std::pair<std::string, uint16_t>>::iterator iter = pairs.begin();
             iter != pairs.end() && c < 10; ++iter, c++) {
            printSym(iter->first);
        }

        printf("\n");
    }
}

int Collector::findSymName(uintptr_t pc, char *symName)
{
    int offset;
    int status;
    uintptr_t start;
    size_t size;
    char elfname[kMaxElfNameLength];

    int ret = ProcUtil::findTargetElf(pc, elfname, &offset);
    if (ret != 0) {
        return ret;
    }

    for (std::map<std::pair<std::string, std::pair<uintptr_t, size_t>>, std::string>::iterator iter = m_symCache.begin();
         iter != m_symCache.end(); iter++)
    {
        std::string elf = iter->first.first;
        uintptr_t offset = iter->first.second.first;
        size_t size = iter->first.second.second;

        if (offset <= pc && pc <= offset + size && strcmp(elfname, elf.c_str()) == 0) {
            strcpy(symName, iter->second.c_str());
            return 0;
        }
    }

    ret = ElfUtil::findSymNameElf(elfname, offset, symName, &start, &size);
    if (ret != 0) {
        return ret;
    }

    char *demangled = abi::__cxa_demangle(symName, NULL, NULL, &status);
    if (status == 0) {
        strcpy(symName, demangled);
    }

    free(demangled);

    std::pair<uintptr_t, size_t> range(start, size);
    std::pair<std::string, std::pair<uintptr_t, size_t>> key(elfname, range);
    m_symCache[key] = symName;

    return 0;
}

int Collector::findThreadIndex(const pid_t &threadID) const
{
    for (int i = 0; i < m_threadCount; i++) {
        const ThreadBucket *buk = &m_threads[i];
        if (buk->tid == threadID)
            return i;
    }

    return -1;
}
