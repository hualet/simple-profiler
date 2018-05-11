#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <sys/types.h>
#include <stdint.h>
#include <map>

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

    std::map<std::pair<std::string, std::pair<uintptr_t, size_t>>, std::string> m_symCache;

    Collector();

    void dumpOne();
    int findSymName(uintptr_t pc, char* symName);
    int findThreadIndex(const pid_t& threadID) const;
};

#endif // COLLECTOR_H
