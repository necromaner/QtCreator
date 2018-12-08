#include "inputwidget.h"
#include "ui_inputwidget.h"

InputWidget::InputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InputWidget)
{
    ui->setupUi(this);
    int max = 100;
    int min = 0;
    /*QLineEdit*/
    initLineEditTab();
    initTextEditTab();
    initComboBoxTab();
}

InputWidget::~InputWidget()
{
    delete ui;
}

void InputWidget::on_checkBox_clearBtn_toggled(bool checked)
{
    ui->lineEdit->setClearButtonEnabled(checked);
}

void InputWidget::on_checkBox_readOnly_toggled(bool checked)
{
    ui->lineEdit->setReadOnly(checked);
}

void InputWidget::on_combo_inputH_currentIndexChanged(int index)
{
    ui->lineEdit->setAlignment((Qt::Alignment) (Qt::AlignLeft<<index));
}

void InputWidget::on_combo_inputV_currentIndexChanged(int index)
{
    ui->lineEdit->setAlignment((Qt::Alignment) (Qt::AlignTop<<index));
}

void InputWidget::on_combo_echoMode_currentIndexChanged(int index)
{
    ui->lineEdit->setEchoMode((QLineEdit::EchoMode)index);
}

void InputWidget::on_combo_validator_currentIndexChanged(int index)
{
    //删除当前验证器
    ui->lineEdit->setValidator(0);
    if (index == 1)
    {
        //整数验证器，0-80
        QIntValidator* validator = new QIntValidator(20, 80);
        ui->lineEdit->setValidator(validator);
    }
    else if (index == 2)
    {
        //双精度验证器，0.-1000.
        QDoubleValidator* validator = new QDoubleValidator();
        validator->setBottom(0.);
        validator->setTop(100.);
        ui->lineEdit->setValidator(validator);
    }
    else  if (index == 3)
    {
        //匹配0-99的整数
        QRegExpValidator* validator = new QRegExpValidator(QRegExp("^\\d\\d?$"));
        ui->lineEdit->setValidator(validator);
    }
}

void InputWidget::on_lineEdit_inputMask_textChanged(const QString &arg1)
{
    ui->lineEdit->setInputMask(arg1);
}

void InputWidget::initLineEditTab()
{
    //输入方向-水平
    QStringList textList;
    textList << "文字左对齐" <<"文字右对齐" << "文字水平居中" << "文字左对齐";
    ui->combo_inputH->addItems(textList);

    //输入方向-垂直
    textList.clear();
    textList << "文字上对齐" <<"文字下对齐" << "文字垂直居中" << "以基准线对齐";
    ui->combo_inputV->addItems(textList);

    //输入模式
    textList.clear();
    textList << "显示字符" <<"不显示字符" << "*显示输入字符" << "编辑时显示字符";
    ui->combo_echoMode->addItems(textList);

    //验证器
    textList.clear();
    textList << "无验证器" << "Int" <<"Double" << "Reqexp";
    ui->combo_validator->addItems(textList);
}
////----------------------------------------////
void InputWidget::on_spinBox_cursorWidth_valueChanged(int arg1)
{
    ui->textEdit->setCursorWidth(arg1);
}

void InputWidget::on_spinBox_lineWrapColumnOrWidth_valueChanged(int arg1)
{
    ui->textEdit->setLineWrapColumnOrWidth(arg1);
}

void InputWidget::on_spinBox_tabStopWidth_valueChanged(int arg1)
{
    ui->textEdit->setTabStopWidth(arg1);
}

void InputWidget::on_checkBox_overwriteMode_toggled(bool checked)
{
    ui->textEdit->setOverwriteMode(checked);
}

void InputWidget::on_checkBox_tabChangesFocus_toggled(bool checked)
{
    ui->textEdit->setTabChangesFocus(checked);
}

