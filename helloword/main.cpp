#include "mainwindow.h"
#include <QApplication>
#include <QPushButton>
int main(int argc, char *argv[])
{
    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x0D,0x00,0x01,0x00,0x01,0x01,'\n',0x55,0x55};

    printf("%s\n",CmdBuf);


    char dat[50];
    QString ite_lise = "abcedf";
    sprintf(dat, "%8.8s", ite_lise.toStdString().data());
    printf("%s\n",dat);
    QString ite_lise1 = "jhi";
    sprintf(dat, "%8.8s", ite_lise1.toStdString().data());
    printf("%s\n",dat);
    QString ite_lise2 = "gklmnopqrs";
    sprintf(dat, "%8.8s", ite_lise2.toStdString().data());
    printf("%s\n",dat);
    QString ite_lise3 = "t";
    sprintf(dat, "%-8.8s", ite_lise3.toStdString().data());
    printf("%s\n",dat);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
