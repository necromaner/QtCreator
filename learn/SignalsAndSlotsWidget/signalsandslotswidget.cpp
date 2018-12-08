#include "signalsandslotswidget.h"
#include "ui_signalsandslotswidget.h"

SignalsAndSlotsWidget::SignalsAndSlotsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SignalsAndSlotsWidget)
{
    ui->setupUi(this);
    QIntValidator* validator = new QIntValidator(0, 150, this);
    ui->lineEdit->setValidator(validator);
}

SignalsAndSlotsWidget::~SignalsAndSlotsWidget()
{
    delete ui;
}
/*
 * 注释对齐
 * 需求：
 * 1.引号中的//不算（完成）
 * 2.开头为//不算（完成）
 * 3.删除/添加//左边空格
 * 4.删除//右边的空格
 *
 * 设计思路：
 * 代码部分-空格部分-注释部分（代码部分包含代码前缩进空格）
 * 空格部分-注释部分
 * int x=0;//代码长度
 * int y=0;//每个空格段的空格长度
 * bool yinhao=true;//引号奇偶个数
 * bool zhushikongge=false;//注释开头部分空格
 * 1.引号中的//不算（特殊\"）
 *   双引号为成对出现的，通过bool类型，设置true为偶数，false为奇数
 *   读取到一次"改变一次值
 *   读取到"//"时，通过判断bool，是否结束读取代码部分。
 * 2.开头为//不算
 *   开头为//的情况为注释前面只有空格
 *   读取到"//"时x为0，则跳过本行操作。
 * 3.适当删除//左边空格
 *   如果读取完代码部分，则x为当前代码段总长度
 *   通过判断位置与x差值，添加适当空格
 * 4.删除//右边的空格
 *   在读取到"//"后，读取到空格不写入，读取到非空格，zhushikongge值改变为true
 *   false状态空格不写入，true状态空格写入
 */

void SignalsAndSlotsWidget::on_pushButton_clicked(){
//    qDebug()<<ui->plainTextEdit->toPlainText();
    QString s=ui->plainTextEdit->toPlainText();
    QString s1="";
    int num=ui->lineEdit->text().toInt();
    int x=0;
    int y=0;
    bool yinhao=true;
//    qDebug()<<"------------begin-------------";
    bool xx=true;
    bool yy=false;
    for(int i=0;i<s.length()-1;i++){
        if(s[i]=='\xa'){
//            qDebug()<<"换行";
            x=0;
            y=0;
            xx=true;
            yy=false;
            yinhao=true;
        }else{
            if(xx){
                if(x==num){
                    xx=false;
                }else{
//                    qDebug()<<s[i]+"----"+x;
                    if(s[i]=='/'&&s[i+1]=='/'){
                        xx=false;
                        if(yy&&yinhao){
                            for(int j=0;j<num-x;j++){
//                                qDebug()<<"空格";
                                s1+=" ";
                            }
                        }
                    }else if((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||s[i]==';'||s[i]==','||s[i]=='('||s[i]==')'||s[i]=='{'||s[i]=='}'||s[i]=='*'||s[i]=='&'){
                        yy=true;
                    }else if(s[i]=='"'){
                        if(true){
                            yinhao=false;
                        }else
                            yinhao=true;
                    }
                    x++;
                }
            }
        }
        s1+=s[i];
    }
    s1+=s[s.length()-1];
//    qDebug()<<"-------------end--------------";
    ui->textEdit_2->append(s1);
}


