#include "../src/elfutil.h"

#include <QObject>
#include <QtTest/QTest>

class TestCollector : public QObject
{
    Q_OBJECT

private slots:
    void sharedCompare();
};

void TestCollector::sharedCompare()
{
    uintptr_t *old1 = NULL;
    uintptr_t now1[] = {3, 2, 1};

    QCOMPARE(ElfUtil::compareShared(old1, 0, now1, 3), (uint8_t)0);

    uintptr_t old2[] = {2, 1};
    uintptr_t now2[] = {3, 2, 1};

    QCOMPARE(ElfUtil::compareShared(old2, 2, now2, 3), (uint8_t)2);

    uintptr_t old3[] = {3, 2, 1};
    uintptr_t now3[] = {3, 2, 1};

    QCOMPARE(ElfUtil::compareShared(old3, 3, now3, 3), (uint8_t)3);

    uintptr_t old4[] = {3, 2, 1};
    uintptr_t now4[] = {4, 3, 2, 1};

    QCOMPARE(ElfUtil::compareShared(old4, 3, now4, 4), (uint8_t)3);
}

QTEST_MAIN(TestCollector)
#include "testcollector.moc"
