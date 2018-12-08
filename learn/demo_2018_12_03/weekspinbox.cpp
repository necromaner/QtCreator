#include <QDebug>
#include "weekspinbox.h"

WeekSpinBox::WeekSpinBox(QWidget* parent):
    QSpinBox(parent)
{
    //一星期有七天，所以设置为1-7
    setMinimum(1);
    setMaximum(7);

    //设置可以循环，如果当前为星期日，则点击增加按钮，则会变成星期一
    setWrapping(true);
}

//验证器
//当你输入完按回车时，此函数用于验证输入的内容是否为所需要的内容。
//例如本例，当只有输入[一二三四五六七]时，才会接受。
QValidator::State WeekSpinBox::validate(QString &input, int &pos) const
{
    /*enum State {
        Invalid,//已经输入完毕（按回车），如果input不符合要求，则返回该值
        Intermediate,//还没输入完毕，但当前所输入的内容是符合要求的，则返回该值
        Acceptable//已经输入完毕，并符合要求，返回该值。
    };*/

    int value = valueFromText(input);
    if (value >= 1 && value <= 7)
    {
        return QValidator::Acceptable;
    }

    return QValidator::Invalid;
}

//如果需要将输入的文本解释为值时，就重新实现该函数。
int WeekSpinBox::valueFromText(const QString &text) const
{
    int index = tr("1234567一二三四五六七").indexOf(text);
    return index%7+1;
}

//如果需要将数字转化为文本，就重新实现该函数。
QString WeekSpinBox::textFromValue(int value) const
{
    QString txt = tr("一二三四五六天");
    return tr("星期%1").arg(txt.at(value-1));
}
