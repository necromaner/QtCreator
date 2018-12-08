#include "simpletest.h"
#include "ui_simpletest.h"

SimpleTest::SimpleTest(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SimpleTest)
{
    ui->setupUi(this);
}

SimpleTest::~SimpleTest()
{
    delete ui;
}
