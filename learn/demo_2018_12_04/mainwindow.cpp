#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->plainTextEdit_2,&QPlainTextEdit::textChanged,this,&MainWindow::zhushi);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::zhushi(const QString& text){
//    qDebug()<<text;
}
void MainWindow::zhushi1(){
    qDebug()<<"123";
}
