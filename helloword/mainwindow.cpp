#include "mainwindow.h"
#include "ui_mainwindow.h"

struct MainWindow::Private{
    int value1;
    int value2;
    int value3;
    Private(){
        value1=-1;
        value2=-1;
        value3=-1;
    }
    ~Private(){}
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    //设置窗口的背景颜色
//    setAutoFillBackground(true);
//    QPalette pa = this->palette();
//    pa.setBrush(QPalette::Background, QBrush(Qt::red));
//    setPalette(pa);


    //设置窗口透明度
    setWindowOpacity(0.9);

    //设置窗口标题
    setWindowTitle("窗口属性");

    //设置悬停提示
    setToolTip("悬停");

    //重设大小
    resize(600, 400);

    //移动
    move(0, 0);


    int max = 100;
    int min = 0;
    ui->horizontalSlider->setMaximum(max);//设置最大最小值
    ui->horizontalSlider->setMinimum(min);

    //设置QLineEdit只能输入数字，且为0-100
    QIntValidator* validator = new QIntValidator(min, max, this);
    ui->lineEdit->setValidator(validator);
    connect(ui->lineEdit_2, &QLineEdit::textChanged, this,  &MainWindow::sltLineEditChanged);

    connect(this, SIGNAL(sigShowVal(QString)), ui->label, SLOT(setText(QString)));



//    setGeometry(100,100,500,400);
//    move(100,100);
//    setWindowFlags(Qt::CustomizeWindowHint|Qt::WindowStaysOnTopHint);
    for(int i=3;i>=0;--i){
        QPushButton* button =new QPushButton(this);
        button->setText(QString("我是控件%1").arg(i));
        button->resize(100*i,100);
        connect(button,SIGNAL(clicked()),button,SLOT(close()));
    }


//    ui->lineEdit->setText("你好，QT！");
}

MainWindow::~MainWindow()
{
    delete d;
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    if(x>10){
        ui->label_3->setText("太大了");
        ui->lineEdit->setText("太大了");
        ui->lineEdit_2->setText("100");
        return;
    }
    ui->lineEdit->setText(QString("%1").arg(x));
    ui->lineEdit_2->setText(QString("%1").arg(x));
    ui->pushButton->setText(QString("按了%1次").arg(x));
    ui->label_3->setText(QString("按钮计数%1").arg(x++));
}
void MainWindow::sltLineEditChanged(const QString &text)
{
    int val = text.toInt();
    ui->horizontalSlider->setValue(val);//设置slider当前值
    emit sigShowVal(text);//通知label显示文字
}

void MainWindow::on_pushButton_2_toggled(bool checked)
{
    if(checked){
        ui->pushButton_2->setText("隐藏进度条");
    }else{
        ui->pushButton_2->setText("显示进度条");

    }
}
