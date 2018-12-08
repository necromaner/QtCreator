#include "stdlayoutwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    StdLayoutWidget w;
    w.show();

    return a.exec();
}
