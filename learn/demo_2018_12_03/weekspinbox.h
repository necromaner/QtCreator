#ifndef WEEKSPINBOX_H
#define WEEKSPINBOX_H

#include <QSpinBox>

class WeekSpinBox : public QSpinBox
{
public:
    WeekSpinBox(QWidget* parent = 0);

protected:
    QValidator::State validate(QString &input, int &pos) const;
    int valueFromText(const QString &text) const;
    QString textFromValue(int val) const;
};

#endif // WEEKSPINBOX_H
