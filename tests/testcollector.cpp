#include "../src/elfutil.h"

#include <QObject>
#include <QtTest/QTest>

class TestCollector : public QObject
{
    Q_OBJECT

private slots:
    void gained();
    void sharedCompare();
};

void TestCollector::sharedCompare()
{
    std::vector<std::string> old1 = {};
    std::vector<std::string> now1 = {"3", "2", "1"};

    QCOMPARE(ElfUtil::compareShared(old1, now1), (uint8_t)0);

    std::vector<std::string> old2 = {"2", "1"};
    std::vector<std::string> now2 = {"3", "2", "1"};

    QCOMPARE(ElfUtil::compareShared(old2, now2), (uint8_t)2);

    std::vector<std::string> old3 = {"3", "2", "1"};
    std::vector<std::string> now3 = {"3", "2", "1"};

    QCOMPARE(ElfUtil::compareShared(old3, now3), (uint8_t)3);

    std::vector<std::string> old4 = {"4", "3", "2", "1"};
    std::vector<std::string> now4 = {"3", "2", "1"};

    QCOMPARE(ElfUtil::compareShared(old4, now4), (uint8_t)3);


    std::vector<std::string> old5 = {"4", "3", "2", "1"};
    std::vector<std::string> now5 = {"5", "4", "2", "1"};

    QCOMPARE(ElfUtil::compareShared(old5, now5), (uint8_t)2);
}

void TestCollector::gained()
{
    std::vector<std::string> old1 = {};
    std::vector<std::string> now1 = {"3", "2", "1"};
    std::vector<std::string> new1 = {"3", "2", "1"};

    QCOMPARE(ElfUtil::gained(old1, now1), new1);

    std::vector<std::string> old2 = {"2", "1"};
    std::vector<std::string> now2 = {"3", "2", "1"};
    std::vector<std::string> new2 = {"3"};

    QCOMPARE(ElfUtil::gained(old2, now2), new2);

    std::vector<std::string> old3 = {"3", "2", "1"};
    std::vector<std::string> now3 = {"3", "2", "1"};
    std::vector<std::string> new3 = {"3"};

    QCOMPARE(ElfUtil::gained(old3, now3), new3);

    std::vector<std::string> old4 = {"4", "3", "2", "1"};
    std::vector<std::string> now4 = {"3", "2", "1"};
    std::vector<std::string> new4 = {"3"};

    QCOMPARE(ElfUtil::gained(old4, now4), new4);


    std::vector<std::string> old5 = {"4", "3", "2", "1"};
    std::vector<std::string> now5 = {"5", "4", "2", "1"};
    std::vector<std::string> new5 = {"5", "4"};

    QCOMPARE(ElfUtil::gained(old5, now5), new5);
}

QTEST_MAIN(TestCollector)
#include "testcollector.moc"
