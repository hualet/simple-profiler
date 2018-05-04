#include "simpleprofiler.h"

#define UNW_LOCAL_ONLY

#include <cerrno>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <sys/syscall.h>

#include <cxxabi.h>
#include <libunwind.h>

#include <QDebug>

#include "collector.h"

SimpleProfiler _instace;

QList<uintptr_t> Backtrace() {
    QList<uintptr_t> ret;

    unw_cursor_t cursor;
    unw_context_t context;

    // Initialize cursor to current frame for local unwinding.
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    unw_step(&cursor); // SigProfHandler
    unw_step(&cursor); // __rt_restore

    // Unwind frames one by one, going up the frame stack.
    while (unw_step(&cursor) > 0) {
        unw_word_t offset, pc;
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        if (pc == 0) {
            break;
        }

        // consume very much CPU compared to things belong to the tracee.
        /*
        if (Collector::instance()->getSymName(pc).isEmpty()) {
            char sym[256];
            if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
                char* nameptr = sym;

                int status;
                char* demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
                if (status == 0) {
                    nameptr = demangled;
                }

                Collector::instance()->setSymName(pc, QString(nameptr));

                std::free(demangled);
            }
        }
        */

        ret << (uintptr_t)pc;
    }

    return ret;
}

void SigProfHandler(int, siginfo_t*, void*)
{
    _instace.disableProfile();

    QList<uintptr_t> backtrace = Backtrace();
    pid_t threadID = syscall(SYS_gettid);
    Collector::instance()->collectSample(threadID, backtrace);

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
    delete Collector::instance();
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
        qDebug() << "failed to enable profiler: " << strerror(errno);
    }
}

void SimpleProfiler::disableProfile()
{
    int ret = setitimer(ITIMER_PROF, nullptr, &m_tick);
    if ( ret != 0) {
        qDebug() << "failed to enable profiler: " << strerror(errno);
    }
}
