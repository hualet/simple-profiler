#include "collector.h"

#include <stdio.h>
#include <memory.h>

#include "procutil.h"

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

//uint8_t Collector::compareShared(const QList<uintptr_t> &lastStack, const QList<uintptr_t> &stack) const
//{
//    if (lastStack.length() == 0 || stack.length() == 0) {
//        return 0;
//    }

//    int i = 1;
//    for (; i < stack.length() && i < stack.length(); i++) {
//        if (lastStack.at(lastStack.length() - i) != stack.at(stack.length() - i))
//            break;
//    }

//    return i - 1;
//}

void Collector::dumpOne() const
{
    for (int i = 0; i < m_threadCount; i++) {
        const ThreadBucket *buk = &m_threads[i];
        printf("Thread: %d\n=======================\n", buk->tid);
        const Sample *samp = &buk->samples[buk->sampleCount - 1];
        for (int z = 0; z < samp->depth; z++) {
            printf("0x%lx \n", samp->stacktrace[z]);
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
