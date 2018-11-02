#include "mainwindow.h"
#include <QApplication>
#include <QPushButton>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QPushButton buttonWindow;
    buttonWindow.setText("我是窗口");
    buttonWindow.show();
    buttonWindow.resize(200,100);


    return a.exec();
}
