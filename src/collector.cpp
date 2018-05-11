#include "collector.h"

#include <stdio.h>
#include <memory.h>
#include <map>

#include "procutil.h"
#include "elfutil.h"
#include "constants.h"

int getSymName(uintptr_t pc, char* symName)
{
    int offset;
    char elfname[256];

    int ret = ProcUtil::findTargetElf(pc, elfname, &offset);
    if (ret != 0) {
        return ret;
    }

    ret = ElfUtil::findSymNameElf(elfname, offset, symName);
    if (ret != 0) {
        return ret;
    }

    return 0;
}

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

uint8_t Collector::compareShared(const uintptr_t *lastStack, int lastStackDepth, const uintptr_t *stack, int stackDepth) const
{
    if (lastStackDepth == 0 || stackDepth == 0) {
        return 0;
    }

    int i = 1;
    for (; i < lastStackDepth && i < stackDepth; i++) {
        if (lastStack[lastStackDepth - i] != stack[stackDepth - i])
            break;
    }

    return i - 1;
}

void Collector::dumpOne() const
{
    for (int i = 0; i < m_threadCount; i++) {
        const ThreadBucket *buk = &m_threads[i];

        std::map<uintptr_t, uint16_t, std::greater<uint16_t>> execCount;

        int lastStackDepth = 0;
        uintptr_t *lastStack = NULL;
        for (int j = 0; j < buk->sampleCount; j++) {
            Sample *samp = const_cast<Sample*>(&buk->samples[j]);
            uint8_t shared = compareShared(lastStack, lastStackDepth, samp->stacktrace, samp->depth);

            if (shared <= samp->depth) {
                for (int z = shared; z < samp->depth; z++) {
                    uintptr_t pc = samp->stacktrace[samp->depth - z - 1];
                    execCount[pc] = ++execCount[pc];
                }
            }

            lastStackDepth = samp->depth;
            lastStack = samp->stacktrace;
        }

        auto printPC = [&execCount](uintptr_t pc) {
            char symName[kMaxSymNameLength];
            uint16_t execTime = execCount[pc];
            int ret = getSymName(pc, symName);
            if (ret == 0) {
                printf("(0x%lx) %s x %d\n", pc, symName, execTime);
            } else {
                printf("(0x%lx) anonymouse x %d \n", pc, execTime);
            }
        };

        printf("========================== Thread: %d =======================\n", buk->tid);
        printf("Current stack: \n");
        const Sample *samp = &buk->samples[buk->sampleCount - 1];
        for (int z = 0; z < samp->depth; z++) {
            printPC(samp->stacktrace[z]);
        }
        printf("\n");


        printf("Max executed function(top  10): \n");
        int c = 0;
        for (std::map<uintptr_t, uint16_t>::iterator iter = execCount.begin();
             iter != execCount.end() && c < 10;
             ++iter, c++) {
            printPC(iter->first);
        }

        printf("\n");
    }
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
