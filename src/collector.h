#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <QMap>

class Collector
{
public:
    ~Collector();

    static Collector* instance();

    QString getSymName(uintptr_t ptr) const;
    void setSymName(uintptr_t ptr, QString name);

    void collectSample(pid_t threadID, QList<uintptr_t> stacktrace);

private:
    QMap<uintptr_t, QString> m_symNameMap;

    QMap<pid_t, QList<QList<uintptr_t>>> m_samples;

    Collector();

    uint8_t compareShared(const QList<uintptr_t> &lastStack, const QList<uintptr_t> &stack) const;

    void dumpOne() const;
};

#endif // COLLECTOR_H
