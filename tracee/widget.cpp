#include "widget.h"

#include <QTimer>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(false);

    connect(timer, &QTimer::timeout,
            this, &Widget::consumeCPU);

    timer->start(500);
}

Widget::~Widget()
{

}

void Widget::consumeCPU()
{
    int j = 0;
    for (int i = 0; i < 1000000; i++) {
        j += i;
    }
}
