#include "collector.h"

#include <QDebug>
#include <QProcess>

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

QString Collector::getSymName(uintptr_t ptr) const
{
    return m_symNameMap.value(ptr, "");
}

void Collector::setSymName(uintptr_t ptr, QString name)
{
    m_symNameMap[ptr] = name;
}

void Collector::collectSample(pid_t threadID, QList<uintptr_t> stacktrace)
{
    qDebug() << "collecting...";

    QList<QList<uintptr_t>> samples = m_samples.value(threadID);
    samples.append(stacktrace);
    m_samples[threadID] = samples;
}

uint8_t Collector::compareShared(const QList<uintptr_t> &lastStack, const QList<uintptr_t> &stack) const
{
    if (lastStack.length() == 0 || stack.length() == 0) {
        return 0;
    }

    int i = 1;
    for (; i < stack.length() && i < stack.length(); i++) {
        if (lastStack.at(lastStack.length() - i) != stack.at(stack.length() - i))
            break;
    }

//    QDebug deg = qDebug();
//    for (const uintptr_t ptr : lastStack) {
//        deg << m_symNameMap.value(ptr);
//    }

//    QDebug deg1 = qDebug();
//    for (const uintptr_t ptr : stack) {
//        deg1 << m_symNameMap.value(ptr);
//    }

//    qDebug() << lastStack << lastStack.length();
//    qDebug() << stack << lastStack.length();
//    qDebug() << i - 1;

    return i - 1;
}

void Collector::dumpOne() const
{
    qDebug() << "";
    qDebug() << "";
    qDebug() << "=====================";

    for (const pid_t threadID : m_samples.keys()) {
        qDebug() << "thread: " << threadID;
        qDebug() << "=====================";

        QList<QList<uintptr_t>> samples = m_samples.value(threadID);

        QMap<uintptr_t, uint64_t> counts;
        QList<uintptr_t> lastStack = QList<uintptr_t>();
        for (const QList<uintptr_t>& stack : samples) {
            const int n = compareShared(lastStack, stack);
            if (n < stack.length()) {
                for (int i = 0; i < (stack.length() - n); i++) {
                    uintptr_t ptr = stack.at(i);
                    counts[ptr] = counts.value(ptr) + 1;
                }
            }
            lastStack = stack;
        }

        for (const uintptr_t ptr : lastStack) {
            qDebug() << QString::number(ptr, 16) << " (" <<  m_symNameMap.value(ptr) << ") " << counts.value(ptr);
        }

        qDebug() << "";
    }
}
