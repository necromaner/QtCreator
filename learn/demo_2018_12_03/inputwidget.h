#ifndef INPUTWIDGET_H
#define INPUTWIDGET_H

#include <QWidget>
#include <QUuid>
#include <QMessageBox>
#include<QDebug>

namespace Ui {
class InputWidget;
}
class Student
{
public:
    Student()
    {
        m_age = 0;
    }
    bool isValid() const
    {
        return !id().isEmpty();
    }

    QString id() const {return m_id;}
    void setId(const QString &id) {m_id = id;}

    QString name() const{return m_name;}
    void setName(const QString &name){m_name = name;}

    uint age() const{return m_age;}
    void setAge(uint age){m_age = age;}

    QString getDisplayText() const
    {
        QString ret;
        if (isValid())
        {
            ret =  QString("姓名：%1\n年龄：%2").arg(name()).arg(age());
        }
        return ret;
    }

private:
    QString m_id;
    QString m_name;
    uint m_age;
};
class InputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InputWidget(QWidget *parent = 0);
    ~InputWidget();

private slots:
    void on_checkBox_clearBtn_toggled(bool checked);
    void on_checkBox_readOnly_toggled(bool checked);
    void on_combo_inputH_currentIndexChanged(int index);
    void on_combo_inputV_currentIndexChanged(int index);
    void on_combo_echoMode_currentIndexChanged(int index);
    void on_combo_validator_currentIndexChanged(int index);
    void on_lineEdit_inputMask_textChanged(const QString &arg1);
    //QTextEdit
    void on_spinBox_cursorWidth_valueChanged(int arg1);
    void on_spinBox_lineWrapColumnOrWidth_valueChanged(int arg1);
    void on_spinBox_tabStopWidth_valueChanged(int arg1);
    void on_checkBox_overwriteMode_toggled(bool checked);
    void on_checkBox_tabChangesFocus_toggled(bool checked);
    void on_comboBox_lineWrapMode_currentIndexChanged(int index);
    void on_comboBox_wordWrapMode_currentIndexChanged(int index);
    //QComboBox
    void on_combo_student_activated(int index);
    void on_combo_insertPolicy_currentIndexChanged(int index);
    void on_check_editable_toggled(bool checked);
    void on_check_duplicatesEnabled_toggled(bool checked);
    void on_spinBox_maxCount_valueChanged(int arg1);
    void on_spinBox_maxVisibleItems_valueChanged(int arg1);
private:
    void initLineEditTab();
    void initTextEditTab();
    void initComboBoxTab();
    Student addStudent();
    void showStudent(int index);
    bool hasAddStudentItem();

private:
    QHash<QString, Student> m_studentHash;//用来保存学生信息，key-id，value-学生类
signals:
    //自定义信号，发出此信号，使得QLabel显示文字
    void sigShowVal(const QString&);

public slots:
    //自定义槽，当LineEdit发出文字改变的信号时，执行这个槽
    void sltLineEditChanged(const QString& text);

private:
    Ui::InputWidget *ui;
};

#endif // INPUTWIDGET_H
