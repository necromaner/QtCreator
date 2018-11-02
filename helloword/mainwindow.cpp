#include "mainwindow.h"
#include "ui_mainwindow.h"

struct MainWindow::Private{
    int value1;
    int value2;
    int value3;
    Private(){
        value1=-1;
        value2=-1;
        value3=-1;
    }
    ~Private(){}
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lineEdit->setText("你好，QT！");
}

MainWindow::~MainWindow()
{
    delete d;
    delete ui;
}
