#include "simpleprofiler.h"

#include <cerrno>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

SimpleProfiler _instace;

void SigProfHandler(int, siginfo_t*, void*)
{
    _instace.disableProfile();

    printf("Doing sampling here. \n");

    _instace.enableProfile();
}

SimpleProfiler::SimpleProfiler()
{
    setupSignal();
    setupProfileTimer();
    enableProfile();
}

SimpleProfiler::~SimpleProfiler()
{
    disableProfile();
}

void SimpleProfiler::setupSignal()
{
    struct sigaction action;
    action.sa_sigaction = reinterpret_cast<void (*)(int, siginfo_t *, void *)>(SigProfHandler);
    sigfillset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGPROF, &action, 0);
}

void SimpleProfiler::setupProfileTimer()
{
    m_tick.it_value.tv_sec = 0;
    m_tick.it_value.tv_usec = m_sampleInterval;
    m_tick.it_interval.tv_sec  = 0;
    m_tick.it_interval.tv_usec = m_sampleInterval;
}

void SimpleProfiler::enableProfile()
{
    int ret = setitimer(ITIMER_PROF, &m_tick, nullptr);
    if ( ret != 0) {
        fprintf(stderr, "failed to enable profiler: %s", strerror(errno));
    }
}

void SimpleProfiler::disableProfile()
{
    int ret = setitimer(ITIMER_PROF, nullptr, &m_tick);
    if ( ret != 0) {
        fprintf(stderr, "failed to disable profiler: %s", strerror(errno));
    }
}
