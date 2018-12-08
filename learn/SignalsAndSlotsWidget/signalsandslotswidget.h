#ifndef SIGNALSANDSLOTSWIDGET_H
#define SIGNALSANDSLOTSWIDGET_H

#include <QWidget>
#include<QValidator>
#include<QDebug>

namespace Ui {
class SignalsAndSlotsWidget;
}

class SignalsAndSlotsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SignalsAndSlotsWidget(QWidget *parent = 0);
    ~SignalsAndSlotsWidget();

private slots:
    void on_pushButton_clicked();

private:
    Ui::SignalsAndSlotsWidget *ui;
};

#endif // SIGNALSANDSLOTSWIDGET_H
