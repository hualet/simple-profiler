#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <sys/types.h>
#include <stdint.h>

class Collector
{
public:
    ~Collector();

    static Collector* instance();

    void collectSample(pid_t threadID, uint8_t depth, uintptr_t *stacktrace);

private:
    static const int kStacktraceNum = 128;
    static const int kSampleNum = 1024;
    static const int kThreadNum = 128;

    struct Sample {
        int depth;
        uintptr_t stacktrace[kStacktraceNum];
    };

    struct ThreadBucket {
        pid_t tid;

        int sampleCount;
        Sample samples[kSampleNum];
    };
    
    int m_threadCount;
    ThreadBucket m_threads[kThreadNum];

    Collector();

    void dumpOne() const;

    int findThreadIndex(const pid_t& threadID) const;
};

#endif // COLLECTOR_H