void InputWidget::on_comboBox_lineWrapMode_currentIndexChanged(int index)
{
    ui->textEdit->setLineWrapMode((QTextEdit::LineWrapMode)index);
}

void InputWidget::on_comboBox_wordWrapMode_currentIndexChanged(int index)
{
    ui->textEdit->setWordWrapMode((QTextOption::WrapMode)index);
}

void InputWidget::initTextEditTab()
{
    //输入方向-水平
    QStringList textList;
    textList << "不换行" <<"到达窗口边缘处换行" << "到达固定的像素值换行" << "到达固定的列号换行";
    ui->comboBox_lineWrapMode->addItems(textList);

    //输入方向-垂直
    textList.clear();
    textList << "不换行" << "边界换行" <<"不换行" << "一个单词可以分多行显示" << "WrapAtWordBoundaryOrAnywhere";
    ui->comboBox_wordWrapMode->addItems(textList);
}
////--------------------------------------------////
void InputWidget::initComboBoxTab()
{
    //插入模式
    QStringList textList;
    textList << "不能插入" <<"插入到最上边" << "替换当前的item文字" << "插入到最后"
               << "在当前item之后插入" << "在当前Item之前插入" << "按字幕顺序插入";
    ui->combo_insertPolicy->addItems(textList);
}

Student InputWidget::addStudent()
{
    Student stu;
    QString text = ui->combo_student->currentText();
    QStringList textList = text.split(",");
    if (textList.size() == 2)
    {
        bool ok;
        QString name = textList.at(0);
        uint age = textList.at(1).toUInt(&ok);
        if (ok)
        {
            //为每个学生创建唯一id
            stu.setId(QUuid::createUuid().toString());
            stu.setName(name);
            stu.setAge(age);
            m_studentHash.insert(stu.id(), stu);
        }
    }
    return stu;
}

void InputWidget::showStudent(int index)
{
    QString id = ui->combo_student->itemData(index).toString();
    Student stu = m_studentHash.value(id);
    if (stu.isValid())
    {
        ui->label_student->setText(stu.getDisplayText());
    }
}

bool InputWidget::hasAddStudentItem()
{
    return m_studentHash.size() < ui->combo_student->count();
}

void InputWidget::on_combo_student_activated(int index)
{
    if (hasAddStudentItem())//判断是否已经添加了学生item，此时的item并不一定合法，若不合法则要删除
    {
        Student stu = addStudent();
        if (stu.isValid())//如果输入正确，则添加学生条目
        {
            ui->combo_student->setItemData(index, stu.id());
        }
        else//如果是输入错误，则删除新增的item，并提示
        {
            ui->combo_student->removeItem(index);
            QMessageBox::information(this,
                                     tr("录入学生信息"),
                                     tr("输入错误，请重新输入，格式为：“姓名，年龄”。回车确认。"),
                                     "滚");
        }
    }

    showStudent(index);
}

void InputWidget::on_combo_insertPolicy_currentIndexChanged(int index)
{
    ui->combo_student->setInsertPolicy((QComboBox::InsertPolicy)index);
}

void InputWidget::on_check_editable_toggled(bool checked)
{
    ui->combo_student->setEditable(checked);
    if (checked)
        ui->combo_student->setEditText("录入格式：姓名,年龄");
}

void InputWidget::on_check_duplicatesEnabled_toggled(bool checked)
{
    ui->combo_student->setDuplicatesEnabled(checked);
}

void InputWidget::on_spinBox_maxCount_valueChanged(int arg1)
{
    ui->combo_student->setMaxCount(arg1);
}

void InputWidget::on_spinBox_maxVisibleItems_valueChanged(int arg1)
{
    ui->combo_student->setMaxVisibleItems(arg1);
}
//
void InputWidget::sltLineEditChanged(const QString &text)
{
    int val = text.toInt();
    ui->horizontalSlider->setValue(val);//设置slider当前值
    emit sigShowVal(text);//通知label显示文字
}
