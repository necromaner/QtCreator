#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTabWidget>
#include "stdlayoutwidget.h"
#include "ui_stdlayoutwidget.h"

StdLayoutWidget::StdLayoutWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StdLayoutWidget)
{
    ui->setupUi(this);

    //这里传入this，表示hLayout设置在主窗口上，如果这里不传this，则需要调用setLayout()接口
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    tabWidget = new QTabWidget;
    hLayout->addWidget(tabWidget);

    initHBoxLayout();
    initGridLayout();
}

StdLayoutWidget::~StdLayoutWidget()
{
    delete ui;
}

void StdLayoutWidget::initHBoxLayout()
{
    QWidget* hWidget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    hWidget->setLayout(layout);

    QStringList textList;
    textList << tr("一") << tr("二") << tr("三") << tr("四") << tr("五");
    for (int i = 0; i < textList.size(); ++i)
    {
        QString txt = textList.at(i);
        QPushButton *button = new QPushButton(txt);
        layout->addWidget(button);//将按钮加入到布局管理器中
        layout->addSpacing(i*10);//设置按钮之间的间距
    }

    //设置边距为20像素
    layout->setMargin(20);

    QPushButton *firstButton = hWidget->findChild<QPushButton *>();
    //将按钮“一”放到第三个位置上
    layout->insertWidget(3, firstButton);

    //将按钮“一”设置在上方
    layout->setAlignment(firstButton, Qt::AlignTop);

    //添加tab页
    tabWidget->addTab(hWidget, "QHBoxLayout");
}

void StdLayoutWidget::initGridLayout()
{
    QWidget* gridWidget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    gridWidget->setLayout(layout);

    QLabel *label1 = new QLabel("一");
    QLabel *label2 = new QLabel("二");
    QLabel *label3 = new QLabel("三");
    QLabel *label4 = new QLabel("四");
    QLabel *label5 = new QLabel("五");

    layout->addWidget(label1, 0/*0行*/, 0/*0列*/, 2/*占2行*/, 3/*占3列*/);
    layout->addWidget(label2, 2, 0, 1, 1);
    layout->addWidget(label3, 2, 1, 1, 1);
    layout->addWidget(label4, 2, 2, 1, 1);
    layout->addWidget(label5, 3, 1, 1, 9);

    //为所有标签设置背景颜色
    QList<QLabel *> allLabels = gridWidget->findChildren<QLabel *>();
    foreach (QLabel* label, allLabels)
    {
        QColor clr(qrand()%255, qrand()%255, qrand()%255);
        //为标签设置背景，关于样式表以后会讲到
        label->setStyleSheet(QString("QLabel{background-color:%1}").arg(clr.name()));
    }

    tabWidget->addTab(gridWidget, "QGridBoxLayout");
}
