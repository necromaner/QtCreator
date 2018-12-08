#include "inputwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    InputWidget w;
    w.show();

    return a.exec();
}
