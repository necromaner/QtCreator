
#include "yixin.h"

int iShowTime;
bool isWidget;

PctGuiThread::PctGuiThread(QThread *parent) : QThread(parent)//构造函数
{
    qDebug() << "#--------------Start YiXin App---------------#";
    qDebug() << "#--------------------------------------------------#";
    iShowTime = 0;
//    iPic = 0;
    isClicked = false;
    isWidget  = false;
    AddCheck = 0;
    isCalibrationKey = false;
    isYmaxError = false;
    AglDeviceP->SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
    MainWindowP = new MyWidget();             //新建窗口
    if(MainWindowP->isVisible())
    {
        MainWindowP->close();
        MainWindowP->deleteLater();
        Delayms(200);
        MainWindowP = new MyWidget();       //新建窗口
    }
    MainWindowP->setGeometry(0,0,800,480);
    MainWindowP->setFixedSize(800,480);          //固定窗口大小

    qDebug() << "#---------------Start Device--------------#";
    Sql = new TestSql(this);
    Sql->init_DatebaseMessage();
    Sql->SaveFile();
    AglDeviceP = new APP_dev(this);
    AglDeviceP->AppSta = AGL_DEV;                    // 初始化设备标识为荧光设备
    AglDeviceP->OpenDevice();                             // 打开设备

    check_type = Check_Start;
    CheckTime = new QTimer(this);
    connect(CheckTime, SIGNAL(timeout()), this, SLOT(CheckRunError()),Qt::UniqueConnection);
    CheckTime->start(50);
//    if(AglDeviceP->Error_Code == Error_OK)
//    {
//        AglDeviceP->agl_rd_ver();                                // 获取荧光电机控制板的程序版本
//        AglDeviceP->agl_rd_pm();                                // 获取荧光电机控制板的电机，荧光电流，温度参数
//    }
    AglDeviceP->start();
    Delayms(100);
    DispStarBmp();//开机的背景图片
//-----Add Tcp Socket------liuyouwei----------------------------------------------------------------------//
    LisSocket = new QTcpSocket(this);
    connect(LisSocket, SIGNAL(connected()), this, SLOT(Socket_connected()),Qt::UniqueConnection);                // 与服务器连接成功时触发
    connect(LisSocket, SIGNAL(disconnected()), this, SLOT(Socket_disconnected()),Qt::UniqueConnection);      // 断开与服务器的连接时触发（并不是连接失败的时候触发）
    connect(LisSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(Socket_Error(QAbstractSocket :: SocketError)),Qt::UniqueConnection);
//---------------------------------------------------------------------------------------------------------------------------------------------//
    QString Fimr = (QString)AglDeviceP->AglDev.Version;
    qDebug() << "Get gujian is " << Fimr;
    if(!Fimr.isEmpty())
    {
        QStringList Fimr_list = Fimr.split("-");
        QString FimrData = Fimr_list.at(1);
        FimrData = FimrData.left(FimrData.length() - 1);
        AglDeviceP->AppCfg->HardVersion = FimrData;
        qDebug() << "#--------------------------------------------#";
    }
}

PctGuiThread::~PctGuiThread()  // 析构函数
{
    ThreadState = false;
    UiTimer->stop();
    Time_Widget->stop();
    disconnect(UiTimer,SIGNAL(timeout()),this,SLOT(UiTimeOut()));
    disconnect(Time_Widget,SIGNAL(timeout()),this,SLOT(ChangeWidget()));
    delete UiTimer;
    delete Time_Widget;
    if(UiType != StarBmpUi && UiType != LoginUi)
    {
        door_time->stop();
        if(check_type != Check_NoNeed)
        {
            CheckTime->stop();
        }
        disconnect(CheckTime, SIGNAL(timeout()), this, SLOT(CheckRunError()));
        disconnect(timer_time,SIGNAL(timeout()),this,SLOT(flush_time()));
        disconnect(timer_time,SIGNAL(timeout()),this,SLOT(flush_icon()));
        disconnect(door_time,SIGNAL(timeout()),this,SLOT(flush_door_time()));
        delete timer_time;
        delete door_time;
        delete CheckTime;
    }
    MainWindowP->close();
    MainWindowP->deleteLater();
    LisSocket->close();
    LisSocket->deleteLater();
    this->exit();
    this->quit();
}

void PctGuiThread::SetMotor_Move()
{
    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x0D,0x00,0x01,0x01,0xff,0xff,'\n',0x55,0x55};
    //--------------------------------//
    if(!AglDeviceP->agl_wr_data(CmdBuf,16))
    {
        qDebug() << "Write BUF ERROR";
        return;
    }
}

void PctGuiThread::SetMotor_Start_Location()
{
    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x0D,0x00,0x01,0x00,0xff,0xff,'\n',0x55,0x55};
    //--------------------------------//
    if(!AglDeviceP->agl_wr_data(CmdBuf,16))
    {
        qDebug() << "Write BUF ERROR";
        return;
    }
}
static bool isReadOK = false;
void PctGuiThread::ReadScanStation()
{
//    static bool isReadOK = false;
    static bool isRead0OK = false;;
    static int iRead_1_ok = 0;
    static bool isReadComData = false;
    static bool isReadEND_OK = false;
    static QByteArray Scan1Data;
    if(true == isReadComData)
    {
        return;
    }
    isReadComData = true;

    if(true == isReadEND_OK)
    {
        qDebug() << "========= Read Data 2OK";
        QByteArray Station = AglDeviceP->agl_rd_data(0x0D, 0);
        if("2OK" == (QString)Station.data())
        {
            qDebug() << "<><><><><><> Read Data 2OK Over";
            if(!HandleScanOK(Scan1Data))
            {
                isReadComData = false;
                isReadOK = false;
                isRead0OK = false;
                isReadEND_OK = false;

                isBarCodeGetOK = false;
                isClickedCloseDoor = false;
                isGetBarCodeDataError = true;

                iRead_1_ok = 0;
                Scan1Data.clear();
                return;
            }
            StartTestWidget();

            isReadComData = false;
            isReadOK = false;
            isRead0OK = false;
            isReadEND_OK = false;
            iRead_1_ok = 0;
            Scan1Data.clear();
        }
        isReadComData = false;
    }
    else
    {
        if(false == isReadOK)
        {
            qDebug() << "======Reader OK";
            static int IError = 0;
            QByteArray Station = AglDeviceP->agl_rd_data(0x05, 0);
            if("OK" == (QString)Station.data() || isBarCodeGetOK == true)
            {
                qDebug() << "<><><><><><> Read Data OK Over";
                isReadOK = true;
                IError = 0;
                emit LocationOK();
            }
            if(60 == IError)
            {
                isGetBarCodeDataError = true;
                IError = 0;
                Voice_ScanError();
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("Measage_GetCardErr"));
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }

                LCD_time->stop();
                disconnect(LCD_time, SIGNAL(timeout()), this, SLOT(LCDtimer()));
                delete LCD_time;
                KeyInput.clear();
                SamEdit->clear();
                KeyInput.clear();
                SamEdit->clear();
                if(UiType == LotKeyui)
                {
                    CardKeyWidget->close();
                }
                InfoWidget->close();
                TestWidget->close();
                QString main = AglDeviceP->GetConfig("station_Main");
                Label_station->setText(main);
                UiType = MainUi;
                isClicked = false;
                isGetBarCodeDataError = false;
                isClickedCloseDoor = false;

                isStart = false;
                isNext = false;
                isMessage = false;
                if(isTimer_CodeStart)
                {
                    qDebug() << "225 Stop Timer";
                    Timer_ScanLOT->stop();
                }
                isReadComData = false;
                return;
            }
            IError++;
            isReadComData = false;
            return;
        }
        if(false == isRead0OK)
        {
            qDebug() << "======Reader 0OK";
            QByteArray Station = AglDeviceP->agl_rd_data(0x0D, 0);
            if("0OK" == (QString)Station.data())
            {
                qDebug() << "<><><><><><> Read Data 0OK Over";
                bool iRet =  AglDeviceP->Scan1D_Code(Scan1Data);
                if(false == iRet || Scan1Data.length() == 0)
                {
                    emit Move_Motor();
                    iRead_1_ok++;
                }
                else       // 扫描成功
                {
                    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x0D,0x00,0x01,0x02,0xff,0xff,'\n',0x55,0x55};
                    //--------------------------------//
                    if(!AglDeviceP->agl_wr_data(CmdBuf,16))
                    {
                        qDebug() << "Write BUF ERROR";
                        return;
                    }
                    isReadEND_OK = true;
                }
                isRead0OK = true;
            }
            isReadComData = false;
            return;
        }
        if(true == isReadOK && true == isRead0OK)
        {
            qDebug() << "======Reader 1OK";
            QByteArray Station = AglDeviceP->agl_rd_data(0x0D, 0);
            if("1OK" == (QString)Station.data())
            {
                qDebug() << "<><><><><><> Read Data 1OK Over";
                bool iRet =  AglDeviceP->Scan1D_Code(Scan1Data);
                if(iRead_1_ok > 10)
                {
                    if(isTimer_CodeStart)
                    {
                        qDebug() << "274 Stop Timer";
                        Timer_ScanLOT->stop();
                    }
                    isReadComData = false;
                    isReadOK = false;
                    isRead0OK = false;
                    isReadEND_OK = false;
                    iRead_1_ok = 0;
                    Scan1Data.clear();
                    HandleScanError();
                    return;
                }
                if(false == iRet || Scan1Data.length() == 0)
                {
                    emit Move_Motor();
                    iRead_1_ok++;
                }
                else       // 扫描成功
                {
                    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x0D,0x00,0x01,0x02,0xff,0xff,'\n',0x55,0x55};
                    //--------------------------------//
                    if(!AglDeviceP->agl_wr_data(CmdBuf,16))
                    {
                        qDebug() << "Write BUF ERROR";
                        return;
                    }
                    isReadEND_OK = true;
                }
            }
        }
        isReadComData = false;
    }
}

bool PctGuiThread::HandleScanOK(QByteArray ScanData)
{
    Sam = ScanData.data();
    int iRent = Sql->CountBatchSql();
    if(iRent == -1)
    {
        qDebug() << "7969 Count Batch DB ERROR";
        isNext = false;
        isStart = false;
        return false;
    }
    QByteArray TextRet;
    if(!Sql->ListBatchSql(TextRet, iRent))
    {
        qDebug() << "7975 List Batch DB ERROR";
        isNext = false;
        isStart = false;
        return false;
    }
    QString TextStr = TextRet.data();
    QStringList Textlist = TextStr.split("///");
    QStringList BatchList;
    QByteArray Dataone;
    QString Dataone_Str;
    QStringList Dataone_list;
    QString anfound;
    bool isFound = false;
    int BatchTime = -1;
    BatchTimes = -1;
    for(int idex = 0; idex < iRent ; idex++)
    {
//                if(!Sql->SelectBatchSql(Dataone ,0 ,Textlist.at(idex)))         // 按批号遍历批号数据库
        TextStr = Textlist.at(idex);
        BatchList = TextStr.split("//");
        if(!Sql->SelectBatchSql(Dataone ,0 ,BatchList.at(0)))         // 按批号遍历批号数据库
        {
            qDebug() << "7990 Select Batch DB ERROR";
            isNext = false;
            isStart = false;
            return false;
        }
        Dataone_Str =  Dataone.data();
        Dataone_list = Dataone_Str.split(",");

        qDebug() << "Dataone_list.at(1) = " << Dataone_list.at(1);
        qDebug() << "Sam = " << Sam;

        if(Dataone_list.at(1) == Sam)           // 一维码匹配二维码
        {
            //-----------------------------Add---------------------------//
            bool ok1 = Sql->GetBatchUseTime(Sam, BatchTime);
            BatchTimes = BatchTime;
            //-------------------------------------------------------------//
            bool ok = Sql->GetDetailInforBatchSql(anfound,Dataone_list.at(1));
            if(ok == false || ok1 == false || -1 == BatchTime)
            {
                Voice_ScanError();
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("Message_nores"));
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                isNext = false;
                isStart = false;
                isMessage = false;
                return false;
            }

            isFound = true;
            break;
        }
        Dataone.clear();
    }
    if(isFound == true)
    {
        if(!Code2Aanly(anfound, &AnaFound))
        {
            isNext = false;
            isStart = false;
            return false;
        }
        bool ok = false;
        int times = AnaFound.PiLiang.toInt(&ok);
        if(ok == false)
        {
            Voice_ScanError();
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_nores"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isNext = false;
            isStart = false;
            isMessage = false;
            return false;
        }
        if(BatchTime >= times)              // 批号的有效次数
        {
            Voice_ScanError();
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_Expired"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isNext = false;
            isStart = false;
            isMessage = false;
            return false;
        }
        isNext = false;
        return true;
    }
    else
    {
        Voice_ScanError();
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Sam_Nofound"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isNext = false;
        isStart = false;
        isMessage = false;
        return false;
    }
}

void PctGuiThread::HandleScanError()
{
    // 当前Inforui
    Voice_ScanError();
    isMessage = true;
    msgBox = new QMessageBox(MainWindowP);
    msgBox->setText(AglDeviceP->GetConfig("Message_yiweima"));
    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
    msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::No);
    msgBox->setIcon(QMessageBox :: Question);
    int ret = msgBox->exec();
    switch (ret)
    {
        case QMessageBox::Yes:
        {
            Voice_pressKey();
            delete msgBox;
            isMessage = false;

            UiType = LotKeyui;
            isStart = false;
            CardKeyWidget = new QWidget(InfoWidget);
            CardKeyWidget->setGeometry(250, 55, 290, 240);
            CardKeyWidget->setStyleSheet(QStringLiteral("background-color: rgb(58, 58, 58);"));

            Edid_LOT  = new myLineEdit(CardKeyWidget);
            Edid_LOT->setGeometry(10, 15, 270, 35);
            Edid_LOT->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));

            QGridLayout *gridLayout_key = new QGridLayout(CardKeyWidget);
            gridLayout_key->setSpacing(6);
            gridLayout_key->setVerticalSpacing(1);
            gridLayout_key->setContentsMargins(10, 50, 10, 0);

            QFont font_key;
            font_key.setPointSize(14);

            Mybutton *button_0 = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_0, 3, 1, 1, 1);
            button_0->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_0->setText(tr("0")); button_0->setFixedHeight(35); button_0->setFont(font_key);
            Mybutton *button_1 = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_1, 0, 0, 1, 1);
            button_1->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_1->setText(tr("1")); button_1->setFixedHeight(35); button_1->setFont(font_key);
            Mybutton *button_2 = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_2, 0, 1, 1, 1);
            button_2->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_2->setText(tr("2")); button_2->setFixedHeight(35); button_2->setFont(font_key);
            Mybutton *button_3 = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_3, 0, 2, 1, 1);
            button_3->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_3->setText(tr("3")); button_3->setFixedHeight(35); button_3->setFont(font_key);
            Mybutton *button_4 = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_4, 1, 0, 1, 1);
            button_4->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_4->setText(tr("4")); button_4->setFixedHeight(35); button_4->setFont(font_key);
            Mybutton *button_5 = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_5, 1, 1, 1, 1);
            button_5->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_5->setText(tr("5")); button_5->setFixedHeight(35); button_5->setFont(font_key);
            Mybutton *button_6 = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_6, 1, 2, 1, 1);
            button_6->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_6->setText(tr("6")); button_6->setFixedHeight(35); button_6->setFont(font_key);
            Mybutton *button_7 = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_7, 2, 0, 1, 1);
            button_7->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_7->setText(tr("7")); button_7->setFixedHeight(35); button_6->setFont(font_key);
            Mybutton *button_8 = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_8, 2, 1, 1, 1);
            button_8->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_8->setText(tr("8")); button_8->setFixedHeight(35); button_8->setFont(font_key);
            Mybutton *button_9 = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_9, 2, 2, 1, 1);
            button_9->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_9->setText(tr("9")); button_9->setFixedHeight(35); button_9->setFont(font_key);
            Mybutton *button_sure = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_sure, 3, 0, 1, 1);
            button_sure->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_sure->setText(tr("OK")); button_sure->setFixedHeight(35); button_sure->setFont(font_key);
            Mybutton *button_clear = new Mybutton(CardKeyWidget); gridLayout_key->addWidget(button_clear, 3, 2, 1, 1);
            button_clear->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
            button_clear->setText(tr("Delete")); button_clear->setFixedHeight(35); button_clear->setFont(font_key);

            connect(button_0, SIGNAL(clicked()), this, SLOT(Input_Key_0()),Qt::UniqueConnection);
            connect(button_1, SIGNAL(clicked()), this, SLOT(Input_Key_1()),Qt::UniqueConnection);
            connect(button_2, SIGNAL(clicked()), this, SLOT(Input_Key_2()),Qt::UniqueConnection);
            connect(button_3, SIGNAL(clicked()), this, SLOT(Input_Key_3()),Qt::UniqueConnection);
            connect(button_4, SIGNAL(clicked()), this, SLOT(Input_Key_4()),Qt::UniqueConnection);
            connect(button_5, SIGNAL(clicked()), this, SLOT(Input_Key_5()),Qt::UniqueConnection);
            connect(button_6, SIGNAL(clicked()), this, SLOT(Input_Key_6()),Qt::UniqueConnection);
            connect(button_7, SIGNAL(clicked()), this, SLOT(Input_Key_7()),Qt::UniqueConnection);
            connect(button_8, SIGNAL(clicked()), this, SLOT(Input_Key_8()),Qt::UniqueConnection);
            connect(button_9, SIGNAL(clicked()), this, SLOT(Input_Key_9()),Qt::UniqueConnection);
            connect(button_sure, SIGNAL(clicked()), this, SLOT(Input_Key_OK()),Qt::UniqueConnection);
            connect(button_clear, SIGNAL(clicked()), this, SLOT(Input_Key_Delete()),Qt::UniqueConnection);

            CardKeyWidget->show();
            Label_station->setText(AglDeviceP->GetConfig("Label_LOT")); // yuanlai xianshi

            break;
        }
        case QMessageBox::No:
        {
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            delete msgBox;
            isMessage = false;
            isNext = false;
            isStart = false;
            isGetBarCodeDataError = true;

            isBarCodeGetOK = false;
            isClickedCloseDoor = false;

            return;
        }
    }
}

void PctGuiThread::StartTestWidget()
{
    if(isTimer_CodeStart)
    {
        qDebug() << "576 Stop Timer";
        Timer_ScanLOT->stop();
    }

    CreatResuWidget();
    ResWidget->hide();
    //----------------------------------------------------------------------------//
    if(Radio_after->isChecked())        // 延时开始
    {
        iTimer = 1;
        QString inforti = Edit_Mea->text();
        QString infosta = AglDeviceP->GetConfig("Infor_station");
        Label_station->setText(infosta);
        Ttime = QTime::fromString(inforti+".00", "m.s");
//-------------------Add---------------------------------------------------------------------//
        Label_Sam_show->hide();
        Label_Sam_1->hide();
        Label_Sam_2->hide();
        Label_Sam_3->hide();
        Label_Sam_4->hide();
        Radio_now->hide();
        Radio_after->hide();
        Label_Mea->hide();
        Edit_Mea->hide();
        InfoBut_start->hide();
        Label_InfoTit->setText(AglDeviceP->GetConfig("Title_delay"));
//---------------------------------------------------------------------------------------------//
        Label_Ti->setText(Ttime.toString("mm:ss") + " " + AglDeviceP->GetConfig("Infor_time"));
        Label_Ti->show();
        LCD_time->setInterval(1000);
        LCD_time->start();
        Label_station->setText(AglDeviceP->GetConfig("Title_delay"));
        isCount = true;
        Radio_after->setDisabled(true);
        Radio_now->setDisabled(true);
        isStart = false;
    }
    else                                                // 立即开始
    {
        isStart = true;
        Label_testing->show();
        TestBar->show();
        Radio_after->hide();
        Radio_now->hide();
        Label_Mea->hide();
        Edit_Mea->hide();
        if((int)AglDeviceP->AglDev.ChxVol == 1)
        {
            UiTimer->setInterval(190);
//                   UiTimer->setInterval(135);
        }
        else
        {
//                UiTimer->setInterval(210);
            UiTimer->setInterval(290);
        }
//            //------------------------------Add----07-11-14--------Record time-----------------------------------//
//            QString TimeData1 = "Test Timer Start Time:     ";
//            QString Surrenttime1 = QTime::currentTime().toString("hh:mm:ss.zzz");
//            TimeData1 += Surrenttime1 + "\r\n";
//            RecordTime(TimeData1);
//             qDebug() << "Test timer start  need record time ";
//            //----------------------------------------------------------------------------------------------------------------//
        UiTimer->start();
//-----------Add--------------------------------------------------------------------------------------------------//
        if(AglDeviceP->GetPasswd("@AUTO") == "YES")
        {
            bool ok;
            QString LisIp = AglDeviceP->GetPasswd("@LisIP");
            quint16 LisPort = AglDeviceP->GetPasswd("@LisPort").toUInt(&ok, 10);
            if(!LisSocket->open(QIODevice :: ReadWrite))
            {
                qDebug() << "'Open Socket error";
                isNext = false;
                isStart = false;
                return;
            }
            LisSocket->connectToHost(LisIp, LisPort);
        }
        else if(AglDeviceP->GetPasswd("@AUTO") == "No")
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_nopass"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isNext = false;
            isStart = false;
            isMessage = false;
            return;
        }
//-------------------------------------------------------------------------------------------------------------------//
        AnalyTest();
        if(isYmaxError != true)
        {
            AnalyValue();
        }
//            //------------------------------Add----07-11-14--------Record time-----------------------------------//
//            QString TimeData = "Count Result Over Time:    ";
//            QString Surrenttime = QTime::currentTime().toString("hh:mm:ss.zzz");
//            TimeData += Surrenttime + "\r\n";
//            RecordTime(TimeData);
//             qDebug() << "Test Over need record time ";
//            //----------------------------------------------------------------------------------------------------------------//
        qDebug() << "------------------------Test Over------------------------";
    }
}

void PctGuiThread::Input_Key_0()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.length() > 8)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang_LOT"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    LOT_Data += "0";
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread::Input_Key_1()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.length() > 8)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang_LOT"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    LOT_Data += "1";
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread::Input_Key_2()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.length() > 8)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang_LOT"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    LOT_Data += "2";
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread::Input_Key_3()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.length() > 8)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang_LOT"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    LOT_Data += "3";
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread::Input_Key_4()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.length() > 8)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang_LOT"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    LOT_Data += "4";
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread::Input_Key_5()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.length() > 8)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang_LOT"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    LOT_Data += "5";
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread::Input_Key_6()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.length() > 8)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang_LOT"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    LOT_Data += "6";
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread::Input_Key_7()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.length() > 8)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang_LOT"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    LOT_Data += "7";
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread::Input_Key_8()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.length() > 8)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang_LOT"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    LOT_Data += "8";
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread::Input_Key_9()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.length() > 8)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang_LOT"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    LOT_Data += "9";
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread::Input_Key_OK()
{
    static bool isClickedHandle = false;
    if(true == isClickedHandle)
    {
        return;
    }
    if(Edid_LOT->text().isEmpty())
    {
        Voice_pressKey();
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_empty"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        isClickedHandle = false;
        isBarCodeGetOK = false;
        isClickedCloseDoor = false;
        isGetBarCodeDataError = true;
        return;
    }
    isClickedHandle = true;
    QByteArray BarCodeData;
    BarCodeData.append(Edid_LOT->text());

    if(!HandleScanOK(BarCodeData))
    {
        qDebug() << "Search BarCode Error";
        isClickedHandle = false;
//        isBarCodeGetOK = false;
        isClickedCloseDoor = false;
        isGetBarCodeDataError = true;
        return;
    }

    Voice_pressKey();

    CardKeyWidget->close();
    UiType = Inforui;
    isClickedHandle = false;
    Label_station->setText(AglDeviceP->GetConfig("Station_readly"));
    StartTestWidget();
}

void PctGuiThread::Input_Key_Delete()
{
    Voice_pressKey();
    QString LOT_Data = Edid_LOT->text();
    if(LOT_Data.isEmpty())
    {
        return;
    }

    LOT_Data = LOT_Data.remove(LOT_Data.length() - 1, 1);
    Edid_LOT->setText(LOT_Data);
}

void PctGuiThread :: run()
{    
    qDebug() << "pctGuiTHread is runing";
}

void PctGuiThread :: DispStarBmp()  //开机画面
{
    qDebug() << "pass is " << AglDeviceP->GetPasswd("@Languag");
//    QString hostIp = GetPassConfig("hostIP");
    QString hostIp = AglDeviceP->GetPasswd("@hostIP");
    if(!AglDeviceP->SetLocalIp(hostIp))
    {
        qDebug() << "Set Local IP Error!";
    }

    AglDeviceP->agl_rd_stat();
    Door_flush = (int)AglDeviceP->AglDev.OutIntState;

    UiType = StarBmpUi;   // 界面标识
    QPalette palette;         // 背景
    //palette.setColor(QPalette::Background,QColor(77, 166,177)); // 进度条颜色
    // 开机进度条
    ProgressBar = new QProgressBar(MainWindowP);
    palette.setColor(QPalette::Background,QColor(222,135,121)); // 进度条背景颜色
    ProgressBar->setAutoFillBackground(true);
    ProgressBar->setPalette(palette);
    ProgressBar->setAlignment(Qt::AlignCenter);
    ProgressBar->setGeometry(50,360,700,50);
    ProgressBar->setValue(0);
    // 设置窗口无边框
    MainWindowP->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowSystemMenuHint);
    MainWindowP->setAutoFillBackground(true);  //背景使能
    // 创建QPixmap类型的对象关联背景图片
    QPixmap pix;
    QString loadpix = AglDeviceP->GetPasswd("@pix_load");   // Logo1为一芯启动画面 logo为biohit
    bool flag=pix.load(loadpix);
    if(!flag)
    {
        qDebug("get logo.png is not ok!");
    }
    palette.setBrush(QPalette::Background,QBrush(pix));
    palette.setColor(QPalette::WindowText,QColor(251,251,251));  // 进度条字体颜色
    MainWindowP->setPalette(palette);  //使能
    //字体字号
     QFont qFont;
     qFont.setPointSize(14);

     //设置字体
     MainWindowP->setFont(qFont);
     MainWindowP->show();
     MainWindowP->repaint();

     Delayms(30);
     //启动定时器
     UiTimer = new QTimer(this);
     UiTimer->setInterval(400);
     connect(UiTimer, SIGNAL(timeout()), this, SLOT(UiTimeOut()),Qt::UniqueConnection);
     UiTimer->start();

     Time_Widget = new QTimer(this);
     connect(Time_Widget, SIGNAL(timeout()), this, SLOT(ChangeWidget()),Qt::UniqueConnection);
     Time_Widget->start(1000);
}
//------Add--------------------------------------------------------------------------------------------------------------------------------------------//

void PctGuiThread::CheckRunError()
{
    QCoreApplication::processEvents();
    if(AglDeviceP->Error_Code != Error_OK || check_type == Check_NoNeed)
    {
        CheckTime->stop();
    }
    if(AddCheck == 5)
    {
        AddCheck = 0;
        check_type = Check_NoNeed;
        CheckTime->stop();
    }

    qDebug() << "check type = " << check_type;

    switch(check_type)
    {
        case Check_OutDoor:
        {
            AddCheck++;
//            AglDeviceP->GetCheckError();
            if(AddCheck == 2)
            {
                AddCheck = 0;
                check_type = Check_NoNeed;
                CheckTime->stop();
            }
            break;
        }
        case Check_Start:
        {
            AddCheck++;
            AglDeviceP->GetCheckError();
            if(AddCheck == 3)
            {
                AddCheck = 0;
                AglDeviceP->agl_rd_ver();                                 // 获取荧光电机控制板的程序版本
                QString Version =  AglDeviceP->AglDev.Version;
                if(Version.length() != 15)
                {
                    AglDeviceP->agl_rd_pm();                                // 获取荧光电机控制板的电机，荧光电流，温度参数
                }

                check_type = Check_NoNeed;
                CheckTime->stop();
            }
            break;
        }
        case Check_InDoor:
        {
            AddCheck++;
//            AglDeviceP->GetCheckError();
            if(AddCheck == 5)
            {
                AddCheck = 0;
                check_type = Check_NoNeed;
                CheckTime->stop();
            }
            break;
        }
        case Check_InDoor_Test:
        {
            AddCheck++;
//            AglDeviceP->GetCheckError();
            if(AddCheck == 2)
            {
                AddCheck = 0;
                check_type = Check_NoNeed;
                CheckTime->stop();
            }
            break;
        }
        case Check_Calib_Run:
        {
            AddCheck++;
            AglDeviceP->GetCheckError();
            if(isCalibrationFourCard == true)
            {
                if(AddCheck == 15)
                {
                    AddCheck = 0;
                    check_type = Check_NoNeed;
                    CheckTime->stop();
                }
             }
            else
            {
                if(AddCheck == 6)
                {
                    AddCheck = 0;
                    check_type = Check_NoNeed;
                    CheckTime->stop();
                }
            }
            break;
        }
        case Check_Run:
        {
            AglDeviceP->GetCheckError();
            break;
        }
        default :
        {
            break;
        }
    }
    if(AglDeviceP->Error_Code == Error_X)
    {
        QMessageBox *msgBox_Check = new QMessageBox(MainWindowP);
        msgBox_Check->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox_Check->setText(AglDeviceP->GetConfig("Message_XErr"));
        msgBox_Check->setStandardButtons(QMessageBox::NoButton);
        msgBox_Check->setIcon(QMessageBox :: Critical);
        msgBox_Check->exec();
    }
    else if(AglDeviceP->Error_Code == Error_Y)
    {
        QMessageBox *msgBox_Check = new QMessageBox(MainWindowP);
        msgBox_Check->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox_Check->setText(AglDeviceP->GetConfig("Message_YErr"));
        msgBox_Check->setStandardButtons(QMessageBox::NoButton);
        msgBox_Check->setIcon(QMessageBox :: Critical);
        msgBox_Check->exec();
    }
    else if(AglDeviceP->Error_Code == Error_Y_Max)
    {
        if(isStart == true)
        {
            isYmaxError = true;
            CheckTime->stop();
            check_type = Check_NoNeed;
        }
        else
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_YMaxErr_Door"));
            msgBox->setIcon(QMessageBox ::Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;

            Voice_pressKey();
            AglDeviceP->Error_Code = Error_OK;
            check_type = Check_NoNeed;
        }
    }
    QCoreApplication::processEvents();
}

void PctGuiThread::ChangeWidget()
{
    if(AglDeviceP->Error_Code == Error_OK)
    {
        if(iShowTime == 300)
        {
            if(isWidget == false)
            {
                AglDeviceP->SysCall("echo 1 >> /sys/class/backlight/pwm-backlight.0/brightness");
                isWidget = true;
                if(isMessage_Save == true)
                {
                    msgBox_Save->close();
                }
                if(isMessage_child == true)
                {
                    msgBox_child->close();
                }
                else
                {
                    if(isMessage == true)
                    {
                        qDebug() << "delete messagebox";
                        msgBox->close();
                    }
                }
            }
        }
        else if(iShowTime == 1800)
        {
            AglDeviceP->SysCall("echo 0 >> /sys/class/backlight/pwm-backlight.0/brightness");
        }
        iShowTime++;
        if(iShowTime > 2000)
        {
            iShowTime = 2000;
        }
    }
}

void PctGuiThread :: Socket_connected()
{
    isConnect = true;
    qDebug() << "网络被连接，触发网络连接槽函数";
}

void PctGuiThread :: Socket_disconnected()
{
    isConnect = false;
    qDebug() << "网络被断开，触发网络断开槽函数";
}

void PctGuiThread :: Socket_Error(QAbstractSocket :: SocketError Socket_err)
{
    switch(Socket_err)
    {
        case QAbstractSocket::ConnectionRefusedError :
        {
            qDebug() << "连接超时，连接失败";
            break;
        }
        case QAbstractSocket::RemoteHostClosedError :
        {
            qDebug() << "远程主机关闭，连接失败";
            break;
        }
        case QAbstractSocket::HostNotFoundError :
        {
            qDebug() << "远程地址有误，连接失败";
            break;
        }
        default :
        {
//            qDebug() << "其他错误，导致连接失败";
            break;
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------//
void PctGuiThread :: CreatWindow()  // 主界面
{
    UiType = MainUi;

    isIPAddrErr = false;
    iTime = 0;
    isMessage = false;
    isMessage_child = false;
    isMessage_Save = false;
    isGetBarCodeDataError = false;

    isAdmin = false;
    Count_test = 0;
    PctGuiP = this;
    isConnect = false;
    isControl = false;
    isPress = false;
    isKeyWidget = false;

    isCalibration = false;
    isCalibrationFourCard = false;

    isClickedCloseDoor = false;
    isBarCodeGetOK = false;

    isDisconnect = false;
    isSaveDev = false;
    state_door = 0;
    iMessage = 0;
    OBR_index = 0;
    PID_index = 0;
    OBX_index = 0;
    QRD_index = 0;
    iTime = 0;                                                               // 记录密码输错次数
    QStringList PassList = AglDeviceP->GetAllUserData();
    iCount_User = PassList.length();                         // 记录当前一般用户个数
    isUpdate = false;                                                   // 记录是否正在进行文件操作
    isSave = false;                                                        // 记录是不是在在退出前点击保存
    for(int j = 0; j < 11; j++)                                          // 状态改变声音控制的辅助变量
    {
        iChange[j] = 1;
    }
    iChange_socket = 1;
    iCount_icon = 1;                                                   // 状态改变声音控制的辅助变量
    iPower = 1;
    isStart = false;                                                      // 记录是否正在进行试验
    isCount = false;                                                    // 记录是否进入倒计时
    isNext = false;                                                      // 记录是否正在等待一维码扫描结果
    isSelectRes = false;                                             // 记录是否是用键盘查询完结果的界面
    isDoorClicked = false;
    isCanOpenDoor = true;
    isCalib = false;

    mainWind = new MyWidget(MainWindowP);
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));
    mainWind->setAutoFillBackground(true);      //显示设置好的背景
    mainWind->setPalette(palette);
    mainWind->setGeometry(0,0,800,480);

    Mybutton *Button_2 = new Mybutton(mainWind);
    Mybutton *Button_3 = new Mybutton(mainWind);
    Mybutton *Button_1 = new Mybutton(mainWind);
    Button_Door = new Mybutton(mainWind);
    Button_1->setGeometry(336, 170, 128, 128);
    Button_2->setGeometry(568, 170, 128, 128);
    Button_3->setGeometry(104, 170, 128, 128);
    Button_Door->setGeometry(741, 370, 48, 48);

    QPixmap ima_door;
    QString doorload = AglDeviceP->GetPasswd("@Main_door");
    ima_door.load(doorload);
    Mainpix_door = ima_door.scaled(Button_Door->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_Door, Mainpix_door);

    QPixmap ima_test;
    QString testload = AglDeviceP->GetPasswd("@Main_3");
    ima_test.load(testload);
    Mainpix_3 = ima_test.scaled(Button_3->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_3, Mainpix_3);

    QPixmap ima_set;
    QString setload = AglDeviceP->GetPasswd("@Main_2");
    ima_set.load(setload);
    Mainpix_2 = ima_set.scaled(Button_2->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_2, Mainpix_2);

    QPixmap ima_res;
    QString resload = AglDeviceP->GetPasswd("@Main_1");
    ima_res.load(resload);
    Mainpix_1 = ima_res.scaled(Button_1->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_1, Mainpix_1);

    QLabel *Label_button1 = new QLabel(mainWind);
    QLabel *Label_button2 = new QLabel(mainWind);
    QLabel *Label_button3 = new QLabel(mainWind);

    QPalette palette_button;
    palette_button.setColor(QPalette::Background, QColor(251,251,251));
    palette_button.setColor(QPalette::WindowText,QColor(0, 0, 0));

    QString Main_test = AglDeviceP->GetConfig("Main_button1");
    QString Main_test1 = AglDeviceP->GetConfig("Main_button1_1");
    Label_button1->setGeometry(104, 298, 128, 70);
    Label_button2->setGeometry(568, 298, 128, 70);
    Label_button3->setGeometry(336, 298, 128, 70);
    Label_button1->setAlignment(Qt::AlignCenter);
    Label_button1->setAutoFillBackground(true);
    Label_button1->setText(Main_test + "\r\n" +Main_test1);
    Label_button1->setPalette(palette_button);

    QString Main_set = AglDeviceP->GetConfig("Main_button2");
    QString Main_set1 = AglDeviceP->GetConfig("Main_button2_1");
    Label_button2->setAlignment(Qt::AlignCenter);
    Label_button2->setAutoFillBackground(true);
    Label_button2->setText(Main_set + "\r\n" + Main_set1);
    Label_button2->setPalette(palette_button);

    QString Main_res = AglDeviceP->GetConfig("Main_button3");
    QString Main_res1 = AglDeviceP->GetConfig("Main_button3_1");
    Label_button3->setAlignment(Qt::AlignCenter);
    Label_button3->setAutoFillBackground(true);
    Label_button3->setText(Main_res + "\r\n" + Main_res1);
    Label_button3->setPalette(palette_button);

    MyGroup *group1 = new MyGroup(mainWind);
    MyGroup *group2 = new MyGroup(mainWind);
    MyGroup *group3 = new MyGroup(mainWind);
    group1->setStyle(QStyleFactory::create("windows"));
    group2->setStyle(QStyleFactory::create("windows"));

    group1->setGeometry(-2, -5, 805, 82);
    group2->setGeometry(-3, 424, 805, 62);
    group3->setGeometry(-3, 423, 805, 3);
    group1->setStyleSheet(QStringLiteral("background-color: rgb(0,173,255);"));
    group2->setStyleSheet(QStringLiteral("background-color: rgb(0,173,255);"));
    group3->setStyleSheet(QStringLiteral("background-color: rgb(0,173,255);"));
    //#########################################################//
//    Button_pic = new Mybutton(group1);
//    Button_pic->setGeometry(300,10,100,30);
//    Button_pic->setStyleSheet(QStringLiteral("background-color: rgb(165,165,165);"));
//    Button_pic->setText(tr("截屏"));
//    Button_pic->show();
//    connect(Button_pic, SIGNAL(clicked()), this, SLOT(pressButton_pic()));
    //#########################################################//
    Label_HB = new QLabel(group1);
    Label_net  = new QLabel(group1);
//    Label_Q = new QLabel(group1);
    Label_SD = new QLabel(group1);
    Label_station = new QLabel(group2);
    Label_time = new QLabel(group2);
    Label_title = new QLabel(group1);
    Label_U = new QLabel(group1);
    //--------------Add------06-13----------------------------//
    Label_station->setStyleSheet(QStringLiteral("color: rgb(255, 255 ,255);"));
    Label_title->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
    Label_time->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
    //-------------------------------------------------------------//
//    Label_SD->setGeometry(490, 18, 50, 50);
//    Label_net->setGeometry(570, 18, 50 ,50);
//    Label_U->setGeometry(650, 18, 50, 50);
//    Label_Q->setGeometry(730, 18, 50, 50);

    Label_SD->setGeometry(570, 18, 50, 50);
    Label_U->setGeometry(650, 18, 50, 50);
    Label_net->setGeometry(730, 18, 50 ,50);

    Label_time->setGeometry(575, 15, 220, 30);
    Label_HB->setGeometry(15, 48, 250, 30);
    Label_title->setGeometry(15, 26, 130, 30);
    Label_station->setGeometry(15, 15, 600, 30);

    QString Main_station = AglDeviceP->GetConfig("station_Main");
    QString Main_title = AglDeviceP->GetConfig("title_text");
    Label_station->setText(Main_station);
    Label_title->setText(Main_title);

//    QString imageQ1 = AglDeviceP->GetPasswd("@image_Q_0");
//    QString imageQ3 = AglDeviceP->GetPasswd("@image_Q_2");
//    QString imageQ4 = AglDeviceP->GetPasswd("@image_Q_3");
//    QString imageQ5 = AglDeviceP->GetPasswd("@image_Q_4");
//    QString imageQ6 = AglDeviceP->GetPasswd("@image_Q_5");

//    image_Q[0].load(imageQ1);
//    image_Q[1].load(imageQ3);
//    image_Q[2].load(imageQ4);
//    image_Q[3].load(imageQ5);
//    image_Q[4].load(imageQ6);

//    for(int i = 0; i < 5; i++)
//    {
//       result_Q[i] = this->image_Q[i].scaled(Label_Q->size(), Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
//    }

    QString imagenet1 = AglDeviceP->GetPasswd("@image_net_0");
    QString imagenet2 = AglDeviceP->GetPasswd("@image_net_1");
    QString image_socket = AglDeviceP->GetPasswd("@image_socket");

    image_net[0].load(imagenet1);
    image_net[1].load(imagenet2);
    image_Socket.load(image_socket);

    QString imagesd = AglDeviceP->GetPasswd("@image_SD_0");
    QString imageu = AglDeviceP->GetPasswd("@image_U_0");

    image_SD.load(imagesd);
    image_U.load(imageu);
    result_net[0] = image_net[0].scaled(Label_net->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    result_net[1] = image_net[1].scaled(Label_net->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    result_SD = image_SD.scaled(Label_SD->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    result_U = image_U.scaled(Label_U->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    result_socket = image_Socket.scaled(Label_net->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);

    flush_time();
    flush_icon();

    timer_time = new QTimer(this);
    door_time = new QTimer(this);

    connect(Button_1,SIGNAL(clicked()),this,SLOT(pressButton_1()),Qt::UniqueConnection);
    connect(Button_2,SIGNAL(clicked()),this, SLOT(pressButton_2()),Qt::UniqueConnection);
    connect(Button_3,SIGNAL(clicked()),this,SLOT(pressButton_3()),Qt::UniqueConnection);
    connect(Button_Door,SIGNAL(clicked()),this,SLOT(pressButton_Door()),Qt::UniqueConnection);
    connect(door_time,SIGNAL(timeout()),this,SLOT(flush_door_time()),Qt::UniqueConnection);
    connect(timer_time,SIGNAL(timeout()),this,SLOT(flush_time()),Qt::UniqueConnection);
    connect(timer_time,SIGNAL(timeout()),this,SLOT(flush_icon()),Qt::UniqueConnection);
    timer_time->start(1000);
    door_time->start(100);
    mainWind->show();

//    AglDeviceP->SysCall("rm -rf *.db *.bak");
}

void PctGuiThread :: CreatSetWidget()             // 创建设置界面
{
    UiType = Setui;
    isBatchError = false;
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));

    QPalette palette_button;
    palette_button.setColor(QPalette::Background, QColor(251,251,251));
    palette_button.setColor(QPalette::WindowText,QColor(0,0,0));
//------------------------------------------------------------------------------------------------//    设置界面主界面
    SetWidget = new QWidget(mainWind);
    SetWidget->setAutoFillBackground(true);
    SetWidget->setGeometry(0, 75, 805, 348);
    SetWidget->setPalette(palette);

    Mybutton *Button_2 = new Mybutton(SetWidget);
    Mybutton *Button_3 = new Mybutton(SetWidget);
    Mybutton *Button_set = new Mybutton(SetWidget);
    Mybutton *Button_Home = new Mybutton(SetWidget);

    Button_Home->setGeometry(13, 11, 48, 48);
    Button_set->setGeometry(186, 88, 128, 128);
    Button_2->setGeometry(485, 88, 128, 128);
    Button_3->setGeometry(11, 290, 48, 48);

    QPixmap ima_home;
    QString homeload = AglDeviceP->GetPasswd("@MainSet_Home");
    ima_home.load(homeload);
    MainSetpix_Home = ima_home.scaled(Button_Home->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_Home, MainSetpix_Home);

    QPixmap ima_left;
    QString leftload = AglDeviceP->GetPasswd("@MainSet_3");
    ima_left.load(leftload);
    MainSetpix_3 = ima_left.scaled(Button_3->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_3, MainSetpix_3);

    QPixmap ima_dev;
    QString ldevload = AglDeviceP->GetPasswd("@MainSet_2");
    ima_dev.load(ldevload);
    MainSetpix_2 = ima_dev.scaled(Button_2->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_2, MainSetpix_2);

    QPixmap ima_num;
    QString numload = AglDeviceP->GetPasswd("@MainSet_1");
    ima_num.load(numload);
    MainSetpix_1 = ima_num.scaled(Button_set->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_set, MainSetpix_1);

    QLabel *Label_button1 = new QLabel(SetWidget);
    QLabel *Label_button2 = new QLabel(SetWidget);

    QString Instru = AglDeviceP->GetConfig("Set_button1");
    QString Instru1 = AglDeviceP->GetConfig("Set_button1_1");
    QString Lot = AglDeviceP->GetConfig("Set_button2");
    QString Lot1 = AglDeviceP->GetConfig("Set_button2_1");

    Label_button2->setGeometry(449, 216, 200, 70);
    Label_button2->setAlignment(Qt::AlignCenter | Qt::AlignTop);
    Label_button2->setAutoFillBackground(true);
    Label_button2->setText(Lot + "\r\n" + Lot1);
    Label_button2->setPalette(palette_button);

    Label_button1->setGeometry(150, 216, 200, 70);
    Label_button1->setAlignment(Qt::AlignCenter | Qt::AlignTop);
    Label_button1->setAutoFillBackground(true);
    Label_button1->setText(Instru + "\r\n" + Instru1);
    Label_button1->setPalette(palette_button);
//------------------------------------------------------------------------------------------------//    批号设置
    Widget_num = new QWidget(MainWindowP);
    Widget_num->setStyle(QStyleFactory::create("windows"));
    Widget_num->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Widget_num->setGeometry(0, 75, 803, 348);
    iBat = Sql->CountBatchSql();
    if(-1 == iBat)
    {
        qDebug() << "447 Count Batch DB Error";
        return;
    }

    QFont qFont;
    qFont.setPointSize(16);
    QFont qFont_Button;
    qFont_Button.setPointSize(14);

    QString Manage = AglDeviceP->GetConfig("SetTitel");
    QLabel *SetTitelLabel = new QLabel(Widget_num);
    SetTitelLabel->setGeometry(250, 10, 300, 40);
    SetTitelLabel->setFont(qFont);
    SetTitelLabel->setAlignment(Qt::AlignCenter);
    SetTitelLabel->setText(Manage);

    table_num = new MyTableWidget(/*iBat, 1, */Widget_num);
    table_num->setStyle(QStyleFactory::create("fusion"));
//    if(iBat > 5)
//    {
//        table_num->setGeometry(100, 95, 220, 172);
//    }
//    else
//    {
//        table_num->setGeometry(100, 95, 190, 172);
//    }

//    QScrollBar *table_num_bar =  table_num->verticalScrollBar();                                                                                     // 获得该表格的滚动条
//    MyScrollBar *Table_bar = new MyScrollBar(table_num);
//    Table_bar->move(190,5);
//    Table_bar = (MyScrollBar*)table_num_bar;
//    Table_bar->setStyleSheet("QScrollBar:vertical {""width: 30px; background-color: rgb(215,215,215);""}");     // 改变滚动条宽度，颜色
//    Table_bar->setStyle(QStyleFactory::create("fusion"));
//    table_num->setHorizontalScrollBarPolicy(Qt :: ScrollBarAlwaysOff);
//    table_num->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
//    table_num->setSelectionMode(QAbstractItemView :: SingleSelection);                                   //   设置表格中的单元格只能被选中一个
//    for(int i = 0; i < iBat; i++)
//    {
//        table_num->setRowHeight(i, 34);
//    }
//    table_num->setColumnWidth(0, 190);
//    QHeaderView *head_ver = table_num->verticalHeader();
//    head_ver->setHidden(true);
//    QHeaderView *head_hor =  table_num->horizontalHeader();
//    head_hor->setHidden(true);

    QByteArray TextRet;
    if(!Sql->ListBatchSql(TextRet, iBat))
    {
        qDebug() << "List Batch DB Error";
        return;
    }
    QString TextStr = TextRet.data();
    qDebug() << "TextStr is " << TextStr;
    QStringList Textlist = TextStr.split("///");
    QStringList BatchList;
//    int iBatRow = table_num->rowCount();
//--------------------------------Add----------------------------------//
    int CanShow = 0;
    QString ShowData;
    int Compare = 0;
    bool ok = false;
    int Times = 0;
    QString GetData;
    TwoCodeStr Batchfound;
    for(int i = 0; i < iBat; i++)
    {
        TextStr = Textlist.at(i);
        BatchList = TextStr.split("//");
        Compare = BatchList.at(1).toInt();
        ok = Sql->GetPiliang(GetData, BatchList.at(0));
        if(ok == false)
        {
            qDebug() << "GetPiliang is Error";
            isBatchError = true;
            Times = -1;
        }
        if(!Code2Aanly(GetData, &Batchfound))
        {
            qDebug() << "Code2Aanly is Error";
            isBatchError = true;
            Times = -1;
        }
        Times = Batchfound.PiLiang.toInt(&ok);
        if(ok == false)
        {
            qDebug() << "toInt is Error";
            isBatchError = true;
            Times = -1;
        }
        if(Compare < Times)                                  // 批号的有效次数
        {
            ShowData += BatchList.at(0) + "#";
            CanShow++;
        }
    }
    if(isBatchError == true)
    {
        iBat = 0;
    }
    else
    {
        iBat = CanShow;
    }
//-----------------------------------------------------------------------//
    table_num->setColumnCount(1);
    table_num->setRowCount(iBat);
    if(iBat > 5)
    {
        table_num->setGeometry(100, 95, 220, 172);
    }
    else
    {
        table_num->setGeometry(100, 95, 190, 172);
    }

    QScrollBar *table_num_bar =  table_num->verticalScrollBar();                                                                                     // 获得该表格的滚动条
    MyScrollBar *Table_bar = new MyScrollBar(table_num);
    Table_bar->move(190,5);
    Table_bar = (MyScrollBar*)table_num_bar;
    Table_bar->setStyleSheet("QScrollBar:vertical {""width: 30px; background-color: rgb(215,215,215);""}");     // 改变滚动条宽度，颜色
    Table_bar->setStyle(QStyleFactory::create("fusion"));
    table_num->setHorizontalScrollBarPolicy(Qt :: ScrollBarAlwaysOff);
    table_num->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    table_num->setSelectionMode(QAbstractItemView :: SingleSelection);                                   //   设置表格中的单元格只能被选中一个
    for(int i = 0; i < iBat; i++)
    {
        table_num->setRowHeight(i, 34);
    }
    table_num->setColumnWidth(0, 190);
    QHeaderView *head_ver = table_num->verticalHeader();
    head_ver->setHidden(true);
    QHeaderView *head_hor =  table_num->horizontalHeader();
    head_hor->setHidden(true);

    Textlist = ShowData.split("#");
    for(int idex = 0; idex < iBat ; idex++)
    {
        item[idex] = new QTableWidgetItem(Textlist.at(idex));
        if(iBat > 5)
        {
            item[idex]->setTextAlignment(Qt::AlignLeft);
        }
        else
        {
            item[idex]->setTextAlignment(Qt::AlignCenter);
        }
        table_num->setItem(idex, 0, item[idex]);
    }
    connect(table_num,SIGNAL(itemClicked(QTableWidgetItem*)),this,SLOT(ClickedItem_Set()),Qt::UniqueConnection);  // 显示选中单元格内容的信号与槽

    QLabel *Label_pro = new QLabel(Widget_num);
    QLabel *Label_last = new QLabel(Widget_num);
    QLabel *Label_enter = new QLabel(Widget_num);
    QLabel *Label_made = new QLabel(Widget_num);
    QLabel *Label_remark = new QLabel(Widget_num);
    QLabel *Label_useNum = new QLabel(Widget_num);

    QString Lotuse = AglDeviceP->GetConfig("Label_useNum_text");
    Label_useNum->setAlignment(Qt::AlignCenter);
    Label_useNum->setText(Lotuse);
    Label_useNum->setGeometry(125, 60, 140, 30);
    Label_useNum->setPalette(palette_button);
    Label_pro->setGeometry(333, 60, 1000, 30);

    QString Lotpro = AglDeviceP->GetConfig("Label_pro_text");
    Label_pro->setText(Lotpro + "#");
    Label_pro->setPalette(palette_button);
    Label_last->setGeometry(333, 105, 100, 30);

    QString Lotlast = AglDeviceP->GetConfig("Label_last_text");
    Label_last->setText(Lotlast);
    Label_last->setPalette(palette_button);
    Label_enter->setGeometry(333, 150, 140, 30);

    QString Lotent = AglDeviceP->GetConfig("Label_enter_text");
    Label_enter->setText(Lotent);
    Label_enter->setPalette(palette_button);
    Label_made->setGeometry(333, 195, 160, 30);

    QString Lotmade = AglDeviceP->GetConfig("Label_made_text");
    Label_made->setText(Lotmade);
    Label_made->setPalette(palette_button);
    Label_remark->setGeometry(333, 240, 160, 30);

    QString Lotinfo = AglDeviceP->GetConfig("Label_infor");
    Label_remark->setText(Lotinfo);
    Label_remark->setPalette(palette_button);

    Mybutton *Button_return = new Mybutton(Widget_num);
    Mybutton *Button_sweep = new Mybutton(Widget_num);
    Mybutton *Button_Lot_Home = new Mybutton(Widget_num);

    Button_return->setStyle(QStyleFactory::create("fusion"));
    Button_sweep->setStyle(QStyleFactory::create("fusion"));
    Button_return->setGeometry(11, 290, 48, 48);
    Button_SetPix(Button_return, MainSetpix_3);
    Button_Lot_Home->setGeometry(13, 11, 48, 48);
    Button_SetPix(Button_Lot_Home, MainSetpix_Home);

    Button_sweep->setGeometry(741, 290, 48, 48);
    QPixmap ima_cod2;
    QString cod2load = AglDeviceP->GetPasswd("@MainSet_Code_2");
    ima_cod2.load(cod2load);
    MainSetpix_Code_2 = ima_cod2.scaled(Button_sweep->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_sweep, MainSetpix_Code_2);

    Button_return->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Button_sweep->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));

    Edit_pro = new myLineEdit(Widget_num);
    Edit_last = new myLineEdit(Widget_num);
    Edit_max = new myLineEdit(Widget_num);
    Edit_made = new myLineEdit(Widget_num);
    Edit_remark = new myLineEdit(Widget_num);

    Edit_pro->setStyle(QStyleFactory::create("fusion"));
    Edit_last->setStyle(QStyleFactory::create("fusion"));
    Edit_max->setStyle(QStyleFactory::create("fusion"));
    Edit_made->setStyle(QStyleFactory::create("fusion"));
    Edit_remark->setStyle(QStyleFactory::create("fusion"));

    Edit_pro->setGeometry(500, 58, 200, 35);
    Edit_pro->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_pro->setEnabled(false);

    Edit_max->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_max->setGeometry(500, 103, 200, 35);
    Edit_max->setEnabled(false);

    Edit_last->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_last->setEnabled(false);
    Edit_last->setGeometry(500, 147, 200, 35);

    Edit_made->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_made->setEnabled(false);
    Edit_made->setGeometry(500, 192, 200, 35);

    Edit_remark->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_remark->setEnabled(false);
    Edit_remark->setGeometry(500, 238, 200, 35);
    Widget_num->hide();

    connect(Button_Lot_Home,SIGNAL(clicked()),this,SLOT(pressButton_Lot_Home()),Qt::UniqueConnection);
    connect(Button_Home,SIGNAL(clicked()),this,SLOT(pressButton_3()),Qt::UniqueConnection);
    connect(Button_return,SIGNAL(clicked()),this,SLOT(pressButton_return()),Qt::UniqueConnection);
    connect(Button_set,SIGNAL(clicked()),this, SLOT(pressButton_set()),Qt::UniqueConnection);
    connect(Button_2,SIGNAL(clicked()),this, SLOT(pressButton_2()),Qt::UniqueConnection);
    connect(Button_3,SIGNAL(clicked()),this,SLOT(pressButton_3()),Qt::UniqueConnection);
    connect(Button_sweep,SIGNAL(clicked()),this,SLOT(pressButton_sweep()),Qt::UniqueConnection);
    SetWidget->show();
}

void PctGuiThread :: CreatCOSWidget()            // 仪器设置界面
{
    UiType = COSui;
    QFont qFont;
    QPalette palette;

    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));

    QPalette palette_button;
    palette_button.setColor(QPalette::Background, QColor(251,251,251));
    palette_button.setColor(QPalette::WindowText,QColor(0,0,0));

    COSWidget = new QWidget(MainWindowP);
    COSWidget->setAutoFillBackground(true);
    COSWidget->setGeometry(0, 75, 805, 348);
    COSWidget->setPalette(palette);

    Mybutton *Button_Home = new Mybutton(COSWidget);
    Mybutton *Button_2 = new Mybutton(COSWidget);
    Mybutton *Button_3 = new Mybutton(COSWidget);
    Mybutton *Button_dev = new Mybutton(COSWidget);
    Mybutton *Button_Infor = new Mybutton(COSWidget);

    Button_Home->setGeometry(13, 11, 48, 48);
    Button_dev->setGeometry(106, 88, 128, 128);
    Button_2->setGeometry(338, 88, 128, 128);
    Button_Infor->setGeometry(570, 88, 128, 128);
    Button_3->setGeometry(11, 290, 48, 48);

    QString listload = AglDeviceP->GetPasswd("@AppSet_3");
    QPixmap ima_list(listload);
    Selectpix_2 = ima_list.scaled(Button_Infor->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_Infor, Selectpix_2);
    Button_SetPix(Button_3, MainSetpix_3);

    QPixmap ima_user;
    QString userload = AglDeviceP->GetPasswd("@AppSet_2");
    ima_user.load(userload);
    AppSetpix_2 = ima_user.scaled(Button_2->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_2, AppSetpix_2);

    QPixmap ima_cos;
    QString cosload = AglDeviceP->GetPasswd("@AppSet_1");
    ima_cos.load(cosload);
    AppSetpix_1 = ima_cos.scaled(Button_dev->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_dev, AppSetpix_1);
    Button_SetPix(Button_Home, MainSetpix_Home);

    QLabel *Label_button1 = new QLabel(COSWidget);
    QLabel *Label_button2 = new QLabel(COSWidget);
    QLabel *Label_infor = new QLabel(COSWidget);

    Label_button1->setGeometry(70, 216, 200, 70);
    Label_button2->setGeometry(302, 216, 200, 70);
    Label_infor->setGeometry(534, 216, 200, 70);

    QString Dev_manag = AglDeviceP->GetConfig("Dev_button1");
    QString Dev_manag1 = AglDeviceP->GetConfig("Dev_button1_1");
    Label_button1->setAlignment(Qt::AlignCenter);
    Label_button1->setAutoFillBackground(true);
    Label_button1->setPalette(palette_button);
     Label_button1->setText(Dev_manag + "\r\n" + Dev_manag1);

    QString Dev_use = AglDeviceP->GetConfig("Dev_button2");
    QString Dev_use1 = AglDeviceP->GetConfig("Dev_button2_1");
    Label_button2->setAlignment(Qt::AlignCenter);
    Label_button2->setAutoFillBackground(true);
    Label_button2->setPalette(palette_button);
    Label_button2->setText(Dev_use + "\r\n" + Dev_use1);

    QString Dev_inf = AglDeviceP->GetConfig("Dev_button3");
    QString Dev_inf1 = AglDeviceP->GetConfig("Dev_button3_1");
    Label_infor->setAlignment(Qt::AlignCenter);
    Label_infor->setAutoFillBackground(true);
    Label_infor->setPalette(palette_button);
    Label_infor->setText(Dev_inf + "\r\n" + Dev_inf1);

    if(isAdmin == false)                                 //  如果当前用户不是root  用户管理按钮对其隐藏
    {
        Button_2->setEnabled(false);
        Button_dev->setEnabled(false);
        Label_button2->setStyleSheet(QStringLiteral("color: rgb(175,175,175);"));
        Label_button1->setStyleSheet(QStringLiteral("color: rgb(175,175,175);"));
    }
//------------------------------------------------------------------------------------------------//    用户管理
    UsrWidget = new QWidget(MainWindowP);
    UsrWidget->setAutoFillBackground(true);
    UsrWidget->setGeometry(0, 75, 805, 346);
    UsrWidget->setPalette(palette);

    Mybutton *Button_Usr = new Mybutton(UsrWidget);
    Mybutton *Button_Pass = new Mybutton(UsrWidget);
    Mybutton *Button_Usr_ret = new Mybutton(UsrWidget);
    Mybutton *Usr_home = new Mybutton(UsrWidget);
    Button_Usr->setGeometry(186, 88, 128, 128);
    Button_Pass->setGeometry(485, 88, 128, 128);
    Button_Usr_ret->setGeometry(11, 290, 48, 48);
    Usr_home->setGeometry(13, 11, 48, 48);
    Button_SetPix(Usr_home, MainSetpix_Home);

    QPixmap ima_usr;
    QString usrload = AglDeviceP->GetPasswd("@Usr");
    ima_usr.load(usrload);
    pix_Usr = ima_usr.scaled(Button_Usr->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_Usr, pix_Usr);

    QPixmap ima_pass;
    QString passload = AglDeviceP->GetPasswd("@Pass");
    ima_pass.load(passload);
    pix_Pass = ima_pass.scaled(Button_Pass->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_Pass, pix_Pass);
    Button_SetPix(Button_Usr_ret, MainSetpix_3);

    QLabel *Label_Usr = new QLabel(UsrWidget);
    QLabel *Label_Pass = new QLabel(UsrWidget);

    Label_Usr->setGeometry(150, 216, 200, 70);
    Label_Pass->setGeometry(449, 216, 200, 70);

    QString Usr_text = AglDeviceP->GetConfig("Label_Usr_text");
    QString Usr_text1 = AglDeviceP->GetConfig("Label_Usr_text1");
    Label_Usr->setAlignment(Qt::AlignCenter);
    Label_Usr->setAutoFillBackground(true);
    Label_Usr->setPalette(palette_button);

    QString Pass_text = AglDeviceP->GetConfig("Label_Pass_text");
    QString Pass_text1 = AglDeviceP->GetConfig("Label_Pass_text1");
    Label_Pass->setAlignment(Qt::AlignCenter);
    Label_Pass->setAutoFillBackground(true);
    Label_Pass->setPalette(palette_button);

    Label_Pass->setText(Pass_text + "\r\n" + Pass_text1);
    Label_Usr->setText(Usr_text + "\r\n" + Usr_text1);

    UsrWidget->hide();
//------------------------------------------------------------------------------------------------//    密码(升级)输入键盘
    Group_Key = new MyGroup(UsrWidget);
    Group_Key->setGeometry(145, 32, 510, 291);
    Group_Key->setStyleSheet(QStringLiteral("background-color: rgb(123, 139, 202);"));

    for(int i = 0; i < 26; i++)
    {
        SelectKey[i] = 'a' + i;
        pushButton[i] = new Mybutton(Group_Key);
        pushButton[i]->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
        pushButton[i]->setText(QString(SelectKey[i]));
    }

    pushButton[0]->setGeometry(QRect(36, 173, 40, 30));
    pushButton[1]->setGeometry(QRect(260, 213, 40, 30));
    pushButton[2]->setGeometry(QRect(160, 213, 40, 30));
    pushButton[3]->setGeometry(QRect(136, 173, 40, 30));
    pushButton[4]->setGeometry(QRect(110, 133, 40, 30));
    pushButton[5]->setGeometry(QRect(186, 173, 40, 30));
    pushButton[6]->setGeometry(QRect(236,173, 40, 30));
    pushButton[7]->setGeometry(QRect(286, 173, 40, 30));
    pushButton[8]->setGeometry(QRect(360, 133, 40, 30));
    pushButton[9]->setGeometry(QRect(336, 173, 40, 30));
    pushButton[10]->setGeometry(QRect(386, 173, 40, 30));
    pushButton[11]->setGeometry(QRect(436, 173, 40, 30));
    pushButton[12]->setGeometry(QRect(360, 213, 40, 30));
    pushButton[13]->setGeometry(QRect(310, 213, 40, 30));
    pushButton[14]->setGeometry(QRect(410, 133, 40, 30));
    pushButton[15]->setGeometry(QRect(460, 133, 40, 30));
    pushButton[16]->setGeometry(QRect(10, 133, 40, 30));
    pushButton[17]->setGeometry(QRect(160, 133, 40, 30));
    pushButton[18]->setGeometry(QRect(86, 173, 40, 30));
    pushButton[19]->setGeometry(QRect(210, 133, 40, 30));
    pushButton[20]->setGeometry(QRect(310, 133, 40, 30));
    pushButton[21]->setGeometry(QRect(210, 213, 40, 30));
    pushButton[22]->setGeometry(QRect(60, 133, 40, 30));
    pushButton[23]->setGeometry(QRect(110, 213, 40, 30));
    pushButton[24]->setGeometry(QRect(260, 133, 40, 30));
    pushButton[25]->setGeometry(QRect(60, 213, 40, 30));

    pushButton_Up = new Mybutton(Group_Key);
    pushButton_Up->setGeometry(QRect(10, 213, 40, 30));
    QString Up1 = AglDeviceP->GetPasswd("@pix_up_0");
    QPixmap pix_up(Up1);
    pushButton_Up->setIcon(pix_up);
    pushButton_Up->setIconSize(QSize(40,25));
    pushButton_Up->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_0 = new Mybutton(Group_Key);
    pushButton_0->setGeometry(QRect(410, 213, 40, 30));
    pushButton_0->setText(tr("0"));
    pushButton_0->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_Space = new Mybutton(Group_Key);
    pushButton_Space->setGeometry(QRect(100, 253, 260, 30));
    pushButton_Space->setText(tr(" "));
    pushButton_Space->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_Del = new Mybutton(Group_Key);
    pushButton_Del->setGeometry(QRect(10, 253, 80, 30));
    pushButton_Del->setText(tr("Delete"));
    pushButton_Del->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_dot = new Mybutton(Group_Key);
    pushButton_dot->setGeometry(QRect(460, 213, 40, 30));
    pushButton_dot->setText(".");
    pushButton_dot->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_sure = new Mybutton(Group_Key);
    pushButton_sure->setGeometry(QRect(420, 253, 80, 30));
    pushButton_sure->setText(tr("OK"));
    pushButton_sure->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_star = new Mybutton(Group_Key);
    pushButton_star->setGeometry(QRect(370, 253, 40, 30));
    pushButton_star->setText(tr("*"));
    pushButton_star->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_7 = new Mybutton(Group_Key);
    pushButton_7->setGeometry(QRect(336, 93, 40, 30));
    pushButton_7->setText(tr("7"));
    pushButton_7->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_2 = new Mybutton(Group_Key);
    pushButton_2->setGeometry(QRect(86, 93, 40, 30));
    pushButton_2->setText(tr("2"));
    pushButton_2->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_9 = new Mybutton(Group_Key);
    pushButton_9->setGeometry(QRect(436, 93, 40, 30));
    pushButton_9->setText(tr("9"));
    pushButton_9->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_3 = new Mybutton(Group_Key);
    pushButton_3->setGeometry(QRect(136, 93, 40, 30));
    pushButton_3->setText(tr("3"));
    pushButton_3->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_4 = new Mybutton(Group_Key);
    pushButton_4->setGeometry(QRect(186, 93, 40, 30));
    pushButton_4->setText(tr("4"));
    pushButton_4->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_8 = new Mybutton(Group_Key);
    pushButton_8->setGeometry(QRect(386, 93, 40, 30));
    pushButton_8->setText(tr("8"));
    pushButton_8->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_5 = new Mybutton(Group_Key);
    pushButton_5->setGeometry(QRect(236, 93, 40, 30));
    pushButton_5->setText(tr("5"));
    pushButton_5->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_1 = new Mybutton(Group_Key);
    pushButton_1->setGeometry(QRect(36, 93, 40, 30));
    pushButton_1->setText(tr("1"));
    pushButton_1->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_6 = new Mybutton(Group_Key);
    pushButton_6->setGeometry(QRect(286, 93, 40, 30));
    pushButton_6->setText(tr("6"));
    pushButton_6->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    QString keyinfor = AglDeviceP->GetConfig("Label_Pass1");
    Ltext = new QLabel(Group_Key);
    Ltext->setGeometry(10, 10, 490, 35);
    Ltext->setText(keyinfor);

    QFont font_key;
    font_key.setPointSize(14);
    QFont f;
    f.setPointSize(10);
    Ltext->setFont(font_key);

    text.clear();
    Edit_key = new myLineEdit(Group_Key);
    Edit_key->setEchoMode(QLineEdit::Password);
    Edit_key->setGeometry(10, 50, 410, 35);
    Edit_key->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    connect(pushButton_sure, SIGNAL(clicked()), this, SLOT(showButton_sure()),Qt::UniqueConnection);
    connect(pushButton_Del, SIGNAL(clicked()), this, SLOT(showButton_clear()),Qt::UniqueConnection);
    connect(pushButton_Up, SIGNAL(clicked()), this, SLOT(pressKeyButton_up()),Qt::UniqueConnection);
    connect(pushButton_star, SIGNAL(clicked()), this, SLOT(pressKeyButton_star()),Qt::UniqueConnection);
    connect(pushButton_dot, SIGNAL(clicked()), this, SLOT(pressKeyButton_dot()),Qt::UniqueConnection);
    connect(pushButton_Space, SIGNAL(clicked()), this, SLOT(pressKeyButton_space()),Qt::UniqueConnection);
    connect(pushButton_0, SIGNAL(clicked()), this, SLOT(pressKeyButton_0()),Qt::UniqueConnection);
    connect(pushButton_1, SIGNAL(clicked()), this, SLOT(pressKeyButton_1()),Qt::UniqueConnection);
    connect(pushButton_2, SIGNAL(clicked()), this, SLOT(pressKeyButton_2()),Qt::UniqueConnection);
    connect(pushButton_3, SIGNAL(clicked()), this, SLOT(pressKeyButton_3()),Qt::UniqueConnection);
    connect(pushButton_4, SIGNAL(clicked()), this, SLOT(pressKeyButton_4()),Qt::UniqueConnection);
    connect(pushButton_5, SIGNAL(clicked()), this, SLOT(pressKeyButton_5()),Qt::UniqueConnection);
    connect(pushButton_6, SIGNAL(clicked()), this, SLOT(pressKeyButton_6()),Qt::UniqueConnection);
    connect(pushButton_7, SIGNAL(clicked()), this, SLOT(pressKeyButton_7()),Qt::UniqueConnection);
    connect(pushButton_8, SIGNAL(clicked()), this, SLOT(pressKeyButton_8()),Qt::UniqueConnection);
    connect(pushButton_9, SIGNAL(clicked()), this, SLOT(pressKeyButton_9()),Qt::UniqueConnection);
    connect(pushButton[0], SIGNAL(clicked()), this, SLOT(pressKeyButton_a()),Qt::UniqueConnection);
    connect(pushButton[1], SIGNAL(clicked()), this, SLOT(pressKeyButton_b()),Qt::UniqueConnection);
    connect(pushButton[2], SIGNAL(clicked()), this, SLOT(pressKeyButton_c()),Qt::UniqueConnection);
    connect(pushButton[3], SIGNAL(clicked()), this, SLOT(pressKeyButton_d()),Qt::UniqueConnection);
    connect(pushButton[4], SIGNAL(clicked()), this, SLOT(pressKeyButton_e()),Qt::UniqueConnection);
    connect(pushButton[5], SIGNAL(clicked()), this, SLOT(pressKeyButton_f()),Qt::UniqueConnection);
    connect(pushButton[6], SIGNAL(clicked()), this, SLOT(pressKeyButton_g()),Qt::UniqueConnection);
    connect(pushButton[7], SIGNAL(clicked()), this, SLOT(pressKeyButton_h()),Qt::UniqueConnection);
    connect(pushButton[8], SIGNAL(clicked()), this, SLOT(pressKeyButton_i()),Qt::UniqueConnection);
    connect(pushButton[9], SIGNAL(clicked()), this, SLOT(pressKeyButton_j()),Qt::UniqueConnection);
    connect(pushButton[10], SIGNAL(clicked()), this, SLOT(pressKeyButton_k()),Qt::UniqueConnection);
    connect(pushButton[11], SIGNAL(clicked()), this, SLOT(pressKeyButton_l()),Qt::UniqueConnection);
    connect(pushButton[12], SIGNAL(clicked()), this, SLOT(pressKeyButton_m()),Qt::UniqueConnection);
    connect(pushButton[13], SIGNAL(clicked()), this, SLOT(pressKeyButton_n()),Qt::UniqueConnection);
    connect(pushButton[14], SIGNAL(clicked()), this, SLOT(pressKeyButton_o()),Qt::UniqueConnection);
    connect(pushButton[15], SIGNAL(clicked()), this, SLOT(pressKeyButton_p()),Qt::UniqueConnection);
    connect(pushButton[16], SIGNAL(clicked()), this, SLOT(pressKeyButton_q()),Qt::UniqueConnection);
    connect(pushButton[17], SIGNAL(clicked()), this, SLOT(pressKeyButton_r()),Qt::UniqueConnection);
    connect(pushButton[18], SIGNAL(clicked()), this, SLOT(pressKeyButton_s()),Qt::UniqueConnection);
    connect(pushButton[19], SIGNAL(clicked()), this, SLOT(pressKeyButton_t()),Qt::UniqueConnection);
    connect(pushButton[20], SIGNAL(clicked()), this, SLOT(pressKeyButton_u()),Qt::UniqueConnection);
    connect(pushButton[21], SIGNAL(clicked()), this, SLOT(pressKeyButton_v()),Qt::UniqueConnection);
    connect(pushButton[22], SIGNAL(clicked()), this, SLOT(pressKeyButton_w()),Qt::UniqueConnection);
    connect(pushButton[23], SIGNAL(clicked()), this, SLOT(pressKeyButton_x()),Qt::UniqueConnection);
    connect(pushButton[24], SIGNAL(clicked()), this, SLOT(pressKeyButton_y()),Qt::UniqueConnection);
    connect(pushButton[25], SIGNAL(clicked()), this, SLOT(pressKeyButton_z()),Qt::UniqueConnection);

    Group_Key->hide();

//------------------------------------------------------------------------------------------------//    信息设置
    Group_SetInfor= new MyGroup(COSWidget);
    Group_SetInfor->setStyle(QStyleFactory::create("windows"));
    Group_SetInfor->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Group_SetInfor->setGeometry(-2, -2, 803, 351);

    qFont.setPointSize(16);
    QLabel *TitelLabel = new QLabel(Group_SetInfor);

    TitelLabel->setGeometry(250, 25, 300, 40);
    TitelLabel->setAlignment(Qt::AlignCenter);
    QString Intitle = AglDeviceP->GetConfig("Infor_titel");
    TitelLabel->setText(Intitle);
    TitelLabel->setFont(qFont);

    QLabel *Label_Num = new QLabel(Group_SetInfor);
    QLabel *Label_Sam = new QLabel(Group_SetInfor);
    QLabel *Label_Name = new QLabel(Group_SetInfor);
    QLabel *Label_VER = new QLabel(Group_SetInfor);
    QLabel *Label_Made = new QLabel(Group_SetInfor);
    QLabel *Label_Date = new QLabel(Group_SetInfor);
    QLabel *Label_Firm = new QLabel(Group_SetInfor);

    QLabel *Label_Num_show = new QLabel(Group_SetInfor);
    QLabel *Label_Sam_show = new QLabel(Group_SetInfor);
    QLabel *Label_Name_show = new QLabel(Group_SetInfor);
    QLabel *Label_VER_show = new QLabel(Group_SetInfor);
    QLabel *Label_Made_show = new QLabel(Group_SetInfor);
    QLabel *Label_Date_show = new QLabel(Group_SetInfor);
    QLabel *Label_Firm_show = new QLabel(Group_SetInfor);

    if(AglDeviceP->GetPasswd("@Languag") == "English")
    {
        Label_Name->setGeometry(165, 80, 350, 30);
        Label_Made->setGeometry(165, 117, 285, 30);
        Label_Firm->setGeometry(165, 154, 250, 30);
        Label_VER->setGeometry(165, 191, 250, 30);
        Label_Sam->setGeometry(165, 228, 250, 30);
        Label_Num->setGeometry(165, 265, 250, 30);
        Label_Date->setGeometry(165, 302, 250, 30);
    }
    else if(AglDeviceP->GetPasswd("@Languag") == "Chinese")
    {
        Label_Name->setGeometry(235, 80, 350, 30);
        Label_Made->setGeometry(235, 117, 285, 30);
        Label_Firm->setGeometry(235, 154, 250, 30);
        Label_VER->setGeometry(235, 191, 250, 30);
        Label_Sam->setGeometry(235, 228, 250, 30);
        Label_Num->setGeometry(235, 265, 250, 30);
        Label_Date->setGeometry(235, 302, 250, 30);
    }

    Label_Name_show->setGeometry(435, 80, 200, 30);
    Label_Made_show->setGeometry(435, 117, 200, 30);
    Label_Firm_show->setGeometry(435, 154,200, 30);
    Label_VER_show->setGeometry(435, 191, 200, 30);
    Label_Sam_show->setGeometry(435, 228, 200, 30);
    Label_Num_show->setGeometry(435, 265, 200, 30);
    Label_Date_show->setGeometry(435, 302, 200, 30);

    int Batall = Sql->CountBatchSql();
    if(-1 == Batall)
    {
        qDebug() << "1095 Count Batch Error";
        return;
    }
    QString all_bat = QString :: number(Batall, 10);
    int TestAll = Sql->CountResultSql();
    if(-1 == TestAll)
    {
        qDebug() << "1105 Count Resule DB Error";
        return;
    }
    QString all_test = QString :: number(TestAll, 10);

    QString Tobat = AglDeviceP->GetConfig("Infor_Num");
    QString Tousa = AglDeviceP->GetConfig("Infor_Sam");
    Label_Num->setPalette(palette_button);
    Label_Num->setText(Tobat);
    Label_Sam->setPalette(palette_button);
    Label_Sam->setText(Tousa);

    QString Info_na = AglDeviceP->GetConfig("Infor_Name");
    Label_Name->setPalette(palette_button);
    Label_Name->setText(Info_na);
    Label_VER->setPalette(palette_button);

    Label_VER->setText(AglDeviceP->GetConfig("Infor_Soft"));
    Label_Firm->setText(AglDeviceP->GetConfig("Infor_Fimr"));
    Label_Firm->setPalette(palette_button);
    QString Info_made= AglDeviceP->GetConfig("Infor_Made");
    QString Info_date= AglDeviceP->GetConfig("Infor_Date");
    Label_Made->setPalette(palette_button);
    Label_Made->setText(Info_made);

    Label_Date->setPalette(palette_button);
    Label_Date->setText(Info_date);
//===================================Add==================================//
    QString Fimr = (QString)AglDeviceP->AglDev.Version;
    QStringList Fimr_list = Fimr.split("-");
    QString FimrData = Fimr_list.at(1);
    FimrData = FimrData.left(FimrData.length() - 1);
    Label_Name_show->setText(AglDeviceP->GetConfig("Infor_Name_show"));
    Label_Made_show->setText(AglDeviceP->GetConfig("Infor_Made_show"));
    Label_Firm_show->setText(FimrData);
    Label_VER_show->setText("V1.0.2");
    Label_Sam_show->setText(all_test);
    Label_Num_show->setText(all_bat);
    Label_Date_show->setText(AglDeviceP->GetConfig("Infor_Date_show"));
//========================================================================//
    Mybutton *But_Inf_ret = new Mybutton(Group_SetInfor);
    Mybutton *But_Inf_home = new Mybutton(Group_SetInfor);
    But_Inf_ret->setGeometry(13, 292, 48, 48);
    But_Inf_home->setGeometry(15, 13, 48, 48);
    Button_SetPix(But_Inf_ret, MainSetpix_3);
    Button_SetPix(But_Inf_home, MainSetpix_Home);
    Group_SetInfor->hide();
//------Add-----------------------------------------------------------------------------------------//
    QPixmap res_pix;
    res_pix.load(AglDeviceP->GetPasswd("@Restore_pix"));
    Mybutton *But_Inf_restore = new Mybutton(Group_SetInfor);
    But_Inf_restore->setGeometry(743, 292, 48, 48);
    QPixmap Restory = res_pix.scaled(But_Inf_restore->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(But_Inf_restore, Restory);
    connect(But_Inf_restore, SIGNAL(clicked()), this, SLOT(pressBut_Inf_restore()),Qt::UniqueConnection);
    if(isAdmin == false)                                 //  如果当前用户不是root  用户管理按钮对其隐藏
    {
        But_Inf_restore->hide();
    }
//------------------------------------------------------------------------------------------------//    设备管理
    Group_COS = new MyGroup(COSWidget);
    Group_COS->setStyle(QStyleFactory::create("windows"));
    Group_COS->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Group_COS->setGeometry(-2, -2, 805, 353);

    qFont.setPointSize(16);
    QString Devic = AglDeviceP->GetConfig("COSTitel");
    QLabel *COSTitelLabel = new QLabel(Group_COS);
    COSTitelLabel->setGeometry(250, 10, 300, 60);
    COSTitelLabel->setAlignment(Qt::AlignCenter);
    COSTitelLabel->setText(Devic);
    COSTitelLabel->setFont(qFont);

    qFont.setPointSize(14);
    QLabel *Label_IP = new QLabel(Group_COS);
    QLabel *Label_LisIP = new QLabel(Group_COS);
    QLabel *Label_LisPort = new QLabel(Group_COS);
    QLabel *Label_name = new QLabel(Group_COS);
    QLabel *Label_mark = new QLabel(Group_COS);
    QLabel *Label_data = new QLabel(Group_COS);
    QLabel *Label_time = new QLabel(Group_COS);
    QLabel *Label_Lan = new QLabel(Group_COS);

    Edit_IP = new myLineEdit(Group_COS);
    Edit_LisIP = new myLineEdit(Group_COS);
    Edit_LisPort = new myLineEdit(Group_COS);
    myLineEdit *Edit_name = new myLineEdit(Group_COS);
    Edit_name->setEnabled(false);
    Edit_data = new DateEdit(Group_COS);
    Edit_time = new TimeEdit(Group_COS);
    box_Lan = new ComBox(Group_COS);
    myLineEdit *Edit_mark = new myLineEdit(Group_COS);
    Edit_mark->setEnabled(false);

    Edit_IP->setStyle(QStyleFactory::create("fusion"));
    Edit_LisIP->setStyle(QStyleFactory::create("fusion"));
    Edit_LisPort->setStyle(QStyleFactory::create("fusion"));
    Edit_name->setStyle(QStyleFactory::create("fusion"));
    Edit_data->setStyle(QStyleFactory::create("fusion"));
    Edit_time->setStyle(QStyleFactory::create("fusion"));
    box_Lan->setStyle(QStyleFactory::create("fusion"));
    Edit_mark->setStyle(QStyleFactory::create("fusion"));

    Label_IP->setGeometry(58, 83, 100, 30);
    Label_LisIP->setGeometry(58, 133, 100, 30);
    Label_LisPort->setGeometry(58, 183, 100, 30);
    Label_name->setGeometry(58, 233, 160, 30);
    Edit_data->setGeometry(535, 83, 200, 30);
    Edit_time->setGeometry(535, 133, 200, 30);
    box_Lan->setGeometry(535, 183, 200, 30);
    Edit_mark->setGeometry(535, 233, 200, 30);

    QString cosip = AglDeviceP->GetConfig("COS_IP");
    Label_IP->setText(cosip);
    Label_IP->setPalette(palette_button);
    QString lisip = AglDeviceP->GetConfig("COS_LisIP");
    Label_LisIP->setText(lisip);
    Label_LisIP->setPalette(palette_button);
    QString lisport = AglDeviceP->GetConfig("COS_LisPort");
    Label_LisPort->setText(lisport);
    Label_LisPort->setPalette(palette_button);
    QString lisna = AglDeviceP->GetConfig("COS_name");
    Label_name->setText(lisna);
    Label_name->setPalette(palette_button);
    Label_data->setGeometry(420, 83, 100, 30);
    QString lisda = AglDeviceP->GetConfig("COS_data");
    Label_data->setText(lisda);
    Label_data->setPalette(palette_button);
    Label_time->setGeometry(420, 133, 100, 30);
    QString listi = AglDeviceP->GetConfig("COS_time");
    Label_time->setText(listi);
    Label_time->setPalette(palette_button);
    Label_Lan->setGeometry(420, 183, 140, 30);
    QString lisla = AglDeviceP->GetConfig("COS_Lan");
    Label_Lan->setText(lisla);
    Label_Lan->setPalette(palette_button);
    Label_mark->setGeometry(420, 233, 140, 30);
    QString lisde = AglDeviceP->GetConfig("COS_mark");
    Label_mark->setText(lisde);
    Label_mark->setPalette(palette_button);

    Edit_IP->setGeometry(173, 83, 200, 30);
    Edit_LisIP->setGeometry(173, 133, 200, 30);
    Edit_LisPort->setGeometry(173, 183, 200, 30);
    Edit_name->setGeometry(173, 233, 200, 30);

    QString coseditna = AglDeviceP->GetConfig("Device_name");
    Edit_name->setText(coseditna);
    QString cosedima = AglDeviceP->GetConfig("COSEdit_mark");
    Edit_mark->setText(cosedima);
//---------------------------------------------------------------------------------------------------------------//
    Box_Send = new CheckBox(Group_COS);
    Box_Send->setGeometry(420, 283, 230, 30);
    Box_Send->setStyle(QStyleFactory::create("fusion"));
    Box_Send->setStyleSheet(QStringLiteral("color: rgb(0,0,0);"));
    Box_Send->setText(AglDeviceP->GetConfig("AUTO_Text"));

    Box_QRcode = new CheckBox(Group_COS);
    Box_QRcode->setGeometry(173, 283, 200, 30);
    Box_QRcode->setStyle(QStyleFactory::create("fusion"));
    Box_QRcode->setStyleSheet(QStringLiteral("color: rgb(0,0,0);"));
    Box_QRcode->setText(AglDeviceP->GetConfig("QRCode_USE"));
    if(isAdmin == false)
    {
        Box_QRcode->hide();
    }
//---------------------------------------------------------------------------------------------------------------//
    Edit_IP->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_LisIP->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_LisPort->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_name->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_data->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_time->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Edit_mark->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    box_Lan->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));

    Mybutton *Button_NumRet = new Mybutton(Group_COS);
    Mybutton *Button_Save = new Mybutton(Group_COS);
    Mybutton *But_dev_home = new Mybutton(Group_COS);
    Mybutton *But_dev_cali = new Mybutton(Group_COS);

    But_dev_cali->setStyle(QStyleFactory::create("fusion"));
    Button_NumRet->setStyle(QStyleFactory::create("fusion"));
    Button_Save->setStyle(QStyleFactory::create("fusion"));
    But_dev_home->setStyle(QStyleFactory::create("fusion"));

    But_dev_cali->setGeometry(743, 13, 48, 48);
    But_dev_home->setGeometry(15, 13, 48, 48);
    Button_NumRet->setGeometry(13, 292, 48, 48);
    QString Cali_path = AglDeviceP->GetPasswd("@Cali_pix");
    QPixmap Cali_pix;
    Cali_pix.load(Cali_path);
    QPixmap PIX_jiao = Cali_pix.scaled(But_dev_cali->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(But_dev_home, MainSetpix_Home);
    Button_SetPix(Button_NumRet, MainSetpix_3);
    Button_SetPix(But_dev_cali, PIX_jiao);

    QPixmap ima_cali;
    QString caliload = AglDeviceP->GetPasswd("@Calib_pix");
    ima_cali.load(caliload);

    Button_Save->setGeometry(743, 292, 48, 48);
    QPixmap ima_save;
    QString saveload = AglDeviceP->GetPasswd("@MainSet_Save");
    ima_save.load(saveload);
    MainSetpix_Save = ima_save.scaled(Button_Save->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_Save, MainSetpix_Save);

//------------------------------------------------------Add---------------------------------------------//
    Mybutton *Button_Contrl = new Mybutton(Group_COS);
    Button_Contrl->setGeometry(743, 72, 48, 48);
    QPixmap ima_contrl;
    ima_contrl.load(AglDeviceP->GetPasswd("@TestContrl_pix"));
    QPixmap ContrlPix = ima_contrl.scaled(Button_Contrl->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_Contrl, ContrlPix);
    connect(Button_Contrl, SIGNAL(clicked()), this, SLOT(pressButtonContrl()),Qt::UniqueConnection);
    if(isAdmin == false)
    {
        Button_Contrl->hide();
    }
//------------------------------------------------------------------------------------------------------------------------//

    connect(Box_Send, SIGNAL(clicked()), this, SLOT(Radio_test()),Qt::UniqueConnection);
    connect(Box_QRcode, SIGNAL(clicked()), this, SLOT(Radio_test()),Qt::UniqueConnection);
    connect(Edit_time, SIGNAL(timeChanged(QTime)), this, SLOT(pressEdit_time()),Qt::UniqueConnection);
    connect(Edit_data, SIGNAL(dateChanged(QDate)), this, SLOT(pressEdit_data()),Qt::UniqueConnection);
    connect(But_dev_cali,SIGNAL(clicked()),this,SLOT(pressBut_dev_cali()),Qt::UniqueConnection);
    connect(Button_Save,SIGNAL(clicked()),this,SLOT(pressButton_Save()),Qt::UniqueConnection);
    connect(But_Inf_ret, SIGNAL(clicked()), this, SLOT(pressBut_Inf_ret()),Qt::UniqueConnection);
    connect(But_Inf_home, SIGNAL(clicked()), this, SLOT(pressBut_Inf_home()),Qt::UniqueConnection);
    connect(Button_Infor, SIGNAL(clicked()), this, SLOT(pressButton_Infor()),Qt::UniqueConnection);
    connect(Button_Usr_ret, SIGNAL(clicked()), this, SLOT(pressButton_Usr_ret()),Qt::UniqueConnection);
    connect(Button_Usr, SIGNAL(clicked()), this, SLOT(pressButton_Usr()),Qt::UniqueConnection);
    connect(Button_Pass, SIGNAL(clicked()), this, SLOT(pressButton_Pass()),Qt::UniqueConnection);
    connect(Button_NumRet, SIGNAL(clicked()), this, SLOT(pressButton_NumRet()),Qt::UniqueConnection);
    connect(Button_dev, SIGNAL(clicked()), this, SLOT(pressButton_dev()),Qt::UniqueConnection);
    connect(Button_2, SIGNAL(clicked()), this, SLOT(pressButton_2()),Qt::UniqueConnection);
    connect(Button_3,SIGNAL(clicked()),this,SLOT(pressButton_3()),Qt::UniqueConnection);
    connect(box_Lan, SIGNAL(activated()), this, SLOT(pressbox_Lan()),Qt::UniqueConnection);
    connect(Button_Home, SIGNAL(clicked()), this, SLOT(Press_Set_home()),Qt::UniqueConnection);
    connect(Usr_home, SIGNAL(clicked()), this, SLOT(pressUSR_Home()),Qt::UniqueConnection);
    connect(But_dev_home, SIGNAL(clicked()), this, SLOT(pressDev_Home()),Qt::UniqueConnection);

    connect(Edit_IP, SIGNAL(clicked()), this, SLOT(pressEdit_hostIP()),Qt::UniqueConnection);
    connect(Edit_LisIP, SIGNAL(clicked()), this, SLOT(pressEdit_LisIP()),Qt::UniqueConnection);
    connect(Edit_LisPort, SIGNAL(clicked()), this, SLOT(pressEdit_LisPort()),Qt::UniqueConnection);

    KeyInput.clear();
    Group_COS->hide();
    COSWidget->show();
}

void PctGuiThread::Radio_test()
{
    Voice_pressKey();
}

void PctGuiThread::pressButtonContrl()              //  试剂卡校准界面
{
    Voice_pressKey();
    CalibrationType = UiType;
    QString AUTO_sta;
    if(Box_Send->isChecked())
    {
        AUTO_sta = "Checked";
    }
    else
    {
        AUTO_sta = "DisChecked";
    }

    QString QRCode_sta;
    if(Box_QRcode->isChecked())
    {
        QRCode_sta = "USE";
    }
    else
    {
        QRCode_sta = "NOUSE";
    }
    QString Auto;
    QString QRco;
    if(AglDeviceP->GetPasswd("@AUTO") == "NOAUTO")
    {
        Auto = "DisChecked";
    }
    else
    {
        Auto = "Checked";
    }
    if(AglDeviceP->GetPasswd("@QRcode") == "YES")
    {
        QRco = "USE";
    }
    else
    {
        QRco = "NOUSE";
    }

    OldUserData = Edit_IP->text()+Edit_LisIP->text()+Edit_LisPort->text()+Edit_data->text()+Edit_time->text()+DefaultLanguage+Auto+QRco;
    QString CurrentData = Edit_IP->text()+Edit_LisIP->text()+Edit_LisPort->text()+Edit_data->text()+Edit_time->text()+box_Lan->currentText()+AUTO_sta+QRCode_sta;
    if(OldUserData != CurrentData/* && isSaveDev != true*/)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_SaDev"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Question);
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                isMessage_child = true;
                Voice_pressKey();
                SaveSet();
                msgBox_child = new QMessageBox(MainWindowP);
                msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox_child->setText(AglDeviceP->GetConfig("Message_succsave"));
                msgBox_child->setIcon(QMessageBox :: Information);
                msgBox_child->exec();
                qDebug() << "like clicked Yes button";
                Voice_pressKey();
                isMessage_child = false;
                delete msgBox_child;
                break;
            }
            case QMessageBox::No:
            {
                qDebug() << "like clicked No button";
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
    if(isWidget == true)
    {
        return;
    }
    state_door = (int)AglDeviceP->AglDev.OutIntState;
    Widget_contrl = new QWidget(Group_COS);
    Widget_contrl->setGeometry(0, 0, 800, 480);
    Label_station->setText("Before calibration");

    QFont font;

    Mybutton *But_con_ret = new Mybutton(Widget_contrl);
    Mybutton *But_con_home = new Mybutton(Widget_contrl);
    Mybutton *Bun_con_Door = new Mybutton(Widget_contrl);

    But_con_ret->setGeometry(13, 292, 48, 48);
    But_con_home->setGeometry(15, 13, 48, 48);
    Button_SetPix(But_con_ret, MainSetpix_3);
    Button_SetPix(But_con_home, MainSetpix_Home);

    Bun_con_Door->setGeometry(737, 292, 48, 48);
    Button_SetPix(Bun_con_Door, Mainpix_door);
    font.setPointSize(16);

    QLabel *Label_con_title = new QLabel(Widget_contrl);
    Label_con_title->setGeometry(200, 26, 400, 30);
    Label_con_title->setText(AglDeviceP->GetConfig("testContrl"));
    Label_con_title->setAlignment(Qt::AlignCenter);
    Label_con_title->setFont(font);

    QString Show_Data;

    Show_Data += AglDeviceP->GetConfig("Move_C") + ":";
    Show_Data += AglDeviceP->GetPasswd("@C_Distance") + "\r\n";

    Show_Data += AglDeviceP->GetConfig("TC_distance") + ":";
    Show_Data += AglDeviceP->GetPasswd("@DX") + "\r\n\r\n";

    Show_Data += "C1 = " + AglDeviceP->GetPasswd("@Standard_C1") + "\r\n";
    Show_Data += "T1 = " + AglDeviceP->GetPasswd("@Standard_T1") + "\r\n";
    Show_Data += "C2 = " + AglDeviceP->GetPasswd("@Standard_C2") + "\r\n";
    Show_Data += "T2 = " + AglDeviceP->GetPasswd("@Standard_T2") + "\r\n";
    Show_Data += "C3 = " + AglDeviceP->GetPasswd("@Standard_C3") + "\r\n";
    Show_Data += "T3 = " + AglDeviceP->GetPasswd("@Standard_T3") + "\r\n";
    Show_Data += "C4 = " + AglDeviceP->GetPasswd("@Standard_C4") + "\r\n";
    Show_Data += "T4 = " + AglDeviceP->GetPasswd("@Standard_T4") + "\r\n";

    Show_Data += "K1 = " + AglDeviceP->GetPasswd("@K1") + "\r\n";
    Show_Data += "B1 = " + AglDeviceP->GetPasswd("@B1") + "\r\n";
    Show_Data += "K2 = " + AglDeviceP->GetPasswd("@K2") + "\r\n";
    Show_Data += "B2 = " + AglDeviceP->GetPasswd("@B2") + "\r\n";
    Show_Data += "K3 = " + AglDeviceP->GetPasswd("@K3") + "\r\n";
    Show_Data += "B3 = " + AglDeviceP->GetPasswd("@B3") + "\r\n";
    Show_Data += "K4 = " + AglDeviceP->GetPasswd("@K4") + "\r\n";
    Show_Data += "B4 = " + AglDeviceP->GetPasswd("@B4") + "\r\n";
    Show_Data += "K5 = " + AglDeviceP->GetPasswd("@K5") + "\r\n";
    Show_Data += "B5 = " + AglDeviceP->GetPasswd("@B5") + "\r\n";
    Show_Data += "K6 = " + AglDeviceP->GetPasswd("@K6") + "\r\n";
    Show_Data += "B6 = " + AglDeviceP->GetPasswd("@B6") + "\r\n";
    Show_Data += "K7 = " + AglDeviceP->GetPasswd("@K7") + "\r\n";
    Show_Data += "B7 = " + AglDeviceP->GetPasswd("@B7") + "\r\n";
    Show_Data += "K8 = " + AglDeviceP->GetPasswd("@K8") + "\r\n";
    Show_Data += "B8 = " + AglDeviceP->GetPasswd("@B8") + "\r\n";

    CaliText = new TextEdit(Widget_contrl);
    CaliText->setGeometry(400, 60, 320, 266);
    CaliText->setStyle(QStyleFactory::create("fusion"));
    CaliText->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    CaliText->setReadOnly(true);
//------------------------------------------------Add---------------------------------------------------//
    QLabel *T1_show = new QLabel(Widget_contrl);
    QLabel *T2_show = new QLabel(Widget_contrl);
    QLabel *T3_show = new QLabel(Widget_contrl);
    QLabel *T4_show = new QLabel(Widget_contrl);
    T1_show->setGeometry(218, 80, 30, 30);
    T2_show->setGeometry(218, 138.7, 30, 30);
    T3_show->setGeometry(218, 197.3, 30, 30);
    T4_show->setGeometry(218, 256, 30, 30);
    T1_show->setText("T1:");
    T2_show->setText("T2:");
    T3_show->setText("T3:");
    T4_show->setText("T4:");
    Edit_T1 = new myLineEdit(Widget_contrl);
    Edit_T2 = new myLineEdit(Widget_contrl);
    Edit_T3 = new myLineEdit(Widget_contrl);
    Edit_T4 = new myLineEdit(Widget_contrl);
    Edit_T1->setGeometry(253, 80, 95, 30);
    Edit_T2->setGeometry(253, 138.7, 95, 30);
    Edit_T3->setGeometry(253, 197.3, 95, 30);
    Edit_T4->setGeometry(253, 256, 95, 30);
    Edit_T1->setStyle(QStyleFactory::create("fusion"));
    Edit_T2->setStyle(QStyleFactory::create("fusion"));
    Edit_T3->setStyle(QStyleFactory::create("fusion"));
    Edit_T4->setStyle(QStyleFactory::create("fusion"));

    QLabel *C1_show = new QLabel(Widget_contrl);
    QLabel *C2_show = new QLabel(Widget_contrl);
    QLabel *C3_show = new QLabel(Widget_contrl);
    QLabel *C4_show = new QLabel(Widget_contrl);
    C1_show->setGeometry(78, 80, 30, 30);
    C2_show->setGeometry(78, 138.7, 30, 30);
    C3_show->setGeometry(78, 197.3, 30, 30);
    C4_show->setGeometry(78, 256, 30, 30);
    C1_show->setText("C1:");
    C2_show->setText("C2:");
    C3_show->setText("C3:");
    C4_show->setText("C4:");
    Edit_C1 = new myLineEdit(Widget_contrl);
    Edit_C2 = new myLineEdit(Widget_contrl);
    Edit_C3 = new myLineEdit(Widget_contrl);
    Edit_C4 = new myLineEdit(Widget_contrl);
    Edit_C1->setGeometry(113, 80, 95, 30);
    Edit_C2->setGeometry(113, 138.7, 95, 30);
    Edit_C3->setGeometry(113, 197.3, 95, 30);
    Edit_C4->setGeometry(113, 256, 95, 30);
    Edit_C1->setStyle(QStyleFactory::create("fusion"));
    Edit_C2->setStyle(QStyleFactory::create("fusion"));
    Edit_C3->setStyle(QStyleFactory::create("fusion"));
    Edit_C4->setStyle(QStyleFactory::create("fusion"));

    Mybutton *Button_Set_TC = new Mybutton(Widget_contrl);
    Button_Set_TC->setGeometry(335, 292, 48, 48);
    Button_SetPix(Button_Set_TC, MainSetpix_Save);

    Mybutton *Button_Restore = new Mybutton(Widget_contrl);
    Button_Restore->setGeometry(737, 13, 48, 48);
    QPixmap Pix_Restorre;
    Pix_Restorre.load(AglDeviceP->GetPasswd("@Restore_Cal_pix"));
    QPixmap Pix_Res_Show = Pix_Restorre.scaled(Button_Restore->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_Restore, Pix_Res_Show);
//--------------------------------------------------------------------------------------------------------//
    QScrollBar *num_bar =  CaliText->verticalScrollBar();                                                                                        // 获得该表格的滚动条
    QScrollBar *Table_ba_childr = new MyScrollBar(CaliText);
    Table_ba_childr->move(615, 5);
    Table_ba_childr = num_bar;
    Table_ba_childr->setStyleSheet("QScrollBar:vertical {""width: 30px; background-color: rgb(215,215,215);""}");     // 改变滚动条宽度，颜色

    Mybutton *Button_start = new Mybutton(Widget_contrl);
    Button_start->setGeometry(737, 229, 48, 48);
    QPixmap ima_start;
    QString startload = AglDeviceP->GetPasswd("@Start_pix");
    ima_start.load(startload);
    Startpix = ima_start.scaled(Button_start->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_start, Startpix);

//    Mybutton *Button_Calib_C= new Mybutton(Widget_contrl);
//    Button_Calib_C->setGeometry(737, 166, 48, 48);
//    QPixmap ima_Calib;
//    QString Calibload = AglDeviceP->GetPasswd("@Location_pix");
//    ima_Calib.load(Calibload);
//    Button_SetPix(Button_Calib_C, ima_Calib);

    CaliText->setText(Show_Data);
    connect(Edit_T1, SIGNAL(clicked()), this, SLOT(pressEdit_T1()),Qt::UniqueConnection);
    connect(Edit_T2, SIGNAL(clicked()), this, SLOT(pressEdit_T2()),Qt::UniqueConnection);
    connect(Edit_T3, SIGNAL(clicked()), this, SLOT(pressEdit_T3()),Qt::UniqueConnection);
    connect(Edit_T4, SIGNAL(clicked()), this, SLOT(pressEdit_T4()),Qt::UniqueConnection);
    connect(Edit_C1, SIGNAL(clicked()), this, SLOT(pressEdit_C1()),Qt::UniqueConnection);
    connect(Edit_C2, SIGNAL(clicked()), this, SLOT(pressEdit_C2()),Qt::UniqueConnection);
    connect(Edit_C3, SIGNAL(clicked()), this, SLOT(pressEdit_C3()),Qt::UniqueConnection);
    connect(Edit_C4, SIGNAL(clicked()), this, SLOT(pressEdit_C4()),Qt::UniqueConnection);
    connect(Button_Restore, SIGNAL(clicked()), this, SLOT(press_Resote_TC()),Qt::UniqueConnection);
    connect(Button_Set_TC, SIGNAL(clicked()), this, SLOT(press_SaveTC_Button()),Qt::UniqueConnection);
//    connect(Button_Calib_C, SIGNAL(clicked()), this, SLOT(pressButton_Calib_C()));
    connect(But_con_ret, SIGNAL(clicked()), this, SLOT(pressBut_con_ret()),Qt::UniqueConnection);
    connect(But_con_home, SIGNAL(clicked()), this, SLOT(pressBut_con_home()),Qt::UniqueConnection);
    connect(Bun_con_Door, SIGNAL(clicked()), this, SLOT(CalibrationDoor()),Qt::UniqueConnection);
    connect(Button_start, SIGNAL(clicked()), this, SLOT(pressButton_start()),Qt::UniqueConnection);
    Widget_contrl->show();
    isCalib = true;
}

void PctGuiThread::press_Resote_TC()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_TC_contrl");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    isClicked = true;
    isMessage = true;
    msgBox = new QMessageBox(MainWindowP);
    msgBox->setText(AglDeviceP->GetConfig("Message_Restore"));
    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
    msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::No);
    msgBox->setIcon(QMessageBox :: Warning);
    int ret = msgBox->exec();
    switch (ret)
    {
        case QMessageBox::Yes:
        {
            Voice_pressKey();
            QString Str_K, Str_B;
            for(int i = 1; i <= 8; i++)
            {
                Str_K = "K" + QString::number(i);
                Str_B = "B" + QString::number(i);
                if(!AglDeviceP->ReWriteFile(Str_K, "1"))
                {
                    qDebug() << "press_Resote_TC Rewite hostIP Error";
                    isClicked = false;
                    return;
                }
                if(!AglDeviceP->ReWriteFile(Str_B, "0"))
                {
                    qDebug() << "press_Resote_TC Rewite hostIP Error";
                    isClicked = false;
                    return;
                }
            }
            if(!AglDeviceP->ReWriteFile("C_Distance", "0"))
            {
                qDebug() << "press_Resote_TC Rewite hostIP Error";
                isClicked = false;
                return;
            }
            if(!AglDeviceP->ReWriteFile("DX", "350"))
            {
                qDebug() << "press_Resote_TC Rewite hostIP Error";
                isClicked = false;
                return;
            }
            if(!(AglDeviceP->WriteTxtToFile("password.txt", AglDeviceP->GetAllPass())))
            {
                qDebug() << "press_Resote_TC Creat file error!";
                isClicked = false;
                return;
            }
            break;
        }
        case QMessageBox::No:
        {
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            break;
        }
    }
    CaliText->clear();
    QString Show_Data;

    Show_Data += AglDeviceP->GetConfig("Move_C") + ":";
    Show_Data += AglDeviceP->GetPasswd("@C_Distance") + "\r\n";

    Show_Data += AglDeviceP->GetConfig("TC_distance") + ":";
    Show_Data += AglDeviceP->GetPasswd("@DX") + "\r\n\r\n";

    Show_Data += "C1 = " + AglDeviceP->GetPasswd("@Standard_C1") + "\r\n";
    Show_Data += "T1 = " + AglDeviceP->GetPasswd("@Standard_T1") + "\r\n";
    Show_Data += "C2 = " + AglDeviceP->GetPasswd("@Standard_C2") + "\r\n";
    Show_Data += "T2 = " + AglDeviceP->GetPasswd("@Standard_T2") + "\r\n";
    Show_Data += "C3 = " + AglDeviceP->GetPasswd("@Standard_C3") + "\r\n";
    Show_Data += "T3 = " + AglDeviceP->GetPasswd("@Standard_T3") + "\r\n";
    Show_Data += "C4 = " + AglDeviceP->GetPasswd("@Standard_C4") + "\r\n";
    Show_Data += "T4 = " + AglDeviceP->GetPasswd("@Standard_T4") + "\r\n";

    Show_Data += "K1 = " + AglDeviceP->GetPasswd("@K1") + "\r\n";
    Show_Data += "B1 = " + AglDeviceP->GetPasswd("@B1") + "\r\n";
    Show_Data += "K2 = " + AglDeviceP->GetPasswd("@K2") + "\r\n";
    Show_Data += "B2 = " + AglDeviceP->GetPasswd("@B2") + "\r\n";
    Show_Data += "K3 = " + AglDeviceP->GetPasswd("@K3") + "\r\n";
    Show_Data += "B3 = " + AglDeviceP->GetPasswd("@B3") + "\r\n";
    Show_Data += "K4 = " + AglDeviceP->GetPasswd("@K4") + "\r\n";
    Show_Data += "B4 = " + AglDeviceP->GetPasswd("@B4") + "\r\n";
    Show_Data += "K5 = " + AglDeviceP->GetPasswd("@K5") + "\r\n";
    Show_Data += "B5 = " + AglDeviceP->GetPasswd("@B5") + "\r\n";
    Show_Data += "K6 = " + AglDeviceP->GetPasswd("@K6") + "\r\n";
    Show_Data += "B6 = " + AglDeviceP->GetPasswd("@B6") + "\r\n";
    Show_Data += "K7 = " + AglDeviceP->GetPasswd("@K7") + "\r\n";
    Show_Data += "B7 = " + AglDeviceP->GetPasswd("@B7") + "\r\n";
    Show_Data += "K8 = " + AglDeviceP->GetPasswd("@K8") + "\r\n";
    Show_Data += "B8 = " + AglDeviceP->GetPasswd("@B8") + "\r\n";

    CaliText->setText(Show_Data);
    isClicked = false;
}

void PctGuiThread::press_SaveTC_Button()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_TC_contrl");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    isClicked = true;
    bool isInputEmp = false;
    QString Str_T1 = Edit_T1->text();
    QString Str_T2 = Edit_T2->text();
    QString Str_T3 = Edit_T3->text();
    QString Str_T4 = Edit_T4->text();

    QString Str_C1 = Edit_C1->text();
    QString Str_C2 = Edit_C2->text();
    QString Str_C3 = Edit_C3->text();
    QString Str_C4 = Edit_C4->text();

    if(Str_T1.isEmpty())
    {
        isInputEmp = true;
    }
    if(Str_T2.isEmpty())
    {
        isInputEmp = true;
    }
    if(Str_T3.isEmpty())
    {
        isInputEmp = true;
    }
    if(Str_T4.isEmpty())
    {
        isInputEmp = true;
    }
    if(Str_C1.isEmpty())
    {
        isInputEmp = true;
    }
    if(Str_C2.isEmpty())
    {
        isInputEmp = true;
    }
    if(Str_C3.isEmpty())
    {
        isInputEmp = true;
    }
    if(Str_C4.isEmpty())
    {
        isInputEmp = true;
    }
    if(isInputEmp == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_TCEmpty");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        isClicked = false;
        return;
    }
    bool ValOK_T1 = false;
    bool ValOK_T2 = false;
    bool ValOK_T3 = false;
    bool ValOK_T4 = false;

    bool ValOK_C1 = false;
    bool ValOK_C2 = false;
    bool ValOK_C3 = false;
    bool ValOK_C4 = false;

    double T1_Val = Str_T1.toDouble(&ValOK_T1);
    double T2_Val = Str_T2.toDouble(&ValOK_T2);
    double T3_Val = Str_T3.toDouble(&ValOK_T3);
    double T4_Val = Str_T4.toDouble(&ValOK_T4);

    double C1_Val = Str_C1.toDouble(&ValOK_C1);
    double C2_Val = Str_C2.toDouble(&ValOK_C2);
    double C3_Val = Str_C3.toDouble(&ValOK_C3);
    double C4_Val = Str_C4.toDouble(&ValOK_C4);
    if(ValOK_T1 == false || ValOK_T2 == false || ValOK_T3 == false || ValOK_T4 == false
        || ValOK_C1 == false || ValOK_C2 == false || ValOK_C3 == false || ValOK_C4 == false
        || T1_Val == 0 || T2_Val == 0 || T3_Val == 0 || T4_Val == 0 || C1_Val == 0 || C2_Val == 0
        || C3_Val == 0 || C4_Val == 0)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_TCErr");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        isClicked = false;
        return;
    }
    if(!AglDeviceP->ReWriteFile("Standard_C1", Str_C1))
    {
        qDebug() << "Standard_C1 Error";
        isClicked = false;
        return;
    }
    if(!AglDeviceP->ReWriteFile("Standard_C2", Str_C2))
    {
        qDebug() << "Standard_C2 Error";
        isClicked = false;
        return;
    }
    if(!AglDeviceP->ReWriteFile("Standard_C3", Str_C3))
    {
        qDebug() << "Standard_C3 Error";
        isClicked = false;
        return;
    }
    if(!AglDeviceP->ReWriteFile("Standard_C4", Str_C4))
    {
        qDebug() << "Standard_C4 Error";
        isClicked = false;
        return;
    }
    if(!AglDeviceP->ReWriteFile("Standard_T1", Str_T1))
    {
        qDebug() << "Standard_T1 Error";
        isClicked = false;
        return;
    }
    if(!AglDeviceP->ReWriteFile("Standard_T2", Str_T2))
    {
        qDebug() << "Standard_T2 Error";
        isClicked = false;
        return;
    }
    if(!AglDeviceP->ReWriteFile("Standard_T3", Str_T3))
    {
        qDebug() << "Standard_T3 Error";
        isClicked = false;
        return;
    }
    if(!AglDeviceP->ReWriteFile("Standard_T4", Str_T4))
    {
        qDebug() << "Standard_T4 Error";
        isClicked = false;
        return;
    }
    if(!AglDeviceP->DeleteFile("password.txt"))
    {
        qDebug() << "Standard TC Delete file error!";
        isClicked = false;
        return;
    }
    if(!(AglDeviceP->WriteTxtToFile("password.txt", AglDeviceP->GetAllPass())))
    {
        qDebug() << "Standard TC Creat file error!";
        isClicked = false;
        return;
    }
    if(isCalibrationKey == true)
    {
        UiType = CalibrationType;
        CalibrationKey->close();
        isCalibrationKey = false;
    }
    CaliText->clear();
    QString Show_Data;

    Show_Data += AglDeviceP->GetConfig("Move_C") + ":";
    Show_Data += AglDeviceP->GetPasswd("@C_Distance") + "\r\n";

    Show_Data += AglDeviceP->GetConfig("TC_distance") + ":";
    Show_Data += AglDeviceP->GetPasswd("@DX") + "\r\n\r\n";

    Show_Data += "C1 = " + AglDeviceP->GetPasswd("@Standard_C1") + "\r\n";
    Show_Data += "T1 = " + AglDeviceP->GetPasswd("@Standard_T1") + "\r\n";
    Show_Data += "C2 = " + AglDeviceP->GetPasswd("@Standard_C2") + "\r\n";
    Show_Data += "T2 = " + AglDeviceP->GetPasswd("@Standard_T2") + "\r\n";
    Show_Data += "C3 = " + AglDeviceP->GetPasswd("@Standard_C3") + "\r\n";
    Show_Data += "T3 = " + AglDeviceP->GetPasswd("@Standard_T3") + "\r\n";
    Show_Data += "C4 = " + AglDeviceP->GetPasswd("@Standard_C4") + "\r\n";
    Show_Data += "T4 = " + AglDeviceP->GetPasswd("@Standard_T4") + "\r\n";

    Show_Data += "K1 = " + AglDeviceP->GetPasswd("@K1") + "\r\n";
    Show_Data += "B1 = " + AglDeviceP->GetPasswd("@B1") + "\r\n";
    Show_Data += "K2 = " + AglDeviceP->GetPasswd("@K2") + "\r\n";
    Show_Data += "B2 = " + AglDeviceP->GetPasswd("@B2") + "\r\n";
    Show_Data += "K3 = " + AglDeviceP->GetPasswd("@K3") + "\r\n";
    Show_Data += "B3 = " + AglDeviceP->GetPasswd("@B3") + "\r\n";
    Show_Data += "K4 = " + AglDeviceP->GetPasswd("@K4") + "\r\n";
    Show_Data += "B4 = " + AglDeviceP->GetPasswd("@B4") + "\r\n";
    Show_Data += "K5 = " + AglDeviceP->GetPasswd("@K5") + "\r\n";
    Show_Data += "B5 = " + AglDeviceP->GetPasswd("@B5") + "\r\n";
    Show_Data += "K6 = " + AglDeviceP->GetPasswd("@K6") + "\r\n";
    Show_Data += "B6 = " + AglDeviceP->GetPasswd("@B6") + "\r\n";
    Show_Data += "K7 = " + AglDeviceP->GetPasswd("@K7") + "\r\n";
    Show_Data += "B7 = " + AglDeviceP->GetPasswd("@B7") + "\r\n";
    Show_Data += "K8 = " + AglDeviceP->GetPasswd("@K8") + "\r\n";
    Show_Data += "B8 = " + AglDeviceP->GetPasswd("@B8") + "\r\n";

    CaliText->setText(Show_Data);
    isClicked = false;
}

void PctGuiThread::pressEdit_T1()
{
    Voice_pressKey();
    UiType = T1ui;
    KeyInput = Edit_T1->text();
    if(isCalibrationKey == true)
    {
        return;
    }
    isCalibrationKey = true;
    CreatCalibrationKey();
}

void PctGuiThread::pressEdit_T2()
{
    Voice_pressKey();
    UiType = T2ui;
    KeyInput = Edit_T2->text();
    if(isCalibrationKey == true)
    {
        return;
    }
    isCalibrationKey = true;
    CreatCalibrationKey();
}

void PctGuiThread::pressEdit_T3()
{
    Voice_pressKey();
    UiType = T3ui;
    KeyInput = Edit_T3->text();
    if(isCalibrationKey == true)
    {
        return;
    }
    isCalibrationKey = true;
    CreatCalibrationKey();
}

void PctGuiThread::pressEdit_T4()
{
    Voice_pressKey();
    UiType = T4ui;
    KeyInput = Edit_T4->text();
    if(isCalibrationKey == true)
    {
        return;
    }
    isCalibrationKey = true;
    CreatCalibrationKey();
}

void PctGuiThread::pressEdit_C1()
{
    Voice_pressKey();
    UiType = C1ui;
    KeyInput = Edit_C1->text();
    if(isCalibrationKey == true)
    {
        return;
    }
    isCalibrationKey = true;
    CreatCalibrationKey();
}

void PctGuiThread::pressEdit_C2()
{
    Voice_pressKey();
    UiType = C2ui;
    KeyInput = Edit_C2->text();
    if(isCalibrationKey == true)
    {
        return;
    }
    isCalibrationKey = true;
    CreatCalibrationKey();
}

void PctGuiThread::pressEdit_C3()
{
    Voice_pressKey();
    UiType = C3ui;
    KeyInput = Edit_C3->text();
    if(isCalibrationKey == true)
    {
        return;
    }
    isCalibrationKey = true;
    CreatCalibrationKey();
}

void PctGuiThread::pressEdit_C4()
{
    Voice_pressKey();
    UiType = C4ui;
    KeyInput = Edit_C4->text();
    if(isCalibrationKey == true)
    {
        return;
    }
    isCalibrationKey = true;
    CreatCalibrationKey();
}

void PctGuiThread::CreatCalibrationKey()
{
    KeyInput.clear();
    CalibrationKey = new QWidget(Widget_contrl);
    CalibrationKey->setGeometry(385, 70, 320, 220);
    CalibrationKey->setStyleSheet(QStringLiteral("background-color: rgb(0, 0, 0);"));

    Mybutton *DevButton_1 = new Mybutton(CalibrationKey);
    DevButton_1->setGeometry(QRect(15, 15, 86.6, 36.3));
    DevButton_1->setText("1");
    DevButton_1->setStyle(QStyleFactory::create("fusion"));
    DevButton_1->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_2 = new Mybutton(CalibrationKey);
    DevButton_2->setGeometry(QRect(116.6, 15, 86.6, 36.3));
    DevButton_2->setText("2");
    DevButton_2->setStyle(QStyleFactory::create("fusion"));
    DevButton_2->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_3 = new Mybutton(CalibrationKey);
    DevButton_3->setGeometry(QRect(218.2, 15, 86.6, 36.3));
    DevButton_3->setText("3");
    DevButton_3->setStyle(QStyleFactory::create("fusion"));
    DevButton_3->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_4 = new Mybutton(CalibrationKey);
    DevButton_4->setGeometry(QRect(15, 66.3, 86.6, 36.3));
    DevButton_4->setText("4");
    DevButton_4->setStyle(QStyleFactory::create("fusion"));
    DevButton_4->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_5 = new Mybutton(CalibrationKey);
    DevButton_5->setGeometry(QRect(116.6, 66.3, 86.6, 36.3));
    DevButton_5->setText("5");
    DevButton_5->setStyle(QStyleFactory::create("fusion"));
    DevButton_5->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_6 = new Mybutton(CalibrationKey);
    DevButton_6->setGeometry(QRect(218.2, 66.3, 86.6, 36.3));
    DevButton_6->setText("6");
    DevButton_6->setStyle(QStyleFactory::create("fusion"));
    DevButton_6->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_7 = new Mybutton(CalibrationKey);
    DevButton_7->setGeometry(QRect(15, 117.6, 86.6, 36.3));
    DevButton_7->setText("7");
    DevButton_7->setStyle(QStyleFactory::create("fusion"));
    DevButton_7->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_8 = new Mybutton(CalibrationKey);
    DevButton_8->setGeometry(QRect(116.6, 117.6, 86.6, 36.3));
    DevButton_8->setText("8");
    DevButton_8->setStyle(QStyleFactory::create("fusion"));
    DevButton_8->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_9 = new Mybutton(CalibrationKey);
    DevButton_9->setGeometry(QRect(218.2, 117.6, 86.6, 36.3));
    DevButton_9->setText("9");
    DevButton_9->setStyle(QStyleFactory::create("fusion"));
    DevButton_9->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_OK = new Mybutton(CalibrationKey);
    DevButton_OK->setGeometry(QRect(15, 168.9, 61.3, 36.3));
    DevButton_OK->setText("OK");
    DevButton_OK->setStyle(QStyleFactory::create("fusion"));
    DevButton_OK->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_Del = new Mybutton(CalibrationKey);
    DevButton_Del->setGeometry(QRect(91.3, 168.9, 61.3, 36.3));
    DevButton_Del->setText("Del");
    DevButton_Del->setStyle(QStyleFactory::create("fusion"));
    DevButton_Del->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *Button_dot = new Mybutton(CalibrationKey);
    Button_dot->setGeometry(QRect(167.6, 168.9, 61.3, 36.3));
    Button_dot->setText(".");
    Button_dot->setStyle(QStyleFactory::create("fusion"));
    Button_dot->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_0 = new Mybutton(CalibrationKey);
    DevButton_0->setGeometry(QRect(243.9, 168.9, 61.3, 36.3));
    DevButton_0->setText("0");
    DevButton_0->setStyle(QStyleFactory::create("fusion"));
    DevButton_0->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    CalibrationKey->show();

    connect(DevButton_0, SIGNAL(clicked()), this, SLOT(pressDevButton_0()),Qt::UniqueConnection);
    connect(DevButton_1, SIGNAL(clicked()), this, SLOT(pressDevButton_1()),Qt::UniqueConnection);
    connect(DevButton_2, SIGNAL(clicked()), this, SLOT(pressDevButton_2()),Qt::UniqueConnection);
    connect(DevButton_3, SIGNAL(clicked()), this, SLOT(pressDevButton_3()),Qt::UniqueConnection);
    connect(DevButton_4, SIGNAL(clicked()), this, SLOT(pressDevButton_4()),Qt::UniqueConnection);
    connect(DevButton_5, SIGNAL(clicked()), this, SLOT(pressDevButton_5()),Qt::UniqueConnection);
    connect(DevButton_6, SIGNAL(clicked()), this, SLOT(pressDevButton_6()),Qt::UniqueConnection);
    connect(DevButton_7, SIGNAL(clicked()), this, SLOT(pressDevButton_7()),Qt::UniqueConnection);
    connect(DevButton_8, SIGNAL(clicked()), this, SLOT(pressDevButton_8()),Qt::UniqueConnection);
    connect(DevButton_9, SIGNAL(clicked()), this, SLOT(pressDevButton_9()),Qt::UniqueConnection);
    connect(DevButton_OK, SIGNAL(clicked()), this, SLOT(pressCalibrationOK()),Qt::UniqueConnection);
    connect(DevButton_Del, SIGNAL(clicked()), this, SLOT(pressDevButton_Del()),Qt::UniqueConnection);
    connect(Button_dot, SIGNAL(clicked()), this, SLOT(pressDevButton_dot()),Qt::UniqueConnection);
}

void PctGuiThread::pressCalibrationOK()
{
     Voice_pressKey();
     UiType = CalibrationType;
     CalibrationKey->close();
     isCalibrationKey = false;
}

//void PctGuiThread::pressButton_Calib_C()
//{
//    Voice_pressKey();
//    if(isClicked == true)
//    {
//        isMessage = true;
//        msgBox = new QMessageBox(MainWindowP);
//        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//        QString loginerr = AglDeviceP->GetConfig("Message_TC_contrl");
//        msgBox->setText(loginerr);
//        msgBox->setWindowTitle("Warning");
//        msgBox->setIcon(QMessageBox :: Warning);
//        msgBox->exec();
//        delete msgBox;
//        if(isWidget == false)
//        {
//            Voice_pressKey();
//        }
//        isMessage = false;
//        return;
//    }
//    isClicked = true;
//    Label_station->clear();
//    QString QRCode = "YX-20170502,0012,1,1,2016/1/1,0,Hb,ng/ml,300,30000,1100,1500,700,1100,30,150,A-B,0.5,300,T,直线方程,1,1,1,1,37;";
//    if(!Code2Aanly(QRCode, &AnaFound))
//    {
//        return;
//    }
//    AglDeviceP->isTestContrl = false;
//    AglDeviceP->isCalibration = true;
//    AnalyTest();
//    AglDeviceP->agl_calc_vol(AnaFound);
//    CaliText->clear();
//    QString Show_Data;

//    Show_Data += AglDeviceP->GetConfig("Move_C") + ":";
//    Show_Data += AglDeviceP->GetPasswd("@C_Distance") + "\r\n";

//    Show_Data += AglDeviceP->GetConfig("TC_distance") + ":";
//    Show_Data += AglDeviceP->GetPasswd("@DX") + "\r\n\r\n";

//    Show_Data += "C1 = " + AglDeviceP->GetPasswd("@Standard_C1") + "\r\n";
//    Show_Data += "T1 = " + AglDeviceP->GetPasswd("@Standard_T1") + "\r\n";
//    Show_Data += "C2 = " + AglDeviceP->GetPasswd("@Standard_C2") + "\r\n";
//    Show_Data += "T2 = " + AglDeviceP->GetPasswd("@Standard_T2") + "\r\n";
//    Show_Data += "C3 = " + AglDeviceP->GetPasswd("@Standard_C3") + "\r\n";
//    Show_Data += "T3 = " + AglDeviceP->GetPasswd("@Standard_T3") + "\r\n";
//    Show_Data += "C4 = " + AglDeviceP->GetPasswd("@Standard_C4") + "\r\n";
//    Show_Data += "T4 = " + AglDeviceP->GetPasswd("@Standard_T4") + "\r\n";

//    Show_Data += "K1 = " + AglDeviceP->GetPasswd("@K1") + "\r\n";
//    Show_Data += "B1 = " + AglDeviceP->GetPasswd("@B1") + "\r\n";
//    Show_Data += "K2 = " + AglDeviceP->GetPasswd("@K2") + "\r\n";
//    Show_Data += "B2 = " + AglDeviceP->GetPasswd("@B2") + "\r\n";
//    Show_Data += "K3 = " + AglDeviceP->GetPasswd("@K3") + "\r\n";
//    Show_Data += "B3 = " + AglDeviceP->GetPasswd("@B3") + "\r\n";
//    Show_Data += "K4 = " + AglDeviceP->GetPasswd("@K4") + "\r\n";
//    Show_Data += "B4 = " + AglDeviceP->GetPasswd("@B4") + "\r\n";
//    Show_Data += "K5 = " + AglDeviceP->GetPasswd("@K5") + "\r\n";
//    Show_Data += "B5 = " + AglDeviceP->GetPasswd("@B5") + "\r\n";
//    Show_Data += "K6 = " + AglDeviceP->GetPasswd("@K6") + "\r\n";
//    Show_Data += "B6 = " + AglDeviceP->GetPasswd("@B6") + "\r\n";
//    Show_Data += "K7 = " + AglDeviceP->GetPasswd("@K7") + "\r\n";
//    Show_Data += "B7 = " + AglDeviceP->GetPasswd("@B7") + "\r\n";
//    Show_Data += "K8 = " + AglDeviceP->GetPasswd("@K8") + "\r\n";
//    Show_Data += "B8 = " + AglDeviceP->GetPasswd("@B8") + "\r\n";

//    Label_station->setText(AglDeviceP->GetConfig("Calibration_C"));
//    CaliText->setText(Show_Data);
//    isClicked = false;
//}

void PctGuiThread::pressButton_start()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_TC_contrl");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    CheckTime->stop();
    Delayms(50);

    isClicked = true;
    Label_station->clear();
//---------------------------------------------Add------------------------------------------//
    isCalibration = true;
    isCalibrationFourCard = false;

    QString QRCode_C = "YX-20170502,0012,1,1,2016/1/1,0,Hb,ng/ml,300,30000,1100,1500,700,1100,30,150,A-B,0.5,300,T,直线方程,1,1,1,1,37;";
    if(!Code2Aanly(QRCode_C, &AnaFound))
    {
        return;
    }
    AglDeviceP->isTestContrl = false;
    AglDeviceP->isCalibration = true;
    AddCheck = 0;
    AnalyTest();
    if(isYmaxError == true)
    {
        CaliText->clear();
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_YMaxErr_Cali"));
        msgBox->setIcon(QMessageBox ::Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        Voice_pressKey();
        isYmaxError = false;
        AglDeviceP->Error_Code = Error_OK;
        check_type = Check_NoNeed;
        isClicked = false;
        //-----------------------------------------------------------------//
        QString Str_K, Str_B;
        for(int i = 1; i <= 8; i++)
        {
            Str_K = "K" + QString::number(i);
            Str_B = "B" + QString::number(i);
            if(!AglDeviceP->ReWriteFile(Str_K, "1"))
            {
                qDebug() << "press_Resote_TC Rewite hostIP Error";
                isClicked = false;
                return;
            }
            if(!AglDeviceP->ReWriteFile(Str_B, "0"))
            {
                qDebug() << "press_Resote_TC Rewite hostIP Error";
                isClicked = false;
                return;
            }
        }
        if(!AglDeviceP->ReWriteFile("C_Distance", "0"))
        {
            qDebug() << "press_Resote_TC Rewite hostIP Error";
            isClicked = false;
            return;
        }
        if(!AglDeviceP->ReWriteFile("DX", "350"))
        {
            qDebug() << "press_Resote_TC Rewite hostIP Error";
            isClicked = false;
            return;
        }
        if(!(AglDeviceP->WriteTxtToFile("password.txt", AglDeviceP->GetAllPass())))
        {
            qDebug() << "press_Resote_TC Creat file error!";
            isClicked = false;
            return;
        }
        //-----------------------------------------------------------------//
        AglDeviceP->isTestContrl = false;
        AglDeviceP->isCalibration = false;
        return;
    }
    AglDeviceP->agl_calc_vol(AnaFound);
    Delayms(500);
//--------------------------------------------------------------------------------------------//
//    QString QRCode = "YX-20170502,0012,1,1,2016/1/1,0,Hb,ng/ml,300,30000,1100,1500,800,1100,30,150,A-B,0.5,300,T,直线方程,1,1,1,1,37;";
    QString QRCode = "YX-0001,123458,4,1,2016/1/1,0,PG1,ng/ml,1,1,1100,1500,700,1100,1,1,<A,1,1,T,直线方程,"  // 旧仪器二维码
                     "1,0,0,0,PG2,ng/ml,1,1,1100,1500,700,1100,1,1,<A,1,1,T,直线方程,"
                     "1,0,0,0,Hp,ng/ml,1,1,1100,1500,700,1100,1,1,<A,1,1,T,直线方程,"
                     "1,0,0,0,G17,ng/ml,1,1,1100,1500,700,1100,1,1,<A,1,1,T,直线方程,1,0,0,0,172;";
    if(!Code2Aanly(QRCode, &AnaFound))
    {
        isClicked = false;
        return;
    }
    AglDeviceP->isCalibration = false;
    AglDeviceP->isTestContrl = true;
    isCalibrationFourCard = false;
    AddCheck = 0;
    AnalyTest();
    CheckTime->stop();
    if(isYmaxError == true)
    {
        CaliText->clear();
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_YMaxErr_Cali"));
        msgBox->setIcon(QMessageBox ::Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        Voice_pressKey();
        isYmaxError = false;
        AglDeviceP->Error_Code = Error_OK;
        check_type = Check_NoNeed;
        isClicked = false;
        //-----------------------------------------------------------------//
        QString Str_K, Str_B;
        for(int i = 1; i <= 8; i++)
        {
            Str_K = "K" + QString::number(i);
            Str_B = "B" + QString::number(i);
            if(!AglDeviceP->ReWriteFile(Str_K, "1"))
            {
                qDebug() << "press_Resote_TC Rewite hostIP Error";
                isClicked = false;
                return;
            }
            if(!AglDeviceP->ReWriteFile(Str_B, "0"))
            {
                qDebug() << "press_Resote_TC Rewite hostIP Error";
                isClicked = false;
                return;
            }
        }
        if(!AglDeviceP->ReWriteFile("C_Distance", "0"))
        {
            qDebug() << "press_Resote_TC Rewite hostIP Error";
            isClicked = false;
            return;
        }
        if(!AglDeviceP->ReWriteFile("DX", "350"))
        {
            qDebug() << "press_Resote_TC Rewite hostIP Error";
            isClicked = false;
            return;
        }
        if(!(AglDeviceP->WriteTxtToFile("password.txt", AglDeviceP->GetAllPass())))
        {
            qDebug() << "press_Resote_TC Creat file error!";
            isClicked = false;
            return;
        }
        //-----------------------------------------------------------------//
        AglDeviceP->isTestContrl = false;
        AglDeviceP->isCalibration = false;
        return;
    }
    AglDeviceP->agl_calc_vol(AnaFound);
    CaliText->clear();

    QString Show_Data;

    Show_Data += AglDeviceP->GetConfig("Move_C") + ":";
    Show_Data += AglDeviceP->GetPasswd("@C_Distance") + "\r\n";

    Show_Data += AglDeviceP->GetConfig("TC_distance") + ":";
    Show_Data += AglDeviceP->GetPasswd("@DX") + "\r\n\r\n";

    Show_Data += "C1 = " + AglDeviceP->GetPasswd("@Standard_C1") + "\r\n";
    Show_Data += "T1 = " + AglDeviceP->GetPasswd("@Standard_T1") + "\r\n";
    Show_Data += "C2 = " + AglDeviceP->GetPasswd("@Standard_C2") + "\r\n";
    Show_Data += "T2 = " + AglDeviceP->GetPasswd("@Standard_T2") + "\r\n";
    Show_Data += "C3 = " + AglDeviceP->GetPasswd("@Standard_C3") + "\r\n";
    Show_Data += "T3 = " + AglDeviceP->GetPasswd("@Standard_T3") + "\r\n";
    Show_Data += "C4 = " + AglDeviceP->GetPasswd("@Standard_C4") + "\r\n";
    Show_Data += "T4 = " + AglDeviceP->GetPasswd("@Standard_T4") + "\r\n";

    Show_Data += "K1 = " + AglDeviceP->GetPasswd("@K1") + "\r\n";
    Show_Data += "B1 = " + AglDeviceP->GetPasswd("@B1") + "\r\n";
    Show_Data += "K2 = " + AglDeviceP->GetPasswd("@K2") + "\r\n";
    Show_Data += "B2 = " + AglDeviceP->GetPasswd("@B2") + "\r\n";
    Show_Data += "K3 = " + AglDeviceP->GetPasswd("@K3") + "\r\n";
    Show_Data += "B3 = " + AglDeviceP->GetPasswd("@B3") + "\r\n";
    Show_Data += "K4 = " + AglDeviceP->GetPasswd("@K4") + "\r\n";
    Show_Data += "B4 = " + AglDeviceP->GetPasswd("@B4") + "\r\n";
    Show_Data += "K5 = " + AglDeviceP->GetPasswd("@K5") + "\r\n";
    Show_Data += "B5 = " + AglDeviceP->GetPasswd("@B5") + "\r\n";
    Show_Data += "K6 = " + AglDeviceP->GetPasswd("@K6") + "\r\n";
    Show_Data += "B6 = " + AglDeviceP->GetPasswd("@B6") + "\r\n";
    Show_Data += "K7 = " + AglDeviceP->GetPasswd("@K7") + "\r\n";
    Show_Data += "B7 = " + AglDeviceP->GetPasswd("@B7") + "\r\n";
    Show_Data += "K8 = " + AglDeviceP->GetPasswd("@K8") + "\r\n";
    Show_Data += "B8 = " + AglDeviceP->GetPasswd("@B8") + "\r\n";

    if(!AglDeviceP->DeleteFile("password.txt"))
    {
        qDebug() << "Delete file error!";
    }
    if(!(AglDeviceP->WriteTxtToFile("password.txt", AglDeviceP->GetAllPass())))
    {
        qDebug() << "12690 Creat file error!";
    }

    AglDeviceP->ReloadPassConfig();
    CaliText->setText(Show_Data);
    Label_station->setText(AglDeviceP->GetConfig("Calibration_Value"));
    isClicked = false;
    AglDeviceP->isTestContrl = false;
    AglDeviceP->isCalibration = false;
}

void PctGuiThread::pressBut_con_ret()
{
    Voice_pressKey();
    if(UiType == T1ui || UiType == T2ui || UiType == T3ui || UiType == T4ui ||UiType == C1ui || UiType == C2ui || UiType == C3ui || UiType == C4ui)
    {
        CalibrationKey->close();
        UiType = CalibrationType;
        isCalibrationKey = false;
        return;
    }
    //------------------------------------------------Add----------------------------------------------//
    if(UiType == HostIpui || UiType == LisIpui || UiType == LisPortui)
    {
        DevNumKey->close();
        UiType = COSui;
    }
    if(isClicked == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_TC_contrl");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }

    int iCout = box_Lan->count();
    while(iCout--)
    {
        box_Lan->removeItem(0);
    }
    iEditTime = 0;
    iDate = 0;
    QString cosedip = AglDeviceP->GetPasswd("@hostIP");
    Edit_IP->setText(cosedip);
    QString lisedip = AglDeviceP->GetPasswd("@LisIP");
    Edit_LisIP->setText(lisedip);
    QString cosedpo = AglDeviceP->GetPasswd("@LisPort");
    Edit_LisPort->setText(cosedpo);
//-----------------------------------new langeuage display-------------------------------------//
    QString ReadLangeuage = AglDeviceP->GetConfig("Reader_Language");
    QStringList LanguageList = ReadLangeuage.split("///");
    int Num_Language = LanguageList.length();
    if(Num_Language < 2)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText("config file Loss or Error!");
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    for(int i = 0; i < Num_Language; i++)
    {
        box_Lan->addItem(LanguageList.at(i), i);
    }
//--------------------------------------old langeuage display---------------------------------------//
//    QString coseditla1 = AglDeviceP->GetConfig("COS_Lan1");
//    QString coseditla2 = AglDeviceP->GetConfig("COS_Lan2");

//    if(AglDeviceP->GetPasswd("@Languag") == "English")
//    {
//        DefaultLanguage = coseditla1;
//        box_Lan->addItem(coseditla1, 0);
//        box_Lan->addItem(coseditla2, 1);
//    }
//    else
//    {
//        DefaultLanguage = coseditla2;
//        box_Lan->addItem(coseditla2, 0);
//        box_Lan->addItem(coseditla1, 1);
//    }
//----------------------------------------------------------------------------------------------------------//
    if(AglDeviceP->GetPasswd("@AUTO") == "YES")
    {
        Box_Send->setChecked(true);
    }
    else
    {
        Box_Send->setChecked(false);
    }
    if(AglDeviceP->GetPasswd("@QRcode") == "YES")
    {
        Box_QRcode->setChecked(true);
    }
    else
    {
        Box_QRcode->setChecked(false);
    }

    QString AUTO_sta;
    if(Box_Send->isChecked())
    {
        AUTO_sta = "Checked";
    }
    else
    {
        AUTO_sta = "DisChecked";
    }

    QString QRCode_sta;
    if(Box_QRcode->isChecked())
    {
        QRCode_sta = "USE";
    }
    else
    {
        QRCode_sta = "NOUSE";
    }
    //---------------------------------------------------------------------------------------------------//
    OldUserData = Edit_IP->text()+Edit_LisIP->text()+Edit_LisPort->text()+Edit_data->text()+Edit_time->text()+box_Lan->currentText()+AUTO_sta+QRCode_sta;
    Widget_contrl->close();
    Label_station->setText(AglDeviceP->GetConfig("station_Dev"));
    isCalib = false;
    isSaveDev = false;
}

void PctGuiThread::pressBut_con_home()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_TC_contrl");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    KeyInput.clear();
    Widget_contrl->close();
    COSWidget->close();
    SetWidget->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    isSaveDev = false;
    isCalib = false;
    UiType = MainUi;
}

//----------------------------------------------------------------------------------------------------------------------------------------//
void PctGuiThread :: pressEdit_hostIP()
{
    Voice_pressKey();
    if(UiType == HostIpui)
    {
        KeyInput = Edit_IP->text();
        return;
    }
    else if(UiType == LisIpui)
    {
        KeyInput = Edit_IP->text();
        UiType = HostIpui;
        return;
    }
    else if(UiType == LisPortui)
    {
        DevButton_dot->setDisabled(false);
        KeyInput = Edit_IP->text();
        UiType = HostIpui;
        return;
    }
    UiType = HostIpui;
    CreatDevNumKey();
    KeyInput = Edit_IP->text();
}

void PctGuiThread :: pressEdit_LisIP()
{
    Voice_pressKey();
    if(UiType == HostIpui)
    {
        KeyInput = Edit_LisIP->text();
        UiType = LisIpui;
        return;
    }
    else if(UiType == LisIpui)
    {
        KeyInput = Edit_LisIP->text();
        return;
    }
    else if(UiType == LisPortui)
    {
        DevButton_dot->setDisabled(false);
        KeyInput = Edit_LisIP->text();
        UiType = LisIpui;
        return;
    }
    UiType = LisIpui;
    CreatDevNumKey();
    KeyInput = Edit_LisIP->text();
}

void PctGuiThread :: pressEdit_LisPort()
{
    Voice_pressKey();
    if(UiType == HostIpui)
    {
        KeyInput = Edit_LisPort->text();
        DevButton_dot->setDisabled(true);
        UiType = LisPortui;
        return;
    }
    else if(UiType == LisIpui)
    {
        KeyInput = Edit_LisPort->text();
        DevButton_dot->setDisabled(true);
        UiType = LisPortui;
        return;
    }
    else if(UiType == LisPortui)
    {
        KeyInput = Edit_LisPort->text();
        return;
    }
    UiType = LisPortui;
    CreatDevNumKey();
    DevButton_dot->setDisabled(true);
    KeyInput = Edit_LisPort->text();
}
//-------------------------------------------------------------------------------------------------------------------------------------------//
void PctGuiThread :: CreatDevNumKey()
{
    isKeyWidget = true;
    KeyInput.clear();
    DevNumKey = new QWidget(Group_COS);
    DevNumKey->setGeometry(420, 85, 320, 220);
    DevNumKey->setStyleSheet(QStringLiteral("background-color: rgb(0, 0, 0);"));

    Mybutton *DevButton_1 = new Mybutton(DevNumKey);
    DevButton_1->setGeometry(QRect(15, 15, 86.6, 36.3));
    DevButton_1->setText("1");
    DevButton_1->setStyle(QStyleFactory::create("fusion"));
    DevButton_1->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_2 = new Mybutton(DevNumKey);
    DevButton_2->setGeometry(QRect(116.6, 15, 86.6, 36.3));
    DevButton_2->setText("2");
    DevButton_2->setStyle(QStyleFactory::create("fusion"));
    DevButton_2->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_3 = new Mybutton(DevNumKey);
    DevButton_3->setGeometry(QRect(218.2, 15, 86.6, 36.3));
    DevButton_3->setText("3");
    DevButton_3->setStyle(QStyleFactory::create("fusion"));
    DevButton_3->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_4 = new Mybutton(DevNumKey);
    DevButton_4->setGeometry(QRect(15, 66.3, 86.6, 36.3));
    DevButton_4->setText("4");
    DevButton_4->setStyle(QStyleFactory::create("fusion"));
    DevButton_4->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_5 = new Mybutton(DevNumKey);
    DevButton_5->setGeometry(QRect(116.6, 66.3, 86.6, 36.3));
    DevButton_5->setText("5");
    DevButton_5->setStyle(QStyleFactory::create("fusion"));
    DevButton_5->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_6 = new Mybutton(DevNumKey);
    DevButton_6->setGeometry(QRect(218.2, 66.3, 86.6, 36.3));
    DevButton_6->setText("6");
    DevButton_6->setStyle(QStyleFactory::create("fusion"));
    DevButton_6->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_7 = new Mybutton(DevNumKey);
    DevButton_7->setGeometry(QRect(15, 117.6, 86.6, 36.3));
    DevButton_7->setText("7");
    DevButton_7->setStyle(QStyleFactory::create("fusion"));
    DevButton_7->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_8 = new Mybutton(DevNumKey);
    DevButton_8->setGeometry(QRect(116.6, 117.6, 86.6, 36.3));
    DevButton_8->setText("8");
    DevButton_8->setStyle(QStyleFactory::create("fusion"));
    DevButton_8->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_9 = new Mybutton(DevNumKey);
    DevButton_9->setGeometry(QRect(218.2, 117.6, 86.6, 36.3));
    DevButton_9->setText("9");
    DevButton_9->setStyle(QStyleFactory::create("fusion"));
    DevButton_9->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_OK = new Mybutton(DevNumKey);
    DevButton_OK->setGeometry(QRect(15, 168.9, 61.3, 36.3));
    DevButton_OK->setText("OK");
    DevButton_OK->setStyle(QStyleFactory::create("fusion"));
    DevButton_OK->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_Del = new Mybutton(DevNumKey);
    DevButton_Del->setGeometry(QRect(91.3, 168.9, 61.3, 36.3));
    DevButton_Del->setText("Del");
    DevButton_Del->setStyle(QStyleFactory::create("fusion"));
    DevButton_Del->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    DevButton_dot = new Mybutton(DevNumKey);
    DevButton_dot->setGeometry(QRect(167.6, 168.9, 61.3, 36.3));
    DevButton_dot->setText(".");
    DevButton_dot->setStyle(QStyleFactory::create("fusion"));
    DevButton_dot->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    Mybutton *DevButton_0 = new Mybutton(DevNumKey);
    DevButton_0->setGeometry(QRect(243.9, 168.9, 61.3, 36.3));
    DevButton_0->setText("0");
    DevButton_0->setStyle(QStyleFactory::create("fusion"));
    DevButton_0->setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));

    DevNumKey->show();

    connect(DevButton_0, SIGNAL(clicked()), this, SLOT(pressDevButton_0()),Qt::UniqueConnection);
    connect(DevButton_1, SIGNAL(clicked()), this, SLOT(pressDevButton_1()),Qt::UniqueConnection);
    connect(DevButton_2, SIGNAL(clicked()), this, SLOT(pressDevButton_2()),Qt::UniqueConnection);
    connect(DevButton_3, SIGNAL(clicked()), this, SLOT(pressDevButton_3()),Qt::UniqueConnection);
    connect(DevButton_4, SIGNAL(clicked()), this, SLOT(pressDevButton_4()),Qt::UniqueConnection);
    connect(DevButton_5, SIGNAL(clicked()), this, SLOT(pressDevButton_5()),Qt::UniqueConnection);
    connect(DevButton_6, SIGNAL(clicked()), this, SLOT(pressDevButton_6()),Qt::UniqueConnection);
    connect(DevButton_7, SIGNAL(clicked()), this, SLOT(pressDevButton_7()),Qt::UniqueConnection);
    connect(DevButton_8, SIGNAL(clicked()), this, SLOT(pressDevButton_8()),Qt::UniqueConnection);
    connect(DevButton_9, SIGNAL(clicked()), this, SLOT(pressDevButton_9()),Qt::UniqueConnection);
    connect(DevButton_OK, SIGNAL(clicked()), this, SLOT(pressDevButton_OK()),Qt::UniqueConnection);
    connect(DevButton_Del, SIGNAL(clicked()), this, SLOT(pressDevButton_Del()),Qt::UniqueConnection);
    connect(DevButton_dot, SIGNAL(clicked()), this, SLOT(pressDevButton_dot()),Qt::UniqueConnection);
}

void PctGuiThread :: pressDevButton_0()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + "0";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + "0";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "0";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "0";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "0";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "0";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "0";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "0";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "0";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "0";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "0";
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_1()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + "1";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + "1";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "1";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "1";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "1";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "1";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "1";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "1";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "1";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "1";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "1";
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_2()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + "2";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + "2";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "2";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "2";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "2";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "2";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "2";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "2";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "2";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "2";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "2";
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_3()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + "3";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + "3";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "3";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "3";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "3";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "3";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "3";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "3";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "3";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "3";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "3";
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_4()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + "4";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + "4";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "4";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "4";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "4";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "4";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "4";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "4";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "4";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "4";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "4")
        {
            return;
        }
        KeyInput = KeyInput + "4";
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_5()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + "5";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + "5";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "5";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "5";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "5";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "5";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "5";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "5";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "5";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "5";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "5";
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_6()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + "6";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + "6";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "6";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "6";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "6";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "6";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "6";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "6";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "6";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "6";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "6";
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_7()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + "7";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + "7";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "7";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "7";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "7";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "7";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "7";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "7";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "7";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "7";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "7";
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_8()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        QMessageBox *msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + "8";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + "8";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "8";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "8";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "8";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "8";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "8";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "8";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "8";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "8";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "8";
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_9()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + "9";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + "9";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "9";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "9";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "9";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "9";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "9";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "9";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "9";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "9";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + "9";
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_OK()
{
    if(isKeyWidget == true)
    {
        Voice_pressKey();
    }
    bool ok;
    QString IPAddr = Edit_IP->text();
    QStringList IPList = IPAddr.split(".");
    if(IPList.length() != 4)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    int IP1 = IPList.at(0).toInt(&ok, 10);
    int IP2 = IPList.at(1).toInt(&ok, 10);
    int IP3 = IPList.at(2).toInt(&ok, 10);
    int IP4 = IPList.at(3).toInt(&ok, 10);

    if((IPList.at(0).left(1) == "0" &&  IPList.at(0).length() != 1)|| (IPList.at(1).left(1) == "0" &&  IPList.at(1).length() != 1) || (IPList.at(2).left(1) == "0" &&  IPList.at(2).length() != 1) || (IPList.at(3).left(1) == "0" &&  IPList.at(3).length() != 1))
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(IP1 > 255 || IP2 > 255 || IP3 > 255 || IP4 > 255)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(Edit_IP->text() == Edit_LisIP->text())
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_double");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    IPAddr = Edit_LisIP->text();
    IPList = IPAddr.split(".");
    if(IPList.length() != 4)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }

    if((IPList.at(0).left(1) == "0" &&  IPList.at(0).length() != 1)|| (IPList.at(1).left(1) == "0" &&  IPList.at(1).length() != 1) || (IPList.at(2).left(1) == "0" &&  IPList.at(2).length() != 1) || (IPList.at(3).left(1) == "0" &&  IPList.at(3).length() != 1))
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    IP1 = IPList.at(0).toInt(&ok, 10);
    IP2 = IPList.at(1).toInt(&ok, 10);
    IP3 = IPList.at(2).toInt(&ok, 10);
    IP4 = IPList.at(3).toInt(&ok, 10);
    if(IP1 > 255 || IP2 > 255 || IP3 > 255 || IP4 > 255)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }

    int port = Edit_LisPort->text().toInt(&ok, 10);
    if(port > 65535)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }

    DevNumKey->close();
    UiType = COSui;
    isKeyWidget = false;
}

void PctGuiThread :: pressDevButton_Del()
{
    Voice_pressKey();
    int lengh = KeyInput.length();
    KeyInput = KeyInput.remove(lengh - 1, 1);
    if(UiType == HostIpui)
    {
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        Edit_C4->setText(KeyInput);
    }
    else
    {
        Edit_LisPort->setText(KeyInput);
    }
}

void PctGuiThread :: pressDevButton_dot()
{
    Voice_pressKey();
    if(KeyInput.length() > 15)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == HostIpui)
    {
        KeyInput = KeyInput + ".";
        Edit_IP->setText(KeyInput);
    }
    else if(UiType == LisIpui)
    {
        KeyInput = KeyInput + ".";
        Edit_LisIP->setText(KeyInput);
    }
    else if(UiType == T1ui)
    {
        if(KeyInput.isEmpty())
        {
            return;
        }
        QStringList ListInput = KeyInput.split(".");
        if(ListInput.length() > 1)
        {
            return;
        }
        KeyInput = KeyInput + ".";
        Edit_T1->setText(KeyInput);
    }
    else if(UiType == T2ui)
    {
        if(KeyInput.isEmpty())
        {
            return;
        }
        QStringList ListInput = KeyInput.split(".");
        if(ListInput.length() > 1)
        {
            return;
        }
        KeyInput = KeyInput + ".";
        Edit_T2->setText(KeyInput);
    }
    else if(UiType == T3ui)
    {
        if(KeyInput.isEmpty())
        {
            return;
        }
        QStringList ListInput = KeyInput.split(".");
        if(ListInput.length() > 1)
        {
            return;
        }
        KeyInput = KeyInput + ".";
        Edit_T3->setText(KeyInput);
    }
    else if(UiType == T4ui)
    {
        if(KeyInput.isEmpty())
        {
            return;
        }
        QStringList ListInput = KeyInput.split(".");
        if(ListInput.length() > 1)
        {
            return;
        }
        KeyInput = KeyInput + ".";
        Edit_T4->setText(KeyInput);
    }
    else if(UiType == C1ui)
    {
        if(KeyInput.isEmpty())
        {
            return;
        }
        QStringList ListInput = KeyInput.split(".");
        if(ListInput.length() > 1)
        {
            return;
        }
        KeyInput = KeyInput + ".";
        Edit_C1->setText(KeyInput);
    }
    else if(UiType == C2ui)
    {
        if(KeyInput.isEmpty())
        {
            return;
        }
        QStringList ListInput = KeyInput.split(".");
        if(ListInput.length() > 1)
        {
            return;
        }
        KeyInput = KeyInput + ".";
        Edit_C2->setText(KeyInput);
    }
    else if(UiType == C3ui)
    {
        if(KeyInput.isEmpty())
        {
            return;
        }
        QStringList ListInput = KeyInput.split(".");
        if(ListInput.length() > 1)
        {
            return;
        }
        KeyInput = KeyInput + ".";
        Edit_C3->setText(KeyInput);
    }
    else if(UiType == C4ui)
    {
        if(KeyInput.isEmpty())
        {
            return;
        }
        QStringList ListInput = KeyInput.split(".");
        if(ListInput.length() > 1)
        {
            return;
        }
        KeyInput = KeyInput + ".";
        Edit_C4->setText(KeyInput);
    }
    else
    {
        if(KeyInput == "0")
        {
            return;
        }
        KeyInput = KeyInput + ".";
        Edit_LisPort->setText(KeyInput);
    }
}
//------Add------liuyouwei------------------------------------------------------------------------------------------------------------------//
void PctGuiThread :: pressButton_Save()
{
    Voice_pressKey();
    if(UiType == LisIpui || UiType == LisPortui || UiType == HostIpui)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_sam");
        msgBox->setText(loginerr);
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setWindowTitle("Question");
        msgBox->setIcon(QMessageBox :: Question);
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                Voice_pressKey();
                msgBox->hide();
                //------------------------------------------------------------------------------------------------//
                if(isKeyWidget == true)
                {
                    Voice_pressKey();
                }
                bool ok;
                QString IPAddr = Edit_IP->text();
                QStringList IPList = IPAddr.split(".");
                if(IPList.length() != 4)
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    isMessage_child = false;
                    isMessage = false;
                    return;
                }
                int IP1 = IPList.at(0).toInt(&ok, 10);
                int IP2 = IPList.at(1).toInt(&ok, 10);
                int IP3 = IPList.at(2).toInt(&ok, 10);
                int IP4 = IPList.at(3).toInt(&ok, 10);

                if((IPList.at(0).left(1) == "0" &&  IPList.at(0).length() != 1)|| (IPList.at(1).left(1) == "0" &&  IPList.at(1).length() != 1) || (IPList.at(2).left(1) == "0" &&  IPList.at(2).length() != 1) || (IPList.at(3).left(1) == "0" &&  IPList.at(3).length() != 1))
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    isMessage_child = false;
                    isMessage = false;
                    return;
                }
                if(IP1 > 255 || IP2 > 255 || IP3 > 255 || IP4 > 255)
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    isMessage_child = false;
                    isMessage = false;
                    return;
                }
                if(Edit_IP->text() == Edit_LisIP->text())
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_double");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    isMessage_child = false;
                    isMessage = false;
                    return;
                }
                IPAddr = Edit_LisIP->text();
                IPList = IPAddr.split(".");
                if(IPList.length() != 4)
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    isMessage_child = false;
                    isMessage = false;
                    return;
                }

                if((IPList.at(0).left(1) == "0" &&  IPList.at(0).length() != 1)|| (IPList.at(1).left(1) == "0" &&  IPList.at(1).length() != 1) || (IPList.at(2).left(1) == "0" &&  IPList.at(2).length() != 1) || (IPList.at(3).left(1) == "0" &&  IPList.at(3).length() != 1))
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    isMessage_child = false;
                    isMessage = false;
                    return;
                }
                IP1 = IPList.at(0).toInt(&ok, 10);
                IP2 = IPList.at(1).toInt(&ok, 10);
                IP3 = IPList.at(2).toInt(&ok, 10);
                IP4 = IPList.at(3).toInt(&ok, 10);
                if(IP1 > 255 || IP2 > 255 || IP3 > 255 || IP4 > 255)
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    isMessage_child = false;
                    isMessage = false;
                    return;
                }

                int port = Edit_LisPort->text().toInt(&ok, 10);
                if(port > 65535)
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_Lang");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    isMessage_child = false;
                    isMessage = false;
                    return;
                }
                //------------------------------------------------------------------------------------------------//
                SaveSet();
                isSaveDev = true;
                isMessage_child = true;
                msgBox_child = new QMessageBox(COSWidget);
                msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Message_succsave");
                msgBox_child->setText(loginerr);
                msgBox_child->setWindowTitle("Warning");
                msgBox_child->setIcon(QMessageBox :: Information);
                msgBox_child->exec();
                delete msgBox_child;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                isMessage_child = false;
                break;
            }
            case QMessageBox::No:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                break;
            }
        }
        delete msgBox;
        isMessage = false;
        return;
    }
//----------------------------------------------------------------------------------------------------------------------------//
    QString IPAddr = Edit_IP->text();
    QStringList IPList = IPAddr.split(".");
    bool ok = false;
    if(IPList.length() != 4)
    {
        isMessage_child = true;
        msgBox_child = new QMessageBox(COSWidget);
        msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox_child->setText(loginerr);
        msgBox_child->setWindowTitle("Warning");
        msgBox_child->setIcon(QMessageBox :: Warning);
        msgBox_child->exec();
        delete msgBox_child;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage_child = false;
        isMessage = false;
        return;
    }
    int IP1 = IPList.at(0).toInt(&ok, 10);
    int IP2 = IPList.at(1).toInt(&ok, 10);
    int IP3 = IPList.at(2).toInt(&ok, 10);
    int IP4 = IPList.at(3).toInt(&ok, 10);

    if((IPList.at(0).left(1) == "0" &&  IPList.at(0).length() != 1)|| (IPList.at(1).left(1) == "0" &&  IPList.at(1).length() != 1) || (IPList.at(2).left(1) == "0" &&  IPList.at(2).length() != 1) || (IPList.at(3).left(1) == "0" &&  IPList.at(3).length() != 1))
    {
        isMessage_child = true;
        msgBox_child = new QMessageBox(COSWidget);
        msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox_child->setText(loginerr);
        msgBox_child->setWindowTitle("Warning");
        msgBox_child->setIcon(QMessageBox :: Warning);
        msgBox_child->exec();
        delete msgBox_child;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage_child = false;
        isMessage = false;
        return;
    }
    if(IP1 > 255 || IP2 > 255 || IP3 > 255 || IP4 > 255)
    {
        isMessage_child = true;
        msgBox_child = new QMessageBox(COSWidget);
        msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox_child->setText(loginerr);
        msgBox_child->setWindowTitle("Warning");
        msgBox_child->setIcon(QMessageBox :: Warning);
        msgBox_child->exec();
        delete msgBox_child;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage_child = false;
        isMessage = false;
        return;
    }
    if(Edit_IP->text() == Edit_LisIP->text())
    {
        isMessage_child = true;
        msgBox_child = new QMessageBox(COSWidget);
        msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_double");
        msgBox_child->setText(loginerr);
        msgBox_child->setWindowTitle("Warning");
        msgBox_child->setIcon(QMessageBox :: Warning);
        msgBox_child->exec();
        delete msgBox_child;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage_child = false;
        isMessage = false;
        return;
    }
    IPAddr = Edit_LisIP->text();
    IPList = IPAddr.split(".");
    if(IPList.length() != 4)
    {
        isMessage_child = true;
        msgBox_child = new QMessageBox(COSWidget);
        msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox_child->setText(loginerr);
        msgBox_child->setWindowTitle("Warning");
        msgBox_child->setIcon(QMessageBox :: Warning);
        msgBox_child->exec();
        delete msgBox_child;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage_child = false;
        isMessage = false;
        return;
    }
    if((IPList.at(0).left(1) == "0" &&  IPList.at(0).length() != 1)|| (IPList.at(1).left(1) == "0" &&  IPList.at(1).length() != 1) || (IPList.at(2).left(1) == "0" &&  IPList.at(2).length() != 1) || (IPList.at(3).left(1) == "0" &&  IPList.at(3).length() != 1))
    {
        isMessage_child = true;
        msgBox_child = new QMessageBox(COSWidget);
        msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox_child->setText(loginerr);
        msgBox_child->setWindowTitle("Warning");
        msgBox_child->setIcon(QMessageBox :: Warning);
        msgBox_child->exec();
        delete msgBox_child;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage_child = false;
        isMessage = false;
        return;
    }
    IP1 = IPList.at(0).toInt(&ok, 10);
    IP2 = IPList.at(1).toInt(&ok, 10);
    IP3 = IPList.at(2).toInt(&ok, 10);
    IP4 = IPList.at(3).toInt(&ok, 10);
    if(IP1 > 255 || IP2 > 255 || IP3 > 255 || IP4 > 255)
    {
        isMessage_child = true;
        msgBox_child = new QMessageBox(COSWidget);
        msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
        msgBox_child->setText(loginerr);
        msgBox_child->setWindowTitle("Warning");
        msgBox_child->setIcon(QMessageBox :: Warning);
        msgBox_child->exec();
        delete msgBox_child;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage_child = false;
        isMessage = false;
        return;
    }
    int port = Edit_LisPort->text().toInt(&ok, 10);
    if(port > 65535)
    {
        isMessage_child = true;
        msgBox_child = new QMessageBox(COSWidget);
        msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Lang");
        msgBox_child->setText(loginerr);
        msgBox_child->setWindowTitle("Warning");
        msgBox_child->setIcon(QMessageBox :: Warning);
        msgBox_child->exec();
        delete msgBox_child;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage_child = false;
        isMessage = false;
        return;
    }
//----------------------------------------------------------------------------------------------------------------------------//
    SaveSet();
    if(isSaveDev == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(COSWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_succsave");
        msgBox->setText(loginerr);
        msgBox->setIcon(QMessageBox::Warning);
        msgBox->setWindowTitle("Warning");
        msgBox->exec();
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        msgBox->close();
        delete msgBox;
        isMessage = false;
        return;
    }
}

void PctGuiThread :: SaveSet()
{
    QString OldLang = AglDeviceP->GetPasswd("@Languag");
    QString Lang;
    QString CurrentLanguage = box_Lan->currentText();
    if(OldLang == "English")
    {
        Lang = CurrentLanguage;
    }
    else if(OldLang == "Chinese")
    {
        if(CurrentLanguage == "中 文")
        {
            Lang = "Chinese";
        }
        else if(CurrentLanguage == "英 文")
        {
            Lang = "English";
        }
    }
//---------------------------可以在这里添加新语言------------------------------//

//    if(box_Lan->currentText() == "中 文" || box_Lan->currentText() == "Chinese")
//    {
//        Lang = "Chinese";
//    }
//    if(box_Lan->currentText() == "英 文" || box_Lan->currentText() == "English")
//    {
//        Lang = "English";
//    }
    QString Send_auto;
    if(Box_Send->isChecked())
    {
        Send_auto = "YES";
    }
    else
    {
        Send_auto = "NOAUTO";
    }

    QString QRcode;
    if(Box_QRcode->isChecked())
    {
        QRcode = "YES";
    }
    else
    {
        QRcode = "NOUSE";
    }
    QString AUTO_sta;
    if(Box_Send->isChecked())
    {
        AUTO_sta = "Checked";
    }
    else
    {
        AUTO_sta = "DisChecked";
    }

    QString QRCode_sta;
    if(Box_QRcode->isChecked())
    {
        QRCode_sta = "USE";
    }
    else
    {
        QRCode_sta = "NOUSE";
    }
    //---------------------------------------------------------------------------//    
        QString Time = Edit_data->text() + " " + Edit_time->text();
        isSaveDev = true;
        if(!AglDeviceP->ReWriteFile("hostIP", Edit_IP->text()))
        {
            qDebug() << "Rewite hostIP Error";
            return;
        }
        if(!AglDeviceP->ReWriteFile("LisIP", Edit_LisIP->text()))
        {
            qDebug() << "Rewite LisIP Error";
            return;
        }
        if(!AglDeviceP->ReWriteFile("LisPort", Edit_LisPort->text()))
        {
            qDebug() << "Rewite LisPort Error";
            return;
        }
        if(!AglDeviceP->ReWriteFile("AUTO", Send_auto))
        {
            qDebug() << "Rewite LisIP Error";
            return;
        }
        if(!AglDeviceP->ReWriteFile("QRcode", QRcode))
        {
            qDebug() << "Rewite QRcode Error";
            return;
        }
        if(Lang == OldLang)
        {
            if(!AglDeviceP->ReWriteFile("Languag", Lang))
            {
                qDebug() << "Rewite Language Error";
                return;
            }
            if(!AglDeviceP->DeleteFile("password.txt"))
            {
                qDebug() << "Delete file error!";
            }
            if(!(AglDeviceP->WriteTxtToFile("password.txt", AglDeviceP->GetAllPass())))
            {
                qDebug() << "Creat file error!";
            }
        }
        AglDeviceP->SetRtcTime(Time);
        AglDeviceP->SetLocalIp(Edit_IP->text());
        qDebug() << "isSaveDev is " << isSaveDev;
        qDebug() << "Lang is " << Lang;
        qDebug() << "OldLang is " << OldLang;

        if(Lang != OldLang)
        {
            isMessage_Save = true;
            msgBox_Save = new QMessageBox(COSWidget);
            msgBox_Save->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            QString loginerr = AglDeviceP->GetConfig("Message_langchange");
            msgBox_Save->setText(loginerr);
            msgBox_Save->setWindowTitle("Warning");
            msgBox_Save->setIcon(QMessageBox :: Question);
            msgBox_Save->exec();
            Voice_pressKey();
            if(!AglDeviceP->ReWriteFile("Languag", Lang))
            {
                qDebug() << "Rewite Language Error";
                return;
            }
            if(!AglDeviceP->DeleteFile("password.txt"))
            {
                qDebug() << "Delete file error!";
            }
            if(!(AglDeviceP->WriteTxtToFile("password.txt", AglDeviceP->GetAllPass())))
            {
                qDebug() << "Creat file error!";
            }
//            AglDeviceP->SysCall("mv config.txt config_save.txt");
//            AglDeviceP->SysCall("mv config_bak.txt config.txt");
//            AglDeviceP->SysCall("mv config_save.txt config_bak.txt");
            AglDeviceP->ReBoot();
            isMessage_Save = false;
            delete msgBox_Save;
        }
}

void PctGuiThread :: pressBut_dev_cali()
{
    Voice_pressKey();
    if(UiType == HostIpui || UiType == LisIpui || UiType == LisPortui)
    {
        return;
    }
    isMessage = true;
    msgBox = new QMessageBox(COSWidget);
    msgBox->setWindowTitle("Warning");
    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180,180,180);"));
    msgBox->setText(AglDeviceP->GetConfig("Message_jiaozhun"));
    msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::No);
    msgBox->setIcon(QMessageBox :: Question);
    int ret = msgBox->exec();
    switch (ret)
    {
        case QMessageBox::Yes:
        {
            Voice_pressKey();
            AglDeviceP->TS_Calibrate();
            AglDeviceP->ReBoot();
            break;
        }
        case QMessageBox::No:
        {
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            msgBox->close();
            break;
        }
    }
    delete msgBox;
    isMessage = false;
}
//------Add------liuyouwei---------------------------------------------------------------------------------------------------------//
void PctGuiThread :: CreatUpdate()                  //  升级界面
{
    UiType = Updateui;
    UpWidget = new QWidget(UsrWidget);
    UpWidget->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    UpWidget->setGeometry(0, 0, 803, 348);

    QFont qFont;
    qFont.setPointSize(16);
    QLabel *TitelLabel = new QLabel(UpWidget);
    TitelLabel->setGeometry(250, 30, 300, 40);
    TitelLabel->setAlignment(Qt::AlignCenter);
    QString Titel = AglDeviceP->GetConfig("Uptitle");
    TitelLabel->setText(Titel);
    TitelLabel->setFont(qFont);

    Mybutton *Up_Ret = new Mybutton(UpWidget);
    Up_Ret->setGeometry(11, 290, 48, 48);
    Button_SetPix(Up_Ret, MainSetpix_3);

    Mybutton *Up_Home = new Mybutton(UpWidget);
    Up_Home->setGeometry(13, 11, 48, 48);
    Button_SetPix(Up_Home, MainSetpix_Home);

    QPixmap Pix_Update;
    QPixmap Pix_Update_Gujian;
    QPixmap Pix_Datein;
    QPixmap Pix_Dateout;
    QPixmap Pix_Config;
    QPixmap Pix_outcon;
    //------------------------//
    QPixmap Pix_Outapp;
    //------------------------//

    QString Update_path = AglDeviceP->GetPasswd("@Updatepix");
    QString Update_path_gujian = AglDeviceP->GetPasswd("@UpdateGujian");
    QString Datein_path = AglDeviceP->GetPasswd("@Dateinpix");
    QString Dateout_path = AglDeviceP->GetPasswd("@Dateoutpix");
    QString Config_path = AglDeviceP->GetPasswd("@Upconfigpix");
    QString outConfig_path = AglDeviceP->GetPasswd("@OutConfig");
    //--------------------------------------------------------------------------------------------//
    QString Outexe_path = AglDeviceP->GetPasswd("@Outexe");
    //--------------------------------------------------------------------------------------------//

    QString UpApp = AglDeviceP->GetConfig("Update_Soft");
    QString Up_gujian = AglDeviceP->GetConfig("UPdate_gujian");
    QString Upcon = AglDeviceP->GetConfig("Upcon");
    QString Outcon = AglDeviceP->GetConfig("Outcon");
    QString Upsal = AglDeviceP->GetConfig("UpSql");
    QString Outsql= AglDeviceP->GetConfig("OutSql");
     //--------------------------------------------------------------------------------------------//
    QString OutApp = AglDeviceP->GetConfig("OutCSV");
     //--------------------------------------------------------------------------------------------//

    Pix_Update_Gujian.load(Update_path_gujian);
    Pix_Update.load(Update_path);
    Pix_Datein.load(Datein_path);
    Pix_Dateout.load(Dateout_path);
    Pix_Config.load(Config_path);
    Pix_outcon.load(outConfig_path);
    //--------------------------------------------------------------------------------------------//
    Pix_Outapp.load(Outexe_path);
    //--------------------------------------------------------------------------------------------//
    qFont.setPointSize(8);
    Mybutton *Up_update = new Mybutton(UpWidget);                                         // 软件更新
    Up_update->setGeometry(145, 90, 72, 72);
    QPixmap RePix_Update = Pix_Update.scaled(Up_update->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Up_update, RePix_Update);
    QLabel *Lab_update = new QLabel(UpWidget);
    Lab_update->setGeometry(115, 162, 132, 30);
    Lab_update->setText(UpApp);
    Lab_update->setFont(qFont);
    Lab_update->setAlignment(Qt::AlignCenter);
    Lab_update->setFont(qFont);

    Mybutton *Up_DateGu = new Mybutton(UpWidget);                                        // 固件更新
    Up_DateGu->setGeometry(291, 90, 72, 72);
    QPixmap RePix_Update_Gujian = Pix_Update_Gujian.scaled(Up_DateGu->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Up_DateGu, RePix_Update_Gujian);
    QLabel *Lab_DateGu = new QLabel(UpWidget);
    Lab_DateGu->setGeometry(261, 162, 132, 30);
    Lab_DateGu->setText(Up_gujian);
    Lab_DateGu->setFont(qFont);
    Lab_DateGu->setAlignment(Qt::AlignCenter);
    Lab_DateGu->setFont(qFont);

    Mybutton *Up_Datein = new Mybutton(UpWidget);                                        // 数据更新
    Up_Datein->setGeometry(437, 90, 72, 72);
    QPixmap RePix_Datein = Pix_Datein.scaled(Up_Datein->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Up_Datein, RePix_Datein);
    QLabel *Lab_Datein = new QLabel(UpWidget);
    Lab_Datein->setGeometry(407, 162, 132, 30);
    Lab_Datein->setText(Upsal);
    Lab_Datein->setFont(qFont);
    Lab_Datein->setAlignment(Qt::AlignCenter);
    Lab_Datein->setFont(qFont);

    Mybutton *Up_Config = new Mybutton(UpWidget);                                        // 配置更新
    Up_Config->setGeometry(583, 90, 72, 72);
    QPixmap RePix_Config = Pix_Config.scaled(Up_Config->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Up_Config, RePix_Config);
    QLabel *Lab_upcon = new QLabel(UpWidget);
    Lab_upcon->setGeometry(53, 162, 132, 30);
    Lab_upcon->setText(Upcon);
    Lab_upcon->setFont(qFont);
    Lab_upcon->setAlignment(Qt::AlignCenter);
    Lab_upcon->setFont(qFont);

    Out_app = new Mybutton(UpWidget);                                                                // CSV导出
    Out_app->setGeometry(145, 212, 72, 72);
    QPixmap RePix_Outapp = Pix_Outapp.scaled(Out_app->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Out_app, RePix_Outapp);
    Lab_outapp = new QLabel(UpWidget);
    Lab_outapp->setGeometry(85, 284, 192, 30);
    Lab_outapp->setText(OutApp);
    Lab_outapp->setFont(qFont);
    Lab_outapp->setAlignment(Qt::AlignCenter);
    Lab_outapp->setFont(qFont);

    Up_Dateout = new Mybutton(UpWidget);                                     // 数据导出
    Up_Dateout->setGeometry(364, 212, 72, 72);
    QPixmap RePix_Dateout = Pix_Dateout.scaled(Up_Dateout->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Up_Dateout, RePix_Dateout);
    Lab_datout = new QLabel(UpWidget);
    Lab_datout->setGeometry(304, 284, 192, 30);
    Lab_datout->setText(Outsql);
    Lab_datout->setFont(qFont);
    Lab_datout->setAlignment(Qt::AlignCenter);
    Lab_datout->setFont(qFont);

    Out_config = new Mybutton(UpWidget);                                     // 配置导出
    Out_config->setGeometry(583, 212, 72, 72);
    QPixmap RePix_outcon = Pix_outcon.scaled(Out_config->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Out_config, RePix_outcon);
    Lab_outcon= new QLabel(UpWidget);
    Lab_outcon->setGeometry(523, 284, 192, 30);
    Lab_outcon->setText(Outcon);
    Lab_outcon->setFont(qFont);
    Lab_outcon->setAlignment(Qt::AlignCenter);
    Lab_outcon->setFont(qFont);

    Time_Update =  new QTimer(this);

    UpdateBar = new QProgressBar(UpWidget);
    UpdateBar->setGeometry(90, 290, 620, 40);
    UpdateBar->setValue(0);
    UpdateBar->setAlignment(Qt :: AlignCenter);
    UpdateBar->hide();

    Label_Update = new QLabel(UpWidget);
    Label_Update->setGeometry(200, 225, 400, 50);
    Label_Update->setStyleSheet(QStringLiteral("color: rgb(217, 22, 38);"));
    Label_Update->setAlignment(Qt :: AlignCenter);
    Label_Update->hide();

    connect(Up_Ret, SIGNAL(clicked()), this, SLOT(pressUp_Ret()),Qt::UniqueConnection);
    connect(Up_Home, SIGNAL(clicked()), this, SLOT(pressUp_Home()),Qt::UniqueConnection);
    connect(Time_Update, SIGNAL(timeout()), this, SLOT(Time_Update_out()),Qt::UniqueConnection);
    connect(Up_DateGu, SIGNAL(clicked()), this, SLOT(pressUp_DateGu()),Qt::UniqueConnection);          // 固件更新
    connect(Up_update, SIGNAL(clicked()), this, SLOT(pressUp_update()),Qt::UniqueConnection);            // 软件更新
    connect(Up_Datein, SIGNAL(clicked()), this, SLOT(pressUp_Datein()),Qt::UniqueConnection);              // 数据更新
    connect(Up_Dateout, SIGNAL(clicked()), this, SLOT(pressUp_Dateout()),Qt::UniqueConnection);         // 数据导出
    connect(Up_Config, SIGNAL(clicked()), this, SLOT(pressUp_Config()),Qt::UniqueConnection);               // 配置更新
    connect(Out_config, SIGNAL(clicked()), this, SLOT(pressOut_config()),Qt::UniqueConnection);             // 配置导出
    connect(Out_app, SIGNAL(clicked()), this, SLOT(pressOut_app()),Qt::UniqueConnection);                     // CSV导出

    UpWidget->show();
}

QString PctGuiThread::ExportCSVFile()
{
    int iCount = Sql->CountResultSql();
    qDebug() << "We have " << iCount << " Datra !";
    QStringList Temp_List;
    QStringList List_Inter;
    QString Temp;
    QString CSVData;
    struct ResultIndex RES_LIST;
    QFile CSV_File("/yx-udisk/GPReader/TestResults.csv");
    if(CSV_File.exists())
    {
        AglDeviceP->SysCall("rm -rf /yx-udisk/GPReader/TestResults.csv");
    }
    if(!CSV_File.open(QFile::WriteOnly | QFile::Append))
    {
        qDebug() << "open file Error";
        return "error";
    }
    QTextStream Stream(&CSV_File);
    for(int  i = 1; i <= iCount; i++)
    {
        QCoreApplication::processEvents();
        if(!Sql->ListResultSql(RES_LIST, i))
        {
            qDebug() << "5013 List Result DB ERROR";
            CSV_File.close();
            return "error";
        }
        CSVData.clear();
//--------------------------------------Date-----------------------------------//
        Temp = RES_LIST.Date;
        Temp_List = Temp.split(" ");
        Temp = Temp_List.at(0);
        CSVData += "\""+ Temp + "\";";
//------------------------------Sample ID-----------------------------------//
        CSVData += "\"" + RES_LIST.SamId + "\";";
//----------------------------------PPI-----------------------------------------//
        Temp = RES_LIST.Porperty;
        Temp_List = Temp.split("#");
        Temp = Temp_List.at(2);
        CSVData += "\"" + Temp + "\";";
//-------------------------------WB/PL----------------------------------------//
        Temp = Temp_List.at(0);
        CSVData += "\"" + Temp + "\";";
//---------------------------HCT value---------------------------------------//
        Temp = Temp_List.at(1);
        CSVData += "\"" + Temp + "\";";
//---------------------------PG1 value---------------------------------------//
        Temp = Temp_List.at(3);
        CSVData += "\"" + Temp + "\";";
//---------------------------PG2 value---------------------------------------//
        Temp = Temp_List.at(4);
        CSVData += "\"" + Temp + "\";";
//---------------------------G17 value---------------------------------------//
        Temp = Temp_List.at(5);
        CSVData += "\"" + Temp + "\";";
//-----------------------------Hp value---------------------------------------//
        Temp = Temp_List.at(6);
        CSVData += "\"" + Temp + "\";";
//------------------------------INTERP----------------------------------------//
        Temp = Temp_List.at(9);
        List_Inter = Temp.split("+");
        if(List_Inter.length() == 1)
        {
            CSVData += "\"\";";
        }
        else
        {
            CSVData += "\"";
            for(int i = 0; i < List_Inter.length() - 1; i++)
            {
                CSVData += AglDeviceP->GetConfig(List_Inter.at(i));
            }
            CSVData += "\";";
        }
//------------------------------User ID---------------------------------------//
        Temp = Temp_List.at(7);
        CSVData += "\"" + Temp + "\";";
//-----------------------------Lot Number----------------------------------//
        Temp = Temp_List.at(8);
        CSVData += "\"" + Temp + "\"\r\n";
        Stream << CSVData;
        QCoreApplication::processEvents();
    }
    CSV_File.close();
    AglDeviceP->SysCall_process("sync");
    return "ok";
}

void PctGuiThread :: ConfigWidget_Home()
{
    Voice_pressKey();
    UpWidget->close();
    UsrWidget->close();
    COSWidget->close();
    SetWidget->close();
    KeyInput.clear();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    UiType = MainUi;
}

void PctGuiThread :: Time_Update_out()
{
    static int Conut_update = 0;
    Conut_update += 1;
    if(Conut_update <= 100)
    {
        UpdateBar->setValue(Conut_update);
    }
    else
    {
        Conut_update = 0;
        Time_Update->stop();
        UpdateBar->hide();
        Label_Update->hide();

        Out_app->show();
        Up_Dateout->show();
        Out_config->show();
        Lab_datout->show();
        Lab_outapp->show();
        Lab_outcon->show();

        if(Update_type == Update_APP)
        {
            if(isUpdateOK == "ok")
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Message_succ");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                isUpdate = false;
                isMessage = false;
                Voice_pressKey();
                AglDeviceP->ReBoot();
            }
            else
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("station_resErr");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                   Voice_pressKey();
                }
                isUpdate = false;
                isMessage = false;
            }
        }
        else if(Update_type == Update_Gujian)
        {
            if(AglDeviceP->McuUpDataP->State == "ok")
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Message_Gujian");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                isMessage = false;
               Voice_pressKey();
               AglDeviceP->GetVersion();
               Label_station->clear();
               QString banben = (QString)AglDeviceP->AglDev.Version;
               qDebug() << "Update is " << banben;
               isUpdate = false;
            }
            else if(AglDeviceP->McuUpDataP->State == "waiting")
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Message_Gujian_wait");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                isUpdate = false;
                isMessage = false;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
            }
            else
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Message_Gujian_ERR");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                isUpdate = false;
                isMessage = true;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
            }
        }
        else if(Update_type == Out_APP)
        {
            if(isUpdateOK == "ok")
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Station_CSVover");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                Voice_pressKey();
                Label_station->clear();
                isUpdate = false;
                isMessage = false;
            }
            else
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Station_CSVErr");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                Voice_pressKey();
                Label_station->clear();
                isUpdate = false;
                isMessage = false;
            }
        }
        else if(Update_type == Out_Config)
        {
            if(isUpdateOK == "ok")
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Station_conover");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                Voice_pressKey();
                Label_station->clear();
                isUpdate = false;
                isMessage = false;
            }
            else
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Station_conErr");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                Voice_pressKey();
                Label_station->clear();
                isUpdate = false;
                isMessage = false;
            }
        }
        else if(Update_type == Out_Database)
        {
            if(true == isDatebaseOK)
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Station_SqlOut");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                Voice_pressKey();
                Label_station->clear();
                isUpdate = false;
                isMessage = false;
            }
            else
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Station_SqlUpErr");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                Voice_pressKey();
                Label_station->clear();
                isUpdate = false;
                isMessage = false;
            }
        }
        else if(Update_type == IN_Database)
        {
            if(true == isDatebaseOK)
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Station_Sqlover");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                Voice_pressKey();
                Label_station->clear();
                AglDeviceP->ReBoot();
                isUpdate = false;
                isMessage = false;
            }
            else
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Station_SqlUpErr");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                Voice_pressKey();
                Label_station->clear();
                isUpdate = false;
                isMessage = false;
            }
        }
    }
}

void PctGuiThread :: pressUp_DateGu()               //  固件更新
{
    Voice_pressKey();
    if(isUpdate == true)
    {
        return;
    }
    Label_station->clear();
    if(!AglDeviceP->yx_udisk)
    {
        isMessage = true;
        msgBox = new QMessageBox(UpWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Measage_noudisk");
        msgBox->setText(loginerr);
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }

    Update_type = Update_Gujian;
    QString wait = AglDeviceP->GetConfig("station_wait");
    Label_Update->setText(wait);

    QString UpdateMSU = this->AglDeviceP->agl_updata_mcu();
    Delayms(50);
    if(UpdateMSU == "ok")
    {
        Out_app->hide();
        Up_Dateout->hide();
        Out_config->hide();
        Lab_datout->hide();
        Lab_outapp->hide();
        Lab_outcon->hide();

        UpdateBar->show();
        Label_Update->show();

        long time = (long)AglDeviceP->McuUpDataP->MaxNum*500+8000;
        Time_Update->setInterval(time/100); // 单位毫秒
        Time_Update->start();
        isUpdate = true;
        QString Outapp = AglDeviceP->GetConfig("Wait_Gujian");
        Label_station->setText(Outapp);
    }
    else
    {
        isMessage = true;
        msgBox = new QMessageBox(UpWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_Gujian_ERR");
        msgBox->setText(loginerr);
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        qDebug() << "Update MCU Error,Because " << UpdateMSU;
        return;
    }
}

void PctGuiThread :: pressUp_update()               // 软件更新
{
    Voice_pressKey();
    if(isUpdate == true)
    {
        return;
    }
    Label_station->clear();
    if(!AglDeviceP->yx_udisk)
    {
        isMessage = true;
        msgBox = new QMessageBox(UpWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Measage_noudisk");
        msgBox->setText(loginerr);
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }

    QString wait = AglDeviceP->GetConfig("station_wait");
    Label_Update->setText(wait);

    Out_app->hide();
    Up_Dateout->hide();
    Out_config->hide();
    Lab_datout->hide();
    Lab_outapp->hide();
    Lab_outcon->hide();

    UpdateBar->show();
    Label_Update->show();

    Update_type = Update_APP;
    Time_Update->start();
    Time_Update->setInterval(60);

    isUpdate = true;
    QString stawait = AglDeviceP->GetConfig("Wait_SoftOut");
    Label_station->setText(stawait);
    isUpdateOK =  this->AglDeviceP->UpDataApp();
    qDebug() << "update software is over,it's " << isUpdateOK;
}

void PctGuiThread :: pressOut_app()   // 软件导出
{
    Voice_pressKey();
    if(isUpdate == true)
    {
        return;
    }
    Label_station->clear();
    if(!AglDeviceP->yx_udisk)
    {
        isMessage = true;
        msgBox = new QMessageBox(UpWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Measage_noudisk");
        msgBox->setText(loginerr);
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        return;
    }
    //-------------------------------------Add------------------------------------//
        QDir OutDir("/yx-udisk/GPReader");
        if(!OutDir.exists())
        {
            qDebug() << "the dir io not found";
            if(!OutDir.mkdir("/yx-udisk/GPReader"))
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Measage_DirErr");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                isMessage = false;
                return;
            }
        }
    //-------------------------------------------------------------------------------//
    QString wait = AglDeviceP->GetConfig("station_waitout");
    Label_Update->setText(wait);

    Out_app->hide();
    Up_Dateout->hide();
    Out_config->hide();
    Lab_datout->hide();
    Lab_outapp->hide();
    Lab_outcon->hide();

    UpdateBar->show();
    Label_Update->show();

    Update_type = Out_APP;
    int iCount = Sql->CountResultSql();
    int Time = 0.05 * iCount;
    Time_Update->setInterval((Time * 1000 + 3000) / 100);           // 每隔多少时间刷新一次时间，总时间除以100就是进度条每走一次的时间
    Time_Update->start();

    isUpdate = true;
    QString stawait = AglDeviceP->GetConfig("Station_CSV");
    Label_station->setText(stawait);
//    isUpdateOK =  this->AglDeviceP->OutDataApp();
    isUpdateOK =  ExportCSVFile();
    qDebug() << "update software is over,it's " << isUpdateOK;
}

void PctGuiThread :: pressOut_config()       // 配置导出
{
    Voice_pressKey();
    if(isUpdate == true)
    {
        return;
    }
    Label_station->clear();
    if(!AglDeviceP->yx_udisk)
    {
        isMessage = true;
        msgBox = new QMessageBox(UpWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Measage_noudisk");
        msgBox->setText(loginerr);
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
//-------------------------------------Add------------------------------------//
    QDir OutDir("/yx-udisk/GPReader");
    if(!OutDir.exists())
    {
        qDebug() << "the dir io not found";
        if(!OutDir.mkdir("/yx-udisk/GPReader"))
        {
            isMessage = true;
            msgBox = new QMessageBox(UpWidget);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            QString loginerr = AglDeviceP->GetConfig("Measage_DirErr");
            msgBox->setText(loginerr);
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
    }
//-------------------------------------------------------------------------------//
    QString wait = AglDeviceP->GetConfig("station_waitout");
    Label_Update->setText(wait);

    Out_app->hide();
    Up_Dateout->hide();
    Out_config->hide();
    Lab_datout->hide();
    Lab_outapp->hide();
    Lab_outcon->hide();

    UpdateBar->show();
    Label_Update->show();

    Update_type = Out_Config;
    Time_Update->setInterval(60);
    Time_Update->start();

    isUpdate = true;
    QString stawait = AglDeviceP->GetConfig("Station_ConOut");
    Label_station->setText(stawait);
    isUpdateOK =  this->AglDeviceP->OutDataCfg();
    qDebug() << "update software is over,it's " << isUpdateOK;
}

void PctGuiThread :: pressUp_Datein()   // 数据更新
{
    Voice_pressKey();
    if(isUpdate == true)
    {
        return;
    }
    Label_station->clear();
    if(!AglDeviceP->yx_udisk)
    {
        isMessage = true;
        msgBox = new QMessageBox(UpWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Measage_noudisk"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        return;
    }

    QString wait = AglDeviceP->GetConfig("station_wait");
    Label_Update->setText(wait);

    Out_app->hide();
    Up_Dateout->hide();
    Out_config->hide();
    Lab_datout->hide();
    Lab_outapp->hide();
    Lab_outcon->hide();

    UpdateBar->show();
    Label_Update->show();

    QString stawait = AglDeviceP->GetConfig("Station_InSql");
    Label_station->setText(stawait);

    Time_Update->setInterval(300);
    Time_Update->start();
    isUpdate = true;

    isDatebaseOK = AglDeviceP->File1ToFile2_Shell("IN");
    Update_type = IN_Database;
}

void PctGuiThread :: pressUp_Dateout()     // 数据导出
{
    Voice_pressKey();
    if(isUpdate == true)
    {
        return;
    }
    Label_station->clear();
    if(!AglDeviceP->yx_udisk)
    {
        isMessage = true;
        msgBox = new QMessageBox(UpWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Measage_noudisk"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    //-------------------------------------Add------------------------------------//
        QDir OutDir("/yx-udisk/GPReader");
        if(!OutDir.exists())
        {
            qDebug() << "the dir io not found";
            if(!OutDir.mkdir("/yx-udisk/GPReader"))
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Measage_DirErr");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                isMessage = false;
                return;
            }
        }
        OutDir.setPath("/yx-udisk/GPReader/Database");
        if(!OutDir.exists())
        {
            qDebug() << "the dir io not found";
            if(!OutDir.mkdir("/yx-udisk/GPReader/Database"))
            {
                isMessage = true;
                msgBox = new QMessageBox(UpWidget);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                QString loginerr = AglDeviceP->GetConfig("Measage_DirErr");
                msgBox->setText(loginerr);
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                isMessage = false;
                return;
            }
        }
    //-------------------------------------------------------------------------------//
    QString wait = AglDeviceP->GetConfig("station_waitout");
    Label_Update->setText(wait);

    Out_app->hide();
    Up_Dateout->hide();
    Out_config->hide();
    Lab_datout->hide();
    Lab_outapp->hide();
    Lab_outcon->hide();

    UpdateBar->show();
    Label_Update->show();
    QString stawait = AglDeviceP->GetConfig("Station_outSql");
    Label_station->setText(stawait);

    isUpdate = true;
    Update_type = Out_Database;
    int SqlConut = Sql->CountResultSql();
    int Bartime = (SqlConut / 20) + (SqlConut % 20);
    Time_Update->setInterval(Bartime);
    Time_Update->start();
//    QEventLoop loop;
    isDatebaseOK = AglDeviceP->File1ToFile2_Shell("OUT");
//    loop.exec();
}

void PctGuiThread :: pressUp_Config()   // 配置更新
{
    Voice_pressKey();
    if(isUpdate == true)                // 如果当前正在进行文件操作，忽略再一次的文件操作请求
    {
        return;
    }
    Label_station->clear();
    if(!AglDeviceP->yx_udisk)
    {
        isMessage = true;
        msgBox = new QMessageBox(UpWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Measage_noudisk"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        return;
    }

    QString upcon = AglDeviceP->GetConfig("Station_Upcon");
    isUpdate = true;
    QString UpConfig = AglDeviceP->UpDataCfg();
    qDebug() << "upconfig is over is " << UpConfig;
    if(UpConfig == "ok")
    {
        isMessage = true;
        msgBox = new QMessageBox(UpWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(upcon);
        msgBox->setIcon(QMessageBox :: Information);
        msgBox->exec();
        delete msgBox;
        Voice_pressKey();
        AglDeviceP->ReBoot();
        isUpdate = false;
        isMessage = false;
    }
    else
    {
        isMessage = true;
        msgBox = new QMessageBox(UpWidget);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Station_UpconErr"));
        msgBox->setIcon(QMessageBox :: Information);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isUpdate = false;
        isMessage = false;
    }
}

void PctGuiThread :: pressUp_Ret()
{
    Voice_pressKey();
    if(isUpdate == true)
    {
        return;
    }
    Time_Update->stop();
    disconnect(Time_Update, SIGNAL(timeout()), this, SLOT(Time_Update_out()));
    delete Time_Update;
    UpWidget->close();
    KeyInput.clear();
    UiType = Usrui;
}

void PctGuiThread :: pressUp_Home()
{
    Voice_pressKey();
    if(isUpdate == true)
    {
        return;
    }
    Time_Update->stop();
    disconnect(Time_Update, SIGNAL(timeout()), this, SLOT(Time_Update_out()));
    delete Time_Update;
    UpWidget->close();
    UsrWidget->close();
    COSWidget->close();
    SetWidget->close();
    KeyInput.clear();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    UiType = MainUi;
}

void PctGuiThread :: CreatUSERWidget()                                  // 账户管理
{
    UiType = SetUserui;
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));

    USERWidget = new QWidget(MainWindowP);
    USERWidget->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    USERWidget->setGeometry(0, 75, 803, 348);
    QFont qFont;
    qFont.setPointSize(16);
    QLabel *TitelLabel = new QLabel(USERWidget);
    TitelLabel->setGeometry(250, 30, 300, 40);
    TitelLabel->setPalette(palette);
    TitelLabel->setAlignment(Qt::AlignCenter);
    QString Titel = AglDeviceP->GetConfig("Usertitel");
    TitelLabel->setText(Titel);
    TitelLabel->setFont(qFont);

    QStringList PassList = AglDeviceP->GetAllUserData();
    iCount_User = PassList.length();
//    Table_Use = new QTableWidget(iCount_User, 2 ,USERWidget);
    Table_Use = new MyTableWidget(iCount_User-1, 2 ,USERWidget);
    if(iCount_User > 5)
    {
        Table_Use->setGeometry(160, 91, 512, 202);
    }
    else
    {
        Table_Use->setGeometry(160, 91, 482, 202);
    }

    Table_Use->setSelectionBehavior(QAbstractItemView :: SelectRows);          //  单元格为一次选中一行模式
    Table_Use->setSelectionMode(QAbstractItemView :: SingleSelection);         //  设置只能被选中一个单元格
    Table_Use->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    QHeaderView *head_ver = Table_Use->verticalHeader();
    head_ver->setHidden(true);                            // 去掉行号
    QHeaderView *head_hor = Table_Use->horizontalHeader();
    connect(head_hor, SIGNAL(sectionClicked(int)), this, SLOT(ClickedHeardview()),Qt::UniqueConnection);
    head_hor->setFixedHeight(40);
//    head_hor->setHidden(true);                           //  去掉列号

    for(int i = 0; i < 2; i++)
    {
        Table_Use->setColumnWidth(i, 240);          // 设置第i列单元格长度
    }
    for(int j = 0; j < iCount_User; j++)
    {
        Table_Use->setRowHeight(j, 40);
    }

    QScrollBar *Table_Res_bar =  Table_Use->verticalScrollBar();                                                                                      // 获得该表格的滚动条
    MyScrollBar *Table_bar = new MyScrollBar(Table_Use);
    Table_bar->move(482, 5);
    Table_bar = (MyScrollBar *)Table_Res_bar;
    Table_bar->setStyleSheet("QScrollBar:vertical {""width: 30px; background-color: rgb(215,215,215);""}");     // 改变滚动条宽度，颜色
    Table_Use->setHorizontalScrollBarPolicy(Qt :: ScrollBarAlwaysOff);

    User_New[0] = AglDeviceP->GetConfig("User_name");
    Pass_New[0] = AglDeviceP->GetConfig("User_pass");

    QTableWidgetItem *Item_user= new QTableWidgetItem(User_New[0]);
    QTableWidgetItem *Item_pass= new QTableWidgetItem(Pass_New[0]);

    Table_Use->setHorizontalHeaderItem(0,Item_user);
    Table_Use->setHorizontalHeaderItem(1,Item_pass);

    QString USR[iCount_User];
    QStringList USr_list[iCount_User];

    OldUserData.clear();
    for(int i = 0;i < iCount_User-1; i++)
    {
        USR[i] = PassList.at(i);
        USr_list[i] = USR[i].split("=");
        User_New[i+1] = USr_list[i].at(0);
        Pass_New[i+1] = USr_list[i].at(1);
        OldUserData += User_New[i+1] + "," + Pass_New[i+1];
    }

//    for(int i = 0; i < iCount_User; i++)
//    {
//        Item_User[i] = new QTableWidgetItem(User_New[i]);
//        Item_User[i]->setTextAlignment(Qt::AlignCenter);
//        Item_Pass[i] = new QTableWidgetItem(Pass_New[i]);
//        Item_Pass[i]->setTextAlignment(Qt::AlignCenter);
//        Table_Use->setItem(i, 0, Item_User[i]);
//        Table_Use->setItem(i ,1, Item_Pass[i]);
//    }

    for(int i = 0; i < iCount_User-1; i++)
    {
        Item_User[i] = new QTableWidgetItem(User_New[i+1]);
        Item_User[i]->setTextAlignment(Qt::AlignCenter);
        Item_Pass[i] = new QTableWidgetItem(Pass_New[i+1]);
        Item_Pass[i]->setTextAlignment(Qt::AlignCenter);
        Table_Use->setItem(i, 0, Item_User[i]);
        Table_Use->setItem(i ,1, Item_Pass[i]);
    }

//    for(int i = 0; i < 2; i++)
//    {
//        Table_Use->item(0, i)->setFlags(Qt :: ItemIsEnabled);
//    }

    qFont.setPointSize(12);

    Mybutton *But_Add = new Mybutton(USERWidget);
    Mybutton *But_Del = new Mybutton(USERWidget);
    Mybutton *But_Set = new Mybutton(USERWidget);
    Mybutton *Butt_Ret = new Mybutton(USERWidget);
    Mybutton *Butt_Home = new Mybutton(USERWidget);
    Mybutton *UsrBut_save = new Mybutton(USERWidget);

    UsrBut_save->setGeometry(743, 11, 48, 48);
    QPixmap ima_save;
    QString saveload = AglDeviceP->GetPasswd("@MainSet_Save");
    ima_save.load(saveload);
    MainSetpix_Save = ima_save.scaled(UsrBut_save->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(UsrBut_save, MainSetpix_Save);

    Butt_Home->setGeometry(13, 11, 48, 48);
    Button_SetPix(Butt_Home, MainSetpix_Home);

    But_Add->setGeometry(741, 172, 48, 48);
    QPixmap ima_add;
    QString taddload = AglDeviceP->GetPasswd("@Adduser_pix");
    ima_add.load(taddload);
    Adduserpix = ima_add.scaled(But_Add->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(But_Add, Adduserpix);

    But_Del->setGeometry(741, 231, 48, 48);
    QPixmap ima_del;
    QString delload = AglDeviceP->GetPasswd("@Deluser_pix");
    ima_del.load(delload);
    Deluserpix = ima_del.scaled(But_Del->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(But_Del, Deluserpix);

    But_Set->setGeometry(741, 290, 48, 48);
    QPixmap ima_change;
    QString changeload = AglDeviceP->GetPasswd("@Passchge_pix");
    ima_change.load(changeload);
    Passchgepix = ima_change.scaled(But_Set->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(But_Set, Passchgepix);

    Butt_Ret->setGeometry(11, 290, 48, 48);
    Button_SetPix(Butt_Ret, MainSetpix_3);

    connect(UsrBut_save, SIGNAL(clicked()), this, SLOT(pressUsrBut_save()),Qt::UniqueConnection);
    connect(Butt_Ret, SIGNAL(clicked()), this, SLOT(pressButt_Ret()),Qt::UniqueConnection);
    connect(Butt_Home, SIGNAL(clicked()), this, SLOT(pressButt_Home()),Qt::UniqueConnection);
    connect(But_Add, SIGNAL(clicked()), this, SLOT(pressBut_Add()),Qt::UniqueConnection);
    connect(But_Set, SIGNAL(clicked()), this, SLOT(pressBut_Set()),Qt::UniqueConnection);
    connect(But_Del, SIGNAL(clicked()), this, SLOT(pressBut_Del()),Qt::UniqueConnection);
    connect(Table_Use,SIGNAL(itemClicked(QTableWidgetItem*)),this,SLOT(ClickedItem_Use()),Qt::UniqueConnection);

    USERWidget->show();
}
//------Add------liuyouwei--------------------------------------------------------------------------------------------------//
void PctGuiThread :: pressUsrBut_save()
{
    Voice_pressKey();
    QString USRmessage;
    int iRow = Table_Use->rowCount();
//    for(int i = 1; i < iRow; i++)
    for(int i = 0; i < iRow; i++)
    {
        USRmessage += Table_Use->item(i, 0)->text() + "=" + Table_Use->item(i, 1)->text() + "#\r\n";
    }
    QString Other = AglDeviceP->GetAllParadata();
    USRmessage += Other;

    if(!AglDeviceP->ReloadPasswdFile(USRmessage))
    {
        qDebug() << "5026 Reload password file Error!";
    }
    if(!AglDeviceP->DeleteFile("password.txt"))
    {
        qDebug() << "Delete file error!";
    }
    if(!AglDeviceP->WriteTxtToFile("password.txt", USRmessage))
    {
        qDebug() << "Creat file error!";
    }

    isMessage = true;
    msgBox = new QMessageBox(MainWindowP);
    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
    msgBox->setText(AglDeviceP->GetConfig("Message_succsave"));
    msgBox->setIcon(QMessageBox :: Information);
    msgBox->exec();
    delete msgBox;
    if(isWidget == false)
    {
        Voice_pressKey();
    }
    isSave = true;
    isMessage = false;
}
//------Add End---------------------------------------------------------------------------------------//
void PctGuiThread :: CreatUserKey()                            //    账户管理带字母键盘
{
    KeyStation = 0;
    UserKeyWidget = new QWidget(USERWidget);
    UserKeyWidget->setGeometry(140, 28, 510, 293);
    UserKeyWidget->setStyleSheet(QStringLiteral("background-color: rgb(123, 139, 202);"));
    for(int i = 0; i < 26; i++)
    {
        SelectKey[i] = 'a' + i;
        pushButton[i] = new Mybutton(UserKeyWidget);
        pushButton[i]->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
        pushButton[i]->setText(QString(SelectKey[i]));
    }
    pushButton[0]->setGeometry(QRect(36, 175, 40, 30));
    pushButton[1]->setGeometry(QRect(260, 215, 40, 30));
    pushButton[2]->setGeometry(QRect(160, 215, 40, 30));
    pushButton[3]->setGeometry(QRect(136, 175, 40, 30));
    pushButton[4]->setGeometry(QRect(110, 135, 40, 30));
    pushButton[5]->setGeometry(QRect(186, 175, 40, 30));
    pushButton[6]->setGeometry(QRect(236, 175, 40, 30));
    pushButton[7]->setGeometry(QRect(286, 175, 40, 30));
    pushButton[8]->setGeometry(QRect(360, 135, 40, 30));
    pushButton[9]->setGeometry(QRect(336, 175, 40, 30));
    pushButton[10]->setGeometry(QRect(386, 175, 40, 30));
    pushButton[11]->setGeometry(QRect(436, 175, 40, 30));
    pushButton[12]->setGeometry(QRect(360, 215, 40, 30));
    pushButton[13]->setGeometry(QRect(310, 215, 40, 30));
    pushButton[14]->setGeometry(QRect(410, 135, 40, 30));
    pushButton[15]->setGeometry(QRect(460, 135, 40, 30));
    pushButton[16]->setGeometry(QRect(10, 135, 40, 30));
    pushButton[17]->setGeometry(QRect(160, 135, 40, 30));
    pushButton[18]->setGeometry(QRect(86, 175, 40, 30));
    pushButton[19]->setGeometry(QRect(210, 135, 40, 30));
    pushButton[20]->setGeometry(QRect(310, 135, 40, 30));
    pushButton[21]->setGeometry(QRect(210, 215, 40, 30));
    pushButton[22]->setGeometry(QRect(60, 135, 40, 30));
    pushButton[23]->setGeometry(QRect(110, 215, 40, 30));
    pushButton[24]->setGeometry(QRect(260, 135, 40, 30));
    pushButton[25]->setGeometry(QRect(60, 215, 40, 30));

    pushButton_Up = new Mybutton(UserKeyWidget);
    pushButton_Up->setGeometry(QRect(10, 215, 40, 30));
    QString Up0 = AglDeviceP->GetPasswd("@pix_up_0");
    QPixmap pix_up(Up0);
    pushButton_Up->setIcon(pix_up);
    pushButton_Up->setIconSize(QSize(40, 25));
    pushButton_Up->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_0 = new Mybutton(UserKeyWidget);
    pushButton_0->setGeometry(QRect(410, 215, 40, 30));
    pushButton_0->setText(tr("0"));
    pushButton_0->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_dot = new Mybutton(UserKeyWidget);
    pushButton_dot->setGeometry(QRect(460, 215, 40, 30));
    pushButton_dot->setText(tr("."));
    pushButton_dot->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_Space = new Mybutton(UserKeyWidget);
    pushButton_Space->setGeometry(QRect(100, 255, 260, 30));
    pushButton_Space->setText(" ");
    pushButton_Space->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_del = new Mybutton(UserKeyWidget);
    pushButton_del->setGeometry(QRect(10, 255, 80, 30));
    pushButton_del->setText("Delete");
    pushButton_del->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_sure = new Mybutton(UserKeyWidget);
    pushButton_sure->setGeometry(QRect(420, 255, 80, 30));
    pushButton_sure->setText(tr("OK"));
    pushButton_sure->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_star = new Mybutton(UserKeyWidget);
    pushButton_star->setGeometry(QRect(370, 255, 40, 30));
    pushButton_star->setText(tr("*"));
    pushButton_star->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_7 = new Mybutton(UserKeyWidget);
    pushButton_7->setGeometry(QRect(336, 95, 40, 30));
    pushButton_7->setText(tr("7"));
    pushButton_7->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_2 = new Mybutton(UserKeyWidget);
    pushButton_2->setGeometry(QRect(86, 95, 40, 30));
    pushButton_2->setText(tr("2"));
    pushButton_2->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_9 = new Mybutton(UserKeyWidget);
    pushButton_9->setGeometry(QRect(436, 95, 40, 30));
    pushButton_9->setText(tr("9"));
    pushButton_9->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_3 = new Mybutton(UserKeyWidget);
    pushButton_3->setGeometry(QRect(136, 95, 40, 30));
    pushButton_3->setText(tr("3"));
    pushButton_3->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_4 = new Mybutton(UserKeyWidget);
    pushButton_4->setGeometry(QRect(186, 95, 40, 30));
    pushButton_4->setText(tr("4"));
    pushButton_4->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_8 = new Mybutton(UserKeyWidget);
    pushButton_8->setGeometry(QRect(386, 95, 40, 30));
    pushButton_8->setText(tr("8"));
    pushButton_8->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_5 = new Mybutton(UserKeyWidget);
    pushButton_5->setGeometry(QRect(236, 95, 40, 30));
    pushButton_5->setText(tr("5"));
    pushButton_5->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_1 = new Mybutton(UserKeyWidget);
    pushButton_1->setGeometry(QRect(36, 95, 40, 30));
    pushButton_1->setText(tr("1"));
    pushButton_1->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_6 = new Mybutton(UserKeyWidget);
    pushButton_6->setGeometry(QRect(286, 95, 40, 30));
    pushButton_6->setText(tr("6"));
    pushButton_6->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    UserEdit = new myLineEdit(UserKeyWidget);
    UserEdit->setGeometry(QRect(10, 42, 396, 40));
    UserEdit->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));

    QFont Font_lab;
    Font_lab.setPointSize(14);
    Label_ADD = new QLabel(UserKeyWidget);
    Label_ADD->setObjectName(QStringLiteral("label"));
    Label_ADD->setGeometry(QRect(10, 8, 489, 35));
    Label_ADD->setAlignment(Qt :: AlignLeft);
    Label_ADD->setFont(Font_lab);
    QString usna = AglDeviceP->GetConfig("Label_user1");
    Label_ADD->setText(usna);

    connect(pushButton_del, SIGNAL(clicked()), this, SLOT(presspushButton_del()),Qt::UniqueConnection);
    connect(pushButton_Up, SIGNAL(clicked()), this, SLOT(pressKeyButton_up()),Qt::UniqueConnection);
    connect(pushButton_star, SIGNAL(clicked()), this, SLOT(pressKeyButton_star()),Qt::UniqueConnection);
    connect(pushButton_dot, SIGNAL(clicked()), this, SLOT(pressKeyButton_dot()),Qt::UniqueConnection);
    connect(pushButton_Space, SIGNAL(clicked()), this, SLOT(pressKeyButton_space()),Qt::UniqueConnection);
    connect(pushButton_sure, SIGNAL(clicked()), this, SLOT(presspushButton_sure()),Qt::UniqueConnection);

    connect(pushButton_0, SIGNAL(clicked()), this, SLOT(pressKeyButton_0()),Qt::UniqueConnection);
    connect(pushButton_1, SIGNAL(clicked()), this, SLOT(pressKeyButton_1()),Qt::UniqueConnection);
    connect(pushButton_2, SIGNAL(clicked()), this, SLOT(pressKeyButton_2()),Qt::UniqueConnection);
    connect(pushButton_3, SIGNAL(clicked()), this, SLOT(pressKeyButton_3()),Qt::UniqueConnection);
    connect(pushButton_4, SIGNAL(clicked()), this, SLOT(pressKeyButton_4()),Qt::UniqueConnection);
    connect(pushButton_5, SIGNAL(clicked()), this, SLOT(pressKeyButton_5()),Qt::UniqueConnection);
    connect(pushButton_6, SIGNAL(clicked()), this, SLOT(pressKeyButton_6()),Qt::UniqueConnection);
    connect(pushButton_7, SIGNAL(clicked()), this, SLOT(pressKeyButton_7()),Qt::UniqueConnection);
    connect(pushButton_8, SIGNAL(clicked()), this, SLOT(pressKeyButton_8()),Qt::UniqueConnection);
    connect(pushButton_9, SIGNAL(clicked()), this, SLOT(pressKeyButton_9()),Qt::UniqueConnection);
    connect(pushButton[0], SIGNAL(clicked()), this, SLOT(pressKeyButton_a()),Qt::UniqueConnection);
    connect(pushButton[1], SIGNAL(clicked()), this, SLOT(pressKeyButton_b()),Qt::UniqueConnection);
    connect(pushButton[2], SIGNAL(clicked()), this, SLOT(pressKeyButton_c()),Qt::UniqueConnection);
    connect(pushButton[3], SIGNAL(clicked()), this, SLOT(pressKeyButton_d()),Qt::UniqueConnection);
    connect(pushButton[4], SIGNAL(clicked()), this, SLOT(pressKeyButton_e()),Qt::UniqueConnection);
    connect(pushButton[5], SIGNAL(clicked()), this, SLOT(pressKeyButton_f()),Qt::UniqueConnection);
    connect(pushButton[6], SIGNAL(clicked()), this, SLOT(pressKeyButton_g()),Qt::UniqueConnection);
    connect(pushButton[7], SIGNAL(clicked()), this, SLOT(pressKeyButton_h()),Qt::UniqueConnection);
    connect(pushButton[8], SIGNAL(clicked()), this, SLOT(pressKeyButton_i()),Qt::UniqueConnection);
    connect(pushButton[9], SIGNAL(clicked()), this, SLOT(pressKeyButton_j()),Qt::UniqueConnection);
    connect(pushButton[10], SIGNAL(clicked()), this, SLOT(pressKeyButton_k()),Qt::UniqueConnection);
    connect(pushButton[11], SIGNAL(clicked()), this, SLOT(pressKeyButton_l()),Qt::UniqueConnection);
    connect(pushButton[12], SIGNAL(clicked()), this, SLOT(pressKeyButton_m()),Qt::UniqueConnection);
    connect(pushButton[13], SIGNAL(clicked()), this, SLOT(pressKeyButton_n()),Qt::UniqueConnection);
    connect(pushButton[14], SIGNAL(clicked()), this, SLOT(pressKeyButton_o()),Qt::UniqueConnection);
    connect(pushButton[15], SIGNAL(clicked()), this, SLOT(pressKeyButton_p()),Qt::UniqueConnection);
    connect(pushButton[16], SIGNAL(clicked()), this, SLOT(pressKeyButton_q()),Qt::UniqueConnection);
    connect(pushButton[17], SIGNAL(clicked()), this, SLOT(pressKeyButton_r()),Qt::UniqueConnection);
    connect(pushButton[18], SIGNAL(clicked()), this, SLOT(pressKeyButton_s()),Qt::UniqueConnection);
    connect(pushButton[19], SIGNAL(clicked()), this, SLOT(pressKeyButton_t()),Qt::UniqueConnection);
    connect(pushButton[20], SIGNAL(clicked()), this, SLOT(pressKeyButton_u()),Qt::UniqueConnection);
    connect(pushButton[21], SIGNAL(clicked()), this, SLOT(pressKeyButton_v()),Qt::UniqueConnection);
    connect(pushButton[22], SIGNAL(clicked()), this, SLOT(pressKeyButton_w()),Qt::UniqueConnection);
    connect(pushButton[23], SIGNAL(clicked()), this, SLOT(pressKeyButton_x()),Qt::UniqueConnection);
    connect(pushButton[24], SIGNAL(clicked()), this, SLOT(pressKeyButton_y()),Qt::UniqueConnection);
    connect(pushButton[25], SIGNAL(clicked()), this, SLOT(pressKeyButton_z()),Qt::UniqueConnection);

    UserKeyWidget->show();
}

void PctGuiThread :: CreatSelecWidget()                     //    查询界面主界面
{
    UiType = Selectui;
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));

    SelecWidget = new QWidget(mainWind);
    SelecWidget->setAutoFillBackground(true);
    SelecWidget->setGeometry(0, 75, 805, 348);
    SelecWidget->setPalette(palette);

    Mybutton *Button_2 = new Mybutton(SelecWidget);
    Mybutton *Button_3 = new Mybutton(SelecWidget);
    Mybutton *Button_1 = new Mybutton(SelecWidget);
    Mybutton *Button_home = new Mybutton(SelecWidget);
    Button_1->setGeometry(186, 88, 128, 128);
    Button_2->setGeometry(485, 88, 128, 128);
    Button_3->setGeometry(11, 290, 48, 48);
    Button_home->setGeometry(13, 11, 48, 48);

    QPixmap ima_home;
    QString homeload = AglDeviceP->GetPasswd("@MainSet_Home");
    ima_home.load(homeload);
    MainSetpix_Home = ima_home.scaled(Button_home->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);

    QPixmap ima_left;
    QString leftload = AglDeviceP->GetPasswd("@MainSet_3");
    ima_left.load(leftload);
    MainSetpix_3 = ima_left.scaled(Button_3->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);

    Button_SetPix(Button_home, MainSetpix_Home);
    Button_SetPix(Button_3, MainSetpix_3);

    QPixmap ima_qc;
    QString qcload = AglDeviceP->GetPasswd("@QC_pix");
    ima_qc.load(qcload);
    QCpix = ima_qc.scaled(Button_2->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_2, QCpix);

    QPixmap ima_ews;
    QString resload = AglDeviceP->GetPasswd("@Report_pix");
    ima_ews.load(resload);
    Selectpix_1 = ima_ews.scaled(Button_1->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Button_1, Selectpix_1);

    QLabel *Label_button1 = new QLabel(SelecWidget);
    QLabel *Label_button2 = new QLabel(SelecWidget);

    Label_button2->setGeometry(449, 216, 200, 70);
    Label_button1->setGeometry(150, 216, 200, 70);

    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));
    //--------------------Add--------------------------//
    if(isAdmin == false)                                 //  如果当前用户不是root  用户管理按钮对其隐藏
    {
        Button_2->setEnabled(false);
        Label_button2->setStyleSheet(QStringLiteral("color: rgb(175,175,175);"));
    }
    //----------------------------------------------------//

    QString patient = AglDeviceP->GetConfig("Selec_But1");
    QString patient1 = AglDeviceP->GetConfig("Selec_But1_1");
    Label_button1->setAlignment(Qt::AlignCenter);
    Label_button1->setAutoFillBackground(true);
    Label_button1->setPalette(palette);
    Label_button1->setText(patient + "\r\n" + patient1);

    QString selebut = AglDeviceP->GetConfig("Selec_But2");
    QString selebut1 = AglDeviceP->GetConfig("Selec_But2_1");

    Label_button2->setAlignment(Qt::AlignCenter);
    Label_button2->setAutoFillBackground(true);
    Label_button2->setPalette(palette);
    Label_button2->setText(selebut + "\r\n" + selebut1);

    connect(Button_1, SIGNAL(clicked()), this, SLOT(pressButton_1()),Qt::UniqueConnection);
    connect(Button_2, SIGNAL(clicked()), this, SLOT(pressButton_2()),Qt::UniqueConnection);          // QC
    connect(Button_3, SIGNAL(clicked()), this, SLOT(pressButton_3()),Qt::UniqueConnection);
    connect(Button_home, SIGNAL(clicked()), this, SLOT(pressButton_3()),Qt::UniqueConnection);
    SelecWidget->show();
    qDebug() << "Results main widget uitype is " << UiType;
 }

void PctGuiThread::CreatQCWidget()                          //    QC界面
{
    QPalette palette;
    QFont qFont;
    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(76, 124, 216));

    Group_Infor = new QWidget(SelecWidget);
    Group_Infor->setStyle(QStyleFactory::create("windows"));
    Group_Infor->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Group_Infor->setGeometry(-2, -2, 803, 351);

    qFont.setPointSize(16);
    QLabel *TitelLabel = new QLabel(Group_Infor);
    TitelLabel->setGeometry(250, 10, 300, 40);
    TitelLabel->setAlignment(Qt::AlignCenter);
    QString inque = AglDeviceP->GetConfig("infor_zhi");
    TitelLabel->setText(inque);
    TitelLabel->setFont(qFont);

    Mybutton *But_Inf_ret = new Mybutton(Group_Infor);
    Mybutton *But_Inf_home = new Mybutton(Group_Infor);
    But_Inf_ret->setGeometry(13, 292, 48, 48);
    But_Inf_home->setGeometry(15, 13, 48, 48);
    Button_SetPix(But_Inf_ret, MainSetpix_3);
    Button_SetPix(But_Inf_home, MainSetpix_Home);

    connect(But_Inf_ret, SIGNAL(clicked()), this, SLOT(pressBut_Inf_ret()),Qt::UniqueConnection);
    connect(But_Inf_home, SIGNAL(clicked()), this, SLOT(pressButton_3()),Qt::UniqueConnection);

    Group_Infor->show();
}

void PctGuiThread::CreatPatientWidget()         // 将结果以图表形式展示界面    查询结果展示
{
    UiType = Selectui;
    QPalette palette;
    QFont qFont;
    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(76, 124, 216));

    Group_Res = new QWidget(mainWind);
    Group_Res->setStyle(QStyleFactory::create("windows"));
    Group_Res->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Group_Res->setGeometry(0, 75, 805, 348);

//    SelecWidget = new QWidget(mainWind);
//    SelecWidget->setAutoFillBackground(true);
//    SelecWidget->setGeometry(0, 75, 805, 348);

    qFont.setPointSize(16);
    QLabel *COSTitelLabel = new QLabel(Group_Res);
    COSTitelLabel->setGeometry(200, 10, 400, 40);
    COSTitelLabel->setAlignment(Qt::AlignCenter);
    QString costi = AglDeviceP->GetConfig("ResTitel");
    COSTitelLabel->setText(costi);
    COSTitelLabel->setFont(qFont);

    qFont.setPointSize(12);
    RadioPtID = new MyRadioButton(Group_Res);
    RadioDate = new MyRadioButton(Group_Res);

    RadioPtID->setStyle(QStyleFactory::create("fusion"));
    RadioDate->setStyle(QStyleFactory::create("fusion"));

    QString rad_sam = AglDeviceP->GetConfig("Radio_sam");
    QString rad_dat = AglDeviceP->GetConfig("Radio_Date");

    RadioPtID->setGeometry(133.5,55,140,30);
    RadioPtID->setText(rad_sam);
    RadioPtID->setFont(qFont);
    RadioDate->setGeometry(272.5,55,80,30);
    RadioDate->setText(rad_dat);
    RadioDate->setFont(qFont);

    if(RadioPtID->isChecked() == false && RadioDate->isChecked() == false)
    {
        iSelect = 0;
    }
    Edit_Sel = new myLineEdit(Group_Res);
    Edit_Sel->setStyle(QStyleFactory::create("fusion"));
    Edit_Sel->setGeometry(368,53,250,35);
    Edit_Sel->setAlignment(Qt::AlignCenter);
    Edit_Sel->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *HomeButton = new Mybutton(Group_Res);
    HomeButton->setStyle(QStyleFactory::create("fusion"));
    HomeButton->setGeometry(15, 13, 48, 48);

    QPixmap ima_home;
    QString homeload = AglDeviceP->GetPasswd("@MainSet_Home");
    ima_home.load(homeload);
    MainSetpix_Home = ima_home.scaled(HomeButton->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(HomeButton, MainSetpix_Home);
    Mybutton *retuButton = new Mybutton(Group_Res);
    retuButton->setStyle(QStyleFactory::create("fusion"));
    retuButton->setGeometry(13, 292, 48, 48);

    QPixmap ima_left;
    QString leftload = AglDeviceP->GetPasswd("@MainSet_3");
    ima_left.load(leftload);
    MainSetpix_3 = ima_left.scaled(retuButton->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(retuButton, MainSetpix_3);
    Mybutton *ParButton = new Mybutton(Group_Res);
    ParButton->setStyle(QStyleFactory::create("fusion"));
    ParButton->setGeometry(743, 292, 48, 48);

    QPixmap ima_repo;
    QString repoload = AglDeviceP->GetPasswd("@Repo_pix");
    ima_repo.load(repoload);
    Reportpix = ima_repo.scaled(ParButton->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(ParButton, Reportpix);

    Mybutton *DelButton = new Mybutton(Group_Res);
    DelButton->setStyle(QStyleFactory::create("fusion"));
    DelButton->setGeometry(743, 13, 48, 48);
    QPixmap ima_del;
    QString depoload = AglDeviceP->GetPasswd("@Dele_pix");
    ima_del.load(depoload);
    Delpix = ima_del.scaled(DelButton->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(DelButton, Delpix);
    if(isAdmin == false)                                 //  如果当前用户不是root  用户管理按钮对其隐藏
    {
        DelButton->hide();
    }
//------------------------------------Add-------------------------------------------------------------------------------//
    Mybutton *FoundButton = new Mybutton(Group_Res);
    FoundButton->setStyle(QStyleFactory::create("fusion"));
    FoundButton->setGeometry(620,53,48,48);
    QPixmap Pix_Found;
    Pix_Found.load(AglDeviceP->GetPasswd("@Found_pix"));
    Button_SetPix(FoundButton, Pix_Found);

    iSearCh = Sql->CountResultSql();
    QString Result_all = AglDeviceP->GetConfig("Label_Tests") + " " + QString::number(iSearCh);
    QLabel *Label_all = new QLabel(Group_Res);
    Label_all->setGeometry(133.5, 300, 400, 30);
    Label_all->setText(Result_all);
//----------------------------------------------------------------------------------------------------------------------------------------//
    if(iSearCh == -1)
    {
        qDebug() << "3723 Count Result DB ERROR";
        return;
    }
    double time = (double)iSearCh / (double)10;
    PageAll = iSearCh / 10;

    if(time > PageAll)
    {
        PageAll += 1;
    }
    if(PageAll == 0)
    {
        PageAll = 1;
    }
    PageNow = 1;
    QString allpage = QString :: number(PageAll, 10);
    QString nowpage = QString :: number(PageNow, 10);

    QPixmap Pg_Down;
    QString down_path = AglDeviceP->GetPasswd("@PageDn_pix");
    Pg_Down.load(down_path);
    But_PgDown = new Mybutton(Group_Res);
    But_PgDown->setStyle(QStyleFactory::create("fusion"));

    QPixmap Pg_Up;
    QString Up_path = AglDeviceP->GetPasswd("@PageUp_pix");
    Pg_Up.load(Up_path);
    But_PgUp = new Mybutton(Group_Res);
    But_PgUp->setStyle(QStyleFactory::create("fusion"));


    if(AglDeviceP->GetPasswd("@Languag") == "English")
    {
        But_PgDown->setGeometry(528, 292, 48, 48);
        But_PgUp->setGeometry(464, 292, 48, 48);
    }
    else
    {
        But_PgDown->setGeometry(563, 292, 48, 48);
        But_PgUp->setGeometry(499, 292, 48, 48);
    }

    QPixmap Pix_Down = Pg_Down.scaled(But_PgDown->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    QPixmap Pix_UP = Pg_Up.scaled(But_PgDown->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(But_PgDown, Pix_Down);
    Button_SetPix(But_PgUp, Pix_UP);

    Label_cou = new QLabel(Group_Res);
    if(AglDeviceP->GetPasswd("@Languag") == "English")
    {
        Label_cou->setGeometry(580, 300, 150, 30);
    }
    else
    {
         Label_cou->setGeometry(610, 300, 130, 30);
    }
    Label_cou->setText(nowpage + "/" + allpage + " " + AglDeviceP->GetConfig("Label_Page"));
    Label_cou->setAlignment(Qt :: AlignRight);
    Label_cou->setFont(qFont);

    if(iSearCh > 10)
    {
        iShow = 10;
    }
    else
    {
        iShow = iSearCh;
    }
    Table_Res = new MyTableWidget(iShow+1, 1, Group_Res);
    Table_Res->setStyle(QStyleFactory::create("fusion"));
    if(iSearCh > 4)
    {
        Table_Res->setGeometry(133.5,101,565,179);
        Table_Res->setColumnWidth(0, 563);
    }
    else
    {
        Table_Res->setGeometry(133.5,101,535,179);
        Table_Res->setColumnWidth(0, 533);
    }
    Table_Res->setSelectionBehavior(QAbstractItemView :: SelectRows);                  //  单元格为一次选中一行模式
    Table_Res->setSelectionMode(QAbstractItemView::SingleSelection);                   //  只能选中一个（两个设置放一起就是只能选中一行）
    Table_Res->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Table_Res->setFont(qFont);
    for(int j = 0; j < iShow; j++)
    {
        Table_Res->setRowHeight(j, 36);                      // 设置第j行单元格宽度
    }

    QString Show_item;
    QString ite_dat = AglDeviceP->GetConfig("Radio_Date");
    QString ite_lise = "#";

    if(AglDeviceP->GetPasswd("@Languag") == "Chinese")
    {
        sprintf(dat, "%-8.8s", ite_lise.toStdString().data());
        Show_item += (QString)dat;

        sprintf(dat, "%-18.18s", rad_sam.toStdString().data());
        Show_item += (QString)dat;

        sprintf(dat, "%-19.19s", ite_dat.toStdString().data());
        Show_item += (QString)dat;
    }
    else
    {
        sprintf(dat, "%-8.8s", ite_lise.toLatin1().data());
        Show_item += (QString)dat;

        sprintf(dat, "%-16.16s", rad_sam.toLatin1().data());
        Show_item += (QString)dat;

        sprintf(dat, "%-19.19s", ite_dat.toLatin1().data());
        Show_item += (QString)dat;
    }

     QTableWidgetItem *item_List = new QTableWidgetItem(Show_item);
     item_List->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);;                                        // 将第一行设置为不可编辑
     QHeaderView *head_hor = Table_Res->horizontalHeader();
     connect(head_hor, SIGNAL(sectionClicked(int)), this, SLOT(ClickedHeardview()),Qt::UniqueConnection);

     QHeaderView *head_ver = Table_Res->verticalHeader();
     head_ver->setHidden(true);                                                                                    // 去掉行号

     Table_Res->removeRow(0);
     Table_Res->setHorizontalHeaderItem(0,item_List);

     QScrollBar *Table_Re = Table_Res->verticalScrollBar(); // 获得该表格的滚动条
     MyScrollBar *Table_Res_bar = new MyScrollBar(Table_Res);
     Table_Res_bar->move(535,3);
     Table_Res_bar = (MyScrollBar*)Table_Re;
     Table_Res_bar->setStyleSheet("QScrollBar:vertical {""width: 30px; background-color: rgb(215,215,215);""}");     // 改变滚动条宽度，颜色

     struct ResultIndex RES_LIST;
     QString DateDate;
     QStringList DateList;
     int inDex = 1;

    QTableWidgetItem *Item_List[10];
    QString Item_index;

    for(; inDex <= iShow; inDex++)
    {
        QCoreApplication::processEvents();
        if(!Sql->ListResultSql(RES_LIST, inDex))
        {
            qDebug() << "3844 List Result DB ERROR";
            return;
        }
        Show_item.clear();
        Item_index = QString::number(inDex);
        sprintf(dat, "%-8.8s", Item_index.toLatin1().data());
        Show_item += (QString)dat;

        sprintf(dat, "%-16.16s", RES_LIST.SamId.toLatin1().data());
        Show_item += (QString)dat;

        DateList = RES_LIST.Date.split(";");
        DateDate = DateList.at(0);
        sprintf(dat, "%-19.19s", DateDate.toLatin1().data());
        Show_item += (QString)dat;

        Item_List[inDex-1] = new QTableWidgetItem;
        Item_List[inDex-1]->setText(Show_item);
        Item_List[inDex-1]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        Table_Res->setItem(inDex-1, 0, Item_List[inDex-1]);
        QCoreApplication::processEvents();
    }

    connect(FoundButton, SIGNAL(clicked()), this, SLOT(SelectOK()),Qt::UniqueConnection);
    connect(HomeButton, SIGNAL(clicked()), this, SLOT(pressretuButton()),Qt::UniqueConnection);
    connect(Table_Res,SIGNAL(itemClicked(QTableWidgetItem*)),this,SLOT(ClickedItemTable_Res()),Qt::UniqueConnection);
    connect(Edit_Sel, SIGNAL(clicked()), this, SLOT(pressSelButton()),Qt::UniqueConnection);
    connect(retuButton, SIGNAL(clicked()), this, SLOT(pressretuButton()),Qt::UniqueConnection);
    connect(ParButton,SIGNAL(clicked()), this, SLOT(pressSaveButton()),Qt::UniqueConnection);
//    connect(SqlPrint,SIGNAL(clicked()),this,SLOT(pressSqlPrint()));
    connect(But_PgDown,SIGNAL(clicked()),this,SLOT(pressBut_PgDown()),Qt::UniqueConnection);
    connect(But_PgUp,SIGNAL(clicked()),this,SLOT(pressBut_PgUp()),Qt::UniqueConnection);
    connect(DelButton, SIGNAL(clicked()), this, SLOT(pressDelButton()),Qt::UniqueConnection);
    connect(RadioDate,SIGNAL(clicked()),this,SLOT(pressRadio()),Qt::UniqueConnection);
    connect(RadioPtID,SIGNAL(clicked()),this,SLOT(pressRadio()),Qt::UniqueConnection);

    Group_Res->show();
    qDebug() << "Resules table uitype is " << UiType;
}

void PctGuiThread::ClickedHeardview()
{
    qDebug() << "ClickedHeardview";
    iShowTime = 0;
    if(isWidget == true)
    {
        AglDeviceP->SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
        isWidget = false;
    }
}

void PctGuiThread::CreatSeleResultWidget()
{
    QPalette palette;
    QFont qFont;
    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(76, 124, 216));

    Group_More = new QWidget(mainWind);
    Group_More->setStyle(QStyleFactory::create("windows"));
    Group_More->setStyleSheet(QStringLiteral("background-color: rgb(251,251,251);"));
    Group_More->setGeometry(0, 75, 805, 348);

    Mybutton *More_ret = new Mybutton(Group_More);
    More_ret->setStyle(QStyleFactory::create("fusion"));
    More_ret->setGeometry(13, 292, 48, 48);
    Button_SetPix(More_ret, MainSetpix_3);

    Mybutton *More_Home = new Mybutton(Group_More);
    More_Home->setStyle(QStyleFactory::create("fusion"));
    More_Home->setGeometry(15, 13, 48, 48);
    Button_SetPix(More_Home, MainSetpix_Home);

    Mybutton *More_line= new Mybutton(Group_More);
    More_line->setGeometry(743, 292, 48, 48);
    QPixmap ima_line;
    QString lineload = AglDeviceP->GetPasswd("@Line_pix");
    ima_line.load(lineload);
    Linepix = ima_line.scaled(More_line->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(More_line, Linepix);
//---------------------------------------Add----------------------------------//
    Mybutton *SqlPrint = new Mybutton(Group_More);
    SqlPrint->setStyle(QStyleFactory::create("fusion"));
    SqlPrint->setGeometry(743, 13, 48, 48);
    QPixmap ima_print;
    QString saveoad = AglDeviceP->GetPasswd("@Print_pix");
    ima_print.load(saveoad);
    Printpix = ima_print.scaled(SqlPrint->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(SqlPrint, Printpix);
//-------------------------------------------------------------------------------//
    qFont.setPointSize(16);
    QLabel *Label = new QLabel(Group_More);
    Label->setGeometry(250, 15, 300, 40);
    Label->setAlignment(Qt::AlignCenter);
    QString detai = AglDeviceP->GetConfig("Res_title");
    Label->setText(detai);
    Label->setFont(qFont);

    qFont.setPointSize(12);
    SelectText = new TextEdit(Group_More);
    SelectText->setStyle(QStyleFactory::create("fusion"));
    SelectText->setGeometry(80, 60, 640, 266);
    SelectText->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    SelectText->setReadOnly(true);
    SelectText->setText(Text_Select);
    SelectText->setFont(qFont);

    int Select = 0;
    Select = Sql->CountResultSql();
    if(-1 == Select)
    {
        qDebug() << "3934 Count Result DB ERROR";
        return;
    }

    Line1 = new QwtPlotCurve();
    Line1->setPen(Qt :: red, 0.5);                                                                         // 设置曲线颜色，粗细
    Line1->setCurveAttribute(QwtPlotCurve :: Fitted, true);

    Line2 = new QwtPlotCurve();
    Line2->setPen(Qt :: red, 0.5);
    Line2->setCurveAttribute(QwtPlotCurve :: Fitted, true);

    Line3 = new QwtPlotCurve();
    Line3->setPen(Qt :: red, 0.5);
    Line3->setCurveAttribute(QwtPlotCurve :: Fitted, true);

    Line4 = new QwtPlotCurve();
    Line4->setPen(Qt :: red, 0.5);
    Line4->setCurveAttribute(QwtPlotCurve :: Fitted, true);

    QScrollBar *table_num_bar =  SelectText->verticalScrollBar();                                                                                          // 获得该表格的滚动条
    MyScrollBar *Table_bar = new MyScrollBar(SelectText);
    Table_bar->move(640, 5);
    Table_bar = (MyScrollBar*)table_num_bar;
    Table_bar->setStyleSheet("QScrollBar:vertical {""width: 30px; background-color: rgb(215,215,215);""}");     // 改变滚动条宽度，颜色

    connect(SqlPrint,SIGNAL(clicked()),this,SLOT(pressSqlPrint()),Qt::UniqueConnection);
    connect(More_ret, SIGNAL(clicked()), this, SLOT(pressMore_Ret()),Qt::UniqueConnection);
    connect(More_Home, SIGNAL(clicked()), this, SLOT(pressMore_Home()),Qt::UniqueConnection);
    connect(More_line, SIGNAL(clicked()), this, SLOT(pressMore_line()),Qt::UniqueConnection);
    Group_More->show();
}

void PctGuiThread :: CreatSelecLine()         // 结果查询曲线图
{
    QFont font;
    font.setPointSize(10);
    QPalette palette;

    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));
    SelecLineWidget = new QWidget(mainWind);
    SelecLineWidget->setStyle(QStyleFactory::create("windows"));
    SelecLineWidget->setAutoFillBackground(true);
    SelecLineWidget->setGeometry(0, 75, 805, 348);
    SelecLineWidget->setPalette(palette);

    Mybutton *SelecLin_Ret = new Mybutton(SelecLineWidget);
    SelecLin_Ret->setGeometry(13, 292, 48, 48);
    Button_SetPix(SelecLin_Ret, MainSetpix_3);

    Mybutton *SelecLin_Home = new Mybutton(SelecLineWidget);
    SelecLin_Home->setGeometry(15, 13, 48, 48);
    Button_SetPix(SelecLin_Home, MainSetpix_Home);

    if(iMode == 1)
    {
        QLabel *Label_se_sam = new QLabel(SelecLineWidget);
        Label_se_sam->setGeometry(10, 80, 200, 30);
        Label_se_sam->setText(SamIDShow);
        Label_se_sam->setFont(font);

        Label_T1 = new QLabel(SelecLineWidget);
        Label_T1->setGeometry(10, 146, 160, 28);
        Label_C1 = new QLabel(SelecLineWidget);
        Label_C1->setGeometry(10, 166, 160, 28);

        font.setPointSize(10);

        QStringList TC_List = TC_Text.split("\r\n");
        Label_T1->setText(TC_List.at(2));
        Label_T1->setFont(font);
        Label_C1->setText(TC_List.at(3));
        Label_C1->setFont(font);

        font.setPointSize(16);
        QwtPlot *qwtplotP_A = new QwtPlot(SelecLineWidget);   // 坐标系
        qwtplotP_A->setGeometry(195, 65, 385, 255);
        qwtplotP_A->setFrameStyle(QFrame::NoFrame);
        qwtplotP_A->setLineWidth(10);
        qwtplotP_A->setCanvasBackground(QColor(255, 255, 255));
        qwtplotP_A->setAxisMaxMajor(QwtPlot::xBottom, 2);
        qwtplotP_A->setAxisMaxMajor(QwtPlot::yLeft, 5);

        QwtPlotGrid *grida = new QwtPlotGrid;
        grida->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
        grida->attach(qwtplotP_A);
        Line1->attach(qwtplotP_A);
        QLabel *LabelHB = new QLabel(SelecLineWidget);
        LabelHB->setGeometry(240, 34, 80, 30); 
        LabelHB->setText(Pro_Item);
    }

    else if(iMode == 4)
    {
        font.setPointSize(8);
        QwtPlot *qwtplotP_A = new QwtPlot(SelecLineWidget);   // 坐标系
        qwtplotP_A->setGeometry(160, 37, 270, 140);
        qwtplotP_A->setFrameStyle(QFrame::NoFrame);
        qwtplotP_A->setLineWidth(10);
        qwtplotP_A->setCanvasBackground(QColor(255, 255, 255));
        qwtplotP_A->setStyle(QStyleFactory::create("fusion"));
        qwtplotP_A->setAxisMaxMajor(QwtPlot::xBottom, 2);
        qwtplotP_A->setAxisMaxMajor(QwtPlot::yLeft, 5);

        QwtPlotGrid *grida = new QwtPlotGrid;
        grida->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
        grida->attach(qwtplotP_A);
        Line1->attach(qwtplotP_A);

        QwtPlot *qwtplotP_B = new QwtPlot(SelecLineWidget);
        qwtplotP_B->setGeometry(450, 37, 270, 140);                                                  //      坐标系
        qwtplotP_B->setFrameStyle(QFrame::NoFrame);                                              //    设置一些窗口熟悉
        qwtplotP_B->setLineWidth(10);
        qwtplotP_B->setCanvasBackground(QColor(255, 255, 255));                          //    设置画布背景
        qwtplotP_B->setStyle(QStyleFactory::create("fusion"));
        qwtplotP_B->setAxisMaxMajor(QwtPlot::xBottom, 2);
        qwtplotP_B->setAxisMaxMajor(QwtPlot::yLeft, 5);

        QwtPlotGrid *gridb = new QwtPlotGrid;                                                            //    增加网格
        gridb->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
        gridb->attach(qwtplotP_B);
        Line2->attach(qwtplotP_B);
        qwtplotP_B->replot();

        QwtPlot *qwtplotP_C = new QwtPlot(SelecLineWidget);
        qwtplotP_C->setGeometry(160, 200, 270, 140);
        qwtplotP_C->setFrameStyle(QFrame::NoFrame);
        qwtplotP_C->setLineWidth(10);
        qwtplotP_C->setCanvasBackground(QColor(255, 255, 255));
        qwtplotP_C->setStyle(QStyleFactory::create("fusion"));
        qwtplotP_C->setAxisMaxMajor(QwtPlot::xBottom, 2);
        qwtplotP_C->setAxisMaxMajor(QwtPlot::yLeft, 5);
        QwtPlotGrid *gridc = new QwtPlotGrid;
        gridc->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
        gridc->attach(qwtplotP_C);
        Line3->attach(qwtplotP_C);
        qwtplotP_C->replot();

        QwtPlot *qwtplotP_D = new QwtPlot(SelecLineWidget);
        qwtplotP_D->setGeometry(450, 200, 270, 140);
        qwtplotP_D->setFrameStyle(QFrame::NoFrame);
        qwtplotP_D->setLineWidth(10);
        qwtplotP_D->setCanvasBackground(QColor(255, 255, 255));
        qwtplotP_D->setStyle(QStyleFactory::create("fusion"));
        qwtplotP_D->setAxisMaxMajor(QwtPlot::xBottom, 2);   // X轴下方只显示两个标注
        qwtplotP_D->setAxisMaxMajor(QwtPlot::yLeft, 5);         //  Y轴左边只显示五个标注
        QwtPlotGrid *gridd = new QwtPlotGrid;
        gridd->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
        gridd->attach(qwtplotP_D);
        Line4->attach(qwtplotP_D);
        qwtplotP_D->replot();

        QLabel *LabelHB = new QLabel(SelecLineWidget);
        LabelHB->setText(AglDeviceP->GetConfig("Item5"));
        LabelHB->setGeometry(204, 180, 180, 20);
        LabelHB->setFont(font);

        QLabel * Label17 = new QLabel(SelecLineWidget);
        Label17->setText(AglDeviceP->GetConfig("Item4"));
        Label17->setGeometry(494 ,180, 180, 20);
        Label17->setFont(font);

        QLabel *LabelPG1 = new QLabel(SelecLineWidget);
        LabelPG1->setText(AglDeviceP->GetConfig("Item1"));
        LabelPG1->setGeometry(204, 17, 180, 20);
        LabelPG1->setFont(font);

        QLabel *LabelPG2 = new QLabel(SelecLineWidget);
        LabelPG2->setText(AglDeviceP->GetConfig("Item2"));
        LabelPG2->setGeometry(494, 17, 180, 20);
        LabelPG2->setFont(font);

        //------------------------------------Add--------------------------------------//
        font.setPointSize(10);

        QLabel *Label_se_sam = new QLabel(SelecLineWidget);
        Label_se_sam->setGeometry(10, 60, 200, 70);
        Label_se_sam->setText(SamIDShow);
        Label_se_sam->setFont(font);

        Label_T1 = new QLabel(SelecLineWidget);
        Label_T1->setGeometry(10, 126, 160, 28);
        Label_C1 = new QLabel(SelecLineWidget);
        Label_C1->setGeometry(10, 146, 160, 28);

        Label_T2 = new QLabel(SelecLineWidget);
        Label_T2->setGeometry(10, 166, 160, 28);
        Label_C2 = new QLabel(SelecLineWidget);
        Label_C2->setGeometry(10, 186, 160, 28);

        Label_T3 = new QLabel(SelecLineWidget);
        Label_T3->setGeometry(10, 206, 160, 28);
        Label_C3 = new QLabel(SelecLineWidget);
        Label_C3->setGeometry(10, 226, 160, 28);

        Label_T4 = new QLabel(SelecLineWidget);
        Label_T4->setGeometry(10, 246, 160, 28);
        Label_C4 = new QLabel(SelecLineWidget);
        Label_C4->setGeometry(10, 266, 160, 28);

        QStringList TC_List = TC_Text.split("\r\n");
        Label_T1->setText(TC_List.at(0));
        Label_T1->setFont(font);
        Label_C1->setText(TC_List.at(1));
        Label_C1->setFont(font);
        Label_T2->setText(TC_List.at(2));
        Label_T2->setFont(font);
        Label_C2->setText(TC_List.at(3));
        Label_C2->setFont(font);
        Label_T3->setText(TC_List.at(4));
        Label_T3->setFont(font);
        Label_C3->setText(TC_List.at(5));
        Label_C3->setFont(font);
        Label_T4->setText(TC_List.at(6));
        Label_T4->setFont(font);
        Label_C4->setText(TC_List.at(7));
        Label_C4->setFont(font);
        //-------------------------------------------------------------------------------//
        font.setPixelSize(6);
        qwtplotP_A->setFont(font);
        qwtplotP_B->setFont(font);
        qwtplotP_C->setFont(font);
        qwtplotP_D->setFont(font);
    }
    SelecLineWidget->show();
    connect(SelecLin_Ret, SIGNAL(clicked()), this, SLOT(pressSelecLin_Ret()),Qt::UniqueConnection);
    connect(SelecLin_Home, SIGNAL(clicked()), this, SLOT(pressSelecLin_Home()),Qt::UniqueConnection);
}

void PctGuiThread :: CreatTestWidget()          //  试验界面
{
    KeyInput.clear();
    isUpload = false;
    UiType = Testui;
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));
    QPalette palette_table;
    palette_table.setColor(QPalette::Background, QColor(251,251,251));
    QFont font;
    font.setPointSize(16);
    QFont font_Button;
    font_Button.setPointSize(10);
//------------------------------------------------------------------------------------------------//                                样本ID
    TestWidget = new QWidget(mainWind);
    TestWidget->setStyle(QStyleFactory::create("windows"));
    TestWidget->setAutoFillBackground(true);
    TestWidget->setGeometry(0, 75, 803, 348);
    TestWidget->setPalette(palette);

    QLabel *Label_Titel = new QLabel(TestWidget);
    Label_Titel->setGeometry(150, 11, 500, 40);
    Label_Titel->setAlignment(Qt :: AlignCenter);
    QString samtit = AglDeviceP->GetConfig("Sam_Titel");
    Label_Titel->setText(samtit);
    Label_Titel->setFont(font);

    QLabel *Label_sam = new QLabel(TestWidget);
//    Label_sam->setGeometry(87, 93, 180, 35);
    Label_sam->setGeometry(87, 76, 180, 35);
    Label_sam->setAlignment(Qt :: AlignLeft);
    QString samna = AglDeviceP->GetConfig("Sam_sam");
    Label_sam->setText(samna);
    Label_sam->setFont(font_Button);

    SamEdit = new myLineEdit(TestWidget);
    SamEdit->setStyle(QStyleFactory::create("fusion"));
//    SamEdit->setGeometry(227, 90, 320, 30);
    SamEdit->setGeometry(227, 73, 320, 30);
    SamEdit->setFont(font_Button);

    QString samuse = AglDeviceP->GetConfig("Label_Prepa");
    QLabel *Label_Use = new QLabel(TestWidget);
//    Label_Use->setGeometry(87, 162, 260, 35);
    Label_Use->setGeometry(87, 145, 260, 35);
    Label_Use->setAlignment(Qt :: AlignLeft);
    Label_Use->setText(samuse);
    Label_Use->setFont(font_Button);

    QLabel *Label_pare = new QLabel(TestWidget);
//    Label_pare->setGeometry(225, 160, 580, 39);
    Label_pare->setGeometry(225, 143, 580, 39);
    Label_pare->setAlignment(Qt :: AlignLeft);
    Label_pare->setFont(font_Button);

    Radio_fasting = new MyRadioButton(Label_pare);
    Radio_fasting->setGeometry(2, 2, 100, 35);
    Radio_fasting->setText(AglDeviceP->GetConfig("Radio_Fasting"));
    Radio_fasting->setChecked(true);

    Radio_Post = new MyRadioButton(Label_pare);
    Radio_Post->setGeometry(120, 2, 160, 35);
    Radio_Post->setText(AglDeviceP->GetConfig("Radio_Post"));

    Check_PPI = new CheckBox(TestWidget);
//    Check_PPI->setGeometry(228, 220, 400, 35);
    Check_PPI->setGeometry(228, 203, 400, 35);
    Check_PPI->setText(AglDeviceP->GetConfig("Sam_Use"));
    Check_PPI->setFont(font_Button);

    //-------------------------------new-------------------------------------//
    QLabel *group_pla = new QLabel(TestWidget);
//    group_pla->setGeometry(88, 274, 720, 38);
    group_pla->setGeometry(88, 257, 720, 38);
    group_pla->setStyle(QStyleFactory::create("windows"));

    QLabel *label_Sam = new QLabel(group_pla);
    label_Sam->setGeometry(2, 2, 100, 35);
    label_Sam->setText(AglDeviceP->GetConfig("Test_sam"));
    label_Sam->setFont(font_Button);

    QString radpla = AglDeviceP->GetConfig("Rad_Pla1");
    Radio_Pla1 = new MyRadioButton(group_pla);
    Radio_Pla2 = new MyRadioButton(group_pla);
    Radio_Pla1->setGeometry(139, 2, 100, 35);
    Radio_Pla1->setText(radpla);
    Radio_Pla1->setFont(font_Button);
    Radio_Pla1->setStyle(QStyleFactory::create("fusion"));

    Radio_Pla2->setGeometry(257, 2, 260, 35);
    QString radpla2 = AglDeviceP->GetConfig("Rad_Pla2");
    Radio_Pla2->setText(radpla2);
    Radio_Pla2->setFont(font_Button);
    Radio_Pla2->setStyle(QStyleFactory::create("fusion"));
    Radio_Pla2->setChecked(true);

    Edit_pla = new myLineEdit(group_pla);
    Edit_pla->setFont(font_Button);
//    Edit_pla->setEnabled(false);

    QLabel *Label_opt = new QLabel(group_pla);
    Edit_pla->setGeometry(515, 2, 30, 30);
    Label_opt->setGeometry(551, 2, 150, 35);

    QString radopt = AglDeviceP->GetConfig("Rad_opt");
    Label_opt->setText(radopt);
    Label_opt->setFont(font_Button);

    SamBut_next = new Mybutton(TestWidget);
    Mybutton *SamBut_ret = new Mybutton(TestWidget);
    Mybutton *SamBut_home = new Mybutton(TestWidget);

    SamBut_next->setStyle(QStyleFactory::create("fusion"));
    SamBut_ret->setStyle(QStyleFactory::create("fusion"));
    SamBut_home->setStyle(QStyleFactory::create("fusion"));

    SamBut_next->setGeometry(741, 290, 48, 48);
    QPixmap ima_next;
    QString nextload = AglDeviceP->GetPasswd("@Next_pix");
    ima_next.load(nextload);
    Nextpix = ima_next.scaled(SamBut_next->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(SamBut_next, Nextpix);
    SamBut_home->setGeometry(13, 11, 48, 48);
    SamBut_ret->setGeometry(11, 290, 48, 48);
    QPixmap ima_left;
    QString leftload = AglDeviceP->GetPasswd("@MainSet_3");
    ima_left.load(leftload);
    MainSetpix_3 = ima_left.scaled(SamBut_ret->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);

    QPixmap ima_home;
    QString homeload = AglDeviceP->GetPasswd("@MainSet_Home");
    ima_home.load(homeload);
    MainSetpix_Home = ima_home.scaled(SamBut_home->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);

    Button_SetPix(SamBut_ret, MainSetpix_3);
    Button_SetPix(SamBut_home, MainSetpix_Home);

    connect(Check_PPI, SIGNAL(clicked()), this, SLOT(Radio_test()),Qt::UniqueConnection);
    connect(Radio_fasting,SIGNAL(clicked()),this,SLOT(Radio_test()),Qt::UniqueConnection);
    connect(Radio_Post,SIGNAL(clicked()),this,SLOT(Radio_test()),Qt::UniqueConnection);
    connect(Radio_Pla1,SIGNAL(clicked()),this,SLOT(pressRadio_Pla1()),Qt::UniqueConnection);
    connect(Radio_Pla2,SIGNAL(clicked()),this,SLOT(pressRadio_Pla2()),Qt::UniqueConnection);
    connect(SamEdit, SIGNAL(clicked()), this, SLOT(pressSamEdit()),Qt::UniqueConnection);
    connect(SamBut_home, SIGNAL(clicked()), this, SLOT(pressSamBut_home()),Qt::UniqueConnection);
    connect(SamBut_ret, SIGNAL(clicked()), this, SLOT(pressSamBut_ret()),Qt::UniqueConnection);
    connect(SamBut_next, SIGNAL(clicked()), this, SLOT(pressSamBut_next()),Qt::UniqueConnection);
    connect(Edit_pla, SIGNAL(clicked()), this, SLOT(pressEdit_pla()),Qt::UniqueConnection);
//------------------------------------------------------------------------------------------------//    样本ID输入键盘
    KeyStation = 0;
    TestKeyWidget = new QWidget(TestWidget);
    TestKeyWidget->setStyle(QStyleFactory::create("fusion"));
//    TestKeyWidget->setGeometry(140, 170, 510, 238);
    TestKeyWidget->setGeometry(140, 105, 510, 238);
    TestKeyWidget->setStyleSheet(QStringLiteral("background-color: rgb(123, 139, 202);"));
    for(int i = 0; i < 26; i++)
    {
        SelectKey[i] = 'a' + i;
        pushButton[i] = new Mybutton(TestKeyWidget);
        pushButton[i]->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
        pushButton[i]->setText(QString(SelectKey[i]));
    }
    pushButton[0]->setGeometry(QRect(36, 120, 40, 30));
    pushButton[1]->setGeometry(QRect(260, 160, 40, 30));
    pushButton[2]->setGeometry(QRect(160, 160, 40, 30));
    pushButton[3]->setGeometry(QRect(136, 120, 40, 30));
    pushButton[4]->setGeometry(QRect(110, 80, 40, 30));
    pushButton[5]->setGeometry(QRect(186, 120, 40, 30));
    pushButton[6]->setGeometry(QRect(236, 120, 40, 30));
    pushButton[7]->setGeometry(QRect(286, 120, 40, 30));
    pushButton[8]->setGeometry(QRect(360, 80, 40, 30));
    pushButton[9]->setGeometry(QRect(336, 120, 40, 30));
    pushButton[10]->setGeometry(QRect(386, 120, 40, 30));
    pushButton[11]->setGeometry(QRect(436, 120, 40, 30));
    pushButton[12]->setGeometry(QRect(360, 160, 40, 30));
    pushButton[13]->setGeometry(QRect(310, 160, 40, 30));
    pushButton[14]->setGeometry(QRect(410, 80, 40, 30));
    pushButton[15]->setGeometry(QRect(460, 80, 40, 30));
    pushButton[16]->setGeometry(QRect(10, 80, 40, 30));
    pushButton[17]->setGeometry(QRect(160, 80, 40, 30));
    pushButton[18]->setGeometry(QRect(86, 120, 40, 30));
    pushButton[19]->setGeometry(QRect(210, 80, 40, 30));
    pushButton[20]->setGeometry(QRect(310, 80, 40, 30));
    pushButton[21]->setGeometry(QRect(210, 160, 40, 30));
    pushButton[22]->setGeometry(QRect(60, 80, 40, 30));
    pushButton[23]->setGeometry(QRect(110, 160, 40, 30));
    pushButton[24]->setGeometry(QRect(260, 80, 40, 30));
    pushButton[25]->setGeometry(QRect(60, 160, 40, 30));

    pushButton_Up = new Mybutton(TestKeyWidget);
    pushButton_Up->setGeometry(QRect(10, 160, 40, 30));
    QString Up0 = AglDeviceP->GetPasswd("@pix_up_0");
    QPixmap pix_up(Up0);
    pushButton_Up->setIcon(pix_up);
    pushButton_Up->setIconSize(QSize(40,25));
    pushButton_Up->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_0 = new Mybutton(TestKeyWidget);
    pushButton_0->setGeometry(QRect(410, 160, 40, 30));
    pushButton_0->setText(tr("0"));
    pushButton_0->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_dot = new Mybutton(TestKeyWidget);
    pushButton_dot->setGeometry(QRect(460, 160, 40, 30));
    pushButton_dot->setText(tr("-"));
    pushButton_dot->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_Space = new Mybutton(TestKeyWidget);
    pushButton_Space->setGeometry(QRect(100, 200, 260, 30));
    pushButton_Space->setText(tr(" "));
    pushButton_Space->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_del = new Mybutton(TestKeyWidget);
    pushButton_del->setGeometry(QRect(10, 200, 80, 30));
    pushButton_del->setText("Delete");
    pushButton_del->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_sure = new Mybutton(TestKeyWidget);
    pushButton_sure->setGeometry(QRect(420, 200, 80, 30));
    pushButton_sure->setText(tr("OK"));
    pushButton_sure->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_star = new Mybutton(TestKeyWidget);
    pushButton_star->setGeometry(QRect(370, 200, 40, 30));
    pushButton_star->setText(tr(":"));
    pushButton_star->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_7 = new Mybutton(TestKeyWidget);
    pushButton_7->setGeometry(QRect(336, 40, 40, 30));
    pushButton_7->setText(tr("7"));
    pushButton_7->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_2 = new Mybutton(TestKeyWidget);
    pushButton_2->setGeometry(QRect(86, 40, 40, 30));
    pushButton_2->setText(tr("2"));
    pushButton_2->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_9 = new Mybutton(TestKeyWidget);
    pushButton_9->setGeometry(QRect(436, 40, 40, 30));
    pushButton_9->setText(tr("9"));
    pushButton_9->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_3 = new Mybutton(TestKeyWidget);
    pushButton_3->setGeometry(QRect(136, 40, 40, 30));
    pushButton_3->setText(tr("3"));
    pushButton_3->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_4 = new Mybutton(TestKeyWidget);
    pushButton_4->setGeometry(QRect(186, 40, 40, 30));
    pushButton_4->setText(tr("4"));
    pushButton_4->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_8 = new Mybutton(TestKeyWidget);
    pushButton_8->setGeometry(QRect(386, 40, 40, 30));
    pushButton_8->setText(tr("8"));
    pushButton_8->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_5 = new Mybutton(TestKeyWidget);
    pushButton_5->setGeometry(QRect(236, 40, 40, 30));
    pushButton_5->setText(tr("5"));
    pushButton_5->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_1 = new Mybutton(TestKeyWidget);
    pushButton_1->setGeometry(QRect(36, 40, 40, 30));
    pushButton_1->setText(tr("1"));
    pushButton_1->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_6 = new Mybutton(TestKeyWidget);
    pushButton_6->setGeometry(QRect(286, 40, 40, 30));
    pushButton_6->setText(tr("6"));
    pushButton_6->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    QFont Font;
    Font.setPointSize(12);
    QLabel *label = new QLabel(TestKeyWidget);
    label->setObjectName(QStringLiteral("label"));
    label->setGeometry(QRect(4.5, 5, 500, 35));
    label->setAlignment(Qt :: AlignCenter);
    QString inpusam = AglDeviceP->GetConfig("SamKey");
    label->setText(inpusam);
    label->setFont(Font);

    connect(pushButton_sure , SIGNAL(clicked()), this, SLOT(presspushButton_sure()),Qt::UniqueConnection);
    connect(pushButton_del, SIGNAL(clicked()), this, SLOT(presspushButton_del()),Qt::UniqueConnection);
    connect(pushButton_Up, SIGNAL(clicked()), this, SLOT(pressKeyButton_up()),Qt::UniqueConnection);
    connect(pushButton_star, SIGNAL(clicked()), this, SLOT(pressKeyButton_star()),Qt::UniqueConnection);
    connect(pushButton_dot, SIGNAL(clicked()), this, SLOT(pressKeyButton_dot()),Qt::UniqueConnection);
    connect(pushButton_Space, SIGNAL(clicked()), this, SLOT(pressKeyButton_space()),Qt::UniqueConnection);
    connect(pushButton_0, SIGNAL(clicked()), this, SLOT(pressKeyButton_0()),Qt::UniqueConnection);
    connect(pushButton_1, SIGNAL(clicked()), this, SLOT(pressKeyButton_1()),Qt::UniqueConnection);
    connect(pushButton_2, SIGNAL(clicked()), this, SLOT(pressKeyButton_2()),Qt::UniqueConnection);
    connect(pushButton_3, SIGNAL(clicked()), this, SLOT(pressKeyButton_3()),Qt::UniqueConnection);
    connect(pushButton_4, SIGNAL(clicked()), this, SLOT(pressKeyButton_4()),Qt::UniqueConnection);
    connect(pushButton_5, SIGNAL(clicked()), this, SLOT(pressKeyButton_5()),Qt::UniqueConnection);
    connect(pushButton_6, SIGNAL(clicked()), this, SLOT(pressKeyButton_6()),Qt::UniqueConnection);
    connect(pushButton_7, SIGNAL(clicked()), this, SLOT(pressKeyButton_7()),Qt::UniqueConnection);
    connect(pushButton_8, SIGNAL(clicked()), this, SLOT(pressKeyButton_8()),Qt::UniqueConnection);
    connect(pushButton_9, SIGNAL(clicked()), this, SLOT(pressKeyButton_9()),Qt::UniqueConnection);
    connect(pushButton[0], SIGNAL(clicked()), this, SLOT(pressKeyButton_a()),Qt::UniqueConnection);
    connect(pushButton[1], SIGNAL(clicked()), this, SLOT(pressKeyButton_b()),Qt::UniqueConnection);
    connect(pushButton[2], SIGNAL(clicked()), this, SLOT(pressKeyButton_c()),Qt::UniqueConnection);
    connect(pushButton[3], SIGNAL(clicked()), this, SLOT(pressKeyButton_d()),Qt::UniqueConnection);
    connect(pushButton[4], SIGNAL(clicked()), this, SLOT(pressKeyButton_e()),Qt::UniqueConnection);
    connect(pushButton[5], SIGNAL(clicked()), this, SLOT(pressKeyButton_f()),Qt::UniqueConnection);
    connect(pushButton[6], SIGNAL(clicked()), this, SLOT(pressKeyButton_g()),Qt::UniqueConnection);
    connect(pushButton[7], SIGNAL(clicked()), this, SLOT(pressKeyButton_h()),Qt::UniqueConnection);
    connect(pushButton[8], SIGNAL(clicked()), this, SLOT(pressKeyButton_i()),Qt::UniqueConnection);
    connect(pushButton[9], SIGNAL(clicked()), this, SLOT(pressKeyButton_j()),Qt::UniqueConnection);
    connect(pushButton[10], SIGNAL(clicked()), this, SLOT(pressKeyButton_k()),Qt::UniqueConnection);
    connect(pushButton[11], SIGNAL(clicked()), this, SLOT(pressKeyButton_l()),Qt::UniqueConnection);
    connect(pushButton[12], SIGNAL(clicked()), this, SLOT(pressKeyButton_m()),Qt::UniqueConnection);
    connect(pushButton[13], SIGNAL(clicked()), this, SLOT(pressKeyButton_n()),Qt::UniqueConnection);
    connect(pushButton[14], SIGNAL(clicked()), this, SLOT(pressKeyButton_o()),Qt::UniqueConnection);
    connect(pushButton[15], SIGNAL(clicked()), this, SLOT(pressKeyButton_p()),Qt::UniqueConnection);
    connect(pushButton[16], SIGNAL(clicked()), this, SLOT(pressKeyButton_q()),Qt::UniqueConnection);
    connect(pushButton[17], SIGNAL(clicked()), this, SLOT(pressKeyButton_r()),Qt::UniqueConnection);
    connect(pushButton[18], SIGNAL(clicked()), this, SLOT(pressKeyButton_s()),Qt::UniqueConnection);
    connect(pushButton[19], SIGNAL(clicked()), this, SLOT(pressKeyButton_t()),Qt::UniqueConnection);
    connect(pushButton[20], SIGNAL(clicked()), this, SLOT(pressKeyButton_u()),Qt::UniqueConnection);
    connect(pushButton[21], SIGNAL(clicked()), this, SLOT(pressKeyButton_v()),Qt::UniqueConnection);
    connect(pushButton[22], SIGNAL(clicked()), this, SLOT(pressKeyButton_w()),Qt::UniqueConnection);
    connect(pushButton[23], SIGNAL(clicked()), this, SLOT(pressKeyButton_x()),Qt::UniqueConnection);
    connect(pushButton[24], SIGNAL(clicked()), this, SLOT(pressKeyButton_y()),Qt::UniqueConnection);
    connect(pushButton[25], SIGNAL(clicked()), this, SLOT(pressKeyButton_z()),Qt::UniqueConnection);

    TestWidget->show();
    TestKeyWidget->hide();
}

void PctGuiThread :: pressEdit_pla()
{
    Voice_pressKey();
    if(UiType == TestNumui || isClicked == true)
    {
        return;
    }
    UiType = TestNumui;
    text.clear();
    Radio_Pla1->setEnabled(false);
    Radio_Pla2->setEnabled(false);
    TestNumKey = new QWidget(TestWidget);
    if(AglDeviceP->GetPasswd("@Languag") == "English")
    {
        TestNumKey->setGeometry(235, 95, 290, 210);
    }
    else
    {
        TestNumKey->setGeometry(140, 100, 290, 210);
    }

    TestNumKey->setStyleSheet(QStringLiteral("background-color: rgb(58, 58, 58);"));

    QGridLayout *gridLayout_key = new QGridLayout(TestNumKey);
    gridLayout_key->setSpacing(6);
    gridLayout_key->setVerticalSpacing(1);
    gridLayout_key->setContentsMargins(10, 10, 10, 0);

    QFont font_key;
    font_key.setPointSize(14);

    Mybutton *button_0 = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_0, 3, 1, 1, 1);
    button_0->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_0->setText(tr("0")); button_0->setFixedHeight(35); button_0->setFont(font_key);
    Mybutton *button_1 = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_1, 0, 0, 1, 1);
    button_1->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_1->setText(tr("1")); button_1->setFixedHeight(35); button_1->setFont(font_key);
    Mybutton *button_2 = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_2, 0, 1, 1, 1);
    button_2->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_2->setText(tr("2")); button_2->setFixedHeight(35); button_2->setFont(font_key);
    Mybutton *button_3 = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_3, 0, 2, 1, 1);
    button_3->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_3->setText(tr("3")); button_3->setFixedHeight(35); button_3->setFont(font_key);
    Mybutton *button_4 = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_4, 1, 0, 1, 1);
    button_4->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_4->setText(tr("4")); button_4->setFixedHeight(35); button_4->setFont(font_key);
    Mybutton *button_5 = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_5, 1, 1, 1, 1);
    button_5->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_5->setText(tr("5")); button_5->setFixedHeight(35); button_5->setFont(font_key);
    Mybutton *button_6 = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_6, 1, 2, 1, 1);
    button_6->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_6->setText(tr("6")); button_6->setFixedHeight(35); button_6->setFont(font_key);
    Mybutton *button_7 = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_7, 2, 0, 1, 1);
    button_7->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_7->setText(tr("7")); button_7->setFixedHeight(35); button_6->setFont(font_key);
    Mybutton *button_8 = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_8, 2, 1, 1, 1);
    button_8->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_8->setText(tr("8")); button_8->setFixedHeight(35); button_8->setFont(font_key);
    Mybutton *button_9 = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_9, 2, 2, 1, 1);
    button_9->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_9->setText(tr("9")); button_9->setFixedHeight(35); button_9->setFont(font_key);
    Mybutton *button_sure = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_sure, 3, 0, 1, 1);
    button_sure->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_sure->setText(tr("OK")); button_sure->setFixedHeight(35); button_sure->setFont(font_key);
    Mybutton *button_clear = new Mybutton(TestNumKey); gridLayout_key->addWidget(button_clear, 3, 2, 1, 1);
    button_clear->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_clear->setText(tr("Delete")); button_clear->setFixedHeight(35); button_clear->setFont(font_key);

    connect(button_0, SIGNAL(clicked()), this, SLOT(showButton_0()),Qt::UniqueConnection);
    connect(button_1, SIGNAL(clicked()), this, SLOT(showButton_1()),Qt::UniqueConnection);
    connect(button_2, SIGNAL(clicked()), this, SLOT(showButton_2()),Qt::UniqueConnection);
    connect(button_3, SIGNAL(clicked()), this, SLOT(showButton_3()),Qt::UniqueConnection);
    connect(button_4, SIGNAL(clicked()), this, SLOT(showButton_4()),Qt::UniqueConnection);
    connect(button_5, SIGNAL(clicked()), this, SLOT(showButton_5()),Qt::UniqueConnection);
    connect(button_6, SIGNAL(clicked()), this, SLOT(showButton_6()),Qt::UniqueConnection);
    connect(button_7, SIGNAL(clicked()), this, SLOT(showButton_7()),Qt::UniqueConnection);
    connect(button_8, SIGNAL(clicked()), this, SLOT(showButton_8()),Qt::UniqueConnection);
    connect(button_9, SIGNAL(clicked()), this, SLOT(showButton_9()),Qt::UniqueConnection);
    connect(button_clear, SIGNAL(clicked()), this, SLOT(showButton_clear()),Qt::UniqueConnection);
    connect(button_sure, SIGNAL(clicked()), this, SLOT(showButton_sure()),Qt::UniqueConnection);

    TestNumKey->show();
}

void PctGuiThread :: CreatInforWidget()       //--------------------------------------//    信息确认
{
    UiType = Inforui;
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));
    QPalette palette_table;
    palette_table.setColor(QPalette::Background, QColor(251,251,251));
    QFont font;
    font.setPointSize(16);
    QFont Font;
    Font.setPointSize(12);

    isTimer_CodeStart = false;
    InfoWidget = new QWidget(TestWidget);
    InfoWidget->setAutoFillBackground(true);
    InfoWidget->setGeometry(-2, -2, 804, 351);
    InfoWidget->setPalette(palette);

    Mybutton *InfoBut_home = new Mybutton(InfoWidget);
    InfoBut_home->setGeometry(15, 13, 48, 48);
    Button_SetPix(InfoBut_home, MainSetpix_Home);

    InfoBut_ret = new Mybutton(InfoWidget);
    InfoBut_start = new Mybutton(InfoWidget);

    InfoBut_ret->setGeometry(13, 292, 48, 48);
    Button_SetPix(InfoBut_ret, MainSetpix_3);
    InfoBut_start->setGeometry(743, 292, 48, 48);

    QPixmap ima_start;
    QString startload = AglDeviceP->GetPasswd("@Start_pix");
    ima_start.load(startload);
    Startpix = ima_start.scaled(InfoBut_start->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(InfoBut_start, Startpix);

    Label_InfoTit = new QLabel(InfoWidget);
    Label_InfoSam_show = new QLabel(InfoWidget);

    Label_testing = new QLabel(InfoWidget);
    Label_testing->setGeometry(200, 245, 400, 50);
    Label_testing->setAlignment(Qt :: AlignCenter);
    QString wait = AglDeviceP->GetConfig("Infor_test");
    Label_testing->setText(wait);
    Label_testing->setStyleSheet(QStringLiteral("color: rgb(217, 22, 38);"));
    Label_testing->hide();

    Label_InfoTit->setGeometry(152, 13, 500, 40);
    Label_InfoTit->setAlignment(Qt :: AlignCenter);
    QString inftit = AglDeviceP->GetConfig("InfoTit");
    Label_InfoTit->setText(inftit);
    Label_InfoTit->setFont(font);
    TestBar = new QProgressBar(InfoWidget);
    TestBar->setGeometry(90, 290, 620, 40);
    TestBar->setPalette(palette);
    TestBar->setValue(0);
    TestBar->setAlignment(Qt :: AlignCenter);
    TestBar->hide();

    QString samp = AglDeviceP->GetConfig("Sam_sam");
    LineSam = SamIDShow;
    QString Sam_text = samp + " " + SamIDShow;
    Label_Sam_show = new QLabel(InfoWidget);            // Patient ID
    Label_Sam_show->setGeometry(90, 60, 400, 35);
    Label_Sam_show->setText(Sam_text);
    Label_Sam_show->setFont(Font);
    Label_Sam_show->setAlignment(Qt :: AlignLeft);

    QString infor1 = AglDeviceP->GetConfig("Infor_1");
    Label_Sam_1 = new QLabel(InfoWidget);
    Label_Sam_1->setGeometry(90, 110, 600, 30);
    Label_Sam_1->setText(infor1);
    Label_Sam_1->setAlignment(Qt :: AlignLeft);
    Label_Sam_1->setFont(Font);
    QString infor2 = AglDeviceP->GetConfig("Infor_2");
    Label_Sam_2 = new QLabel(InfoWidget);
    Label_Sam_2->setGeometry(90, 142, 600, 30);
    Label_Sam_2->setText(infor2);
    Label_Sam_2->setAlignment(Qt :: AlignLeft);
    Label_Sam_2->setFont(Font);
    QString infor3 = AglDeviceP->GetConfig("Infor_3");
    Label_Sam_3 = new QLabel(InfoWidget);
    Label_Sam_3->setGeometry(90, 174, 600, 30);
    Label_Sam_3->setText(infor3);
    Label_Sam_3->setAlignment(Qt :: AlignLeft);
    Label_Sam_3->setFont(Font);
    QString infor4 = AglDeviceP->GetConfig("Infor_4");
    Label_Sam_4 = new QLabel(InfoWidget);
    Label_Sam_4->setGeometry(90, 206, 700, 30);
    Label_Sam_4->setText(infor4);
    Label_Sam_4->setFont(Font);
    Label_Sam_4->setAlignment(Qt :: AlignLeft);

    Radio_after = new MyRadioButton(InfoWidget);
    Radio_after->setGeometry(86, 258, 230, 30);
    QString inforaf = AglDeviceP->GetConfig("Infor_after");
    Radio_after->setText(inforaf);
    Radio_after->setFont(Font);
    QString inforti = AglDeviceP->GetConfig("Edit_time");
    Edit_Mea = new myLineEdit(InfoWidget);
    Edit_Mea->setText(inforti);
    Edit_Mea->setFont(Font);
    Edit_Mea->setEnabled(false);
    QString informin = AglDeviceP->GetConfig("Infor_time");         // min
    Label_Mea = new QLabel(InfoWidget);

    Label_Mea->setText(informin);
    Label_Mea->setFont(Font);
    if(AglDeviceP->GetPasswd("@Languag") == "English")
    {
        Edit_Mea->setGeometry(262, 258, 30, 30);
        Label_Mea->setGeometry(296, 258, 50, 30);
    }
    else
    {
        Edit_Mea->setGeometry(208, 258, 30, 30);
        Label_Mea->setGeometry(243, 258, 50, 30);
    }
    QString infornow = AglDeviceP->GetConfig("Infor_now");
    Radio_now = new MyRadioButton(InfoWidget);
    Radio_now->setGeometry(86, 290, 230, 30);
    Radio_now->setText(infornow);
    Radio_now->setChecked(true);
    Radio_now->setFont(Font);

    Ttime = QTime::fromString(inforti+".00", "m.s");
//    QString Stime = Ttime.toString("mm:ss") + " " + AglDeviceP->GetConfig("Infor_time");
    QFont font_time;
    font_time.setPointSize(42);

    Label_Ti = new QLabel(InfoWidget);
    Label_Ti->setGeometry(200, 75,400, 200);
//    Label_Ti->setText(Stime);
    Label_Ti->setAlignment(Qt :: AlignCenter | Qt::AlignHCenter);
    Label_Ti->setFont(font_time);
    Label_Ti->hide();

    LCD_time = new QTimer(this);
    connect(LCD_time, SIGNAL(timeout()), this, SLOT(LCDtimer()),Qt::UniqueConnection);
    connect(InfoBut_home, SIGNAL(clicked()), this, SLOT(pressInfoBut_home()),Qt::UniqueConnection);
    connect(Radio_now, SIGNAL(clicked()), this, SLOT(pressRadio_now()),Qt::UniqueConnection);
    connect(Radio_after, SIGNAL(clicked()), this, SLOT(pressRadio_after()),Qt::UniqueConnection);
    connect(InfoBut_ret, SIGNAL(clicked()), this, SLOT(pressInfoBut_ret()),Qt::UniqueConnection);
    connect(InfoBut_start, SIGNAL(clicked()), this, SLOT(pressInfoBut_start()),Qt::UniqueConnection);
    connect(Edit_Mea, SIGNAL(clicked()), this, SLOT(pressEdit_Mea()),Qt::UniqueConnection);


    Timer_ScanLOT = new QTimer(this);
    connect(Timer_ScanLOT, SIGNAL(timeout()), this, SLOT(ReadScanStation()),Qt::UniqueConnection);
    connect(this, SIGNAL(LocationOK()),this,SLOT(SetMotor_Start_Location()),Qt::UniqueConnection);
    connect(this, SIGNAL(Move_Motor()),this,SLOT(SetMotor_Move()),Qt::UniqueConnection);

    InfoWidget->show();
}

void PctGuiThread :: CreatTestKey()                                                      // 试剂界面键盘
{
    KeyWidget = new QWidget(TestWidget);
    KeyWidget->setGeometry(230, 115, 340, 270);
    KeyWidget->setStyleSheet(QStringLiteral("background-color: rgb(58, 58, 58);"));
    text.clear();
    QGridLayout *gridLayout_key = new QGridLayout(KeyWidget);
    gridLayout_key->setSpacing(6);
    gridLayout_key->setVerticalSpacing(1);
    gridLayout_key->setContentsMargins(10, 60, 10, 0);

    QFont font_key;
    font_key.setPointSize(14);

    Edit_key = new myLineEdit(KeyWidget);
    Edit_key->setGeometry(10, 20, 212, 35);
    Edit_key->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));

    Mybutton *button_0 = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_0, 3, 1, 1, 1);
    button_0->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_0->setText(tr("0")); button_0->setFixedHeight(35); button_0->setFont(font_key);
    Mybutton *button_1 = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_1, 0, 0, 1, 1);
    button_1->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_1->setText(tr("1")); button_1->setFixedHeight(35); button_1->setFont(font_key);
    Mybutton *button_2 = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_2, 0, 1, 1, 1);
    button_2->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_2->setText(tr("2")); button_2->setFixedHeight(35); button_2->setFont(font_key);
    Mybutton *button_3 = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_3, 0, 2, 1, 1);
    button_3->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_3->setText(tr("3")); button_3->setFixedHeight(35); button_3->setFont(font_key);
    Mybutton *button_4 = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_4, 1, 0, 1, 1);
    button_4->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_4->setText(tr("4")); button_4->setFixedHeight(35); button_4->setFont(font_key);
    Mybutton *button_5 = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_5, 1, 1, 1, 1);
    button_5->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_5->setText(tr("5")); button_5->setFixedHeight(35); button_5->setFont(font_key);
    Mybutton *button_6 = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_6, 1, 2, 1, 1);
    button_6->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_6->setText(tr("6")); button_6->setFixedHeight(35); button_6->setFont(font_key);
    Mybutton *button_7 = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_7, 2, 0, 1, 1);
    button_7->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_7->setText(tr("7")); button_7->setFixedHeight(35); button_6->setFont(font_key);
    Mybutton *button_8 = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_8, 2, 1, 1, 1);
    button_8->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_8->setText(tr("8")); button_8->setFixedHeight(35); button_8->setFont(font_key);
    Mybutton *button_9 = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_9, 2, 2, 1, 1);
    button_9->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_9->setText(tr("9")); button_9->setFixedHeight(35); button_9->setFont(font_key);
    Mybutton *button_ret = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_ret, 3, 0, 1, 1);
    button_ret->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_ret->setText(tr("Return")); button_ret->setFixedHeight(35); button_ret->setFont(font_key);
    Mybutton *button_clear = new Mybutton(KeyWidget); gridLayout_key->addWidget(button_clear, 3, 2, 1, 1);
    button_clear->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_clear->setText(tr("Delete")); button_clear->setFixedHeight(35); button_clear->setFont(font_key);
    Mybutton *button_sure = new Mybutton(KeyWidget);
    button_sure->setGeometry(231, 20, 99, 35);
    button_sure->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_sure->setText(tr("OK")); button_sure->setFixedHeight(35); button_sure->setFont(font_key);

    connect(button_0, SIGNAL(clicked()), this, SLOT(showButton_0()),Qt::UniqueConnection);
    connect(button_1, SIGNAL(clicked()), this, SLOT(showButton_1()),Qt::UniqueConnection);
    connect(button_2, SIGNAL(clicked()), this, SLOT(showButton_2()),Qt::UniqueConnection);
    connect(button_3, SIGNAL(clicked()), this, SLOT(showButton_3()),Qt::UniqueConnection);
    connect(button_4, SIGNAL(clicked()), this, SLOT(showButton_4()),Qt::UniqueConnection);
    connect(button_5, SIGNAL(clicked()), this, SLOT(showButton_5()),Qt::UniqueConnection);
    connect(button_6, SIGNAL(clicked()), this, SLOT(showButton_6()),Qt::UniqueConnection);
    connect(button_7, SIGNAL(clicked()), this, SLOT(showButton_7()),Qt::UniqueConnection);
    connect(button_8, SIGNAL(clicked()), this, SLOT(showButton_8()),Qt::UniqueConnection);
    connect(button_9, SIGNAL(clicked()), this, SLOT(showButton_9()),Qt::UniqueConnection);
    connect(button_ret, SIGNAL(clicked()), this, SLOT(pressbutton_ret()),Qt::UniqueConnection);
    connect(button_clear, SIGNAL(clicked()), this, SLOT(showButton_clear()),Qt::UniqueConnection);
    connect(button_sure, SIGNAL(clicked()), this, SLOT(showButton_sure()),Qt::UniqueConnection);

    KeyWidget->show();
}

void PctGuiThread :: CreatResuWidget()                                      // 测试结果与分析界面
{
    UiType = TesTResui;
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(251,251,251));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));
    QPalette palette_table;
    palette_table.setColor(QPalette::Background, QColor(251,251,251));
    QFont font;
    font.setPointSize(16);
    QFont font_Button;
    font_Button.setPointSize(14);
//------------------------------------------------------------------------------------------------//    测试结果
    ResWidget = new QWidget(MainWindowP);
    ResWidget->setAutoFillBackground(true);
    ResWidget->setGeometry(0, 75, 803, 348);
    ResWidget->setPalette(palette);
    QLabel *Label_resu = new QLabel(ResWidget);
    Label_resu->setGeometry(200, 10, 400, 40);
    Label_resu->setAlignment(Qt :: AlignCenter);
    QString repo = AglDeviceP->GetConfig("res_resu");
    Label_resu->setText(repo);
    Label_resu->setFont(font);

    Mybutton *HomeButton = new Mybutton(ResWidget);
    HomeButton->setGeometry(13, 11, 48, 48);
    Button_SetPix(HomeButton, MainSetpix_Home);

    Mybutton *Button_Res_Rt = new Mybutton(ResWidget);                                                    // return button
    Button_Res_Rt->setGeometry(11, 290, 48, 48);
    Button_SetPix(Button_Res_Rt, MainSetpix_3);

    Mybutton *NextButton = new Mybutton(ResWidget);
    NextButton->setGeometry(741, 290, 48, 48);
    Button_SetPix(NextButton, Nextpix);

    Mybutton *pritButton = new Mybutton(ResWidget);
    pritButton->setGeometry(741, 231, 48, 48);
    QPixmap ima_print;
    QString printload = AglDeviceP->GetPasswd("@Print_pix");
    ima_print.load(printload);
    Printpix = ima_print.scaled(pritButton->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(pritButton, Printpix);

    Mybutton *AniButton = new Mybutton(ResWidget);
    AniButton->setGeometry(741, 11, 48, 48);
    QPixmap ima_line;
    QString lineload = AglDeviceP->GetPasswd("@Line_pix");
    ima_line.load(lineload);
    Linepix = ima_line.scaled(AniButton->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(AniButton, Linepix);
//----------Add--------------------------------------------------------------------------------------------------------//
    Mybutton *Upload_Button = new Mybutton(ResWidget);
    Upload_Button->setGeometry(741, 172, 48, 48);
    QPixmap ima_upload;
    QString upload = AglDeviceP->GetPasswd("@Upload_pix");
    ima_upload.load(upload);
    QPixmap UploadPix = ima_upload.scaled(Upload_Button->size(),Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    Button_SetPix(Upload_Button, UploadPix);
//-----------------------------------------------------------------------------------------------------------------------//
    font.setPointSize(12);
    ResText = new TextEdit(ResWidget);
    ResText->setGeometry(80, 60, 640, 266);
    ResText->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    ResText->setReadOnly(true);
    ResText->setFont(font);

    QScrollBar *table_num_bar =  ResText->verticalScrollBar();                                                                                          // 获得该表格的滚动条
    QScrollBar *Table_bar = new MyScrollBar(ResText);
    Table_bar->move(615, 5);
    Table_bar = table_num_bar;
    Table_bar->setStyleSheet("QScrollBar:vertical {""width: 30px; background-color: rgb(215,215,215);""}");     // 改变滚动条宽度，颜色

    TextShow.clear();

    QString UsrData = AglDeviceP->GetConfig("Login_user");
    TextDat = UsrData;
    sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
    TextShow += dat;
    sprintf(dat, "%-20.20s", LineSam.toLatin1().data());
    TextDat = dat;
    TextShow += TextDat + "\r\n";

//------------------------------------------------------------------------------------------------//    详细信息分析
    AnaWidget = new QWidget(MainWindowP);
    AnaWidget->setAutoFillBackground(true);
    AnaWidget->setGeometry(0, 75, 803, 350);
    AnaWidget->setPalette(palette);

    Mybutton *LineRet = new Mybutton(AnaWidget);
    LineRet->setGeometry(11, 290, 48, 48);
    Button_SetPix(LineRet, MainSetpix_3);

    Mybutton *Line_home = new Mybutton(AnaWidget);
    Line_home->setGeometry(13, 11, 48, 48);
    Button_SetPix(Line_home, MainSetpix_Home);

    font.setPointSize(10);
    Label_Hb = new QLabel(AnaWidget);
    Label_Hb->setFont(font);

    QLabel *Label_AnaSam = new QLabel(AnaWidget);
    Label_AnaSam->setAlignment(Qt :: AlignLeft);
    Label_AnaSam->setText(LineSam);
    Label_AnaSam->setFont(font);


//   AqwtplotP = new QwtPlot(AnaWidget);    // 坐标系 坐标系窗口A
//   AqwtplotP->setFrameStyle(QFrame::NoFrame);  //    设置一些窗口熟悉
//   AqwtplotP->setLineWidth(10);
//   AqwtplotP->setCanvasBackground(QColor(255, 255, 255)); //    设置画布背景
//   AqwtplotP->setStyle(QStyleFactory::create("fusion"));
//   AqwtplotP->setAxisMaxMajor(QwtPlot::xBottom, 2);  //    X轴下方只显示两个标注（1000，2000）
//   AqwtplotP->setAxisMaxMajor(QwtPlot::yLeft, 5);

//   QwtPlotGrid *grida = new QwtPlotGrid; //    增加网格
//   grida->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
//   grida->attach(AqwtplotP);

//   AyvLine = new QwtPlotCurve();                                                                          // 曲线A
//   AyvLine->setPen(Qt :: red, 0.5);                                                                         // 设置曲线颜色，粗细
//   AyvLine->setCurveAttribute(QwtPlotCurve :: Fitted, true);                             // 设置曲线光滑
//   AyvLine->attach(AqwtplotP);
//   AqwtplotP->replot();                                                                                           // 添加曲线后重新绘制坐标系

    if((int)AglDeviceP->AglDev.ChxVol == 1)
    {
      font.setPointSize(16);

      AqwtplotP = new QwtPlot(AnaWidget);    // 坐标系 坐标系窗口A
      AqwtplotP->setFrameStyle(QFrame::NoFrame);  //    设置一些窗口熟悉
      AqwtplotP->setLineWidth(10);
      AqwtplotP->setCanvasBackground(QColor(255, 255, 255)); //    设置画布背景
      AqwtplotP->setStyle(QStyleFactory::create("fusion"));
      AqwtplotP->setAxisMaxMajor(QwtPlot::xBottom, 2);  //    X轴下方只显示两个标注（1000，2000）
      AqwtplotP->setAxisMaxMajor(QwtPlot::yLeft, 5);
      AqwtplotP->setGeometry(195, 65, 385, 255);

      font.setPointSize(8);
      AqwtplotP->setFont(font);

      QwtPlotGrid *grida = new QwtPlotGrid; //    增加网格
      grida->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
      grida->attach(AqwtplotP);

      AyvLine = new QwtPlotCurve();                                                                          // 曲线A
      AyvLine->setPen(Qt :: red, 0.5);                                                                         // 设置曲线颜色，粗细
      AyvLine->setCurveAttribute(QwtPlotCurve :: Fitted, true);                             // 设置曲线光滑
//      AyvLine->attach(AqwtplotP);
//      AqwtplotP->replot();                                                                                           // 添加曲线后重新绘制坐标系

      Label_Hb->setGeometry(240, 34, 80, 30);
      Label_AnaSam->setGeometry(10, 80, 120, 30);
      Label_Hb->setText(AnaFound.XiangMuPm[0].xiangmu);

      Label_T1 = new QLabel(AnaWidget);
      Label_T1->setGeometry(10, 146, 160, 28);
      Label_C1 = new QLabel(AnaWidget);
      Label_C1->setGeometry(10, 166, 160, 28);

      font.setPointSize(10);

      Label_C1->setFont(font);
      Label_T1->setFont(font);
    }

    if((int)AglDeviceP->AglDev.ChxVol == 4)
    {
        font.setPointSize(9);
//------------------------------------------------------坐标系A---------------------------------------------------//
        AqwtplotP = new QwtPlot(AnaWidget);    // 坐标系 坐标系窗口A
        AqwtplotP->setFrameStyle(QFrame::NoFrame);  //    设置一些窗口熟悉
        AqwtplotP->setLineWidth(10);
        AqwtplotP->setCanvasBackground(QColor(255, 255, 255)); //    设置画布背景
        AqwtplotP->setStyle(QStyleFactory::create("fusion"));
        AqwtplotP->setAxisMaxMajor(QwtPlot::xBottom, 2);  //    X轴下方只显示两个标注（1000，2000）
        AqwtplotP->setAxisMaxMajor(QwtPlot::yLeft, 5);
        AqwtplotP->setGeometry(160, 37, 270, 140);

        font.setPointSize(8);
        AqwtplotP->setFont(font);

        QwtPlotGrid *grida = new QwtPlotGrid; //    增加网格
        grida->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
        grida->attach(AqwtplotP);

        AyvLine = new QwtPlotCurve();                                                                          // 曲线A
        AyvLine->setPen(Qt :: red, 0.5);                                                                         // 设置曲线颜色，粗细
        AyvLine->setCurveAttribute(QwtPlotCurve :: Fitted, true);                             // 设置曲线光滑
//        AyvLine->attach(AqwtplotP);
//        AqwtplotP->replot();
//------------------------------------------------------------------------------------------------------------------//
        Label_Hb->setGeometry(204,17, 260, 20);
        Label_AnaSam->setGeometry(10, 70, 100, 28);

        Label_T1 = new QLabel(AnaWidget);
        Label_T1->setGeometry(10, 126, 160, 28);
        Label_C1 = new QLabel(AnaWidget);
        Label_C1->setGeometry(10, 146, 160, 28);

        Label_T2 = new QLabel(AnaWidget);
        Label_T2->setGeometry(10, 166, 160, 28);
        Label_C2 = new QLabel(AnaWidget);
        Label_C2->setGeometry(10, 186, 160, 28);

        Label_T3 = new QLabel(AnaWidget);
        Label_T3->setGeometry(10, 206, 160, 28);
        Label_C3 = new QLabel(AnaWidget);
        Label_C3->setGeometry(10, 226, 160, 28);

        Label_T4 = new QLabel(AnaWidget);
        Label_T4->setGeometry(10, 246, 160, 28);
        Label_C4 = new QLabel(AnaWidget);
        Label_C4->setGeometry(10, 266, 160, 28);

        Label_Hb->setText(AglDeviceP->GetConfig("Item1"));                  // PG1
        Label_PG2 = new QLabel(AnaWidget);
        Label_PG2->setText(AglDeviceP->GetConfig("Item2"));               // PG2
        Label_PG2->setGeometry(494, 17, 260, 20);
//------------------------------------------------------坐标系B---------------------------------------------------//
        BqwtplotP = new QwtPlot(AnaWidget);
        BqwtplotP->setGeometry(450, 37, 270, 140);  //      坐标系
        BqwtplotP->setStyle(QStyleFactory::create("fusion"));
        BqwtplotP->setFrameStyle(QFrame::NoFrame);  //    设置一些窗口熟悉
        BqwtplotP->setLineWidth(10);
        BqwtplotP->setCanvasBackground(QColor(255, 255, 255)); //    设置画布背景
        BqwtplotP->setAxisMaxMajor(QwtPlot::xBottom, 2);
        BqwtplotP->setAxisMaxMajor(QwtPlot::yLeft, 5);

        QwtPlotGrid *gridb = new QwtPlotGrid; //    增加网格
        gridb->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
        gridb->attach(BqwtplotP);

        ByvLine = new QwtPlotCurve();                                                                           // 曲线B
        ByvLine->setPen(Qt :: red, 0.5);                                                                          // 设置曲线颜色，粗细
        ByvLine->setCurveAttribute(QwtPlotCurve :: Fitted, true);                             // 设置曲线光滑
//        ByvLine->attach(BqwtplotP);
//        BqwtplotP->replot();
//----------------------------------------------------------------------------------------------------------------//
        Label_HB = new QLabel(AnaWidget);
        Label_HB->setText(AglDeviceP->GetConfig("Item5"));          // Hp
        Label_HB->setGeometry(204, 180, 180, 20);
//------------------------------------------------------坐标系C---------------------------------------------------//
        CqwtplotP = new QwtPlot(AnaWidget);   // 坐标系
        CqwtplotP->setGeometry(160, 200, 270, 140);
        CqwtplotP->setFrameStyle(QFrame::NoFrame);
        CqwtplotP->setLineWidth(10);
        CqwtplotP->setCanvasBackground(QColor(255, 255, 255));
        CqwtplotP->setStyle(QStyleFactory::create("fusion"));
        CqwtplotP->setAxisMaxMajor(QwtPlot::xBottom, 2);
        CqwtplotP->setAxisMaxMajor(QwtPlot::yLeft, 5);

        QwtPlotGrid *gridc = new QwtPlotGrid;
        gridc->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
        gridc->attach(CqwtplotP);
        CyvLine = new QwtPlotCurve();                                                                           // 曲线C
        CyvLine->setPen(Qt :: red, 0.5);                                                                         // 设置曲线颜色，粗细
        CyvLine->setCurveAttribute(QwtPlotCurve :: Fitted, true);                            // 设置曲线光滑
//        CyvLine->attach(CqwtplotP);
//        CqwtplotP->replot();
//----------------------------------------------------------------------------------------------------------------//
        Label_17 = new QLabel(AnaWidget);
        Label_17->setText(AglDeviceP->GetConfig("Item4"));                   // G17
        Label_17->setGeometry(494 ,180, 180, 20);
//------------------------------------------------------坐标系D---------------------------------------------------//
        DqwtplotP = new QwtPlot(AnaWidget);   // 坐标系
        DqwtplotP->setGeometry(450, 200, 270, 140);
        DqwtplotP->setFrameStyle(QFrame::NoFrame);
        DqwtplotP->setLineWidth(10);
        DqwtplotP->setCanvasBackground(QColor(255, 255, 255));
        DqwtplotP->setStyle(QStyleFactory::create("fusion"));
        DqwtplotP->setAxisMaxMajor(QwtPlot::xBottom, 2);
        DqwtplotP->setAxisMaxMajor(QwtPlot::yLeft, 5);
//    增加网格
        QwtPlotGrid *gridd = new QwtPlotGrid;
        gridd->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
        gridd->attach(DqwtplotP);

        DyvLine = new QwtPlotCurve();                                                                          // 曲线D
        DyvLine->setPen(Qt :: red, 0.5);                                                                         // 设置曲线颜色，粗细
        DyvLine->setCurveAttribute(QwtPlotCurve :: Fitted, true);                            // 设置曲线光滑
//        DyvLine->attach(DqwtplotP);
//        DqwtplotP->replot();

        qDebug() << "-----------------坐标系设置完成";
//----------------------------------------------------------------------------------------------------------------//
        font.setPixelSize(6);
        DqwtplotP->setFont(font);
        CqwtplotP->setFont(font);
        BqwtplotP->setFont(font);
        AqwtplotP->setFont(font);

        font.setPointSize(8);
        Label_17->setFont(font);
        Label_HB->setFont(font);
        Label_PG2->setFont(font);
        Label_Hb->setFont(font);

        font.setPointSize(10);
        Label_C1->setFont(font);
        Label_C2->setFont(font);
        Label_C3->setFont(font);
        Label_C4->setFont(font);

        Label_T1->setFont(font);
        Label_T2->setFont(font);
        Label_T3->setFont(font);
        Label_T4->setFont(font);

    }

    connect(Upload_Button, SIGNAL(clicked()), this, SLOT(pressUpload_Button()),Qt::UniqueConnection);
    connect(Line_home, SIGNAL(clicked()), this, SLOT(pressLine_home()),Qt::UniqueConnection);
    connect(HomeButton, SIGNAL(clicked()), this, SLOT(pressHomeButton()),Qt::UniqueConnection);
    connect(AniButton, SIGNAL(clicked()), this, SLOT(pressAnaButton()),Qt::UniqueConnection);
    connect(LineRet, SIGNAL(clicked()), this, SLOT(pressRetButton()),Qt::UniqueConnection);
    connect(Button_Res_Rt, SIGNAL(clicked()), this, SLOT(pressHomeButton()),Qt::UniqueConnection);
    connect(NextButton, SIGNAL(clicked()), this, SLOT(pressHomeButton()),Qt::UniqueConnection);
    connect(pritButton, SIGNAL(clicked()), this, SLOT(presspritButton()),Qt::UniqueConnection);

    AnaWidget->hide();
    ResWidget->show();
}

void PctGuiThread::flush_door_time()
{
    if(UiType == MainUi)
    {
        static bool isReadDoor = false;
        if(true == isReadDoor)
        {
            return;
        }
        isReadDoor = true;
        QCoreApplication::processEvents();
        AglDeviceP->agl_rd_stat();
        QCoreApplication::processEvents();
        Door_flush = (int)AglDeviceP->AglDev.OutIntState;
        if(isDoorClicked == true)
        {
            if(Door_State == Door_flush)
            {
                isCanOpenDoor = true;
                isDoorClicked = false;
            }
            else
            {
                isCanOpenDoor = false;
            }
        }
        isReadDoor = false;
    }
    if(UiType == Updateui)
    {
        return;
    }
    if(false == isBarCodeGetOK && isClickedCloseDoor == false)
    {
        QString ReadCOMData = "";
        QByteArray Station = AglDeviceP->agl_rd_data(0x05, 0);
        ReadCOMData = Station.data();
//        qDebug("<><><><><><><><><><><><><><><><><><> Select OK");
        if("OK" == ReadCOMData)
        {
            qDebug() << "We have Read OK";
            isBarCodeGetOK = true;
        }
    }
}

void PctGuiThread :: flush_time()                                                                // 时间刷新函数
{
//    if(false == isBarCodeGetOK)
//    {
//        qDebug() << "<><><><><><><><><><><><><><><><><><> false == isBarCodeGetOK";
//    }
//    else
//    {
//        qDebug() << "<><><><><><><><><><><><><><><><><><> true == isBarCodeGetOK";
//    }
//    if(false == isClickedCloseDoor)
//    {
//        qDebug() << "<><><><><><><><><><><><><><><><><><> false == isClickedCloseDoor";
//    }
//    else
//    {
//        qDebug() << "<><><><><><><><><><><><><><><><><><> true == isClickedCloseDoor";
//    }
//    if(false == isNext)
//    {
//        qDebug() << "<><><><><><><><><><><><><><><><><><> false == isNext";
//    }
//    else
//    {
//        qDebug() << "<><><><><><><><><><><><><><><><><><> true == isNext";
//    }
//    if(false == isClicked)
//    {
//        qDebug() << "<><><><><><><><><><><><><><><><><><> false == isClicked";
//    }
//    else
//    {
//        qDebug() << "<><><><><><><><><><><><><><><><><><> true == isClicked";
//    }
//    if(false == isStart)
//    {
//        qDebug() << "<><><><><><><><><><><><><><><><><><> false == isStart\r\n\r\n";
//    }
//    else
//    {
//        qDebug() << "<><><><><><><><><><><><><><><><><><> true == isStart\r\n\r\n";
//    }

    QDateTime time = QDateTime :: currentDateTime();
    QString now = time.toString("yyyy-MM-dd hh:mm:ss");
    Label_time->setText(now);
    if(UiType == MainUi)
    {
        if(AglDeviceP->AppCfg->LoginPort == 3)
        {
            AglDeviceP->AppCfg->LoginPort = 0;
        }
    }
    else
    {
        if(AglDeviceP->AppCfg->LoginPort == 0)
        {
            AglDeviceP->AppCfg->LoginPort = 3;
        }
    }
    int8_t Port_state = AglDeviceP->AppCfg->LoginPort;
    if(Port_state == (int8_t)1)
    {
        if(isControl == false)
        {
            Label_station->setText(AglDeviceP->GetConfig("Control_COM"));
            Label_station->setAlignment(Qt::AlignLeft);
            isControl = true;
        }
//        else
//        {
//            QMessageBox *msgBox = new QMessageBox(MainWindowP);
//            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//            msgBox->setText(AglDeviceP->GetConfig("Control_COM"));
//            msgBox->setIcon(QMessageBox :: Warning);
//            msgBox->exec();
//            Voice_pressKey();
//        }
//        switch (UiType)
//        {
//            case Setui:
//            {
//                SetWidget->close();
//                Widget_num->close();
//                UiType = MainUi;
//                break;
//            }
//            case COSui:
//            {
//                UsrWidget->close();
//                COSWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case HostIpui:
//            {
//                UsrWidget->close();
//                COSWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case LisIpui:
//            {
//                UsrWidget->close();
//                COSWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case LisPortui:
//            {
//                UsrWidget->close();
//                COSWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case Usrui :
//            {
//                COSWidget->close();
//                UsrWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case Usrkeyui :
//            {
//                KeyInput.clear();
//                COSWidget->close();
//                UsrWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case Updateui:
//            {
//                isUpdate = false;
//                UsrWidget->close();
//                COSWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case SetUserui:
//            {
//                USERWidget->close();
//                COSWidget->close();
//                UsrWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case Passui:
//            {
//                COSWidget->close();
//                UsrWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case UserKeyui:
//            {
//                KeyInput.clear();
//                USERWidget->close();
//                COSWidget->close();
//                UsrWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case PassKeyui:
//            {
//                KeyInput.clear();
//                USERWidget->close();
//                COSWidget->close();
//                UsrWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case NewKeyui:
//            {
//                KeyInput.clear();
//                USERWidget->close();
//                COSWidget->close();
//                UsrWidget->close();
//                SetWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case Selectui:
//            {
//                SelecWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case SelectKeyui:
//            {
//                SelecWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case Testui:
//            {
//                TestWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case TestKeyui:
//            {
//                TestWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case TestNumui:
//            {
//                text.clear();
//                TestWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case Inforui:
//            {
//                text.clear();
//                InfoWidget->close();
//                TestWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            case TesTResui:
//            {
//                LCD_time->stop();
//                KeyInput.clear();
//                SamEdit->clear();
//                isCount = false;
//                isStart = false;
//                Count_test = 0;
//                UiTimer->stop();
//                AnaWidget->close();
//                ResWidget->close();
//                InfoWidget->close();
//                TestWidget->close();
//                UiType = MainUi;
//                break;
//            }
//            default:
//            {
//                break;
//            }
//        }
    }
    else if(Port_state == (int8_t)2)
    {
        if(isControl == false)
        {
            Label_station->setText(AglDeviceP->GetConfig("Control_NET"));
            Label_station->setAlignment(Qt::AlignLeft);
            isControl = true;
        }
        else
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Control_NET"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
        }
    }
    else if(Port_state == (int8_t)0)
    {
        if(isControl == true)
        {
            isControl = false;
            Label_station->clear();
        }
    }
}

void PctGuiThread :: flush_icon()                                                                 // 图标刷新函数
{
    int i = 1;
    while (i--)
    {       
//        if(2.00 == AglDeviceP->yx_power)
//        {
//            Label_Q->setPixmap(QPixmap :: fromImage(result_Q[0]));
//            if(iChange[0] > 0)                      // 记录当前状态声音响应状态，初始化为1，表示响应当前状态
//            {
//                if(iCount_icon < 1)                // 记录当前状态模式，初始化为1，表示是开机缺省状态，不响应。
//                {
//                    Voice_LowBattery();
//                    isMessage = true;
//                    msgBox = new QMessageBox(MainWindowP);
//                    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//                    msgBox->setText(AglDeviceP->GetConfig("sattion_connect"));
//                    msgBox->setIcon(QMessageBox :: Warning);
//                    msgBox->exec();
//                    delete msgBox;
//                    isMessage = false;
//                    if(isWidget == false)
//                    {
//                        Voice_pressKey();
//                    }
//                }
//                iChange[0] = 0;                  // 响应完当前状态后，赋值为0，表示当前状态不再响应。确保每种状态只有一次响应。
//                iChange[1] = 1;
//                iChange[2] = 1;
//                iChange[3] = 1;
//                iChange[4] = 1;
//                iPower = 1;
//            }
//            break;
//        }
//        else if(1.00 == AglDeviceP->yx_power)
//        {
//            Label_Q->setPixmap(QPixmap :: fromImage(result_Q[4]));
//            if(iChange[1] > 0)
//            {
//                if(iCount_icon < 1)
//                {
//                    if(iPower > 0)
//                    {
//                        Voice_LowBattery();
//                        isMessage = true;
//                        msgBox = new QMessageBox(MainWindowP);
//                        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//                        msgBox->setText(AglDeviceP->GetConfig("sattion_connect"));
//                        msgBox->setIcon(QMessageBox :: Warning);
//                        msgBox->exec();
//                        delete msgBox;
//                        isMessage = false;
//                        if(isWidget == false)
//                        {
//                            Voice_pressKey();
//                        }
//                    }
//                    else
//                    {
//                        Voice_ChangStation();
//                    }
//                }
//                iChange[0] = 1;
//                iChange[1] = 0;
//                iChange[2] = 1;
//                iChange[3] = 1;
//                iChange[4] = 1;
//                iPower = 0;
//            }
//            break;
//        }
//        else if(0.75 == AglDeviceP->yx_power)
//        {
//            Label_Q->setPixmap(QPixmap :: fromImage(result_Q[3]));
//            if(iChange[2] > 0)
//            {
//                if(iCount_icon < 1)
//                {
//                    if(iPower > 0)
//                    {
//                        Voice_LowBattery();
//                        isMessage = true;
//                        msgBox = new QMessageBox(MainWindowP);
//                        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//                        msgBox->setText(AglDeviceP->GetConfig("sattion_connect"));
//                        msgBox->setIcon(QMessageBox :: Warning);
//                        msgBox->exec();
//                        delete msgBox;
//                        if(isWidget == false)
//                        {
//                            Voice_pressKey();
//                        }
//                        isMessage = false;
//                    }
//                    else
//                    {
//                        Voice_ChangStation();
//                    }
//                }
//                iChange[0] = 1;
//                iChange[1] = 1;
//                iChange[2] = 0;
//                iChange[3] = 1;
//                iChange[4] = 1;
//                iPower = 0;
//            }
//            break;
//        }
//        else if(0.5 == AglDeviceP->yx_power)
//        {
//            Label_Q->setPixmap(QPixmap :: fromImage(result_Q[2]));
//            if(iChange[3] > 0 )
//            {
//                if(iCount_icon < 1)
//                {
//                    Voice_LowBattery();
//                    if(iPower > 0)
//                    {
//                        isMessage = true;
//                        msgBox = new QMessageBox(MainWindowP);
//                        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//                        msgBox->setText(AglDeviceP->GetConfig("sattion_discon"));
//                        msgBox->setIcon(QMessageBox :: Warning);
//                        msgBox->exec();
//                        delete msgBox;
//                        isMessage = false;
//                        if(isWidget == false)
//                        {
//                            Voice_pressKey();
//                        }
//                    }
//                    else
//                    {
//                        isMessage = true;
//                        msgBox = new QMessageBox(MainWindowP);
//                        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//                        msgBox->setText(AglDeviceP->GetConfig("station_low"));
//                        msgBox->setIcon(QMessageBox :: Warning);
//                        msgBox->exec();
//                        delete msgBox;
//                        isMessage = false;
//                        if(isWidget == false)
//                        {
//                            Voice_pressKey();
//                        }
//                    }
//                }
//                iChange[0] = 1;
//                iChange[1] = 1;
//                iChange[2] = 1;
//                iChange[3] = 0;
//                iChange[4] = 1;
//                iPower = 0;
//            }
//            break;
//        }
//        else if(0.25 == AglDeviceP->yx_power)
//        {
//            Label_Q->setPixmap(QPixmap :: fromImage(result_Q[1]));
//            if(iChange[4] > 0)
//            {
//                if(iCount_icon < 1)
//                {
//                    Voice_LowBattery();
//                    if(iPower > 0)
//                    {
//                        isMessage = true;
//                        msgBox = new QMessageBox(MainWindowP);
//                        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//                        msgBox->setText(AglDeviceP->GetConfig("sattion_discon"));
//                        msgBox->setIcon(QMessageBox :: Warning);
//                        msgBox->exec();
//                        delete msgBox;
//                        isMessage = false;
//                        if(isWidget == false)
//                        {
//                            Voice_pressKey();
//                        }
//                    }
//                    else
//                    {
//                        isMessage = true;
//                        msgBox = new QMessageBox(MainWindowP);
//                        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//                        msgBox->setText(AglDeviceP->GetConfig("station_low"));
//                        msgBox->setIcon(QMessageBox :: Warning);
//                        msgBox->exec();
//                        delete msgBox;
//                        isMessage = false;
//                        if(isWidget == false)
//                        {
//                            Voice_pressKey();
//                        }
//                    }
//                }
//                iChange[0] = 1;
//                iChange[1] = 1;
//                iChange[2] = 1;
//                iChange[3] = 1;
//                iChange[4] = 0;
//                iPower = 0;
//            }
//            break;
//        }
    }
    if(AglDeviceP->yx_network)
    {
//------Add-----------------------------------------------------------------------------------------------------//
        if(isStart == true || (UiType == TesTResui && isUpload == false && isCount == false))
        {
            if(isDisconnect == true)
            {
                if(LisSocket->state() == 3)
                {
                    LisSocket->disconnectFromHost();
                    LisSocket->close();
                }
                qDebug() << "网线掉落后，断开网络连接";
            }
            if(LisSocket->state() == 0)
            {              
                bool ok;
                QString LisIp = AglDeviceP->GetPasswd("@LisIP");
                quint16 LisPort = AglDeviceP->GetPasswd("@LisPort").toUInt(&ok, 10);
                if(!LisSocket->open(QIODevice :: ReadWrite))
                {
                    qDebug() << "'Open Socket error";
                    return;
                }
                LisSocket->connectToHost(LisIp, LisPort);
                isDisconnect = false;
            }
        }
        if(LisSocket->state() == 3)
        {
            if(iChange[5] > 0)
            {
                if(iCount_icon < 1)
                {
                    Voice_ChangStation();
                }
                iChange[5] = 0;
                iChange[6] = 1;
                iChange_socket = 1;
            }
            Label_net->setPixmap(QPixmap :: fromImage(result_net[1]));
        }
        else
        {
            if(iChange_socket > 0)
            {
                if(iCount_icon < 1)
                {
                    Voice_ChangStation();
                }
                iChange[5] = 1;
                iChange[6] = 1;
                iChange_socket = 0;
            }
            Label_net->setPixmap(QPixmap :: fromImage(result_socket));
        }
//----------------------------------------------------------------------------------------------------------------//
    }
    else
    {
        isConnect = false;
        isDisconnect = true;
        if(iChange[6] > 0)
        {
            if(iCount_icon < 1)
            {
                Voice_ChangStation();
            }
            iChange[5] = 1;
            iChange[6] = 0;
            iChange_socket = 1;
        }
        Label_net->setPixmap(QPixmap :: fromImage(result_net[0]));       
    }
    if(AglDeviceP->yx_udisk)
    {
        if(iChange[7] > 0)
        {
            if(iCount_icon < 1)
            {
                Voice_ChangStation();
            }
            iChange[7] = 0;
            iChange[8] = 1;
        }
        Label_U->setPixmap(QPixmap :: fromImage(result_U));
    }
    else
    {
        if(iChange[8] > 0)
        {
            if(iCount_icon < 1)
            {
                Voice_ChangStation();
            }
            iChange[7] = 1;
            iChange[8] = 0;
        }
        Label_U->clear();
    }

    if(AglDeviceP->yx_sdisk)
    {
        Label_SD->setPixmap(QPixmap :: fromImage(result_SD));
        if(iChange[9] > 0)
        {
            if(iCount_icon < 1)
            {
                Voice_ChangStation();
            }
            iChange[9] = 0;
            iChange[10] = 1;
        }
    }
    else
    {
        if(iChange[10] > 0)
        {
            if(iCount_icon < 1)
            {
                Voice_ChangStation();
            }
            iChange[9] = 1;
            iChange[10] = 0;
        }
        Label_SD->clear();
    }
    iCount_icon = 0;
}

void PctGuiThread::LoginWindow()  // 登录界面
{
    UiType = LoginUi;
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(0,173,255));
    palette.setColor(QPalette::WindowText,QColor(0,0,0));//字体色
    //登陆界面
    LoginWidetP = new MyWidget(MainWindowP);
    //LoginWidetP->setWindowFlags(Qt::WindowTitleHint|Qt::WindowSystemMenuHint);
    LoginWidetP->setGeometry(QRect(0,0,800,480));
    LoginWidetP->setFixedSize(800,480);
    LoginWidetP->setAutoFillBackground(true);
    LoginWidetP->setPalette(palette);
    LoginWidetP->setAttribute(Qt::WA_AcceptTouchEvents);

    QLabel *Label_user = new QLabel(LoginWidetP);
    QLabel *Label_pass = new QLabel(LoginWidetP);

    QString Pass = AglDeviceP->GetConfig("Pass_text");
    QString User = AglDeviceP->GetConfig("User_text");

    Label_pass->setText(Pass);
    Label_user->setText(User);
    Label_user->setGeometry(221, 78, 125, 40);
    Label_pass->setGeometry(221, 128, 125, 40);

    LineEdit_input = new myLineEdit(LoginWidetP);
    LineEdit_input->setEchoMode(QLineEdit::Password);;
    LineEdit_input->setGeometry(352, 132, 210, 40);

    QStringList UsrList = AglDeviceP->GetAllUserData();
    int UsrLengh = UsrList.length();

    QString USR[UsrLengh];
    QStringList USr_list[UsrLengh];
    for(int i = 0;i < UsrLengh-1; i++)
    {
        USR[i] = UsrList.at(i);
        USr_list[i] = USR[i].split("=");
        qDebug() << "User Message has " << USR[i];
    }

    combox = new ComBox(LoginWidetP);
    for(int i = 0; i < UsrLengh-1; i++)
    {
        combox->addItem(USr_list[i].at(0), i+1);
    }
    combox->setGeometry(352, 82, 210, 40);
//----------------------------------------------------------------------------------------------------------------------//
//    QTimer *Timer_pic = new QTimer(this);
//    Timer_pic->start(3000);
//    connect(Timer_pic,SIGNAL(timeout()),this,SLOT(Showbutton()));
//    Button_pic = new Mybutton(LoginWidetP);
//    Button_pic->setGeometry(300,30,100,30);
//    Button_pic->setStyleSheet(QStringLiteral("background-color: rgb(165,165,165);"));
//    Button_pic->setText(tr("截 屏"));
//    Button_pic->show();
//    connect(Button_pic, SIGNAL(clicked()), this, SLOT(pressButton_pic()));
//----------------------------------------------------------------------------------------------------------------------//
    QLabel *Label_Login = new QLabel(LoginWidetP);
    Label_Login->setGeometry(15, 435, 600, 30);
    QString login = AglDeviceP->GetConfig("Login_text");
    Label_Login->setText(login);
//----------------------------------------------------------------------------------------------------------// 登录界面键盘
    MyGroup *group = new MyGroup(LoginWidetP);
    group->setGeometry(145, 180, 510, 208);
    group->setStyleSheet(QStringLiteral("background-color: rgb(0, 0, 0);"));

    for(int i = 0; i < 26; i++)
    {
        SelectKey[i] = 'a' + i;
        pushButton[i] = new Mybutton(group);
        pushButton[i]->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
        pushButton[i]->setText(QString(SelectKey[i]));
    }
    pushButton[0]->setGeometry(QRect(36, 90, 40, 30));
    pushButton[1]->setGeometry(QRect(260, 130, 40, 30));
    pushButton[2]->setGeometry(QRect(160, 130, 40, 30));
    pushButton[3]->setGeometry(QRect(136, 90, 40, 30));
    pushButton[4]->setGeometry(QRect(110, 50, 40, 30));
    pushButton[5]->setGeometry(QRect(186, 90, 40, 30));
    pushButton[6]->setGeometry(QRect(236, 90, 40, 30));
    pushButton[7]->setGeometry(QRect(286, 90, 40, 30));
    pushButton[8]->setGeometry(QRect(360, 50, 40, 30));
    pushButton[9]->setGeometry(QRect(336, 90, 40, 30));
    pushButton[10]->setGeometry(QRect(386, 90, 40, 30));
    pushButton[11]->setGeometry(QRect(436, 90, 40, 30));
    pushButton[12]->setGeometry(QRect(360, 130, 40, 30));
    pushButton[13]->setGeometry(QRect(310, 130, 40, 30));
    pushButton[14]->setGeometry(QRect(410, 50, 40, 30));
    pushButton[15]->setGeometry(QRect(460, 50, 40, 30));
    pushButton[16]->setGeometry(QRect(10, 50, 40, 30));
    pushButton[17]->setGeometry(QRect(160, 50, 40, 30));
    pushButton[18]->setGeometry(QRect(86, 90, 40, 30));
    pushButton[19]->setGeometry(QRect(210, 50, 40, 30));
    pushButton[20]->setGeometry(QRect(310, 50, 40, 30));
    pushButton[21]->setGeometry(QRect(210, 130, 40, 30));
    pushButton[22]->setGeometry(QRect(60, 50, 40, 30));
    pushButton[23]->setGeometry(QRect(110, 130, 40, 30));
    pushButton[24]->setGeometry(QRect(260, 50, 40, 30));
    pushButton[25]->setGeometry(QRect(60, 130, 40, 30));

    pushButton_Up = new Mybutton(group);
    pushButton_Up->setGeometry(QRect(10, 130, 40, 30));
    QString Up0 = AglDeviceP->GetPasswd("@pix_up_0");
    QPixmap pix_up(Up0);
    pushButton_Up->setIcon(pix_up);
    pushButton_Up->setIconSize(QSize(40,25));
    pushButton_Up->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_0 = new Mybutton(group);
    pushButton_0->setGeometry(QRect(410, 130, 40, 30));
    pushButton_0->setText(tr("0"));
    pushButton_0->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_Space = new Mybutton(group);
    pushButton_Space->setGeometry(QRect(100, 170, 260, 30));
    pushButton_Space->setText(tr(" "));
    pushButton_Space->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_Del = new Mybutton(group);
    pushButton_Del->setGeometry(QRect(10, 170, 80, 30));
    pushButton_Del->setText(tr("Delete"));
    pushButton_Del->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_dot = new Mybutton(group);
    pushButton_dot->setGeometry(QRect(460, 130, 40, 30));
    pushButton_dot->setText(".");
    pushButton_dot->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_sure = new Mybutton(group);
    pushButton_sure->setGeometry(QRect(420, 170, 80, 30));
    pushButton_sure->setText(tr("OK"));
    pushButton_sure->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_star = new Mybutton(group);
    pushButton_star->setGeometry(QRect(370, 170, 40, 30));
    pushButton_star->setText(tr("*"));
    pushButton_star->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_7 = new Mybutton(group);
    pushButton_7->setGeometry(QRect(336, 10, 40, 30));
    pushButton_7->setText(tr("7"));
    pushButton_7->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_2 = new Mybutton(group);
    pushButton_2->setGeometry(QRect(86, 10, 40, 30));
    pushButton_2->setText(tr("2"));
    pushButton_2->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_9 = new Mybutton(group);
    pushButton_9->setGeometry(QRect(436, 10, 40, 30));
    pushButton_9->setText(tr("9"));
    pushButton_9->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_3 = new Mybutton(group);
    pushButton_3->setGeometry(QRect(136, 10, 40, 30));
    pushButton_3->setText(tr("3"));
    pushButton_3->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_4 = new Mybutton(group);
    pushButton_4->setGeometry(QRect(186, 10, 40, 30));
    pushButton_4->setText(tr("4"));
    pushButton_4->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_8 = new Mybutton(group);
    pushButton_8->setGeometry(QRect(386, 10, 40, 30));
    pushButton_8->setText(tr("8"));
    pushButton_8->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_5 = new Mybutton(group);
    pushButton_5->setGeometry(QRect(236, 10, 40, 30));
    pushButton_5->setText(tr("5"));
    pushButton_5->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_1 = new Mybutton(group);
    pushButton_1->setGeometry(QRect(36, 10, 40, 30));
    pushButton_1->setText(tr("1"));
    pushButton_1->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_6 = new Mybutton(group);
    pushButton_6->setGeometry(QRect(286, 10, 40, 30));
    pushButton_6->setText(tr("6"));
    pushButton_6->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    connect(pushButton_sure, SIGNAL(clicked()), this, SLOT(showButton_sure()),Qt::UniqueConnection);
    connect(pushButton_Del, SIGNAL(clicked()), this, SLOT(presspushButton_del()),Qt::UniqueConnection);
    connect(pushButton_Up, SIGNAL(clicked()), this, SLOT(pressKeyButton_up()),Qt::UniqueConnection);
    connect(pushButton_star, SIGNAL(clicked()), this, SLOT(pressKeyButton_star()),Qt::UniqueConnection);
    connect(pushButton_dot, SIGNAL(clicked()), this, SLOT(pressKeyButton_dot()),Qt::UniqueConnection);
    connect(pushButton_Space, SIGNAL(clicked()), this, SLOT(pressKeyButton_space()),Qt::UniqueConnection);
    connect(pushButton_0, SIGNAL(clicked()), this, SLOT(pressKeyButton_0()),Qt::UniqueConnection);
    connect(pushButton_1, SIGNAL(clicked()), this, SLOT(pressKeyButton_1()),Qt::UniqueConnection);
    connect(pushButton_2, SIGNAL(clicked()), this, SLOT(pressKeyButton_2()),Qt::UniqueConnection);
    connect(pushButton_3, SIGNAL(clicked()), this, SLOT(pressKeyButton_3()),Qt::UniqueConnection);
    connect(pushButton_4, SIGNAL(clicked()), this, SLOT(pressKeyButton_4()),Qt::UniqueConnection);
    connect(pushButton_5, SIGNAL(clicked()), this, SLOT(pressKeyButton_5()),Qt::UniqueConnection);
    connect(pushButton_6, SIGNAL(clicked()), this, SLOT(pressKeyButton_6()),Qt::UniqueConnection);
    connect(pushButton_7, SIGNAL(clicked()), this, SLOT(pressKeyButton_7()),Qt::UniqueConnection);
    connect(pushButton_8, SIGNAL(clicked()), this, SLOT(pressKeyButton_8()),Qt::UniqueConnection);
    connect(pushButton_9, SIGNAL(clicked()), this, SLOT(pressKeyButton_9()),Qt::UniqueConnection);
    connect(pushButton[0], SIGNAL(clicked()), this, SLOT(pressKeyButton_a()),Qt::UniqueConnection);
    connect(pushButton[1], SIGNAL(clicked()), this, SLOT(pressKeyButton_b()),Qt::UniqueConnection);
    connect(pushButton[2], SIGNAL(clicked()), this, SLOT(pressKeyButton_c()),Qt::UniqueConnection);
    connect(pushButton[3], SIGNAL(clicked()), this, SLOT(pressKeyButton_d()),Qt::UniqueConnection);
    connect(pushButton[4], SIGNAL(clicked()), this, SLOT(pressKeyButton_e()),Qt::UniqueConnection);
    connect(pushButton[5], SIGNAL(clicked()), this, SLOT(pressKeyButton_f()),Qt::UniqueConnection);
    connect(pushButton[6], SIGNAL(clicked()), this, SLOT(pressKeyButton_g()),Qt::UniqueConnection);
    connect(pushButton[7], SIGNAL(clicked()), this, SLOT(pressKeyButton_h()),Qt::UniqueConnection);
    connect(pushButton[8], SIGNAL(clicked()), this, SLOT(pressKeyButton_i()),Qt::UniqueConnection);
    connect(pushButton[9], SIGNAL(clicked()), this, SLOT(pressKeyButton_j()),Qt::UniqueConnection);
    connect(pushButton[10], SIGNAL(clicked()), this, SLOT(pressKeyButton_k()),Qt::UniqueConnection);
    connect(pushButton[11], SIGNAL(clicked()), this, SLOT(pressKeyButton_l()),Qt::UniqueConnection);
    connect(pushButton[12], SIGNAL(clicked()), this, SLOT(pressKeyButton_m()),Qt::UniqueConnection);
    connect(pushButton[13], SIGNAL(clicked()), this, SLOT(pressKeyButton_n()),Qt::UniqueConnection);
    connect(pushButton[14], SIGNAL(clicked()), this, SLOT(pressKeyButton_o()),Qt::UniqueConnection);
    connect(pushButton[15], SIGNAL(clicked()), this, SLOT(pressKeyButton_p()),Qt::UniqueConnection);
    connect(pushButton[16], SIGNAL(clicked()), this, SLOT(pressKeyButton_q()),Qt::UniqueConnection);
    connect(pushButton[17], SIGNAL(clicked()), this, SLOT(pressKeyButton_r()),Qt::UniqueConnection);
    connect(pushButton[18], SIGNAL(clicked()), this, SLOT(pressKeyButton_s()),Qt::UniqueConnection);
    connect(pushButton[19], SIGNAL(clicked()), this, SLOT(pressKeyButton_t()),Qt::UniqueConnection);
    connect(pushButton[20], SIGNAL(clicked()), this, SLOT(pressKeyButton_u()),Qt::UniqueConnection);
    connect(pushButton[21], SIGNAL(clicked()), this, SLOT(pressKeyButton_v()),Qt::UniqueConnection);
    connect(pushButton[22], SIGNAL(clicked()), this, SLOT(pressKeyButton_w()),Qt::UniqueConnection);
    connect(pushButton[23], SIGNAL(clicked()), this, SLOT(pressKeyButton_x()),Qt::UniqueConnection);
    connect(pushButton[24], SIGNAL(clicked()), this, SLOT(pressKeyButton_y()),Qt::UniqueConnection);
    connect(pushButton[25], SIGNAL(clicked()), this, SLOT(pressKeyButton_z()),Qt::UniqueConnection);
    connect(combox, SIGNAL(activated()), this, SLOT(pressbox_Lan()),Qt::UniqueConnection);
    LoginWidetP->show();
    group->show();

    switch(AglDeviceP->Error_Code)
    {
        case Error_OK:
        {
            qDebug() << "self check ok";
            break;
        }
        case Error_SPI:
        {
            qDebug() << "Error_SPI is Error";
            QMessageBox *msgBox_Check = new QMessageBox(LoginWidetP);
            msgBox_Check->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox_Check->setText(AglDeviceP->GetConfig("Message_SPIErr"));
            msgBox_Check->setStandardButtons(QMessageBox::NoButton);
            msgBox_Check->setIcon(QMessageBox :: Critical);
            msgBox_Check->exec();
            break;
        }
        case Error_X:
        {
            qDebug() << "Error_X is Error";
            QMessageBox *msgBox_Check = new QMessageBox(LoginWidetP);
            msgBox_Check->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox_Check->setText(AglDeviceP->GetConfig("Message_XErr"));
            msgBox_Check->setStandardButtons(QMessageBox::NoButton);
            msgBox_Check->setIcon(QMessageBox :: Critical);
            msgBox_Check->exec();
            break;
        }
        case Error_Y:
        {
            qDebug() << "Error_Y is Error";
            QMessageBox *msgBox_Check = new QMessageBox(LoginWidetP);
            msgBox_Check->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox_Check->setText(AglDeviceP->GetConfig("Message_YErr"));
            msgBox_Check->setStandardButtons(QMessageBox::NoButton);
            msgBox_Check->setIcon(QMessageBox :: Critical);
            msgBox_Check->exec();
            break;
        }
        case Error_Y_Max:
        {
            qDebug() << "Error_Y max is Error";
            QMessageBox *msgBox_Check = new QMessageBox(LoginWidetP);
            msgBox_Check->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox_Check->setText(AglDeviceP->GetConfig("Message_YMaxErr"));
            msgBox_Check->setStandardButtons(QMessageBox::NoButton);
            msgBox_Check->setIcon(QMessageBox :: Critical);
            msgBox_Check->exec();
            break;
        }
        case Error_Chip:
        {
            qDebug() << "Error_Chip max is Error";
            QMessageBox *msgBox_Check = new QMessageBox(LoginWidetP);
            msgBox_Check->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox_Check->setText(AglDeviceP->GetConfig("Message_ChipErr"));
            msgBox_Check->setStandardButtons(QMessageBox::NoButton);
            msgBox_Check->setIcon(QMessageBox :: Critical);
            msgBox_Check->exec();
            break;
        }
        default:
        {
            qDebug() << "Error_Com max is Error";
            QMessageBox *msgBox_Check = new QMessageBox(LoginWidetP);
            msgBox_Check->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox_Check->setText(AglDeviceP->GetConfig("Message_NoCom"));
            msgBox_Check->setStandardButtons(QMessageBox::NoButton);
            msgBox_Check->setIcon(QMessageBox :: Critical);
            msgBox_Check->exec();
            break;
        }
    }
}

void PctGuiThread::Delayms(uint del)        // 延时函数
{
    QTime now = QTime :: currentTime().addMSecs(del);
    while(QTime :: currentTime() < now);
}

void PctGuiThread::UiTimeOut()                   // 进度条处理函数
{
       static int count = 0;
       if(UiType == StarBmpUi)                          // 开机进度条
       {
           count += 5;
           if(count<=100)
           {
               ProgressBar->setValue(count);    //设置进度条当前值
               MainWindowP->show();
               MainWindowP->repaint();            // 重新绘制窗口界面(界面刷新)
           }
           else
           {
               count=0;
               this->UiTimer->stop();
               LoginWindow();
           }
       }

       if(UiType == TesTResui)                          //  测试中进度条
       {
           Count_test += 1;
           if(Count_test <= 100)
           {
               if((int)AglDeviceP->AglDev.ChxVol == 4)
               {
                   if(Count_test == 70)
                   {
                       qDebug() << "Need stop Read Check Data";
                       check_type = Check_NoNeed;
                   }
               }
               else if((int)AglDeviceP->AglDev.ChxVol == 1)
               {
                   if(Count_test == 65)
                   {
                       qDebug() << "Need stop Read Check Data";
                       check_type = Check_NoNeed;
                   }
               }

               TestBar->setValue(Count_test);
           }
           else
           {
               qDebug("<><><><><><><> Door_State = %d",Door_State);
               Count_test = 0;
               UiTimer->stop();
//               //------------------------------Add----07-11-14--------Record time-----------------------------------//
//               QString TimeData = "Test Timer stop Time:      ";
//               QString Surrenttime = QTime::currentTime().toString("hh:mm:ss.zzz");
//               TimeData += Surrenttime + "\r\n===========================================================\r\n";
//               RecordTime(TimeData);
//                qDebug() << "Test timer stop need record time ";
//               //----------------------------------------------------------------------------------------------------------------//
//               if(3 == Door_State)
               if(0)
               {
                    return;
               }
               else
               {
                   check_type = Check_OutDoor;
                   CheckTime->start(500);
                   isBarCodeGetOK = false;
                   bool iOut = AglDeviceP->agl_out();
                   if(false == iOut)
                   {
                       return;
                   }
                   isStart = false;
                   isCount = false;

                   isBarCodeGetOK = false;
                   isClickedCloseDoor = false;

                   Radio_after->setDisabled(false);
                   Radio_now->setDisabled(false);
                   ResWidget->show();
                   Door_State = 3;
                   QString resre = AglDeviceP->GetConfig("res_station");
                   Label_station->setText(resre);

                   if(isYmaxError == true)
                   {
                       isMessage = true;
                       msgBox = new QMessageBox(MainWindowP);
                       msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                       msgBox->setText(AglDeviceP->GetConfig("Message_YMaxErr"));
                       msgBox->setIcon(QMessageBox ::Critical);
                       msgBox->exec();
                       delete msgBox;
                       if(isWidget == false)
                       {
                           Voice_pressKey();
                       }
                       isMessage = false;
                       AglDeviceP->Error_Code = Error_OK;
                       check_type = Check_NoNeed;
                       return;
                   }
//---Add------liuyouwei-----------------------------------------------------------------------------------------------------//
                   QString Model = QString :: number((int)AglDeviceP->AglDev.ChxVol, 10);
                   int iInsert = Sql->CountResultSql();
                   if(-1 == iInsert)
                   {
                       qDebug() << "5997 Count Result DB ERROR";
                       return;
                   }
                   QDateTime time = QDateTime :: currentDateTime();
                   QString now = time.toString("yyyy-MM-dd hh:mm:ss") + ";" + Model;
                   QString gp;                  
 //-----------------------------------------------------------------------------------------------------------------------------//    数据库插入
//                   double ImageY[4][2000];
                   int16_t ImageY[4][2000];
                   if((int)AglDeviceP->AglDev.ChxVol == 4)
                   {
                       gp = "GP";
                       //-----------------Add----------------------//
                       double *DatP0 = &AglDeviceP->AglDev.VolBuf[0][0];
                       double *DatP1 = &AglDeviceP->AglDev.VolBuf[1][0];
                       double *DatP2 = &AglDeviceP->AglDev.VolBuf[2][0];
                       double *DatP3 = &AglDeviceP->AglDev.VolBuf[3][0];
                       for(int i = 0; i < 2000; i++)
                       {
                           ImageY[0][i] =  (int16_t)DatP0[i];  //PG1
                           ImageY[1][i] =  (int16_t)DatP1[i];  //PG2
                           ImageY[2][i] =  (int16_t)DatP2[i];  //Hp
                           ImageY[3][i] =  (int16_t)DatP3[i];  //G17
                       }
                       //--------------------------------------------//
                   }
                   else if((int)AglDeviceP->AglDev.ChxVol == 1)
                   {
                       //--------------Add--------------//
                       double * DatP = &AglDeviceP->AglDev.VolBuf[0][0];
                       for(int i = 0; i < 2000; i++)
                       {
                           ImageY[0][i] = DatP[i];
                       }
                       //---------------------------------//
                       gp = AnaFound.XiangMuPm[0].xiangmu;
                   }
                   RES_Data.Id = iInsert;
                   QString Test_time = AglDeviceP->GetConfig("Print_time");
                   sprintf(dat, "%-13.13s", Test_time.toLatin1().data());
                   QString nowdata = dat;

                   sprintf(dat, "%-14.14s", time.toString("yyyy-MM-dd").toLatin1().data());
                   QString nowtime = dat;
                   nowtime += time.toString("hh:mm");
                   nowdata += nowtime;
                   QString LastData = TextShow + "///" +  "\r\n" + nowdata + Str_line + TC_Value +  "///" + Str_Flag + "\r\n";

                   strcpy(RES_Data.SamId, SamIDShow.toLatin1().data());
                   strcpy(RES_Data.Item,gp.toLatin1().data());
                   strcpy(RES_Data.Porperty, CSV_information.toLatin1().data());
                   strcpy(RES_Data.Date, now.toLatin1().data());
                   strcpy(RES_Data.Report, LastData.toLatin1().data());
                   memcpy(RES_Data.DatBuf,(char *)(&ImageY[0][0]),8000 * sizeof(int16_t));

                   if(Sql->isLimit == true)
                   {
                       isMessage = true;
                       msgBox = new QMessageBox(ResWidget);
                       msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                       msgBox->setText(AglDeviceP->GetConfig("Message_memory"));
                       msgBox->setIcon(QMessageBox :: Critical);
                       msgBox->exec();
                       if(isWidget == false)
                       {
                           Voice_pressKey();
                       }
                       delete msgBox;
                       isMessage = false;
                       return;
                   }
                   else
                   {
//                       qDebug() << "AglDeviceP->Error_Code is " << AglDeviceP->Error_Code;
                       if(!Sql->InsertResultSql(&RES_Data))
                       {
                          qDebug() << "6028 Inert Result DB ERROR";
                          return;
                       }
                       if(AglDeviceP->GetPasswd("@QRcode") != "YES")
                       {
                           if(!Sql->UpdateBatUseTime(Sam, ++BatchTimes))
                           {
                               qDebug() << "Update Error";
                           }
                       }
                   }
                   //----------------------------------------------------------------------------------------------------------------------------//
                   char CR = 13;  //CR
                   char EB = 28;  //FS
                   QString Select_ID = SamIDShow;
                   SEND_Sam = GetMSH("QRY^Q02") + GetQRD(Select_ID) + GetQRF() + (QString)EB + (QString)CR;
                  if(AglDeviceP->GetPasswd("@AUTO") == "YES" && LisSocket->state() == 3 && isConnect == true)                   //  向LIS服务器发送样本申请信息
                  {
                      qint16 Sam_len =  LisSocket->write(SEND_Sam.toLatin1().data(), SEND_Sam.length());
                      LisSocket->waitForBytesWritten(1000);
                      qDebug() << "Send lengh is " << Sam_len;
                      QByteArray Read_Sam_Data;
                      LisSocket->waitForReadyRead(1000);
                      Read_Sam_Data = LisSocket->readAll();
                      QString all_SamData = Read_Sam_Data.data();
                      qDebug() << "Read data is " << all_SamData;
                  }
                   if((int)AglDeviceP->AglDev.ChxVol == 4)
                   {
                       //GetOBX 入参分别是
                       QString All_OBX = GetOBX(OBX_4, OBX_5, OBX_6, OBX_7, OBX_8) + GetOBX(OBX_4_1, OBX_5_1, OBX_6_1, OBX_7_1, OBX_8_1)
                       + GetOBX(OBX_4_b, OBX_5_b, OBX_6_b, OBX_7_b, OBX_8_b) + GetOBX(OBX_4_2, OBX_5_2, OBX_6_2, OBX_7_2, OBX_8_2)
                       + GetOBX(OBX_4_3, OBX_5_3, OBX_6_3, OBX_7_3, OBX_8_3);

                       SEND_Res  = GetMSH("ORU^R01") + GetPID() + GetOBR() + All_OBX + (QString)EB + (QString)CR;
                       if(AglDeviceP->GetPasswd("@AUTO") == "YES" && isYmaxError != true)
                       {
                           if(LisSocket->state() == 3 && isConnect == true)                                                                    //    向LIS服务器发送测试结果
                           {
                               qint16 Datalen =  LisSocket->write(SEND_Res.toLatin1().data(), SEND_Res.length());
                               if(Datalen != SEND_Res.length())                                                                                             //  如果发送出去的数据长度不等于要发送的长度
                               {
                                   isUpload = false;
                                   isMessage = true;
                                   msgBox = new QMessageBox(MainWindowP);
                                   msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                                   msgBox->setText(AglDeviceP->GetConfig("Message_Auto"));
                                   msgBox->setIcon(QMessageBox :: Critical);
                                   msgBox->exec();
                                   delete msgBox;
                                   if(isWidget == false)
                                   {
                                       Voice_pressKey();
                                   }
                                   isMessage = false;
                                   return;
                               }
                               LisSocket->waitForBytesWritten(100);

                               QByteArray ReadData;
                               LisSocket->waitForReadyRead(100);
                               ReadData = LisSocket->readAll();
                               QString allData = ReadData.data();
                               QStringList Read_list = allData.split((QString)CR);

                               int Len_CR = Read_list.length();
                               if(4 != Len_CR)
                               {
                                   isUpload = false;
                                   isMessage = true;
                                   msgBox = new QMessageBox(MainWindowP);
                                   msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                                   msgBox->setText(AglDeviceP->GetConfig("Message_Uperr"));
                                   msgBox->setIcon(QMessageBox :: Critical);
                                   msgBox->exec();
                                   delete msgBox;
                                   if(isWidget == false)
                                   {
                                       Voice_pressKey();
                                   }
                                   isMessage = false;
                                   return;
                               }
                               allData = Read_list.at(1);
                               Read_list = allData.split("|");
                               int Len_MA = Read_list.length();
                               if(8 != Len_MA)
                               {
                                   isUpload = false;
                                   isMessage = true;
                                   msgBox = new QMessageBox(MainWindowP);
                                   msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                                   msgBox->setText(AglDeviceP->GetConfig("Message_Uperr"));
                                   msgBox->setIcon(QMessageBox :: Critical);
                                   msgBox->exec();
                                   delete msgBox;
                                   isMessage = false;
                                   if(isWidget == false)
                                   {
                                       Voice_pressKey();
                                   }
                                   return;
                               }
                              if(Read_list.at(1) == "AA" && Read_list.at(3) == "Message accepted" && Read_list.at(6) == "0")    // 判断消息确认中的关键字，判断消息是否正常发送
                               {
                                   isUpload= true;
                                   LisSocket->disconnectFromHost();
                                   LisSocket->close();
                                   qDebug() << "四联数据发送完成，断开网络连接 5249";
                               }
                               else
                               {
                                  isMessage = true;
                                   isUpload = false;
                                   msgBox = new QMessageBox(MainWindowP);
                                   msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                                   msgBox->setText(AglDeviceP->GetConfig("Message_Uperr"));
                                   msgBox->setIcon(QMessageBox :: Critical);
                                   msgBox->exec();
                                   delete msgBox;
                                   isMessage = false;
                                   if(isWidget == false)
                                   {
                                       Voice_pressKey();
                                   }
                                   return;
                               }
                           }
                           else if(LisSocket->state() != 3 || isConnect == false)                                                          // 如果当前与服务器连接失败
                           {
                               isUpload = false;
                               isMessage = true;
                               msgBox = new QMessageBox(MainWindowP);
                               msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                               msgBox->setText(AglDeviceP->GetConfig("Message_NoAuto"));
                               msgBox->setIcon(QMessageBox :: Critical);
                               msgBox->exec();
                               delete msgBox;
                               if(isWidget == false)
                               {
                                   Voice_pressKey();
                               }
                               isMessage = false;
                               return;
                           }
                        }
                   }                  
                   else if((int)AglDeviceP->AglDev.ChxVol == 1)
                   {
                       QString All_OBX = GetOBX(OBX_4, OBX_5, OBX_6, OBX_7, OBX_8);
                       SEND_Res = GetMSH("ORU^R01") + GetPID() + GetOBR() + All_OBX + (QString)EB + (QString)CR;
                       if(AglDeviceP->GetPasswd("@AUTO") == "YES" || isYmaxError != true)
                       {
                           if(LisSocket->state() == 3 && isConnect == true)
                           {
                               qint16 Datalen =  LisSocket->write(SEND_Res.toLatin1().data(), SEND_Res.length());
                               if(Datalen != SEND_Res.length())
                               {
                                   qDebug() << "Send Error";
                                   isUpload = false;
                                   isMessage = true;
                                   msgBox = new QMessageBox(MainWindowP);
                                   msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                                   msgBox->setText(AglDeviceP->GetConfig("Message_Auto"));
                                   msgBox->setIcon(QMessageBox :: Critical);
                                   msgBox->exec();
                                   delete msgBox;
                                   isMessage = false;
                                   if(isWidget == false)
                                   {
                                       Voice_pressKey();
                                   }
                                   return;
                               }

                               LisSocket->waitForBytesWritten(1000);
                               QByteArray ReadData;
                               LisSocket->waitForReadyRead(1000);
                               ReadData = LisSocket->readAll();
                               QString allData = ReadData.data();
                               QStringList Read_list = allData.split((QString)CR);
                               qDebug() << "second read is " << allData;
                               int Len_CR = Read_list.length();
                               qDebug() << "second read lengh is " << Len_CR;
                               if(4 != Len_CR)
                               {
                                   qDebug() << "此处有错误 5305";
                                   isUpload = false;
                                   isMessage = true;
                                   msgBox = new QMessageBox(MainWindowP);
                                   msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                                   msgBox->setText(AglDeviceP->GetConfig("Message_Uperr"));
                                   msgBox->setIcon(QMessageBox :: Critical);
                                   msgBox->exec();
                                   delete msgBox;
                                   isMessage = false;
                                   if(isWidget == false)
                                   {
                                       Voice_pressKey();
                                   }
                                   return;
                               }
                               allData = Read_list.at(1);
                               Read_list = allData.split("|");
                               int Len_MA = Read_list.length();
                               if(8 != Len_MA)
                               {
                                   qDebug() << "此处有错误 5320";
                                   isUpload = false;
                                   isMessage = true;
                                   msgBox = new QMessageBox(MainWindowP);
                                   msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                                   msgBox->setText(AglDeviceP->GetConfig("Message_Uperr"));
                                   msgBox->setIcon(QMessageBox :: Critical);
                                   msgBox->exec();
                                   delete msgBox;
                                   isMessage = false;
                                   if(isWidget == false)
                                   {
                                       Voice_pressKey();
                                   }
                                   return;
                               }
                               if(Read_list.at(1) == "AA" && Read_list.at(3) == "Message accepted" && Read_list.at(6) == "0")
                               {
                                   isUpload = true;
                                   LisSocket->disconnectFromHost();
                                   LisSocket->close();
                                   qDebug() << "单联数据发送完成，断开网络连接 line is 5349";
                               }
                               else
                               {
                                   isUpload = false;
                                   isMessage = true;
                                   msgBox = new QMessageBox(MainWindowP);
                                   msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                                   msgBox->setText(AglDeviceP->GetConfig("Message_Uperr"));
                                   msgBox->setIcon(QMessageBox :: Critical);
                                   msgBox->exec();
                                   delete msgBox;
                                   isMessage = false;
                                   if(isWidget == false)
                                   {
                                       Voice_pressKey();
                                   }
                                   return;
                               }
                           }
                           else if(LisSocket->state() != 3 || isConnect == false)
                           {
                               isUpload = false;
                               isMessage = true;
                               msgBox = new QMessageBox(MainWindowP);
                               msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                               msgBox->setText(AglDeviceP->GetConfig("Message_NoAuto"));
                               msgBox->setIcon(QMessageBox :: Critical);
                               msgBox->exec();
                               delete msgBox;
                               if(isWidget == false)
                               {
                                   Voice_pressKey();
                               }
                               isMessage = false;
                               return;
                           }
                       }
                   }
//---------------------------------------------------------------------------------------------------------//
               }
           }
       }
}

void PctGuiThread :: CreatSelectKey()               //  查询界面键盘
{
    UiType = SelectKeyui;
    KeyStation = 0;
    SelecKeyWidget = new QWidget(Group_Res);
    SelecKeyWidget->setStyle(QStyleFactory::create("fusion"));
    SelecKeyWidget->setGeometry(140, 105, 510, 238);
    SelecKeyWidget->setStyleSheet(QStringLiteral("background-color: rgb(123, 139, 202);"));
    for(int i = 0; i < 26; i++)
    {
        SelectKey[i] = 'a' + i;
        pushButton[i] = new Mybutton(SelecKeyWidget);
        pushButton[i]->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
        pushButton[i]->setText(QString(SelectKey[i]));
    }
    pushButton[0]->setGeometry(QRect(36, 120, 40, 30));
    pushButton[1]->setGeometry(QRect(260, 160, 40, 30));
    pushButton[2]->setGeometry(QRect(160, 160, 40, 30));
    pushButton[3]->setGeometry(QRect(136, 120, 40, 30));
    pushButton[4]->setGeometry(QRect(110, 80, 40, 30));
    pushButton[5]->setGeometry(QRect(186, 120, 40, 30));
    pushButton[6]->setGeometry(QRect(236, 120, 40, 30));
    pushButton[7]->setGeometry(QRect(286, 120, 40, 30));
    pushButton[8]->setGeometry(QRect(360, 80, 40, 30));
    pushButton[9]->setGeometry(QRect(336, 120, 40, 30));
    pushButton[10]->setGeometry(QRect(386, 120, 40, 30));
    pushButton[11]->setGeometry(QRect(436, 120, 40, 30));
    pushButton[12]->setGeometry(QRect(360, 160, 40, 30));
    pushButton[13]->setGeometry(QRect(310, 160, 40, 30));
    pushButton[14]->setGeometry(QRect(410, 80, 40, 30));
    pushButton[15]->setGeometry(QRect(460, 80, 40, 30));
    pushButton[16]->setGeometry(QRect(10, 80, 40, 30));
    pushButton[17]->setGeometry(QRect(160, 80, 40, 30));
    pushButton[18]->setGeometry(QRect(86, 120, 40, 30));
    pushButton[19]->setGeometry(QRect(210, 80, 40, 30));
    pushButton[20]->setGeometry(QRect(310, 80, 40, 30));
    pushButton[21]->setGeometry(QRect(210, 160, 40, 30));
    pushButton[22]->setGeometry(QRect(60, 80, 40, 30));
    pushButton[23]->setGeometry(QRect(110, 160, 40, 30));
    pushButton[24]->setGeometry(QRect(260, 80, 40, 30));
    pushButton[25]->setGeometry(QRect(60, 160, 40, 30));

    pushButton_Up = new Mybutton(SelecKeyWidget);
    pushButton_Up->setGeometry(QRect(10, 160, 40, 30));
    QString Up0 = AglDeviceP->GetPasswd("@pix_up_0");
    QPixmap pix_up(Up0);
    pushButton_Up->setIcon(pix_up);
    pushButton_Up->setIconSize(QSize(40,25)); 
    pushButton_Up->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_0 = new Mybutton(SelecKeyWidget);
    pushButton_0->setGeometry(QRect(410, 160, 40, 30));
    pushButton_0->setText(tr("0"));
    pushButton_0->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_dot = new Mybutton(SelecKeyWidget);
    pushButton_dot->setGeometry(QRect(460, 160, 40, 30));
    pushButton_dot->setText(tr("-"));
    pushButton_dot->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    Mybutton *pushButton_Space = new Mybutton(SelecKeyWidget);
    pushButton_Space->setGeometry(QRect(150, 200, 210, 30));
    pushButton_Space->setText(tr(" "));
    pushButton_Space->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
//--------Add-----------------------------------------------------------------------------------------------------------------------------//
    Mybutton *pushButton_Mao = new Mybutton(SelecKeyWidget);
    pushButton_Mao->setGeometry(QRect(100, 200, 40, 30));
    pushButton_Mao->setText(tr(":"));
    pushButton_Mao->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
//------------------------------------------------------------------------------------------------------------------------------------------//
    Mybutton *pushButton_del = new Mybutton(SelecKeyWidget);
    pushButton_del->setGeometry(QRect(10, 200, 80, 30));
    pushButton_del->setText("Delete");
    pushButton_del->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_sure = new Mybutton(SelecKeyWidget);
    pushButton_sure->setGeometry(QRect(420, 200, 80, 30));
    pushButton_sure->setText(tr("OK"));
    pushButton_sure->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_star = new Mybutton(SelecKeyWidget);
    pushButton_star->setGeometry(QRect(370, 200, 40, 30));
    pushButton_star->setText(tr("*"));
    pushButton_star->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_7 = new Mybutton(SelecKeyWidget);
    pushButton_7->setGeometry(QRect(336, 40, 40, 30));
    pushButton_7->setText(tr("7"));
    pushButton_7->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_2 = new Mybutton(SelecKeyWidget);
    pushButton_2->setGeometry(QRect(86, 40, 40, 30));
    pushButton_2->setText(tr("2"));
    pushButton_2->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_9 = new Mybutton(SelecKeyWidget);
    pushButton_9->setGeometry(QRect(436, 40, 40, 30));
    pushButton_9->setText(tr("9"));
    pushButton_9->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_3 = new Mybutton(SelecKeyWidget);
    pushButton_3->setGeometry(QRect(136, 40, 40, 30));
    pushButton_3->setText(tr("3"));
    pushButton_3->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_4 = new Mybutton(SelecKeyWidget);
    pushButton_4->setGeometry(QRect(186, 40, 40, 30));
    pushButton_4->setText(tr("4"));
    pushButton_4->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_8 = new Mybutton(SelecKeyWidget);
    pushButton_8->setGeometry(QRect(386, 40, 40, 30));
    pushButton_8->setText(tr("8"));
    pushButton_8->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_5 = new Mybutton(SelecKeyWidget);
    pushButton_5->setGeometry(QRect(236, 40, 40, 30));
    pushButton_5->setText(tr("5"));
    pushButton_5->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_1 = new Mybutton(SelecKeyWidget);
    pushButton_1->setGeometry(QRect(36, 40, 40, 30));
    pushButton_1->setText(tr("1"));
    pushButton_1->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));
    Mybutton *pushButton_6 = new Mybutton(SelecKeyWidget);
    pushButton_6->setGeometry(QRect(286, 40, 40, 30));
    pushButton_6->setText(tr("6"));
    pushButton_6->setStyleSheet(QStringLiteral("background-color: rgb(251, 251, 251);"));

    QFont Font;
    Font.setPointSize(12);
    QLabel *label = new QLabel(SelecKeyWidget);
    label->setObjectName(QStringLiteral("label"));
    label->setGeometry(QRect(4.5, 5, 500, 35));
    label->setAlignment(Qt :: AlignCenter);
    QString search = AglDeviceP->GetConfig("Result_key");
    label->setText(search);
    label->setFont(Font);

    connect(pushButton_sure, SIGNAL(clicked()), this, SLOT(CloseKey()),Qt::UniqueConnection);
    connect(pushButton_Mao, SIGNAL(clicked()), this, SLOT(presspushButton_Mao()),Qt::UniqueConnection);
    connect(pushButton_del, SIGNAL(clicked()), this, SLOT(presspushButton_del()),Qt::UniqueConnection);
    connect(pushButton_Up, SIGNAL(clicked()), this, SLOT(pressKeyButton_up()),Qt::UniqueConnection);
    connect(pushButton_star, SIGNAL(clicked()), this, SLOT(pressKeyButton_star()),Qt::UniqueConnection);
    connect(pushButton_dot, SIGNAL(clicked()), this, SLOT(pressKeyButton_dot()),Qt::UniqueConnection);
    connect(pushButton_Space, SIGNAL(clicked()), this, SLOT(pressKeyButton_space()),Qt::UniqueConnection);
    connect(pushButton_0, SIGNAL(clicked()), this, SLOT(pressKeyButton_0()),Qt::UniqueConnection);
    connect(pushButton_1, SIGNAL(clicked()), this, SLOT(pressKeyButton_1()),Qt::UniqueConnection);
    connect(pushButton_2, SIGNAL(clicked()), this, SLOT(pressKeyButton_2()),Qt::UniqueConnection);
    connect(pushButton_3, SIGNAL(clicked()), this, SLOT(pressKeyButton_3()),Qt::UniqueConnection);
    connect(pushButton_4, SIGNAL(clicked()), this, SLOT(pressKeyButton_4()),Qt::UniqueConnection);
    connect(pushButton_5, SIGNAL(clicked()), this, SLOT(pressKeyButton_5()),Qt::UniqueConnection);
    connect(pushButton_6, SIGNAL(clicked()), this, SLOT(pressKeyButton_6()),Qt::UniqueConnection);
    connect(pushButton_7, SIGNAL(clicked()), this, SLOT(pressKeyButton_7()),Qt::UniqueConnection);
    connect(pushButton_8, SIGNAL(clicked()), this, SLOT(pressKeyButton_8()),Qt::UniqueConnection);
    connect(pushButton_9, SIGNAL(clicked()), this, SLOT(pressKeyButton_9()),Qt::UniqueConnection);
    connect(pushButton[0], SIGNAL(clicked()), this, SLOT(pressKeyButton_a()),Qt::UniqueConnection);
    connect(pushButton[1], SIGNAL(clicked()), this, SLOT(pressKeyButton_b()),Qt::UniqueConnection);
    connect(pushButton[2], SIGNAL(clicked()), this, SLOT(pressKeyButton_c()),Qt::UniqueConnection);
    connect(pushButton[3], SIGNAL(clicked()), this, SLOT(pressKeyButton_d()),Qt::UniqueConnection);
    connect(pushButton[4], SIGNAL(clicked()), this, SLOT(pressKeyButton_e()),Qt::UniqueConnection);
    connect(pushButton[5], SIGNAL(clicked()), this, SLOT(pressKeyButton_f()),Qt::UniqueConnection);
    connect(pushButton[6], SIGNAL(clicked()), this, SLOT(pressKeyButton_g()),Qt::UniqueConnection);
    connect(pushButton[7], SIGNAL(clicked()), this, SLOT(pressKeyButton_h()),Qt::UniqueConnection);
    connect(pushButton[8], SIGNAL(clicked()), this, SLOT(pressKeyButton_i()),Qt::UniqueConnection);
    connect(pushButton[9], SIGNAL(clicked()), this, SLOT(pressKeyButton_j()),Qt::UniqueConnection);
    connect(pushButton[10], SIGNAL(clicked()), this, SLOT(pressKeyButton_k()),Qt::UniqueConnection);
    connect(pushButton[11], SIGNAL(clicked()), this, SLOT(pressKeyButton_l()),Qt::UniqueConnection);
    connect(pushButton[12], SIGNAL(clicked()), this, SLOT(pressKeyButton_m()),Qt::UniqueConnection);
    connect(pushButton[13], SIGNAL(clicked()), this, SLOT(pressKeyButton_n()),Qt::UniqueConnection);
    connect(pushButton[14], SIGNAL(clicked()), this, SLOT(pressKeyButton_o()),Qt::UniqueConnection);
    connect(pushButton[15], SIGNAL(clicked()), this, SLOT(pressKeyButton_p()),Qt::UniqueConnection);
    connect(pushButton[16], SIGNAL(clicked()), this, SLOT(pressKeyButton_q()),Qt::UniqueConnection);
    connect(pushButton[17], SIGNAL(clicked()), this, SLOT(pressKeyButton_r()),Qt::UniqueConnection);
    connect(pushButton[18], SIGNAL(clicked()), this, SLOT(pressKeyButton_s()),Qt::UniqueConnection);
    connect(pushButton[19], SIGNAL(clicked()), this, SLOT(pressKeyButton_t()),Qt::UniqueConnection);
    connect(pushButton[20], SIGNAL(clicked()), this, SLOT(pressKeyButton_u()),Qt::UniqueConnection);
    connect(pushButton[21], SIGNAL(clicked()), this, SLOT(pressKeyButton_v()),Qt::UniqueConnection);
    connect(pushButton[22], SIGNAL(clicked()), this, SLOT(pressKeyButton_w()),Qt::UniqueConnection);
    connect(pushButton[23], SIGNAL(clicked()), this, SLOT(pressKeyButton_x()),Qt::UniqueConnection);
    connect(pushButton[24], SIGNAL(clicked()), this, SLOT(pressKeyButton_y()),Qt::UniqueConnection);
    connect(pushButton[25], SIGNAL(clicked()), this, SLOT(pressKeyButton_z()),Qt::UniqueConnection);

    SelecKeyWidget->show();
}

void PctGuiThread::showButton_0()
{
    Voice_pressKey();
    if(UiType == InforKeyui)
    {
        if(Edit_Mea->text().isEmpty())
        {
            return;
        }
        text = Edit_Mea->text() + "0";
    }
    else
    {
        if(Edit_pla->text().isEmpty())
        {
            return;
        }
        text = Edit_pla->text() + "0";
    }
    int Leng = text.length();
    if(Leng == 0)
    {
        return;
    }
    if(Leng == 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == InforKeyui)
    {
        Edit_Mea->setText(text);
    }
    else
    {
        Edit_pla->setText(text);
    }
    text.clear();
}

void PctGuiThread::showButton_1()
{
    Voice_pressKey();
    if(UiType == InforKeyui)
    {
        text = Edit_Mea->text() + "1";
    }
    else
    {
        text = Edit_pla->text() + "1";
    }
    int Leng = text.length();
    if(Leng == 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == InforKeyui)
    {
        Edit_Mea->setText(text);
    }
    else
    {
        Edit_pla->setText(text);
    }
    text.clear();
}

void PctGuiThread::showButton_2()
{
    Voice_pressKey();
    if(UiType == InforKeyui)
    {
        text = Edit_Mea->text() + "2";
    }
    else
    {
        text = Edit_pla->text() + "2";
    }
    int Leng = text.length();
    if(Leng == 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == InforKeyui)
    {
        Edit_Mea->setText(text);
    }
    else
    {
        Edit_pla->setText(text);
    }
    text.clear();
}

void PctGuiThread::showButton_3()
{
    Voice_pressKey();
    if(UiType == InforKeyui)
    {
        text = Edit_Mea->text() + "3";
    }
    else
    {
        text = Edit_pla->text() + "3";
    }
    int Leng = text.length();
    if(Leng == 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        };
        isMessage = false;
        return;
    }
    if(UiType == InforKeyui)
    {
        Edit_Mea->setText(text);
    }
    else
    {
        Edit_pla->setText(text);
    }
    text.clear();
}

void PctGuiThread::showButton_4()
{
    Voice_pressKey();
    if(UiType == InforKeyui)
    {
        text = Edit_Mea->text() + "4";
    }
    else
    {
        text = Edit_pla->text() + "4";
    }
    int Leng = text.length();
    if(Leng == 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == InforKeyui)
    {
        Edit_Mea->setText(text);
    }
    else
    {
        Edit_pla->setText(text);
    }
    text.clear();
}

void PctGuiThread::showButton_5()
{
    Voice_pressKey();
    if(UiType == InforKeyui)
    {
        text = Edit_Mea->text() + "5";
    }
    else
    {
        text = Edit_pla->text() + "5";
    }
    int Leng = text.length();
    if(Leng == 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == InforKeyui)
    {
        Edit_Mea->setText(text);
    }
    else
    {
        Edit_pla->setText(text);
    }
    text.clear();
}

void PctGuiThread::showButton_6()
{
    Voice_pressKey();
    if(UiType == InforKeyui)
    {
        text = Edit_Mea->text() + "6";
    }
    else
    {
        text = Edit_pla->text() + "6";
    }
    int Leng = text.length();
    if(Leng == 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = true;
        return;
    }
    if(UiType == InforKeyui)
    {
        Edit_Mea->setText(text);
    }
    else
    {
        Edit_pla->setText(text);
    }
    text.clear();
}

void PctGuiThread::showButton_7()
{
    Voice_pressKey();
    if(UiType == InforKeyui)
    {
        text = Edit_Mea->text() + "7";
    }
    else
    {
        text = Edit_pla->text() + "7";
    }
    int Leng = text.length();
    if(Leng == 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == InforKeyui)
    {
        Edit_Mea->setText(text);
    }
    else
    {
        Edit_pla->setText(text);
    }
    text.clear();
}

void PctGuiThread::showButton_8()
{
    Voice_pressKey();
    if(UiType == InforKeyui)
    {
        text = Edit_Mea->text() + "8";
    }
    else
    {
        text = Edit_pla->text() + "8";
    }
    int Leng = text.length();
    if(Leng == 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == InforKeyui)
    {
        Edit_Mea->setText(text);
    }
    else
    {
        Edit_pla->setText(text);
    }
    text.clear();
}

void PctGuiThread::showButton_9()
{
    Voice_pressKey();
    if(UiType == InforKeyui)
    {
        text = Edit_Mea->text() + "9";
    }
    else
    {
        text = Edit_pla->text() + "9";
    }
    int Leng = text.length();
    if(Leng == 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(UiType == InforKeyui)
    {
        Edit_Mea->setText(text);
    }
    else
    {
        Edit_pla->setText(text);
    }
    text.clear();
}

void PctGuiThread::showButton_clear()
{
    Voice_pressKey();
    if(UiType == TestNumui)
    {
        text = Edit_pla->text();
        int Leng = text.length();
        text = text.remove(Leng - 1, 1);
        Edit_pla->setText(text);
    }
    else if(UiType == InforKeyui)
    {
        text = Edit_Mea->text();
        int Leng = text.length();
        text = text.remove(Leng - 1, 1);
        Edit_Mea->setText(text);
    }
    else if(UiType == Usrkeyui || UiType == Passui|| UiType == NewPassui || UiType == NewPassui2)
    {
        text = Edit_key->text();
        int lengh = KeyInput.length();
        KeyInput = KeyInput.remove(lengh - 1, 1);
        Edit_key->setText(KeyInput);
    }
}

void PctGuiThread::showButton_sure()
{
    Voice_pressKey();
    QString User = combox->currentText();
    QString Root = AglDeviceP->GetPasswd("@Pass1");
    QString Pass;

    if(UiType == TestNumui)
    {
        TestNumKey->close();
        text.clear();
        UiType = Testui;
        Radio_Pla1->setEnabled(true);
        Radio_Pla2->setEnabled(true);
    }
    else if(UiType == InforKeyui)
    {
        TimeNumKey->close();
        text.clear();
        isClicked = false;
        UiType = Inforui;
        Radio_after->setEnabled(true);
        Radio_now->setEnabled(true);
    }
    else if(UiType == LoginUi)
    {
        if(LineEdit_input->text().isEmpty())
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_empty"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
        Pass = AglDeviceP->GetPasswd(User);
        if(User == "Admin")
        {
            if(LineEdit_input->text() == Pass || LineEdit_input->text() == Root)
            {
                LoginUserName = combox->currentText();
//                AglDeviceP->agl_rd_stat();
//                Door_flush = (int)AglDeviceP->AglDev.OutIntState;
                LoginWidetP->close();
                KeyInput.clear();
                CreatWindow();
                isAdmin = true;
            }
            else
            {
                qDebug() << "creat Message box";
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("LoginMessage_err"));
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                KeyInput.clear();
                LineEdit_input->clear();
                isMessage = false;
                qDebug() << "Creat Box Over";
                return;
            }
        }
        else
        {
            if(LineEdit_input->text() == Pass)
            {
                LoginUserName = combox->currentText();
//                AglDeviceP->agl_rd_stat();
//                Door_flush = (int)AglDeviceP->AglDev.OutIntState;
                LoginWidetP->close();
                KeyInput.clear();
                CreatWindow();
                isAdmin = false;
            }
            else
            {
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("LoginMessage_err"));
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                KeyInput.clear();
                LineEdit_input->clear();
                isMessage = false;
                return;
            }
        }
    }
    else if(UiType == PassKeyui)
    {
        if(LineEdit_input->text().isEmpty())
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("LoginMessage_err"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            return;
        }
        iRow = Table_Use->currentRow();                                       //    获取Table选中的行号
        Table_Use->item(iRow, 1)->setText(text);
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_change"));
        msgBox->setIcon(QMessageBox :: Information);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
            PassKeyWidget->close();
        }
    }
    else if(UiType == NewKeyui)
    {
        if(text.isEmpty())
        {
            return;
        }
        Pass_New[iCount_User] = text;
        PassEdit->clear();
        text.clear();

        iCount_User = Table_Use->rowCount();                                               //  获得当前表格行数
        Table_Use->setRowCount(iCount_User + 1);                                       //  在获得的行数下面子新增一行
        Table_Use->setRowHeight(iCount_User, 40);                                      //  设置新增行的高度
        Item_User[iCount_User] = new QTableWidgetItem(User_New[iCount_User]);
        Item_Pass[iCount_User] = new QTableWidgetItem(Pass_New[iCount_User]);
        Item_User[iCount_User]->setTextAlignment(Qt::AlignCenter);
        Item_Pass[iCount_User]->setTextAlignment(Qt::AlignCenter);
        Table_Use->setItem(iCount_User, 0, Item_User[iCount_User]);
        Table_Use->setItem(iCount_User, 1, Item_Pass[iCount_User]);
        iCount_User++;
        NewKeyWidget->close();
    }
    else if(UiType == Testui)                                       // 样本ID输入处理
    {
        if(text.isEmpty())
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_empty"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
                text.clear();
            }
            return;
        }
        else
        {
            KeyWidget->hide();
            Edit_key->clear();
            int iCount = Table_Sam->rowCount();                                          //  获得当前表格行数
            Table_Sam->setRowCount(iCount + 1);                                       //  在获得的行数下面子新增一行
            Table_Sam->setRowHeight(iCount, 40);                                      //  设置新增行的高度
            Text_ID[iCount] = text;
            item_sam[iCount] = new QTableWidgetItem(Text_ID[iCount]);

            for(int i = iCount; i > 0; i--)
            {
                item_sam[i]->setText(item_sam[i - 1]->text());
            }
            item_sam[0]->setText(text);
            Table_Sam->setItem(iCount, 0, item_sam[iCount]);
            Table_Sam->repaint();
            iCount_ID = iCount + 1;
            text.clear();
        }
    }
    else if(UiType == NewPassui)
    {
        if(KeyInput.isEmpty())
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_empty"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
        Pass1 = Edit_key->text();
        UiType = NewPassui2;
        QString agin = AglDeviceP->GetConfig("Label_Pass3");
        Ltext->setText(agin);
        Edit_key->clear();
        KeyInput.clear();
    }
    else if(UiType == NewPassui2)
    {
        Pass2 = Edit_key->text();
        if(iTime < TIMES)
        {
            if(Pass1 == Pass2)
            {
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("Message_change"));
                msgBox->setIcon(QMessageBox :: Information);
                msgBox->exec();
                delete msgBox;
                isMessage = false;
                if(isWidget == false)
                {
                    Voice_pressKey();
                    Edit_key->clear();
                    KeyInput.clear();
                    QString accoun = AglDeviceP->GetConfig("Label_Pass1");
                    Ltext->setText(accoun);
                    Group_Key->hide();
                }
            }
            else
            {
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("Message_nosame"));
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                isMessage = false;
                if(isWidget == false)
                {
                    Voice_pressKey();
                    KeyInput.clear();
                    Edit_key->clear();
    //                iTime++;
                }
            }
        }
        else
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_24"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
                Group_Key->hide();
            }
        }
    }
    else if(UiType == Passui)               // 从升级界面点击OK
    {
        if(iTime < TIMES)
        {
            if(KeyInput.isEmpty())
            {
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("Message_empty"));
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                isMessage = false;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                return;
            }
            Pass = AglDeviceP->GetPasswd(User);
            if(User == "Admin")
            {
                if(Edit_key->text() == Pass || Edit_key->text() == Root)
                {
                    Group_Key->close();
                    CreatUpdate();
                    KeyInput.clear();
                    Edit_key->clear();
                }
                else
                {
                    isMessage = true;
                    msgBox = new QMessageBox(MainWindowP);
                    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    msgBox->setText(AglDeviceP->GetConfig("LoginMessage_err"));
                    msgBox->setIcon(QMessageBox :: Critical);
                    msgBox->exec();
                    delete msgBox;
                    isMessage = false;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                        KeyInput.clear();
                        Edit_key->clear();
                        //                    iTime++;
                    }
                    return;
                }
            }
        }
        else
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_24"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
                Group_Key->hide();
            }
        }
     }
    else if(UiType == Usrkeyui)
    {           
        if(iTime < TIMES)
        {
            if(KeyInput.isEmpty())
            {
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("Message_empty"));
                msgBox->setIcon(QMessageBox :: Warning);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                isMessage = false;
                return;
            }
            Pass = AglDeviceP->GetPasswd(User);
            if(User == "Admin")
            {
                if(Edit_key->text() == Pass || Edit_key->text() == Root)
                {
                    Group_Key->hide();
                    CreatUSERWidget();
                    KeyInput.clear();
                    Edit_key->clear();
                    Label_station->setText(AglDeviceP->GetConfig("Station_User"));
                }
                else
                {
                    isMessage = true;
                    msgBox = new QMessageBox(MainWindowP);
                    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    msgBox->setText(AglDeviceP->GetConfig("LoginMessage_err"));
                    msgBox->setIcon(QMessageBox :: Critical);
                    msgBox->exec();
                    delete msgBox;
                    isMessage = false;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                        KeyInput.clear();
                        Edit_key->clear();
    //                    iTime++;
                    }
                    return;
                }
            }
        }
        else
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_24"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
                Group_Key->hide();
            }
        }
    }
}
void PctGuiThread :: pressretuButton()
{
    Voice_pressKey();
    qDebug() << "ret button uitype is " << UiType;
    if(UiType == Selectui)
    {
        QTableWidgetItem *Item_Sha[10];
        if(isSelectRes == true)                     // 从按条件查询显示部分结果界面退回到所有结果显示
        {
            int iRow = Table_Res->rowCount();
            for(int i = 0; i < iRow; i++)
            {
                Table_Res->removeRow(0);
            }
            iSearCh = Sql->CountResultSql();
            if(-1 == iSearCh)
            {
                qDebug() << "6939 Count Result DB ERROR";
                return;
            }
            double time = (double)iSearCh / (double)10;
            PageAll = iSearCh / 10;

            if(time > PageAll)
            {
                PageAll += 1;
            }
            if(PageAll == 0)
            {
                PageAll = 1;
            }
            PageNow = 1;
            QString allpage = QString :: number(PageAll, 10);
            QString nowpage = QString :: number(PageNow, 10);
            if(iSearCh > 10)
            {
                iShow = 10;
            }
            else
            {
                iShow = iSearCh;
            }
            if(iSearCh > 4)
            {
                Table_Res->setGeometry(133.5,101,565,179);
                Table_Res->setRowCount(iShow);
                Table_Res->setColumnCount(1);
                Table_Res->setColumnWidth(0, 563);
            }
            else
            {
                Table_Res->setGeometry(133.5,101,535,179);
                Table_Res->setRowCount(iShow);
                Table_Res->setColumnCount(1);
                Table_Res->setColumnWidth(0, 533);
            }
            for(int j = 0; j < iShow; j++)
            {
                Table_Res->setRowHeight(j, 36);                      // 设置第j行单元格宽度
            }
            struct ResultIndex RES_LIST;
            QString DateDate;
            QStringList DateList;
            QString ite_lise, Show_item;

            for(int  i = 1; i <= iShow; i++)
            {
                if(!Sql->ListResultSql(RES_LIST, i))
                {
                    qDebug() << "6992 List Result DB ERROR";
                    return;
                }
                Show_item.clear();
                ite_lise = QString::number(i);
                sprintf(dat, "%-8.8s", ite_lise.toLatin1().data());
                Show_item += (QString)dat;

                sprintf(dat, "%-16.16s", RES_LIST.SamId.toLatin1().data());
                Show_item += (QString)dat;

                DateList = RES_LIST.Date.split(";");
                DateDate = DateList.at(0);
                sprintf(dat, "%-19.19s", DateDate.toLatin1().data());
                Show_item += (QString)dat;

                Item_Sha[i-1] = new QTableWidgetItem;
                Item_Sha[i-1]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                Item_Sha[i-1]->setText(Show_item);
                Table_Res->setItem(i-1, 0, Item_Sha[i-1]);
            }
            isSelectRes = false;
            Label_cou->show();
            But_PgDown->show();
            But_PgUp->show();
            Label_cou->clear();
            Label_cou->setText(nowpage + "/" + allpage +  " " + AglDeviceP->GetConfig("Label_Page"));
            Label_station->clear();
            return;
        }
        Edit_Sel->clear();
        Group_Res->close();
        KeyInput.clear();
        Label_station->setText(AglDeviceP->GetConfig("station_Main"));
        UiType = MainUi;
        qDebug() << "Group is close";
    }
    else if(UiType == SelectKeyui)
    {
        if(isClicked == true)
        {
            return;
        }
        SelecKeyWidget->close();
//        Table_Res->show();
        UiType = Selectui;
    }
}

void PctGuiThread :: pressBut_Inf_restore()
{
    Voice_pressKey();
    isMessage = true;
    msgBox = new QMessageBox(MainWindowP);
    msgBox->setText(AglDeviceP->GetConfig("Message_restore"));
    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
    msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::No);
    msgBox->setIcon(QMessageBox :: Question);
    int ret = msgBox->exec();
    switch (ret)
    {
        case QMessageBox::Yes:
        {
            Voice_pressKey();
            AglDeviceP->SysCall("rm -rf *.db *.bak");
            QString OtherData =  AglDeviceP->GetAllParadata();
            QString AllData = "Admin=123#\r\n" + OtherData;

            if(!AglDeviceP->DeleteFile("password.txt"))
            {
                return;
            }
            if(!AglDeviceP->WriteTxtToFile("password.txt", AllData))
            {
                return;
            }
            delete msgBox;
            AglDeviceP->ReBoot();
            break;
        }
        case QMessageBox::No:
        {
            if(isWidget == false)
            {
               Voice_pressKey();
            }
            delete msgBox;
            break;
        }
    }
    isMessage = false;
}

void PctGuiThread :: pressBut_Inf_ret()
{
    Voice_pressKey();
    if(UiType == Selectui)
    {
//        Group_Infor->hide();
        Group_Infor->close();
        Label_station->setText(AglDeviceP->GetConfig("Station_Select"));
    }
    if(UiType == COSui)
    {
        Group_SetInfor->hide();
        Label_station->setText(AglDeviceP->GetConfig("Station_Select"));
    }
}

void PctGuiThread :: pressButton_Usr_ret()
{
    Voice_pressKey();
    if(UiType == Usrui)
    {
        UsrWidget->hide();
        COSWidget->show();
        UiType = COSui;
    }
    else if(UiType == Usrkeyui || UiType == Passui)
    {
        KeyInput.clear();
        Edit_key->clear();
        Group_Key->hide();
        UiType = Usrui;
    }
    else if(UiType == NewPassui2 || UiType == NewPassui)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_nosuccess"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Warning);
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                Voice_pressKey();
                Group_Key->hide();
                text.clear();
                Ltext->clear();
                QString account = AglDeviceP->GetConfig("Label_Pass1");
                Ltext->setText(account);
                break;
            }
            case QMessageBox::No:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                msgBox->close();
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
}

void PctGuiThread :: pressButton_dev()
{
    qDebug() << "show Device Management widget";
    Voice_pressKey();
    int iCout = box_Lan->count();
    while(iCout--)
    {
        box_Lan->removeItem(0);
    }
    iEditTime = 0;
    iDate = 0;
    Label_station->setText(AglDeviceP->GetConfig("station_Dev"));

    QDate Date = QDate :: currentDate();
    QTime Time = QTime :: currentTime();
    QString cosedip = AglDeviceP->GetPasswd("@hostIP");
    Edit_IP->setText(cosedip);
    QString lisedip = AglDeviceP->GetPasswd("@LisIP");
    Edit_LisIP->setText(lisedip);
    QString cosedpo = AglDeviceP->GetPasswd("@LisPort");
    Edit_LisPort->setText(cosedpo);

    Edit_data->setDisplayFormat("yyyy-MM-dd");
    Edit_data->setDate(Date);
    Edit_time->setTime(Time);
//---------------------------new language display-------------------------------//
    QString ReadLangeuage = AglDeviceP->GetConfig("Reader_Language");
    QStringList LanguageList = ReadLangeuage.split("///");
    DefaultLanguage = LanguageList.at(0);
    int Num_Language = LanguageList.length();
    if(Num_Language < 2)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText("config file Loss or Error!");
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    for(int i = 0; i < Num_Language; i++)
    {
        box_Lan->addItem(LanguageList.at(i), i);
    }
//----------------------------old language display---------------------------------//
//    QString coseditla1 = AglDeviceP->GetConfig("COS_Lan1");
//    QString coseditla2 = AglDeviceP->GetConfig("COS_Lan2");
//    if(AglDeviceP->GetPasswd("@Languag") == "English")
//    {
//        DefaultLanguage = coseditla1;
//        box_Lan->addItem(coseditla1, 0);
//        box_Lan->addItem(coseditla2, 1);
//    }
//    else
//    {
//        DefaultLanguage = coseditla2;
//        box_Lan->addItem(coseditla2, 0);
//        box_Lan->addItem(coseditla1, 1);
//    }
//-----------------------------------------------------------------------------------------//
    if(AglDeviceP->GetPasswd("@AUTO") == "YES")
    {
        Box_Send->setChecked(true);
    }
    else
    {
        Box_Send->setChecked(false);
    }
    if(AglDeviceP->GetPasswd("@QRcode") == "YES")
    {
        Box_QRcode->setChecked(true);
    }
    else
    {
        Box_QRcode->setChecked(false);
    }

    QString AUTO_sta;
    if(Box_Send->isChecked())
    {
        AUTO_sta = "Checked";
    }
    else
    {
        AUTO_sta = "DisChecked";
    }

    QString QRcode_sta;
    if(Box_QRcode->isChecked())
    {
        QRcode_sta = "USE";
    }
    else
    {
        QRcode_sta = "NOUSE";
    }
    OldUserData = Edit_IP->text()+Edit_LisIP->text()+Edit_LisPort->text()+Edit_data->text()+Edit_time->text()+box_Lan->currentText()+AUTO_sta+QRcode_sta;
    Group_COS->show();
}

void PctGuiThread :: pressButton_set()
{
    Voice_pressKey();
    CreatCOSWidget();
}

void PctGuiThread :: pressButton_1()
{
    if(UiType == MainUi)
    {
        Voice_pressKey();
        if(isControl == true || isClicked == true)
        {
            return;
        }
        //-----------------Add-------------------------//
        if(!Sql->init_BatchSql("Main"))
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText("init_BatchSql SQL Table Error!");
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            return;
        }
        if(!Sql->init_ResultSql())
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText("init_ResultSql SQL Table Error!");
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }

        int iData = Sql->CountResultSql();
        if(-1 == iData)
        {
            qDebug() << "7243 Count Result DB ERROR";
            isClicked = false;
            return;
        }
        if(0 == iData)
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_nodata"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isClicked = false;
            isMessage = false;
            return;
        }
        CreatPatientWidget();
        Label_station->setText(AglDeviceP->GetConfig("station_Result"));
        isClicked = false;

//        CreatSelecWidget();
//        Label_station->setText(AglDeviceP->GetConfig("Station_Select"));
        //-----------------------------------------------//
    }
//    else if(UiType == Selectui)
//    {
//        Voice_pressKey();
//        if(isClicked == true)
//        {
//            return;
//        }
//        isClicked = true;
//        int iData = Sql->CountResultSql();
//        if(-1 == iData)
//        {
//            qDebug() << "7243 Count Result DB ERROR";
//            isClicked = false;
//            return;
//        }
//        if(0 == iData)
//        {
//            isMessage = true;
//            msgBox = new QMessageBox(MainWindowP);
//            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//            msgBox->setText(AglDeviceP->GetConfig("Message_nodata"));
//            msgBox->setIcon(QMessageBox :: Warning);
//            msgBox->exec();
//            delete msgBox;
//            if(isWidget == false)
//            {
//                Voice_pressKey();
//            }
//            isClicked = false;
//            isMessage = false;
//            return;
//        }
//        CreatPatientWidget();
//        Label_station->setText(AglDeviceP->GetConfig("station_Result"));
//        isClicked = false;
//    }
}

void PctGuiThread :: pressButton_2()
{   
    Voice_pressKey();
    if(UiType == MainUi)
    {
        if(isControl == true || isClicked == true)
        {
            return;
        }
        //-----------------------Add--------------------------------//
        if(!Sql->init_BatchSql("Main"))
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText("Creat Batch Table Error");
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
        if(!Sql->init_ResultSql())
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText("Creat Resule Table Error");
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
        CreatSetWidget();
        Label_station->setText(AglDeviceP->GetConfig("Station_Select"));
        //------------------------------------------------------------//
    }
    else if(UiType == Setui)
    {
        SetWidget->hide();
        Widget_num->show();
        QString lot_sta = AglDeviceP->GetConfig("station_Lot");
        Label_station->setText(lot_sta);
        if(isBatchError == true)
        {
            Voice_ScanError();
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_nores"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
    }
    else if(UiType == COSui)
    {
        COSWidget->hide();
        UsrWidget->show(); 
        UiType = Usrui;
    }
    else if(UiType == Selectui)
    {
//        Group_Infor->show();
        Label_station->setText(AglDeviceP->GetConfig("QC_Station"));
        CreatQCWidget();
    }
}

void PctGuiThread::CalibrationDoor()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        QString loginerr = AglDeviceP->GetConfig("Message_TC_contrl");
        msgBox->setText(loginerr);
        msgBox->setWindowTitle("Warning");
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    if(isControl == true)
    {
        return;
    }
    if(2 == state_door)
    {
        check_type = Check_OutDoor;
        CheckTime->start(500);
        isBarCodeGetOK = false;
        if(AglDeviceP->agl_out())
        {
            state_door = 3;
        }
        else
        {
            return;
        }
    }
    else
    {
        check_type = Check_InDoor;
        CheckTime->start(900);
        if(AglDeviceP->agl_int())
        {
            state_door = 2;
        }
        else
        {
            return;
        }
    }
}

void PctGuiThread :: pressButton_Door()
{
    Voice_pressKey();
    if(isCanOpenDoor == true)
    {
        qDebug() << "isCanOpenDoor is true";
        if(isControl == true)
        {
            qDebug() << "isControl is true";
            return;
        }
        if(2 == Door_flush)
        {
            qDebug() << "2 == Door_flush";
            check_type = Check_OutDoor;
            CheckTime->start(500);
            isBarCodeGetOK = false;
            if(AglDeviceP->agl_out())
            {
                Door_State = 3;
            }
        }
        else if(3 == Door_flush)
        {
            qDebug() << "3 == Door_flush";
            check_type = Check_InDoor;
            CheckTime->start(900);
            if(AglDeviceP->agl_int())
            {
                Door_State = 2;
            }
        }
        isDoorClicked = true;
    }
}

void PctGuiThread :: pressButt_Home()
{
    Voice_pressKey();    
    QString CurrUSRdata;
    int iLow = Table_Use->rowCount();
//    for(int i = 1; i < iLow; i++)
    for(int i = 0; i < iLow; i++)
    {
        CurrUSRdata += Table_Use->item(i, 0)->text() + "," + Table_Use->item(i, 1)->text();
    }
    if(OldUserData != CurrUSRdata && isSave == false)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_save"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Question);
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                Voice_pressKey();
                QString USRmessage;
                int iRow = Table_Use->rowCount();
//                for(int i = 1; i < iRow; i++)
                for(int i = 0; i < iRow; i++)
                {
                    USRmessage += Table_Use->item(i, 0)->text() + "=" + Table_Use->item(i, 1)->text() + "#\r\n";
                }
                QString Other = AglDeviceP->GetAllParadata();
                USRmessage += Other;

                if(!AglDeviceP->DeleteFile("password.txt"))
                {
                    qDebug() << "Delete file error!";
                }
                if(!AglDeviceP->WriteTxtToFile("password.txt", USRmessage))
                {
                    qDebug() << "Creat file error!";
                }
                if(!AglDeviceP->ReloadPasswdFile(USRmessage))
                {
                    qDebug() << "Reload Password file Error!";
                }
                isSave = true;
                break;
            }
            case QMessageBox::No:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                msgBox->close();
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
    if(isWidget == false)
    {
        KeyInput.clear();
        USERWidget->close();
        UsrWidget->close();
        COSWidget->close();
        SetWidget->close();
        QString main = AglDeviceP->GetConfig("station_Main");
        Label_station->setText(main);
        UiType = MainUi;
    }
}

void PctGuiThread :: pressButton_Lot_Home()
{
    Voice_pressKey();
    SetWidget->close();
    Widget_num->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    UiType = MainUi;
}

void PctGuiThread :: pressUSR_Home()
{
    Voice_pressKey();
    KeyInput.clear();
    UsrWidget->close();
    COSWidget->close();
    SetWidget->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    isSaveDev = false;
    UiType = MainUi;
}

void PctGuiThread :: Press_Set_home()
{
    Voice_pressKey();
    KeyInput.clear();
    COSWidget->close();
    SetWidget->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    isSaveDev = false;
    UiType = MainUi;
}

void PctGuiThread :: pressDev_Home()
{
    Voice_pressKey();
    if(UiType == Usrui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2 )
    {
        UsrWidget->close();
    }
    else if(UiType == Usrkeyui)
    {
        KeyInput.clear();
        Edit_key->clear();
        Group_Key->close();
        UsrWidget->close();
    }
    QString AUTO_sta;
    if(Box_Send->isChecked())
    {
        AUTO_sta = "Checked";
    }
    else
    {
        AUTO_sta = "DisChecked";
    }

    QString QRCode_sta;
    if(Box_QRcode->isChecked())
    {
        QRCode_sta = "USE";
    }
    else
    {
        QRCode_sta = "NOUSE";
    }
    QString CurrentData = Edit_IP->text()+Edit_LisIP->text()+Edit_LisPort->text()+Edit_data->text()+Edit_time->text()+box_Lan->currentText()+AUTO_sta+QRCode_sta;
    if(OldUserData != CurrentData && isSaveDev != true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_SaDev"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Question);
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                Voice_pressKey();
//                if(isIPAddrErr == true)
//                {
//                    isMessage_child = true;
//                    msgBox_child = new QMessageBox(MainWindowP);
//                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//                    msgBox_child->setText("The content you entered is incorrect and can not be saved!");
//                    msgBox_child->setIcon(QMessageBox :: Warning);
//                    msgBox_child->exec();
//                    delete msgBox_child;
//                    Voice_pressKey();
//                    isMessage_child = false;
//                    isMessage = false;
//                    COSWidget->close();
//                    SetWidget->close();
//                    QString main = AglDeviceP->GetConfig("station_Main");
//                    Label_station->setText(main);
//                    isSaveDev = false;
//                    UiType = MainUi;
//                    return;
                //------------------------------------------------------------------------------------------------//
                bool ok;
                QString IPAddr = Edit_IP->text();
                QStringList IPList = IPAddr.split(".");
                if(IPList.length() != 4)
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    isMessage_child = false;
                    isMessage = false;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                        COSWidget->close();
                        SetWidget->close();
                        QString main = AglDeviceP->GetConfig("station_Main");
                        Label_station->setText(main);
                        isSaveDev = false;
                        UiType = MainUi;
                    }
                    return;
                }
                int IP1 = IPList.at(0).toInt(&ok, 10);
                int IP2 = IPList.at(1).toInt(&ok, 10);
                int IP3 = IPList.at(2).toInt(&ok, 10);
                int IP4 = IPList.at(3).toInt(&ok, 10);

                if((IPList.at(0).left(1) == "0" &&  IPList.at(0).length() != 1)|| (IPList.at(1).left(1) == "0" &&  IPList.at(1).length() != 1) || (IPList.at(2).left(1) == "0" &&  IPList.at(2).length() != 1) || (IPList.at(3).left(1) == "0" &&  IPList.at(3).length() != 1))
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    isMessage_child = false;
                    isMessage = false;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                        COSWidget->close();
                        SetWidget->close();
                        QString main = AglDeviceP->GetConfig("station_Main");
                        Label_station->setText(main);
                        isSaveDev = false;
                        UiType = MainUi;
                    }
                    return;
                }
                if(IP1 > 255 || IP2 > 255 || IP3 > 255 || IP4 > 255)
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    isMessage_child = false;
                    isMessage = false;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                        COSWidget->close();
                        SetWidget->close();
                        QString main = AglDeviceP->GetConfig("station_Main");
                        Label_station->setText(main);
                        isSaveDev = false;
                        UiType = MainUi;
                    }
                    return;
                }
                if(Edit_IP->text() == Edit_LisIP->text())
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_double");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    isMessage_child = false;
                    isMessage = false;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                        COSWidget->close();
                        SetWidget->close();
                        QString main = AglDeviceP->GetConfig("station_Main");
                        Label_station->setText(main);
                        isSaveDev = false;
                        UiType = MainUi;
                    }
                    return;
                }
                IPAddr = Edit_LisIP->text();
                IPList = IPAddr.split(".");
                if(IPList.length() != 4)
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    isMessage_child = false;
                    isMessage = false;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                        COSWidget->close();
                        SetWidget->close();
                        QString main = AglDeviceP->GetConfig("station_Main");
                        Label_station->setText(main);
                        isSaveDev = false;
                        UiType = MainUi;
                    }
                    return;
                }

                if((IPList.at(0).left(1) == "0" &&  IPList.at(0).length() != 1)|| (IPList.at(1).left(1) == "0" &&  IPList.at(1).length() != 1) || (IPList.at(2).left(1) == "0" &&  IPList.at(2).length() != 1) || (IPList.at(3).left(1) == "0" &&  IPList.at(3).length() != 1))
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    isMessage_child = false;
                    isMessage = false;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                        COSWidget->close();
                        SetWidget->close();
                        QString main = AglDeviceP->GetConfig("station_Main");
                        Label_station->setText(main);
                        isSaveDev = false;
                        UiType = MainUi;
                    }
                    return;
                }
                IP1 = IPList.at(0).toInt(&ok, 10);
                IP2 = IPList.at(1).toInt(&ok, 10);
                IP3 = IPList.at(2).toInt(&ok, 10);
                IP4 = IPList.at(3).toInt(&ok, 10);
                if(IP1 > 255 || IP2 > 255 || IP3 > 255 || IP4 > 255)
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_IPErr");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    isMessage_child = false;
                    isMessage = false;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                        COSWidget->close();
                        SetWidget->close();
                        QString main = AglDeviceP->GetConfig("station_Main");
                        Label_station->setText(main);
                        isSaveDev = false;
                        UiType = MainUi;
                    }
                    return;
                }

                int port = Edit_LisPort->text().toInt(&ok, 10);
                if(port > 65535)
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(COSWidget);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    QString loginerr = AglDeviceP->GetConfig("Message_Lang");
                    msgBox_child->setText(loginerr);
                    msgBox_child->setWindowTitle("Warning");
                    msgBox_child->setIcon(QMessageBox :: Warning);
                    msgBox_child->exec();
                    delete msgBox_child;
                    isMessage_child = false;
                    isMessage = false;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                        COSWidget->close();
                        SetWidget->close();
                        QString main = AglDeviceP->GetConfig("station_Main");
                        Label_station->setText(main);
                        isSaveDev = false;
                        UiType = MainUi;
                    }
                    return;
                }
                //------------------------------------------------------------------------------------------------//
                SaveSet();
                isMessage_child = true;
                msgBox_child = new QMessageBox(MainWindowP);
                msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox_child->setText(AglDeviceP->GetConfig("Message_succsave"));
                msgBox_child->setIcon(QMessageBox :: Information);
                msgBox_child->exec();
                delete msgBox_child;
                isMessage_child = false;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                break;
            }
            case QMessageBox::No:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                msgBox->close();
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
    if(isWidget == true)
    {
        return;
    }
    COSWidget->close();
    SetWidget->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    isSaveDev = false;
    UiType = MainUi;
}

void PctGuiThread :: pressButton_3()
{
    Voice_pressKey();
    if(UiType == MainUi)
    {
        if(isControl == true || isClicked == true)
        {
            return;
        }
        isClicked = true;
//        door_time->stop();
//        if(2 == Door_State || 2 == Door_flush)
//        {
            check_type = Check_OutDoor;
            CheckTime->start(500);
            isBarCodeGetOK = false;
            bool iOut = AglDeviceP->agl_out();
            if(false == iOut)
            {
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText("open door Error!");
                msgBox->setIcon(QMessageBox ::Critical);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                isMessage = false;
                isClicked = false;
                return;
            }
            Door_State = 3;
//        }
        QString patient = AglDeviceP->GetConfig("Sam_sation");
        Label_station->setText(patient);
        if(!Sql->init_BatchSql("Main"))
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText("Creat Batch Table Error");
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isClicked = false;
            isMessage = false;
            return;
        }
        if(!Sql->init_ResultSql())
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText("Creat Result Table Error");
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isClicked = false;
            isMessage = false;
            return;
        }
        CreatTestWidget();
        isClicked = false;
    }
    else if(UiType == Setui)
    {
        SetWidget->close();       
        QString main = AglDeviceP->GetConfig("station_Main");
        Label_station->setText(main);
        UiType = MainUi;
    }
    else if(UiType == COSui)
    {
        COSWidget->close();
        UiType = Setui;
    }
    else if(UiType == Selectui)
    {
        SelecWidget->close();
        QString main = AglDeviceP->GetConfig("station_Main");
        Label_station->setText(main);
        KeyInput.clear();
        UiType = MainUi;
        isClicked = false;
        isSelectRes = false;
    }
    else if(UiType == SelectKeyui)
    {
        if(isClicked == true)
        {
            return;
        }
        SelecKeyWidget->close();
        SelecWidget->close();
        QString main = AglDeviceP->GetConfig("station_Main");
        Label_station->setText(main);
        UiType = MainUi;
        isClicked = false;
        isSelectRes = false;
    }
}

void PctGuiThread :: pressbutton_ret()
{
    Voice_pressKey();
    if(UiType == Testui)
    {
        KeyWidget->close();
    }
    else if(UiType == PassKeyui)
    {
        PassKeyWidget->close();
    }
    else if(UiType == NewKeyui)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_nosuccess"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Question);
        int ret = msgBox->exec();
        switch(ret)
        {
            case  QMessageBox :: Yes :
            {
                Voice_pressKey();
                NewKeyWidget->close();
                text.clear();
                PassEdit->clear();
                break;
            }
            default:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
    else
    {
        KeyWidget->close();
    }
}

void PctGuiThread :: pressButton_return()
{
    Voice_pressKey();
    Widget_num->hide();
    SetWidget->show();
    QString sett = AglDeviceP->GetConfig("Station_Select");
    Label_station->setText(sett);
    text.clear();
}

void PctGuiThread :: pressChoNumBut_ret()
{
    Voice_pressKey();
    GroupBat->close();
    GroupSam->show();
    UiType = Testui;
}

void PctGuiThread :: pressChoNumBut_net()
{
    Voice_pressKey();
    InfoWidget->show();
    GroupBat->close();
}

void PctGuiThread :: pressSamEdit()
{
    Voice_pressKey();

    if(isNext == true || isClicked == true)
    {
        return;
    }
    if( UiType == TestNumui)
    {
        TestNumKey->close();
    }
    UiType = TestKeyui;
    TestKeyWidget->show();
    pressButton_sweep();
}

void PctGuiThread :: pressSamBut_home()
{
    Voice_pressKey();
    if(isNext == true || isClicked == true)
    {
        return;
    }
    isClicked = true;
    if(3 == Door_State)
    {
        check_type = Check_InDoor;
        CheckTime->start(900);
        bool iOut = AglDeviceP->agl_int();
        if(false == iOut)
        {
            isClicked = false;
            return;
        }
        Door_State = 2;
    }
    KeyInput.clear();
    if(UiType == TestKeyui)
    {
        TestKeyWidget->close();
    }
    else if(UiType == TestNumui)
    {
        TestNumKey->close();
        Radio_Pla1->setEnabled(true);
        Radio_Pla2->setEnabled(true);
        text.clear();
    }
    TestWidget->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    UiType = MainUi;
    isClicked = false;
}

void PctGuiThread :: pressSamBut_ret()
{
    Voice_pressKey();
    if(isNext == true)
    {
        return;
    }
    if(UiType == Testui)
    {                        
        if(3 == Door_State)
        {
            if(isClicked == true)
            {
                return;
            }
            isClicked = true;
            check_type = Check_InDoor;
            CheckTime->start(900);
            bool iOut = AglDeviceP->agl_int();
            if(false == iOut)
            {
                isClicked = false;
                return;
            }
            Door_State = 2;
        }
        KeyInput.clear();
//        SamEdit->clear();
        TestWidget->close();
        QString main = AglDeviceP->GetConfig("station_Main");
        Label_station->setText(main);
        UiType = MainUi;
        isClicked = false;
    }
    else if(UiType == TestKeyui)
    {
        SamIDShow = SamEdit->text();
        TestKeyWidget->hide();
        UiType = Testui;
    }
    else if(UiType == TestNumui)
    {
        TestNumKey->close();
        Radio_Pla1->setEnabled(true);
        Radio_Pla2->setEnabled(true);
        text.clear();
        UiType = Testui;
    }
}

void PctGuiThread :: pressInfoBut_home()
{
    Voice_pressKey();
    isNext = false;
    if(isStart == true)
    {
        return;
    }
    if(isCount == false)
    {
        LCD_time->stop();
        disconnect(LCD_time, SIGNAL(timeout()), this, SLOT(LCDtimer()));
        delete LCD_time;
        KeyInput.clear();
        SamEdit->clear();
        if(UiType == LotKeyui)
        {
            CardKeyWidget->close();
            isGetBarCodeDataError = true;
        }
        InfoWidget->close();
        TestWidget->close();
        QString main = AglDeviceP->GetConfig("station_Main");
        Label_station->setText(main);
        UiType = MainUi;
        isClicked = false;
        isGetBarCodeDataError = false;
        if(isTimer_CodeStart)
        {
            qDebug() << "13050 Stop Timer";
            Timer_ScanLOT->stop();
        }
        isBarCodeGetOK = false;
        isClickedCloseDoor = false;
    }
    else
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_ret"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Question);
        LCD_time->stop();
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                Voice_pressKey();
//                LCD_time->stop();
                disconnect(LCD_time, SIGNAL(timeout()), this, SLOT(LCDtimer()));
                delete LCD_time;
                LCD_time->stop();
                KeyInput.clear();
                SamEdit->clear();
                InfoWidget->close();
                TestWidget->close();
                QString main= AglDeviceP->GetConfig("station_Main");
                Label_station->setText(main);
                UiType = MainUi;
                isCount = false;
                isStart = false;
                if(isTimer_CodeStart)
                {
                    qDebug() << "13087 Stop Timer";
                    Timer_ScanLOT->stop();
                }
                isBarCodeGetOK = false;
                isClickedCloseDoor = false;
                break;
            }
            case QMessageBox::No:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                LCD_time->start(1000);
                msgBox->close();
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
}

void PctGuiThread :: pressSamBut_next()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    CSV_information.clear();
//----------Add-------------------------------------------------------------//
    Str_Pre.clear();
    if(Radio_Pla1->isChecked())                                                             // 如果选择血清PL
    {
        Str_Pre += AglDeviceP->GetConfig("Report_PL") + ",";
        CSV_information += AglDeviceP->GetConfig("Report_PL")+"#"+" "+"#";
    }
    if(Radio_Pla2->isChecked())                                                             // 如果选择全血WB
    {
        if(Edit_pla->text().isEmpty())
        {
            Str_Pre += AglDeviceP->GetConfig("Report_WB") + "," + AglDeviceP->GetConfig("Report_HCT") + "(" + AglDeviceP->GetConfig("Report_empty") + "),";
            CSV_information += AglDeviceP->GetConfig("Report_WB") + "#" + AglDeviceP->GetConfig("Report_empty") + "#";
        }
        else
        {
            int Value_Wb = Edit_pla->text().toInt();
            if(Value_Wb < 16 || Value_Wb > 85)
            {
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("Message_WB"));
                msgBox->setIcon(QMessageBox :: Warning);
                msgBox->setModal(false);
                msgBox->setStandardButtons(QMessageBox::Ok);
                msgBox->exec();
                delete msgBox;
                isMessage = false;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                return;
            }
            Str_Pre += AglDeviceP->GetConfig("Report_WB") + "," + AglDeviceP->GetConfig("Report_HCT") + "(" + Edit_pla->text() + "%),";
            CSV_information += AglDeviceP->GetConfig("Report_WB") + "#" + Edit_pla->text() + "#";
        }
    }
    if(Radio_fasting->isChecked())
    {
        Str_Pre += AglDeviceP->GetConfig("Radio_Fasting");
        Interpretation_post = 0;
    }
    if(Radio_Post->isChecked())
    {
        Str_Pre += AglDeviceP->GetConfig("Radio_Post");
        Interpretation_post = 1;
    }
    if(Check_PPI->isChecked())
    {
        Str_Pre += "," + AglDeviceP->GetConfig("Repotr_PPI");
        CSV_information += AglDeviceP->GetConfig("Repotr_PPI") + "#";
    }
    else
    {
        CSV_information += "#";
    }
//----------------------------------------------------------------------------//
    if(UiType == Testui)
    {
        if(isNext == true)
        {
            return;
        }
        isNext = true;
        Radio_Pla1->setDisabled(true);
        Radio_Pla2->setDisabled(true);
//-------------------------------------------------------------------------------------------------------------------------------//
        if(SamEdit->text().isEmpty())               //  未输入样本ID
        {
            Voice_ScanError();
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Sam_noinput"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->setModal(false);
            msgBox->setStandardButtons(QMessageBox::Ok);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isNext = false;
            Radio_Pla1->setDisabled(false);
            Radio_Pla2->setDisabled(false);
            return;
        }
        text.clear();
        CreatInforWidget();
        Label_station->setText(AglDeviceP->GetConfig("Station_readly"));
    }
    else if(UiType == TestNumui || UiType == TestKeyui)
    {
        if(isNext == true)
        {
            return;
        }
        isNext = true;
        Radio_Pla1->setDisabled(true);
        Radio_Pla2->setDisabled(true);

        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_sam"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Question);
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                Voice_pressKey();
                if(SamEdit->text().isEmpty())
                {
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(MainWindowP);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    msgBox_child->setText(AglDeviceP->GetConfig("Message_nosam"));
                    msgBox_child->setIcon(QMessageBox :: Critical);
                    msgBox_child->exec();
                    Voice_pressKey();
                    delete msgBox_child;
                    isMessage_child = false;
                    isNext = false;
                    Radio_Pla1->setDisabled(false);
                    Radio_Pla2->setDisabled(false);
                    return;
                }
                SamIDShow = SamEdit->text();
                if(UiType == TestNumui)
                {
                    TestNumKey->close();
                }
                else
                {
                    TestKeyWidget->hide();
                }
                text.clear();
                UiType = Testui;
                CreatInforWidget();
                Label_station->setText(AglDeviceP->GetConfig("Station_readly"));
                break;
            }
            case QMessageBox::No:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                msgBox->close();
                isNext = false;
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
}

void PctGuiThread :: pressInfoBut_ret()
{
    Voice_pressKey();
    //--------------------------------Add---------------------------//
    if(UiType == InforKeyui)
    {
        TimeNumKey->close();
        isClicked = false;
        UiType = Inforui;
        Radio_after->setEnabled(true);
        Radio_now->setEnabled(true);
        return;
    }
    if(UiType == LotKeyui)
    {
        Edid_LOT->clear();
        CardKeyWidget->close();
        UiType = Inforui;
        isBarCodeGetOK = false;
        isClickedCloseDoor = false;
        isGetBarCodeDataError = true;
        return;
    }
    isNext = false;
    Radio_Pla1->setDisabled(false);
    Radio_Pla2->setDisabled(false);
    //------------------------------------------------------------//
    if(isStart == true)
    {
        qDebug() << "is start is  true";
        return;
    }
    if(isCount == false)
    {
        check_type = Check_OutDoor;
        CheckTime->start(500);
        isBarCodeGetOK = false;
        bool iOut = AglDeviceP->agl_out();
        if(false == iOut)
        {
            return;
        }
        Door_State = 3;

        LCD_time->stop();
        disconnect(LCD_time, SIGNAL(timeout()), this, SLOT(LCDtimer()));
        delete LCD_time;
        InfoWidget->close();
//        SamEdit->clear();
        KeyInput.clear();
        QString saminfo= AglDeviceP->GetConfig("Sam_sation");
        Label_station->setText(saminfo);
        UiType = Testui;
        isGetBarCodeDataError = false;
        if(isTimer_CodeStart)
        {
            qDebug() << "13334 Stop Timer";
            Timer_ScanLOT->stop();
        }
    }
    else
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_ret"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Warning);
         LCD_time->stop();
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                Voice_pressKey();
                if(2 == Door_State)
                {
                    check_type = Check_OutDoor;
                    CheckTime->start(500);
                    isBarCodeGetOK = false;
                    bool iOut = AglDeviceP->agl_out();
                    if(false == iOut)
                    {
                        return;
                    }
                    Door_State = 3;
                }
                disconnect(LCD_time, SIGNAL(timeout()), this, SLOT(LCDtimer()));
                delete LCD_time;
                InfoWidget->close();
//                SamEdit->clear();
                KeyInput.clear();
                QString entinfo = AglDeviceP->GetConfig("Sam_sation");
                Label_station->setText(entinfo);
                UiType = Testui;
                isCount = false;
                isStart = false;
                if(isTimer_CodeStart)
                {
                    qDebug() << "13378 Stop Timer";
                    Timer_ScanLOT->stop();
                }
                isBarCodeGetOK = false;
                isClickedCloseDoor = false;
                break;
            }
            case QMessageBox::No:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                 LCD_time->start(1000);
                msgBox->close();
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
}

void PctGuiThread :: LCDtimer()
{
    Num_now = Ttime.addSecs((-1) * iTimer);
    QString Stime = Num_now.toString("mm:ss") + " " + AglDeviceP->GetConfig("Infor_time");
    Label_Ti->setText(Stime);
    iTimer++;
    QString times = Edit_Mea->text();
    bool ok;
    int time_index = times.toInt(&ok, 10) * 60 + 1;
    if(iTimer == time_index)
    {
        LCD_time->stop();
        isStart = true;
        Label_testing->show();
        TestBar->show();
        Radio_after->hide();
        Radio_now->hide();
        Label_Mea->hide();
        Edit_Mea->hide();
        //--------------Add-------------//
//        Group_time->hide();
//        Label_Ti->hide();
        //--------------------------------//

        if((int)AglDeviceP->AglDev.ChxVol == 1)
        {
//            UiTimer->setInterval(135);
                        UiTimer->setInterval(190);
        }
        else
        {
//            UiTimer->setInterval(210);
               UiTimer->setInterval(290);
        }
        UiTimer->start();
//-----------Add--------------------------------------------------------------------------------------------------//
        if(AglDeviceP->GetPasswd("@AUTO") == "YES")
        {
            bool ok;
            QString LisIp = AglDeviceP->GetPasswd("@LisIP");
            quint16 LisPort = AglDeviceP->GetPasswd("@LisPort").toUInt(&ok, 10);
            if(!LisSocket->open(QIODevice :: ReadWrite))
            {
                qDebug() << "'Open Socket error";
                return;
            }
            LisSocket->connectToHost(LisIp, LisPort);
        }
        else if(AglDeviceP->GetPasswd("@AUTO") == "No")
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_nopass"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
//-------------------------------------------------------------------------------------------------------------------//
        AnalyTest();
        if(isYmaxError != true)
        {
            qDebug() << "############################### use Analyvalue function";
            AnalyValue();
        }
    }
}

void PctGuiThread :: pressEdit_Mea()
{
    qDebug() << "time Edit has clicked";
    Voice_pressKey();
    UiType = InforKeyui;
    Radio_after->setEnabled(false);
    Radio_now->setEnabled(false);
    Edit_Mea->clear();
    if(isClicked == true)
    {
        return;
    }
    isClicked = true;
    TimeNumKey = new QWidget(InfoWidget);
    TimeNumKey->setGeometry(340, 103, 290, 210);
    TimeNumKey->setStyleSheet(QStringLiteral("background-color: rgb(58, 58, 58);"));

    QGridLayout *gridLayout_key = new QGridLayout(TimeNumKey);
    gridLayout_key->setSpacing(6);
    gridLayout_key->setVerticalSpacing(1);
    gridLayout_key->setContentsMargins(10, 10, 10, 0);

    QFont font_key;
    font_key.setPointSize(14);

    Mybutton *button_0 = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_0, 3, 1, 1, 1);
    button_0->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_0->setText(tr("0")); button_0->setFixedHeight(35); button_0->setFont(font_key);
    Mybutton *button_1 = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_1, 0, 0, 1, 1);
    button_1->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_1->setText(tr("1")); button_1->setFixedHeight(35); button_1->setFont(font_key);
    Mybutton *button_2 = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_2, 0, 1, 1, 1);
    button_2->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_2->setText(tr("2")); button_2->setFixedHeight(35); button_2->setFont(font_key);
    Mybutton *button_3 = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_3, 0, 2, 1, 1);
    button_3->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_3->setText(tr("3")); button_3->setFixedHeight(35); button_3->setFont(font_key);
    Mybutton *button_4 = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_4, 1, 0, 1, 1);
    button_4->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_4->setText(tr("4")); button_4->setFixedHeight(35); button_4->setFont(font_key);
    Mybutton *button_5 = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_5, 1, 1, 1, 1);
    button_5->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_5->setText(tr("5")); button_5->setFixedHeight(35); button_5->setFont(font_key);
    Mybutton *button_6 = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_6, 1, 2, 1, 1);
    button_6->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_6->setText(tr("6")); button_6->setFixedHeight(35); button_6->setFont(font_key);
    Mybutton *button_7 = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_7, 2, 0, 1, 1);
    button_7->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_7->setText(tr("7")); button_7->setFixedHeight(35); button_6->setFont(font_key);
    Mybutton *button_8 = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_8, 2, 1, 1, 1);
    button_8->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_8->setText(tr("8")); button_8->setFixedHeight(35); button_8->setFont(font_key);
    Mybutton *button_9 = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_9, 2, 2, 1, 1);
    button_9->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_9->setText(tr("9")); button_9->setFixedHeight(35); button_9->setFont(font_key);
    Mybutton *button_sure = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_sure, 3, 0, 1, 1);
    button_sure->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_sure->setText(tr("OK")); button_sure->setFixedHeight(35); button_sure->setFont(font_key);
    Mybutton *button_clear = new Mybutton(TimeNumKey); gridLayout_key->addWidget(button_clear, 3, 2, 1, 1);
    button_clear->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
    button_clear->setText(tr("Delete")); button_clear->setFixedHeight(35); button_clear->setFont(font_key);

    connect(button_0, SIGNAL(clicked()), this, SLOT(showButton_0()),Qt::UniqueConnection);
    connect(button_1, SIGNAL(clicked()), this, SLOT(showButton_1()),Qt::UniqueConnection);
    connect(button_2, SIGNAL(clicked()), this, SLOT(showButton_2()),Qt::UniqueConnection);
    connect(button_3, SIGNAL(clicked()), this, SLOT(showButton_3()),Qt::UniqueConnection);
    connect(button_4, SIGNAL(clicked()), this, SLOT(showButton_4()),Qt::UniqueConnection);
    connect(button_5, SIGNAL(clicked()), this, SLOT(showButton_5()),Qt::UniqueConnection);
    connect(button_6, SIGNAL(clicked()), this, SLOT(showButton_6()),Qt::UniqueConnection);
    connect(button_7, SIGNAL(clicked()), this, SLOT(showButton_7()),Qt::UniqueConnection);
    connect(button_8, SIGNAL(clicked()), this, SLOT(showButton_8()),Qt::UniqueConnection);
    connect(button_9, SIGNAL(clicked()), this, SLOT(showButton_9()),Qt::UniqueConnection);
    connect(button_clear, SIGNAL(clicked()), this, SLOT(showButton_clear()),Qt::UniqueConnection);
    connect(button_sure, SIGNAL(clicked()), this, SLOT(showButton_sure()),Qt::UniqueConnection);

    TimeNumKey->show();
}

void PctGuiThread :: pressInfoBut_start()
{
    Voice_pressKey();   
    if(UiType == InforKeyui)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_sam"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Question);
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                Voice_pressKey();
                TimeNumKey->close();
                UiType = Inforui;
                isClicked = false;
                Radio_after->setEnabled(true);
                Radio_now->setEnabled(true);
                break;
            }
            case QMessageBox::No:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                return;
            }
        }
        delete msgBox;
        isMessage = false;
    }
    if(isStart == true || UiType == LotKeyui)                                // 正在测试
    {
        return;
    }
    isStart = true;

    if(isCount == false)                            // 正在等待
    {
        if(Radio_after->isChecked())        // 延时开始
        {
            QString inforti = Edit_Mea->text();
            int Delay_time = inforti.toInt();
            if(inforti.isEmpty())
            {
                Voice_ScanError();
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("Message_Notime"));
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                isCount = false;
                isStart = false;
                isMessage = false;
                return;
            }

            if(Delay_time > 25)
            {
                Voice_ScanError();
                isMessage = true;
                msgBox = new QMessageBox(MainWindowP);
                msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox->setText(AglDeviceP->GetConfig("Message_Time_Err"));
                msgBox->setIcon(QMessageBox :: Critical);
                msgBox->exec();
                delete msgBox;
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                isCount = false;
                isStart = false;
                isMessage = false;
                return;
            }
        }
////------------------------------Add----07-11-14--------Record time-----------------------------------//
//        QString TimeData = "Clicked Start Button Time: ";
//        QString Surrenttime = QTime::currentTime().toString("hh:mm:ss.zzz");
//        TimeData += Surrenttime + "\r\n";
//        RecordTime(TimeData);
//        qDebug() << "clicked start button need record time ";
////----------------------------------------------------------------------------------------------------------------//
        AddCheck = 0;
        check_type = Check_InDoor_Test;
        CheckTime->start(100);

//        Delayms(500);
        if(!(isAdmin == true && AglDeviceP->GetPasswd("@QRcode") == "YES"))          // no Admin need scan BCcode
        {
            Timer_ScanLOT->start(100);
            isTimer_CodeStart = true;
        }

        if(isBarCodeGetOK == false)
        {
            if(!AglDeviceP->agl_int())
            {
                isNext = false;
                isStart = false;
                return;
            }
        }

        isClickedCloseDoor = true;

        if(isGetBarCodeDataError == true)
        {
            Voice_ScanError();
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Measage_GetCardErr"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }

            LCD_time->stop();
            disconnect(LCD_time, SIGNAL(timeout()), this, SLOT(LCDtimer()));
            delete LCD_time;
            KeyInput.clear();
            SamEdit->clear();
            KeyInput.clear();
            SamEdit->clear();
            if(UiType == LotKeyui)
            {
                CardKeyWidget->close();
            }
            InfoWidget->close();
            TestWidget->close();
            QString main = AglDeviceP->GetConfig("station_Main");
            Label_station->setText(main);
            UiType = MainUi;
            isClicked = false;
            isGetBarCodeDataError = false;
            isClickedCloseDoor = false;

            isStart = false;
            isNext = false;
            isMessage = false;
            if(isTimer_CodeStart)
            {
                qDebug() << "13753 Stop Timer";
                Timer_ScanLOT->stop();
            }
            return;
        }

        if(isAdmin == true && AglDeviceP->GetPasswd("@QRcode") == "YES")       // 不需要匹配数据库的二维码时使用默认的二维码
        {
            QString QRCode = "YX-YX-20171127,0301,4,4100,2018-1-1,15,PG1,ng/mL,300,30000,1,32,5000,PG1,30,165,A-B,15,200,T,"
                             "直线方程,8.48987,31.74516,0,0,PG2,ng/mL,300,30000,1,25.30668,7276,PG2,3,15,A-B,3,60,T,"
                             "四参数方程,7276.89537,-0.99137,238.78807,25.30668,HP,EIU,300,30000,1,57.15145,13498,HP,0,30,A-B,15,400,T,"
                             "四参数方程,13498.94021,-2.38055,61.73207,57.15145,G17,pmol/L,300,30000,1,38.72666,7249,G17,1,7,A-B,1,40,T,"
                             "四参数方程,7249.28397,-0.99565,148.38925,38.72666,70;";
//            qDebug() << "================ USE BIOHIT QRcode";
//            QString QRCode = "YX-20170820,031700,4,500,2019/10/1,15,PG1,ng/ml,300,30000,1100,1500,700,1100,30,165,A-B,0,1000,T,直线方程,"
//                             "8.58717,-11.25255,0,0,PG2,ng/ml,300,30000,1100,1500,700,1100,3,15,A-B,0,200,T,四参数方程,"
//                             "6101.47702,-1.04006,218.40426,27.95379,Hp,EIU,300,30000,1100,1500,700,1100,30,,<A,0,700,T,四参数方程,"
//                             "14984.0557,-1.68207,80.49697,-277.77984,G17,pmol/L,300,30000,1100,1500,700,1100,1,7,A-B,0,200,T,四参数方程,14366.76717,-0.98181,258.69256,53,93;";
            if(!Code2Aanly(QRCode, &AnaFound))
            {
                isNext = false;
                isStart = false;
                return;
            }

            if(isBarCodeGetOK == false)                             // 如果没有收到OK，在此处循环读取OK值
            {
//                qDebug() << "<><><><><><><><><>如果没有收到OK，在此处循环读取OK值";
                QString ReadCOMData = "";
                int iCount_Door = 0;
                while("OK" != ReadCOMData)
                {
                    Delayms(300);
                    QByteArray Station = AglDeviceP->agl_rd_data(0x05, 0);
                    ReadCOMData = Station.data();
                    iCount_Door++;
//                    qDebug() << "=====================如果没有收到OK，在此处循环读取OK值 << CMD = 0x05";
                    if(20 < iCount_Door)
                    {
                        Voice_ScanError();
                        isMessage = true;
                        msgBox = new QMessageBox(MainWindowP);
                        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                        msgBox->setText("Lower machine error!");
                        msgBox->setIcon(QMessageBox :: Critical);
                        msgBox->exec();
                        delete msgBox;
                        if(isWidget == false)
                        {
                            Voice_pressKey();
                        }
                        isCount = false;
                        isStart = false;
                        isNext = false;
                        isMessage = false;
                        return;
                    }
                }
            }

            isNext = false;
//----------------------------------Add----------------------------------------------//
            CreatResuWidget();
            ResWidget->hide();
//-------------------------------------------------------------------------------------//
            if(Radio_after->isChecked())        // 延时开始
            {
                iTimer = 1;
                QString inforti = Edit_Mea->text();
                QString infosta = AglDeviceP->GetConfig("Infor_station");
                Label_station->setText(infosta);
                Ttime = QTime::fromString(inforti+".00", "m.s");
//-------------------Add---------------------------------------------------------------------//
                Label_Sam_show->hide();
                Label_Sam_1->hide();
                Label_Sam_2->hide();
                Label_Sam_3->hide();
                Label_Sam_4->hide();
                Radio_now->hide();
                Radio_after->hide();
                Label_Mea->hide();
                Edit_Mea->hide();
                InfoBut_start->hide();
                Label_InfoTit->setText(AglDeviceP->GetConfig("Title_delay"));
//---------------------------------------------------------------------------------------------//
                Label_Ti->setText(Ttime.toString("mm:ss") + " " + AglDeviceP->GetConfig("Infor_time"));
                Label_Ti->show();
                LCD_time->setInterval(1000);
                LCD_time->start();
                Label_station->setText(AglDeviceP->GetConfig("Title_delay"));
                isCount = true;
                Radio_after->setDisabled(true);
                Radio_now->setDisabled(true);
                isStart = false;
            }
            else                                                // 立即开始
            {
                isStart = true;
                Label_testing->show();
                TestBar->show();
                Radio_after->hide();
                Radio_now->hide();
                Label_Mea->hide();
                Edit_Mea->hide();
                if((int)AglDeviceP->AglDev.ChxVol == 1)
                {
                    UiTimer->setInterval(190);
    //                   UiTimer->setInterval(135);
                }
                else
                {
    //                UiTimer->setInterval(210);
                    UiTimer->setInterval(290);
                }
    //            //------------------------------Add----07-11-14--------Record time-----------------------------------//
    //            QString TimeData1 = "Test Timer Start Time:     ";
    //            QString Surrenttime1 = QTime::currentTime().toString("hh:mm:ss.zzz");
    //            TimeData1 += Surrenttime1 + "\r\n";
    //            RecordTime(TimeData1);
    //             qDebug() << "Test timer start  need record time ";
    //            //----------------------------------------------------------------------------------------------------------------//
                UiTimer->start();
    //-----------Add--------------------------------------------------------------------------------------------------//
                if(AglDeviceP->GetPasswd("@AUTO") == "YES")
                {
                    bool ok;
                    QString LisIp = AglDeviceP->GetPasswd("@LisIP");
                    quint16 LisPort = AglDeviceP->GetPasswd("@LisPort").toUInt(&ok, 10);
                    if(!LisSocket->open(QIODevice :: ReadWrite))
                    {
                        qDebug() << "'Open Socket error";
                        isNext = false;
                        isStart = false;
                        return;
                    }
                    LisSocket->connectToHost(LisIp, LisPort);
                }
                else if(AglDeviceP->GetPasswd("@AUTO") == "No")
                {
                    isMessage = true;
                    msgBox = new QMessageBox(MainWindowP);
                    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    msgBox->setText(AglDeviceP->GetConfig("Message_nopass"));
                    msgBox->setIcon(QMessageBox :: Critical);
                    msgBox->exec();
                    delete msgBox;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    isNext = false;
                    isStart = false;
                    isMessage = false;
                    return;
                }
                AnalyTest();
                if(isYmaxError != true)
                {
                    AnalyValue();
                }
    //            //------------------------------Add----07-11-14--------Record time-----------------------------------//
    //            QString TimeData = "Count Result Over Time:    ";
    //            QString Surrenttime = QTime::currentTime().toString("hh:mm:ss.zzz");
    //            TimeData += Surrenttime + "\r\n";
    //            RecordTime(TimeData);
    //             qDebug() << "Test Over need record time ";
    //            //----------------------------------------------------------------------------------------------------------------//
                qDebug() << "------------------------Test Over------------------------";
            }
//-------------------------------------------------------------------------------------//
        }
    }
    else                                                // 正在倒计时
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_Quit"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Question);
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                Voice_pressKey();
                LCD_time->stop();
                Label_station->setText(AglDeviceP->GetConfig("Sam_sation"));
                isCount = false;
                Radio_after->setDisabled(false);
                Radio_now->setDisabled(false);
//                Group_time->hide();
                Label_Ti->hide();
                check_type = Check_OutDoor;
                CheckTime->start(500);
                isBarCodeGetOK = false;
                if(!AglDeviceP->agl_out())
                {
                    return;
                }
                Door_State = 3;
                isNext = false;
                isStart = false;
                break;
            }
            case QMessageBox::No:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                msgBox->close();
                isNext = false;
                isStart = false;
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
}

void PctGuiThread :: pressExitButton()
{
    Voice_pressKey();
    ResWidget->close();
    UiType = MainUi;
    Label_station->clear();
    Label_HB->clear();
}

void PctGuiThread :: pressUpload_Button()
{
    Voice_pressKey();
    if(isYmaxError == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_YMaxErr_upload"));
        msgBox->setIcon(QMessageBox ::Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    char CR = 13;  //CR
    qDebug() << "Send infor is " << SEND_Res << "\r\n\r\n";
    if(LisSocket->state() == 3 && isConnect == true && isUpload == false)
    {
        qint16 Datalen =  LisSocket->write(SEND_Res.toLatin1().data(), SEND_Res.length());
        if(Datalen != SEND_Res.length())
        {
            isUpload = false;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_Uperr"));
            msgBox->setIcon(QMessageBox ::Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
        LisSocket->waitForBytesWritten(100);
        QByteArray ReadData;
        LisSocket->waitForReadyRead(100);
        ReadData = LisSocket->readAll();
        QString allData = ReadData.data();
        qDebug() << "Read data is " << allData;

        QStringList Read_list = allData.split((QString)CR);
        int Len_CR = Read_list.length();
        if(4 != Len_CR)
        {
            isUpload = true;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_Uperr"));
            msgBox->setIcon(QMessageBox ::Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            return;
        }
        allData = Read_list.at(1);
        Read_list = allData.split("|");
        int Len_MA = Read_list.length();
        if(8 != Len_MA)
        {
            isUpload = true;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_Uperr"));
            msgBox->setIcon(QMessageBox ::Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            return;
        }
        if(Read_list.at(1) == "AA" && Read_list.at(3) == "Message accepted" && Read_list.at(6) == "0")
        {
            isUpload = true;
            LisSocket->disconnectFromHost();
            LisSocket->close();
            qDebug() << "手动上传数据完成后，断开网络连接 line is 7748";
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_Upsuc"));
            msgBox->setIcon(QMessageBox ::Warning);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
        }
        else
        {
            isUpload = false;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_Uperr"));
            msgBox->setIcon(QMessageBox ::Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
        }
    }
    else if(isUpload == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_Upload"));
        msgBox->setIcon(QMessageBox ::Warning);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
    }
    else if(LisSocket->state() != 3 || isConnect == false)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_NoNet"));
        msgBox->setIcon(QMessageBox ::Critical);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
    }
}

void PctGuiThread :: pressLine_home()
{
    Voice_pressKey();
    check_type = Check_InDoor;
    CheckTime->start(900);
    if(!AglDeviceP->agl_int())
    {
        return;
    }
    Door_State = 2;
    AnaWidget->close();
    ResWidget->close();
    TestWidget->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    UiType = MainUi;
}

void PctGuiThread :: pressHomeButton()
{
    Voice_pressKey();
    check_type = Check_InDoor;
    CheckTime->start(900);
    if(!AglDeviceP->agl_int())
    {
        return;
    }
    Door_State = 2;
    int Socket_sta = LisSocket->state();
    if(Socket_sta == 3)
    {
        LisSocket->disconnectFromHost();
    }
    LisSocket->close();
    ResWidget->close();
    TestWidget->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    UiType = MainUi;
}

void PctGuiThread :: pressAnaButton()
{
    Voice_pressKey();
    if(isYmaxError == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_YMaxErr_Line"));
        msgBox->setIcon(QMessageBox ::Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    AnaWidget->show();
    QStringList TC_list = Str_Flag.split("\r\n");

    Label_T1->setText(TC_list.at(0));
    Label_C1->setText(TC_list.at(1));
    if((int)AglDeviceP->AglDev.ChxVol == 4)
    {
        Label_T2->setText(TC_list.at(2));
        Label_C2->setText(TC_list.at(3));
        Label_T3->setText(TC_list.at(4));
        Label_C3->setText(TC_list.at(5));
        Label_T4->setText(TC_list.at(6));
        Label_C4->setText(TC_list.at(7));
    }

//    Label_T1->setText(TC_list.at(2));
//    Label_C1->setText(TC_list.at(3));
//    if((int)AglDeviceP->AglDev.ChxVol == 4)
//    {
//        Label_T2->setText(TC_list.at(4));
//        Label_C2->setText(TC_list.at(5));
//        Label_T3->setText(TC_list.at(6));
//        Label_C3->setText(TC_list.at(7));
//        Label_T4->setText(TC_list.at(8));
//        Label_C4->setText(TC_list.at(9));
//    }
}

void PctGuiThread::pressRetButton()
{
    Voice_pressKey();
    AnaWidget->hide();
}

void PctGuiThread :: pressreButton()
{
    Voice_pressKey();
    ResWidget->close();
    CreatTestWidget();
}

void PctGuiThread :: pressSamBut_input()
{
    Voice_pressKey();
    CreatTestKey();
}

void PctGuiThread :: pressButton_NumRet()
{
    Voice_pressKey();
    bool ok;
    if(UiType == HostIpui || UiType == LisIpui || UiType == LisPortui)
    {
        QString IPAddr = Edit_IP->text();
        QStringList IPList = IPAddr.split(".");
        if(IPList.length() != 4)
        {
            isIPAddrErr = true;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_IPErr"));
            msgBox->setIcon(QMessageBox ::Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            return;
        }
        int IP1 = IPList.at(0).toInt(&ok, 10);
        int IP2 = IPList.at(1).toInt(&ok, 10);
        int IP3 = IPList.at(2).toInt(&ok, 10);
        int IP4 = IPList.at(3).toInt(&ok, 10);

        if((IPList.at(0).left(1) == "0" &&  IPList.at(0).length() != 1)|| (IPList.at(1).left(1) == "0" &&  IPList.at(1).length() != 1) || (IPList.at(2).left(1) == "0" &&  IPList.at(2).length() != 1) || (IPList.at(3).left(1) == "0" &&  IPList.at(3).length() != 1))
        {
            isIPAddrErr = true;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_IPErr"));
            msgBox->setIcon(QMessageBox ::Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }

        if(IP1 > 255 || IP2 > 255 || IP3 > 255 || IP4 > 255)
        {
            isIPAddrErr = true;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_IPErr"));
            msgBox->setIcon(QMessageBox ::Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
        if(Edit_IP->text() == Edit_LisIP->text())
        {
            isIPAddrErr = true;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_double"));
            msgBox->setIcon(QMessageBox ::Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            return;
        }

        IPAddr = Edit_LisIP->text();
        IPList = IPAddr.split(".");
        if(IPList.length() != 4)
        {
            isIPAddrErr = true;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_IPErr"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }

        if((IPList.at(0).left(1) == "0" &&  IPList.at(0).length() != 1)|| (IPList.at(1).left(1) == "0" &&  IPList.at(1).length() != 1) || (IPList.at(2).left(1) == "0" &&  IPList.at(2).length() != 1) || (IPList.at(3).left(1) == "0" &&  IPList.at(3).length() != 1))
        {
            isIPAddrErr = true;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_IPErr"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }

        IP1 = IPList.at(0).toInt(&ok, 10);
        IP2 = IPList.at(1).toInt(&ok, 10);
        IP3 = IPList.at(2).toInt(&ok, 10);
        IP4 = IPList.at(3).toInt(&ok, 10);
        if(IP1 > 255 || IP2 > 255 || IP3 > 255 || IP4 > 255)
        {
            isIPAddrErr = true;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_IPErr"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            return;
        }

        int port = Edit_LisPort->text().toInt(&ok, 10);
        if(port > 65535)
        {
            isIPAddrErr = true;
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_Lang"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            return;
        }
        DevNumKey->close();
        UiType = COSui;
    }
    else
    {
        QString AUTO_sta;
        if(Box_Send->isChecked())
        {
            AUTO_sta = "Checked";
        }
        else
        {
            AUTO_sta = "DisChecked";
        }

        QString QRCode_sta;
        if(Box_QRcode->isChecked())
        {
            QRCode_sta = "USE";
        }
        else
        {
            QRCode_sta = "NOUSE";
        }
        QString Auto;
        QString QRco;
        if(AglDeviceP->GetPasswd("@AUTO") == "NOAUTO")
        {
            Auto = "DisChecked";
        }
        else
        {
            Auto = "Checked";
        }
        if(AglDeviceP->GetPasswd("@QRcode") == "YES")
        {
            QRco = "USE";
        }
        else
        {
            QRco = "NOUSE";
        }

        OldUserData = Edit_IP->text()+Edit_LisIP->text()+Edit_LisPort->text()+Edit_data->text()+Edit_time->text()+DefaultLanguage+Auto+QRco;
        QString CurrentData = Edit_IP->text()+Edit_LisIP->text()+Edit_LisPort->text()+Edit_data->text()+Edit_time->text()+box_Lan->currentText()+AUTO_sta+QRCode_sta;
        if(OldUserData != CurrentData /*&& isSaveDev != true*/)
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setText(AglDeviceP->GetConfig("Message_SaDev"));
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
            msgBox->setDefaultButton(QMessageBox::No);
            msgBox->setIcon(QMessageBox :: Question);
            int ret = msgBox->exec();
            switch (ret)
            {
                case QMessageBox::Yes:
                {
                    Voice_pressKey();
                    SaveSet();
                    isMessage_child = true;
                    msgBox_child = new QMessageBox(MainWindowP);
                    msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    msgBox_child->setText(AglDeviceP->GetConfig("Message_succsave"));
                    msgBox_child->setIcon(QMessageBox :: Information);
                    msgBox_child->exec();
                    Voice_pressKey();
                    isMessage_child = false;
                    break;
                }
                case QMessageBox::No:
                {
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    msgBox->close();
                    break;
                }
            }
            delete msgBox;
            isMessage = false;
        }
        if(isWidget == true)
        {
            return;
        }
        isSaveDev = false;
        KeyInput.clear();
        Group_COS->hide();
        Label_station->setText(AglDeviceP->GetConfig("Station_Select"));
        UiType = COSui;
    }
}

void PctGuiThread :: ClickedItem_Set()
{
    Voice_pressKey();
    if(UiType == Setui)
    {
        QList<QTableWidgetItem *> Item = table_num->selectedItems();    // 获得选中的单元格，返回值是一个单元格列表QList<QTableWidgetItem *>
        Edit_pro->clear();                                                                                     //  列表中只有一个Item， Item.at(0)取出这个Item，获得它的内容并显示
        Edit_pro->setText(Item.at(0)->text());
        QString LotData = Edit_pro->text();

        QByteArray DataByte;
        if(!Sql->SelectBatchSql(DataByte, 0, LotData))
        {
            qDebug() << "8964 Select Batch DB ERROR";
            return;
        }
        LotData = DataByte.data();
        qDebug() << "LotData is " << LotData;

        Num = Item.at(0)->text();
        QStringList time = LotData.split(",");
        Edit_max->setText(time.at(2));
        Edit_max->setText(AglDeviceP->GetConfig("LOT_Test"));
        QString Last = time.at(3);
        time = Last.split("@");
        Last = time.at(0);
        Edit_last->setText(Last);
        Edit_remark->setText(AglDeviceP->GetConfig("LOT_Infor"));
        Edit_made->setText(AglDeviceP->GetConfig("LOT_Manufa"));
//        Edit_remark->setText("Agile100s");
//        Edit_made->setText("ShangHai YiXin");
    }
}

void PctGuiThread :: ClickedItemTable_Res()                                            // 将数据库中的ID解析出来，赋值给SelectID
{
    qDebug() << "rowCount is " << Table_Res->rowCount();
    Voice_pressKey();
}

void PctGuiThread :: ClickedItem_Use()
{
    Voice_pressKey();
}

void PctGuiThread :: pressbutton_close()
{
    Voice_pressKey();
    if(UiType == NewPassui2 || UiType == NewPassui)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_nosuccess"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Warning);
        int ret = msgBox->exec();
        switch (ret)
        {
            case QMessageBox::Yes:
            {
                Voice_pressKey();
                Group_Key->hide();
                text.clear();
                Ltext->clear();
                QString inpcruu= AglDeviceP->GetConfig("Label_curr");
                Ltext->setText(inpcruu);
                break;
            }
            case QMessageBox::No:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
    else
    {
        Group_Key->hide();
        QString inpcruu= AglDeviceP->GetConfig("Label_curr");
        Ltext->setText(inpcruu);
        Edit_key->clear();
        UiType = Usrui;
    }
}

void PctGuiThread :: pressButton_Pass()
{
    Voice_pressKey();
    if(iTime < TIMES)
    {
        UiType = Passui;
        KeyInput.clear();
        Edit_key->clear();
        Group_Key->show();
    }
    else
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_24"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
    }
}

void PctGuiThread :: pressButton_Usr()
{
    Voice_pressKey();
    if(iTime < TIMES)
    {
        UiType = Usrkeyui;
        text.clear();
        Edit_key->clear();
        Group_Key->show();
    }
    else
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_24"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
    }
}

void PctGuiThread :: pressBut_Inf_home()
{
    Voice_pressKey();
    COSWidget->close();
    SetWidget->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    UiType = MainUi;
}

void PctGuiThread :: pressButton_Infor()
{
    Voice_pressKey();
    Label_station->setText(AglDeviceP->GetConfig("Station_infor"));
    Group_SetInfor->show();
}

void PctGuiThread :: pressMore_Home()
{
    Voice_pressKey();
    Group_More->close();
    Group_Res->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    UiType = MainUi;
}

void PctGuiThread :: pressMore_Ret()
{
    Voice_pressKey();
    if(UiType == Selectui)
    {
        Group_More->hide();
        Label_station->setText(AglDeviceP->GetConfig("station_Result"));
    }
    else
    {
        return;
    }
}

void PctGuiThread :: pressSaveButton()
{
    Voice_pressKey();
    if(UiType != Selectui)
    {
        return;
    }
    ResList = Table_Res->selectedItems();                                                           // 获得选中的单元格，返回值是一个单元格列表QList<QTableWidgetItem *>
    if(ResList.isEmpty())
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_noResult"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    QString SelectItem = SelectID = ResList.at(0)->text();
    QStringList List =  CharacterStr(SelectItem);

    ID = List.at(1);
    SelectID = List.at(0);
    Pro_Item = List.at(2);

    struct Results Found;
    bool ok;
    int found_id = SelectID.toInt(&ok, 10);
    ok =  Sql->GetDetailInforResultSql(Found,found_id);
    if(ok == false)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_nores"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    SamIDShow = Found.SamId;
    QString Fondtext = Found.Report;
    Pro_text = Fondtext;
    QStringList FoundList = Fondtext.split("///");
    if(FoundList.length() != 3)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_nores"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    TC_Text = FoundList.at(2);
    Text_Select =  FoundList.at(0);

    CreatSeleResultWidget();
    Label_station->setText(AglDeviceP->GetConfig("Selec_But1_1"));
    QString DaTa;
    QStringList DataList;

    DaTa = Found.Date;
    DataList = DaTa.split(";");
    if(DataList.length() != 2)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_nores"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
            Group_More->hide();
        }
        return;
    }
    int16_t LineData[4][2000];
    double LineShow[4][2000];
    iMode = DataList.at(1).toInt(&ok, 10);
    memcpy((char*)(&LineData[0][0]), (char*)Found.DatBuf,8000*sizeof(int16_t));
    double Resd[2000];
    for(int i = 0; i < 2000; i++)
    {
        Resd[i] = i;
        LineShow[0][i] = (double)LineData[0][i];
        LineShow[1][i] = (double)LineData[1][i];
        LineShow[2][i] = (double)LineData[2][i];
        LineShow[3][i] = (double)LineData[3][i];
    }
    Line1->setSamples(Resd, (double*)(&LineShow[0][0]),1800);
    Line2->setSamples(Resd, (double*)(&LineShow[1][0]),1800);
    Line3->setSamples(Resd, (double*)(&LineShow[2][0]),1800);
    Line4->setSamples(Resd, (double*)(&LineShow[3][0]),1800);
}

void PctGuiThread :: pressBut_PgDown()
{
    Voice_pressKey();
    if(PageAll == 1 || isClicked == true)
    {
        return;
    }
    QTableWidgetItem *Item_Sha[10];
    int iWhile = 1;
    if(isSelectRes == true)
    {
         while(iWhile--)
         {
             if(PageNow < PageAll)
             {
                 iOver = Select_Res_All - PageNow * 10;
                 if(iOver > 10)
                 {
                     iShow = 10;
                     Table_Res->setGeometry(133.5,101,565,179);
                     Table_Res->setColumnWidth(0, 563);
                 }
                 else
                 {
                     iShow = iOver;
                     if(iShow > 4)
                     {
                         Table_Res->setGeometry(133.5,101,565,179);
                         Table_Res->setColumnWidth(0, 563);
                     }
                     else
                     {
                         Table_Res->setGeometry(133.5,101,535,179);
                         Table_Res->setColumnWidth(0, 533);
                     }
                 }
                 QString DateDate;
                 QStringList DateList;
                 QStringList Sele_list;
                 for(int  j = 0; j < 10; j++)
                 {
                     Table_Res->removeRow(0);
                 }

                 Table_Res->setRowCount(iShow);
                 Table_Res->setColumnCount(1);

                 for(int j = 0; j < iShow; j++)
                 {
                     Table_Res->setRowHeight(j, 36);
                 }

                 QString Show_item;
                 int LINE = 0;
                 for(LINE = 0; LINE < iShow; LINE++)             // 表格上方显示查找到的数据
                 {
                     DateDate = Res_list.at(LINE + Select_Res_show);
                     DateList = DateDate.split(",");

                     Show_item.clear();
                     sprintf(dat, "%-8.8s", DateList.at(0).toLatin1().data());
                     Show_item += (QString)dat;

                     sprintf(dat, "%-16.16s", DateList.at(1).toLatin1().data());
                     Show_item += (QString)dat;

                     Sele_list = DateList.at(4).split(";");
                     DateDate = Sele_list.at(0);
                     sprintf(dat, "%-19.19s", DateDate.toLatin1().data());
                     Show_item += (QString)dat;

                     Item_Sha[LINE] = new QTableWidgetItem;
                     Item_Sha[LINE]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                     Item_Sha[LINE]->setText(Show_item);
                     Table_Res->setItem(LINE, 0, Item_Sha[LINE]);
                 }
                 Select_Res_show += LINE;
                 PageNow += 1;
                 QString Now = QString::number(PageNow);
                 QString ALL = QString::number(PageAll);
                 Label_cou->setText(Now + "/" + ALL + " page");
                 break;
             }
             if(PageNow == PageAll)
             {
                 int iDel = Table_Res->rowCount();
                 for(int  j = 0; j < iDel; j++)
                 {
                     Table_Res->removeRow(0);
                 }
                 Table_Res->setGeometry(133.5,101,565,179);
                 Table_Res->setColumnCount(1);
                 Table_Res->setColumnWidth(0, 563);
                 Table_Res->setRowCount(10);

                 for(int j = 0; j < 10; j++)
                 {
                     Table_Res->setRowHeight(j, 36);
                 }
                 QString DateDate;
                 QStringList DateList;
                 QStringList Sele_list;
                 QString Show_item;
                 int  i = 0;
                 for( i = 0; i < 10; i++)
                 {
                     DateDate = Res_list.at(i);
                     DateList = DateDate.split(",");

                     Show_item.clear();
                     sprintf(dat, "%-8.8s", DateList.at(0).toLatin1().data());
                     Show_item += (QString)dat;

                     sprintf(dat, "%-16.16s", DateList.at(1).toLatin1().data());
                     Show_item += (QString)dat;

                     Sele_list = DateList.at(4).split(";");
                     DateDate = Sele_list.at(0);
                     sprintf(dat, "%-19.19s", DateDate.toLatin1().data());
                     Show_item += (QString)dat;

                     Item_Sha[i] = new QTableWidgetItem;
                     Item_Sha[i]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                     Item_Sha[i]->setText(Show_item);
                     Table_Res->setItem(i, 0, Item_Sha[i]);
                 }
                 Select_Res_show = i;
                 PageNow = 1;
                 QString Now = QString::number(PageNow);
                 QString ALL = QString::number(PageAll);
                 Label_cou->setText(Now + "/" + ALL + " page");
                 break;
             }
         }
    }
    else
    {
        int Allres = Sql->CountResultSql();
        if(-1 == Allres)
        {
            qDebug() << "9219 Count Result DB ERROR";
            return;
        }
        while(iWhile--)
        {
            if(PageNow < PageAll)
            {
                iOver = Allres - 10 * PageNow;
                for(int  j = 0; j < 10; j++)
                {
                    Table_Res->removeRow(0);
                }
                if(iOver > 10)
                {
                    iShow = 10;

                    Table_Res->setRowCount(iShow);
                    Table_Res->setColumnCount(1);
                    for(int j = 0; j < iShow; j++)
                    {
                        Table_Res->setRowHeight(j, 36);
                    }
                    Table_Res->setGeometry(133.5,101,565,179);
                    Table_Res->setColumnWidth(0, 563);
                }
                else
                {
                    iShow = iOver;
                    Table_Res->setRowCount(iShow);
                    Table_Res->setColumnCount(1);
                    for(int j = 0; j < iShow; j++)
                    {
                        Table_Res->setRowHeight(j, 36);
                    }
                    if(iShow > 4)
                    {
                        Table_Res->setGeometry(133.5,101,565,179);
                        Table_Res->setColumnWidth(0, 563);
                    }
                    else
                    {
                        Table_Res->setGeometry(133.5,101,535,179);
                        Table_Res->setColumnWidth(0, 533);
                    }
                }
                struct ResultIndex RES_LIST;
                QString DateDate;
                QStringList DateList;

                QString Show_item;
                QString ite_lise;
                qDebug() << "====== isSelectRes = false  Select Data Error! ";
                for(int  i =  PageNow*10; i < PageNow*10+iShow; i++)
                {
                    if(!Sql->ListResultSql(RES_LIST, i+1))
                    {
                        qDebug() << "9269 List Result DB ERROR";
                        return;
                    }
                    Show_item.clear();
                    ite_lise = QString::number(i+1);
                    sprintf(dat, "%-8.8s", ite_lise.toLatin1().data());
                    Show_item += (QString)dat;

                    sprintf(dat, "%-16.16s", RES_LIST.SamId.toLatin1().data());
                    Show_item += (QString)dat;

                    DateList = RES_LIST.Date.split(";");
                    DateDate = DateList.at(0);
                    sprintf(dat, "%-19.19s", DateDate.toLatin1().data());
                    Show_item += (QString)dat;

                    Item_Sha[i-PageNow*10] = new QTableWidgetItem;
                    Item_Sha[i-PageNow*10]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    Item_Sha[i-PageNow*10]->setText(Show_item);
                    Table_Res->setItem(i-PageNow*10, 0, Item_Sha[i-PageNow*10]);
                }
                PageNow += 1;
                QString nowpage = QString :: number(PageNow, 10);
                QString allpage = QString :: number(PageAll, 10);
                Label_cou->clear();
                Label_cou->setText(nowpage + "/" + allpage +  " " + AglDeviceP->GetConfig("Label_Page"));
                break;
            }
            if(PageNow == PageAll)
            {
                for(int  j = 0; j < iShow; j++)
                {
                    Table_Res->removeRow(0);
                }

                Table_Res->setGeometry(133.5,101,565,179);
                Table_Res->setColumnCount(1);
                Table_Res->setRowCount(10);
                Table_Res->setColumnWidth(0, 563);

                for(int j = 0; j < 10; j++)
                {
                    Table_Res->setRowHeight(j, 36);
                }
                struct ResultIndex RES_LIST;
                QString DateDate;
                QStringList DateList;
                QString Show_item;
                QString ite_lise;
                for(int  i = 0; i < 10; i++)
                {
                    if(!Sql->ListResultSql(RES_LIST, i+1))
                    {
                        qDebug() << "9324 List Result DB ERROR";
                        return;
                    }
                    ite_lise = QString::number(i+1);
                    Show_item.clear();
                    sprintf(dat, "%-8.8s", ite_lise.toLatin1().data());
                    Show_item += (QString)dat;

                    sprintf(dat, "%-16.16s", RES_LIST.SamId.toLatin1().data());
                    Show_item += (QString)dat;

                    DateList = RES_LIST.Date.split(";");
                    DateDate = DateList.at(0);
                    sprintf(dat, "%-19.19s", DateDate.toLatin1().data());
                    Show_item += (QString)dat;

                    Item_Sha[i] = new QTableWidgetItem;
                    Item_Sha[i]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    Item_Sha[i]->setText(Show_item);
                    Table_Res->setItem(i, 0, Item_Sha[i]);
                }
                PageNow = 1;
                QString nowpage = QString :: number(PageNow, 10);
                QString allpage = QString :: number(PageAll, 10);
                Label_cou->clear();
                Label_cou->setText(nowpage + "/" + allpage +  " " + AglDeviceP->GetConfig("Label_Page"));
                break;
            }
        }
    }
    isClicked = false;
}

void PctGuiThread :: pressBut_PgUp()
{
    Voice_pressKey();
    if(PageAll == 1 || isClicked == true)
    {
        return;
    }
    isClicked = true;
    QTableWidgetItem *Item_Sha[10];
    int iWhile = 1;
    if(isSelectRes == true)
    {
        while(iWhile--)
        {
            if(PageNow == 1)
            {
                iOver = Select_Res_All - 10 * (PageAll - 1);
                for(int  j = 0; j < 10; j++)
                {
                    Table_Res->removeRow(0);
                }
                Table_Res->setRowCount(iOver);
                Table_Res->setColumnCount(1);
                if(iOver > 4)
                {
                    Table_Res->setGeometry(133.5,101,565,179);
                    Table_Res->setColumnWidth(0, 563);
                }
                else
                {
                    Table_Res->setGeometry(133.5,101,535,179);
                    Table_Res->setColumnWidth(0, 533);
                }
                for(int j = 0; j < iOver; j++)
                {
                    Table_Res->setRowHeight(j, 36);
                }
                QString DateDate;
                QStringList DateList;
                QStringList Sele_list;
                QString Show_item;
                int index = (PageAll-1)*10;
                for(int  i = index; i < Select_Res_All; i++)
                {
                    DateDate = Res_list.at(i);
                    DateList = DateDate.split(",");

                    Show_item.clear();
                    sprintf(dat, "%-8.8s", DateList.at(0).toLatin1().data());
                    Show_item += (QString)dat;

                    sprintf(dat, "%-16.16s", DateList.at(1).toLatin1().data());
                    Show_item += (QString)dat;

                    Sele_list = DateList.at(4).split(";");
                    DateDate = Sele_list.at(0);
                    sprintf(dat, "%-19.19s", DateDate.toLatin1().data());
                    Show_item += (QString)dat;

                    Item_Sha[i-index] = new QTableWidgetItem;
                    Item_Sha[i-index]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    Item_Sha[i-index]->setText(Show_item);
                    Table_Res->setItem(i-index, 0, Item_Sha[i-index]);
                }
                Select_Res_show = Select_Res_All - 1;
                PageNow = PageAll;
                QString Now = QString::number(PageNow);
                QString ALL = QString::number(PageAll);
                Label_cou->setText(Now + "/" + ALL + " page");
                break;
            }
            else
            {
                int iDel = Select_Res_All - 10 * (PageNow - 1);
                for(int  j = 0; j < iDel; j++)                                                 // 删除表格中所有内容（包括Item）
                {
                    Table_Res->removeRow(0);
                }
                Table_Res->setGeometry(133.5,101,565,179);
                Table_Res->setColumnCount(1);
                Table_Res->setColumnWidth(0, 563);
                Table_Res->setRowCount(10);

                for(int j = 0; j < 10; j++)
                {
                    Table_Res->setRowHeight(j, 36);
                }
                QString DateDate;
                QStringList DateList;
                QStringList Sele_list;
                QString Show_item;

                int InDex = (PageNow-2)*10;
                int  i  = 0;
                for(i = InDex; i < InDex + 10; i++)
                {
                    DateDate = Res_list.at(i);
                    DateList = DateDate.split(",");

                    Show_item.clear();
                    sprintf(dat, "%-8.8s", DateList.at(0).toLatin1().data());
                    Show_item += (QString)dat;

                    sprintf(dat, "%-16.16s", DateList.at(1).toLatin1().data());
                    Show_item += (QString)dat;

                    Sele_list = DateList.at(4).split(";");
                    DateDate = Sele_list.at(0);
                    sprintf(dat, "%-19.19s", DateDate.toLatin1().data());
                    Show_item += (QString)dat;

                    Item_Sha[i-InDex] = new QTableWidgetItem;
                    Item_Sha[i-InDex]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    Item_Sha[i-InDex]->setText(Show_item);
                    Table_Res->setItem(i-InDex, 0, Item_Sha[i-InDex]);
                }
                Select_Res_show = i;
                PageNow -= 1;
                QString Now = QString::number(PageNow);
                QString ALL = QString::number(PageAll);
                Label_cou->setText(Now + "/" + ALL + " page");
                break;
            }
        }
    }
    else
    {
        int iSum = Sql->CountResultSql();
        if(-1 == iSum)
        {
            qDebug() << "9400 Count Result DB ERROR";
            return;
        }
        while(iWhile--)
        {
            if(PageNow == 1)
            {
                iOver = iSum - 10 * (PageAll - 1);
                for(int  j = 0; j < 10; j++)
                {
                    Table_Res->removeRow(0);
                }
                iShow = iSum - 10 * (PageAll - 1);
                if(iShow > 4)
                {
                    Table_Res->setGeometry(133.5,101,565,179);
                    Table_Res->setRowCount(iShow);
                    Table_Res->setColumnCount(1);
                    Table_Res->setColumnWidth(0, 563);
                }
                else
                {
                    Table_Res->setGeometry(133.5,101,535,179);
                    Table_Res->setRowCount(iShow);
                    Table_Res->setColumnCount(1);
                    Table_Res->setColumnWidth(0, 533);
                }
                for(int j = 0; j < iOver; j++)
                {
                    Table_Res->setRowHeight(j, 36);
                }

                struct ResultIndex RES_LIST;
                QString DateDate;
                QStringList DateList;
                QString ite_lise;
                QString Show_item;
                for(int  i = (PageAll-1)*10+1; i <= iSum; i++)
                {
                    if(!Sql->ListResultSql(RES_LIST,  i))
                    {
                        qDebug() << "9442 List Result DB ERROR";
                        return;
                    }
                    Show_item.clear();
                    ite_lise = QString::number(i);
                    sprintf(dat, "%-8.8s", ite_lise.toLatin1().data());
                    Show_item += (QString)dat;

                    sprintf(dat, "%-16.16s", RES_LIST.SamId.toLatin1().data());
                    Show_item += (QString)dat;

                    DateList = RES_LIST.Date.split(";");
                    DateDate = DateList.at(0);
                    sprintf(dat, "%-19.19s", DateDate.toLatin1().data());
                    Show_item += (QString)dat;

                    Item_Sha[i-(PageAll-1)*10-1] = new QTableWidgetItem;
                    Item_Sha[i-(PageAll-1)*10-1]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    Item_Sha[i-(PageAll-1)*10-1]->setText(Show_item);
                    Table_Res->setItem(i-(PageAll-1)*10-1, 0, Item_Sha[i-(PageAll-1)*10-1]);
                }
                PageNow = PageAll;
                QString nowpage = QString :: number(PageNow, 10);
                QString allpage = QString :: number(PageAll, 10);
                Label_cou->clear();
                Label_cou->setText(nowpage + "/" + allpage +  " " + AglDeviceP->GetConfig("Label_Page"));
                break;
            }
            else
            {
                int iDel = iSum - 10 * (PageNow - 1);
                for(int  j = 0; j < iDel; j++)                                                 // 删除表格中所有内容（包括Item）
                {
                    Table_Res->removeRow(0);
                }
                Table_Res->setGeometry(133.5,101,565,179);
                Table_Res->setColumnWidth(0, 563);
                Table_Res->setRowCount(10);
                Table_Res->setColumnCount(1);

                for(int j = 0; j < 10; j++)
                {
                    Table_Res->setRowHeight(j, 36);
                }
                struct ResultIndex RES_LIST;
                QString DateDate;
                QStringList DateList;
                QString Show_item;
                QString ite_lise;

                int InDex = (PageNow-2)*10+1;
                for(int  i = InDex; i <= InDex + 10; i++)
                {
                    if(!Sql->ListResultSql(RES_LIST, i))
                    {
                        qDebug() << "9515 List Result DB ERROR";
                        return;
                    }
                    Show_item.clear();
                    ite_lise = QString::number(i);
                    sprintf(dat, "%-8.8s", ite_lise.toLatin1().data());
                    Show_item += (QString)dat;

                    sprintf(dat, "%-16.16s", RES_LIST.SamId.toLatin1().data());
                    Show_item += (QString)dat;

                    DateList = RES_LIST.Date.split(";");
                    DateDate = DateList.at(0);
                    sprintf(dat, "%-19.19s", DateDate.toLatin1().data());
                    Show_item += (QString)dat;

                    Item_Sha[i-InDex] = new QTableWidgetItem;
                    Item_Sha[i-InDex]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    Item_Sha[i-InDex]->setText(Show_item);
                    Table_Res->setItem(i-InDex, 0, Item_Sha[i-InDex]);
                }
                PageNow--;
                QString nowpage = QString :: number(PageNow, 10);
                QString allpage = QString :: number(PageAll, 10);
                Label_cou->clear();
                Label_cou->setText(nowpage + "/" + allpage +  " " + AglDeviceP->GetConfig("Label_Page"));
                break;
            }
        }
    }
    isClicked = false;
}

void PctGuiThread :: pressSqlPrint()
{
    Voice_pressKey();
    struct Results Found;
    bool ok = false;
    int found_id = SelectID.toInt(&ok, 10);

    ok =  Sql->GetDetailInforResultSql(Found,found_id);
    if(ok == false)
    {
        qDebug() << "Found ERROR";
    }

    QString Fondtext = Found.Report;
    QStringList FoundList = Fondtext.split("///");
    Print_Text.clear();
    Print_Text = FoundList.at(1) + "\r\n";
    Print_Text += "                                               ";

//    Print_Text = "昨夜风开露井桃，未央前殿月轮高。平阳歌舞新承宠，帘外春寒赐锦袍。";
    AglDeviceP->PrintData(Print_Text.toLatin1());
//     AglDeviceP->PrintData(Print_Text.toStdString());
    qDebug() << "--------------------- Printf over";
}

void PctGuiThread :: pressButt_Ret()
{
    Voice_pressKey();
    if(UiType == UserKeyui || UiType == PassKeyui)
    {
        UserKeyWidget->close();
        UiType = SetUserui;
        KeyInput.clear();
        UserEdit->clear();
    }
    else if(UiType == NewKeyui)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setText(AglDeviceP->GetConfig("Message_nosuccess"));
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        msgBox->setIcon(QMessageBox :: Question);
        int ret = msgBox->exec();
        switch(ret)
        {
            case  QMessageBox :: Yes :
            {
                Voice_pressKey();
                UserKeyWidget->close();
                UiType = SetUserui;
                KeyInput.clear();
                UserEdit->clear();
                break;
            }
            default:
            {
                if(isWidget == false)
                {
                    Voice_pressKey();
                }
                break;
            }
        }
        delete msgBox;
        isMessage = false;
    }
    else if(UiType == SetUserui)
    {       
        QString CurrUSRdata;
        int iLow = Table_Use->rowCount();
//        for(int i = 1; i < iLow; i++)
        for(int i = 0; i < iLow; i++)
        {
            CurrUSRdata += Table_Use->item(i, 0)->text() + "," + Table_Use->item(i, 1)->text();
        }
        if(OldUserData != CurrUSRdata && isSave == false)
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setText(AglDeviceP->GetConfig("Message_save"));
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
            msgBox->setDefaultButton(QMessageBox::No);
            msgBox->setIcon(QMessageBox :: Question);
            int ret = msgBox->exec();
            switch (ret)
            {
                case QMessageBox::Yes:
                {
                    Voice_pressKey();
                    QString USRmessage;
                    int iRow = Table_Use->rowCount();
//                    for(int i = 1; i < iRow; i++)
                    for(int i = 0; i < iRow; i++)
                    {
                        USRmessage += Table_Use->item(i, 0)->text() + "=" + Table_Use->item(i, 1)->text() + "#\r\n";
                    }
                    QString Other = AglDeviceP->GetAllParadata();
                    USRmessage += Other;

                    if(!AglDeviceP->DeleteFile("password.txt"))
                    {
                        qDebug() << "Delete file error!";
                    }
                    if(!AglDeviceP->WriteTxtToFile("password.txt", USRmessage))
                    {
                        qDebug() << "12690 Creat file error!";
                    }
                    if(!AglDeviceP->ReloadPasswdFile(USRmessage))
                    {
                        qDebug() << "Reload password Error!";
                    }
                    isSave = true;
                    break;
                }
                case QMessageBox::No:
                {
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    break;
                }
            }
            delete msgBox;
            isMessage = false;
        }
        if(isWidget == false)
        {
            USERWidget->close();
            UiType = Usrui;
            Label_station->setText(AglDeviceP->GetConfig("Station_Select"));
        }
    }
}

void PctGuiThread :: pressRadio_Pla1()
{
    Voice_pressKey();
    Edit_pla->setEnabled(false);
}

void PctGuiThread :: pressRadio_Pla2()
{
    Voice_pressKey();
    Edit_pla->setEnabled(true);
}

void PctGuiThread :: pressRadio_after()
{
    Voice_pressKey();
    Edit_Mea->setEnabled(true);
}

void PctGuiThread :: pressRadio_now()
{
    Voice_pressKey();
    Edit_Mea->setEnabled(false);
}

void PctGuiThread :: pressRadio()
{
    Voice_pressKey();
    qDebug() << "we have selece radiobutton";
    if(RadioPtID->isChecked())
    {
        iSelect = 1;
        qDebug() << "----------- RadioPtID is select ";
    }
    else if(RadioDate->isChecked())
    {
        iSelect = 2;
        qDebug() << "----------- RadioDate is select ";
    }
}

void PctGuiThread :: pressSelButton()
{
    Voice_pressKey();
    if(iSelect == 0)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_noRadio"));
        msgBox->setIcon(QMessageBox :: Information);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
    }
    else
    {
//        if(UiType == SelectKeyui)
//        {
//            return;
//        }
//        KeyInput.clear();
//        Table_Res->hide();
        UiType = SelectKeyui;
        CreatSelectKey();
    }
}

void PctGuiThread :: pressKeyButton_up()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    QPixmap pix_up;
    QString Up0 = AglDeviceP->GetPasswd("@pix_up_1");
    QString Up1 = AglDeviceP->GetPasswd("@pix_up_0");
    KeyStation++;
    if(KeyStation % 2 == 1)
    {
        pix_up.load(Up0);
        for(int i = 0; i < 26; i++)
        {
            SelectKey[i] = 'a' + i - 32;
            pushButton[i]->setText(QString(SelectKey[i]));
        }
    }
    else
    {
        for(int i = 0; i < 26; i++)
        {
            SelectKey[i] = 'a' + i;
            pushButton[i]->setText(QString(SelectKey[i]));
        }
        pix_up.load(Up1);
    }
    pushButton_Up->setIcon(pix_up);
    pushButton_Up->setIconSize(QSize(40,25));
}

void PctGuiThread :: presspushButton_del()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(UiType == SelectKeyui)
    {
        KeyInput = Edit_Sel->text();
        int len = KeyInput.length();
        KeyInput = KeyInput.remove(len - 1, 1);
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        KeyInput = LineEdit_input->text();
        int len = KeyInput.length();
        KeyInput = KeyInput.remove(len - 1, 1);
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        KeyInput = UserEdit->text();
        int len = KeyInput.length();
        KeyInput = KeyInput.remove(len - 1, 1);
        UserEdit->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        KeyInput = SamEdit->text();
        int len = KeyInput.length();
        KeyInput = KeyInput.remove(len - 1, 1);
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_a()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "a";
    }
    else
    {
        KeyInput = KeyInput + "A";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_b()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "b";
    }
    else
    {
        KeyInput = KeyInput + "B";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_c()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "c";
    }
    else
    {
        KeyInput = KeyInput + "C";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_d()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "d";
    }
    else
    {
        KeyInput = KeyInput + "D";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_e()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "e";
    }
    else
    {
        KeyInput = KeyInput + "E";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_f()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "f";
    }
    else
    {
        KeyInput = KeyInput + "F";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_g()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "g";
    }
    else
    {
        KeyInput = KeyInput + "G";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_h()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "h";
    }
    else
    {
        KeyInput = KeyInput + "H";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_i()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "i";
    }
    else
    {
        KeyInput = KeyInput + "I";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_j()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "j";
    }
    else
    {
        KeyInput = KeyInput + "J";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_k()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "k";
    }
    else
    {
        KeyInput = KeyInput + "K";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_l()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "l";
    }
    else
    {
        KeyInput = KeyInput + "L";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_m()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "m";
    }
    else
    {
        KeyInput = KeyInput + "M";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_n()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "n";
    }
    else
    {
        KeyInput = KeyInput + "N";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_o()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "o";
    }
    else
    {
        KeyInput = KeyInput + "O";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_p()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "p";
    }
    else
    {
        KeyInput = KeyInput + "P";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_q()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "q";
    }
    else
    {
        KeyInput = KeyInput + "Q";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_r()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "r";
    }
    else
    {
        KeyInput = KeyInput + "R";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_s()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "s";
    }
    else
    {
        KeyInput = KeyInput + "S";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_t()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "t";
    }
    else
    {
        KeyInput = KeyInput + "T";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_u()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "u";
    }
    else
    {
        KeyInput = KeyInput + "U";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_v()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "v";
    }
    else
    {
        KeyInput = KeyInput + "V";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_w()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "w";
    }
    else
    {
        KeyInput = KeyInput + "W";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_x()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "x";
    }
    else
    {
        KeyInput = KeyInput + "X";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_y()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "y";
    }
    else
    {
        KeyInput = KeyInput + "Y";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_z()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(KeyStation % 2 == 0)
    {
        KeyInput = KeyInput + "z";
    }
    else
    {
        KeyInput = KeyInput + "Z";
    }
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_0()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + "0";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_1()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + "1";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_2()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + "2";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_3()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + "3";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_4()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + "4";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_5()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + "5";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_6()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + "6";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_7()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + "7";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_8()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + "8";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_9()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + "9";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi )
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui|| UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: pressKeyButton_star()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(UiType == SelectKeyui)
    {
        KeyInput = KeyInput + "*";
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        KeyInput = KeyInput + "*";
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        KeyInput = KeyInput + "*";
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        KeyInput = KeyInput + "*";
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        KeyInput = KeyInput + ":";
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: presspushButton_Mao()
{
    Voice_pressKey();
    KeyInput = KeyInput + ":";
    Edit_Sel->setText(KeyInput);
}

void PctGuiThread :: pressKeyButton_dot()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    if(UiType == SelectKeyui)
    {
        KeyInput = KeyInput + "-";
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui || UiType == PassKeyui)
    {
        KeyInput = KeyInput + ".";
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        KeyInput = KeyInput + ".";
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui || UiType == Passui || UiType == NewPassui || UiType == NewPassui2)
    {
        KeyInput = KeyInput + ".";
        Edit_key->setText(KeyInput);
    }
    else if(UiType == TestKeyui)
    {
        KeyInput = KeyInput + "-";
        QString EditData = SamEdit->text();
        KeyInput = EditData + KeyInput;
        SamEdit->setText(KeyInput);
        KeyInput.clear();
    }
}

void PctGuiThread :: presspushButton_sure()
{
    Voice_pressKey();
    if(UiType == UserKeyui)
    {
//        for(int i = 1; i < iCount_User; i++)
        iCount_User = Table_Use->rowCount();
        for(int i = 0; i < iCount_User; i++)
        {
           if(Table_Use->item(i, 0)->text() == KeyInput)
           {
               isMessage = true;
               msgBox = new QMessageBox(MainWindowP);
               msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
               msgBox->setText(AglDeviceP->GetConfig("User_exists"));
               msgBox->setIcon(QMessageBox :: Critical);
               msgBox->exec();
               delete msgBox;
               isMessage = false;
               if(isWidget == false)
               {
                   Voice_pressKey();
               }
               UserEdit->clear();
               KeyInput.clear();
               return;
           }
        }
        if(KeyInput.isEmpty())
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_empty"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
        if(KeyInput.left(1) == " " || KeyInput.right(1) == " ")
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_sapce"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            UserEdit->clear();
            KeyInput.clear();
            return;
        }
//        if(KeyInput.length() > 9)
        if(KeyInput.length() > 10)                   // 用户名最多10个字母
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_long"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            UserEdit->clear();
            KeyInput.clear();
            isMessage = false;
            return;
        }
        iCount_User = Table_Use->rowCount();
        User_New[iCount_User] = KeyInput;
        UserEdit->clear();
        KeyInput.clear();
        UiType = NewKeyui;
        QString newpass = AglDeviceP->GetConfig("Label_user2");
        Label_ADD->setText(newpass);
    }
    else if(UiType == NewKeyui)
    {
        if(KeyInput.isEmpty())
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_empty"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
        if(KeyInput.left(1) == " " || KeyInput.right(1) == " ")
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_Pass_sapce"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            UserEdit->clear();
            KeyInput.clear();
            return;
        }
        if(KeyInput.length() > 10)                   // 密码最多10个字母
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_Pass_long"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            UserEdit->clear();
            KeyInput.clear();
            isMessage = false;
            return;
        }
        Pass_New[iCount_User] = KeyInput;
        UserEdit->clear();
        KeyInput.clear();
//        iCount_User = Table_Use->rowCount();                                               //  获得当前表格行数
        Table_Use->setRowCount(iCount_User + 1);                                       //  在获得的行数下面子新增一行
        Table_Use->setRowHeight(iCount_User, 40);                                      //  设置新增行的高度
        Item_User[iCount_User] = new QTableWidgetItem(User_New[iCount_User]);
        Item_Pass[iCount_User] = new QTableWidgetItem(Pass_New[iCount_User]);
        Item_User[iCount_User]->setTextAlignment(Qt::AlignCenter);
        Item_Pass[iCount_User]->setTextAlignment(Qt::AlignCenter);
        Table_Use->setItem(iCount_User, 0, Item_User[iCount_User]);
        Table_Use->setItem(iCount_User, 1, Item_Pass[iCount_User]);
        iCount_User++;

        KeyInput.clear();
        UserEdit->clear();
        UserKeyWidget->close();
        UiType = SetUserui;
        isSave = false;
        if(iCount_User > 4)                                                             //   当行数大于或小于4时，重绘表格
        {
            Table_Use->setGeometry(160, 91, 512, 202);
        }
        else
        {
            Table_Use->setGeometry(160, 91, 482, 202);
        }
    }
    else if(UiType == PassKeyui)
    {
        if(KeyInput.isEmpty())
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_empty"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            isMessage = false;
            return;
        }
        if(KeyInput.left(1) == " " || KeyInput.right(1) == " ")
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_Pass_sapce"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            UserEdit->clear();
            KeyInput.clear();
            return;
        }

        iRow = Table_Use->currentRow();                                       //    获取Table选中的行号
        Table_Use->item(iRow, 1)->setText(KeyInput);

        KeyInput.clear();
        UserEdit->clear();
        UserKeyWidget->close();
        UiType = SetUserui;
        isSave = false;
    }
    else if(UiType == TestKeyui)
    {
        SamIDShow = SamEdit->text();
        TestKeyWidget->hide();
        UiType = Testui;
    }
}

void PctGuiThread :: pressKeyButton_space()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    KeyInput = KeyInput + " ";
    if(UiType == SelectKeyui)
    {
        Edit_Sel->setText(KeyInput);
    }
    else if(UiType == UserKeyui || UiType == NewKeyui)
    {
        UserEdit->setText(KeyInput);
    }
    else if(UiType == LoginUi)
    {
        LineEdit_input->setText(KeyInput);
    }
    else if(UiType == Usrkeyui)
    {
        Edit_key->setText(KeyInput);
    }
}

void PctGuiThread :: presspushButton_Ret()
{
    Voice_pressKey();
    if(UiType == SelectKeyui)
    {
        Table_Res->show();
        retuButton->show();
        DownButton->show();
        SelecKeyWidget->close();
        UiType = Selectui;
        KeyInput.clear();
        Edit_Sel->clear();
    }
    else if(UiType == UserKeyui)
    {
        UserKeyWidget->close();
        UiType = COSui;
        KeyInput.clear();
        UserEdit->clear();
    }
}

void PctGuiThread :: pressBut_Add()
{
    Voice_pressKey();
    if(UiType != SetUserui)
    {
        return;
    }
    iCount_User = Table_Use->rowCount();
    if(iCount_User > 5)                                               // 用户最多有5个
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_reache"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    UiType = UserKeyui;
    KeyInput.clear();
    CreatUserKey();
}

void PctGuiThread :: pressBut_Set()
{
    Voice_pressKey();
    if(UiType != SetUserui)
    {
        return;
    }
//    if(Table_Use->rowCount() == 1)
    if(Table_Use->rowCount() == 0)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_nomodify"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        return;
    }
    QList<QTableWidgetItem *>Item_List = Table_Use->selectedItems();
    if(Item_List.isEmpty())
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_semodify"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
    User_old = Item_List.at(0)->text();
    Pass_old = Item_List.at(1)->text();
    KeyInput.clear();
    UiType = PassKeyui;
    CreatUserKey();
    QString newpass= AglDeviceP->GetConfig("Label_Pass2");
    Label_ADD->setText(newpass);
}

void PctGuiThread :: pressBut_Del()
{
    Voice_pressKey();
    if(UiType != SetUserui)
    {
        return;
    }
    QList<QTableWidgetItem *>Item_List = Table_Use->selectedItems();
    if(Item_List.isEmpty())
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_seuser"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        return;
    }
    if(Table_Use->rowCount() == 0)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_nouser"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }

    isMessage = true;
    msgBox = new QMessageBox(MainWindowP);
    msgBox->setWindowTitle("Warning");
    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180,180,180);"));
    msgBox->setText(AglDeviceP->GetConfig("Message_deluser"));
    msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::No);
    msgBox->setIcon(QMessageBox :: Warning);
    int ret = msgBox->exec();
    switch (ret)
    {
        case QMessageBox::Yes:
        {
            Voice_pressKey();
            if(Item_List.at(0)->text() == "Admin")
            {
                isMessage_child = true;
                msgBox_child = new QMessageBox(MainWindowP);
                msgBox_child->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                msgBox_child->setText(AglDeviceP->GetConfig("Message_admin"));
                msgBox_child->setIcon(QMessageBox :: Warning);
                msgBox_child->exec();
                Voice_pressKey();
                delete msgBox_child;
                isMessage_child = false;
                return;
            }
            int iLow = Table_Use->rowCount()-1;
            if(iLow < 5)
            {
                Table_Use->setGeometry(160, 91, 482, 202);
            }
            else
            {
                Table_Use->setGeometry(160, 91, 512, 202);
            }
            iRow = Table_Use->currentRow();          //    获取当前选中Table的行号
            Table_Use->removeRow(iRow);
            iCount_User--;
            isSave = false;
            break;
        }
        case QMessageBox::No:
        {
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            break;
        }
    }
    delete msgBox;
    isMessage = false;
}

void PctGuiThread :: pressEdit_time()
{    
    iTime = 0;
    if(isWidget == true)
    {
        AglDeviceP->SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
        isWidget = false;
        return;
    }
    if(iEditTime < 1)
    {
        iEditTime++;
        return;
    }
    Voice_pressKey();
}

void PctGuiThread :: pressEdit_data()
{
    iTime = 0;
    if(isWidget == true)
    {
        AglDeviceP->SysCall("echo 100 >> /sys/class/backlight/pwm-backlight.0/brightness");
        isWidget = false;
        return;
    }
    if(iDate < 1)
    {
        iDate++;
        return;
    }
    Voice_pressKey();
}

//void PctGuiThread :: presscombox()
//{
//    Voice_pressKey();
//}

void PctGuiThread :: presscombox_Info()
{
    Voice_pressKey();
}

void PctGuiThread :: pressbox_Lan()
{
    Voice_pressKey();
    if(UiType == LoginUi)
    {
        combox->showPopup();
    }
    else
    {
        box_Lan->showPopup();
    }
}

//#endif

void PctGuiThread :: Voice_ScanSucess()
{
    AglDeviceP->FmqON();
    Delayms(150);
    AglDeviceP->FmqOFF();
}

void PctGuiThread :: Voice_ScanError()
{
    AglDeviceP->FmqON();
    Delayms(50);
    AglDeviceP->FmqOFF();
    Delayms(150);
    AglDeviceP->FmqON();
    Delayms(50);
    AglDeviceP->FmqOFF();
    Delayms(150);
}

void PctGuiThread :: Voice_LowBattery()
{
    AglDeviceP->FmqON();
    Delayms(80);
    AglDeviceP->FmqOFF();
    Delayms(130);
    AglDeviceP->FmqON();
    Delayms(80);
    AglDeviceP->FmqOFF();
    Delayms(130);
}

void PctGuiThread :: Voice_ChangStation()
{
    AglDeviceP->FmqON();
    Delayms(200);
    AglDeviceP->FmqOFF();
}

void PctGuiThread :: Voice_pressKey()
{
    AglDeviceP->FmqON();
    Delayms(80);
    AglDeviceP->FmqOFF();
}

void PctGuiThread :: pressMore_line()
{
    Voice_pressKey();
    CreatSelecLine();
}

void PctGuiThread :: pressSelecLin_Home()
{
    Voice_pressKey();
    SelecLineWidget->close();
    Group_More->close();
    Group_Res->close();
    QString main = AglDeviceP->GetConfig("station_Main");
    Label_station->setText(main);
    UiType = MainUi;
}

void PctGuiThread :: pressSelecLin_Ret()
{
    Voice_pressKey();
    SelecLineWidget->close();
}

void PctGuiThread :: pressButton_in()
{
    Voice_pressKey();
    if(2 == Door_State)
    {
        return;
    }
    check_type = Check_InDoor;
    CheckTime->start(900);
    if(!AglDeviceP->agl_int())
    {
        return;
    }
    Door_State = 2;
}

void PctGuiThread :: pressButton_out()
{
    Voice_pressKey();
    if(3 == Door_State)
    {
        return;
    }
    check_type = Check_OutDoor;
    CheckTime->start(500);
    isBarCodeGetOK = false;
    if(!AglDeviceP->agl_out())
    {
        return;
    }
    Door_State = 3;
}

void PctGuiThread :: presspritButton()
{   
    Voice_pressKey();
    if(isYmaxError == true)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_YMaxErr_print"));
        msgBox->setIcon(QMessageBox ::Critical);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }

    QString Showtext;
    Showtext = Str_line + TC_Value;
    Showtext += "                                                ";
    AglDeviceP->PrintData(Showtext.toLatin1());
//    AglDeviceP->PrintData(Showtext.toStdString());
}

//void PctGuiThread :: pressSamBut_sweep()                                        // 一维码扫描
//{
//    Voice_pressKey();
//--------------------------------------------
//    QFile ImageFile("agl_2d.bin");
//    if(ImageFile.open(QIODevice::ReadOnly))
//    {
//        TwoCodeStr Analyload;
//        QByteArray BSam0=ImageFile.readAll();
//        QString SSam0 = BSam0.data();
//        bool iRet = Code2Aanly(SSam0,&Analyload);
//        if(iRet == true)
//        {
//            Analy=Analyload;
//        }
//    }
//    ImageFile.close();
    //-----------------------
//    QByteArray BSam;
//    AglDeviceP->Scan1D_Code(BSam);
//    if(BSam.isEmpty())
//    {
//        Voice_ScanError();
//        QMessageBox msgBox;
//        msgBox.setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));
//        QString filed = AglDeviceP->GetConfig("Message_failed");
//        msgBox.setText(filed);
//        msgBox.setIcon(QMessageBox :: Warning);
//#ifndef ENGLISH
//        msgBox.setButtonText(QMessageBox::Ok, "确 定");
//        msgBox.move(230, 180);
//#else
//        msgBox.move(198, 175);
//#endif
//        msgBox.exec();
//        Voice_pressKey();
//        return;
//    }
//    Voice_ScanSucess();
//    Sam = BSam.data();

//    if(UiType == Batui && (Analy.Yiweima + "\r\n") != Sam)
//    {
//        QMessageBox msgBox;
//        msgBox.setStyleSheet(QStringLiteral("background-color: rgb(215, 215, 215);"));
//        QString nofound = AglDeviceP->GetConfig("Sam_Nofound");
//        msgBox.setText(nofound);
//        msgBox.setIcon(QMessageBox :: Warning);
//        msgBox.move(230, 220);
//        msgBox.exec();
//        return;
//    }
//}

void PctGuiThread :: pressButton_sweep()                                        // 二维码扫描
{
    if(UiType == Setui)
    {
        Voice_pressKey();
    }
    QByteArray BSam;
    bool Ret =  AglDeviceP->Scan2D_Code(BSam);
    if(Ret == false || BSam.isEmpty())
    {
        if(UiType == Setui)
        {
            Voice_ScanError();
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_failed"));
            msgBox->setIcon(QMessageBox :: Warning);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
        }
        return;
    }
    if(UiType == Setui)
    {
        Voice_pressKey();
    }
    QString SSam = BSam.data();
    bool iRet = Code2Aanly(SSam,&Analy);
    if(iRet == false)
    {
        return;
    }

    if(UiType == Setui)
    {                          
        iBat = table_num->rowCount();
        BatNu[iBat] = Analy.Head + Analy.Pihao;
        if(iBat != 0)
        {
            for(int i = 0; i < iBat; i++)
            {
                if(table_num->item(i , 0)->text() == BatNu[iBat])
                {
                    isMessage = true;
                    msgBox = new QMessageBox(MainWindowP);
                    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
                    msgBox->setText(AglDeviceP->GetConfig("Measage_batexi"));
                    msgBox->setIcon(QMessageBox :: Critical);
                    msgBox->exec();
                    delete msgBox;
                    if(isWidget == false)
                    {
                        Voice_pressKey();
                    }
                    isMessage = false;
                    return;
                }
            }
        }
        Edit_pro->setText(Analy.Pihao);
        Edit_last->setText(Analy.youxiaoshijian);
        if(Analy.Xiangmushuliang == "1")
        {
               Edit_max->setText(AglDeviceP->GetConfig("LOT_Test"));
        }
        else if(Analy.Xiangmushuliang == "4")
        {
            Edit_max->setText(AglDeviceP->GetConfig("LOT_Test"));
        }
        Edit_remark->setText(AglDeviceP->GetConfig("LOT_Infor"));
        Edit_made->setText(AglDeviceP->GetConfig("LOT_Manufa"));
        Code2d inser;
        item[iBat] = new QTableWidgetItem(BatNu[iBat]);
        inser.Code2D = SSam;
//        if(!Sql->InsertBatchSql(&inser))
        int Time = 0;
        if(!Sql->InsertBatchSql(&inser, Time))
        {
            qDebug() << "11735 Insert Batch DB ERROR";
            return;
        }
        if(iBat > 4)
        {
            table_num->setGeometry(100, 95, 220, 172);
            item[0]->setTextAlignment(Qt::AlignLeft);
            for(int idex = 0; idex < iBat ; idex++)
            {
                item[idex]->setTextAlignment(Qt::AlignLeft);
            }
        }
        table_num->setRowCount(iBat + 1);                                       //  在获得的行数下面子新增一行
        table_num->setRowHeight(iBat + 1, 34);
        for(int i = iBat; i > 0; i--)
        {
            item[i]->setText(item[i - 1]->text());
        }
        item[0]->setText(BatNu[iBat]);
        if(iBat <= 4)
        {
            item[iBat]->setTextAlignment(Qt::AlignCenter);
        }

        table_num->setItem(iBat, 0, item[iBat]);
    }
    else if(UiType == TestKeyui)
    {
        if(SamID.length() > 30)
        {
            isMessage = true;
            msgBox = new QMessageBox(MainWindowP);
            msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
            msgBox->setText(AglDeviceP->GetConfig("Message_qrcode"));
            msgBox->setIcon(QMessageBox :: Critical);
            msgBox->exec();
            delete msgBox;
            isMessage = false;
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            return;
        }
        if(SamID.isEmpty())
        {
            return;
        }
        SamEdit->setText(SamID);
    }
}

bool PctGuiThread :: Code2Aanly(QString Str, TwoCodeStr *Code)       // 二维码解析
{
    QStringList Cod;
    Cod = Str.split(",");
    if(UiType == Setui && Cod.length() < 25)
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_qrcode"));
        msgBox->setIcon(QMessageBox :: Critical);
        msgBox->exec();
        delete msgBox;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        return false;
    }
    if(UiType == TestKeyui && Cod.length() == 1)
    {
        QString Samfound = Cod.at(0);
        int ifound = Samfound.indexOf("\r\n");
        SamID =  Samfound.left(ifound);                                                            //  获得扫描到的样本ID
        return true;
    }
    QString Header = Cod.at(0);
    bool ok = false;
    if(Header.left(3) != "YX-")
    {
        Header = "YX-" + Header;
    }
    Code->Pihao = Header;
    Code->Yiweima = Cod.at(1);
    Code->Xiangmushuliang = Cod.at(2);
    Code->PiLiang = Cod.at(3);
    Code->PiLiang.toInt(&ok);
    if(ok == false)
    {
        qDebug() << "Pi liang input Error";
        return false;
    }
    AglDeviceP->AglDev.ChxVol = (char)Code->Xiangmushuliang.toInt(&ok, 10);
    Code->youxiaoshijian = Cod.at(4);
    Code->yanshi = Cod.at(5);
    switch ((int)AglDeviceP->AglDev.ChxVol)
    {
        case 1:
        {
            Code->XiangMuPm[0].xiangmu = Cod.at(6);
            Code->XiangMuPm[0].xiangmudanwei = Cod.at(7);
            Code->XiangMuPm[0].czuidizhi = Cod.at(8);
            Code->XiangMuPm[0].czuigaozhi = Cod.at(9);
            Code->XiangMuPm[0].WB_Para = Cod.at(10);
            Code->XiangMuPm[0].Equation_Limit = Cod.at(11);
            Code->XiangMuPm[0].Equation_Max = Cod.at(12);
            Code->XiangMuPm[0].NOP = Cod.at(13);
            Code->XiangMuPm[0].anongduzhi = Cod.at(14);
            Code->XiangMuPm[0].bnongduzhi = Cod.at(15);
            Code->XiangMuPm[0].cankaoxingshi = Cod.at(16);
            Code->XiangMuPm[0].nongdudizhi = Cod.at(17);
            Code->XiangMuPm[0].nongdugaozhi = Cod.at(18);
            Code->XiangMuPm[0].jisuanfangshi = Cod.at(19);
            Code->XiangMuPm[0].jisuanfangfa = Cod.at(20);
            Code->XiangMuPm[0].canshua = Cod.at(21);
            Code->XiangMuPm[0].canshub = Cod.at(22);
            Code->XiangMuPm[0].canshuc = Cod.at(23);
            Code->XiangMuPm[0].canshud = Cod.at(24);
            break;
        }
        case 2:
        {
            Code->XiangMuPm[0].xiangmu = Cod.at(6);
            Code->XiangMuPm[0].xiangmudanwei = Cod.at(7);
            Code->XiangMuPm[0].czuidizhi = Cod.at(8);
            Code->XiangMuPm[0].czuigaozhi = Cod.at(9);
            Code->XiangMuPm[0].WB_Para = Cod.at(10);
            Code->XiangMuPm[0].Equation_Limit = Cod.at(11);
            Code->XiangMuPm[0].Equation_Max = Cod.at(12);
            Code->XiangMuPm[0].NOP = Cod.at(13);
            Code->XiangMuPm[0].anongduzhi = Cod.at(14);
            Code->XiangMuPm[0].bnongduzhi = Cod.at(15);
            Code->XiangMuPm[0].cankaoxingshi = Cod.at(16);
            Code->XiangMuPm[0].nongdudizhi = Cod.at(17);
            Code->XiangMuPm[0].nongdugaozhi = Cod.at(18);
            Code->XiangMuPm[0].jisuanfangshi = Cod.at(19);
            Code->XiangMuPm[0].jisuanfangfa = Cod.at(20);
            Code->XiangMuPm[0].canshua = Cod.at(21);
            Code->XiangMuPm[0].canshub = Cod.at(22);
            Code->XiangMuPm[0].canshuc = Cod.at(23);
            Code->XiangMuPm[0].canshud = Cod.at(24);
            Code->XiangMuPm[1].xiangmu = Cod.at(25);
            Code->XiangMuPm[1].xiangmudanwei = Cod.at(26);
            Code->XiangMuPm[1].czuidizhi = Cod.at(27);
            Code->XiangMuPm[1].czuigaozhi = Cod.at(28);
            Code->XiangMuPm[1].WB_Para = Cod.at(29);
            Code->XiangMuPm[1].Equation_Limit = Cod.at(30);
            Code->XiangMuPm[1].Equation_Max = Cod.at(31);
            Code->XiangMuPm[1].NOP = Cod.at(32);
            Code->XiangMuPm[1].anongduzhi = Cod.at(33);
            Code->XiangMuPm[1].bnongduzhi = Cod.at(34);
            Code->XiangMuPm[1].cankaoxingshi = Cod.at(35);
            Code->XiangMuPm[1].nongdudizhi = Cod.at(36);
            Code->XiangMuPm[1].nongdugaozhi = Cod.at(37);
            Code->XiangMuPm[1].jisuanfangshi = Cod.at(38);
            Code->XiangMuPm[1].jisuanfangfa = Cod.at(39);
            Code->XiangMuPm[1].canshua = Cod.at(40);
            Code->XiangMuPm[1].canshub = Cod.at(41);
            Code->XiangMuPm[1].canshuc = Cod.at(42);
            Code->XiangMuPm[1].canshud = Cod.at(43);
            break;
        }
        case 3:
        {
            Code->XiangMuPm[0].xiangmu = Cod.at(6);
            Code->XiangMuPm[0].xiangmudanwei = Cod.at(7);
            Code->XiangMuPm[0].czuidizhi = Cod.at(8);
            Code->XiangMuPm[0].czuigaozhi = Cod.at(9);
            Code->XiangMuPm[0].WB_Para = Cod.at(10);
            Code->XiangMuPm[0].Equation_Limit = Cod.at(11);
            Code->XiangMuPm[0].Equation_Max = Cod.at(12);
            Code->XiangMuPm[0].NOP = Cod.at(13);
            Code->XiangMuPm[0].anongduzhi = Cod.at(14);
            Code->XiangMuPm[0].bnongduzhi = Cod.at(15);
            Code->XiangMuPm[0].cankaoxingshi = Cod.at(16);
            Code->XiangMuPm[0].nongdudizhi = Cod.at(17);
            Code->XiangMuPm[0].nongdugaozhi = Cod.at(18);
            Code->XiangMuPm[0].jisuanfangshi = Cod.at(19);
            Code->XiangMuPm[0].jisuanfangfa = Cod.at(20);
            Code->XiangMuPm[0].canshua = Cod.at(21);
            Code->XiangMuPm[0].canshub = Cod.at(22);
            Code->XiangMuPm[0].canshuc = Cod.at(23);
            Code->XiangMuPm[0].canshud = Cod.at(24);
            Code->XiangMuPm[1].xiangmu = Cod.at(25);
            Code->XiangMuPm[1].xiangmudanwei = Cod.at(26);
            Code->XiangMuPm[1].czuidizhi = Cod.at(27);
            Code->XiangMuPm[1].czuigaozhi = Cod.at(28);
            Code->XiangMuPm[1].WB_Para = Cod.at(29);
            Code->XiangMuPm[1].Equation_Limit = Cod.at(30);
            Code->XiangMuPm[1].Equation_Max = Cod.at(31);
            Code->XiangMuPm[1].NOP = Cod.at(32);
            Code->XiangMuPm[1].anongduzhi = Cod.at(33);
            Code->XiangMuPm[1].bnongduzhi = Cod.at(34);
            Code->XiangMuPm[1].cankaoxingshi = Cod.at(35);
            Code->XiangMuPm[1].nongdudizhi = Cod.at(36);
            Code->XiangMuPm[1].nongdugaozhi = Cod.at(37);
            Code->XiangMuPm[1].jisuanfangshi = Cod.at(38);
            Code->XiangMuPm[1].jisuanfangfa = Cod.at(39);
            Code->XiangMuPm[1].canshua = Cod.at(40);
            Code->XiangMuPm[1].canshub = Cod.at(41);
            Code->XiangMuPm[1].canshuc = Cod.at(42);
            Code->XiangMuPm[1].canshud = Cod.at(43);
            Code->XiangMuPm[2].xiangmu = Cod.at(44);
            Code->XiangMuPm[2].xiangmudanwei = Cod.at(45);
            Code->XiangMuPm[2].czuidizhi = Cod.at(46);
            Code->XiangMuPm[2].czuigaozhi = Cod.at(47);
            Code->XiangMuPm[2].WB_Para = Cod.at(48);
            Code->XiangMuPm[2].Equation_Limit = Cod.at(49);
            Code->XiangMuPm[2].Equation_Max = Cod.at(50);
            Code->XiangMuPm[2].NOP = Cod.at(51);
            Code->XiangMuPm[2].anongduzhi = Cod.at(52);
            Code->XiangMuPm[2].bnongduzhi = Cod.at(53);
            Code->XiangMuPm[2].cankaoxingshi = Cod.at(54);
            Code->XiangMuPm[2].nongdudizhi = Cod.at(55);
            Code->XiangMuPm[2].nongdugaozhi = Cod.at(56);
            Code->XiangMuPm[2].jisuanfangshi = Cod.at(57);
            Code->XiangMuPm[2].jisuanfangfa = Cod.at(58);
            Code->XiangMuPm[2].canshua = Cod.at(59);
            Code->XiangMuPm[2].canshub = Cod.at(60);
            Code->XiangMuPm[2].canshuc = Cod.at(61);
            Code->XiangMuPm[2].canshud = Cod.at(62);
            break;
        }
        case 4:
        {
            Code->XiangMuPm[0].xiangmu = Cod.at(6);
            Code->XiangMuPm[0].xiangmudanwei = Cod.at(7);
            Code->XiangMuPm[0].czuidizhi = Cod.at(8);
            Code->XiangMuPm[0].czuigaozhi = Cod.at(9);
            Code->XiangMuPm[0].WB_Para = Cod.at(10);
            Code->XiangMuPm[0].Equation_Limit = Cod.at(11);
            Code->XiangMuPm[0].Equation_Max = Cod.at(12);
            Code->XiangMuPm[0].NOP = Cod.at(13);
            Code->XiangMuPm[0].anongduzhi = Cod.at(14);
            Code->XiangMuPm[0].bnongduzhi = Cod.at(15);
            Code->XiangMuPm[0].cankaoxingshi = Cod.at(16);
            Code->XiangMuPm[0].nongdudizhi = Cod.at(17);
            Code->XiangMuPm[0].nongdugaozhi = Cod.at(18);
            Code->XiangMuPm[0].jisuanfangshi = Cod.at(19);
            Code->XiangMuPm[0].jisuanfangfa = Cod.at(20);
            Code->XiangMuPm[0].canshua = Cod.at(21);
            Code->XiangMuPm[0].canshub = Cod.at(22);
            Code->XiangMuPm[0].canshuc = Cod.at(23);
            Code->XiangMuPm[0].canshud = Cod.at(24);
            Code->XiangMuPm[1].xiangmu = Cod.at(25);
            Code->XiangMuPm[1].xiangmudanwei = Cod.at(26);
            Code->XiangMuPm[1].czuidizhi = Cod.at(27);
            Code->XiangMuPm[1].czuigaozhi = Cod.at(28);
            Code->XiangMuPm[1].WB_Para = Cod.at(29);
            Code->XiangMuPm[1].Equation_Limit = Cod.at(30);
            Code->XiangMuPm[1].Equation_Max = Cod.at(31);
            Code->XiangMuPm[1].NOP = Cod.at(32);
            Code->XiangMuPm[1].anongduzhi = Cod.at(33);
            Code->XiangMuPm[1].bnongduzhi = Cod.at(34);
            Code->XiangMuPm[1].cankaoxingshi = Cod.at(35);
            Code->XiangMuPm[1].nongdudizhi = Cod.at(36);
            Code->XiangMuPm[1].nongdugaozhi = Cod.at(37);
            Code->XiangMuPm[1].jisuanfangshi = Cod.at(38);
            Code->XiangMuPm[1].jisuanfangfa = Cod.at(39);
            Code->XiangMuPm[1].canshua = Cod.at(40);
            Code->XiangMuPm[1].canshub = Cod.at(41);
            Code->XiangMuPm[1].canshuc = Cod.at(42);
            Code->XiangMuPm[1].canshud = Cod.at(43);
            Code->XiangMuPm[2].xiangmu = Cod.at(44);
            Code->XiangMuPm[2].xiangmudanwei = Cod.at(45);
            Code->XiangMuPm[2].czuidizhi = Cod.at(46);
            Code->XiangMuPm[2].czuigaozhi = Cod.at(47);
            Code->XiangMuPm[2].WB_Para = Cod.at(48);
            Code->XiangMuPm[2].Equation_Limit = Cod.at(49);
            Code->XiangMuPm[2].Equation_Max = Cod.at(50);
            Code->XiangMuPm[2].NOP = Cod.at(51);
            Code->XiangMuPm[2].anongduzhi = Cod.at(52);
            Code->XiangMuPm[2].bnongduzhi = Cod.at(53);
            Code->XiangMuPm[2].cankaoxingshi = Cod.at(54);
            Code->XiangMuPm[2].nongdudizhi = Cod.at(55);
            Code->XiangMuPm[2].nongdugaozhi = Cod.at(56);
            Code->XiangMuPm[2].jisuanfangshi = Cod.at(57);
            Code->XiangMuPm[2].jisuanfangfa = Cod.at(58);
            Code->XiangMuPm[2].canshua = Cod.at(59);
            Code->XiangMuPm[2].canshub = Cod.at(60);
            Code->XiangMuPm[2].canshuc = Cod.at(61);
            Code->XiangMuPm[2].canshud = Cod.at(62);
            Code->XiangMuPm[3].xiangmu = Cod.at(63);
            Code->XiangMuPm[3].xiangmudanwei = Cod.at(64);
            Code->XiangMuPm[3].czuidizhi = Cod.at(65);
            Code->XiangMuPm[3].czuigaozhi = Cod.at(66);
            Code->XiangMuPm[3].WB_Para = Cod.at(67);
            Code->XiangMuPm[3].Equation_Limit = Cod.at(68);
            Code->XiangMuPm[3].Equation_Max = Cod.at(69);
            Code->XiangMuPm[3].NOP = Cod.at(70);
            Code->XiangMuPm[3].anongduzhi = Cod.at(71);
            Code->XiangMuPm[3].bnongduzhi = Cod.at(72);
            Code->XiangMuPm[3].cankaoxingshi = Cod.at(73);
            Code->XiangMuPm[3].nongdudizhi = Cod.at(74);
            Code->XiangMuPm[3].nongdugaozhi = Cod.at(75);
            Code->XiangMuPm[3].jisuanfangshi = Cod.at(76);
            Code->XiangMuPm[3].jisuanfangfa = Cod.at(77);
            Code->XiangMuPm[3].canshua = Cod.at(78);
            Code->XiangMuPm[3].canshub = Cod.at(79);
            Code->XiangMuPm[3].canshuc = Cod.at(80);
            Code->XiangMuPm[3].canshud = Cod.at(81);
            break;
        }
    }
    return true;
}
static bool getdat = false;

void PctGuiThread :: AnalyTest()
{
//    //------------------------------Add----07-11-14--------Record time-----------------------------------//
//    QString TimeData = "Test Start Time:           ";
//    QString Surrenttime = QTime::currentTime().toString("hh:mm:ss.zzz");
//    TimeData += Surrenttime + "\r\n";
//    RecordTime(TimeData);
//    qDebug() << "test start button need record time ";
//    //----------------------------------------------------------------------------------------------------------------//
    getdat = false;
    memset((char *)&AglDeviceP->AglDev.AdcCalc,0,sizeof(AdcCalcStr)*4);
    memset((char *)&AglDeviceP->AglDev.ChxBuf,0,sizeof(double)*4*2000);
    memset((char *)&AglDeviceP->AglDev.BakBuf,0,sizeof(double)*4*2000);
    memset((char *)&AglDeviceP->AglDev.VolBuf,0,sizeof(double)*4*2000);
//--------------------------------Add--------------------------------//
//    CheckTime->stop();
    Delayms(500);
    bool ok;
    AglDeviceP->agl_rd_pm();
    if((int)AglDeviceP->AglDev.ChxVol!=AnaFound.Xiangmushuliang.toInt(&ok, 10))
    {
        AglDeviceP->AglDev.ChxVol=AnaFound.Xiangmushuliang.toInt(&ok, 10);
        AglDeviceP->agl_wr_pm();
    }
//---------------------------------------------------------------------//
    if(isCalibration == true)
    {
        check_type = Check_Calib_Run;
        CheckTime->start(1000);
    }
    else
    {
        check_type = Check_Run;
        CheckTime->start(500);
        qDebug() << "Start run Readcheck Data";
    }


    AglDeviceP->agl_adc(AnaFound.Xiangmushuliang.toInt(&ok, 10));   // 启动采集
    AglDeviceP->AglDev.AdcState = 0x00;                                                    // 荧光采集状态（初始化为空闲）
    while(AglDeviceP->AglDev.AdcState != 0x02)
    {
        if(isYmaxError == true)
        {
            break;
        }
        AglDeviceP->agl_rd_stat();                                                                      //采集完成后读下位机状态
    }
    AglDeviceP->agl_get();                                                                                //读取采集数值
    getdat = true;
    return;
}

void PctGuiThread::AnalyValue()
{
    while(!getdat)
    {
        Delayms(100);
    }
    getdat = false;
    AglDeviceP->agl_calc_vol(AnaFound);                                                           //计算结果
//    if(isCalc == false || AglDeviceP->isCardError == true)
//    {
//        return;
//    }
//------Add-------------------------------------------------------------------------------------------------------------//
    TextDat = AglDeviceP->GetConfig("Test_sam");
    sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
    TextDat = dat;
    TextShow += TextDat;
    TextDat = Str_Pre;
    sprintf(dat, "%-31.31s", TextDat.toLatin1().data());
    TextDat = dat;
    TextShow += TextDat;
//------------------------------------------------------------------------------------------------------------------------//
    QString res_ref = AglDeviceP->GetConfig("res_ref");
    TextDat = res_ref;
    sprintf(dat, "%52.52s", TextDat.toLatin1().data());
    TextDat = dat;
    TextShow +=  "\r\n" + TextDat + "\r\n";
    if((int)AglDeviceP->AglDev.ChxVol == 1)                                                      // 如果是单卡 结果显示
    {
        //--------------------------------------------------------------------------------//
        QString Little = AnaFound.XiangMuPm[0].czuidizhi;               // 用来判断最终结果是否有效的最低值
        QString Big = AnaFound.XiangMuPm[0].czuigaozhi;               // 用来判断最终结果是否有效的最高值
        QString Show = AnaFound.XiangMuPm[0].cankaoxingshi;    // 判断范围的显示方式
        QString Zuigao= AnaFound.XiangMuPm[0].bnongduzhi;        // 正常取值区间低值
        QString Zuidi = AnaFound.XiangMuPm[0].anongduzhi;          // 正常取值区间高值
        bool ok;
        //--------------------------------------------------------------------------------//
        double Gas1 = AglDeviceP->AglDev.AdcCalc[0].YSumVol;
        QString Limit;

        if(AglDeviceP->AglDev.AdcCalc[0].CPeakVol < Little.toDouble(&ok) || AglDeviceP->AglDev.AdcCalc[0].CPeakVol > Big.toDouble(&ok))
        {
            Limit = " ";
            TextDat = AnaFound.XiangMuPm[0].xiangmu;
            sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat;
            gas1 = AglDeviceP->GetConfig("res_invalid");
            TextDat = gas1 + " " + Limit;
            sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat;
            QString limt1;
            if(Show == "<A")
            {
                limt1 = "<" + Zuigao;
            }
            else if(Show == "A-B")
            {
                limt1 =Zuidi + "-" + Zuigao;
            }

            TextDat = limt1;
            sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat + "\r\n";
        }
        else
        {
            TextDat = AnaFound.XiangMuPm[0].xiangmu;
            sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat;
            double dizhi = Zuidi.toDouble(&ok);
            double gaozhi = Zuigao.toDouble(&ok);
            if(Gas1 < dizhi || Gas1 > gaozhi)
            {
                Limit = " *";
                if(Gas1 < dizhi)
                {
                    OBX_8 = "L";
                    OBX_5 = "<" + Zuidi;
                }
                else
                {
                    OBX_8 = "H";
                    OBX_5 = ">" + Zuigao;
                }
                gas1 = OBX_5;
            }
            else
            {
                OBX_8 = "N";
                Limit = " ";
                sprintf(dat, "%-4.1f", Gas1);//sprintf(dat, "%-4.4f", Gas1);
                OBX_5 = dat;
                gas1 = dat;
            }

            OBX_4 =  AnaFound.XiangMuPm[0].xiangmu;
            OBX_6 = AnaFound.XiangMuPm[0].xiangmudanwei;
            if(Show == "<A")
            {
                OBX_7 = "< " + Zuigao;
            }
            else if(Show == "A-B")
            {
                OBX_7 = Zuidi + "-" + Zuigao;
            } 
            TextDat = gas1 + " " + AnaFound.XiangMuPm[0].xiangmudanwei + Limit;
            sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat;
            QString limt1 = OBX_7;
            TextDat = limt1+" "+AnaFound.XiangMuPm[0].xiangmudanwei;
            sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat + "\r\n";
        }
        CSV_information += gas1 + "#";
//        Interpre = gas1;
    }
    else if((int)AglDeviceP->AglDev.ChxVol == 4)          // 如果是4卡 结果显示
    {
//         AglDeviceP->Gas_Show.clear();
//         AglDeviceP->Gas_Show += "Sample ID is ";
//         AglDeviceP->Gas_Show += LineSam + ",";
        double Gas1 = AglDeviceP->AglDev.AdcCalc[0].YSumVol;
        double Gas2 = AglDeviceP->AglDev.AdcCalc[1].YSumVol;
        double Gas3 = AglDeviceP->AglDev.AdcCalc[2].YSumVol;
        double Gas4 = AglDeviceP->AglDev.AdcCalc[3].YSumVol;
//        double Gas1 =  AglDeviceP->GetConfig("PG1_Val").toDouble();
//        double Gas2 =  AglDeviceP->GetConfig("PG2_Val").toDouble();
//        double Gas3 =  AglDeviceP->GetConfig("HP_Val").toDouble();  // Hp
//        double Gas4 =  AglDeviceP->GetConfig("G17_Val").toDouble();  // G17
        double Gas1_Gas2 = 0;
        if(Gas2 == 0)
        {
            Gas1_Gas2 = 0;
        }
        else
        {
            Gas1_Gas2 = Gas1 / Gas2;
        }

        QString Limit;
        int Limit_PG1 = AglDeviceP->GetPasswd("@PG").toInt();   // 判断PG1，PG2，G17是否无效
        int Limit_Hp = AglDeviceP->GetPasswd("@HP").toInt();     // 判断HP是否无效
        bool isPG1Error = false;
        bool isPG2Error = false;

        bool isPG1Invalid = false;
        bool isPG2Invalid = false;
        bool isG17Invalid = false;
        bool isHpInvalid = false;
        bool ok = false;
//-------------PG1---------------//
        int PG1_Mea_Limit = AglDeviceP->GetConfig("PG1_Mea_Limit").toInt(&ok);
        if(ok == false)
        {
            PG1_Mea_Limit = 15;
        }
        int PG1_Mea_Max = AglDeviceP->GetConfig("PG1_Mea_Max").toInt(&ok);
        if(ok == false)
        {
            PG1_Mea_Max = 200;
        }
        int PG1_Ref_Limit = AglDeviceP->GetConfig("PG1_Ref_Limit").toInt(&ok);
        if(ok == false)
        {
            PG1_Ref_Limit = 30;
        }
        int PG1_Ref_Max = AglDeviceP->GetConfig("PG1_Ref_Max").toInt(&ok);
        if(ok == false)
        {
            PG1_Ref_Max = 60;
        }
        TextDat = AglDeviceP->GetConfig("Item1");            // "Pepsinogen I";
        sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
        TextDat = dat;
        TextShow += TextDat;        
        if(AglDeviceP->Card_index[0] == true)
        {
            isPG1Error = true;
            Limit = "";
            OBX_8 = "L";
            OBX_5 = AglDeviceP->GetConfig("res_Err");          // "Error";
            gas1 = OBX_5;
            OBX_4 = AglDeviceP->GetConfig("Item1");            // "Pepsinogen I";
            OBX_6 = AglDeviceP->GetConfig("PG1_Unit");      // "ng/ml";
            OBX_7 = AglDeviceP->GetConfig("PG1_Ref_Limit") + "-" + AglDeviceP->GetConfig("PG1_Ref_Max"); // "30-160";

            TextDat = gas1 + " " + Limit;
            sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat;
            QString limt1 = OBX_7;
             TextDat = limt1+" "+AglDeviceP->GetConfig("PG1_Unit"); // "ng/ml";
            sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat + "\r\n";

            CSV_information += OBX_5 + "#";

////-------------------------------------Add-11-16-----------------------------------------//
//            qDebug() << "PG1 record value";
//            QString Gas1_Str = OBX_5;
//            QString Gas1_show = "In Results Diaplay : PGI = ";
//             AglDeviceP->Gas_Show += Gas1_show + Gas1_Str;
////--------------------------------------------------------------------------------------------//
        }
//--------------------------------------------------PGI Invalid-------------------------------------------------//
        else if(AglDeviceP->AglDev.AdcCalc[0].CPeakVol < Limit_PG1)
        {
            isPG1Invalid  = true;
            Limit = "";
            OBX_8 = "L";
            OBX_5 = AglDeviceP->GetConfig("res_invalid");       // "Invalid";
            gas1 = OBX_5;
            OBX_4 = AglDeviceP->GetConfig("Item1");               // "Pepsinogen I";
            OBX_6 = AglDeviceP->GetConfig("PG1_Unit");         // "ng/ml";
            OBX_7 = AglDeviceP->GetConfig("PG1_Ref_Limit") + "-" + AglDeviceP->GetConfig("PG1_Ref_Max"); // "30-160";

            TextDat = gas1 + " " + Limit;
            sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat;
            QString limt1 = OBX_7;
            TextDat = limt1+" "+AglDeviceP->GetConfig("PG1_Unit"); // "ng/ml";
            sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat + "\r\n";

            CSV_information += OBX_5 + "#";
////-------------------------------------Add-11-16-----------------------------------------//
//            qDebug() << "PG1 record value";
//            QString Gas1_Str = OBX_5;
//            QString Gas1_show = "In Results Diaplay : PGI = ";
//             AglDeviceP->Gas_Show += Gas1_show + Gas1_Str;
////--------------------------------------------------------------------------------------------//
        }
//-----------------------------------------------------------------------------------------------------------------------//
        else
        {
////-------------------------------------Add-11-16-----------------------------------------//
//            qDebug() << "PG1 record value";
//            QString Gas1_Str = QString::number(Gas1);
//            QString Gas1_show = "In Results Diaplay : PGI = ";
//             AglDeviceP->Gas_Show += Gas1_show + Gas1_Str;
////--------------------------------------------------------------------------------------------//
//            AglDeviceP->Gas_Show += "The results of PG1 when compared = " + QString::number(Gas1) + "\r\n";
//            AglDeviceP->Gas_Show += "The reference range of PG1_Mea_Limit = " + QString::number(PG1_Mea_Limit) + "\r\n";
//            AglDeviceP->Gas_Show += "The reference range of PG1_Mea_Max = " + QString::number(PG1_Mea_Max) + "\r\n";
//            AglDeviceP->Gas_Show += "The reference range of PG1_Ref_Limit = " + QString::number(PG1_Ref_Limit) + "\r\n";
//            AglDeviceP->Gas_Show += "The reference range of PG1_Ref_Max = " + QString::number(PG1_Ref_Max) + "\r\n";
//            AglDeviceP->RecordInterpetation(AglDeviceP->Gas_Show);
//            AglDeviceP->Gas_Show.clear();

            if(Gas1 < PG1_Mea_Limit)  // 15.0
            {
                Limit = " "+AglDeviceP->GetConfig("DORR"); // " *";
                OBX_8 = "L";
                OBX_5 = "<" + AglDeviceP->GetConfig("PG1_Mea_Limit");  // "15";
                gas1 = OBX_5;
            }
            else if(Gas1 >= PG1_Mea_Limit && Gas1 < PG1_Ref_Limit) // (Gas1 >= 15.0 && Gas1 < 30.0)
            {
                Limit = " "+AglDeviceP->GetConfig("DORR");  // " *";
                OBX_8 = "N";
                if(Gas1 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas1); //sprintf(dat, "%-4.4f", Gas1);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas1); //sprintf(dat, "%-4.4f", Gas1);
                }
                OBX_5 = (QString)dat;
                gas1 = dat;
            }
            else if(Gas1 >= PG1_Ref_Limit && Gas1 < PG1_Ref_Max) // (Gas1 >= 30.0 && Gas1 < 160.0)
            {
                Limit = "";
                OBX_8 = "N";
                if(Gas1 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas1); //sprintf(dat, "%-4.4f", Gas1);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas1); //sprintf(dat, "%-4.4f", Gas1);
                }
                OBX_5 = (QString)dat;
                gas1 = dat;
            }
            else if(Gas1 >= PG1_Ref_Max && Gas1 < PG1_Mea_Max)  // (Gas1 >= 160.0 && Gas1 < 200.0)
            {
                Limit = " "+AglDeviceP->GetConfig("DORR"); // " *";
                OBX_8 = "N";
                if(Gas1 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas1); //sprintf(dat, "%-4.4f", Gas1);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas1); //sprintf(dat, "%-4.4f", Gas1);
                }
                OBX_5 = (QString)dat;
                gas1 = dat;
            }
            else
            {
                Limit = " "+AglDeviceP->GetConfig("DORR");     // " *";
                OBX_8 = "N";
                OBX_5 = ">" + AglDeviceP->GetConfig("PG1_Mea_Max"); // ">200";
                gas1 = OBX_5;
            }
            OBX_4 = AglDeviceP->GetConfig("Item1");   // "Pepsinogen I";
            OBX_6 = AglDeviceP->GetConfig("PG1_Unit");  // "ng/ml";
            OBX_7 = AglDeviceP->GetConfig("PG1_Ref_Limit") + "-" + AglDeviceP->GetConfig("PG1_Ref_Max");   // "30-160";

            TextDat = gas1 + " " + AglDeviceP->GetConfig("PG1_Unit") + Limit;  // "ng/ml"
            sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat;
            QString limt1 = OBX_7;
            TextDat = limt1+" "+AglDeviceP->GetConfig("PG1_Unit");   // "ng/ml";
            sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat + "\r\n";

            CSV_information += OBX_5 + "#";
        }
//-------------PG2---------------//
        int PG2_Mea_Limit = AglDeviceP->GetConfig("PG2_Mea_Limit").toInt(&ok);
        if(ok == false)
        {
            PG2_Mea_Limit = 3;
        }
        int PG2_Mea_Max = AglDeviceP->GetConfig("PG2_Mea_Max").toInt(&ok);
        if(ok == false)
        {
            PG2_Mea_Max = 60;
        }
        int PG2_Ref_Limit = AglDeviceP->GetConfig("PG2_Ref_Limit").toInt(&ok);
        if(ok == false)
        {
            PG2_Ref_Limit = 3;
        }
        int PG2_Ref_Max = AglDeviceP->GetConfig("PG2_Ref_Max").toInt(&ok);
        if(ok == false)
        {
            PG2_Ref_Max = 15;
        }
         TextDat = AglDeviceP->GetConfig("Item2");  // "Pepsinogen II";
         sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
         TextDat = dat;
         TextShow += TextDat;
        if(AglDeviceP->Card_index[1] == true)
        {
            isPG2Error = true;
            Limit = "";
            OBX_8_1 = "L";
            OBX_5_1 = AglDeviceP->GetConfig("res_Err");  // "Error";
            gas1 = OBX_5_1;

            OBX_4_1 = AglDeviceP->GetConfig("Item2");  // "Pepsinogen II";
            OBX_6_1 = AglDeviceP->GetConfig("PG2_Unit");  // "ng/ml";
            OBX_7_1 = AglDeviceP->GetConfig("PG2_Ref_Limit") + "-" + AglDeviceP->GetConfig("PG2_Ref_Max");  // "3-15";

            TextDat = gas1 + " " + Limit;
            sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat;
            QString limt2 = OBX_7_1;
            TextDat = limt2+" "+AglDeviceP->GetConfig("PG2_Unit");   // "ng/ml";
            sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat + "\r\n";

            CSV_information += OBX_5_1 + "#";

////-------------------------------------Add-11-16-----------------------------------------//
//            qDebug() << "PG2 record value";
//            QString Gas1_Str = OBX_5_1;
//            QString Gas1_show = ", PGII = ";
//             AglDeviceP->Gas_Show += Gas1_show + Gas1_Str;
////--------------------------------------------------------------------------------------------//
        }
//--------------------------------------------------PGII Invalid-------------------------------------------------//
        else if(AglDeviceP->AglDev.AdcCalc[1].CPeakVol < Limit_PG1)
        {
            isPG2Invalid = true;
            Limit = "";
            OBX_8_1 = "L";
            OBX_5_1 = AglDeviceP->GetConfig("res_invalid");  // "Invalid";
            gas1 = OBX_5_1;

            OBX_4_1 = AglDeviceP->GetConfig("Item2");   // "Pepsinogen II";
            OBX_6_1 = AglDeviceP->GetConfig("PG2_Unit");  // "ng/ml";
            OBX_7_1 = AglDeviceP->GetConfig("PG2_Ref_Limit") + "-" + AglDeviceP->GetConfig("PG2_Ref_Max"); // "3-15";

            TextDat = gas1 + " " + Limit;
            sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat;
            QString limt2 = OBX_7_1;
            TextDat = limt2+" "+AglDeviceP->GetConfig("PG2_Unit");  // "ng/ml";
            sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat + "\r\n";

            CSV_information += OBX_5_1 + "#";

////-------------------------------------Add-11-16-----------------------------------------//
//            qDebug() << "PG2 record value";
//            QString Gas1_Str = OBX_5_1;
//            QString Gas1_show = ", PGII = ";
//             AglDeviceP->Gas_Show += Gas1_show + Gas1_Str;
////--------------------------------------------------------------------------------------------//
        }
//-------------------------------------------------------------------------------------------------------------------//
        else
        {
////-------------------------------------Add-11-16-----------------------------------------//
//            qDebug() << "PG2 record value";
//            QString Gas1_Str = QString::number(Gas2);
//            QString Gas1_show = ", PGII = ";
//            AglDeviceP->Gas_Show += Gas1_show + Gas1_Str;
////--------------------------------------------------------------------------------------------//
            if(Gas2 < PG2_Ref_Limit)
            {
                Limit = " "+AglDeviceP->GetConfig("DORR");  //  " *";
                OBX_8_1 = "L";
                OBX_5_1 = "<" + AglDeviceP->GetConfig("PG2_Mea_Limit"); //  "3";
                gas1 = OBX_5_1;
            }
            else if(Gas2 >= PG2_Ref_Limit && Gas2 < PG2_Ref_Max)
            {
                Limit = "";
                OBX_8_1 = "N";
                if(Gas2 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas2); //sprintf(dat, "%-4.4f", Gas2);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas2); //sprintf(dat, "%-4.4f", Gas2);
                }
                OBX_5_1 = (QString)dat;
                gas1 = dat;
            }
            else if(Gas2 >= PG2_Ref_Max && Gas2 < PG2_Mea_Max)
            {
                Limit = " "+AglDeviceP->GetConfig("DORR");  // " *";
                OBX_8_1 = "N";
                if(Gas2 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas2); //sprintf(dat, "%-4.4f", Gas2);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas2); //sprintf(dat, "%-4.4f", Gas2);
                }
                OBX_5_1 = (QString)dat;
                gas1 = dat;
            }
            else
            {
                Limit = " "+AglDeviceP->GetConfig("DORR");   // " *";
                OBX_8_1 = "N";
                OBX_5_1 = ">" + AglDeviceP->GetConfig("PG2_Mea_Max"); // "60";
                gas1 = OBX_5_1;
            }

            OBX_4_1 = AglDeviceP->GetConfig("Item2");     //"Pepsinogen II";
            OBX_6_1 = AglDeviceP->GetConfig("PG2_Unit"); // "ng/ml";
            OBX_7_1 = AglDeviceP->GetConfig("PG2_Ref_Limit") + "-" + AglDeviceP->GetConfig("PG2_Ref_Max"); // "3-15";

            TextDat = gas1 + " " +AglDeviceP->GetConfig("PG2_Unit") + Limit;  // "ng/ml"
            sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat;
            QString limt2 = OBX_7_1;
            TextDat = limt2+" "+AglDeviceP->GetConfig("PG2_Unit");  // "ng/ml"
            sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
            TextDat = dat;
            TextShow += TextDat + "\r\n";

            CSV_information += OBX_5_1 + "#";
        }
//---------------PG1/PG2-----------------------//
        int PG1_2_Limit = AglDeviceP->GetConfig("PG1_2_Limit").toInt(&ok);
        if(ok == false)
        {
            PG1_2_Limit = 3;
        }
        int PG1_2_Max = AglDeviceP->GetConfig("PG1_2_Max").toInt(&ok);
        if(ok == false)
        {
            PG1_2_Max = 20;
        }
        TextDat = AglDeviceP->GetConfig("Item3");  // "Pepsinogen I/II";
        sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
        TextDat = dat;
        TextShow += TextDat;
       if(isPG1Error == true || isPG2Error == true)
       {
           Limit = "";
           OBX_8_b = "L";
           OBX_5_b = AglDeviceP->GetConfig("res_Err");  // "Error";
           gas1 = OBX_5_b;

           OBX_4_b =  AglDeviceP->GetConfig("Item3"); // "Pepsinogen I/II";
           OBX_6_b = " ";
           OBX_7_b = AglDeviceP->GetConfig("PG1_2_Limit") + "-" + AglDeviceP->GetConfig("PG1_2_Max"); // "3-20";

           TextDat = gas1 + Limit;
           sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
           TextDat = dat;
           TextShow += TextDat;
           QString limt3 = OBX_7_b;
           TextDat = limt3;
           sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
           TextDat = dat;
           TextShow += TextDat + "\r\n";
       }
//--------------------------------------------------PGI/II Invalid-------------------------------------------------//
       else if(isPG1Invalid == true || isPG2Invalid == true)
       {
           Limit = "";
           OBX_8_b = "L";
           OBX_5_b = AglDeviceP->GetConfig("res_invalid");  // "Invalid";
           gas1 = OBX_5_b;

           OBX_4_b =  AglDeviceP->GetConfig("Item3");  // "Pepsinogen I/II";
           OBX_6_b = " ";
           OBX_7_b = AglDeviceP->GetConfig("PG1_2_Limit") + "-" + AglDeviceP->GetConfig("PG1_2_Max"); // "3-20";

           TextDat = gas1 + Limit;
           sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
           TextDat = dat;
           TextShow += TextDat;
           QString limt3 = OBX_7_b;
           TextDat = limt3;
           sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
           TextDat = dat;
           TextShow += TextDat + "\r\n";
       }
//---------------------------------------------------------------------------------------------------------------------//
       else
       {
           if(Gas1_Gas2 == 0)
           {
               Limit = " " + AglDeviceP->GetConfig("DORR");  // " *";
               OBX_8_b = "L";
               OBX_5_b = "<" + AglDeviceP->GetConfig("PG1_2_Limit");  // "3";
               gas1 = OBX_5_b;
           }
           else if(Gas1_Gas2 < PG1_2_Limit && Gas1_Gas2 != 0)
           {
              Limit = " " + AglDeviceP->GetConfig("DORR");  // " *";
               OBX_8_b = "L";
               OBX_5_b = "<" + AglDeviceP->GetConfig("PG1_2_Limit"); // "<3";
               gas1 = OBX_5_b ;
           }
           else if(Gas1_Gas2 >= 3.0 && Gas1_Gas2 < 20.0)
           {
               Limit = " ";
               OBX_8_b = "N";
               if(Gas1_Gas2 >= 100)
               {
                   sprintf(dat, "%-1.0f", Gas1_Gas2); //sprintf(dat, "%-4.4f", Gas1_Gas2);
               }
               else
               {
                   sprintf(dat, "%-1.1f", Gas1_Gas2); //sprintf(dat, "%-4.4f", Gas1_Gas2);
               }
               OBX_5_b = (QString)dat;
               gas1 = dat;
           }
           else
           {
               Limit = " " + AglDeviceP->GetConfig("DORR");  // " *";
               OBX_8_b = "N";
               OBX_5_b = ">" + AglDeviceP->GetConfig("PG1_2_Max");  // ">20";
               gas1 = OBX_5_b;
           }

           OBX_4_b =  AglDeviceP->GetConfig("Item3");  // "Pepsinogen I/II";
           OBX_6_b = " ";
           OBX_7_b = AglDeviceP->GetConfig("PG1_2_Limit") + "-" + AglDeviceP->GetConfig("PG1_2_Max");  // "3-20";

           TextDat = gas1 + Limit;
           sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
           TextDat = dat;
           TextShow += TextDat;
           QString limt3 = OBX_7_b;
           TextDat = limt3;
           sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
           TextDat = dat;
           TextShow += TextDat + "\r\n";
       }
//--------------------G17--------------------------//
       int G17_Mea_Limit = AglDeviceP->GetConfig("G17_Mea_Limit").toInt(&ok);
       if(ok == false)
       {
           G17_Mea_Limit = 1;
       }
       int G17_Mea_Max = AglDeviceP->GetConfig("G17_Mea_Max").toInt(&ok);
       if(ok == false)
       {
           G17_Mea_Max = 40;
       }
       int G17_Ref_Limit = AglDeviceP->GetConfig("G17_Ref_Limit").toInt(&ok);
       if(ok == false)
       {
           G17_Ref_Limit = 1;
       }
       int G17_Ref_Max = AglDeviceP->GetConfig("G17_Ref_Max").toInt(&ok);
       if(ok == false)
       {
           G17_Ref_Max = 7;
       }
       TextDat = AglDeviceP->GetConfig("Item4");  // "Gastrin-17";
       sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
       TextDat = dat;
       TextShow += TextDat;
      if(AglDeviceP->Card_index[3] == true)
      {
          Limit = "";
          OBX_8_2 = "L";
          OBX_5_2 = AglDeviceP->GetConfig("res_Err");        // "Error";
          gas1 = OBX_5_2;

          OBX_4_2= AglDeviceP->GetConfig("Item4");           // "Gastrin-17B";
          OBX_6_2 = AglDeviceP->GetConfig("G17_Unit");      // "pmol/l";
          OBX_7_2 = AglDeviceP->GetConfig("G17_Ref_Limit")+"-"+AglDeviceP->GetConfig("G17_Ref_Max");  // "1-7";

          TextDat = gas1 + " " + Limit;
          sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat;
          QString limt4 = OBX_7_2;

          TextDat = limt4+" "+AglDeviceP->GetConfig("G17_Unit");  // "pmol/l";
          sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat + "\r\n";

          CSV_information += OBX_5_2 + "#";

//  //-------------------------------------Add-11-16-----------------------------------------//
//            qDebug() << "G-17 record value";
//            QString Gas1_Str = OBX_5_2;
//            QString Gas1_show ;
//            if(Interpretation_post == 1)
//            {
//               Gas1_show = ",Select Postprandial Post = 1; G-17 = " + Gas1_Str;
//            }
//            else
//            {
//                Gas1_show = ",Select Fasting Post = 0; G-17 = " + Gas1_Str;
//            }

//             AglDeviceP->Gas_Show += Gas1_show + Gas1_Str;
//  //--------------------------------------------------------------------------------------------//
      }
//--------------------------------------------------G17 Invalid-------------------------------------------------//
      else if(AglDeviceP->AglDev.AdcCalc[3].CPeakVol < Limit_PG1)
      {
          isG17Invalid = true;
          Limit = "";
          OBX_8_2 = "L";
          OBX_5_2 = AglDeviceP->GetConfig("res_invalid");  // "Invalid";
          gas1 = OBX_5_2;

          OBX_4_2 = AglDeviceP->GetConfig("Item4");  // "Gastrin-17B";
          OBX_6_2 = AglDeviceP->GetConfig("G17_Unit");  // "pmol/l";
          OBX_7_2 = AglDeviceP->GetConfig("G17_Ref_Limit")+"-"+AglDeviceP->GetConfig("G17_Ref_Max");  // "1-7";

          TextDat = gas1 + " "+ Limit;
          sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat;
          QString limt4 = OBX_7_2;

//          TextDat = limt4+" "+AglDeviceP->GetConfig("G17_Unit"); // "pmol/l";
          TextDat = limt4+" "; // "pmol/l";
          sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat + "\r\n";

          CSV_information += OBX_5_2 + "#";

//  //-------------------------------------Add-11-16-----------------------------------------//
//            qDebug() << "G-17 record value";
//            QString Gas1_Str = OBX_5_2;
//            QString Gas1_show ;
//            if(Interpretation_post == 1)
//            {
//               Gas1_show = ",Select Postprandial Post = 1; G-17 = " + Gas1_Str;
//            }
//            else
//            {
//                Gas1_show = ",Select Fasting Post = 0; G-17 = " + Gas1_Str;
//            }

//             AglDeviceP->Gas_Show += Gas1_show + Gas1_Str;
//  //--------------------------------------------------------------------------------------------//
      }
//-----------------------------------------------------------------------------------------------------------------//
      else
      {
////-------------------------------------Add-11-16-----------------------------------------//
//          qDebug() << "G-17 record value";
//          sprintf(dat, "%.1f", Gas4);
//          QString Gas1_Str = QString::number(Gas4);
//          QString Gas1_show ;
//          if(Interpretation_post == 1)
//          {
//             Gas1_show = ",Select Postprandial Post = 1; G-17 = " + Gas1_Str;
//          }
//          else
//          {
//              Gas1_show = ",Select Fasting Post = 0; G-17 = " + Gas1_Str;
//          }

//           AglDeviceP->Gas_Show += Gas1_show;
////--------------------------------------------------------------------------------------------//
          if(Gas4 < G17_Ref_Limit)
          {
              Limit = " "+AglDeviceP->GetConfig("DORR");  // " *";
              OBX_8_2 = "L";
              OBX_5_2 = "<" + AglDeviceP->GetConfig("G17_Ref_Limit");  // "<1"
              gas1 = OBX_5_2;
          }
          else if(Gas4 >= G17_Ref_Limit && Gas4 < G17_Ref_Max)
          {
              Limit = " ";
              OBX_8_2 = "N";
              if(Gas4 >= 100)
              {
                 sprintf(dat, "%-1.0f", Gas4); //sprintf(dat, "%-4.4f", Gas4);
              }
              else
              {
                  sprintf(dat, "%-1.1f", Gas4); //sprintf(dat, "%-4.4f", Gas4);
              }
              OBX_5_2 = (QString)dat;
              gas1 = dat;
          }
          else if(Gas4 >= G17_Ref_Max && Gas4 < G17_Mea_Max)
          {
              Limit = " "+AglDeviceP->GetConfig("DORR");   // " *";
              OBX_8_2 = "N";
              if(Gas4 >= 100)
              {
                 sprintf(dat, "%-1.0f", Gas4); //sprintf(dat, "%-4.4f", Gas4);
              }
              else
              {
                  sprintf(dat, "%-1.1f", Gas4); //sprintf(dat, "%-4.4f", Gas4);
              }
              OBX_5_2 = (QString)dat;
              gas1 = dat;
          }
          else if(Gas4 >= G17_Mea_Max)
          {
              Limit = " "+AglDeviceP->GetConfig("DORR");  // " *";
              OBX_8_2 = "N";
              OBX_5_2 = ">" + AglDeviceP->GetConfig("G17_Mea_Max");  // ">40";
              gas1 = OBX_5_2;
          }

          OBX_4_2 = AglDeviceP->GetConfig("Item4");   // "Gastrin-17B";
          OBX_6_2 = AglDeviceP->GetConfig("G17_Unit");    // "pmol/l";
          OBX_7_2 = AglDeviceP->GetConfig("G17_Ref_Limit")+"-"+AglDeviceP->GetConfig("G17_Ref_Max"); // "1-7";

          TextDat = gas1 + " " +AglDeviceP->GetConfig("G17_Unit") + Limit;  // "pmol/l"
          sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat;
          QString limt4 = OBX_7_2;

          TextDat = limt4+" "+AglDeviceP->GetConfig("G17_Unit");  // "pmol/l";
          sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat + "\r\n";

          CSV_information += OBX_5_2 + "#";
      }
//-------------------Hp------------------//
      int HP_Mea_Limit = AglDeviceP->GetConfig("HP_Mea_Limit").toInt(&ok);
      if(ok == false)
      {
          HP_Mea_Limit = 15;
      }
      int HP_Mea_Max = AglDeviceP->GetConfig("HP_Mea_Max").toInt(&ok);
      if(ok == false)
      {
          HP_Mea_Max = 700;
      }
      int HP_Ref_Max = AglDeviceP->GetConfig("HP_Ref_Max").toInt(&ok);
      if(ok == false)
      {
          HP_Ref_Max = 30;
      }

      TextDat = AglDeviceP->GetConfig("Item5"); // "H.pylori";
      sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
      TextDat = dat;
      TextShow += TextDat;
      if(AglDeviceP->Card_index[2] == true)
      {
          Limit = "";
          OBX_8_3 = "N";
          OBX_5_3 = AglDeviceP->GetConfig("res_Err");  // "Error";
          gas1 = OBX_5_3;

          OBX_4_3 = AglDeviceP->GetConfig("Item5");  // "H.pylori";
          OBX_6_3 = AglDeviceP->GetConfig("HP_Unit");  // "EIU";
          OBX_7_3 = "<" + AglDeviceP->GetConfig("HP_Ref_Max"); // "30"

          TextDat = gas1 + " " + Limit;
          sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat;
          QString limt5= OBX_7_3;

          TextDat = limt5+" "+AglDeviceP->GetConfig("HP_Unit"); // "EIU";
          sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat + "\r\n";

          CSV_information += OBX_5_3 + "#";

//      //-------------------------------------Add-11-16-----------------------------------------//
//            qDebug() << "Hp record value";
//            QString Gas1_Str = OBX_5_3;
//            QString Gas1_show = ",Hp = " + Gas1_Str;
//            AglDeviceP->Gas_Show += Gas1_show + Gas1_Str + "\r\n";
//      //--------------------------------------------------------------------------------------------//
      }
//---------------------------------------------------Hp Invalid--------------------------------------------------//
      else if(AglDeviceP->AglDev.AdcCalc[2].CPeakVol < Limit_Hp)
      {
          isHpInvalid = true;
          Limit = "";
          OBX_8_3 = "N";
          OBX_5_3 = AglDeviceP->GetConfig("res_invalid");  // "Invalid";
          gas1 = OBX_5_3;

          OBX_4_3 = AglDeviceP->GetConfig("Item5");  // "H.pylori";
          OBX_6_3 = AglDeviceP->GetConfig("HP_Unit");  // "EIU";
          OBX_7_3 = "<" + AglDeviceP->GetConfig("HP_Ref_Max"); // "<30";

          TextDat = gas1 + " "+ Limit;
          sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat;
          QString limt5= OBX_7_3;

          TextDat = limt5+" "+AglDeviceP->GetConfig("HP_Unit"); // "EIU";
          sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat + "\r\n";

          CSV_information += OBX_5_3 + "#";

//    //-------------------------------------Add-11-16-----------------------------------------//
//          qDebug() << "Hp record value";
//          QString Gas1_Str = OBX_5_3;
//          QString Gas1_show = ",Hp = " + Gas1_Str;
//          AglDeviceP->Gas_Show += Gas1_show + Gas1_Str + "\r\n";
//    //--------------------------------------------------------------------------------------------//
      }
//----------------------------------------------------------------------------------------------------------------------//
      else
      {
////-------------------------------------Add-11-16-----------------------------------------//
//        qDebug() << "Hp record value";
//        sprintf(dat, "%.1f", Gas3);
//        QString Gas1_Str = QString::number(Gas3);
//        QString Gas1_show = ",Hp = " + Gas1_Str;
//        AglDeviceP->Gas_Show += Gas1_show + "\r\n";
////--------------------------------------------------------------------------------------------//
          if(Gas3 < HP_Mea_Limit) // < 15
          {
              Limit = "";  // "";
              OBX_8_3 = "L";
              OBX_5_3 = "<" + AglDeviceP->GetConfig("HP_Mea_Limit"); // "<15";
              gas1 = OBX_5_3;
          }
          else if(Gas3 >= HP_Mea_Limit && Gas3 < HP_Ref_Max) // 15 - 30
          {
              Limit = "";
              OBX_8_3 = "N";
              if(Gas3 >= 100)
              {
                  sprintf(dat, "%-1.0f", Gas3); //sprintf(dat, "%-4.4f", Gas3);
              }
              else
              {
                  sprintf(dat, "%-1.1f", Gas3); //sprintf(dat, "%-4.4f", Gas3);
              }
              OBX_5_3 = (QString)dat;
              gas1 = dat;
          }
          else if(Gas3 >= HP_Ref_Max && Gas3 < HP_Mea_Max)  // 30 - 700
          {
              Limit = " " + AglDeviceP->GetConfig("DORR"); //" *";
              OBX_8_3 = "N";
              if(Gas3 >= 100)
              {
                  sprintf(dat, "%-1.0f", Gas3); //sprintf(dat, "%-4.4f", Gas3);
              }
              else
              {
                  sprintf(dat, "%-1.1f", Gas3); //sprintf(dat, "%-4.4f", Gas3);
              }
              OBX_5_3 = (QString)dat;
              gas1 = dat;
          }
          else if(Gas3 >= HP_Mea_Max)
          {
              Limit = " " + AglDeviceP->GetConfig("DORR");   // " *";
              OBX_8_3 = "N";
              OBX_5_3 = ">" + AglDeviceP->GetConfig("HP_Mea_Max");  // ">700";
              gas1 = OBX_5_3;
          }

          OBX_4_3 = AglDeviceP->GetConfig("Item5");  // "H.pylori";
          OBX_6_3 = AglDeviceP->GetConfig("HP_Unit");  // "EIU";
          OBX_7_3 = "<" + AglDeviceP->GetConfig("HP_Ref_Max");  // "<30";

          TextDat = gas1 + " " + AglDeviceP->GetConfig("HP_Unit") + Limit;  // "EIU"
          sprintf(dat, "%-19.19s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat;
          QString limt5= OBX_7_3;
          TextDat = limt5+" "+AglDeviceP->GetConfig("HP_Unit");  // "EIU";
          sprintf(dat, "%-18.18s", TextDat.toLatin1().data());
          TextDat = dat;
          TextShow += TextDat + "\r\n";

          CSV_information += OBX_5_3 + "#";
      }   
//      QDateTime time = QDateTime :: currentDateTime();
//      QString now = time.toString("yyyy-MM-dd hh:mm:ss");
//      QString ZZ = QTime::currentTime().toString("zzz");
//      QString compele = now+ "." + ZZ + "     ";
//      AglDeviceP->Gas_Show = compele + AglDeviceP->Gas_Show;

       int PG1NAN = isnan(Gas1);
       int PG1INF = isinf(Gas1);
       int PG2NAN = isnan(Gas2);
       int PG2INF = isinf(Gas2);
       int HpNAN = isnan(Gas3);
       int HpINF = isinf(Gas3);
       int G17NAN = isnan(Gas4);
       int G17INF = isinf(Gas4);

       if(PG1NAN || PG1INF || PG2NAN || PG2INF || HpNAN || HpINF || G17NAN || G17INF)
       {
//           //---------------------------------------------Add-11-16-------------------------------------------//
//           QString isNAn = "the value is nan or inf , Interparetation loss!\r\n";
//           AglDeviceP->Gas_Show += isNAn;
//           //-------------------------------------------------------------------------------------------------------//
           isPG1Invalid = true;
       }

        QString End;
        if(isPG1Invalid == true || isPG2Invalid == true || isG17Invalid == true || isHpInvalid == true)
        {
//            //---------------------------------------------Add-11-16-------------------------------------------//
//            QString isNAn = "the value is invalid , Interparetation loss!\r\n";
//             AglDeviceP->Gas_Show += isNAn;
//            //-------------------------------------------------------------------------------------------------------//
            End.clear();
        }
        else
        {
            std::string Inter = AglDeviceP->GetInterpapreTation(Gas1, Gas2, Gas4, Gas3, Interpretation_post);
            End = QString::fromStdString(Inter);
//            End = AglDeviceP->agl_calc_string(Gas1, Gas2, Gas3, Gas4, Interpretation_post);
        }

//        QString Ret = "\r\n===========================================================================================================\r\n";
//        AglDeviceP->Gas_Show += Ret;
//        AglDeviceP->RecordInterpetation(AglDeviceP->Gas_Show);

        if(End.isEmpty() == false)
        {
            End = "\r\n" + End + "\r\n";
        }
        TextShow += End + "\r\n";

        TextShow += AglDeviceP->GetConfig("report_oper");
        TextShow += " "+ LoginUserName;

        TextShow += "       " + AglDeviceP->GetConfig("Repotr_LOT");
        TextShow += " " + AnaFound.Pihao;

        CSV_information += LoginUserName + "#" + AnaFound.Pihao + "#";
        if(!(isPG1Invalid == true || isPG2Invalid == true || isG17Invalid == true || isHpInvalid == true))
        {
            CSV_information += AglDeviceP->GetCsvInterpretation(Gas1, Gas2, Gas3, Gas4, Interpretation_post);
        }
    }
//-----Add--------------------------------------------------------------------------------------------------------------------------//
    QString Str_Dat;
    QString SText;
    QString Limit;
    Str_line.clear();

    sprintf(dat, "%-13.13s", (AglDeviceP->GetConfig("Res_Sam")).toStdString().data());
    Str_Dat = dat;
    SText = Str_Dat + SamIDShow;
    Str_line += SText + "\r\n";

    sprintf(dat, "%-13.13s", AglDeviceP->GetConfig("Test_sam").toStdString().data());
    Str_Dat = dat;
    SText = Str_Dat + Str_Pre;
    Str_line += SText + "\r\n";

    sprintf(dat, "%32.32s", AglDeviceP->GetConfig("Print_Ref").toStdString().data());
    Str_Dat = dat;
    Str_line += Str_Dat + "\r\n";

    if((int)AglDeviceP->AglDev.ChxVol == 1)    // 打印结果
    {
        //----------Add-------------------------------------------------------//
        QString res_vail = AglDeviceP->GetConfig("res_invalid");

        QString Danwei = AnaFound.XiangMuPm[0].xiangmu;
        QString Little = AnaFound.XiangMuPm[0].czuidizhi;           // 用来判断最终结果是否有效的最低值
        QString Big = AnaFound.XiangMuPm[0].czuigaozhi;           // 用来判断最终结果是否有效的最高值
        QString Show = AnaFound.XiangMuPm[0].cankaoxingshi; // 判断范围的显示方式

        QString Zuigao= AnaFound.XiangMuPm[0].bnongduzhi;       // 正常取值区间低值
        QString Zuidi = AnaFound.XiangMuPm[0].anongduzhi;   // 正常取值区间高值

        bool ok;
        //----------------------------------------------------------------------//
        if(AglDeviceP->AglDev.AdcCalc[0].CPeakVol < Little.toDouble(&ok) || AglDeviceP->AglDev.AdcCalc[0].CPeakVol > Big.toDouble(&ok))
        {
            SText.clear();
            SText += Danwei+"=" + res_vail;
            sprintf(dat, "%-24.24s", SText.toLatin1().data());
            Str_Dat = dat;
            if(Show == "<A")
            {
                Str_line += Str_Dat + "(< " + Zuigao + ")\r\n";
            }
            else if(Show == "A-B")
            {
                Str_line += Str_Dat + "(" + Zuidi +  "-" + Zuigao + ")\r\n";
            }
        }
        else
        {
            if((AglDeviceP->AglDev.AdcCalc[0].YSumVol) > Zuigao.toDouble(&ok))
            {
                Limit = "*";
            }
            else if((AglDeviceP->AglDev.AdcCalc[0].YSumVol) < Zuidi.toDouble(&ok))
            {
                Limit = "*";
            }
            else
            {
                 Limit = " ";
            }
            SText.clear();
            double res1 = (AglDeviceP->AglDev.AdcCalc[0].YSumVol);
            sprintf(dat, "%-4.1f", res1);// sprintf(dat, "%-4.4f", res1);
            Str_Dat = dat;
            SText += AnaFound.XiangMuPm[0].xiangmu+"=";
            SText += Str_Dat + " ";
            SText += AnaFound.XiangMuPm[0].xiangmudanwei+ " " + Limit;
            sprintf(dat, "%-24.24s", SText.toLatin1().data());
            Str_Dat = dat;
            if(Show == "<A")
            {
                Str_line += Str_Dat + "(< " + Zuigao + ")\r\n";
                qDebug() << "Zuigao is " << Zuigao;
            }
            else if(Show == "A-B")
            {
                Str_line += Str_Dat + "(" + Zuidi +  "-" + Zuigao + ")\r\n";
            }
        }

        Str_Flag.clear();
        Str_Flag += "\r\n";
        Str_Flag += "Lot Number " + AnaFound.Pihao + "\r\n";

        Str_Flag += "T0 = ";
        double Dou_T0 = AglDeviceP->AglDev.AdcCalc[0].TPeakVol;
        sprintf(dat, "%-4.0f", Dou_T0);
        Str_Flag +=  dat;

        Str_Flag += "\r\nC0 = ";
        double Dou_C0 = AglDeviceP->AglDev.AdcCalc[0].CPeakVol;
        sprintf(dat, "%-4.0f", Dou_C0);
        Str_Flag +=  dat;
        Str_Flag += "\r\n";
    }

    if((int)AglDeviceP->AglDev.ChxVol == 4)
    {
        SText.clear();
        double Gas1 =  AglDeviceP->AglDev.AdcCalc[0].YSumVol;
        double Gas2 =  AglDeviceP->AglDev.AdcCalc[1].YSumVol;
        double Gas3 =  AglDeviceP->AglDev.AdcCalc[2].YSumVol;  // Hp
        double Gas4 =  AglDeviceP->AglDev.AdcCalc[3].YSumVol;  // G17

//        double Gas1 =  AglDeviceP->GetConfig("PG1_Val").toDouble();
//        double Gas2 =  AglDeviceP->GetConfig("PG2_Val").toDouble();
//        double Gas3 =  AglDeviceP->GetConfig("G17_Val").toDouble();  // Hp
//        double Gas4 =  AglDeviceP->GetConfig("PG1_Val").toDouble();  // G17

        double Gas1_Gas2 = 0;
        if(Gas2 != 0)
        {
            Gas1_Gas2 = Gas1 / Gas2;
        }

        int Limit_PG1 = AglDeviceP->GetPasswd("@PG").toInt();
        int Limit_Hp = AglDeviceP->GetPasswd("@HP").toInt();
        bool isPG1Error = false;
        bool isPG2Error = false;
        bool isPG1Invalid = false;
        bool isPG2Invalid = false;
        bool isHpError = false;
        bool isG17Error = false;
        bool isHpInvalid = false;
        bool isG17Invalid = false;
        bool ok = false;
//-----------------PG1--------------------//  打印
        int PG1_Mea_Limit = AglDeviceP->GetConfig("PG1_Mea_Limit").toInt(&ok);
        if(ok == false)
        {
            PG1_Mea_Limit = 15;
        }
        int PG1_Mea_Max = AglDeviceP->GetConfig("PG1_Mea_Max").toInt(&ok);
        if(ok == false)
        {
            PG1_Mea_Max = 200;
        }
        int PG1_Ref_Limit = AglDeviceP->GetConfig("PG1_Ref_Limit").toInt(&ok);
        if(ok == false)
        {
            PG1_Ref_Limit = 30;
        }
        int PG1_Ref_Max = AglDeviceP->GetConfig("PG1_Ref_Max").toInt(&ok);
        if(ok == false)
        {
            PG1_Ref_Max = 160;
        }
        Str_Dat = AglDeviceP->GetConfig("Print_Item1");  //"PGI";
        sprintf(dat, "%-8.8s", Str_Dat.toLatin1().data());
        Str_Dat = dat;
        SText += Str_Dat;
        if(AglDeviceP->Card_index[0] == true)
        {
            isPG1Error = true;
            Limit = "";
            Str_Dat = AglDeviceP->GetConfig("res_Err") + Limit; // "Error"
            sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
            Str_Dat = dat;
            SText += Str_Dat;
        }
//-----------------------------------------------PGI Invalid--------------------------------------------//
        else if(AglDeviceP->AglDev.AdcCalc[0].CPeakVol < Limit_PG1)
        {
            isPG1Invalid = true;
            Limit = "";
            Str_Dat = AglDeviceP->GetConfig("res_invalid") + Limit; //"Invalid"
            sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
            Str_Dat = dat;
            SText += Str_Dat;
        }
//-----------------------------------------------------------------------------------------------------------//
        else
        {
            if(Gas1 < PG1_Mea_Limit) //15
            {
                Limit = " " + AglDeviceP->GetConfig("DORR"); // " *";
                Str_Dat = "<"+AglDeviceP->GetConfig("PG1_Mea_Limit")+" "+AglDeviceP->GetConfig("PG1_Unit")+Limit; // "<15 ng/ml"
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas1 >= PG1_Mea_Limit && Gas1 < PG1_Ref_Limit)   // 15, 30
            {
                Limit = " " + AglDeviceP->GetConfig("DORR"); // " *";
                if(Gas1 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas1);//sprintf(dat, "%-4.4f", res1);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas1);//sprintf(dat, "%-4.4f", res1);
                }
                Str_Dat = (QString)dat + " " + AglDeviceP->GetConfig("PG1_Unit")+Limit;  // " ng/ml";
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas1 >= PG1_Ref_Limit && Gas1 < PG1_Ref_Max) // 30, 160
            {
                Limit = " ";
                if(Gas1 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas1);//sprintf(dat, "%-4.4f", res1);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas1);//sprintf(dat, "%-4.4f", res1);
                }
                Str_Dat = (QString)dat + " " + AglDeviceP->GetConfig("PG1_Unit");  // " ng/ml";
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas1 >= PG1_Ref_Max && Gas1 < PG1_Mea_Max) // 160, 200
            {
                Limit = " " + AglDeviceP->GetConfig("DORR");    // " *";
                if(Gas1 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas1);//sprintf(dat, "%-4.4f", res1);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas1);//sprintf(dat, "%-4.4f", res1);
                }
                Str_Dat = (QString)dat + " " + AglDeviceP->GetConfig("PG1_Unit")+Limit;  // " ng/ml";
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else
            {
                Limit = " " + AglDeviceP->GetConfig("DORR");    // " *";
                Str_Dat = ">"+ AglDeviceP->GetConfig("PG1_Mea_Max")+" "+AglDeviceP->GetConfig("PG1_Unit")+Limit;  // ">200 ng/ml"
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
        }
        Str_line += SText + AglDeviceP->GetConfig("PG1_Ref_Limit")+"-"+ AglDeviceP->GetConfig("PG1_Ref_Max")+"\r\n";// "30-160\r\n\r\n";
//---------------------------PG2-----------------------------//
        int PG2_Mea_Limit = AglDeviceP->GetConfig("PG2_Mea_Limit").toInt(&ok);
        if(false == ok)
        {
            PG2_Mea_Limit = 3;
        }
        int PG2_Mea_Max = AglDeviceP->GetConfig("PG2_Mea_Max").toInt(&ok);
        if(false == ok)
        {
            PG2_Mea_Max = 60;
        }
        int PG2_Ref_Limit = AglDeviceP->GetConfig("PG2_Ref_Limit").toInt(&ok);
        if(false == ok)
        {
            PG2_Ref_Limit = 3;
        }
        int PG2_Ref_Max = AglDeviceP->GetConfig("PG2_Ref_Max").toInt(&ok);
        if(false == ok)
        {
            PG2_Ref_Max = 15;
        }
        SText.clear();
        Str_Dat = AglDeviceP->GetConfig("Print_Item2");  // "PGII";
        sprintf(dat, "%-8.8s", Str_Dat.toLatin1().data());
        Str_Dat = dat;
        SText += Str_Dat;
        if(AglDeviceP->Card_index[1] == true)
        {
            isPG2Error = true;
            Limit = "";
            Str_Dat = AglDeviceP->GetConfig("res_Err") + Limit;  // "Error"
            sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
            Str_Dat = dat;
            SText += Str_Dat;
        }
//------------------------------------------------PGII Invalid-----------------------------------------//
        else if(AglDeviceP->AglDev.AdcCalc[1].CPeakVol < Limit_PG1)
        {
            isPG2Invalid = true;
            Limit = "";
            Str_Dat = AglDeviceP->GetConfig("res_invalid") + Limit; // "Invalid"
            sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
            Str_Dat = dat;
            SText += Str_Dat;
        }
//--------------------------------------------------------------------------------------------------------//
        else
        {
            if(Gas2 < PG2_Mea_Limit)  //< 3
            {
                Limit = " " + AglDeviceP->GetConfig("DORR");   // " *";
                Str_Dat = "<" + AglDeviceP->GetConfig("PG2_Mea_Limit") + " " + AglDeviceP->GetConfig("PG2_Unit") + Limit;  // "<3 ng/ml"
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas2 >= PG2_Ref_Limit && Gas2 < PG2_Ref_Max) // 3 <= gas2 <= 15
            {
                Limit = " ";
                sprintf(dat, "%-1.1f", Gas2);//sprintf(dat, "%-4.4f", res1);
                Str_Dat = (QString)dat + " " + AglDeviceP->GetConfig("PG2_Unit");       // " ng/ml";
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas2 >= PG2_Ref_Max && Gas2 < PG2_Mea_Max) // 15 <= Gas2 <= 60
            {
                Limit = " " + AglDeviceP->GetConfig("DORR");      // " *";
                if(Gas2 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas2);//sprintf(dat, "%-4.4f", Gas2);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas2);//sprintf(dat, "%-4.4f", Gas2);
                }
                Str_Dat = (QString)dat + " " + AglDeviceP->GetConfig("PG2_Unit")+Limit;       // " ng/ml";
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else //  > 60
            {
                Limit = " " + AglDeviceP->GetConfig("DORR");      // " *";
                Str_Dat = ">"+AglDeviceP->GetConfig("PG2_Mea_Max")+" "+AglDeviceP->GetConfig("PG2_Unit")+Limit;  // ">60 ng/ml"
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
        }
        Str_line += SText+AglDeviceP->GetConfig("PG2_Ref_Limit")+"-"+AglDeviceP->GetConfig("PG2_Ref_Max")+"\r\n";   // "3-15\r\n\r\n";
//---------------------------PG1/2-----------------------------//
        SText.clear();
        int PG1_2_Limit = AglDeviceP->GetConfig("PG1_2_Limit").toInt(&ok);
        if(false == ok)
        {
            PG1_2_Limit = 3;
        }
        int PG1_2_Max = AglDeviceP->GetConfig("PG1_2_Max").toInt(&ok);
        if(false == ok)
        {
            PG1_2_Max = 20;
        }
        Str_Dat = AglDeviceP->GetConfig("Print_Item3");  // "PGI/II"
        sprintf(dat, "%-8.8s", Str_Dat.toLatin1().data());
        Str_Dat = dat;
        SText += Str_Dat;
        if(isPG1Error == true || isPG2Error == true)
        {
            Limit = "";
            Str_Dat = AglDeviceP->GetConfig("res_Err") + Limit;     //  "Error"
            sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
            Str_Dat = dat;
            SText += Str_Dat;
        }
//-------------------------------------PGI/II Invalid----------------------------------------//
        else if(isPG1Invalid == true || isPG2Invalid == true)
        {
            Limit = "";
            Str_Dat = AglDeviceP->GetConfig("res_invalid") + Limit;   // "Invalid"
            sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
            Str_Dat = dat;
            SText += Str_Dat;
        }
//-----------------------------------------------------------------------------------------------//
        else
        {
            if(Gas1_Gas2 == 0)
            {
                Limit = "";
                Str_Dat = AglDeviceP->GetConfig("res_invalid") + Limit;   // "Invalid"
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas1_Gas2 < PG1_2_Limit)
            {
                Limit = " " +AglDeviceP->GetConfig("DORR");        // " *";
                Str_Dat = "<" + AglDeviceP->GetConfig("PG1_2_Limit") + Limit;  // ""<3""
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas1_Gas2 >= PG1_2_Limit && Gas1_Gas2 < PG1_2_Max)
            {
                Limit = " ";
                if(Gas1_Gas2 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas1_Gas2);//sprintf(dat, "%-4.4f", Gas1_Gas2);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas1_Gas2);//sprintf(dat, "%-4.4f", Gas1_Gas2);
                }
                Str_Dat = (QString)dat;
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else
            {
                Limit = " " +AglDeviceP->GetConfig("DORR");        // " *";
                Str_Dat = ">" + AglDeviceP->GetConfig("PG1_2_Max") + " " + Limit;         // ">20 "
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
        }
        Str_line += SText+AglDeviceP->GetConfig("PG1_2_Limit")+"-"+AglDeviceP->GetConfig("PG1_2_Max")+"\r\n";         // "3-20\r\n\r\n";
//----------------------------G17----------------------------//
        SText.clear();
        int G17_Mea_Limit = AglDeviceP->GetConfig("G17_Mea_Limit").toInt(&ok);
        if(false == ok)
        {
            G17_Mea_Limit = 1;
        }
        int G17_Mea_Max = AglDeviceP->GetConfig("G17_Mea_Max").toInt(&ok);
        if(false == ok)
        {
            G17_Mea_Max = 40;
        }
        int G17_Ref_Limit = AglDeviceP->GetConfig("G17_Ref_Limit").toInt(&ok);
        if(false == ok)
        {
            G17_Ref_Limit = 1;
        }
        int G17_Ref_Max = AglDeviceP->GetConfig("G17_Ref_Max").toInt(&ok);
        if(false == ok)
        {
            G17_Ref_Max = 7;
        }
        Str_Dat = AglDeviceP->GetConfig("Print_Item4");      // "G17";
        sprintf(dat, "%-8.8s", Str_Dat.toLatin1().data());
        Str_Dat = dat;
        SText += Str_Dat;
        if(AglDeviceP->Card_index[3] == true)
        {
            isG17Error = true;
            Limit = "";
            Str_Dat = AglDeviceP->GetConfig("res_Err") + Limit;     // "Error"
            sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
            Str_Dat = dat;
            SText += Str_Dat;
        }
//----------------------------------------------------G17 Invalid--------------------------------------------//
        else if(AglDeviceP->AglDev.AdcCalc[3].CPeakVol < Limit_PG1)
        {
            isG17Invalid = true;
            Limit = "";
            Str_Dat = AglDeviceP->GetConfig("res_invalid") + Limit;  // "Invalid"
            sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
            Str_Dat = dat;
            SText += Str_Dat;
        }
//---------------------------------------------------------------------------------------------------------------//
        else
        {
            if(Gas4 < G17_Mea_Limit)   // < 1
            {
                Limit = " " +AglDeviceP->GetConfig("DORR");        // " *";
                Str_Dat = "<" + AglDeviceP->GetConfig("G17_Mea_Limit")+" "+AglDeviceP->GetConfig("G17_Unit")+Limit;  // "<1 pmol/l"
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas4 >= G17_Mea_Limit && Gas4 < G17_Ref_Max)  // 1 - 7
            {
                Limit = " ";
                if(Gas4 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas4);//sprintf(dat, "%-4.4f", Gas4);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas4);//sprintf(dat, "%-4.4f", Gas4);
                }
                Str_Dat = (QString)dat + " " + AglDeviceP->GetConfig("G17_Unit"); // " pmol/l";
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas4 >= G17_Ref_Max && Gas4 < G17_Mea_Max) // 7 - 40
            {
                Limit = " " +AglDeviceP->GetConfig("DORR");        // " *";
                if(Gas4 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas4);//sprintf(dat, "%-4.4f", Gas4);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas4);//sprintf(dat, "%-4.4f", Gas4);
                }
                Str_Dat = (QString)dat + " " + AglDeviceP->GetConfig("G17_Unit")+Limit; // " pmol/l";
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else  //   > 40
            {
                Limit = " " +AglDeviceP->GetConfig("DORR");        // " *";
                Str_Dat = ">" + AglDeviceP->GetConfig("G17_Mea_Max") + " " + AglDeviceP->GetConfig("G17_Unit") + Limit;   // ">40 pmol/l"
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
        }
        Str_line += SText+AglDeviceP->GetConfig("G17_Ref_Limit")+"-"+AglDeviceP->GetConfig("G17_Ref_Max")+"\r\n";     // "1-7\r\n\r\n";
//----------------------------Hp-------------------------//
        SText.clear();
        int HP_Mea_Limit = AglDeviceP->GetConfig("HP_Mea_Limit").toInt(&ok);
        if(false == ok)
        {
            HP_Mea_Limit = 15;
        }
        int HP_Mea_Max = AglDeviceP->GetConfig("HP_Mea_Max").toInt(&ok);
        if(false == ok)
        {
            HP_Mea_Max = 700;
        }
        int HP_Ref_Max = AglDeviceP->GetConfig("HP_Ref_Max").toInt(&ok);
        if(false == ok)
        {
            HP_Ref_Max = 30;
        }
        Str_Dat = AglDeviceP->GetConfig("Print_Item5");          // "Hp";
        sprintf(dat, "%-8.8s", Str_Dat.toLatin1().data());
        Str_Dat = dat;
        SText += Str_Dat;
        if(AglDeviceP->Card_index[2] == true)
        {
            isHpError = true;
            Limit = "";
            Str_Dat = AglDeviceP->GetConfig("res_Err") + Limit;  // "Error"
            sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
            Str_Dat = dat;
            SText += Str_Dat;
        }
//-----------------------------------------Hp Invalid---------------------------------------------//
        else if(AglDeviceP->AglDev.AdcCalc[2].CPeakVol < Limit_Hp)
        {
            isHpInvalid = true;
            Limit = "";
            Str_Dat = AglDeviceP->GetConfig("res_invalid") + Limit;    // "Invalid"
            sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
            Str_Dat = dat;
            SText += Str_Dat;
        }
//-----------------------------------------------------------------------------------------------------//
        else
        {
            if(Gas3 < HP_Mea_Limit) // < 15
            {
                Limit = " ";        // " ";
                Str_Dat = "<"+AglDeviceP->GetConfig("HP_Mea_Limit")+" "+AglDeviceP->GetConfig("HP_Unit")+Limit;  // "<15 EIU"
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas3 >= HP_Mea_Limit && Gas3 < HP_Ref_Max)  // 15 - 30
            {
                Limit = " ";
                if(Gas3 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas3);//sprintf(dat, "%-4.4f", Gas3);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas3);//sprintf(dat, "%-4.4f", Gas3);
                }
                Str_Dat = (QString)dat + " " + AglDeviceP->GetConfig("HP_Unit");     // " EIU";
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else if(Gas3 >= HP_Ref_Max && Gas3 < HP_Mea_Max)  // 30 -700
            {
                Limit = " " +AglDeviceP->GetConfig("DORR");        // " *";
                if(Gas3 >= 100)
                {
                    sprintf(dat, "%-1.0f", Gas3);//sprintf(dat, "%-4.4f", Gas3);
                }
                else
                {
                    sprintf(dat, "%-1.1f", Gas3);//sprintf(dat, "%-4.4f", Gas3);
                }
                Str_Dat = (QString)dat + " " + AglDeviceP->GetConfig("HP_Unit")+Limit;     // " EIU";
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
            else    // > 700
            {
                Limit = " " +AglDeviceP->GetConfig("DORR");        // " *";
                Str_Dat = ">"+AglDeviceP->GetConfig("HP_Mea_Max")+" "+AglDeviceP->GetConfig("HP_Unit")+Limit;  // ">700 EIU"
                sprintf(dat, "%-16.16s", Str_Dat.toLatin1().data());
                Str_Dat = dat;
                SText += Str_Dat;
            }
        }
        int PG1NAN = isnan(Gas1);
        int PG1INF = isinf(Gas1);
        int PG2NAN = isnan(Gas2);
        int PG2INF = isinf(Gas2);
        int HpNAN = isnan(Gas3);
        int HpINF = isinf(Gas3);
        int G17NAN = isnan(Gas4);
        int G17INF = isinf(Gas4);

        if(PG1NAN || PG1INF || PG2NAN || PG2INF || HpNAN || HpINF || G17NAN || G17INF)
        {
            isPG1Invalid = true;
        }
        QString Res_inter;
        if(isPG1Invalid == true || isPG2Invalid == true || isG17Invalid == true || isHpInvalid == true)
        {
            Res_inter.clear();
        }
        else
        {
            std::string Inter = AglDeviceP->GetInterpapreTation(Gas1, Gas2, Gas4, Gas3, Interpretation_post);
            Res_inter = QString::fromStdString(Inter);
//            Res_inter = AglDeviceP->agl_calc_string(Gas1, Gas2, Gas3, Gas4, Interpretation_post);
        }
        if(Res_inter.isEmpty() == false)
        {
            Res_inter = "\r\n" + Res_inter + "\r\n";
        }
        Str_line += SText + "<" +AglDeviceP->GetConfig("HP_Ref_Max")+"\r\n";             // "<30\r\n\r\n";
        Str_line += Res_inter;
//-----------------------------------------Hp over------------------------------------------//
        TC_Value.clear();
        Res_inter = AglDeviceP->GetConfig("report_oper");
        sprintf(dat, "%-13.13s", Res_inter.toLatin1().data());
        Res_inter = dat;
        TC_Value += "\r\n" + Res_inter + LoginUserName;

        Res_inter =  AglDeviceP->GetConfig("Print_Lot");
        sprintf(dat, "%-13.13s", Res_inter.toLatin1().data());
        Res_inter = dat;
        TC_Value += "\r\n" + Res_inter + AnaFound.Pihao + "\r\n";
//-------------------------T1,C1---------------------------------//
        Str_Flag.clear();
        if(isPG1Error == true)
        {
            Str_Flag += "T1 = Error";
            Str_Flag += "\r\nC1 = Error";
        }
//        else if(isPG1Invalid == true)
//        {
//            Str_Flag += "T1 = Invalid";
//            Str_Flag += "\r\nC1 = Invalid";
//        }
        else
        {
            Str_Flag += "T1 = ";
            double Dou_T0 = AglDeviceP->AglDev.AdcCalc[0].TPeakVol;
            sprintf(dat, "%-4.0f", Dou_T0);    // sprintf(dat, "%-4.2f", Dou_T0);
            Str_Flag +=  dat;


            Str_Flag += "\r\nC1 = ";
            double Dou_C0 = AglDeviceP->AglDev.AdcCalc[0].CPeakVol;
            sprintf(dat, "%-4.0f", Dou_C0);    // sprintf(dat, "%-4.2f", Dou_C0);
            Str_Flag +=  dat;
        }
//--------------------------T2, C2----------------------------//
        if(isPG2Error == true)
        {
            Str_Flag += "\r\nT2 = Error";
            Str_Flag += "\r\nC2 = Error";
        }
//        else if(isPG2Invalid == true)
//        {
//            Str_Flag += "\r\nT2 = Invalid";
//            Str_Flag += "\r\nC2 = Invalid";
//        }
        else
        {
            Str_Flag += "\r\nT2 = ";
            double Dou_T1 = AglDeviceP->AglDev.AdcCalc[1].TPeakVol;
            sprintf(dat, "%-4.0f", Dou_T1);    // sprintf(dat, "%-4.2f", Dou_T1);
            Str_Flag +=  dat;

            Str_Flag += "\r\nC2 = ";
            double Dou_C1 = AglDeviceP->AglDev.AdcCalc[1].CPeakVol;
            sprintf(dat, "%-4.0f", Dou_C1);  // sprintf(dat, "%-4.2f", Dou_C1);
            Str_Flag +=  dat;
        }
//------------------------------T3,C3--------------------------------//
        if(isHpError == true)
        {
            Str_Flag += "\r\nT3 = Error";
            Str_Flag += "\r\nC3 = Error";
        }
//        else if(isHpInvalid == true)
//        {
//            Str_Flag += "\r\nT3 = Invalid";
//            Str_Flag += "\r\nC3 = Invalid";
//        }
        else
        {
            Str_Flag += "\r\nT3 = ";
            double Dou_T2 = AglDeviceP->AglDev.AdcCalc[2].TPeakVol;
            sprintf(dat, "%-4.0f", Dou_T2); //  sprintf(dat, "%-4.2f", Dou_T2);
            Str_Flag +=  dat;

            Str_Flag += "\r\nC3 = ";
            double Dou_C2 = AglDeviceP->AglDev.AdcCalc[2].CPeakVol;
            sprintf(dat, "%-4.0f", Dou_C2);//   sprintf(dat, "%-4.2f", Dou_C2);
            Str_Flag +=  dat;
        }
//------------------------------T4, C4------------------------------//
        if(isG17Error == true)
        {
            Str_Flag += "\r\nT4 = Error";
            Str_Flag += "\r\nC4 = Error";
        }
//        else if(isG17Invalid == true)
//        {
//            Str_Flag += "\r\nT4 = Invalid";
//            Str_Flag += "\r\nC4 = Invalid";
//        }
        else
        {
            Str_Flag += "\r\nT4 = ";
            double Dou_T3 = AglDeviceP->AglDev.AdcCalc[3].TPeakVol;
            sprintf(dat, "%-4.0f", Dou_T3);//  sprintf(dat, "%-4.2f", Dou_T3);
            Str_Flag +=  dat;
            Str_Flag += "\r\nC4 = ";
            double Dou_C3 = AglDeviceP->AglDev.AdcCalc[3].CPeakVol;
            sprintf(dat, "%-4.0f", Dou_C3);//  sprintf(dat, "%-4.2f", Dou_C3);
            Str_Flag +=  dat;
            Str_Flag += "\r\n";
        }
    }

//------------------------------------------------------------------------------------------------------------------------------------//
    ResText->setText(TextShow);
    double ImageX[2000];
    double ImageY[4][2000];
    for(int i = 0; i < 2000; i++)
    {
        ImageX[i] = i;
        ImageY[0][i] = 0;
        ImageY[1][i] = 0;
        ImageY[2][i] = 0;
        ImageY[3][i] = 0;
    }

    if((int)AglDeviceP->AglDev.ChxVol == 1)
    {
        int DX_c = AglDeviceP->GetPasswd("@C_Distance").toInt();
        if(DX_c < 0)
        {
            for(int i = 0; i < 1800+DX_c; i++)
            {
                AglDeviceP->AglDev.VolBuf[0][i] = AglDeviceP->AglDev.VolBuf[0][i-DX_c];
            }
            for(int i = 1800+DX_c; i < 1800; i++)
            {
                AglDeviceP->AglDev.VolBuf[0][i] = AglDeviceP->AglDev.VolBuf[0][1799+DX_c];
            }
        }
        else if(DX_c > 0)
        {
            for(int i = 1799; i > DX_c; i--)
            {
                AglDeviceP->AglDev.VolBuf[0][i] = AglDeviceP->AglDev.VolBuf[0][i-DX_c];
            }
            for(int i = DX_c; i > 0; i--)
            {
                AglDeviceP->AglDev.VolBuf[0][i] = AglDeviceP->AglDev.VolBuf[0][0];
            }
        }
        double * DatP = &AglDeviceP->AglDev.VolBuf[0][0];
        for(int i = 0; i < 2000; i++)
        {
            ImageX[i] = i;
            ImageY[0][i] = DatP[i];

            AyvLine->setSamples((double*)ImageX, (double*)(&ImageY[0][0]),1800);
            AyvLine->attach(AqwtplotP);
            AqwtplotP->replot();
        }
    }
    else if((int)AglDeviceP->AglDev.ChxVol == 4)
    {
        int DX_c = AglDeviceP->GetPasswd("@C_Distance").toInt();
        if(DX_c < 0)
        {
            for(int i = 0; i < 1800+DX_c; i++)
            {
                AglDeviceP->AglDev.VolBuf[0][i] = AglDeviceP->AglDev.VolBuf[0][i-DX_c];
                AglDeviceP->AglDev.VolBuf[1][i] = AglDeviceP->AglDev.VolBuf[1][i-DX_c];
                AglDeviceP->AglDev.VolBuf[2][i] = AglDeviceP->AglDev.VolBuf[2][i-DX_c];
                AglDeviceP->AglDev.VolBuf[3][i] = AglDeviceP->AglDev.VolBuf[3][i-DX_c];
            }
            for(int i = 1800+DX_c; i < 1800; i++)
            {
                AglDeviceP->AglDev.VolBuf[0][i] = AglDeviceP->AglDev.VolBuf[0][1799+DX_c];
                AglDeviceP->AglDev.VolBuf[1][i] = AglDeviceP->AglDev.VolBuf[1][1799+DX_c];
                AglDeviceP->AglDev.VolBuf[2][i] = AglDeviceP->AglDev.VolBuf[2][1799+DX_c];
                AglDeviceP->AglDev.VolBuf[3][i] = AglDeviceP->AglDev.VolBuf[3][1799+DX_c];
            }
        }
        else if(DX_c > 0)
        {
            for(int i = 1799; i > DX_c; i--)
            {
                AglDeviceP->AglDev.VolBuf[0][i] = AglDeviceP->AglDev.VolBuf[0][i-DX_c];
                AglDeviceP->AglDev.VolBuf[1][i] = AglDeviceP->AglDev.VolBuf[1][i-DX_c];
                AglDeviceP->AglDev.VolBuf[2][i] = AglDeviceP->AglDev.VolBuf[2][i-DX_c];
                AglDeviceP->AglDev.VolBuf[3][i] = AglDeviceP->AglDev.VolBuf[3][i-DX_c];
            }
            for(int i = DX_c; i > 0; i--)
            {
                AglDeviceP->AglDev.VolBuf[0][i] = AglDeviceP->AglDev.VolBuf[0][0];
                AglDeviceP->AglDev.VolBuf[1][i] = AglDeviceP->AglDev.VolBuf[1][0];
                AglDeviceP->AglDev.VolBuf[2][i] = AglDeviceP->AglDev.VolBuf[2][0];
                AglDeviceP->AglDev.VolBuf[3][i] = AglDeviceP->AglDev.VolBuf[3][0];
            }
        }
        double *DatP0 = &AglDeviceP->AglDev.VolBuf[0][0];
        double *DatP1 = &AglDeviceP->AglDev.VolBuf[1][0];
        double *DatP2 = &AglDeviceP->AglDev.VolBuf[2][0];
        double *DatP3 = &AglDeviceP->AglDev.VolBuf[3][0];
        for(int i = 0; i < 2000; i++)
        {
            ImageX[i] = i;
            ImageY[0][i] =  DatP0[i];  //PG1
            ImageY[1][i] =  DatP1[i];  //PG2
            ImageY[2][i] =  DatP2[i];  //Hp
            ImageY[3][i] =  DatP3[i];  //G17
        }

        AyvLine->setSamples((double*)ImageX, (double*)(&ImageY[0][0]),1800);
        AyvLine->attach(AqwtplotP);

        ByvLine->setSamples((double*)ImageX, (double*)(&ImageY[1][0]),1800);
        ByvLine->attach(BqwtplotP);

        CyvLine->setSamples((double*)ImageX, (double*)(&ImageY[2][0]),1800);
        CyvLine->attach(CqwtplotP);

        DyvLine->setSamples((double*)ImageX, (double*)(&ImageY[3][0]),1800);
        DyvLine->attach(DqwtplotP);

        AqwtplotP->replot();
        BqwtplotP->replot();
        CqwtplotP->replot();
        DqwtplotP->replot();
    }
    qDebug() << "--------------- 18864 绘图完成";
}

void PctGuiThread :: Button_SetPix(Mybutton *button, QPixmap pixmap)
{
    button->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowSystemMenuHint);
    button->setIcon(pixmap);
    button->setIconSize(button->size());
    button->setFocusPolicy(Qt :: NoFocus);
    button->setMask(pixmap.createHeuristicMask());
    button->setFlat(true);
}

void PctGuiThread::CloseKey()
{
    Voice_pressKey();
    SelecKeyWidget->close();
    UiType = Selectui;
}

void PctGuiThread :: SelectOK()
{
    Voice_pressKey();
    if(isClicked == true)
    {
        return;
    }
    isClicked = true;
    QString Str_Sel = Edit_Sel->text();
    if(Str_Sel == "*")
    {
//        SelecKeyWidget->close();
//        Table_Res->show();
        isClicked = false;
        isSelectRes = false;
        UiType = Selectui;
        return;
    }
    if(Str_Sel.isEmpty())
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Result_key"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isClicked = false;
        isMessage = false;
        return;
    }
    QMessageBox *WaitBox = new QMessageBox(MainWindowP);
    WaitBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
    WaitBox->setText(AglDeviceP->GetConfig("Wait_Select"));
    WaitBox->setIcon(QMessageBox :: Warning);
    WaitBox->setStandardButtons(QMessageBox::NoButton);
    WaitBox->show();
    WaitBox->move(WaitBox->pos().x(),WaitBox->pos().y()+40);
//---Add---------------------------------------------------------------------------liuyouwei----------------------------------------//
    int Current_line = 0;
    QString REST;
    bool isFound  = false;
    if(iSelect == 1)
    {
        isFound  = Sql->SelectResultSql(REST,1,Str_Sel);
    }
    else if(iSelect == 2)
    {
        Str_Sel += "*";
        isFound  = Sql->SelectResultSql(REST,4,Str_Sel);
    }
    else if(iSelect == 3)
    {
        isFound  = Sql->SelectResultSql(REST,2,Str_Sel);
    }

    Res_list = REST.split("@");
    int ilengh = 0;
    ilengh = Res_list.length();
    int iShow = ilengh;
    Select_Res_All = iShow - 1;

    if(isFound == false || ilengh == 1)
    {
        Delayms(500);
        WaitBox->close();
        delete WaitBox;
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_nofound"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        isClicked = false;
        isMessage = false;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        KeyInput.clear();
        Edit_Sel->clear();
        return;
    }

    Current_line = Table_Res->rowCount();
    for(int  i = 0; i < Current_line; i++)                                 // 删除表格中所有内容（包括Item）
    {
        Table_Res->removeRow(0);
    }
    //------------------------------Add---------------------------------//
    if(iShow > 10)
    {
        iShow = 10;
    }
    else
    {
        iShow = iShow - 1;
    }

    double time = ((double)ilengh - 1) / (double)10;
    PageAll = (ilengh - 1) / 10;

    if(time > PageAll)
    {
        PageAll += 1;
    }
    if(PageAll == 0)
    {
        PageAll = 1;
    }
    PageNow = 1;
    //--------------------------------------------------------------------//
    if(ilengh - 1 > 4)
    {
        Table_Res->setGeometry(133.5,101,565,179);
        Table_Res->setRowCount(iShow);
        Table_Res->setColumnCount(1);
        Table_Res->setColumnWidth(0, 563);
    }
    else
    {
        Table_Res->setGeometry(133.5,101,535,179);
        Table_Res->setColumnWidth(0, 533);
        Table_Res->setRowCount(iShow);
        Table_Res->setColumnCount(1);
    }

    for(int j = 0; j < Current_line; j++)
    {
        Table_Res->setRowHeight(j, 36);
    }

    int LINE = 0;
    QString Sele_Data;
    QString Show_item;
    QStringList Sele_list;
    QString DateData;
    QStringList DateList;
    QTableWidgetItem *Item_Sha[iShow];
    for(LINE = 0; LINE < iShow; LINE++)             // 表格上方显示查找到的数据
    {
        Sele_Data = Res_list.at(LINE);
        Sele_list = Sele_Data.split(",");

        Show_item.clear();
        sprintf(dat, "%-8.8s", Sele_list.at(0).toLatin1().data());
        Show_item += (QString)dat;

        sprintf(dat, "%-16.16s", Sele_list.at(1).toLatin1().data());
        Show_item += (QString)dat;

        DateList = Sele_list.at(4).split(";");
        DateData = DateList.at(0);
        sprintf(dat, "%-19.19s", DateData.toLatin1().data());
        Show_item += (QString)dat;

        Item_Sha[LINE] = new QTableWidgetItem;
        Item_Sha[LINE]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        Item_Sha[LINE]->setText(Show_item);
        Table_Res->setItem(LINE, 0, Item_Sha[LINE]);
    }

    Delayms(500);
    WaitBox->close();
    delete WaitBox;
    Select_Res_show = LINE;
    SelecKeyWidget->close();
    Table_Res->show();
    KeyInput.clear();
    Edit_Sel->clear();
    Label_station->setText(AglDeviceP->GetConfig("Station_ResData"));
    isSelectRes = true;
    QString All = QString::number(PageAll);
    QString Now = QString::number(PageNow);
    Label_cou->setText(Now + "/" + All + " " + "page");
    UiType = Selectui;
    isClicked = false;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------//
//---Add-----------------------------------------------------------------------liuyouwei------------------//
void PctGuiThread :: pressDelButton()
{
    Voice_pressKey();
    if(UiType != Selectui)
    {
        return;
    }
    bool ok = false;
    QTableWidgetItem *Item_Sha[10];
    QList<QTableWidgetItem *> List = Table_Res->selectedItems();                       // 获得选中的单元格，返回值是一个单元格列表QList<QTableWidgetItem *>
    if(List.isEmpty())
    {
        isMessage = true;
        msgBox = new QMessageBox(MainWindowP);
        msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
        msgBox->setText(AglDeviceP->GetConfig("Message_nodel"));
        msgBox->setIcon(QMessageBox :: Warning);
        msgBox->exec();
        delete msgBox;
        if(isWidget == false)
        {
            Voice_pressKey();
        }
        isMessage = false;
        return;
    }
//------------------------------------------------------------------------------------------------------------------//
    isMessage = true;
    msgBox = new QMessageBox(MainWindowP);
    msgBox->setWindowTitle("Warning");
    msgBox->setStyleSheet(QStringLiteral("background-color: rgb(180,180,180);"));
    msgBox->setText(AglDeviceP->GetConfig("Message_delete"));
    msgBox->setStandardButtons(QMessageBox::Yes  | QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::No);
    msgBox->setIcon(QMessageBox :: Question);
    int ret = msgBox->exec();
    switch (ret)
    {
        case QMessageBox::Yes:
        {
            Voice_pressKey();
            QString SelectItem = List.at(0)->text();
            QStringList List_data =  CharacterStr(SelectItem);
            SelectID = List_data.at(0);
            int found_id = SelectID.toInt(&ok);
            if(ok == false)
            {
                qDebug() << "----- 15536 Select ID Error";
            }
            if(!Sql->DeleteResultSql(found_id))
            {
                qDebug() << "13378 Delete Result DB ERROR";
                return;
            }
           int Count_res = Sql->CountResultSql();
           qDebug() << "Count_res is " << Count_res;
           int currentpage = PageAll;
           int Count  = Table_Res->rowCount();
           double time = ((double)Count_res) / (double)10;
           PageAll = Count_res / 10;
           if(time > PageAll)
           {
               PageAll += 1;
           }
           if(PageAll == 0)
           {
               PageAll = 1;
           }
           qDebug() << "PageAll is " << PageAll;
           if(PageAll == currentpage - 1)
           {
               PageNow = PageAll;
               QString nowpage = QString::number(PageNow);
               QString allpage = QString::number(PageAll);
               qDebug() << "nowpage is " << nowpage;
               qDebug() << "allpage is " << allpage;
               Label_cou->setText(nowpage + "/" + allpage + " " + AglDeviceP->GetConfig("Label_Page"));
           }
           for(int  i = 0; i < Count; i++)                        // 由于第0行为表头，所以每一次只删除第一行，如此循环，删除表格中所有内容（包括Item）
           {
               Table_Res->removeRow(0);
           }
           if(-1 == Count_res)
           {
               qDebug() << "13257 Count Result DB ERROR";
               return;
           }
           int RowCount = Count_res - (PageNow-1) * 10;
           if(RowCount > 10)
           {
               RowCount = 10;
           }
           if(RowCount  > 4)
           {
               Table_Res->setGeometry(133.5,101,565,179);
               Table_Res->setRowCount(RowCount);
               Table_Res->setColumnCount(1);
               Table_Res->setColumnWidth(0, 563);
           }
           else
           {
               Table_Res->setGeometry(133.5,101,535,179);
               Table_Res->setColumnWidth(0, 533);
               Table_Res->setRowCount(RowCount);
               Table_Res->setColumnCount(1);
           }
           int InDex = (PageNow-1) * 10 + 1;
           for(int j = 0; j < RowCount; j++)
           {
               Table_Res->setRowHeight(j, 36);                                   // 设置第j行单元格宽度
           }
            struct ResultIndex RES_LIST;
            QString DateData;
            QStringList DateList;
            QString Show_item;
            for(int  i = InDex; i < RowCount + InDex; i++)
            {
                if(!Sql->ListResultSql(RES_LIST,i))
                {
                    qDebug() << "13489 List Result DB ERROR";
                    return;
                }

                Show_item.clear();
                QString iData = QString::number(i);
                sprintf(dat, "%-8.8s", iData.toLatin1().data());
                Show_item += (QString)dat;

                sprintf(dat, "%-16.16s", RES_LIST.SamId.toLatin1().data());
                Show_item += (QString)dat;

                DateList = RES_LIST.Date.split(";");
                DateData = DateList.at(0);
                sprintf(dat, "%-19.19s", DateData.toLatin1().data());
                Show_item += (QString)dat;

                Item_Sha[i-InDex] = new QTableWidgetItem;
                Item_Sha[i-InDex]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                Item_Sha[i-InDex]->setText(Show_item);
                Table_Res->setItem(i-InDex, 0, Item_Sha[i-InDex]);
            }
            break;
        }
        case QMessageBox::No:
        {
            if(isWidget == false)
            {
                Voice_pressKey();
            }
            break;
        }
    }
    delete msgBox;
    isMessage = false;
//------------------------------------------------------------------------------------------------------------------//
}

//void PctGuiThread :: presssendbutton()
//{
//    char CR = 13;  //CR
//    char EB = 28;  //FS
//    Voice_pressKey();

//    QString All_OBX = GetOBX(OBX_4, OBX_5, OBX_6, OBX_7, OBX_8);
//    QString Send_mess = GetMSH("ORU^R01") + GetPID() + GetOBR() + All_OBX + (QString)EB + (QString)CR;

//    LisSocket->waitForBytesWritten(100);
//    LisSocket->write(Send_mess.toLatin1().data(), Send_mess.length());

//    QByteArray ReadData;
//    LisSocket->waitForReadyRead(100);
//    ReadData = LisSocket->readAll();
//    QString allData = ReadData.data();
//    SocketEdit->append(allData);

//    QStringList Read_list = allData.split((QString)CR);
//    int Len_CR = Read_list.length();
//    qDebug() << "lieng is " << Len_CR;
//    if(4 != Len_CR)
//    {
//        QMessageBox msgBox;
//        msgBox.setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//        QString loginerr = AglDeviceP->GetConfig("Message_Uperr");
//        msgBox.setText(loginerr);
//        msgBox.setIcon(QMessageBox :: Warning);
//        msgBox.move(230,180);
//        msgBox.exec();
//        Voice_pressKey();
//        return;
//    }
//    allData = Read_list.at(1);
//    Read_list = allData.split("|");
//    int Len_MA = Read_list.length();
//    if(8 != Len_MA)
//    {
//        QMessageBox msgBox;
//        msgBox.setStyleSheet(QStringLiteral("background-color: rgb(180, 180, 180);"));
//        QString loginerr = AglDeviceP->GetConfig("Message_Uperr");
//        msgBox.setText(loginerr);
//        msgBox.setIcon(QMessageBox :: Warning);
//        msgBox.move(230,180);
//        msgBox.exec();
//        Voice_pressKey();
//        return;
//    }

//    QString CR_Str = (QString)CR;
//    Read_list = allData.split(CR_Str);
//    allData = Read_list.at(0);
//    Read_list = allData.split("|");

//    QString send = "\r\nSend is " + Read_list.at(0);
//    QString time = "\r\n Time is "+ Read_list.at(2);
//    QString mess = "\r\n Message is " + Read_list.at(4);
//    SocketEdit->append(send+time+mess);
//}

//void PctGuiThread :: pressstartbutton()
//{
//    Voice_pressKey();
//    bool ok;
//    QString LisIp = GetNetIp("LisIP");
//    quint16 LisPort = GetNetIp("LisPort").toUInt(&ok, 10);
//    if(!LisSocket->open(QIODevice :: ReadWrite))
//    {
//        qDebug() << "'Open Socket error";
//        return;
//    }
//    LisSocket->connectToHost(LisIp, LisPort);
//}

//void PctGuiThread :: pressclearbutton()
//{
//    Voice_pressKey();
//    LisSocket->disconnectFromHost();
//    LisSocket->close();
//}

QString PctGuiThread :: GetMSH(QString MSH_head)                 // 消息头
{
    char CR = 13;  //CR
    char SB = 11;  //VT
//    char EB= 28;  //FS
    QDateTime time = QDateTime :: currentDateTime();
    QString now = time.toString("yyyyMMddhhmmss");
    QString Message_index = QString :: number(++iMessage, 10);
    QString MSH_Data;
    MSH_Data = "MSH|";                                  //  1
    MSH_Data += "^~\\&|";                              //  2 转义分隔符，和子组件分隔符（^~\&）
    MSH_Data += "YiXin-001|";                       //  3 发送端程序
    MSH_Data += "GP Reader|";                     //  4 置空，保留。发送端设备
    MSH_Data += "|";                                       //  5 接收端应用程序
    MSH_Data += "|";                                       //  6 置空，保留。接收端设备
    MSH_Data += now + "|";                           //  7 当前消息的时间。调用系统的时间信息
    MSH_Data += "|";                                      //  8 置空，保留。安全性
    MSH_Data += MSH_head + "|";               //  9 消息的类型，如：ORU^R01
//    MSH_Data += "QRY^Q02|";                      //  9 消息的类型，如：ORU^R01
    MSH_Data += Message_index + "|";        //  10 消息控制ID，唯一标识一个消息，随消息数目增加从1 递增
    MSH_Data += "P|";                                    //  11 处理ID，一直取P （表示产品）
    MSH_Data += "2.3.1|";                              //  12 版本ID，HL7 协议版本：2.3.1
    MSH_Data += "|";                                       //  13 置空，保留。序列号
    MSH_Data += "|";                                       //  14 置空，保留。连续指针
    MSH_Data += "|";                                       //  15 置空，保留。接收应答类型
    MSH_Data += "0|";                                     //  16 应用程序应答类型，作为发送的结果类型。0-病人样本测试结果；1-校准结果；2-质控结果；
    MSH_Data += "|";                                       //  17 置空，保留。国家代码。
    MSH_Data += "ASCII|";                              //  18 字符集，取ASCII。
    MSH_Data += "|";                                       //  19 置空，保留。消息主要语言
    MSH_Data += "|";                                       //  20 置空，保留。交替字符集处理方案
    QString MSH_ORU_R01 = (QString)SB + MSH_Data + (QString)CR;

    return MSH_ORU_R01;
}

QString PctGuiThread :: GetPID()
{
    char CR = 13;  //CR
//    char SB= 11;  //VT
//    char EB= 28;  //FS
    QString PID_Data;
    PID_Data = "PID|";
    PID_Data += QString :: number(++PID_index, 10) + "|";     // 1 确定不同的病人消息段
    PID_Data += "408|";                                                                 // 2  置空，保留。病人的住院号
    PID_Data += "12345|";                                                             // 3 病历号
    PID_Data += "003|";                                                                 // 4 床号
    PID_Data += "Jack|";                                                                 // 5  病人的姓名
    PID_Data += "bingqu|";                                                            // 6 病区
    PID_Data += "19880302|";                                                       // 7 置空，保留。病人出生日期
    PID_Data += "M|";                                                                    // 8 性别男，发送M；女，发送F；其它，发送O
    PID_Data += "O|";                                                                     // 9 置空，保留。血型
    PID_Data += "|";                                                                        // 10 置空，保留。种族
    PID_Data += "Samplie addr|";                                                 // 11 置空，保留。病人地址
    PID_Data += "712204|";                                                           // 12 置空，保留。郡县代码（邮编）
    PID_Data += "07552568488|";                                                 // 13 置空，保留。电话号码
    PID_Data += "|";                                                                        // 14 置空，保留。置空，保留。电话号码-公司
    PID_Data += "|";                                                                        // 15 置空，保留。置空，保留。主要语言
    PID_Data += "|";                                                                        // 16 置空，保留。置空，保留。婚姻状况
    PID_Data += "|";                                                                        // 17 置空，保留。置空，保留。宗教
    PID_Data += "jizhen|";                                                              // 18 病人类别
    PID_Data += "1222|";                                                                // 19 置空，保留。医保账号
    PID_Data += "xianjin|";                                                             // 20 置空，保留。收费类型
    PID_Data += "|";                                                                        // 21 置空，保留。母亲标识符
    PID_Data += "|";                                                                        // 22 民族
    PID_Data += "guangdong|";                                                     // 23 置空，保留。出身地（籍贯）
    PID_Data += "N|";                                                                     // 24 置空，保留。置空，保留。多胞胎指示符，是为Y，否为N
    PID_Data += "|";                                                                        // 25 置空，保留。出生次序，大于0的整数
    PID_Data += "beizhu|";                                                            // 26 备注
    PID_Data += "|";                                                                        // 27 置空，保留。退伍军人状态
    PID_Data += "China|";                                                              // 28 置空，保留。国家
    PID_Data += "|";                                                                        // 29 置空，保留。病人死亡时间
    PID_Data += "|";                                                                        // 30 置空，保留。病人死亡指示符，是为Y，否为N
// PID_Data += "28^Y|";                                                                // 31 年龄和年龄单位。年龄与年龄单位之间用^分隔.岁发送Y，月发送M，天发送D，小时发送H
    QString PID = PID_Data + (QString)CR;

    return PID;
}

QString PctGuiThread :: GetOBR()    // 观察报告（关于检验报告的医嘱信息）
{
    char CR = 13;  //CR
//    char SB= 11;  //VT
//    char EB= 28;  //FS
    QString OBR_Data;
    OBR_Data += "OBR|";
    OBR_Data += QString :: number(++OBR_index, 10) + "|"; // 1确定不同的OBR字段
    OBR_Data += "01234|1";                         // 2 请求者医嘱号，用作样本条码号
    OBR_Data += "01|";                                  // 3 执行者医嘱号，用作样本编号
    OBR_Data += "Analyzer ID^CS6400|";    // 4 通用服务标识符
    OBR_Data += "N|";                                    // 5 是否急诊，是为Y，否为N
    OBR_Data += "|";                                       // 6 置空，保留。请求时间/日期
    OBR_Data += "20170108|";                      // 7 观察日期/时间，用作检验日期/时间
    OBR_Data += "|";                                       // 8 置空，保留。观察结束日期/时间
    OBR_Data += "1|";                                     // 9 用作重复测试次数，取1
    OBR_Data += "E002^08|";                        // 10 采集者标示。用作样本和位置。样本架与位置之间用^分隔。样本架长度为4，位置长度为3。
    OBR_Data += "|";                                       // 11 置空，保留。样本处理代码
    OBR_Data += "N|";                                    // 12 危险代码。用作是否稀释，是为Y，否为N
    OBR_Data += "|";                                       // 13 相关临床信息，用作病人临床诊断信息
    OBR_Data += "20170105|";                      // 14 送检日期/时间
    OBR_Data += "0|";                                     // 15 样本来源，用作样本类型，如血清、血浆、尿液等。0-血清，1-尿液，2-血浆，3-脑脊液，4-胸腹水，5-其他
    OBR_Data += "Doctor Li|";                        // 16 医嘱提供者，用作送检医生
    OBR_Data += "Waike|";                             // 17 用作送检科室
    OBR_Data += "zhi xue|";                           // 18  置空，保留。样本性状（黄疸、溶血、脂血）
    OBR_Data += "008|";                                 // 19  置空，保留。血袋编号
    OBR_Data += "Doctor zhu|";                    // 20  主治医生，用作检验医生
    OBR_Data += "nei ke|";                             // 21  置空，保留。治疗科室
    OBR_Data += "20170109123030|";         // 22  结果报告日期/时间
    OBR_Data += "|";                                       // 23  置空，保留。实行费用
    OBR_Data += "|";                                       // 24  置空，保留。诊断部分ID
    OBR_Data += "|";                                       // 25  置空，保留。结果状态
    OBR_Data += "|";                                       // 26  置空，保留。父医嘱结果
    OBR_Data += "|";                                       // 27  置空，保留。数量/时间
    OBR_Data += "|";                                       // 28  置空，保留。结果抄送
    OBR_Data += "|";                                       // 29  置空，保留。父医嘱
    OBR_Data += "|";                                       // 30  置空，保留。传输模式
    OBR_Data += "|";                                       // 31  置空，保留。研究原因
    OBR_Data += "|";                                       // 32  结果主要解释者，用作审核医生
    OBR_Data += "|";                                       // 33  置空，保留。结果辅助解释者
    OBR_Data += "|";                                       // 34  置空，保留。技术员
    OBR_Data += "|";                                       // 35  置空，保留。转录
    OBR_Data += "|";                                       // 36  置空，保留。预定日期/时间
    OBR_Data += "|";                                       // 37  置空，保留。样本容器数量
    OBR_Data += "|";                                       // 38  置空，保留。采集样本的运输后勤
    OBR_Data += "|";                                       // 39  置空，保留。采集者注释
    OBR_Data += "|";                                       // 40  置空，保留。运输安排负责
    OBR_Data += "|";                                       // 41  置空，保留。运输是否安排
    OBR_Data += "|";                                       // 42  置空，保留。需要护送
    OBR_Data += "|";                                       // 43  置空，保留。已安排的病人运输注释
    OBR_Data += "|";                                       // 44  置空，保留。请求者名字
    OBR_Data += "|";                                       // 45  置空，保留。请求者地址
    OBR_Data += "|";                                       // 46  置空，保留。请求者电话号码
    OBR_Data += "|";                                       // 47  置空，保留。请求者提供者地址
    QString OBR = OBR_Data + (QString)CR;

    return OBR;
}

QString PctGuiThread :: GetOBX(QString OBX_Data1,QString OBX_Data2,QString OBX_Data3,QString OBX_Data4,QString OBX_Data5)    // 检查结果
{
    char CR = 13;  //CR
//    char SB = 11;  //VT
//    char EB = 28;  //FS
    QDateTime time = QDateTime :: currentDateTime();
    QString now = time.toString("yyyyMMddhhmmss");

    QString OBX_Data;
    OBX_Data = "OBX|";
    OBX_Data += QString :: number(++OBX_index, 10) + "|";      // 1 确定不同的OBX字段
    OBX_Data += "NM|";             // 2  值类型，用作标识测试结果的类型NM (numeric)表示数字值，用于定量项目ST (string)表示字符串值，用于定性项目
    OBX_Data += "604|";             // 3  观察标识符，用作项目ID号
    OBX_Data += OBX_Data1 + "|";   // 4  观察Sub-ID，用作项目名称
    OBX_Data += OBX_Data2 + "|";   // 5  观察值，用作检验结果值（结果浓度或阴性、阳性等）
    OBX_Data += OBX_Data3 + "|";   // 6  单位，用作检验结果值的单位
    OBX_Data += OBX_Data4 + "|";   // 7  参考范围，检验结果值正常范围
    OBX_Data += OBX_Data5 + "|";   // 8  异常标志，检验结果是否正常（描述）L-偏低H-偏高N-正常
    OBX_Data += "|";                   // 9  置空，保留。可能性
    OBX_Data += "|";                   // 10  置空，保留。异常测试原因
    OBX_Data += "F-finalresults|"; // 11  置空，保留。观察结果状态取F-finalresults
    OBX_Data += "|";                   // 12  置空，保留。最后观察正常值日期
    OBX_Data += "|";                   // 13  用户自定义访问检查，用作原始结果
    OBX_Data += now + "|";       // 14  观察日期/时间，用作检测日期/时间
    OBX_Data += "|";                   // 15  结果生成者ID
    OBX_Data += "Doctor Li|";    // 16  负责观察者，用作检验医生
    OBX_Data += "|";                    // 17  置空，保留。观察方法

    QString OBX = OBX_Data + (QString)CR ;
    return OBX;
}

QString PctGuiThread :: GetQRD(QString Select_ID)   // 查询定义
{
    char CR = 13;  //CR
//    char SB= 11;  //VT
//    char EB= 28;  //FS
    QDateTime time = QDateTime :: currentDateTime();
    QString now = time.toString("yyyyMMddhhmmss");
    QString QRD_Data;
    QRD_Data = "QRD|";
    QRD_Data += now + "|";    // 1 本次查询产生时间，取系统时间
    QRD_Data += "BC|";          // 2 查询格式代码， 取R（record-oriented format）
    QRD_Data += "D|";            // 3 查询优先权，取D（deferred）
    QRD_Data +=  QString :: number(++QRD_index, 10) + "|";    // 4 查询ID，表征不同的查询，随查询数目由1 递增
    QRD_Data += "|";               // 5 置空，保留。延迟响应类型
    QRD_Data += "|";               // 6 置空，保留。延迟响应日期/时间
    QRD_Data += "RD|";          // 7 数量限制要求， 取RD（Records）
    QRD_Data += Select_ID + "|";    // 8 查询人过滤符，用作病人的样本条码(select)
    QRD_Data += "OTH|";       // 9 查询内容过滤符，查询时置为OTH
    QRD_Data += "|";              // 10 置空，保留。部门数据代码
    QRD_Data += "|";              // 11 置空，保留。数据代码值限定
    QRD_Data += "T|";            // 12 置空，保留。查询结果水平，取T（Full results）

    QString QRD = QRD_Data + (QString)CR;
    return QRD;
}

QString PctGuiThread :: GetQRF()     // 查询筛选
{
    char CR = 13;  //CR
//    char SB= 11;  //VT
//    char EB= 28;  //FS
    QString QRF_Data;
    QRF_Data = "QRF|";
    QRF_Data += "CHEMRAY420|";        // 1查询者地点过滤符， 取CHEMRAY420
    QRF_Data += "20170203113030|"; // 2 记录开始日期/时间，用作查询时的样本接收时间之始
    QRF_Data += "20170205080210|"; // 3 记录结束日期/时间，用作查询时的样本接收时间之末
    QRF_Data += "|";                               // 4 置空，保留。使用者合格标志
    QRF_Data += "|";                               // 5 置空，保留。其它QRF 接受过滤符
    QRF_Data += "RCT|";                        // 6 目标类型，取RCT（Specimenreceipt date/time, receipt of specimenin filling ancillary (Lab)）
    QRF_Data += "COR|";                       // 7 目标状态，取COR（Correctedonly (no final with corrections)）
    QRF_Data += "ALL|";                        // 8 日期/时间选择限定符，取ALL（All values within the range）
    QRF_Data += "|";                              // 9 置空，保留。时间间隔段

    QString QRF = QRF_Data + (QString)CR;
    return QRF;
}

QString PctGuiThread :: GetERR()    // 错误信息
{
    char CR = 13;  //CR
    QString ERR_Data = "0";
    QString ERR = "ERR|" + ERR_Data + "|" + (QString)CR;

    return ERR;
}

QStringList PctGuiThread::CharacterStr(QString SpaceStr)
{
    QStringList RetList;
    QString FoundStr;
    int Lengh = SpaceStr.length();
    for(int i = 0; i < Lengh; i++)
    {
        if(" " != SpaceStr.at(i))
        {
            FoundStr += SpaceStr.at(i);
        }
        if(" " == SpaceStr.at(i) || i == Lengh-1)
        {
            if(!FoundStr.isEmpty())
            {
                RetList.append(FoundStr);
                FoundStr.clear();
            }
        }
    }

    return RetList;
}

//void PctGuiThread::RecordTime(QString Time)
//{
//    QFile TimeFile("./OperationTime.txt");
//    if(!TimeFile.open(QFile::WriteOnly | QFile::Append))
//    {
//        qDebug() << "open file Error";
//        return;
//    }
//    QTextStream Stream(&TimeFile);
//    Stream << Time;
//    TimeFile.close();
//}



//void PctGuiThread :: pressButton_pic()
//{
//    Voice_pressKey();
//    Button_pic->hide();
//    QString Pic = QString::number(++iPic);
//    QPixmap pixmap = QPixmap::grabWidget(MainWindowP);
//    pixmap.save(Pic+".png", "png");
//}
