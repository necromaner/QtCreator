#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals://声明信号
    //自定义信号，发出此信号，使得QLabel显示文字
    void sigShowVal(const QString&);


private slots:
    //自定义槽，当LineEdit发出文字改变的信号时，执行这个槽
    void sltLineEditChanged(const QString& text);
    void on_pushButton_clicked();

    void on_pushButton_2_toggled(bool checked);

private:
    Ui::MainWindow *ui;

    struct Private;
    struct Private *d;
    int x=1;
};

#endif // MAINWINDOW_H
