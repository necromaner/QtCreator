#include "myclass.h"

extern int iShowTime;
extern bool isWidget;
//-------------------------------------------------------------------------------------------// TimeEdit Class
TimeEdit::TimeEdit(QWidget *parent) : QTimeEdit(parent)
{

}

TimeEdit::~TimeEdit()
{
    this->deleteLater();
}

void TimeEdit::focusInEvent(QFocusEvent *)
{
    iShowTime = 0;
    if(isWidget == true)
    {
        SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
        isWidget = false;
    }
}
//-------------------------------------------------------------------------------------------// DateEdit Class
DateEdit::DateEdit(QWidget *parent) : QDateEdit(parent)
{

}

DateEdit::~DateEdit()
{
    this->deleteLater();
}

void DateEdit::focusInEvent(QFocusEvent *)
{
    iShowTime = 0;
    if(isWidget == true)
    {
        SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
        isWidget = false;
    }
}
//-------------------------------------------------------------------------------------------// CheckBox Class
CheckBox::CheckBox(QWidget *parent) : QCheckBox(parent)
{

}

CheckBox::~CheckBox()
{
    this->deleteLater();
}

void CheckBox::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
        }
        else
        {
            QCheckBox::mousePressEvent(event);
            emit clicked();
        }
    }
}
//-------------------------------------------------------------------------------------------// ComBox Class
ComBox::ComBox(QWidget *parent):QComboBox(parent)
{

}

ComBox::~ComBox()
{
    this->deleteLater();
}

void ComBox :: mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt :: LeftButton)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
        }
        else
        {
            QComboBox :: mouseMoveEvent(event);
            emit activated();
        }
    }
}
//-------------------------------------------------------------------------------------------//  MyButton Class
Mybutton::Mybutton(QWidget *parent):
QPushButton(parent)
{

}

Mybutton::~Mybutton()
{
    this->deleteLater();
}

void Mybutton :: mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
        }
        else
        {
            emit clicked();
        }
    }
}
//-------------------------------------------------------------------------------------------//   MyGroup Class
MyGroup::MyGroup(QWidget *parent):
QGroupBox(parent)
{

}

MyGroup::~MyGroup()
{
    this->deleteLater();
}

void MyGroup :: mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt :: LeftButton)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
        }
    }
}
//-------------------------------------------------------------------------------------------// myLineEdit Class
myLineEdit::myLineEdit(QWidget *parent):
QLineEdit(parent)
{

}

myLineEdit::~myLineEdit()
{
    this->deleteLater();
}

void myLineEdit :: mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt :: LeftButton)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
        }
        else
        {
            emit clicked();
        }
    }
    QLineEdit :: mouseMoveEvent(event);
}
//-------------------------------------------------------------------------------------------//MyRadioButton Class
MyRadioButton::MyRadioButton(QWidget *parent):
QRadioButton(parent)
{

}

MyRadioButton::~MyRadioButton()
{
    this->deleteLater();
}

void MyRadioButton::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt :: LeftButton)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
        }
        else
        {
            QRadioButton::mousePressEvent(event);
//            emit clicked();
        }
    }
}
//-------------------------------------------------------------------------------------------//MyScrollBar Class
MyScrollBar::MyScrollBar(QWidget *parent):
QScrollBar(parent)
{

}

MyScrollBar::MyScrollBar(Qt::Orientation orientation, QWidget *parent) : QScrollBar(orientation, parent)
{

}

MyScrollBar::~MyScrollBar()
{
    this->deleteLater();
}

bool MyScrollBar::event(QEvent *event)
{
    if(event->type() == QEvent::Paint)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
        }
    }
    return true;
}
//-------------------------------------------------------------------------------------------//MyTableWidget Class
MyTableWidget::MyTableWidget(QWidget *parent):
QTableWidget(parent)
{

}

MyTableWidget::MyTableWidget(int rows, int columns, QWidget * parent):QTableWidget(rows, columns, parent)
{

}

MyTableWidget::~MyTableWidget()
{
    this->deleteLater();
}

void MyTableWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt :: LeftButton)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
            event->ignore();
        }
        else
        {
            QTableWidget::mousePressEvent(event);
        }
    }
}
//-------------------------------------------------------------------------------------------//TextEdit Class
TextEdit::TextEdit(QWidget *parent):QTextEdit(parent)
{

}

TextEdit::~TextEdit()
{
    this->deleteLater();
}

void TextEdit::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt :: LeftButton)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
        }
    }
}
//-------------------------------------------------------------------------------------------//MyWidget Class
MyWidget::MyWidget(QWidget *parent):
QWidget(parent)
{

}

MyWidget::~MyWidget()
{
    this->deleteLater();
}

void MyWidget :: mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt :: LeftButton)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
        }
    }
}
//-------------------------------------------------------------------------------------------// MyMessageBox Class
MyMessageBox::MyMessageBox(QWidget *parent):
QMessageBox(parent)
{

}

MyMessageBox::~MyMessageBox()
{
    this->deleteLater();
}

void MyMessageBox :: mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt :: LeftButton)
    {
        iShowTime = 0;
        if(isWidget == true)
        {
            SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
            isWidget = false;
        }
    }
}
//-------------------------------------------------------------------------------------------//
void SysCall(QString Cmd)
{
    system((char *)Cmd.toStdString().data());
}
