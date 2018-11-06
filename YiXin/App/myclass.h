#ifndef TIMEEDIT_H
#define TIMEEDIT_H

#include <QTimeEdit>
#include <QDateEdit>
#include <QMouseEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QScrollBar>
#include <QTableWidget>
#include <QWidget>
#include <QTextEdit>
#include <QMessageBox>
#include <QDebug>

void SysCall(QString Cmd);
//-----------------------------------------------------------------------------// TimeEdit Class
class TimeEdit : public QTimeEdit
{
    Q_OBJECT  // 如果要使用信号与槽机制，就必须定义该宏

public:
   explicit TimeEdit(QWidget *parent = 0);
    ~TimeEdit();

protected:
    virtual void	focusInEvent(QFocusEvent *);
    friend void SysCall(QString Cmd);
};
//-----------------------------------------------------------------------------// DateEdit Class
class DateEdit : public QDateEdit
{
    Q_OBJECT  // 如果要使用信号与槽机制，就必须定义该宏

public:
   explicit DateEdit(QWidget *parent = 0);
    ~DateEdit();

protected:
    virtual void	focusInEvent(QFocusEvent *);
    friend void SysCall(QString Cmd);
};
//-----------------------------------------------------------------------------// CheckBox Class
class CheckBox : public QCheckBox
{
    Q_OBJECT  // 如果要使用信号与槽机制，就必须定义该宏

public:
   explicit CheckBox(QWidget *parent = 0);
    ~CheckBox();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    friend  void SysCall(QString Cmd);

signals:
    void clicked();
};
//-----------------------------------------------------------------------------// ComBox Class
class ComBox : public QComboBox
{
    Q_OBJECT  // 如果要使用信号与槽机制，就必须定义该宏

public:
   explicit ComBox(QWidget *parent = 0);
    ~ComBox();
   friend void SysCall(QString Cmd);

signals:
   void activated();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
};
//-----------------------------------------------------------------------------// Mybutton Class
class Mybutton : public QPushButton
{
    Q_OBJECT

public:
   explicit Mybutton(QWidget *parent = 0);
    ~Mybutton();
   friend void SysCall(QString Cmd);

protected:
    virtual void mousePressEvent(QMouseEvent *event);

signals:
    void clicked();
};
//-----------------------------------------------------------------------------// MyGroup Class
class MyGroup : public QGroupBox
{
    Q_OBJECT

public:
   explicit MyGroup(QWidget *parent = 0);
    ~MyGroup();
   friend void SysCall(QString Cmd);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
};
//-----------------------------------------------------------------------------//myLineEdit Class
class myLineEdit : public QLineEdit
{
    Q_OBJECT  // 如果要使用信号与槽机制，就必须定义该宏

public:
   explicit myLineEdit(QWidget *parent = 0);
    ~myLineEdit();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    friend void SysCall(QString Cmd);

signals:
    void clicked();
};
//-----------------------------------------------------------------------------// MyRadioButton Class
class MyRadioButton : public QRadioButton
{
    Q_OBJECT

public:
   explicit MyRadioButton(QWidget *parent = 0);
    ~MyRadioButton();
    friend void SysCall(QString Cmd);

protected:
    virtual void mousePressEvent(QMouseEvent *event);

//signals:
//    void clicked();
};
//-----------------------------------------------------------------------------// MyScrollBar Class
class MyScrollBar : public QScrollBar
{
    Q_OBJECT

public:
   explicit MyScrollBar(QWidget *parent = 0);
   MyScrollBar(Qt::Orientation orientation, QWidget * parent = 0);
    ~MyScrollBar();
    friend void SysCall(QString Cmd);

protected:
    virtual bool event(QEvent *event);
};
//-----------------------------------------------------------------------------// MyTableWidget Class
class MyTableWidget : public QTableWidget
{
    Q_OBJECT
public:

    explicit MyTableWidget(QWidget *parent = 0);
    MyTableWidget(int rows, int columns, QWidget * parent = 0);
    ~MyTableWidget();
    friend void SysCall(QString Cmd);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
};
//-----------------------------------------------------------------------------// MyTableWidget Class
class TextEdit : public QTextEdit
{
public:
   explicit TextEdit(QWidget *parent = 0);
    ~TextEdit();
   friend void SysCall(QString Cmd);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
};
//-----------------------------------------------------------------------------// MyWidget Class
class MyWidget : public QWidget
{
    Q_OBJECT  // 如果要使用信号与槽机制，就必须定义该宏

public:
   explicit MyWidget(QWidget *parent = 0);
    ~MyWidget();
    friend void SysCall(QString Cmd);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
};
//-----------------------------------------------------------------------------// MyMessageBox Class
class MyMessageBox : public QMessageBox
{
    Q_OBJECT

public:
   explicit MyMessageBox(QWidget *parent = 0);
    ~MyMessageBox();
    friend void SysCall(QString Cmd);

protected:
    virtual void mousePressEvent(QMouseEvent *event);

signals:
    void clicked();
};
//-------------------------------------------------------------------------------//
#endif // TIMEEDIT_H
