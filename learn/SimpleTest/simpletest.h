#ifndef SIMPLETEST_H
#define SIMPLETEST_H

#include <QMainWindow>

namespace Ui {
class SimpleTest;
}

class SimpleTest : public QMainWindow
{
    Q_OBJECT

public:
    explicit SimpleTest(QWidget *parent = 0);
    ~SimpleTest();

private:
    Ui::SimpleTest *ui;
};

#endif // SIMPLETEST_H
