#ifndef SIMPLEPROFILER_H
#define SIMPLEPROFILER_H

#include "simple-profiler_global.h"

#include <time.h>
#include <sys/time.h>

class SIMPLEPROFILERSHARED_EXPORT SimpleProfiler
{

public:
    SimpleProfiler();
    ~SimpleProfiler();

    void enableProfile();
    void disableProfile();

private:
    time_t m_sampleInterval = 1000 * 10;

    struct itimerval m_tick;

    void setupSignal();
    void setupProfileTimer();
};

#endif // SIMPLEPROFILER_H
