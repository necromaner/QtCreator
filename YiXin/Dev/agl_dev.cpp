//---------------------------------------------------------------------------------//
#include "agl_dev.h"
//---------------------------------------------------------------------------------//
#include "uUsbCamera.h"
#include "uUsbCamera.cpp"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QNetworkConfigurationManager>
//---------------------------------------------------------------------------------//
#define CMD_PORT_SELECTED 0x0C
#define CMD_BIT_SELECTED 0x0D
#define Scan1D_RstPin 11
#define Scan1D_TrgPin 10
#define Scan2D_RstPin 9
#define Scan2D_TrgPin 8
#define CardInsert_Pin 0
#define AdcMaxCycle        (1800)//bytes
#define AdcMaxDataLen   (1800*2)//bytes

#define UdpServerPort  10000
#define UdpClientPort   10001
#define TcpServerPort   20000
#define TcpClientPort    20001
//QTcpServer *NetServerP;
//QTcpSocket *TcpServerP;
//QTcpSocket *TcpClientP;
//QUdpSocket *UdpServerP;
//QUdpSocket *UdpClientP;
//---------------------------------------------------------------------------------//
int GpioDev;
int LedDev;
int AdcDev;
int TemDev;
#ifdef WY_COM
ComStr *ComPort1;//AGL-CGR-Com
ComStr *ComPort2;//PC-Com
ComStr *ComPort3;//Scan-1D,Scan-2D,Printer,Bak-Com
//------------------------Add 10-10-26-for test--------------------------------------//
ComStr *ComPortTest;
//--------------------------------------------------------------------------------------------//
#else
QSerialPort *ComPort1;//AGL-CGR-Com
QSerialPort *ComPort2;//PC-Com
QSerialPort *ComPort3;//Scan-1D,Scan-2D,Printer,Bak-Com
#endif
QUsbCamera *UsbCamera;
static uchar PixImgBuf[64*1024];
static double ImageA[40][60][600];
static double ImageB[40][60][600];


//---------------------------------------------------------------------------------//
APP_dev::APP_dev(QThread *parent) : QThread(parent)
{
    //------------------------//
    AppSta=NOP_DEV;
    memset((char*)&AglDev,0,sizeof(AglDevStr));
    memset((char*)&CgrDev,0,sizeof(CgrDevStr));
    //qDebug()<<QDir::currentPath();
    //------------------------//
    yx_udisk=false;
    yx_sdisk=false;
    yx_network=false;
    yx_power=1.00;

    isTestContrl = false;
    isCalibration = false;
    isDoor = false;
    Error_Code = Error_OK;
//    isCardError = false;
//    isFoundSamll = false;
    //------------------------//
    isFoundPasscfg = LoadPasswdFile();
    LoadConfigFile();
    //------------------------//
    AppCfg=new ConfigStr();
    //------------------------//
    AppCfg->LoginPort=3;
    AppCfg->SN=GetConfig("SN");
    if(AppCfg->SN=="No") { AppCfg->SN="123456"; }
    AppCfg->LocalIp=GetPasswd("@hostIP");
    if(AppCfg->LocalIp=="No") { AppCfg->LocalIp="0.0.0.0"; }
    AppCfg->LisIP=GetPasswd("@LisIP");
    if(AppCfg->LisIP=="No") { AppCfg->LisIP="0.0.0.0"; }
    AppCfg->LisPort=GetPasswd("@LisPort");
    if(AppCfg->LisPort=="No") { AppCfg->LisIP="10000"; }
    AppCfg->LisMark=GetConfig("COSEdit_mark");
    if(AppCfg->LisMark=="No") { AppCfg->LisMark="YX_DEV"; }
    AppCfg->DevName=GetConfig("Device_name");
    if(AppCfg->DevName=="No") { AppCfg->DevName="YX_DEV"; }
    AppCfg->LoginName=GetPasswd("@USR1");

    if(AppCfg->LoginName=="No") { AppCfg->LoginName="Admin"; }
    AppCfg->LoginPasswd=GetPasswd(AppCfg->LoginName);

    if(AppCfg->LoginPasswd=="No") { AppCfg->LoginPasswd="123"; }
    AppCfg->HardVersion=GetConfig("HardVer");

    if(AppCfg->HardVersion=="No") { AppCfg->HardVersion="HV0.0.0"; }
    AppCfg->SoftVersion=GetConfig("SoftVer");
    if(AppCfg->SoftVersion=="No") { AppCfg->SoftVersion="V1.0.0"; }
    //------------------------//
    QTimer *Timer = new QTimer();
    Timer->setInterval(2000);
    connect(Timer,SIGNAL(timeout()),this,SLOT(TimerOut()));//连接槽
    Timer->start();
    //------------------------//
}

APP_dev::~APP_dev()
{
    this->CloseDevice();
    this->deleteLater();
}

QString APP_dev::GetVersion()
{
    QString Ver="V1.0.000-20161010";
    return Ver;
}

bool APP_dev::PrintData(QByteArray DatStr)
//bool APP_dev::PrintData(std::string DatStr)
{
    qDebug() << "printf lengh is " << DatStr.length();
    QDateTime time = QDateTime :: currentDateTime();
    QString now = time.toString("yyyy-MM-dd hh:mm:ss");
    QString Time = "-------"+now+"------\r\n";
#ifdef WY_COM
    if(ComPort3->Port<0)
    {
        qDebug()<<"Printer is error !";
        return false;
    }
    qDebug()<<"Printer is ok !";
    SetCom3State(ComPrinter);
    Delayms(500);
    //----------------------------------------------------------------------//
    char StarBuf[]={0x1b,0x40};
    char EndBuf[]={0x1b,0x4a,0x10};
//    char str[]={0x1b,0x31,16};
//    WriteCom(ComPort3,(char *)str,3);
//    Delayms(100);
    WriteCom(ComPort3,(char *)StarBuf,2);
    Delayms(500);
//    WriteCom(ComPort3,(char *)"------------------\r\n",20);
    WriteCom(ComPort3,Time.toLatin1().data(),Time.length());
    WriteCom(ComPort3,(char *)DatStr.data(),DatStr.length());
//    WriteCom(ComPort3,(char *)"--------------------------------\r\n",34);
    WriteCom(ComPort3,(char *)"\r\n",2);
    Delayms(500);
    WriteCom(ComPort3,(char *)EndBuf,3);
    Delayms(500);
    ReadCom(ComPort3,ComPort3->Buf,4096);
    return true;
#else
    if(!ComPort3->isOpen())
    {
        qDebug()<<"Printer is error !";
        return false;
    }
    qDebug()<<"Printer is ok !";
    //disconnect(ComPort3,SIGNAL(readyRead()),this,SLOT(Com3Read()));//连接槽
    SetCom3State(ComPrinter);
    Delayms(100);
//    QString RtcTime;
//    GetRtcTime(RtcTime);
//    RtcTime +="  \r\n";
    //----------------------------------------------------------------------//
    char InitBuf[]={0x1b,0x40};
    char LineBuf[]={0x1b,0x4a,0x04};
    ComPort3->write(InitBuf,2);
    Delayms(100);
    ComPort3->write("--------------------\r\n");
//    ComPort3->write(RtcTime.toLatin1());
    ComPort3->write(DatStr);
    ComPort3->write("---------------------\r\n");
    ComPort3->write(" \r\n");
    ComPort3->write(" \r\n");
    Delayms(100);
    ComPort3->write(LineBuf,3);
    Delayms(100);
    ComPort3->readAll();
    return true;
#endif
}

bool APP_dev::Scan1D_Code(QByteArray &Code)
{
#ifdef WY_COM
    if(ComPort3->Port<0)
    {
        qDebug()<<"Scan1d is error !";
        return false;
    }
    qDebug()<<"Scan1d is ok !";
    SetCom3State(Com1D);
    Delayms(100);
    //----------------------------------------------------------------------//
    ReadCom(ComPort3,ComPort3->Buf,10000);
    memset(ComPort3->Buf,0,10000);
    Delayms(100);
    SetPinState(Scan1D_TrgPin,0);
//    Delayms(3000);
    Delayms(300);
    SetPinState(Scan1D_TrgPin,1);
    Delayms(100);
    int DatLen=ReadCom(ComPort3,ComPort3->Buf,4096);
    qDebug("Scan1d=%s",ComPort3->Buf);
    //---------------Add----------------//
    QByteArray Test;
    Test.append(ComPort3->Buf,DatLen);
    QString iCurrent = Test.mid(Test.length() - 2, 2);
    //------------------------------------//
    if(DatLen < 2)
    {
         return false;
    }
    if(iCurrent == "\r\n")
    {
       Code.append(ComPort3->Buf,DatLen-2);
    }
    else
    {
        Code.append(ComPort3->Buf,DatLen);
    }
    //----------------------------------------------------------------------//    
    return true;
#else
    if(!ComPort3->isOpen())
    {
        qDebug()<<"Scan1d is error !";
        return false;
    }
    SetCom3State(Com1D);
    Delayms(100);
    ComPort3->readAll();
    Delayms(100);
    SetPinState(Scan1D_TrgPin,0);
    Delayms(3000);
    SetPinState(Scan1D_TrgPin,1);
    Delayms(100);
    Code=ComPort3->readAll();
    qDebug()<<"Scan1d = "<<Code;
    return true;
#endif
}

bool APP_dev::Scan2D_Code(QByteArray &Code)
{
#ifdef WY_COM
    if(ComPort3->Port<0)
    {
        qDebug()<<"Scan2d is error !";
        return false;
    }
    qDebug()<<"-------------- Scan2d is ok !";
    SetCom3State(Com2D);
    Delayms(100);
    //----------------------------------------------------------------------//
    ReadCom(ComPort3,ComPort3->Buf,10000);
    memset(ComPort3->Buf,0,10000);
    Delayms(100);
    SetPinState(Scan2D_TrgPin,0);
//    Delayms(3000);
    Delayms(1000);
    SetPinState(Scan2D_TrgPin,1);
    Delayms(100);
    int DatLen=ReadCom(ComPort3,ComPort3->Buf,10000);
    qDebug("-----------Scan2d=%s",ComPort3->Buf);
    if(DatLen<2 || DatLen>1024)
    {
         return false;
    }
    Code.append(ComPort3->Buf,DatLen-2);
    //----------------------------------------------------------------------//
    return true;
#else
    if(!ComPort3->isOpen())
    {
        qDebug()<<"Scan2d is error !";
        return false;
    }
    SetCom3State(Com2D);
    Delayms(100);
    ComPort3->readAll();
    Delayms(100);
    SetPinState(Scan2D_TrgPin,0);
    Delayms(3000);
    SetPinState(Scan2D_TrgPin,1);
    Delayms(100);
    Code=ComPort3->readAll();
    qDebug()<<"Scan2d = "<<Code;
    return true;
#endif
}

bool APP_dev::GetRtcTime(QString &TimeStr)
{
    QDateTime DateTime;
    QTime time;
    QDate date;
    DateTime.setTime(time.currentTime());
    DateTime.setDate(date.currentDate());
    TimeStr = DateTime.toString("yyyy-MM-dd hh:mm:ss");
    //qDebug()<<"Time = "<<TimeStr;
    return true;
}

QString APP_dev::GetRtcTime()
{
    QString TimeStr;
    QDateTime DateTime;
    QTime time;
    QDate date;
    DateTime.setTime(time.currentTime());
    DateTime.setDate(date.currentDate());
    TimeStr = DateTime.toString("yyyy_MM_dd_hh_mm_ss");
    //qDebug()<<"Time = "<<TimeStr;
    return TimeStr;
}

QString APP_dev::GetDateRtcTime()
{
    QString TimeStr;
    QDateTime DateTime;
    QTime time;
    QDate date;
    DateTime.setTime(time.currentTime());
    DateTime.setDate(date.currentDate());
    TimeStr = DateTime.toString("yyyy-MM-dd hh:mm:ss");
    //qDebug()<<"Time = "<<TimeStr;
    return TimeStr;
}

bool APP_dev::SetRtcTime(QString TimeStr)
{
    //date  -s "2015-5-8 12:48:00"
    QString ShellCmd="date -s \""+TimeStr+"\"";
    SysCall(ShellCmd);
    Delayms(500);
    SysCall("hwclock -w");
    Delayms(500);
    return true;
}

QString APP_dev::GetLocalIP()
{
    QHostAddress IpAddr;
    IpAddr.clear();
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list)
    {
        if ((address.toString() != "127.0.0.1") && (address.protocol() == QAbstractSocket::IPv4Protocol))
        {
            IpAddr= address;
        }
    }
    //QString IP="IP="+IpAddr.toString();
    //qDebug()<<IP;
    return IpAddr.toString();
}

QHostAddress APP_dev::GetLocalIPaddr()
{
    QHostAddress IpAddr;
    IpAddr.clear();
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list)
    {
        if ((address.toString() != "127.0.0.1") && (address.protocol() == QAbstractSocket::IPv4Protocol))
        {
            IpAddr= address;
        }
    }
    //QString IP="IP="+IpAddr.toString();
    //qDebug()<<IP;
    return IpAddr;
}

void APP_dev::ReBoot()
{
    SysCall("reboot ");
}

void APP_dev::SysCall(QString Cmd)
{
    qDebug()<<Cmd;
    QString SYNC = "sync";
    system((char *)Cmd.toStdString().data());
    system((char *)SYNC.toStdString().data());
}

//-------------------------------------------Add---------------------------------------//

void APP_dev::SysCall_process(QString CMD)
{
    QProcess *Process = new QProcess();
//    Process->execute(CMD);
    Process->start(CMD);
    Process->waitForFinished();
    Process->close();
    Process->deleteLater();
}

//---------------------------------------------------------------------------------------//

bool APP_dev::TS_Calibrate()
{
    SysCall("rm -rf  /opt/qta/qt_5.4.0/tslib/etc/pointercal ");
    return true;
}

bool APP_dev::SetLocalIp(QString Ip)
{
    qDebug()<<"set local IP!";
    SysCall("ifconfig eth0 "+Ip);
    return true;
}

QString APP_dev::UpDataApp()
{
    if(yx_udisk==false)
    {
        return "error";
    }
    QString Name = GetPasswd("@UpDataApp");
    if(Name=="No")
    {
        Name="/YiXin_APP.tar";
    }
    QString FileName="/yx-udisk" +Name;
    QString Txt=File1ToFile2(FileName,"./YiXin_APP.tar");
    qDebug()<<Txt;
    return Txt;
}

QString APP_dev::UpDataCfg()
{
    if(yx_udisk==false)
    {
        return "error";
    }
    QString Name=GetPasswd("@UpDataCfg");
    if(Name=="No")
    {
        Name="/config.txt";
    }
    QString FileName="/yx-udisk" + Name + "config_ENG.txt";
    QString Txt=File1ToFile2(FileName,"./config_ENG.txt");
    if(Txt != "ok")
    {
        return Txt;
    }
    FileName="/yx-udisk" + Name + "config_CHN.txt";
    Txt=File1ToFile2(FileName,"./config_CHN.txt");
    return Txt;
}

QString APP_dev::UpDataResualtdb()
{
    // /opt/qta/qt_5.4.0/bin/
    //batch.db#
    //TestResult.db#
    if(yx_udisk==false)
    {
        return "error";
    }
//    QString FileName=GetConfig("UpDataDdb");
//    if(FileName=="No") { FileName="/database.db"; }
//    QString Cmd="cp -f /yx-udisk" +FileName + " ./";
//    SysCall(Cmd);
//    Delayms(100);
//    SysCall("ls -l");
//    qDebug()<<"up data DB ok!";

    QString Name=GetConfig("TestResult");
    if(Name=="No")
    {
        Name="/TestResult.db";
    }
    QString FileName="/yx-udisk" +Name;
    QString Txt=File1ToFile2(FileName,"./TestResult.db");
    qDebug()<<Txt;
    return Txt;
}

QString APP_dev::UpDataBatchdb()
{
    QString Name = GetConfig("Batch");
    if(Name=="No")
    {
        Name="/batch.db";
    }
    QString FileName="/yx-udisk" +Name;
    QString Txt=File1ToFile2(FileName,"./batch.db");
    qDebug()<<Txt;
    return Txt;
}

//----------------------------------------------------Add---------------------------------------------------//
bool APP_dev::File1ToFile2_Shell(QString Dir)
{
    qDebug() << "---------------操作数据库文件-------------";
    QFile *File_ResDB = new QFile();
    QString DBName;
    QString BAKName;
    if(Dir == "IN")             // 导入
    {
        for(int i = 1; i <= 25 ; i++)
        {
//            DBName = "/yx-udisk" + GetConfig("SqlData") + "Yixin" + QString::number(i) + ".db";
            DBName = "/yx-udisk" + GetPasswd("@SqlData") + "Yixin" + QString::number(i) + ".db";
            File_ResDB->setFileName(DBName);
            if(File_ResDB->exists())
            {
                SysCall_process("cp "+DBName + " ./");
            }
            QCoreApplication::processEvents();
            BAKName = "/yx-udisk" + GetPasswd("@SqlData") + "Yixin" + QString::number(i) + ".bak";
            File_ResDB->setFileName(BAKName);
            if(File_ResDB->exists())
            {
                SysCall_process("cp "+BAKName + " ./");
            }
            QCoreApplication::processEvents();
            SysCall_process("sync");
        }
        QString DatebaseName = "/yx-udisk" + GetPasswd("@SqlData") + "SqlMessage.db";

        File_ResDB->setFileName(DatebaseName);
        if(!File_ResDB->exists())
        {
            qDebug() << "the U-Disk datebase Error";
            delete File_ResDB;
            return false;
        }
        SysCall_process("cp "+DatebaseName + " ./");

        DatebaseName = "/yx-udisk" + GetPasswd("@SqlData") + "SqlMessageSave.db";
        File_ResDB->setFileName(DatebaseName);
        if(File_ResDB->exists())
        {
           SysCall_process("cp "+DatebaseName + " ./");
        }

        DatebaseName = "/yx-udisk" + GetPasswd("@SqlData") + "SqlMessage.db";
        File_ResDB->setFileName(DatebaseName);
        if(File_ResDB->exists())
        {
           SysCall_process("cp "+BAKName + " ./");
        }

        DatebaseName = "/yx-udisk" + GetPasswd("@SqlData") + "batch.db";
        File_ResDB->setFileName(DatebaseName);
        if(File_ResDB->exists())
        {
           SysCall_process("cp "+DatebaseName + " ./");
        }
        SysCall_process("sync");
        delete File_ResDB;
        return true;
    }
    else if(Dir == "OUT")   // 导出
    {
        for(int i = 1; i <= 25 ; i++)
        {
            DBName = "./Yixin" + QString::number(i) + ".db";
            File_ResDB->setFileName(DBName);
            if(File_ResDB->exists())
            {
                SysCall_process("cp "+DBName + " /yx-udisk/GPReader/Database/");
            }
            QCoreApplication::processEvents();
            BAKName = "./Yixin" + QString::number(26-i) + ".bak";
            File_ResDB->setFileName(BAKName);
            if(File_ResDB->exists())
            {
                SysCall_process("cp "+BAKName + " /yx-udisk/GPReader/Database/");
            }
            QCoreApplication::processEvents();
            SysCall_process("sync");
        }

        QString DatebaseName = "./SqlMessage.db";
        File_ResDB->setFileName(DatebaseName);
        if(!File_ResDB->exists())
        {
            qDebug() << "the datebase Error";
            delete File_ResDB;
            return false;
        }
        SysCall_process("cp "+DatebaseName + " /yx-udisk/GPReader/Database/");

        DatebaseName = "./SqlMessageSave.db";
        File_ResDB->setFileName(DatebaseName);
        if(File_ResDB->exists())
        {
           SysCall_process("cp "+DatebaseName + " /yx-udisk/GPReader/Database/");
        }

        DatebaseName = "./SqlMessage.bak";
        File_ResDB->setFileName(DatebaseName);
        if(File_ResDB->exists())
        {
           SysCall_process("cp "+DatebaseName + " /yx-udisk/GPReader/Database/");
        }

        DatebaseName = "./batch.db";
        File_ResDB->setFileName(DatebaseName);
        if(File_ResDB->exists())
        {
           SysCall_process("cp "+DatebaseName + " /yx-udisk/GPReader/Database/");
        }
        SysCall_process("sync");
        delete File_ResDB;
        return true;
    }
    else
    {
        qDebug() << "------ 487 Para is error";
        delete File_ResDB;
        return false;
    }
}
//------------------------------------------------------------------------------------------------------------//

QString APP_dev::File1ToFile2(QString File1Name,QString File2Name)
{
    qDebug()<<"copy "+File1Name+" to "+File2Name+" !";
    QFile File1(File1Name);
//    QFile File2(File2Name);
    if(!File1.exists())
    {
        return "error:"+File1Name+" is not exists!";
    }
//    if(!File2.exists())
//    {
//        qDebug()<<File2Name<<"will be recover!";
//        return "error:"+File2Name+" is not exists!";
//    }
    QString CP_CMD = "cp " + File1Name + " " + File2Name;
    qDebug() << "------ 671 ------" << CP_CMD;
    SysCall(CP_CMD);
    return "ok";

//    if(!File1.open(QIODevice::ReadOnly))
//    {
//        return "error:"+File1Name+"open is error!";
//    }
//    QByteArray File1Data=File1.readAll();
//    File1.close();
//    if(File1Data.length()<10)
//    {
//        return "error:"+File1Name+"read is error!";
//    }

//    if(!File2.open(QIODevice::WriteOnly))
//    {
//        return "error:"+File2Name+"open is error!";
//    }
//    qint64 size=File2.write(File1Data);
//    File2.close();
//    if(size!=File1Data.length())
//    {
//        return "error:"+File2Name+"save is error!";
//    }
//    else
//    {
//        return "ok";
//    }
}

//QString APP_dev::OutDataApp()
//{
//    if(yx_udisk==false)
//    {
//        return "error";
//    }
////    SysCall("tar -cvf  ../YiXin_APP.tar ./*");
////    Delayms(100);
////    SysCall("mv -f ../YiXin_APP.tar /yx-udisk/ ");
////    Delayms(100);
////    SysCall("ls -l /yx-udisk/");
////    qDebug()<<"output app  ok!";
//    QString Txt=File1ToFile2("./YX_APP","/yx-udisk/YX_APP.bak");
//    qDebug()<<Txt;
//    return Txt;
//}

QString APP_dev::OutDataCfg()
{
    if(yx_udisk==false)
    {
        return "error";
    }
//    QString Txt=File1ToFile2("./config.txt","/yx-udisk/GPReader/config.bak");
    QString Txt = File1ToFile2("./config_ENG.txt","/yx-udisk/GPReader/config_ENG.bak");
    if(Txt != "ok")
    {
        return Txt;
    }
    Txt = File1ToFile2("./config_CHN.txt","/yx-udisk/GPReader/config_CHN.bak");
////------------------------07-11-14-------Export TimeFile----------------------------------------------------//
//    Txt = File1ToFile2("./PG1.txt","/yx-udisk/GPReader/PG1.txt");
//    Txt = File1ToFile2("./Interpretation.txt","/yx-udisk/GPReader/Interpretation.txt");
////---------------------------------------------------------------------------------------------------------------------//
    qDebug()<<Txt;
    return Txt;
}

QString APP_dev::OutDataResualtdb()
{
    if(yx_udisk==false)
    {
        return "error";
    }
    QString Txt = File1ToFile2("./TestResult.db","/yx-udisk/Result.db");
    qDebug()<<Txt;
    return Txt;
}

QString APP_dev::OutDataBatchdb()
{
    if(yx_udisk==false)
    {
        return "error";
    }
    QString Txt = File1ToFile2("./batch.db","/yx-udisk/Batch.db");
    qDebug()<<Txt;
    return Txt;
}

bool APP_dev::DeleteFile(QString FileName)
{
    SysCall("rm -rf " + FileName+" ");
    return true;
}

bool APP_dev::CreatFile(QString FileName)
{
    QFile File(FileName);
    if(File.open(QIODevice::ReadWrite))
    {
        File.close();
        return true;
    }
    else
    {
        return false;
    }
}

bool APP_dev::WriteTxtToFile(QString FileName, QString Txt)
{
    QFile File(FileName);
    if(!File.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        File.close();
        return false;
    }

    QDateTime da_time;
    QString time_str=da_time.currentDateTime().toString("yyyy-MM-dd HH-mm-ss");
    QTextStream TxtInput(&File);

    TxtInput<<"#------------------------------------#\r\n";
    TxtInput<<"#"<<time_str<<"#\r\n";
    TxtInput<<"#------------------------------------#\r\n";
    TxtInput<<Txt<<"\r\n";
    TxtInput<<"#------------------------------------#\r\n";

    File.close();
    return true;
}

bool APP_dev::ReadTxtFromFile(QString FileName, QString &Txt)
{
    QFile File(FileName);
    if(!File.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        File.close();
        return false;
    }
    Txt=QString(File.readAll());
    return true;
}

int APP_dev::WriteCom(ComStr *ComX, char *DatBuf, int DatLen)
{
    int Len = write(ComX->Port,DatBuf,DatLen);
    return Len;
}

int APP_dev::ReadCom(ComStr *ComX, char *DatBuf, int DatLen)
{
    int Len = read(ComX->Port,DatBuf,DatLen);
    return Len;
}

QByteArray APP_dev::ReadComTimeout(QSerialPort *ComP, int Timeout)
{
    QByteArray Data;
    if(ComP->isOpen())
    {
        Delayms(Timeout);
        Data=ComP->readAll();
    }
    else
    {
        qDebug()<<"com is error!";
    }
    return Data;
}

QByteArray APP_dev::ReadComTimeout(ComStr *ComP, int Timeout)
{
    QByteArray Data;
    if(ComP->State)
    {
        int DatLen=0;
        int Len=0;
        Delayms(Timeout);
        while(Len>=0)
        {
            Delayms(100);
            Len=ReadCom(ComP,ComP->Buf+DatLen,32*1024);
            if(Len > 0)
            {
                DatLen+=Len;
            }
        }
        Data.append(ComP->Buf,DatLen);
//        printf("\r\n");
//        printf("agl rxd length=%d\r\n",DatLen);
//        for(int i=0;i<DatLen;i++)
//        {
//            printf("%x,",ComP->Buf[i]);
//        }
//        printf(" \r\n");
    }
    else
    {
        qDebug()<<"com is error!";
    }
    return Data;
}

bool APP_dev::agl_wr_data(char *DatBuf,int DatLen)
{
    if(AppSta!=AGL_DEV)
    {
        return false;
    }
#ifdef WY_COM
    if(ComPort1->State)
    {
        WriteCom(ComPort1,DatBuf,DatLen);
        return true;
    }
    else
    {
        qDebug()<<"agl com is error!";
        return false;
    }
#else
    if(ComPort1->isOpen())
    {
        ComPort1->write(DatBuf,DatLen);
        return true;
    }
    else
    {
        qDebug()<<"agl com is error!";
        return false;
    }
#endif
}

QByteArray APP_dev::agl_rd_updata(uchar Cmd, int Timeout)
{
    QByteArray ComRxDat;
    if(ComPort1->State)
    {
        Delayms(Timeout);
        int DatLen=ReadCom(ComPort1,ComPort1->Buf,10000);
        ComRxDat.append(ComPort1->Buf,DatLen);
    }
    else
    {
        qDebug()<<"com is error!";
        return ComRxDat;
    }

//    printf("AglDat=");
//    for(int i=0;i<ComRxDat.length();i++)
//    {
//        printf("%x,",ComRxDat.at(i));
//    }
//    printf("\r\n");
    if(ComRxDat.length()<10)
    {
        qDebug()<<"agl rxd data is too short!";
        ComRxDat.clear();
        return ComRxDat;
    }
    char *DatP = ComRxDat.data();
    bool head =false;
    int index=0;
    for(int i=0;i<ComRxDat.length();i++)
    {
        if(DatP[i]=='&' && DatP[i+1]=='A' && DatP[i+2]=='G' && DatP[i+3]=='L'
                && DatP[i+4]==(Cmd+0x80))
        {
            head=true;
            index=i;
        }
    }
    if(!head)
    {
        qDebug()<<"agl rxd data is not find head!";
        ComRxDat.clear();
        return ComRxDat;
    }
    u_int16_t Len=DatP[index+9]*256+DatP[index+10];
    if(ComRxDat.length()<(int)Len+14)
    {
        qDebug()<<"agl rxd data is short!";
        ComRxDat.clear();
        return ComRxDat;
    }
//    qDebug()<<"CRC!";
    u_int16_t CRCget=DatP[index+Len+11]*256+DatP[index+Len+12];
//    printf("DatLen=%d\r\n",Len);
//    printf("CRCget=%d\r\n",CRCget);
    u_int16_t CRC16 =GetCRC(DatP+index+11,Len);
//    printf("CRC16=%d\r\n",CRC16);
    if(CRC16 != CRCget)
    {
        qDebug()<<"agl rxd data crc is error!";
        ComRxDat.clear();
        return ComRxDat;
    }
    QByteArray RxData;
    RxData.clear();
    RxData.append(DatP+index+5,2+2+2+Len);
    return RxData;
    //------------------------//
}

//-------------------for test------------------//
//QByteArray APP_dev::ReadPcComData(void)
//{
//    QByteArray ComRxDat=ReadComTimeout(ComPortTest,1000);
//    return ComRxDat;
//}
//-----------------------------------------------//
QByteArray APP_dev::agl_rd_data(uchar Cmd, int Timeout)        // 读取下位机串口数据
{
    QByteArray ComRxDat=ReadComTimeout(ComPort1,Timeout);
    if(ComRxDat.length()<10)
    {
//        qDebug()<<"agl rxd data is too short!";
        ComRxDat.clear();
        return ComRxDat;
    }

    char *DatP = ComRxDat.data();
    bool head = false;
    int index = 0;
    for(int i=0;i<ComRxDat.length();i++)
    {
        if(DatP[i]=='&' && DatP[i+1]=='A' && DatP[i+2]=='G' && DatP[i+3]=='L'
                && DatP[i+4]==(Cmd+0x80))
        {
            head=true;
            index=i;
        }
    }
    if(!head)
    {
        qDebug()<<"agl rxd data is not find head!";
        ComRxDat.clear();
        return ComRxDat;
    }
    u_int16_t Len=DatP[index+5]*256+DatP[index+6];
    if(ComRxDat.length()<(int)Len+10)
    {
        qDebug()<<"agl rxd data is short!";
        ComRxDat.clear();
        return ComRxDat;
    }
    u_int16_t CRCget=DatP[index+Len+7]*256+DatP[index+Len+8];    
    u_int16_t CRC16 =GetCRC(DatP+index+7,Len);
 //   printf("Cmd=%x,CRCget=%x,CRC16=%x\r\n",Cmd+0x80,CRCget,CRC16);
    if(CRC16 != CRCget)
    {
        qDebug()<<"agl rxd data crc is error!";
        ComRxDat.clear();
        return ComRxDat;
    }
    QByteArray RxData;
    RxData.clear();
    RxData.append(DatP+index+7,Len);
    return RxData;
    //------------------------//
}

bool APP_dev::cgr_wr_data(char *DatBuf,int DatLen)
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
#ifdef WY_COM
    if(ComPort1->State)
    {
        WriteCom(ComPort1,DatBuf,DatLen);
        return true;
    }
    else
    {
        qDebug()<<"agl com is error!";
        return false;
    }
#else
    if(ComPort1->isOpen())
    {
        ComPort1->write(DatBuf,DatLen);
        return true;
    }
    else
    {
        qDebug()<<"agl com is error!";
        return false;
    }
#endif
}
QByteArray APP_dev::cgr_rd_data(uchar Cmd, int Timeout)
{
    //------------------------//
    QByteArray ComRxDat=ReadComTimeout(ComPort1,Timeout);
    if(ComRxDat.length()<10)
    {
        //qDebug()<<"Rxd Error!!";
        ComRxDat.clear();
        return ComRxDat;
    }
    char *DatP = ComRxDat.data();
    bool head =false;
    int index=0;
    for(int i=0;i<ComRxDat.length();i++)
    {
        if(DatP[i]=='&' && DatP[i+1]=='C' && DatP[i+2]=='G' && DatP[i+3]=='R'
                && DatP[i+4]==(Cmd+0x80))
        {
            head=true;
            index=i;
        }
    }
    if(!head)
    {
        qDebug()<<"the rxd data is not find head!";
        ComRxDat.clear();
        return ComRxDat;
    }
    u_int16_t Len=DatP[index+5]*256+DatP[index+6];
    if(ComRxDat.length()<(int)Len+10)
    {
        qDebug()<<"the rxd data is short !";
        ComRxDat.clear();
        return ComRxDat;
    }
    u_int16_t CRCget=DatP[index+Len+7]*256+DatP[index+Len+8];
    u_int16_t CRC16 =GetCRC(DatP+index+7,Len);
    if(CRC16 != CRCget)
    {
        qDebug()<<"the rxd data crc is error!";
        ComRxDat.clear();
        return ComRxDat;
    }
    QByteArray RxData;
    RxData.clear();
    RxData.append(DatP+index+7,Len);
    return RxData;
    //------------------------//
}

QByteArray APP_dev::pc_rd_data()
{
    QByteArray Data;
    Data.clear();
#ifdef WY_COM
    if(ComPort2->State)
    {
        int DatLen=ReadCom(ComPort2,ComPort2->Buf,10000);
        Data.append(ComPort2->Buf,DatLen);
        memset(ComPort2->Buf,0,10000);
    }
#else
    if(ComPort2->isOpen())
    {
        Data=ComPort2->readAll();
    }
#endif
     return Data;
}

bool APP_dev::pc_wr_data(QByteArray Data)
{
#ifdef WY_COM
    if(ComPort2->State)
    {
        int RetLeng=0;
        int DatStart=0;
        int DatLeng=Data.length();
        while(1)
        {
            RetLeng=WriteCom(ComPort2,Data.data()+DatStart,DatLeng);
            if(RetLeng<0)
            {
                //qDebug()<<"RetLeng="<<RetLeng;
                Delayms(100);
            }
            else if(RetLeng==DatLeng)
            {
                return true;
            }
            else
            {
                DatStart+=RetLeng;
                DatLeng-=RetLeng;
                qDebug()<<"RetLeng="<<RetLeng;
            }
        }
        return true;
    }
    else
    {
        qDebug()<<"pc com is error!";
        return false;
    }
#else
    if(ComPort2->isOpen())
    {
        ComPort2->write(Data);
        return true;
    }
    else
    {
        qDebug()<<"pc com is error!";
        return false;
    }
#endif
}

static char CmdBuf[1080];
QString APP_dev::agl_updata_mcu()
{
//    printf("------------------------\r\n");
    qDebug()<<"updata mcu bin!";
    //AglMcuBin=AglMcu.bin#
    QString BinName;
//    QString Name=GetConfig("AglMcuBin");
    QString Name=GetPasswd("@AglMcuBin");
    if(Name=="No")
    {
        qDebug()<<"no get name of updata file!";
        Name="./AglMcu.bin";
        BinName="./AglMcu.bin";
    }
    else
    {
        BinName ="/yx-udisk"+Name;
    }
    qDebug()<<"updata file="<<BinName<<"!";
    QFile BinFile;
    BinFile.setFileName(BinName);
    if(!BinFile.exists())
    {
        return "updata file is not exits!";
    }
    if(!BinFile.open(QIODevice::ReadOnly))
    {
        return "updata file open error!";
    }
    McuUpDataP =new McuUpDataStr();
    McuUpDataP->BinDat.clear();
    McuUpDataP->BinDat=BinFile.readAll();
    BinFile.close();

    if(McuUpDataP->BinDat.length()<64)
    {
        return "updata file is error!";
    }
    McuUpDataP->MaxNum=0;//Head
    McuUpDataP->PreNum=0;
    McuUpDataP->DatStar=0;
    McuUpDataP->DatLen=0;

    BinHeadStr *BinHeadP=(BinHeadStr *)(McuUpDataP->BinDat.data());
    McuUpDataP->MaxNum=(McuUpDataP->BinDat.length()-64)/1024;
    if((McuUpDataP->BinDat.length()-64)%1024>0)
    {
        McuUpDataP->MaxNum+=1;
    }
    McuUpDataP->MaxNum+=1;//Head
    McuUpDataP->PreNum=0;
    McuUpDataP->DatStar=0;
    McuUpDataP->DatLen=0;

    printf("------------------------\r\n");
    QString Txt;
    Txt += "Head=";
    for(int i=0;i<10;i++) { Txt.append(BinHeadP->Head[i]); }
    Txt += "\r\n";
    Txt += "Ver=";
    QString Version;
    for(int i=0;i<10;i++) { Txt.append(BinHeadP->Ver[i]);Version.append(BinHeadP->Ver[i]); }
    Txt += "\r\n";
    Txt += "Date=";
    QString Date;
    for(int i=0;i<12;i++) { Txt.append(BinHeadP->Date[i]);Date.append(BinHeadP->Date[i]); }
    Txt += "\r\n";
    Txt += "MD5=";
    for(int i=0;i<32;i++) { Txt.append(BinHeadP->MD5[i]); }
    Txt += "\r\n";
    Txt += "Size=";
    Txt +=QString::number(McuUpDataP->BinDat.length(),10);
    Txt += "\r\n";
    Txt += "Num=";
    Txt +=QString::number(McuUpDataP->MaxNum,10);
    Txt += "\r\n";
    qDebug()<<Txt;
    printf("------------------------\r\n");
    if(memcmp(BinHeadP->Head,"[YiXinBIO]",10)!=0)
    {
        return "updata file is error!";
    }

    McuUpDataP->BinName=Name;
    McuUpDataP->BinVersion=Version;
    McuUpDataP->BinDate=Date;

    McuUpDataP->State="waiting";
    McuUpDataP->TimerP=new QTimer();
    McuUpDataP->TimerP->setInterval(500);
    connect(McuUpDataP->TimerP,SIGNAL(timeout()),this,SLOT(UpDataTimeout()));
    McuUpDataP->TimerP->start();
    return "ok";
}

void APP_dev::UpDataTimeout()
{
    static uchar FirstFlag=0;
    static char ErorCont=0;
    bool RxdFlag=false;
    //RXD
    if(FirstFlag==0)
    {
        McuUpDataP->PreNum=0;
        McuUpDataP->DatStar=0;
        McuUpDataP->DatLen=64;
        FirstFlag=1;
        ErorCont=0;
    }
    else
    {
        McuUpDataP->RxdDat.clear();
        McuUpDataP->RxdDat=agl_rd_updata(0x0f,1);
        if(McuUpDataP->RxdDat.length()==7)
        {
            char *DatP=McuUpDataP->RxdDat.data();
            int16_t RetMaxNum=DatP[0]*256+DatP[1];
            int16_t RetPreNum=DatP[2]*256+DatP[3];
            int16_t RetDatLen=DatP[4]*256+DatP[5];
            int8_t RetDat=DatP[7];
            if(RetMaxNum==McuUpDataP->MaxNum && RetPreNum==McuUpDataP->PreNum && RetDatLen==1 && RetDat==0)
            {
                RxdFlag=true;
            }
        }
        if(RxdFlag)
        {
            ErorCont=0;
            McuUpDataP->State="waiting";
            McuUpDataP->DatStar+=McuUpDataP->DatLen;
            McuUpDataP->PreNum++;
            if(McuUpDataP->PreNum>=McuUpDataP->MaxNum)
            {
                FirstFlag=0;
                McuUpDataP->State="ok";
                McuUpDataP->TimerP->stop();
                McuUpDataP->TimerP->deleteLater();
                qDebug()<<"Up Data ok!";
                return;
            }
            McuUpDataP->DatLen=McuUpDataP->BinDat.length()-McuUpDataP->DatStar;
            if(McuUpDataP->DatLen>=1024)
            {
                McuUpDataP->DatLen =1024;
            }
        }
        else
        {
            if(++ErorCont<8)
            {
                WriteCom(ComPort1,CmdBuf,McuUpDataP->DatLen+16);
                qDebug()<<"ReUp Data Frame="<<QString::number(McuUpDataP->PreNum,10)<<"!";
                return;
            }
            else
            {
                McuUpDataP->State="mcu rxd data is error 3 !";
                FirstFlag=0;
                if(McuUpDataP->TimerP->isActive())
                {
                    McuUpDataP->TimerP->stop();
                    McuUpDataP->TimerP->deleteLater();
                }
                return;
            }
        }
    }

    //TXD
    memset(CmdBuf,0,1080);
    CmdBuf[0]=0x55;
    CmdBuf[1]='&';
    CmdBuf[2]='A';
    CmdBuf[3]='G';
    CmdBuf[4]='L';
    CmdBuf[5]=0x0f;
    CmdBuf[6]=McuUpDataP->MaxNum/256;
    CmdBuf[7]=McuUpDataP->MaxNum%256;
    CmdBuf[8]=McuUpDataP->PreNum/256;
    CmdBuf[9]=McuUpDataP->PreNum%256;
    CmdBuf[10]=McuUpDataP->DatLen/256;
    CmdBuf[11]=McuUpDataP->DatLen%256;
    memcpy(&CmdBuf[12],McuUpDataP->BinDat.data()+McuUpDataP->DatStar,McuUpDataP->DatLen);
    u_int16_t CRC16 =GetCRC(&CmdBuf[12],McuUpDataP->DatLen);
    CmdBuf[12+McuUpDataP->DatLen]=uchar(CRC16>>8);
    CmdBuf[13+McuUpDataP->DatLen]=uchar(CRC16>>0);
    CmdBuf[14+McuUpDataP->DatLen]='\n';
    CmdBuf[15+McuUpDataP->DatLen]=0x55;
    WriteCom(ComPort1,CmdBuf,McuUpDataP->DatLen+16);
    qDebug()<<"Up Data Frame="<<QString::number(McuUpDataP->PreNum,10)<<"!";
    //qDebug()<<"Up Data Start ="<<QString::number(McuUpDataP->DatStar,10)<<"!";
    //qDebug()<<"Up Data Len="<<QString::number(McuUpDataP->DatLen,10)<<"!";
    return;
}

bool APP_dev::agl_rd_ver()
{
    qDebug()<<"agl read version!";
    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x00,0x00,0x00,0xff,0xff,'\n',0x55,0x55,0x55,0x55};
    if(!agl_wr_data(CmdBuf,16))
    {
        return false;
    }
    QByteArray RxData = agl_rd_data(0x00,1000);
    if(RxData.length() == 0)
    {
        Error_Code = Error_NoCom;
        qDebug() << "Get ver error";
        return false;
    }
    if(RxData.length()<2)
    {
        return false;
    }
    for(int index=0;index<RxData.length();index++)
    {
        AglDev.Version[index]=RxData.at(index);
    }
    qDebug("agl mcu ver=%s\r\n",AglDev.Version);
    return true;
}

bool APP_dev::GetCheckError()
{
    QByteArray RxData = agl_rd_data(0x33,1000);
    if(RxData.length() == 0)
    {
        Error_Code = Error_OK;
        return true;
    }

    bool isCodeError = false;
    unsigned char *Data = (unsigned char *)RxData.data();
    switch(Data[0])
    {
        case 0x01:
        {
            qDebug() << "001 SPI通信出错，可能是MCU板和PD板的接线接触不良";
            Error_Code = Error_SPI;
            break;
        }
        case 0x02:
        {
            Error_Code = Error_X;
            qDebug() << "002 X轴电机线没接好或是X轴光电开关失效";
            break;
        }
        case 0x03:
        {
            Error_Code = Error_Y;
            qDebug() << "003 Y轴电机线没接好或是Y轴里/外面的光电开关失效";
            break;
        }
        case 0x04:
        {
            Error_Code = Error_Y_Max;
            qDebug() << "004 Y轴运动过程中过零位误差过大";
            break;
        }
        case 0x05:
        {
            Error_Code = Error_Chip;
            qDebug() << "005 参数写入存储芯片错误";
            break;
        }
        default:
        {
            Error_Code = Error_Err;
            qDebug() << "Error 自检通信出错";
            isCodeError = true;
            break;
        }
    }
    if(isCodeError == true)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool APP_dev::agl_rd_stat()
{
//    qDebug()<<"agl read state!";
    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x01,0x00,0x00,0xff,0xff,'\n',0x55,0x55,0x55,0x55};
    if(!agl_wr_data(CmdBuf,16))
    {
        return false;
    }
    QByteArray RxData=agl_rd_data(0x01,1000);
    //qDebug("agl_rd_stat DatLen=%d",RxData.length());
    if(RxData.length()!=5)
    {
        return false;
    }
    char *DatP = RxData.data();
    AglDev.LedCurrent=DatP[0];
    AglDev.PdTemperature=DatP[1];
    AglDev.LedTemperature=DatP[2];
    AglDev.OutIntState=DatP[3];
    AglDev.AdcState=DatP[4];

    QString State;
//    State += ("灯电流"+QString::number(AglDev.LedCurrent,10)+",");
//    State += ("PD温度"+QString::number(AglDev.PdTemperature,10)+",");
//    State += ("灯温度"+QString::number(AglDev.LedTemperature,10)+",");
//    if(AglDev.OutIntState==0x00)
//    {
//        State += ("仓空闲,");
//    }
//    if(AglDev.OutIntState==0x01)
//    {
//        State += ("仓运行,");
//    }
//    if(AglDev.OutIntState==0x02)
//    {
//        State += ("入仓,");
//    }
//    if(AglDev.OutIntState==0x03)
//    {
//        State += ("出仓,");
//    }
//    if(AglDev.AdcState==0x00)
//    {
//        State += ("ADC空闲");
//    }
//    if(AglDev.AdcState==0x01)
//    {
//        State += ("ADC运行");
//    }
//    if(AglDev.AdcState==0x02)
//    {
//        State += ("ADC完成");
//    }
    //qDebug("AglDev.AdcState=%d",AglDev.AdcState);
    State += ("LED"+QString::number(AglDev.LedCurrent,10)+",");
    State += ("PDt"+QString::number(AglDev.PdTemperature,10)+",");
    State += ("LEDt"+QString::number(AglDev.LedTemperature,10)+",");
    if(AglDev.OutIntState==0x00)
    {
        State += ("door nop,");
    }
    if(AglDev.OutIntState==0x01)
    {
        State += ("door run,");
    }
    if(AglDev.OutIntState==0x02)
    {
        State += ("int,");
    }
    if(AglDev.OutIntState==0x03)
    {
        State += ("out,");
    }
    if(AglDev.AdcState==0x00)
    {
        State += ("adc nop,");
    }
    if(AglDev.AdcState==0x01)
    {
        State += ("adc run,");
    }
    if(AglDev.AdcState==0x02)
    {
        State += ("adc ok,");
    }
//    qDebug()<<State;
    return true;
}

bool APP_dev::agl_rd_pm()
{
    qDebug()<<"agl read pm!";
    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x02,0x00,0x00,0xff,0xff,'\n',0x55,0x55,0x55,0x55,};
    if(!agl_wr_data(CmdBuf,16))
    {
        return false;
    }
    QByteArray RxData=agl_rd_data(0x02,1000);
    if(RxData.length() == 0)
    {
        Error_Code = Error_NoCom;
        qDebug() << "Get pm error";
        return false;
    }
    if(RxData.length()!=4)
    {
        return false;
    }
    char *DatP = RxData.data();
    AglDev.MotorDX=DatP[0];
    AglDev.MotorDY=DatP[1];
    AglDev.LedSetVol=DatP[2];
    AglDev.ChxVol=DatP[3];
    QString State;
    State += ("Get MotoX"+QString::number(DatP[0],10)+",");
    State += ("MotoY"+QString::number(DatP[1],10)+",");
    State += ("Led"+QString::number(DatP[2],10)+",");
    State += ("Card"+QString::number(DatP[3],10));
    qDebug()<<State;
    return true;
}

bool APP_dev::agl_wr_pm()
{
    qDebug()<<"agl write pm!";
    char CmdBuf[16]={0x55,'&','A','G','L',0x03,0x00,0x04,0x00,0x00,0x00,0x00,0xff,0xff,'\n',0x55};
    CmdBuf[8]=AglDev.MotorDX;
    CmdBuf[9]=AglDev.MotorDY;
    CmdBuf[10]=AglDev.LedSetVol;
    CmdBuf[11]=AglDev.ChxVol;
    u_int16_t CRC16 =GetCRC(CmdBuf+8,4);
    CmdBuf[12]=uchar(CRC16>>8);
    CmdBuf[13]=uchar(CRC16>>0);
    //--------------------------------//
    if(!agl_wr_data(CmdBuf,16))
    {
        return false;
    }
    QByteArray RxData=agl_rd_data(0x03,1000);
    //--------------------------------//
    QString State;
    State += ("Set MotoX"+QString::number(CmdBuf[8],10)+",");
    State += ("MotoY"+QString::number(CmdBuf[9],10)+",");
    State += ("Led"+QString::number(CmdBuf[10],10)+",");
    State += ("Card"+QString::number(CmdBuf[11],10));
    qDebug()<<State;
    return true;
}

bool APP_dev::agl_out()
{
    if(isDoor == true)
    {
        return false;
    }
    isDoor = true;
    qDebug()<<"agl out! ";
    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x04,0x00,0x00,0xff,0xff,'\n',0x55,0x55};
    //--------------------------------//
    if(!agl_wr_data(CmdBuf,16))
    {
        return false;
    }
//    QByteArray RxData=agl_rd_data(0x04,1000);
    //--------------------------------//
//    Delayms(1000);
//    agl_get();
    isDoor = false;
    return true;
}

bool APP_dev::agl_int()
{
    if(isDoor == true)
    {
        return false;
    }
    isDoor = true;
    qDebug()<<"agl int! ";
    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x05,0x00,0x00,0xff,0xff,'\n',0x55,0x55};
    //--------------------------------//
    if(!agl_wr_data(CmdBuf,16))
    {
         isDoor = false;
        return false;
    }
//    QByteArray RxData=agl_rd_data(0x05,1000);
    //--------------------------------//
//    Delayms(1000);
//    agl_get();
    isDoor = false;
    return true;
}

bool APP_dev::agl_adc(char CarNumb)
{
    qDebug()<<"agl start! ";
//    agl_rd_pm();
//    if(AglDev.ChxVol!=CarNumb)
//    {
//        AglDev.ChxVol=CarNumb;
//        agl_wr_pm();
//    }
    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x06,0x00,0x00,0xff,0xff,'\n',0x55,0x55};
    //--------------------------------//
    if(!agl_wr_data(CmdBuf,16))
    {
        return false;
    }
    QByteArray RxData=agl_rd_data(0x06,1000);
    //--------------------------------//
    return true;
}

bool APP_dev::agl_get()
{
    qDebug()<<"agl read data!";
    char CmdBuf[16]={0x55,0x55,'&','A','G','L',0x07,0x00,0x00,0xff,0xff,'\n',0x55,0x55};
    //--------------------------------//
    if(!agl_wr_data(CmdBuf,16))
    {
        return false;
    }
    QByteArray RxData=agl_rd_data(0x07,1000);
    //--------------------------------//
    qDebug("agl adc data length=%d !",RxData.length());
    memset((char*)AglDev.ChxBuf,0,2000*4*sizeof(double));
    memset((char*)AglDev.BakBuf,0,2000*4*sizeof(double));
    memset((char*)AglDev.VolBuf,0,2000*4*sizeof(double));
    uchar *DatP = (uchar *)RxData.data();
    double *ChxBuf;
    double *VolBuf ;
    int Temp=0;
    int ChPos=0;
    if(AglDev.ChxVol==1)
    {
        if(RxData.length()!=1*3600)
        {
            qDebug()<<"agl adc data is error!";
            return false;
        }
        ChxBuf=&AglDev.ChxBuf[0][0];
        VolBuf =&AglDev.VolBuf[0][0];
        for(u_int16_t i=0;i<(1800);i++)
        {
            Temp=((int)DatP[ChPos+i*2+0]*256)+DatP[ChPos+i*2+1];
            ChxBuf[i]=(double)(Temp);
            VolBuf[i] =(double)(Temp);//数值
        }
    }
    if(AglDev.ChxVol==4)
    {
        if(RxData.length()!=4*3600)
        {
            qDebug()<<"agl adc data is error!";
            return false;
        }

        ChPos=3600*0;
        ChxBuf=&AglDev.ChxBuf[3][0];
        VolBuf =&AglDev.VolBuf[3][0];
        for(int i=0;i<(1800);i++)
        {
            Temp=((int)DatP[ChPos+i*2+0]*256)+DatP[ChPos+i*2+1];
            ChxBuf[i]=(double)(Temp);
            VolBuf[i] =(double)(Temp);//数值
        }

        ChPos=3600*1;
        ChxBuf=&AglDev.ChxBuf[2][0];
        VolBuf =&AglDev.VolBuf[2][0];
        for(int i=0;i<(1800);i++)
        {
            Temp=((int)DatP[ChPos+i*2+0]*256)+DatP[ChPos+i*2+1];
            ChxBuf[i]=(double)(Temp);
            VolBuf[i] =(double)(Temp);//数值
        }

        ChPos=3600*2;
        ChxBuf=&AglDev.ChxBuf[1][0];
        VolBuf =&AglDev.VolBuf[1][0];
        for(int i=0;i<(1800);i++)
        {
            Temp=((int)DatP[ChPos+i*2+0]*256)+DatP[ChPos+i*2+1];
            ChxBuf[i]=(double)(Temp);
            VolBuf[i] =(double)(Temp);//数值
        }

        ChPos=3600*3;
        ChxBuf=&AglDev.ChxBuf[0][0];
        VolBuf =&AglDev.VolBuf[0][0];
        for(int i=0;i<1800;i++)
        {
            Temp=((int)DatP[ChPos+i*2+0]*256)+DatP[ChPos+i*2+1];
            ChxBuf[i]=(double)(Temp);
            VolBuf[i] =(double)(Temp);//数值
        }

//------------------------------------------Add-------------------------------------//
        for(int i = 0; i < 4; i++)
        {
            for(int j = 0; j < 2000; j++)
            {
                if(AglDev.VolBuf[i][j] > 32768.0)
                {
                    AglDev.VolBuf[i][j] = 0;
                }
            }
        }
//-------------------------------------------------------------------------------------//

//        //save agl.txt
//        QDateTime da_time;
//        QString time_str=da_time.currentDateTime().toString("yyyy-MM-dd-HH-mm-ss");
//        //QFile ImageFile("./agl-"+time_str+".txt");
//        QFile ImageFile("./agl.txt");
//        if(!ImageFile.open(QIODevice::WriteOnly  | QIODevice::Text))
//        {
//           return true;
//        }
//        QTextStream ImageInput(&ImageFile);

//        ImageInput<<time_str<<"\r\n";
//        ImageInput<<"CH1,CH2,CH3,CH4,\r\n";
//        for(int i=0;i<AdcMaxCycle;i++)
//        {
//            QString value;
//            value +=(QString::number(AglDev.ChxBuf[0][i],'g',10)+",");
//            value +=(QString::number(AglDev.ChxBuf[1][i],'g',10)+",");
//            value +=(QString::number(AglDev.ChxBuf[2][i],'g',10)+",");
//            value +=(QString::number(AglDev.ChxBuf[3][i],'g',10)+",");
//            value += "\r\n";
//            ImageInput<<value;
//        }
//        ImageFile.close();
    }
    return true;
}

//save adc.txt
//    QDateTime da_time;
//    QString time_str=da_time.currentDateTime().toString("yyyy-MM-dd-HH-mm-ss");
//    QFile ImageFile("/udisk/agl/agl-"+time_str+".txt");
//    if(!ImageFile.open(QIODevice::ReadWrite  | QIODevice::Text))
//    {
//       return true;
//    }
//    QTextStream ImageInput(&ImageFile);

//    ImageInput<<time_str<<"\r\n";
//    ImageInput<<"CH1,CH2,CH3,CH4,\r\n";
//    for(int i=0;i<AdcMaxCycle;i++)
//    {
//        QString value;
//        value +=(QString::number(AglDev.ChxBuf[0][i],'g',10)+",");
//        value +=(QString::number(AglDev.ChxBuf[1][i],'g',10)+",");
//        value +=(QString::number(AglDev.ChxBuf[2][i],'g',10)+",");
//        value +=(QString::number(AglDev.ChxBuf[3][i],'g',10)+",");
//        value += "\r\n";
//        ImageInput<<value;
//    }
//    ImageFile.close();

bool APP_dev::agl_read_pm()
{
    return true;
//    QByteArray TwoCode;
//    bool flag=Scan2D_Code(TwoCode);
//    QString TwoCodeString;
//    TwoCodeString.append(TwoCode);
//    qDebug()<<"TwoCode = "<<TwoCodeString;
//    return flag;
}


bool APP_dev::agl_save_pm()
{
    return true;
//    if(AppSta!=AGL_DEV)
//    {
//        return false;
//    }
//    //char *DatP=(char*)&(AglDev.CardPm);
//    QByteArray ImgDat;
//    ImgDat.clear();
//    //for(uint index=0;index<4*sizeof(TwoCardStr);index++)
//    {
//        ImgDat.append(DatP[index]);
//    }
//    QFile ImgFile("CardPm.bin");
//    if(!ImgFile.open(QIODevice::ReadWrite))
//    {
//        return false;
//    }
//    ImgFile.write(ImgDat);
//    ImgFile.close();
//    return true;
}

bool APP_dev::agl_Load_pm()
{
    return true;
//    if(AppSta!=AGL_DEV)
//    {
//        return false;
//    }
//    //char *DatP=(char*)&(AglDev.CardPm);
//    //memset(DatP,0,4*sizeof(TwoCardStr));
//    QFile ImgFile("CardPm.bin");
//    if(!ImgFile.exists())
//    {
//        return false;
//    }
//    if(!ImgFile.open(QIODevice::ReadOnly))
//    {
//        return false;
//    }
//    QByteArray ImgDat = ImgFile.readAll();
//    ImgFile.close();

//    for(int index=0;index<ImgDat.length();index++)
//    {
//        DatP[index]=ImgDat.at(index);
//    }
//    return true;
}

bool APP_dev::agl_save_back()
{
//    if(AppSta!=AGL_DEV)
//    {
//        return false;
//    }
//    return true;
//    double *DatP1=(double*)&(AglDev.ChxBuf[0][0]);
//    double *DatP2=(double*)&(AglDev.BakBuf[0][0]);
//    memcpy(DatP2,DatP1,4*AdcMaxCycle*sizeof(double));
//    QByteArray ImgDat;
//    ImgDat.clear();
//    char *DatP=(char*)&(AglDev.BakBuf[0][0]);
//    for(uint index=0;index<4*AdcMaxCycle*sizeof(double);index++)
//    {
//        ImgDat.append(DatP[index]);
//    }
//    QFile ImgFile("backdat.bin");
//    if(!ImgFile.open(QIODevice::ReadWrite))
//    {
//        return false;
//    }
//    ImgFile.write(ImgDat);
//    ImgFile.close();
    return true;
}

bool APP_dev::agl_load_back()
{
//    if(AppSta!=AGL_DEV)
//    {
//        return false;
//    }
//    return true;
//    char *DatP=(char*)&(AglDev.BakBuf[0][0]);
//    memset(DatP,0,4*AdcMaxCycle*sizeof(double));
//    QFile ImgFile("backdat.bin");
//    if(!ImgFile.exists())
//    {
//        return false;
//    }
//    if(!ImgFile.open(QIODevice::ReadOnly))
//    {
//        return false;
//    }
//    QByteArray ImgDat = ImgFile.readAll();
//    ImgFile.close();

//    for(int index=0;index<ImgDat.length();index++)
//    {
//        DatP[index]=ImgDat.at(index);
//    }
    return true;
}

bool APP_dev::agl_clear_back()
{
//    if(AppSta!=AGL_DEV)
//    {
//        return false;
//    }
//    return true;
//    char *DatP=(char*)&(AglDev.BakBuf[0][0]);
//    memset(DatP,0,4*AdcMaxCycle*sizeof(double));
//    return QFile::remove("backdat.bin");
    return true;
}

TwoCodeStr APP_dev::agl_get_two(QString Code1D)
{
    qDebug()<<"Code1D="<<Code1D;
    SqlDataP =new TestSql(this);
    SqlDataP->init_BatchSql("DEV");

    QString Code2D;
    SqlDataP->GetDetailInforBatchSql(Code2D,Code1D);
    SqlDataP->deleteLater();
    qDebug()<<"Code2D="<<Code2D;

    QStringList StringList=Code2D.split(",");
    TwoCodeStr TwoCode;
    TwoCode.Head="NO";
    if(StringList.at(0).mid(0,3)=="YX-")
    {
        TwoCode.Head=StringList.at(0).mid(0,3);//YX-
        qDebug()<<"Head="<<TwoCode.Head;
        TwoCode.Pihao=StringList.at(0);//批号
        qDebug()<<"Pihao="<<TwoCode.Pihao;
        TwoCode.Yiweima=StringList.at(1);//一维码
        qDebug()<<"Yiweima="<<TwoCode.Yiweima;
        TwoCode.Xiangmushuliang=StringList.at(2);//项目数量
        qDebug()<<"Xiangmushuliang="<<TwoCode.Xiangmushuliang;
        TwoCode.PiLiang=StringList.at(3);//批量
        TwoCode.youxiaoshijian=StringList.at(4);//有效时间
        qDebug()<<"youxiaoshijian="<<TwoCode.youxiaoshijian;
        TwoCode.yanshi=StringList.at(5);//时间
        qDebug()<<"yanshi="<<TwoCode.yanshi;
        for(int i=0;i<TwoCode.Xiangmushuliang.toInt();i++)
        {
            TwoCode.XiangMuPm[i].xiangmu=StringList.at(6+i*19+0);//项目名称
            TwoCode.XiangMuPm[i].xiangmudanwei=StringList.at(6+i*19+1);//项目单位
            TwoCode.XiangMuPm[i].czuidizhi=StringList.at(6+i*19+2);//C线极限低值
            TwoCode.XiangMuPm[i].czuigaozhi=StringList.at(6+i*19+3);//C线极限高值
            TwoCode.XiangMuPm[i].WB_Para=StringList.at(6+i*19+4);//C线起点
            TwoCode.XiangMuPm[i].Equation_Limit=StringList.at(6+i*19+5);//C线终点
            TwoCode.XiangMuPm[i].Equation_Max=StringList.at(6+i*19+6);//T线起点
            TwoCode.XiangMuPm[i].NOP=StringList.at(6+i*19+7);//T线终点
            TwoCode.XiangMuPm[i].anongduzhi=StringList.at(6+i*19+8);//浓度参考值A
            TwoCode.XiangMuPm[i].bnongduzhi=StringList.at(6+i*19+9);//浓度参考值B
            TwoCode.XiangMuPm[i].cankaoxingshi=StringList.at(6+i*19+10);//浓度参考显示形式
            TwoCode.XiangMuPm[i].nongdudizhi=StringList.at(6+i*19+11);//浓度极限低值
            TwoCode.XiangMuPm[i].nongdugaozhi=StringList.at(6+i*19+12);//浓度极限高值
            TwoCode.XiangMuPm[i].jisuanfangshi=StringList.at(6+i*19+13);//计算方式
            TwoCode.XiangMuPm[i].jisuanfangfa=StringList.at(6+i*19+14);//计算浓度方法
            TwoCode.XiangMuPm[i].canshua=StringList.at(6+i*19+15);//参数A
            TwoCode.XiangMuPm[i].canshub=StringList.at(6+i*19+16);//参数B
            TwoCode.XiangMuPm[i].canshuc=StringList.at(6+i*19+17);//参数C
            TwoCode.XiangMuPm[i].canshud=StringList.at(6+i*19+18);//参数D

//            qDebug()<<"xiangmu"<<TwoCode.XiangMuPm[i].xiangmu;
//            qDebug()<<"xiangmudanwei"<<TwoCode.XiangMuPm[i].xiangmudanwei;
//            qDebug()<<"czuidizhi"<<TwoCode.XiangMuPm[i].czuidizhi;
//            qDebug()<<"czuigaozhi"<<TwoCode.XiangMuPm[i].czuigaozhi;
//            qDebug()<<"cqidian"<<TwoCode.XiangMuPm[i].cqidian;
//            qDebug()<<"czhongdian"<<TwoCode.XiangMuPm[i].czhongdian;
//            qDebug()<<"tqidian"<<TwoCode.XiangMuPm[i].tqidian;
//            qDebug()<<"tzhongdian"<<TwoCode.XiangMuPm[i].tzhongdian;
//            qDebug()<<"anongduzhi"<<TwoCode.XiangMuPm[i].anongduzhi;
//            qDebug()<<"bnongduzhi"<<TwoCode.XiangMuPm[i].bnongduzhi;
//            qDebug()<<"cankaoxingshi"<<TwoCode.XiangMuPm[i].cankaoxingshi;
//            qDebug()<<"nongdudizhi"<<TwoCode.XiangMuPm[i].nongdudizhi;
//            qDebug()<<"nongdugaozhi"<<TwoCode.XiangMuPm[i].nongdugaozhi;
//            qDebug()<<"jisuanfangshi"<<TwoCode.XiangMuPm[i].jisuanfangshi;
//            qDebug()<<"jisuanfangfa"<<TwoCode.XiangMuPm[i].jisuanfangfa;
//            qDebug()<<"canshua"<<TwoCode.XiangMuPm[i].canshua;
//            qDebug()<<"canshub"<<TwoCode.XiangMuPm[i].canshub;
//            qDebug()<<"canshuc"<<TwoCode.XiangMuPm[i].canshuc;
//            qDebug()<<"canshud"<<TwoCode.XiangMuPm[i].canshud;
        }
    }
    return TwoCode;
}

bool APP_dev::agl_start()
{
    AppCfg->AdcStep=0;
    AppCfg->TimerP=new QTimer(this);
    AppCfg->TimerP->setInterval(100);
    connect(AppCfg->TimerP,SIGNAL(timeout()),this,SLOT(agl_work()));
    AppCfg->TimerP->start();
    return true;
}

void APP_dev::agl_work()
{
    if(AppCfg->AdcStep==0)
    {
        AppCfg->AdcStep=1;
        AppCfg->TimerP->stop();
        AppCfg->TimerP->setInterval(4000);
        AppCfg->TimerP->start();
        QByteArray Code1D;
        AppCfg->Code1D.clear();
        Scan1D_Code(Code1D);
        //--------------------Add----------------------//
        if(Code1D.length() > 6)
        {
            qDebug() << "the Code is too long";
            AppCfg->TimerP->stop();
            AppCfg->AdcStep=0;
            QString ComAck="STX,ER1,ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
            return;
        }
        AppCfg->Code1D = Code1D;
        //------------------------------------------------//
//        AppCfg->Code1D=Code1D.mid(0,4);
//        if(AppCfg->Code1D.length() != 4)
//        {
//            AppCfg->TimerP->stop();
//            AppCfg->AdcStep=0;
//            QString ComAck="STX,ER1,ETX\r\n";
//            pc_wr_data(ComAck.toLatin1());
//            return;
//        }
    }
    else if(AppCfg->AdcStep==1)
    {
        AppCfg->AdcStep=2;
        AppCfg->TimerP->stop();
        AppCfg->TimerP->setInterval(1500);
        AppCfg->TimerP->start();
        AppCfg->TwoDimCode=agl_get_two(AppCfg->Code1D);
        if(AppCfg->TwoDimCode.Head!= "YX-")
        {
            AppCfg->TimerP->stop();
            AppCfg->AdcStep=0;
            QString ComAck="STX,ER2,ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
            return;
        }
    }
    else if(AppCfg->AdcStep==2)
    {
        AppCfg->AdcStep=3;
        agl_adc(AppCfg->TwoDimCode.Xiangmushuliang.toInt());
        AglDev.AdcState=0;
    }
    else if(AppCfg->AdcStep==3)
    {
        agl_rd_stat();
        if(AglDev.AdcState==2)
        {
            AppCfg->AdcStep=4;
        }
    }
    else if(AppCfg->AdcStep==4)
    {
        agl_get();
        AppCfg->AdcStep=5;
    }
    else if(AppCfg->AdcStep==5)
    {
        AppCfg->TimerP->stop();
        AppCfg->TimerP->deleteLater();
        agl_calc_vol(AppCfg->TwoDimCode);
        if(AglDev.AdcCalc[0].TPeakVol>=0 && AglDev.AdcCalc[0].CPeakVol>=0)
        {
            AppCfg->AdcStep=6;
            QString ComAck="STX,ISOK,ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
            return ;
        }
        else
        {
            AppCfg->AdcStep=0;
            QString ComAck="STX,ER3,ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
            return ;
        }
    }
}

void APP_dev::agl_com()
{
    static QString RxdData;
    RxdData +=pc_rd_data();
    int Length=RxdData.length();
    if(RxdData.length()<6)
    {
        return;
    }
    if(Length>1024)
    {
        RxdData.clear();
        return;
    }
    QString End = RxdData.mid(Length-2,2);
    if(End!="\r\n")
    {
        return;
    }
    QString ComCmd = RxdData;
    QString ComAck;
    RxdData.clear();
    qDebug()<<"ComCmd="<<ComCmd;
    if(AppCfg->LoginPort==0)             //   为0时只能登录，将所有发送过来的信息看作是登录信息
    {
        //用户名，密码，登陆        
        //Login:Admin;Passwd:123;\r\n
        QString LoginTxt="Login:"+AppCfg->LoginName+";Password:"+AppCfg->LoginPasswd+";\r\n";
        qDebug()<<"LoginTxt="<<LoginTxt;
        if(ComCmd==LoginTxt)
        {
            AppCfg->LoginPort=1;        //  登录成功后，将状态置为1（表示当前是串口登连接状态）。
            ComAck="Welcome Login !\r\n";
            AppCfg->Timer=0;
            AppCfg->Timeout=new QTimer();
            AppCfg->Timeout->setInterval(6000);
            connect(AppCfg->Timeout, SIGNAL(timeout()),this, SLOT(CheckTcpTimeOut()));
            AppCfg->Timeout->start();
        }
        else
        {
            ComAck="Hello,Please Login First !\r\n";
        }
        pc_wr_data(ComAck.toLatin1());
        qDebug()<<"ComAck="<<ComAck;
        return;
    }
    else if(AppCfg->LoginPort==1)       // 为1时，可以通过串口进行命令指令发送
    {
        QString Head=ComCmd.mid(0,3);
        QString End=ComCmd.mid(Length-5,3);
        if(Head!="STX" || End!="ETX")//Check Head and End
        {
            ComAck="STX,ER0,ETX\r\n";//错误帧格式应答
            pc_wr_data(ComAck.toLatin1());
            qDebug()<<"ComAck="<<ComAck;
            return;
        }
        qDebug()<<"Cmd OK";
         AppCfg->Timer=0;
        if(ComCmd=="STX,LoginOff,ETX\r\n")//退出设备登陆
        {
            AppCfg->LoginPort=0;
            ComAck="Login Off\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,GETDEVNAME,ETX\r\n")//读取设备名称
        {
            ComAck="STX,"+AppCfg->DevName+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,GETDEVSN,ETX\r\n")//读取设备ID号
        {
            ComAck="STX,"+AppCfg->SN+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,GETSWVER,ETX\r\n")//读取设备软件版本
        {
            ComAck="STX,"+AppCfg->SoftVersion+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,GETHWVER,ETX\r\n")//读取设备硬件版本
        {
            ComAck="STX,"+AppCfg->HardVersion+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,GETTIME,ETX\r\n")//读取设备时间
        {
            ComAck="STX,"+GetDateRtcTime()+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,GETLOCALIP,ETX\r\n")//读取本机IP
        {
            ComAck="STX,"+AppCfg->LocalIp+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,GETLISIP,ETX\r\n")//读取服务器IP
        {
            ComAck="STX,"+AppCfg->LisIP+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,GETLISPORT,ETX\r\n")//读取服务器端口
        {
            ComAck="STX,"+AppCfg->LisPort+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,GETLISMARK,ETX\r\n")//读取LIS标识
        {
            ComAck="STX,"+AppCfg->LisMark+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,GETLAN,ETX\r\n")//读取设备语言标识
        {
            ComAck="STX,UTF8-English,ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,READBCD,ETX\r\n")//读取二维码
        {
            ComAck="STX,OK,ETX\r\n";
            qDebug()<<"ComAck="<<ComAck;
            pc_wr_data(ComAck.toLatin1());
            QByteArray Code;
            Scan2D_Code(Code);
            QString CodeTxt=Code;
            ComAck="STX,"+CodeTxt+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,READCCD,ETX\r\n")//读取一维码
        {
            ComAck="STX,OK,ETX\r\n";
            qDebug()<<"ComAck="<<ComAck;
            pc_wr_data(ComAck.toLatin1());
            QByteArray Code;
            Scan1D_Code(Code);
            QString CodeTxt=Code;
            ComAck="STX,"+CodeTxt+",ETX\r\n";
            pc_wr_data(ComAck.toLatin1());
        }
        else if(ComCmd=="STX,OPENTRAY,ETX\r\n")//设备出仓
        {
            if(AppCfg->AdcStep==0 || AppCfg->AdcStep==6)
            {
                ComAck="STX,OK,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
                agl_out();
            }
            else
            {
                ComAck="STX,DEVISBUSY,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
            }
        }
        else if(ComCmd=="STX,CLOSETRAY,ETX\r\n")//设备入仓
        {
            if(AppCfg->AdcStep==0 || AppCfg->AdcStep==6)
            {
                ComAck="STX,OK,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
                agl_int();
            }
            else
            {
                ComAck="STX,DEVISBUSY,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
            }
        }
        else if(ComCmd=="STX,START,ETX\r\n")//启动通道采集
        {
            if(AppCfg->AdcStep==0 || AppCfg->AdcStep==6)
            {
                ComAck="STX,OK,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
                agl_start();
            }
            else
            {
                ComAck="STX,DEVISBUSY,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
            }
        }
        //-----------Add--------------liuyouwei-----------------------------------------------------------//
        else if(ComCmd.split(",").length() == 4 && ComCmd.split(",").at(0) == "STX" && ComCmd.split(",").at(3) == "ETX\r\n")
        {
            if(isFoundPasscfg == false)
            {
                ComAck = "STX,ERROR,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
                return;
            }
            if(ComCmd.split(",").at(1) == "SETPARA")
            {
                if(AppCfg->AdcStep==0 || AppCfg->AdcStep==6)
                {
                    QString SetData = ComCmd.split(",").at(2);
                    QStringList SetData_list = SetData.split(";");
//                    if(SetData_list.length() != 7)
                    if(SetData_list.length() != 8)
                    {
                        ComAck = "STX,ERROR,ETX\r\n";
                        pc_wr_data(ComAck.toLatin1());
                        return;
                    }
//------Add------05-31------------------------------------------------------------//
                    if(!ReWriteFile("Standard_C1", SetData_list.at(0)))
                    {
                        qDebug() << "Rewrite C1 Error!";
                        return;
                    }
                    if(!ReWriteFile("Standard_T1", SetData_list.at(1)))
                    {
                        qDebug() << "Rewrite C1 Error!";
                        return;
                    }
                    if(!ReWriteFile("Standard_C2", SetData_list.at(2)))
                    {
                        qDebug() << "Rewrite C1 Error!";
                        return;
                    }
                    if(!ReWriteFile("Standard_T2", SetData_list.at(3)))
                    {
                        qDebug() << "Rewrite C1 Error!";
                        return;
                    }
                    if(!ReWriteFile("Standard_C3", SetData_list.at(4)))
                    {
                        qDebug() << "Rewrite C1 Error!";
                        return;
                    }
                    if(!ReWriteFile("Standard_T3", SetData_list.at(5)))
                    {
                        qDebug() << "Rewrite C1 Error!";
                        return;
                    }
                    if(!ReWriteFile("Standard_C4", SetData_list.at(6)))
                    {
                        qDebug() << "Rewrite C1 Error!";
                        return;
                    }
                    if(!ReWriteFile("Standard_T4", SetData_list.at(7)))
                    {
                        qDebug() << "Rewrite C1 Error!";
                        return;
                    }
//-------------------------------------------------------------------------------------//
//                    QStringList All_list = GetAll_Data();
//                    QString AllPassData;
//                    int iUserData = All_list.length();
//                    for(int i = 0; i < iUserData-1; i++)
//                    {
//                        AllPassData += All_list.at(i) + "#\r\n";
//                    }
//                    AllPassData += "@Languag=" + GetPasswd("@Languag") + "#\r\n";
//                    AllPassData += "@hostIP=" + GetPasswd("@hostIP") + "#\r\n";
//                    AllPassData += "@LisIP=" + GetPasswd("@LisIP") + "#\r\n";
//                    AllPassData += "@LisPort=" + GetPasswd("@LisPort") + "#\r\n";
//                    AllPassData += "@AUTO=" + GetPasswd("@AUTO") + "#\r\n";
//                    AllPassData += "@QRcode=" + GetPasswd("@QRcode") + "#\r\n";
//                    //----------------------------------------------Add----------------------------------------------//
//                    AllPassData += "@DX=" + GetPasswd("@DX") + "#\r\n";
//                    AllPassData += "@Location_C=" + GetPasswd("@Location_C") + "#\r\n";
//                    //-------------------------------------------------------------------------------------------------//
//                    AllPassData += "@YiDongJuLi1=" + GetPasswd("@YiDongJuLi1") + "#\r\n";
//                    AllPassData += "@YiDongJuLi2=" + GetPasswd("@YiDongJuLi2") + "#\r\n";
//                    AllPassData += "@YiDongJuLi3=" + GetPasswd("@YiDongJuLi3") + "#\r\n";
//                    AllPassData += "@YiDongJuLi4=" + GetPasswd("@YiDongJuLi4") + "#\r\n";

//                    if(SetData_list.at(0) == "1")
//                    {
//                        AllPassData += "@Chx_A1t=" + SetData_list.at(1) + "#\r\n";
//                        AllPassData += "@Chx_B1t=" + SetData_list.at(2) + "#\r\n";
//                        AllPassData += "@Chx_C1t=" + SetData_list.at(3) + "#\r\n";
//                        AllPassData += "@Chx_A1c=" + SetData_list.at(4) + "#\r\n";
//                        AllPassData += "@Chx_B1c=" + SetData_list.at(5) + "#\r\n";
//                        AllPassData += "@Chx_C1c=" + SetData_list.at(6) + "#\r\n";

//                        AllPassData += "@Chx_A2t=" + GetPasswd("@Chx_A2t") + "#\r\n";
//                        AllPassData += "@Chx_B2t=" + GetPasswd("@Chx_B2t") + "#\r\n";
//                        AllPassData += "@Chx_C2t=" + GetPasswd("@Chx_C2t") + "#\r\n";
//                        AllPassData += "@Chx_A2c=" + GetPasswd("@Chx_A2c") + "#\r\n";
//                        AllPassData += "@Chx_B2c=" + GetPasswd("@Chx_B2c") + "#\r\n";
//                        AllPassData += "@Chx_C2c=" + GetPasswd("@Chx_C2c") + "#\r\n";

//                        AllPassData += "@Chx_A3t=" + GetPasswd("@Chx_A3t") + "#\r\n";
//                        AllPassData += "@Chx_B3t=" + GetPasswd("@Chx_B3t") + "#\r\n";
//                        AllPassData += "@Chx_C3t=" + GetPasswd("@Chx_C3t") + "#\r\n";
//                        AllPassData += "@Chx_A3c=" + GetPasswd("@Chx_A3c") + "#\r\n";
//                        AllPassData += "@Chx_B3c=" + GetPasswd("@Chx_B3c") + "#\r\n";
//                        AllPassData += "@Chx_C3c=" + GetPasswd("@Chx_C3c") + "#\r\n";

//                        AllPassData += "@Chx_A4t=" + GetPasswd("@Chx_A4t") + "#\r\n";
//                        AllPassData += "@Chx_B4t=" + GetPasswd("@Chx_B4t") + "#\r\n";
//                        AllPassData += "@Chx_C4t=" + GetPasswd("@Chx_C4t") + "#\r\n";
//                        AllPassData += "@Chx_A4c=" + GetPasswd("@Chx_A4c") + "#\r\n";
//                        AllPassData += "@Chx_B4c=" + GetPasswd("@Chx_B4c") + "#\r\n";
//                        AllPassData += "@Chx_C4c=" + GetPasswd("@Chx_C4c") + "#\r\n";
//                    }
//                    else if(SetData_list.at(0) == "2")
//                    {
//                        AllPassData += "@Chx_A1t=" + GetPasswd("@Chx_A1t") + "#\r\n";
//                        AllPassData += "@Chx_B1t=" + GetPasswd("@Chx_B1t") + "#\r\n";
//                        AllPassData += "@Chx_C1t=" + GetPasswd("@Chx_C1t") + "#\r\n";
//                        AllPassData += "@Chx_A1c=" + GetPasswd("@Chx_A1c") + "#\r\n";
//                        AllPassData += "@Chx_B1c=" + GetPasswd("@Chx_B1c") + "#\r\n";
//                        AllPassData += "@Chx_C1c=" + GetPasswd("@Chx_C1c") + "#\r\n";

//                        AllPassData += "@Chx_A2t=" + SetData_list.at(1) + "#\r\n";
//                        AllPassData += "@Chx_B2t=" + SetData_list.at(2) + "#\r\n";
//                        AllPassData += "@Chx_C2t=" + SetData_list.at(3) + "#\r\n";
//                        AllPassData += "@Chx_A2c=" + SetData_list.at(4) + "#\r\n";
//                        AllPassData += "@Chx_B2c=" + SetData_list.at(5) + "#\r\n";
//                        AllPassData += "@Chx_C2c=" + SetData_list.at(6) + "#\r\n";

//                        AllPassData += "@Chx_A3t=" + GetPasswd("@Chx_A3t") + "#\r\n";
//                        AllPassData += "@Chx_B3t=" + GetPasswd("@Chx_B3t") + "#\r\n";
//                        AllPassData += "@Chx_C3t=" + GetPasswd("@Chx_C3t") + "#\r\n";
//                        AllPassData += "@Chx_A3c=" + GetPasswd("@Chx_A3c") + "#\r\n";
//                        AllPassData += "@Chx_B3c=" + GetPasswd("@Chx_B3c") + "#\r\n";
//                        AllPassData += "@Chx_C3c=" + GetPasswd("@Chx_C3c") + "#\r\n";

//                        AllPassData += "@Chx_A4t=" + GetPasswd("@Chx_A4t") + "#\r\n";
//                        AllPassData += "@Chx_B4t=" + GetPasswd("@Chx_B4t") + "#\r\n";
//                        AllPassData += "@Chx_C4t=" + GetPasswd("@Chx_C4t") + "#\r\n";
//                        AllPassData += "@Chx_A4c=" + GetPasswd("@Chx_A4c") + "#\r\n";
//                        AllPassData += "@Chx_B4c=" + GetPasswd("@Chx_B4c") + "#\r\n";
//                        AllPassData += "@Chx_C4c=" + GetPasswd("@Chx_C4c") + "#\r\n";
//                    }
//                    else if(SetData_list.at(0) == "3")
//                    {
//                        AllPassData += "@Chx_A1t=" + GetPasswd("@Chx_A1t") + "#\r\n";
//                        AllPassData += "@Chx_B1t=" + GetPasswd("@Chx_B1t") + "#\r\n";
//                        AllPassData += "@Chx_C1t=" + GetPasswd("@Chx_C1t") + "#\r\n";
//                        AllPassData += "@Chx_A1c=" + GetPasswd("@Chx_A1c") + "#\r\n";
//                        AllPassData += "@Chx_B1c=" + GetPasswd("@Chx_B1c") + "#\r\n";
//                        AllPassData += "@Chx_C1c=" + GetPasswd("@Chx_C1c") + "#\r\n";

//                        AllPassData += "@Chx_A2t=" + GetPasswd("@Chx_A2t") + "#\r\n";
//                        AllPassData += "@Chx_B2t=" + GetPasswd("@Chx_B2t") + "#\r\n";
//                        AllPassData += "@Chx_C2t=" + GetPasswd("@Chx_C2t") + "#\r\n";
//                        AllPassData += "@Chx_A2c=" + GetPasswd("@Chx_A2c") + "#\r\n";
//                        AllPassData += "@Chx_B2c=" + GetPasswd("@Chx_B2c") + "#\r\n";
//                        AllPassData += "@Chx_C2c=" + GetPasswd("@Chx_C2c") + "#\r\n";

//                        AllPassData += "@Chx_A3t=" + SetData_list.at(1) + "#\r\n";
//                        AllPassData += "@Chx_B3t=" + SetData_list.at(2) + "#\r\n";
//                        AllPassData += "@Chx_C3t=" + SetData_list.at(3) + "#\r\n";
//                        AllPassData += "@Chx_A3c=" + SetData_list.at(4) + "#\r\n";
//                        AllPassData += "@Chx_B3c=" + SetData_list.at(5) + "#\r\n";
//                        AllPassData += "@Chx_C3c=" + SetData_list.at(6) + "#\r\n";

//                        AllPassData += "@Chx_A4t=" + GetPasswd("@Chx_A4t") + "#\r\n";
//                        AllPassData += "@Chx_B4t=" + GetPasswd("@Chx_B4t") + "#\r\n";
//                        AllPassData += "@Chx_C4t=" + GetPasswd("@Chx_C4t") + "#\r\n";
//                        AllPassData += "@Chx_A4c=" + GetPasswd("@Chx_A4c") + "#\r\n";
//                        AllPassData += "@Chx_B4c=" + GetPasswd("@Chx_B4c") + "#\r\n";
//                        AllPassData += "@Chx_C4c=" + GetPasswd("@Chx_C4c") + "#\r\n";
//                    }
//                    else if(SetData_list.at(0) == "4")
//                    {
//                        AllPassData += "@Chx_A1t=" + GetPasswd("@Chx_A1t") + "#\r\n";
//                        AllPassData += "@Chx_B1t=" + GetPasswd("@Chx_B1t") + "#\r\n";
//                        AllPassData += "@Chx_C1t=" + GetPasswd("@Chx_C1t") + "#\r\n";
//                        AllPassData += "@Chx_A1c=" + GetPasswd("@Chx_A1c") + "#\r\n";
//                        AllPassData += "@Chx_B1c=" + GetPasswd("@Chx_B1c") + "#\r\n";
//                        AllPassData += "@Chx_C1c=" + GetPasswd("@Chx_C1c") + "#\r\n";

//                        AllPassData += "@Chx_A2t=" + GetPasswd("@Chx_A2t") + "#\r\n";
//                        AllPassData += "@Chx_B2t=" + GetPasswd("@Chx_B2t") + "#\r\n";
//                        AllPassData += "@Chx_C2t=" + GetPasswd("@Chx_C2t") + "#\r\n";
//                        AllPassData += "@Chx_A2c=" + GetPasswd("@Chx_A2c") + "#\r\n";
//                        AllPassData += "@Chx_B2c=" + GetPasswd("@Chx_B2c") + "#\r\n";
//                        AllPassData += "@Chx_C2c=" + GetPasswd("@Chx_C2c") + "#\r\n";

//                        AllPassData += "@Chx_A3t=" + GetPasswd("@Chx_A3t") + "#\r\n";
//                        AllPassData += "@Chx_B3t=" + GetPasswd("@Chx_B3t") + "#\r\n";
//                        AllPassData += "@Chx_C3t=" + GetPasswd("@Chx_C3t") + "#\r\n";
//                        AllPassData += "@Chx_A3c=" + GetPasswd("@Chx_A3c") + "#\r\n";
//                        AllPassData += "@Chx_B3c=" + GetPasswd("@Chx_B3c") + "#\r\n";
//                        AllPassData += "@Chx_C3c=" + GetPasswd("@Chx_C3c") + "#\r\n";

//                        AllPassData += "@Chx_A4t=" + SetData_list.at(1) + "#\r\n";
//                        AllPassData += "@Chx_B4t=" + SetData_list.at(2) + "#\r\n";
//                        AllPassData += "@Chx_C4t=" + SetData_list.at(3) + "#\r\n";
//                        AllPassData += "@Chx_A4c=" + SetData_list.at(4) + "#\r\n";
//                        AllPassData += "@Chx_B4c=" + SetData_list.at(5) + "#\r\n";
//                        AllPassData += "@Chx_C4c=" + SetData_list.at(6) + "#\r\n";
//                    }
//                    AllPassData += "@TC_Flag=" + GetPasswd("@TC_Flag") + "#\r\n";

//                    bool Dele =  DeleteFile("Passwor.txt");
//                    bool Writ =  WriteTxtToFile("password.txt", AllPassData);
//                    if(Dele == false || Writ == false)
//                    {
//                        ComAck = "STX,ERROR,ETX\r\n";
//                        pc_wr_data(ComAck.toLatin1());
//                        return;
//                    }
                    ComAck = "STX,OK,ETX\r\n";
                    pc_wr_data(ComAck.toLatin1());
                }
                else
                {
                    ComAck="STX,DEVISBUSY,ETX\r\n";
                    pc_wr_data(ComAck.toLatin1());
                }
            }
            else if(ComCmd.split(",").at(1) == "SETTC")
            {
                if(AppCfg->AdcStep==0 || AppCfg->AdcStep==6)
                {
//                    QStringList All_list = GetAll_Data();
//                    QString AllPassData;
//                    int iUserData = All_list.length();
//                    for(int i = 0; i < iUserData-1; i++)
//                    {
//                        AllPassData += All_list.at(i) + "#\r\n";
//                    }
//                    AllPassData += "@Languag=" + GetPasswd("@Languag") + "#\r\n";
//                    AllPassData += "@hostIP=" + GetPasswd("@hostIP") + "#\r\n";
//                    AllPassData += "@LisIP=" + GetPasswd("@LisIP") + "#\r\n";
//                    AllPassData += "@LisPort=" + GetPasswd("@LisPort") + "#\r\n";
//                    AllPassData += "@AUTO=" + GetPasswd("@AUTO") + "#\r\n";
//                    AllPassData += "@QRcode=" + GetPasswd("@QRcode") + "#\r\n";
//                    //----------------------------------------------Add----------------------------------------------//
//                    AllPassData += "@DX=" + GetPasswd("@DX") + "#\r\n";
//                    AllPassData += "@Location_C=" + GetPasswd("@Location_C") + "#\r\n";
//                    //-------------------------------------------------------------------------------------------------//

//                    AllPassData += "@YiDongJuLi1=" + GetPasswd("@YiDongJuLi1") + "#\r\n";
//                    AllPassData += "@YiDongJuLi2=" + GetPasswd("@YiDongJuLi2") + "#\r\n";
//                    AllPassData += "@YiDongJuLi3=" + GetPasswd("@YiDongJuLi3") + "#\r\n";
//                    AllPassData += "@YiDongJuLi4=" + GetPasswd("@YiDongJuLi4") + "#\r\n";

//                    AllPassData += "@Chx_A1t=" + GetPasswd("@Chx_A1t") + "#\r\n";
//                    AllPassData += "@Chx_B1t=" + GetPasswd("@Chx_B1t") + "#\r\n";
//                    AllPassData += "@Chx_C1t=" + GetPasswd("@Chx_C1t") + "#\r\n";
//                    AllPassData += "@Chx_A1c=" + GetPasswd("@Chx_A1c") + "#\r\n";
//                    AllPassData += "@Chx_B1c=" + GetPasswd("@Chx_B1c") + "#\r\n";
//                    AllPassData += "@Chx_C1c=" + GetPasswd("@Chx_C1c") + "#\r\n";

//                    AllPassData += "@Chx_A2t=" + GetPasswd("@Chx_A2t") + "#\r\n";
//                    AllPassData += "@Chx_B2t=" + GetPasswd("@Chx_B2t") + "#\r\n";
//                    AllPassData += "@Chx_C2t=" + GetPasswd("@Chx_C2t") + "#\r\n";
//                    AllPassData += "@Chx_A2c=" + GetPasswd("@Chx_A2c") + "#\r\n";
//                    AllPassData += "@Chx_B2c=" + GetPasswd("@Chx_B2c") + "#\r\n";
//                    AllPassData += "@Chx_C2c=" + GetPasswd("@Chx_C2c") + "#\r\n";

//                    AllPassData += "@Chx_A3t=" + GetPasswd("@Chx_A3t") + "#\r\n";
//                    AllPassData += "@Chx_B3t=" + GetPasswd("@Chx_B3t") + "#\r\n";
//                    AllPassData += "@Chx_C3t=" + GetPasswd("@Chx_C3t") + "#\r\n";
//                    AllPassData += "@Chx_A3c=" + GetPasswd("@Chx_A3c") + "#\r\n";
//                    AllPassData += "@Chx_B3c=" + GetPasswd("@Chx_B3c") + "#\r\n";
//                    AllPassData += "@Chx_C3c=" + GetPasswd("@Chx_C3c") + "#\r\n";

//                    AllPassData += "@Chx_A4t=" + GetPasswd("@Chx_A4t") + "#\r\n";
//                    AllPassData += "@Chx_B4t=" + GetPasswd("@Chx_B4t") + "#\r\n";
//                    AllPassData += "@Chx_C4t=" + GetPasswd("@Chx_C4t") + "#\r\n";
//                    AllPassData += "@Chx_A4c=" + GetPasswd("@Chx_A4c") + "#\r\n";
//                    AllPassData += "@Chx_B4c=" + GetPasswd("@Chx_B4c") + "#\r\n";
//                    AllPassData += "@Chx_C4c=" + GetPasswd("@Chx_C4c") + "#\r\n";
                    QString TC_Res = ComCmd.split(",").at(2);
                    if(!ReWriteFile("TC_Flag", TC_Res))
                    {
                        ComAck = "STX,ERROR,ETX\r\n";
                        pc_wr_data(ComAck.toLatin1());
                        return;
                    }
//                    AllPassData += "@TC_Flag=" + ComCmd.split(",").at(2) + "#\r\n";
//                    bool Dele =  DeleteFile("Passwor.txt");
//                    bool Writ =  WriteTxtToFile("password.txt", AllPassData);
//                    if(Dele == false || Writ == false)
//                    {
//                        ComAck = "STX,ERROR,ETX\r\n";
//                        pc_wr_data(ComAck.toLatin1());
//                        return;
//                    }
                    else
                    {
                        ComAck = "STX,OK,ETX\r\n";
                        pc_wr_data(ComAck.toLatin1());
                    }
                }
                else
                {
                    ComAck="STX,DEVISBUSY,ETX\r\n";
                    pc_wr_data(ComAck.toLatin1());
                }
            }
            else if(ComCmd.split(",").at(1) == "SETMOVE")
            {
                if(AppCfg->AdcStep==0 || AppCfg->AdcStep==6)
                {
                    QString SetData = ComCmd.split(",").at(2);
                    QStringList SetData_list = SetData.split(";");
                    if(SetData_list.length() != 4)
                    {
                        ComAck = "STX,ERROR,ETX\r\n";
                        pc_wr_data(ComAck.toLatin1());
                        return;
                    }
//                    QStringList All_list = GetAll_Data();
//                    QString AllPassData;
//                    int iUserData = All_list.length();
//                    for(int i = 0; i < iUserData-1; i++)
//                    {
//                        AllPassData += All_list.at(i) + "#\r\n";
//                    }

//                    AllPassData += "@Languag=" + GetPasswd("@Languag") + "#\r\n";
//                    AllPassData += "@hostIP=" + GetPasswd("@hostIP") + "#\r\n";
//                    AllPassData += "@LisIP=" + GetPasswd("@LisIP") + "#\r\n";
//                    AllPassData += "@LisPort=" + GetPasswd("@LisPort") + "#\r\n";
//                    AllPassData += "@AUTO=" + GetPasswd("@AUTO") + "#\r\n";
//                    AllPassData += "@QRcode=" + GetPasswd("@QRcode") + "#\r\n";
//                    //----------------------------------------------Add----------------------------------------------//
//                    AllPassData += "@DX=" + GetPasswd("@DX") + "#\r\n";
//                    AllPassData += "@Location_C=" + GetPasswd("@Location_C") + "#\r\n";
//                    //-------------------------------------------------------------------------------------------------//

//                    AllPassData += "@YiDongJuLi1=" + SetData_list.at(0) + "#\r\n";
//                    AllPassData += "@YiDongJuLi2=" + SetData_list.at(1) + "#\r\n";
//                    AllPassData += "@YiDongJuLi3=" + SetData_list.at(2) + "#\r\n";
//                    AllPassData += "@YiDongJuLi4=" + SetData_list.at(3) + "#\r\n";

//                    AllPassData += "@Chx_A1t=" + GetPasswd("@Chx_A1t") + "#\r\n";
//                    AllPassData += "@Chx_B1t=" + GetPasswd("@Chx_B1t") + "#\r\n";
//                    AllPassData += "@Chx_C1t=" + GetPasswd("@Chx_C1t") + "#\r\n";
//                    AllPassData += "@Chx_A1c=" + GetPasswd("@Chx_A1c") + "#\r\n";
//                    AllPassData += "@Chx_B1c=" + GetPasswd("@Chx_B1c") + "#\r\n";
//                    AllPassData += "@Chx_C1c=" + GetPasswd("@Chx_C1c") + "#\r\n";

//                    AllPassData += "@Chx_A2t=" + GetPasswd("@Chx_A2t") + "#\r\n";
//                    AllPassData += "@Chx_B2t=" + GetPasswd("@Chx_B2t") + "#\r\n";
//                    AllPassData += "@Chx_C2t=" + GetPasswd("@Chx_C2t") + "#\r\n";
//                    AllPassData += "@Chx_A2c=" + GetPasswd("@Chx_A2c") + "#\r\n";
//                    AllPassData += "@Chx_B2c=" + GetPasswd("@Chx_B2c") + "#\r\n";
//                    AllPassData += "@Chx_C2c=" + GetPasswd("@Chx_C2c") + "#\r\n";

//                    AllPassData += "@Chx_A3t=" + GetPasswd("@Chx_A3t") + "#\r\n";
//                    AllPassData += "@Chx_B3t=" + GetPasswd("@Chx_B3t") + "#\r\n";
//                    AllPassData += "@Chx_C3t=" + GetPasswd("@Chx_C3t") + "#\r\n";
//                    AllPassData += "@Chx_A3c=" + GetPasswd("@Chx_A3c") + "#\r\n";
//                    AllPassData += "@Chx_B3c=" + GetPasswd("@Chx_B3c") + "#\r\n";
//                    AllPassData += "@Chx_C3c=" + GetPasswd("@Chx_C3c") + "#\r\n";

//                    AllPassData += "@Chx_A4t=" + GetPasswd("@Chx_A4t") + "#\r\n";
//                    AllPassData += "@Chx_B4t=" + GetPasswd("@Chx_B4t") + "#\r\n";
//                    AllPassData += "@Chx_C4t=" + GetPasswd("@Chx_C4t") + "#\r\n";
//                    AllPassData += "@Chx_A4c=" + GetPasswd("@Chx_A4c") + "#\r\n";
//                    AllPassData += "@Chx_B4c=" + GetPasswd("@Chx_B4c") + "#\r\n";
//                    AllPassData += "@Chx_C4c=" + GetPasswd("@Chx_C4c") + "#\r\n";

//                    AllPassData += "@TC_Flag=" + GetPasswd("@TC_Flag") + "#\r\n";

//                    bool Dele =  DeleteFile("Passwor.txt");
//                    bool Writ =  WriteTxtToFile("password.txt", AllPassData);
//                    if(Dele == false || Writ == false)
//                    {
//                        ComAck = "STX,ERROR,ETX\r\n";
//                        pc_wr_data(ComAck.toLatin1());
//                        return;
//                    }
                    if(!ReWriteFile("YiDongJuLi1", SetData_list.at(0)))
                    {
                        ComAck = "STX,ERROR,ETX\r\n";
                        pc_wr_data(ComAck.toLatin1());
                        return;
                    }
                    if(!ReWriteFile("YiDongJuLi2", SetData_list.at(1)))
                    {
                        ComAck = "STX,ERROR,ETX\r\n";
                        pc_wr_data(ComAck.toLatin1());
                        return;
                    }
                    if(!ReWriteFile("YiDongJuLi3", SetData_list.at(2)))
                    {
                        ComAck = "STX,ERROR,ETX\r\n";
                        pc_wr_data(ComAck.toLatin1());
                        return;
                    }
                    if(!ReWriteFile("YiDongJuLi4", SetData_list.at(3)))
                    {
                        ComAck = "STX,ERROR,ETX\r\n";
                        pc_wr_data(ComAck.toLatin1());
                        return;
                    }
                    ComAck = "STX,OK,ETX\r\n";
                    pc_wr_data(ComAck.toLatin1());
                }
                else
                {
                    ComAck="STX,DEVISBUSY,ETX\r\n";
                    pc_wr_data(ComAck.toLatin1());
                }
            }
        }
        else if(ComCmd == "STX,GETPARA,ETX\r\n" )
        {
            qDebug() << "####### it's Get Para ";
            if(isFoundPasscfg == false)
            {
                ComAck = "STX,ERROR,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
                return;
            }
            if(AppCfg->AdcStep==0 || AppCfg->AdcStep==6)
            {
                QString GetData;
//                GetData = "YiDongJuLi1=" + GetPasswd("@YiDongJuLi1") + ";";
//                GetData += "YiDongJuLi2=" + GetPasswd("@YiDongJuLi2") + ";";
//                GetData += "YiDongJuLi3=" + GetPasswd("@YiDongJuLi3") + ";";
//                GetData += "YiDongJuLi4=" + GetPasswd("@YiDongJuLi4") + ";";

                GetData += "C1=" + GetPasswd("@Standard_C1") + ";";
                GetData += "T1=" + GetPasswd("@Standard_T1") + ";";
                GetData += "C2=" + GetPasswd("@Standard_C2") + ";";
                GetData += "T2=" + GetPasswd("@Standard_T2") + ";";
                GetData += "C3=" + GetPasswd("@Standard_C3") + ";";
                GetData += "T3=" + GetPasswd("@Standard_T3") + ";";
                GetData += "C4=" + GetPasswd("@Standard_C4") + ";";
                GetData += "T4=" + GetPasswd("@Standard_T4") + ";";
//                GetData += "Standard_C5=" + GetPasswd("@Standard_C5") + ";";
//                GetData += "Standard_T5=" + GetPasswd("@Standard_T5") + ";";
//                GetData += "Standard_C6=" + GetPasswd("@Standard_C6") + ";";
//                GetData += "Standard_T6=" + GetPasswd("@Standard_T6") + ";";
//                GetData += "Standard_C7=" + GetPasswd("@Standard_C7") + ";";
//                GetData += "Standard_T7=" + GetPasswd("@Standard_T7") + ";";
//                GetData += "Standard_C8=" + GetPasswd("@Standard_C8") + ";";
//                GetData += "Standard_T8=" + GetPasswd("@Standard_T8") + ";";

//                GetData += "Chx_A1t=" + GetPasswd("@Chx_A1t") + ";";
//                GetData += "Chx_B1t=" + GetPasswd("@Chx_B1t") + ";";
//                GetData += "Chx_C1t=" + GetPasswd("@Chx_C1t") + ";";
//                GetData += "Chx_A1c=" + GetPasswd("@Chx_A1c") + ";";
//                GetData += "Chx_B1c=" + GetPasswd("@Chx_B1c") + ";";
//                GetData += "Chx_C1c=" + GetPasswd("@Chx_C1c") + ";";

//                GetData += "Chx_A2t=" + GetPasswd("@Chx_A2t") + ";";
//                GetData += "Chx_B2t=" + GetPasswd("@Chx_B2t") + ";";
//                GetData += "Chx_C2t=" + GetPasswd("@Chx_C2t") + ";";
//                GetData += "Chx_A2c=" + GetPasswd("@Chx_A2c") + ";";
//                GetData += "Chx_B2c=" + GetPasswd("@Chx_B2c") + ";";
//                GetData += "Chx_C2c=" + GetPasswd("@Chx_C2c") + ";";

//                GetData += "Chx_A3t=" + GetPasswd("@Chx_A3t") + ";";
//                GetData += "Chx_B3t=" + GetPasswd("@Chx_B3t") + ";";
//                GetData += "Chx_C3t=" + GetPasswd("@Chx_C3t") + ";";
//                GetData += "Chx_A3c=" + GetPasswd("@Chx_A3c") + ";";
//                GetData += "Chx_B3c=" + GetPasswd("@Chx_B3c") + ";";
//                GetData += "Chx_C3c=" + GetPasswd("@Chx_C3c") + ";";

//                GetData += "Chx_A4t=" + GetPasswd("@Chx_A4t") + ";";
//                GetData += "Chx_B4t=" + GetPasswd("@Chx_B4t") + ";";
//                GetData += "Chx_C4t=" + GetPasswd("@Chx_C4t") + ";";
//                GetData += "Chx_A4c=" + GetPasswd("@Chx_A4c") + ";";
//                GetData += "Chx_B4c=" + GetPasswd("@Chx_B4c") + ";";
//                GetData += "Chx_C4c=" + GetPasswd("@Chx_C4c") + ";";
//                GetData += "TC_Flag=" + GetPasswd("@TC_Flag");

                ComAck = "STX," + GetData + ",ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
            }
            else
            {
                ComAck = "STX,DEVISBUSY,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
            }
        }
        //--------------------------------------------------------------------------------------------------//
        else if(ComCmd=="STX,GETRESUALTSLINE,ETX\r\n")//读取曲线。
        {
            if(AppCfg->AdcStep==0)
            {
                ComAck="STX,ISNULL,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
            }
            else if(AppCfg->AdcStep==6)
            {
                ComAck="STX,OK,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
                Delayms(10);
                int Temp;
                //"STX,CH4,2000*4,ETX\r\n"
                QString Data="STX,CH"+AppCfg->TwoDimCode.Xiangmushuliang+",";
                for(int index=0;index<AppCfg->TwoDimCode.Xiangmushuliang.toInt();index++)
                {
                    for(int indey=0;indey<2000;indey++)
                    {
                        Temp=(int)AglDev.VolBuf[index][indey];
                        Data += (QString::number(Temp,10)+",");//曲线值
                    }
                 }
                Data +="ETX\r\n";
                //qDebug()<<"ComRet="<<Data;
                qDebug()<<"Data length="<<Data.length();
                pc_wr_data(Data.toLatin1());
            }
            else
            {
                ComAck="STX,DEVISBUSY,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
            }
        }
        else if(ComCmd=="STX,GETRESUALTS,ETX\r\n")//读取T线，C线数值，浓度值。
        {
            if(AppCfg->AdcStep==0)
            {
                 ComAck="STX,ISNULL,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
            }
            else if(AppCfg->AdcStep==6)
            {
                ComAck="STX,OK,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
                Delayms(100);
                //"STX,T1=1.11,C1=2.22,S=3.33,ETX\r\n"
                QString Txt="STX,";
                for(int index=0;index<AppCfg->TwoDimCode.Xiangmushuliang.toInt();index++)
                {
                    Txt += ("T"+QString::number(index+1,10)+"="+QString::number(AglDev.AdcCalc[index].TPeakVol,'g',10)+",");//T值
                    Txt += ("C"+QString::number(index+1,10)+"="+QString::number(AglDev.AdcCalc[index].CPeakVol,'g',10)+",");//C值
                    Txt += ("S"+QString::number(index+1,10)+"="+QString::number(AglDev.AdcCalc[index].YSumVol ,'g',10)+",");//荧光结果浓度
                 }
                Txt +="ETX\r\n";
                pc_wr_data(Txt.toLatin1());
                qDebug()<<"ComRet="<<Txt;
            }
            else
            {
                ComAck="STX,DEVISBUSY,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
            }
        }
        else// Get SampleID and ER Cmd
        {
            QString Cmd=GetKey(1,ComCmd);
            //“STX,SAMPLEID,A123456,ETX\r\n”
            if(Cmd=="SAMPLEID")//接收样本ID
            {
                AppCfg->SampleID=GetKey(2,ComCmd);
                ComAck="STX,OK,ETX\r\n";
                pc_wr_data(ComAck.toLatin1());
                qDebug()<<"SampleID="<<AppCfg->SampleID;
            }
            else
            {
                ComAck="STX,ER0,ETX\r\n";//错误帧格式应答
                pc_wr_data(ComAck.toLatin1());
            }            
        }
        qDebug()<<"ComAck="<<ComAck;
        return;
    }
    else if(AppCfg->LoginPort==2)       // 为2时，表示正在通过TCP连接，可以通过TCP发送命令指令
    {
        //用户名，密码，登陆
        //Login:Admin;Password:123;\r\n
        QString LoginTxt="Login:"+AppCfg->LoginName+";Password:"+AppCfg->LoginPasswd+";\r\n";
        qDebug()<<"LoginTxt="<<LoginTxt;
        if(ComCmd==LoginTxt)
        {
            ComAck="Dev is Busy2 !\r\n";//错误帧格式应答
            pc_wr_data(ComAck.toLatin1());
            qDebug()<<"ComAck="<<ComAck;
        }
        return;
    }
    else if(AppCfg->LoginPort==3)       // 为3时，表示当前仪器没有在主界面，不允许登录
    {
        //用户名，密码，登陆
        //Login:Admin;Password:123;\r\n
        QString LoginTxt="Login:"+AppCfg->LoginName+";Password:"+AppCfg->LoginPasswd+";\r\n";
        qDebug()<<"LoginTxt="<<LoginTxt;
        if(ComCmd==LoginTxt)
        {
            ComAck="Dev is Busy3 !\r\n";//错误帧格式应答
            pc_wr_data(ComAck.toLatin1());
            qDebug()<<"ComAck="<<ComAck;
        }
        return;
    }
}

bool APP_dev::agl_calc_vol(TwoCodeStr TwoDimCode)
{
    if(AppSta!=AGL_DEV)
    {
        return false;
    }
    for(int i = 0; i < 4; i++)
    {
        Card_index[i] = false;
    }

    TlineClinePosStr TcPos;
    iPeakTimes = 0;
    //T值和C值计算
    bool ok;
    for(int index=0;index<AglDev.ChxVol;index++)
    {        
//        TlineClinePosStr TcPos;
//        TcPos.Tstart=TwoDimCode.XiangMuPm[index].tqidian.toFloat(&ok) /*+YiDongJuLi*/;
//         if(TcPos.Tstart<0 && TcPos.Tstart>1800) { TcPos.Tstart=700; }

//         TcPos.Tend=TwoDimCode.XiangMuPm[index].tzhongdian.toFloat(&ok)/*+YiDongJuLi*/;
//        if(TcPos.Tend<0 && TcPos.Tend>1800) { TcPos.Tend=1100; }

//        TcPos.Cstart=TwoDimCode.XiangMuPm[index].cqidian.toFloat(&ok)/*+YiDongJuLi*/;
//        if(TcPos.Cstart<0 && TcPos.Cstart>1800) { TcPos.Cstart=1100; }

//        TcPos.Cend=TwoDimCode.XiangMuPm[index].czhongdian.toFloat(&ok)/*+YiDongJuLi*/;
//        if(TcPos.Cend<0 && TcPos.Cend>1800) { TcPos.Cend=1500; }
//        qDebug("T%d,Tstart=%d",index,TcPos.Tstart);
//        qDebug("T%d,Tend=%d",index,TcPos.Tend);
//        qDebug("C%d,Cstart=%d",index,TcPos.Cstart);
//        qDebug("C%d,Cend=%d",index,TcPos.Cend);

        agl_calc_buf(TcPos,&AglDev.ChxBuf[index][0],AdcMaxCycle,AglDev.AdcCalc[index]);
        iPeakTimes++;
        if(isCalibration == true)
        {
            qDebug() << "C线位置校准完毕";
             isCalibration = false;
             isTestContrl = false;
            return true;
        }
    }
    //HuGuo=A*YiXin*YiXin+B*YiXin+C
    if(isTestContrl == true)        //  校准
    {
        double Res_C1 = 0;
        double Res_T1 = 0;
        double Res_C2 = 0;
        double Res_T2 = 0;
        double Res_C3 = 0;
        double Res_T3 = 0;
        double Res_C4 = 0;
        double Res_T4 = 0;

        double C1_hu_D = 0;
        double T1_hu_D = 0;
        double C2_hu_D = 0;
        double T2_hu_D = 0;
        double C3_hu_D = 0;
        double T3_hu_D = 0;
        double C4_hu_D = 0;
        double T4_hu_D = 0;
        qDebug() << "试剂卡数值校准";
        QString C1_huguo = GetPasswd("@Standard_C1");
        QString T1_huguo = GetPasswd("@Standard_T1");
        C1_hu_D = C1_huguo.toDouble(&ok);
        T1_hu_D = T1_huguo.toDouble(&ok);
        Res_C1 = AglDev.AdcCalc[0].CPeakVol;
        Res_T1 = AglDev.AdcCalc[0].TPeakVol;
        double K1 = Res_C1 / C1_hu_D;
        double B1 = 0;

        QString K1_str = QString::number(K1,'g',6);
        QString B1_str = QString::number(B1,'g',6);

        double K2 = (Res_T1 - Res_C1) / (T1_hu_D - C1_hu_D);
        double B2 = Res_C1 - C1_hu_D * K2;

        QString K2_str = QString::number(K2,'g',6);
        QString B2_str = QString::number(B2,'g',6);

        if(!ReWriteFile("K1", K1_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("B1", B1_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("K2", K2_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("B2", B2_str))
        {
            qDebug() << "rewite error";
        }
       //---------------------------------------------------------------------------------------//
        QString C2_huguo = GetPasswd("@Standard_C2");
        QString T2_huguo = GetPasswd("@Standard_T2");
        C2_hu_D = C2_huguo.toDouble(&ok);
        T2_hu_D = T2_huguo.toDouble(&ok);
        Res_C2 = AglDev.AdcCalc[1].CPeakVol;
        Res_T2 = AglDev.AdcCalc[1].TPeakVol;
        double K3 = (Res_C2 - Res_T1) / (C2_hu_D - T1_hu_D);
        double B3 = Res_T1 - T1_hu_D * K3;

        QString K3_str = QString::number(K3,'g',6);
        QString B3_str = QString::number(B3,'g',6);

        double K4 = (Res_T2 - Res_C2) / (T2_hu_D - C2_hu_D);
        double B4 = Res_C2 - C2_hu_D * K4;

        QString K4_str = QString::number(K4,'g',6);
        QString B4_str = QString::number(B4,'g',6);
        if(!ReWriteFile("K3", K3_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("B3", B3_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("K4", K4_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("B4", B4_str))
        {
            qDebug() << "rewite error";
        }
        //-----------------------------------------------------------------//
        QString C3_huguo = GetPasswd("@Standard_C3");
        QString T3_huguo = GetPasswd("@Standard_T3");
        C3_hu_D = C3_huguo.toDouble(&ok);
        T3_hu_D = T3_huguo.toDouble(&ok);
        Res_C3 = AglDev.AdcCalc[2].CPeakVol;
        Res_T3 = AglDev.AdcCalc[2].TPeakVol;
        double K5 = (Res_C3 - Res_T2) / (C3_hu_D - T2_hu_D);
        double B5 = Res_T2 - T2_hu_D * K5;

        QString K5_str = QString::number(K5,'g',6);
        QString B5_str = QString::number(B5,'g',6);

        double K6 = (Res_T3 - Res_C3) / (T3_hu_D - C3_hu_D);
        double B6 = Res_C3 - C3_hu_D * K6;

        QString K6_str = QString::number(K6,'g',6);
        QString B6_str = QString::number(B6,'g',6);
        if(!ReWriteFile("K5", K5_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("B5", B5_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("K6", K6_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("B6", B6_str))
        {
            qDebug() << "rewite error";
        }
        //--------------------------------------------------------------------------------//
        QString C4_huguo = GetPasswd("@Standard_C4");
        QString T4_huguo = GetPasswd("@Standard_T4");
        C4_hu_D = C4_huguo.toDouble(&ok);
        T4_hu_D = T4_huguo.toDouble(&ok);
        Res_C4 = AglDev.AdcCalc[3].CPeakVol;
        Res_T4 = AglDev.AdcCalc[3].TPeakVol;
        double K7 = (Res_C4 - Res_T3) / (C4_hu_D - T3_hu_D);
        double B7 = Res_T3 - T3_hu_D * K7;

        QString K7_str = QString::number(K7,'g',6);
        QString B7_str = QString::number(B7,'g',6);

        double K8 = (Res_T4 - Res_C4) / (T4_hu_D - C4_hu_D);
        double B8 = Res_C4 - C4_hu_D * K8;

        QString K8_str = QString::number(K8,'g',6);
        QString B8_str = QString::number(B8,'g',6);

        if(!ReWriteFile("K7", K7_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("B7", B7_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("K8", K8_str))
        {
            qDebug() << "rewite error";
        }
        if(!ReWriteFile("B8", B8_str))
        {
            qDebug() << "rewite error";
        }
        isTestContrl = false;
    }
    else                                        // 一般测试
    {
        double K_res[8] = {0};
        double B_res[8] = {0};
        for(int i = 0; i < 8; i++)
        {
            K_res[i] = GetPasswd(tr("@K%0").arg(i+1)).toDouble(&ok);
            B_res[i] = GetPasswd(tr("@B%0").arg(i+1)).toDouble(&ok);
        }
 //--------------------------------------------------------Add-------liuyouwei------------------------------//
           double Std_res[8] = {0};
           Std_res[0] = GetPasswd("@Standard_C1").toDouble(&ok);
           Std_res[1] = GetPasswd("@Standard_T1").toDouble(&ok);
           Std_res[2] = GetPasswd("@Standard_C2").toDouble(&ok);
           Std_res[3] = GetPasswd("@Standard_T2").toDouble(&ok);
           Std_res[4] = GetPasswd("@Standard_C3").toDouble(&ok);
           Std_res[5] = GetPasswd("@Standard_T3").toDouble(&ok);
           Std_res[6] = GetPasswd("@Standard_C4").toDouble(&ok);
           Std_res[7] = GetPasswd("@Standard_T4").toDouble(&ok);
           for(int iTc = 0; iTc < (int)AglDev.ChxVol; iTc++)
           {
               double KT_res = 0;
               double BT_res = 0;
               double KC_res = 0;
               double BC_res = 0;

               double Xt = AglDev.AdcCalc[iTc].TPeakVol;
               double Xc = AglDev.AdcCalc[iTc].CPeakVol;

               double Different_T = 0;
               double Max_diff_T = 1000000;
               double Different_C = 0;
               double Max_diff_C = 1000000;

               for(int i = 0; i < 8; i ++)
               {
                   //--------------------------------------------//  T值K，B判断
                   Different_T = Xt - Std_res[i];
                   if(Different_T < 0)
                   {
                       Different_T = Different_T * (-1);
                   }
                   if(Max_diff_T > Different_T)
                   {
                       Max_diff_T = Different_T;
                       KT_res = K_res[i];
                       BT_res = B_res[i];
                   }
                   //--------------------------------------------//  C值K，B判断
                   Different_C = Xc - Std_res[i];
                   if(Different_C < 0)
                   {
                       Different_C = Different_C * (-1);
                   }
                   if(Max_diff_C > Different_C)
                   {
                       Max_diff_C = Different_C;
                       KC_res = K_res[i];
                       BC_res = B_res[i];
                   }
               }
               double Yt = (Xt - BT_res) / KT_res;
               double Yc = (Xc - BC_res) / KC_res;
               if(Yt<0)
               {
                   Yt=0;
               }
               if(Yc<0)
               {
                   Yc=0;
               }
               AglDev.AdcCalc[iTc].TPeakVol = Yt;
               AglDev.AdcCalc[iTc].CPeakVol = Yc;
               qDebug("T%d=%6.2f, C%d=%6.2f",iTc,AglDev.AdcCalc[iTc].TPeakVol,iTc,AglDev.AdcCalc[iTc].CPeakVol);
           }
//------------------------------------------------------------------------------------------------------------------//
    }
    //浓度计算
    AglDev.AdcCalc[0].YSumVol=0.001;
    AglDev.AdcCalc[1].YSumVol=0.001;
    AglDev.AdcCalc[2].YSumVol=0.001;
    AglDev.AdcCalc[3].YSumVol=0.001;
    for(int index=0;index<TwoDimCode.Xiangmushuliang.toInt();index++)
    {
        double X=0;
        double Y=0;

        double A=TwoDimCode.XiangMuPm[index].canshua.toDouble();
        double B=TwoDimCode.XiangMuPm[index].canshub.toDouble();
        double C=TwoDimCode.XiangMuPm[index].canshuc.toDouble();
        double D=TwoDimCode.XiangMuPm[index].canshud.toDouble();

        double Show_Limit = TwoDimCode.XiangMuPm[index].nongdudizhi.toDouble();
        double Show_Max = TwoDimCode.XiangMuPm[index].nongdugaozhi.toDouble();
        double Limit_Equation = TwoDimCode.XiangMuPm[index].Equation_Limit.toDouble();
        double Max_Equation = TwoDimCode.XiangMuPm[index].Equation_Max.toDouble();

        if(Limit_Equation > Max_Equation)
        {
            Y = 0;
            qDebug() << "Equation Limit and Max Error ! " << index;
            return false;
        }

        if(TwoDimCode.XiangMuPm[index].jisuanfangshi=="T")
        {
            X=AglDev.AdcCalc[index].TPeakVol;
        }
        else if(TwoDimCode.XiangMuPm[index].jisuanfangshi=="T/C")
        {
            X=AglDev.AdcCalc[index].TPeakVol/AglDev.AdcCalc[index].CPeakVol;
        }
        else
        {
            qDebug("CH%d 2Dcode  T or T/C is error !",index);
            qDebug() << "Error=" << TwoDimCode.XiangMuPm[index].jisuanfangshi;
        }

        if(X < Limit_Equation)
        {
            Y = 0;
        }
        else if(X > Max_Equation)
        {
            Y = 20000;
        }
        else
        {
            if(TwoDimCode.XiangMuPm[index].jisuanfangfa=="直线方程"
                    || TwoDimCode.XiangMuPm[index].jisuanfangfa=="直线")
            {
    //            Y=A*X+B;
                if(A > 0)
                {
                    Y = (X - B) / A;
                }
               else
                {
                    Y = 0;
                }
            }
            else if(TwoDimCode.XiangMuPm[index].jisuanfangfa=="二次方程"
                    || TwoDimCode.XiangMuPm[index].jisuanfangfa=="二次")
            {
                Y=A*X*X+B*X+C;
            }
            else if(TwoDimCode.XiangMuPm[index].jisuanfangfa=="三次方程"
                    || TwoDimCode.XiangMuPm[index].jisuanfangfa=="三次")
            {
                Y=A*X*X*X+B*X*X+C*X+D;
            }
            else if(TwoDimCode.XiangMuPm[index].jisuanfangfa=="四参数方程"
                    || TwoDimCode.XiangMuPm[index].jisuanfangfa=="四参数")
            {
                if(C == 0)
                {
                    qDebug("CH%d 2Dcode  C of pm is error !",index);
                }
                else
                {
    //                Y=(A-D)/(1+qPow(X/C,B))+D;
                    if(X == D || B == 0)
                    {
                        Y = 0;
                    }
                    else
                    {
                        double b = 1 / B;
    //                    Y = qPow(((A-D) / (X-D) - 1), (b)) * C;
                        Y = qPow((A-X)/(X-D),b) * C;
                    }
                }
            }
            else
            {
               qDebug("CH%d 2Dcode  funcation is error !",index);
            }
        }
        //浓度
        if(Y < 0 || 1 == isnan(Y))
        {
            Y = 0;
        }
        AglDev.AdcCalc[index].YSumVol = Y;
        qDebug("============= NongDu %d = %6.2f" ,index,Y);
    }
    return true;
}

bool APP_dev::agl_calc_buf(TlineClinePosStr TcPos,double *bufP, int Length, AdcCalcStr &CalcVol)
{
    if(AppSta!=AGL_DEV)
    {
        return false;
    }

    if(Length < 1800)
    {
        qDebug()<<"the adc data length is < 1800!";
        return false;
    }
    //-----------------------------------Add---liuyouwei--------------------------//
    if(isCalibration == true)       // 如果是C位置校准
    {
        qDebug() << "C位置校准";
        double cMinVol=1000000;
        int PeakCPos=1100;
        double PeakCVol=0;
        int tMinPos=1100;
        double tMinVol=1000000;
        int PeakTPos=900;
        double PeakTVol=0;
//        double Rank_Peak[1799] = {0};
//        for(int i = 0 ; i < 1799; i++)
//        {
//            Rank_Peak[i] = bufP[i+1] - bufP[i];
//        }
        double Samll_Peak[180] = {0};
        int iDiff = 0;
        int iSum = 0;
        int iDex = 0;
        for(int i = 0; i < 180; i++)                            //   将曲线原值进行滤波处理
        {
            iDiff = 1800 - iDex;
            if(iDiff > 10)
            {
                iDiff = 10;
            }
            iSum = 0;
            for(int j = i * 10; j < i * 10 + iDiff; j++)
            {
                iSum += bufP[j];
                Samll_Peak[i] =   iSum / iDiff;
                iDex++;
            }
        }
        double Samll_Rank[179] = {0};
        for(int i = 60; i < 170; i++)                            //   得到滤波之后曲线的倒数曲线
        {
            Samll_Rank[i] = Samll_Peak[i+1] - Samll_Peak[i];
        }
        QString Rank_str;
        int c_Left = 0;
        int c_Right = 0;
        int C_Lefttimes = 0;
        int C_Righttimes = 0;
        bool isCpeakOK = false;
        bool ok = false;

        for(int i = 170; i > 10; i--)                                            // 在滤波之后的曲线的倒数曲线寻找峰值点
        {
            C_Lefttimes = 0;
            C_Righttimes = 0;
            c_Left = i - 5;
            c_Right = i + 5;
            for(int j = c_Left; j < i; j++)
            {
                if(Samll_Rank[j] > 0)
                {
                    C_Lefttimes++;
                }
                else if(Samll_Rank[j] < 0)
                {
                    break;
                }
            }
            if(C_Lefttimes > 1)
            {
                for(int k = i; k < c_Right; k++)
                {
                    if(Samll_Rank[k] < 0)
                    {
                        C_Righttimes++;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if(C_Righttimes > 1)
            {
                if(isCpeakOK == false)
                {
                    isCpeakOK = true;
                    Rank_str += ";";
                }
                Rank_str += QString::number(i) + ",";
            }
            else
            {
                isCpeakOK = false;
            }
        }
        qDebug() << "We found Rank_str " << Rank_str;

        int TC_DX =  GetPasswd("@DX").toInt(&ok);
        if(ok == false)
        {
            return false;
        }
        int Limit_C = GetPasswd("@LeftC").toInt(&ok);
        if(ok == false)
        {
            return false;
        }
        int Max_C = GetPasswd("@RightC").toInt(&ok);
        if(ok == false)
        {
            return false;
        }

        QStringList CPeak_times = Rank_str.split(";");
        int Peak_Lengh = CPeak_times.length() - 1;              // 得到找到的符合范围的峰值点范围的次数
        if(0 == Peak_Lengh)                                                     // 找到0个峰
        {
            qDebug() << "We have no found" ;
            tMinPos = 0;
            tMinVol = 0;
            PeakCPos = 0;
            PeakCVol = 0;
            cMinVol = tMinVol = 0;
        }
        else if(1 == Peak_Lengh)                                             // 找到1个峰
        {
            QString PeakData = CPeak_times.at(1);
            QStringList CPeak_List = PeakData.split(",");
            Peak_Lengh = CPeak_List.length() - 1;
            FoundPeakVol C_Peak[Peak_Lengh];
            memset(C_Peak, 0, sizeof(C_Peak));
            for(int i = 0; i <  Peak_Lengh; i++)
            {
                C_Peak[i].ReadData = bufP[CPeak_List.at(i).toInt() * 10];
                C_Peak[i].Location = CPeak_List.at(i).toInt() * 10;
            }
            Rank_Data(C_Peak, Peak_Lengh);
            int LeftC = C_Peak[0].Location - 20;
            int RightC = C_Peak[0].Location + 20;
            PeakCVol = 0;
            cMinVol =1000000;
            int  C_Location = 0;
            double C_max = 0;
            for(int i = LeftC; i <= RightC; i++)                                 // 在找到的峰的区间找到最大值和位置，即为真正的C峰与位置
            {
                if(bufP[i] >= PeakCVol)
                {
                    C_Location = i;
                    PeakCVol = bufP[C_Location];
                }
                C_max = PeakCVol;
            }
            if(Limit_C <= C_Location && C_Location <= Max_C)       // 如果位置符合C线位置
            {
                PeakCPos = C_Location;
                PeakCVol = C_max;
                int LeftT = C_Location- TC_DX - 90;
                int RightT = C_Location - TC_DX + 90;
                PeakTVol = 0;
                tMinVol = 1000000;
                for(int i = LeftT; i <= RightT; i++)
                {
                    if(bufP[i] >= PeakTVol)
                    {
                        PeakTPos = i;
                        PeakTVol = bufP[PeakTPos];
                    }
                }
                LeftT = PeakTPos;
                RightT = PeakCPos;
                for(int i = LeftT; i <= RightT; i++)
                {
                    if(bufP[i] <= tMinVol)
                    {
                        tMinPos = i;
                        tMinVol = bufP[tMinPos];
                    }
                }
                cMinVol = tMinVol;
            }
            else                                                                               // 如果位置不符合C线位置
            {
                qDebug() << "The Card is Error";
                return false;
            }
        }
        else                                                                              // 找到多个峰
        {
            qDebug() << "We have found times " <<  Peak_Lengh;
            QString CPeakstr;
            QString TPeakstr;
            double MaxData = 0;
            int MaxLocation = 0;
            for(int index = 0; index < Peak_Lengh; index++)
            {
                QString PeakC = CPeak_times.at(index+1);
                QStringList CPeak_List = PeakC.split(",");
                int Lengh = CPeak_List.length() - 1;
                FoundPeakVol C_Peak[Lengh];
                memset(C_Peak, 0, sizeof(C_Peak));
                for(int i = 0; i <  Lengh; i++)
                {
                    C_Peak[i].ReadData = bufP[CPeak_List.at(i).toInt() * 10];
                    C_Peak[i].Location = CPeak_List.at(i).toInt() * 10;
                }
                Rank_Data(C_Peak, Lengh);
                int LeftC = C_Peak[0].Location - 20;
                int RightC = C_Peak[0].Location + 20;
                PeakCVol = 0;
                cMinVol =1000000;
                int  C_Location = 0;
                for(int i = LeftC; i <= RightC; i++)
                {
                    if(bufP[i] >= PeakCVol)
                    {
                        C_Location = i;
                        PeakCVol = bufP[C_Location];
                    }
                }
                MaxData = PeakCVol;
                MaxLocation = C_Location;
                if(C_Peak[0].Location >= 1000)
                {
                    CPeakstr += QString::number(MaxLocation) + ",";
                }
                else
                {
                    TPeakstr += QString::number(MaxLocation) + ",";
                }
            }
            QStringList ListPeakC = CPeakstr.split(",");
            int CPeaktime = ListPeakC.length() - 1;
            QStringList ListPeakT = TPeakstr.split(",");
            int TPeaktime = ListPeakT.length() - 1;
            FoundPeakVol CPeakFound[CPeaktime];
            FoundPeakVol TPeakFound[TPeaktime];
            memset(CPeakFound, 0, sizeof(CPeakFound));
            memset(TPeakFound, 0, sizeof(TPeakFound));
            for(int i = 0; i < CPeaktime; i++)
            {
                CPeakFound[i].Location = ListPeakC.at(i).toInt();
                CPeakFound[i].ReadData = bufP[ListPeakC.at(i).toInt()];
            }
            for(int i = 0; i < TPeaktime; i++)
            {
                TPeakFound[i].Location = ListPeakT.at(i).toInt();
                TPeakFound[i].ReadData = bufP[ListPeakT.at(i).toInt()];
            }
            Rank_Data(CPeakFound, CPeaktime);
            Rank_Data(TPeakFound, TPeaktime);
            PeakCPos = CPeakFound[0].Location;
            PeakTPos = TPeakFound[0].Location;
            qDebug() << "------C Location have " << PeakCPos;
            qDebug() << "------T Location have " << PeakTPos;
            qDebug() << "C Data have " << CPeakFound[0].ReadData;
            qDebug() << "T Data have " << TPeakFound[0].ReadData;
//------------------------------Add-----liuyouwei---------------------------------------------------------//
            int TCDx = PeakCPos - PeakTPos;
            QString TC_Dx = QString::number(TCDx);
            if(!ReWriteFile("DX", TC_Dx))
            {
                qDebug() << "rewite error";
            }
            int CLocation = GetPasswd("@C_Location").toInt(&ok);
            if(ok == false)
            {
                return false;
            }

            int Cdistance = CLocation - PeakCPos;
            QString CDis_str = QString::number(Cdistance);
            if(!ReWriteFile("C_Distance", CDis_str))
            {
                qDebug() << "3518 rewite error";
            }
//--------------------------------------------------------------------------------------------------------------//
        }
    }
    else
    {
        qDebug() << "非C线校准，读TC值";
        bool ok = false;
        //--------------------------------------------------new algorithm--------------------------//
        double cMinVol=1000000;
        int PeakCPos=1100;
        double PeakCVol=0;
        int tMinPos=1100;
        double tMinVol=1000000;
        int PeakTPos=900;
        double PeakTVol=0;
//        double Rank_Peak[1799] = {0};
//----------------------------Add----liuyouwei----------------------------------//
        int DX_c = GetPasswd("@C_Distance").toInt(&ok);
        if(DX_c < 0)
        {
            for(int i = 0; i < 1800+DX_c; i++)
            {
                bufP[i] = bufP[i-DX_c];
            }
            for(int i = 1800+DX_c; i < 1800; i++)
            {
                bufP[i] = bufP[1799+DX_c];
            }
        }
        else if(DX_c > 0)
        {
            for(int i = 1799; i > DX_c; i--)
            {
                bufP[i] = bufP[i-DX_c];
            }
            for(int i = DX_c; i > 0; i--)
            {
                bufP[i] = bufP[DX_c];
            }
        }
//-------------------------------------------------------------------------------------//
//        for(int i = 0 ; i < 1799; i++)
//        {
//            Rank_Peak[i] = bufP[i+1] - bufP[i];
//        }
        double Samll_Peak[180] = {0};
        int iDiff = 0;
        int iSum = 0;
        int iDex = 0;
        for(int i = 0; i < 180; i++)                            //   将曲线原值进行滤波处理
        {
            iDiff = 1800 - iDex;
            if(iDiff > 10)
            {
                iDiff = 10;
            }
            iSum = 0;
            for(int j = i * 10; j < i * 10 + iDiff; j++)
            {
                iSum += bufP[j];
                Samll_Peak[i] =   iSum / iDiff;
                iDex++;
            }
        }
        double Samll_Rank[179] = {0};
        for(int i = 0; i < 179; i++)                            //   得到滤波之后曲线的倒数曲线
        {
            Samll_Rank[i] = Samll_Peak[i+1] - Samll_Peak[i];
        }
        QString Rank_str;
        int c_Left = 0;
        int c_Right = 0;
        int C_Lefttimes = 0;
        int C_Righttimes = 0;
        bool isCpeakOK = false;
//        bool ok = false;
        for(int i = 168; i > 100; i--)                                            // 在滤波之后的曲线的倒数曲线寻找峰值点
        {
            C_Lefttimes = 0;
            C_Righttimes = 0;
            c_Left = i - 5;
            c_Right = i + 5;
            for(int j = c_Left; j < i; j++)
            {
                if(Samll_Rank[j] > 0)
                {
                    C_Lefttimes++;
                }
                else if(Samll_Rank[j] < 0)
                {
                    break;
                }
            }
            if(C_Lefttimes > 1)
            {
                for(int k = i; k < c_Right; k++)
                {
                    if(Samll_Rank[k] < 0)
                    {
                        C_Righttimes++;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if(C_Righttimes > 1)
            {
                if(isCpeakOK == false)
                {
                    isCpeakOK = true;
                    Rank_str += ";";
                }
                Rank_str += QString::number(i) + ",";
            }
            else
            {
                isCpeakOK = false;
            }
        }
        qDebug() << "We found Rank_str " << Rank_str;

        int TC_DX =  GetPasswd("@DX").toInt(&ok);
        if(ok == false)
        {
            return false;
        }
        int Limit_C = GetPasswd("@LeftC").toInt(&ok);
        if(ok == false)
        {
            return false;
        }
        int Max_C = GetPasswd("@RightC").toInt(&ok);
        if(ok == false)
        {
            return false;
        }
        QStringList CPeak_times = Rank_str.split(";");
        int Peak_Lengh = CPeak_times.length() - 1;              // 得到找到的符合范围的峰值点范围的次数
        if(0 == Peak_Lengh)                                                     // 找到0个峰
        {
            qDebug() << "We have no found" ;
            tMinPos = 0;
            tMinVol = 0;
            PeakCPos = 0;
            PeakCVol = 0;
            cMinVol = tMinVol = 0;
        }
        else if(1 == Peak_Lengh)                                             // 找到1个峰
        {
            QString PeakData = CPeak_times.at(1);
            QStringList CPeak_List = PeakData.split(",");
            Peak_Lengh = CPeak_List.length() - 1;
            FoundPeakVol C_Peak[Peak_Lengh];
            memset(C_Peak, 0, sizeof(C_Peak));
            for(int i = 0; i <  Peak_Lengh; i++)
            {
                C_Peak[i].ReadData = bufP[CPeak_List.at(i).toInt() * 10];
                C_Peak[i].Location = CPeak_List.at(i).toInt() * 10;
            }
            Rank_Data(C_Peak, Peak_Lengh);
            int LeftC = C_Peak[0].Location - 20;
            int RightC = C_Peak[0].Location + 20;
            PeakCVol = 0;
            cMinVol =1000000;
            int  C_Location = 0;
            double C_max = 0;
            for(int i = LeftC; i <= RightC; i++)                                 // 在找到的峰的区间找到最大值和位置，即为真正的C峰与位置
            {
                if(bufP[i] >= PeakCVol)
                {
                    C_Location = i;
                    PeakCVol = bufP[C_Location];
                }
                C_max = PeakCVol;
            }
            if(Limit_C <= C_Location && C_Location <= Max_C)       // 如果位置符合C线位置
            {
                PeakCPos = C_Location;
                PeakCVol = C_max;
                int LeftT = C_Location- TC_DX - 90;
                int RightT = C_Location - TC_DX + 90;
                PeakTVol = 0;
                tMinVol = 1000000;
                for(int i = LeftT; i <= RightT; i++)
                {
                    if(bufP[i] >= PeakTVol)
                    {
                        PeakTPos = i;
                        PeakTVol = bufP[PeakTPos];
                    }
                }
                LeftT = PeakTPos;
                RightT = PeakCPos;
                for(int i = LeftT; i <= RightT; i++)
                {
                    if(bufP[i] <= tMinVol)
                    {
                        tMinPos = i;
                        tMinVol = bufP[tMinPos];
                    }
                }
                cMinVol = tMinVol;
            }
            else                                                                               // 如果位置不符合C线位置
            {
                Card_index[iPeakTimes] = true;
            }
        }
        else                                                                              // 找到多个峰
        {
            qDebug() << "We have found times " <<  Peak_Lengh;
            FoundPeakVol Max_Peak[Peak_Lengh];
            memset(Max_Peak, 0, sizeof(Max_Peak));
            for(int index = 0; index < Peak_Lengh; index++)
            {
                QString PeakC = CPeak_times.at(index+1);
                QStringList CPeak_List = PeakC.split(",");
                int Lengh = CPeak_List.length() - 1;
                FoundPeakVol C_Peak[Lengh];
                memset(C_Peak, 0, sizeof(C_Peak));
                for(int i = 0; i <  Lengh; i++)
                {
                    C_Peak[i].ReadData = bufP[CPeak_List.at(i).toInt() * 10];
                    C_Peak[i].Location = CPeak_List.at(i).toInt() * 10;
                }
                Rank_Data(C_Peak, Lengh);
                int LeftC = C_Peak[0].Location - 20;
                int RightC = C_Peak[0].Location + 20;
                PeakCVol = 0;
                cMinVol =1000000;
                int  C_Location = 0;
                double C_max = 0;
                for(int i = LeftC; i <= RightC; i++)                                 // 在找到的峰的区间找到最大值和位置，即为真正的C峰与位置
                {
                    if(bufP[i] >= PeakCVol)
                    {
                        C_Location = i;
                        PeakCVol = bufP[C_Location];
                    }
                }
                C_max = PeakCVol;
                Max_Peak[index].Location = C_Location;
                Max_Peak[index].ReadData = C_max;
            }
            Rank_Data(Max_Peak, Peak_Lengh);
            PeakCPos = Max_Peak[0].Location;
            PeakCVol = Max_Peak[0].ReadData;
            qDebug() << "======We have found the max Peak Location is " << Max_Peak[0].Location;
            qDebug() << "======We have found the max Peak Data is " << Max_Peak[0].ReadData;

            qDebug() << "=================== C_Location is " << PeakCPos;
            qDebug() << "=================== Limit_C is " << Limit_C;
            qDebug() << "=================== Max_C is " << Max_C;

            if(Limit_C <= PeakCPos && PeakCPos <= Max_C)
            {
                int LeftT = PeakCPos - TC_DX - 90;
                int RightT = PeakCPos - TC_DX + 90;
                PeakTVol = 0;
                tMinVol = 1000000;
                for(int i = LeftT; i <= RightT; i++)
                {
                    if(bufP[i] >= PeakTVol)
                    {
                        PeakTPos = i;
                        PeakTVol = bufP[PeakTPos];
                    }
                }
                LeftT = PeakTPos;
                RightT = PeakCPos;
                for(int i = LeftT; i <= RightT; i++)
                {
                    if(bufP[i] <= tMinVol)
                    {
                        tMinPos = i;
                        tMinVol = bufP[tMinPos];
                    }
                }
                cMinVol = tMinVol;
            }
            else
            {
                qDebug() << "--------------have Error";
                Card_index[iPeakTimes] = true;
            }
        }

        //---------------------------------------------------------------------------------------------//
        CalcVol.TPeakPos = PeakTPos;                                 //  T线在最高值处的X轴位置
        CalcVol.CPeakPos = PeakCPos;                                //  C线在最高值处的X轴位置
        CalcVol.TPeakVol = PeakTVol-tMinVol;                    // T最终结果等于读值的最高值减去读值的最低值
        CalcVol.CPeakVol = PeakCVol-cMinVol;                   // C最终结果等于读值的最高值减去读值的最低值

        qDebug("TPos=%d, TMinVol=%f, TVol=%f" ,PeakTPos,tMinVol,CalcVol.TPeakVol);
        qDebug("CPos=%d, CMinVol=%f, CVol=%f",PeakCPos,cMinVol,CalcVol.CPeakVol);
        qDebug() << "C-T =  " << PeakCPos - PeakTPos;
    }
    return true;
}
//------------------------------------------------------Add-------liu--------------------------------------------------------------------//
//std::string APP_dev::agl_calc_string(double PG1,double PG2,double HPG,double G17, int post)
//---------------------------------------------------------new interpretation---------------------------------------------------------------//
std::string APP_dev::GetInterpapreTation(double PG1, double PG2, double G17, double HPG, int post)
{
////--------------------------------------------Add-11-16------------------------------------------//
//    QDateTime time = QDateTime :: currentDateTime();
//    QString now = time.toString("yyyy-MM-dd hh:mm:ss");
//    QString ZZ = QTime::currentTime().toString("zzz");
//    QString compele = now+ "." + ZZ + "     ";
////    Gas_Show = compele + Gas_Show;

//    QString PG1_Str = QString::number(PG1);
//    QString PG2_Str = QString::number(PG2);
//    QString G17_Str = QString::number(G17);
//    QString Hp_Str = QString::number(HPG);
//    QString Post_Str = QString::number(post) + "\r\n";

//    QString EnterInter = "Enetr GetInterparetation function : PG1 = ";
//    EnterInter = compele + EnterInter;
//    QString Interp = EnterInter + PG1_Str;
//    EnterInter = ", PG2 = ";
//    Interp += EnterInter + PG2_Str;
//    EnterInter = ", G17 = ";
//    Interp += EnterInter + G17_Str;
//    EnterInter = ", Hp = ";
//    Interp += EnterInter + Hp_Str;
//    EnterInter = ", Post = ";
//    Interp += EnterInter + Post_Str;
//    Gas_Show += Interp;
////----------------------------------------------------------------------------------------------------//
    bool ok = false;
    double PG1_LOW = GetConfig("PG1_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PG1_LOW paraterm Error !";
    }
    double PG1_HIGH = GetConfig("PG1_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PG1_HIGH paraterm Error !";
    }
    double PG2_LOW = GetConfig("PG2_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PG2_LOW paraterm Error !";
    }
    double PG2_HIGH = GetConfig("PG2_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PG2_HIGH paraterm Error !";
    }
    double G17B_LOW = GetConfig("G17B_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation G17B_LOW paraterm Error !";
    }
    double G17B_HIGH = GetConfig("G17B_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation G17B_HIGH paraterm Error !";
    }
    double G17S_LOW = GetConfig("G17S_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation G17S_LOW paraterm Error !";
    }
    double G17S_HIGH = GetConfig("G17S_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation G17S_HIGH paraterm Error !";
    }
    double HPG_LOW = GetConfig("HPG_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation HPG_LOW paraterm Error !";
    }
    double HPG_HIGH = GetConfig("HPG_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation HPG_HIGH paraterm Error !";
    }
    double PGR_LOW = GetConfig("PGR_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PGR_LOW paraterm Error !";
    }
    double PGR_HIGH = GetConfig("PGR_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PGR_HIGH paraterm Error !";
    }
   double PG1_MED = GetConfig("PG1_MED").toDouble(&ok);
   if(ok == false)
   {
       return "Get config interpretation PG1_MED paraterm Error !";
   }
   double PGR_MED = GetConfig("PGR_MED").toDouble(&ok);
   if(ok == false)
   {
       return "Get config interpretation PGR_MED paraterm Error !";
   }
   double G17B_REF2 = GetConfig("G17B_REF2").toDouble(&ok);
   if(ok == false)
   {
       return "Get config interpretation G17B_REF2 paraterm Error !";
   }
   double G17B_REF3 = GetConfig("G17B_REF3").toDouble(&ok);
   if(ok == false)
   {
       return "Get config interpretation G17B_REF3 paraterm Error !";
   }
   double G17B_REF4 = GetConfig("G17B_REF4").toDouble(&ok);
   if(ok == false)
   {
       return "Get config interpretation G17B_REF4 paraterm Error !";
   }

    // Initialize zInterpretationngs
    std::string zInterpretation_A = "";
    std::string zInterpretation_B = "";
    std::string zInterpretation_C = "";
    std::string zConsultation = "";

    // Initialize parameters
    double PGR = -1.0;
    double G17_LOW = -1.0;
    double G17_HIGH = -1.0;

    // Marker values must exist; e.g 'invalid' not processed
    if (G17<0 || PG1<0 || PG2<0 || HPG<0)
    {
//        //--------------------------------Add-11-16---------------------------------//
//        QString ReTstr = "Inetr function return to \" if (G17<0 || PG1<0 || PG2<0 || HPG<0)\", return is Empty, Interparetation Loss!\r\n";
//        Gas_Show += ReTstr;
//        //-------------------------------------------------------------------------------//
        return "";
    }

    // Pepsonogen I/Pepsinogen II; division by 0 not allowed
    if (PG2 > 0.0)
    {
        PGR = PG1/PG2;
    }
    else
    {
        // PGII invalid
//        //--------------------------------Add-11-16---------------------------------//
//        QString ReTstr = "Inetr function return to \" PG2 <= 0(PG2 invalid)\", return is Empty, Interparetation Loss!\r\n";
//        Gas_Show += ReTstr;
//        //-------------------------------------------------------------------------------//
        return "";
    }

    // Select reference based on patient preparation (sample type)
    if (post > 0) // Stimulated sample, if postprandial selected
    {
        G17_LOW = G17S_LOW;
        G17_HIGH = G17S_HIGH;
    }
    else		   // Basal sample, if fasting selected
    {
        G17_LOW = G17B_LOW;
        G17_HIGH = G17B_HIGH;
    }

    //
    // Atrophic gastritis
    //
    if (PG1 < PG1_LOW || (PGR < PGR_LOW && PG1 < PG1_MED))
    {
        if (HPG >= HPG_HIGH)
        {
            if (G17 < G17_LOW)
            {
                zInterpretation_A.assign(GetConfig("TXT_P_POS").toStdString());
//                zInterpretation_A.assign(TXT_P_POS);
            }
            else
            {
                zInterpretation_A.assign(GetConfig("TXT_C_POS").toStdString());
//                zInterpretation_A.assign(TXT_C_POS);
            }
        }
        else
        {
            if (G17 < G17_LOW)
            {
//                zInterpretation_A.assign(TXT_C_NEG_LOW_G17);
                zInterpretation_A.assign(GetConfig("TXT_C_NEG_LOW_G17").toStdString());
            }
            else
            {
                zInterpretation_A.assign(GetConfig("TXT_C_NEG").toStdString());
//                zInterpretation_A.assign(TXT_C_NEG);
            }
        }

        // Acid
        if (post < 1 && G17 > G17B_REF4)
        {
            zInterpretation_B.assign(GetConfig("TXT_VHIGH_G17B").toStdString());
//            zInterpretation_B.assign(TXT_VHIGH_G17B);
        }
        else if (post < 1 && G17 > G17B_REF2)
        {
            zInterpretation_B.assign(GetConfig("TXT_HIGH_G17B").toStdString());
//            zInterpretation_B.assign(TXT_HIGH_G17B);
        }

        // Consultation
        zConsultation.assign(GetConfig("TXT_C2").toStdString());
//        zConsultation.assign(TXT_C2);
    }

    //
    // H.pylori
    //
    else if (HPG >= HPG_HIGH)
    {
        zInterpretation_A.assign(GetConfig("TXT_S").toStdString());
        zConsultation.assign(GetConfig("TXT_C3").toStdString());
//        zInterpretation_A.assign(TXT_S);
//        zConsultation.assign(TXT_C3);

        if (G17 < G17_LOW)
        {
            zInterpretation_B.assign(GetConfig("TXT_AGC_OK").toStdString());
//            zInterpretation_B.assign(TXT_AGC_OK);
        }
        else
        {
            zInterpretation_B.assign(GetConfig("TXT_AG_OK").toStdString());
//            zInterpretation_B.assign(TXT_AG_OK);
        }

        if (post > 0 && G17 < G17S_LOW)
        {
            zInterpretation_C.assign(GetConfig("TXT_A_POS_LOW_G17S").toStdString());
            zConsultation.assign(GetConfig("TXT_C2").toStdString());
//            zInterpretation_C.assign(TXT_A_POS_LOW_G17S);
//            zConsultation.assign(TXT_C2);
        }
        else if (post < 1 && G17 < G17B_LOW)
        {
            if (PG1 > PG1_MED && PGR > PGR_MED)
            {
                zInterpretation_C.assign(GetConfig("TXT_A_POS_ULCER").toStdString());
//                zInterpretation_C.assign(TXT_A_POS_ULCER);
            }
            else
            {
                zInterpretation_C.assign(GetConfig("TXT_A_POS").toStdString());
//                zInterpretation_C.assign(TXT_A_POS);
            }
            zConsultation.assign(GetConfig("TXT_C1").toStdString());
//            zConsultation.assign(TXT_C1);
        }
    }
    //
    // Normal
    //
    else
    {
        zInterpretation_A.assign(GetConfig("TXT_N").toStdString());
//        zInterpretation_A.assign(TXT_N);
        if (PG1 > PG1_HIGH || PG2 > PG2_HIGH || (post < 1 && G17 > G17B_HIGH) )
        {
            zInterpretation_B.assign(GetConfig("TXT_N_PPI_UNKNOWN").toStdString());
//            zInterpretation_B.assign(TXT_N_PPI_UNKNOWN);
        }

        if (G17 < G17_LOW)
        {
            zInterpretation_C.assign(GetConfig("TXT_N_LOW_G17").toStdString());
//            zInterpretation_C.assign(TXT_N_LOW_G17);
        }
        else if (post<1 && G17 < G17B_REF2)
        {
            zInterpretation_C.assign(GetConfig("TXT_NORMAL_G17B").toStdString());
//            zInterpretation_C.assign(TXT_NORMAL_G17B);
        }
        else if (post<1 && G17 < G17B_REF3)
        {
            zInterpretation_C.assign(GetConfig("TXT_ELEV_G17B").toStdString());
//            zInterpretation_C.assign(TXT_ELEV_G17B);
        }
        else if (post<1 && G17 < G17B_REF4)
        {
            zInterpretation_C.assign(GetConfig("TXT_HIGH_G17B").toStdString());
//            zInterpretation_C.assign(TXT_HIGH_G17B);
        }
        else if (post<1 && G17 >= G17B_REF4)
        {
            zInterpretation_C.assign(GetConfig("TXT_VHIGH_G17B").toStdString());
//            zInterpretation_C.assign(TXT_VHIGH_G17B);
        }
    }
//    //--------------------------------Add-11-16---------------------------------//
//    QString ReTstr = "Inetr function return to end, return is " ;
//    ReTstr += QString::fromStdString(zInterpretation_A) + QString::fromStdString(zInterpretation_B) + QString::fromStdString(zInterpretation_C) + QString::fromStdString(zConsultation);
//    Gas_Show += ReTstr;
//    //-------------------------------------------------------------------------------//
    return zInterpretation_A + zInterpretation_B + zInterpretation_C + zConsultation;
}
//---------------------------------------------------------new interpretation---------------------------------------------------------------//
QString APP_dev::GetCsvInterpretation(double PG1, double PG2, double HPG, double G17, int post)
{
    bool ok = false;
    double PG1_LOW = GetConfig("PG1_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PG1_LOW paraterm Error !";
    }
    double PG1_HIGH = GetConfig("PG1_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PG1_HIGH paraterm Error !";
    }
    double PG2_LOW = GetConfig("PG2_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PG2_LOW paraterm Error !";
    }
    double PG2_HIGH = GetConfig("PG2_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PG2_HIGH paraterm Error !";
    }
    double G17B_LOW = GetConfig("G17B_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation G17B_LOW paraterm Error !";
    }
    double G17B_HIGH = GetConfig("G17B_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation G17B_HIGH paraterm Error !";
    }
    double G17S_LOW = GetConfig("G17S_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation G17S_LOW paraterm Error !";
    }
    double G17S_HIGH = GetConfig("G17S_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation G17S_HIGH paraterm Error !";
    }
    double HPG_LOW = GetConfig("HPG_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation HPG_LOW paraterm Error !";
    }
    double HPG_HIGH = GetConfig("HPG_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation HPG_HIGH paraterm Error !";
    }
    double PGR_LOW = GetConfig("PGR_LOW").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PGR_LOW paraterm Error !";
    }
    double PGR_HIGH = GetConfig("PGR_HIGH").toDouble(&ok);
    if(ok == false)
    {
        return "Get config interpretation PGR_HIGH paraterm Error !";
    }
   double PG1_MED = GetConfig("PG1_MED").toDouble(&ok);
   if(ok == false)
   {
       return "Get config interpretation PG1_MED paraterm Error !";
   }
   double PGR_MED = GetConfig("PGR_MED").toDouble(&ok);
   if(ok == false)
   {
       return "Get config interpretation PGR_MED paraterm Error !";
   }
   double G17B_REF2 = GetConfig("G17B_REF2").toDouble(&ok);
   if(ok == false)
   {
       return "Get config interpretation G17B_REF2 paraterm Error !";
   }
   double G17B_REF3 = GetConfig("G17B_REF3").toDouble(&ok);
   if(ok == false)
   {
       return "Get config interpretation G17B_REF3 paraterm Error !";
   }
   double G17B_REF4 = GetConfig("G17B_REF4").toDouble(&ok);
   if(ok == false)
   {
       return "Get config interpretation G17B_REF4 paraterm Error !";
   }
    QString zInterpretation_A = "";
    QString zInterpretation_B = "";
    QString zInterpretation_C = "";
    QString zConsultation = "";

    // Initialize parameters
    double PGR = -1.0;
    double G17_LOW = -1.0;
    double G17_HIGH = -1.0;

    // Marker values must exist; e.g 'invalid' not processed
    if (G17<0 || PG1<0 || PG2<0 || HPG<0)
    {
        return "#";
    }

    // Pepsonogen I/Pepsinogen II; division by 0 not allowed
    if (PG2 > 0.0)
    {
        PGR = PG1/PG2;
    }
    else
    {
        // PGII invalid
        return "#";
    }

    // Select reference based on patient preparation (sample type)
    if (post > 0) // Stimulated sample, if postprandial selected
    {
        G17_LOW = G17S_LOW;
        G17_HIGH = G17S_HIGH;
    }
    else		   // Basal sample, if fasting selected
    {
        G17_LOW = G17B_LOW;
        G17_HIGH = G17B_HIGH;
    }

    //
    // Atrophic gastritis
    //
    if (PG1 < PG1_LOW || (PGR < PGR_LOW && PG1 < PG1_MED))
    {
        if (HPG >= HPG_HIGH)
        {
            if (G17 < G17_LOW)
            {
                zInterpretation_A = ("TXT_P_POS+");
            }
            else
            {
                zInterpretation_A = ("TXT_C_POS+");
            }
        }
        else
        {
            if (G17 < G17_LOW)
            {
                zInterpretation_A = ("TXT_C_NEG_LOW_G17+");
            }
            else
            {
                zInterpretation_A = ("TXT_C_NEG+");
            }
        }

        // Acid
        if (post < 1 && G17 > G17B_REF4)
        {
            zInterpretation_B = ("TXT_VHIGH_G17B+");
        }
        else if (post < 1 && G17 > G17B_REF2)
        {
            zInterpretation_B = ("TXT_HIGH_G17B+");
        }

        // Consultation
        zConsultation = ("TXT_C2+");
    }

    //
    // H.pylori
    //
    else if (HPG >= HPG_HIGH)
    {
        zInterpretation_A = ("TXT_S+");
        zConsultation = ("TXT_C3+");

        if (G17 < G17_LOW)
        {
            zInterpretation_B = ("TXT_AGC_OK+");
        }
        else
        {
            zInterpretation_B = ("TXT_AG_OK+");
        }

        if (post > 0 && G17 < G17S_LOW)
        {
            zInterpretation_C = ("TXT_A_POS_LOW_G17S+");
            zConsultation = ("TXT_C2+");
        }
        else if (post < 1 && G17 < G17B_LOW)
        {
            if (PG1 > PG1_MED && PGR > PGR_MED)
            {
                zInterpretation_C = ("TXT_A_POS_ULCER+");
            }
            else
            {
                zInterpretation_C = ("TXT_A_POS+");
            }
             zConsultation = ("TXT_C1+");
        }
    }

    //
    // Normal
    //
    else
    {
        zInterpretation_A = ("TXT_N+");

        if (PG1 > PG1_HIGH || PG2 > PG2_HIGH || (post < 1 && G17 > G17B_HIGH) )
        {
            zInterpretation_B = ("TXT_N_PPI_UNKNOWN+");
        }

        if (G17 < G17_LOW)
        {
            zInterpretation_C = ("TXT_N_LOW_G17+");
        }
        else if (post<1 && G17 < G17B_REF2)
        {
            zInterpretation_C = ("TXT_NORMAL_G17B+");
        }
        else if (post<1 && G17 < G17B_REF3)
        {
             zInterpretation_C = ("TXT_ELEV_G17B+");
        }
        else if (post<1 && G17 < G17B_REF4)
        {
            zInterpretation_C = ("TXT_HIGH_G17B+");
        }
        else if (post<1 && G17 >= G17B_REF4)
        {
            zInterpretation_C = ("TXT_VHIGH_G17B+");
        }
    }

    return zInterpretation_A + zInterpretation_B + zInterpretation_C + zConsultation + "#";
}

bool APP_dev::cgr_save_back()
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    int ImageA[50][300];
    int ImageB[50][300];
    QImage ImagA[4];
    QImage ImagB[4];
    CgrDev.Timer->stop();
    for(int i=0;i<4;i++)
    {
        Delayms(50);
        cgr_get_imgab(ImagA[i], ImagB[i]);
    }
    CgrDev.Timer->start();
    //读取图像的G值
    for(int i=0;i<CgrDev.ImgPosPm.Ysize;i++)
    {
        for(int j=0;j<CgrDev.ImgPosPm.Xsize;j++)
        {
            ImageA[i][j] =0;
            ImageB[i][j] =0;

            ImageA[i][j] += qGreen(ImagA[3].pixel(j,i));
            ImageB[i][j] += qGreen(ImagB[4].pixel(j,i));

            ImageA[i][j] += qGreen(ImagA[1].pixel(j,i));
            ImageB[i][j] += qGreen(ImagB[1].pixel(j,i));

            CgrDev.CgrVol.ImgAbk[i][j]=ImageA[i][j]/2;
            CgrDev.CgrVol.ImgBbk[i][j]=ImageB[i][j]/2;
        }
    }
    QByteArray ImageBin;
    char *DataP;
    ImageBin.clear();
    DataP=(char*)(&CgrDev.CgrVol.ImgAbk[0][0]);
    ImageBin.append(DataP,sizeof(int)*50*300);

    DataP=(char*)(&CgrDev.CgrVol.ImgBbk[0][0]);
    ImageBin.append(DataP,sizeof(int)*50*300);

    QFile ImageFile("cgr_bk.bin");
    if(!ImageFile.open(QIODevice::WriteOnly))
    {
        ImageFile.close();
        return false;
    }
    ImageFile.write(ImageBin);
    ImageFile.close();
    return true;
}

bool APP_dev::cgr_load_back()
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    memset(&CgrDev.CgrVol.ImgAbk[0][0],0,50*300*sizeof(int));
    memset(&CgrDev.CgrVol.ImgBbk[0][0],0,50*300*sizeof(int));
    QByteArray ImageBin;
    char *DataP;
    QFile ImageFile("cgr_bk.bin");
    if(!ImageFile.open(QIODevice::ReadOnly))
    {
        ImageFile.close();
        return false;
    }
    ImageBin.clear();
    ImageBin=ImageFile.read(50*300*sizeof(int)) ;
    DataP=(char*)(&CgrDev.CgrVol.ImgAbk[0][0]);
    for(uint i=0;i<50*300*sizeof(int);i++)
    {
        DataP[i]=ImageBin.at(i);
    }
    ImageBin.clear();
    ImageBin=ImageFile.read(50*300*sizeof(int)) ;
    DataP=(char*)(&CgrDev.CgrVol.ImgBbk[0][0]);
    for(uint i=0;i<50*300*sizeof(int);i++)
    {
        DataP[i]=ImageBin.at(i);
    }
    ImageFile.close();
    return true;
}

bool APP_dev::cgr_load_pm()
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
#ifdef VideoFormartRGB
    CgrDev.ImgPosPm.Xpos=166;
    CgrDev.ImgPosPm.Ypos=  80;
    CgrDev.ImgPosPm.Xsize=300;
    CgrDev.ImgPosPm.Ysize= 30;
    CgrDev.ImgPosPm.abx  =255;
    CgrDev.CameraPm.Agc=0;//增益
    CgrDev.CameraPm.Exp=110;//曝光
    CgrDev.CameraPm.WhiteBalance=4600;//白平衡
    CgrDev.CameraPm.Brightness=0;//亮度
    CgrDev.CameraPm.Contrast=32;//对比度
    CgrDev.CameraPm.Saturation=64;//饱和度
    CgrDev.CameraPm.Hue=0;//色调
    CgrDev.CameraPm.Sharpness=3;//清晰度
    CgrDev.CameraPm.Gamma=100;//伽玛
    CgrDev.CameraPm.Focus=10;//焦点
#endif
#ifdef VideoFormartJPG
    CgrDev.ImgPosPm.Xpos=330;
    CgrDev.ImgPosPm.Ypos= 60;
    CgrDev.ImgPosPm.Xsize=600;
    CgrDev.ImgPosPm.Ysize= 60;
    CgrDev.ImgPosPm.abx  =500;

    CgrDev.CameraPm.Agc=0;//增益
    CgrDev.CameraPm.Exp=50;//曝光110
    CgrDev.CameraPm.WhiteBalance=50;//白平衡4600
    CgrDev.CameraPm.Brightness=0;//亮度
    CgrDev.CameraPm.Contrast=50;//对比度
    CgrDev.CameraPm.Saturation=10;//饱和度
    CgrDev.CameraPm.Hue=0;//色调
    CgrDev.CameraPm.Sharpness=10;//清晰度
    CgrDev.CameraPm.Gamma=10;//伽玛
    CgrDev.CameraPm.Focus=0;//焦点
#endif
    QByteArray ImageBin;
    char *DataP;
    QFile ImageFile("cgr_pm.bin");
    if(!ImageFile.open(QIODevice::ReadOnly))
    {
        ImageFile.close();
        return false;
    }
    //获得像素位置和像素框大小
    ImageBin.clear();
    ImageBin=ImageFile.read(sizeof(ImgPosStr)) ;
     DataP=(char*)&(CgrDev.ImgPosPm);
     int len=sizeof(ImgPosStr);
    for(int i=0;i<len;i++)
    {
        DataP[i]=ImageBin.at(i);
    }
    //获得相机参数
    ImageBin.clear();
    ImageBin=ImageFile.read(sizeof(CameraPmStr)) ;
    DataP=(char*)&(CgrDev.CameraPm);
    int leng=sizeof(CameraPmStr);
    for(int i=0;i<leng;i++)
    {
        DataP[i]=ImageBin.at(i);
    }
    ImageFile.close();
    //ImgPosStr ImgPosPm=CgrDev.ImgPosPm;
    //qDebug("Xpos=%d,Ypos=%d,Abx=%d,Ysize=%d,Xsize=%d,",ImgPosPm.Xpos,ImgPosPm.Ypos,ImgPosPm.abx,ImgPosPm.Ysize,ImgPosPm.Xsize);
    return true;
}

bool APP_dev::cgr_save_pm()
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    QByteArray ImageBin;
    char *DataP;
    ImageBin.clear();
    DataP=(char*)(&CgrDev.ImgPosPm);
    ImageBin.append(DataP,sizeof(ImgPosStr));
    DataP=(char*)(&CgrDev.CameraPm);
    ImageBin.append(DataP,sizeof(CameraPmStr));

    QFile ImageFile("cgr_pm.bin");
    if(!ImageFile.open(QIODevice::WriteOnly))
    {
        ImageFile.close();
        return false;
    }
    ImageFile.write(ImageBin);
    ImageFile.close();
    return true;
}

uchar APP_dev::cgr_get_insert()
{
    if(AppSta!=CGR_DEV)
    {
        return 2;
    }
    if(GetPinState(CardInsert_Pin)==0)
    {
        Delayms(10);
        if(GetPinState(CardInsert_Pin)==0)
        {
            Delayms(10);
            qDebug()<<"card is not insert!";
            return 0;
        }
    }
    if(GetPinState(CardInsert_Pin)==1)
    {
        Delayms(10);
        if(GetPinState(CardInsert_Pin)==1)
        {
            Delayms(10);
            qDebug()<<"card is insert!";
            return 1;
        }
    }
    return 2;
}

bool APP_dev::cgr_rd_led_ver()
{
    qDebug()<<"cgr read version!";
    char CmdBuf[16]={0x55,0x55,'&','C','G','R',0x00,0x00,0x00,0xff,0xff,'\n',0x55,0x55};
    QByteArray RxData;
    if(cgr_wr_data(CmdBuf,16))
    {
        RxData=cgr_rd_data(0x00,1000);
    }
    RxData.append("\r\n");
    memset(CgrDev.LedState.Version,0,50);
    for(int index=0;index<50 && index<<RxData.length();index++)
    {
         CgrDev.LedState.Version[index]=RxData.at(index);
    }
    return true;
}

bool APP_dev::cgr_rd_led_stat()
{
    qDebug()<<"cgr read state!";
    char CmdBuf[16]={0x55,0x55,'&','C','G','R',0x01,0x00,0x00,0xff,0xff,'\n',0x55,0x55};
    QByteArray RxData;
    if(cgr_wr_data(CmdBuf,16))
    {
        RxData=cgr_rd_data(0x01,1000);
    }
    CgrDev.LedState.LedOnoff=RxData.at(0);
    CgrDev.LedState.Led1AdcVol=RxData.at(1);
    CgrDev.LedState.Led2AdcVol=RxData.at(2);
    CgrDev.LedState.Led3AdcVol=RxData.at(3);
    CgrDev.LedState.Led4AdcVol=RxData.at(4);
    CgrDev.LedState.LedTemp=RxData.at(5);
    return true;
}

bool APP_dev::cgr_rd_led_pm()
{
    qDebug()<<"cgr read pm!";
    char CmdBuf[16]={0x55,0x55,'&','C','G','R',0x02,0x00,0x00,0xff,0xff,'\n',0x55,0x55};

    QByteArray RxData;
    if(cgr_wr_data(CmdBuf,16))
    {
        RxData=cgr_rd_data(0x02,1000);
    }

    CgrDev.LedState.Led1SetVol=RxData.at(0);
    CgrDev.LedState.Led2SetVol=RxData.at(1);
    CgrDev.LedState.Led3SetVol=RxData.at(2);
    CgrDev.LedState.Led4SetVol=RxData.at(3);
    return true;
}

bool APP_dev::cgr_wr_led_pm()
{
    qDebug()<<"cgr write ver!";
    char CmdBuf[16]={0x55,'&','C','G','R',0x03,0x00,0x04,0,0,0,0,0xff,0xff,'\n',0x55};
    CmdBuf[  8]=CgrDev.LedState.Led1SetVol;
    CmdBuf[  9]=CgrDev.LedState.Led2SetVol;
    CmdBuf[10]=CgrDev.LedState.Led3SetVol;
    CmdBuf[11]=CgrDev.LedState.Led4SetVol;
    u_int16_t CRC16 =GetCRC(CmdBuf+8,4);
    CmdBuf[12]=uchar(CRC16>>8);
    CmdBuf[13]=uchar(CRC16>>0);

    QByteArray RxData;
    if(cgr_wr_data(CmdBuf,16))
    {
        RxData=cgr_rd_data(0x03,1000);
    }
    return true;
}

bool APP_dev::cgr_led_onoff(uchar Stat)
{
    if(Stat>0)
    {
        qDebug()<<"RGB Led ON!";
    }
    else
    {
        qDebug()<<"RGB Led Off!";
    }
    char CmdBuf[16]={0x55,'&','C','G','R',0x04,0x00,0x01,0x00,0xff,0xff,'\n',0x55};
    CmdBuf[8]=Stat;
    u_int16_t CRC16 =GetCRC(CmdBuf+8,1);
    CmdBuf[9]=uchar(CRC16>>8);
    CmdBuf[10]=uchar(CRC16>>0);

    QByteArray RxData;
    if(cgr_wr_data(CmdBuf,16))
    {
        RxData=cgr_rd_data(0x04,1000);
    }
    return true;
}

bool APP_dev::cgr_led_on()
{
    return cgr_led_onoff(0x0f);
}

bool APP_dev::cgr_led_off()
{
    return cgr_led_onoff(0x00);
}

bool APP_dev::cgr_set_cam(CameraPmStr &CameraPm)
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    if(UsbCamera<0)
    {
        qDebug()<<"camera is error!";
        return false;
    }
#ifdef  VideoFormartRGB
    //qDebug()<<"set camera !";
    UsbCamera->SetGain(CameraPm.Agc); //增益
    Delayms(20);
    UsbCamera->SetExposure(CameraPm.Exp); //曝光
    Delayms(20);
    UsbCamera->SetWhiteBalance(CameraPm.WhiteBalance); //白平衡
    Delayms(20);
    UsbCamera->SetBrightness(CameraPm.Brightness);  //亮度
    Delayms(20);
    UsbCamera->SetContrast(CameraPm.Contrast); //对比度
    Delayms(20);
    UsbCamera->SetSaturation (CameraPm.Saturation); //饱和度
    Delayms(20);
    UsbCamera->SetHue(CameraPm.Hue); //色调
    Delayms(20);
    UsbCamera->SetSharpness(CameraPm.Sharpness); //清晰度
    Delayms(20);
    UsbCamera->SetGamma(CameraPm.Gamma); //伽玛
    Delayms(20);
    UsbCamera->SetFocusing(CameraPm.Focus); //焦点
    Delayms(20);
    UsbCamera->GetBuffer(PixImgBuf);//获得数据
#endif
#ifdef  VideoFormartJPG
//    UsbCamera->SetGain(CameraPm.Agc); //增益
//    Delayms(20);
//    UsbCamera->SetExposure(CameraPm.Exp); //曝光
//    Delayms(20);
//    UsbCamera->SetWhiteBalance(CameraPm.WhiteBalance); //白平衡
//    Delayms(20);
//    UsbCamera->SetBrightness(CameraPm.Brightness);  //亮度
//    Delayms(20);
//    UsbCamera->SetContrast(CameraPm.Contrast); //对比度
//    Delayms(20);
//    UsbCamera->SetSaturation (CameraPm.Saturation); //饱和度
//    Delayms(20);
//    UsbCamera->SetHue(CameraPm.Hue); //色调
//    Delayms(20);
//    UsbCamera->SetSharpness(CameraPm.Sharpness); //清晰度
//    Delayms(20);
//    UsbCamera->SetGamma(CameraPm.Gamma); //伽玛
//    Delayms(20);
//    UsbCamera->SetFocusing(CameraPm.Focus); //焦点
//    Delayms(20);
//    UsbCamera->GetBuffer(PixImgBuf);//获得数据
#endif
    return true;
}

bool APP_dev::cgr_get_image(QPixmap &ImagePix)
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    if(UsbCamera<0)
    {
        return false;
    }
    UsbCamera->GetBuffer(PixImgBuf);
    QImage Image;
#ifdef  VideoFormartRGB
    Image= QImage((uchar *) PixImgBuf, 640, 480, QImage::Format_RGB888);
    //标示线
    ImgPosStr ImgPos=CgrDev.ImgPosPm;
    for(int Index=0;Index<=ImgPos.Xsize;Index++)
    {
        Image.setPixel(Index+ImgPos.Xpos,ImgPos.Ypos,Qt::blue);//水平线1
        Image.setPixel(Index+ImgPos.Xpos,ImgPos.Ypos+ImgPos.Ysize,Qt::blue);//水平线2
    }
    for(int Indey=0;Indey<=ImgPos.Ysize;Indey++)
    {
        Image.setPixel(ImgPos.Xpos,Indey+ImgPos.Ypos,Qt::blue);//垂直线1
        Image.setPixel(ImgPos.Xpos+ImgPos.Xsize,Indey+ImgPos.Ypos,Qt::blue);//垂直线2
    }
    for(int Index=0;Index<=ImgPos.Xsize;Index++)
    {
        Image.setPixel(Index+ImgPos.Xpos,ImgPos.Ypos+ImgPos.abx,Qt::blue);//水平线1
        Image.setPixel(Index+ImgPos.Xpos,ImgPos.Ypos+ImgPos.abx+ImgPos.Ysize,Qt::blue);//水平线2
    }
    for(int Indey=0;Indey<=ImgPos.Ysize;Indey++)
    {
        Image.setPixel(ImgPos.Xpos,Indey+ImgPos.Ypos+ImgPos.abx,Qt::blue);//垂直线1
        Image.setPixel(ImgPos.Xpos+ImgPos.Xsize,Indey+ImgPos.Ypos+ImgPos.abx,Qt::blue);//垂直线2
     }
#endif
#ifdef  VideoFormartJPG
    Image.loadFromData(PixImgBuf,UsbCamera->MjpgSize,"JPG");
#endif
    if(!Image.isNull())
    {
//        //标示线
//        ImgPosStr ImgPos=CgrDev.ImgPosPm;
//        for(int Index=0;Index<=ImgPos.Xsize;Index++)
//        {
//            Image.setPixel(Index+ImgPos.Xpos,ImgPos.Ypos,Qt::blue);//水平线1
//            Image.setPixel(Index+ImgPos.Xpos,ImgPos.Ypos+ImgPos.Ysize,Qt::blue);//水平线2
//        }
//        for(int Indey=0;Indey<=ImgPos.Ysize;Indey++)
//        {
//            Image.setPixel(ImgPos.Xpos,Indey+ImgPos.Ypos,Qt::blue);//垂直线1
//            Image.setPixel(ImgPos.Xpos+ImgPos.Xsize,Indey+ImgPos.Ypos,Qt::blue);//垂直线2
//        }
//        for(int Index=0;Index<=ImgPos.Xsize;Index++)
//        {
//            Image.setPixel(Index+ImgPos.Xpos,ImgPos.Ypos+ImgPos.abx,Qt::blue);//水平线1
//            Image.setPixel(Index+ImgPos.Xpos,ImgPos.Ypos+ImgPos.abx+ImgPos.Ysize,Qt::blue);//水平线2
//        }
//        for(int Indey=0;Indey<=ImgPos.Ysize;Indey++)
//        {
//            Image.setPixel(ImgPos.Xpos,Indey+ImgPos.Ypos+ImgPos.abx,Qt::blue);//垂直线1
//            Image.setPixel(ImgPos.Xpos+ImgPos.Xsize,Indey+ImgPos.Ypos+ImgPos.abx,Qt::blue);//垂直线2
//         }
        ImagePix= ImagePix.fromImage(Image);
    }
    return true;
}

bool APP_dev::cgr_get_imgab(QPixmap &ImagePixA, QPixmap &ImagePixB)
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    if(UsbCamera<0)
    {
        return false;
    }
    UsbCamera->GetBuffer(PixImgBuf);
    QImage Image;
    QImage PixImgA;
    QImage PixImgB;
#ifdef  VideoFormartRGB
    Image= QImage((uchar *) PixImgBuf, 640, 480, QImage::Format_RGB888);    
#endif
#ifdef  VideoFormartJPG
    Image.loadFromData(PixImgBuf,UsbCamera->MjpgSize,"JPG");
#endif
    PixImgB=Image.copy(CgrDev.ImgPosPm.Xpos,CgrDev.ImgPosPm.Ypos,CgrDev.ImgPosPm.Xsize,CgrDev.ImgPosPm.Ysize);
    PixImgA=Image.copy(CgrDev.ImgPosPm.Xpos,CgrDev.ImgPosPm.Ypos+CgrDev.ImgPosPm.abx,CgrDev.ImgPosPm.Xsize,CgrDev.ImgPosPm.Ysize);
    ImagePixA=QPixmap::fromImage(PixImgA);
    ImagePixB=QPixmap::fromImage(PixImgB);
    return true;
}

bool APP_dev::cgr_get_imgab(QImage &ImageA, QImage &ImageB)
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    if(UsbCamera<0)
    {
        return false;
    }
    UsbCamera->GetBuffer(PixImgBuf);
    QImage Image;
#ifdef  VideoFormartRGB
    Image= QImage((uchar *) PixImgBuf, 640, 480, QImage::Format_RGB888);    
#endif
#ifdef  VideoFormartJPG
    Image.loadFromData(PixImgBuf,UsbCamera->MjpgSize,"JPG");
#endif
    ImageB=Image.copy(CgrDev.ImgPosPm.Xpos,CgrDev.ImgPosPm.Ypos,CgrDev.ImgPosPm.Xsize,CgrDev.ImgPosPm.Ysize);
    ImageA=Image.copy(CgrDev.ImgPosPm.Xpos,CgrDev.ImgPosPm.Ypos+CgrDev.ImgPosPm.abx,CgrDev.ImgPosPm.Xsize,CgrDev.ImgPosPm.Ysize);
    return true;
}

bool APP_dev::cgr_open()
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    CgrDev.State = CgrNop;
    CgrDev.Timer = new QTimer();
    connect(CgrDev.Timer,SIGNAL(timeout()),this,SLOT(cgr_timeout()));
#ifdef  VideoFormartRGB
    CgrDev.Timer->setInterval(150);
#endif
#ifdef  VideoFormartJPG
    CgrDev.Timer->setInterval(200);
#endif
    CgrDev.Timer->start();
    return true;
}

TwoCodeStr CgrCode;
bool APP_dev::cgr_start(TwoCodeStr TwoDimCode)
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    CgrCode=TwoDimCode;
    CgrDev.State = CgrSavImg;
    CgrDev.Timer->stop();
#ifdef  VideoFormartRGB
    CgrDev.Timer->setInterval(150);
#endif
#ifdef  VideoFormartJPG
    CgrDev.Timer->setInterval(300);
#endif
    CgrDev.Timer->start();
    Delayms(10);
    return true;
}

bool APP_dev::cgr_timeout()
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    static int ImgCont=0;
    if(UsbCamera<0)
    {
        ImgCont=0;
        CgrDev.Timer->stop();
        CgrDev.State = CgrIsError;
        qDebug()<<"camera is error!";
        return false;
    }
    UsbCamera->GetBuffer(PixImgBuf);
    if(CgrDev.State != CgrSavImg)
    {
        ImgCont=0;
        return false;
    }
    //qDebug("cgr_timeout =%d",ImgCont);
    QImage Image;
    QImage PixImgHb;
    QImage PixImgHp;
#ifdef  VideoFormartRGB
    Image= QImage((uchar *) PixImgBuf, 640, 480, QImage::Format_RGB888);    
    PixImgB=Image.copy(CgrDev.ImgPosPm.Xpos,CgrDev.ImgPosPm.Ypos,CgrDev.ImgPosPm.Xsize,CgrDev.ImgPosPm.Ysize);
    PixImgA=Image.copy(CgrDev.ImgPosPm.Xpos,CgrDev.ImgPosPm.Ypos+CgrDev.ImgPosPm.abx,CgrDev.ImgPosPm.Xsize,CgrDev.ImgPosPm.Ysize);
    //读取图像的G值
    for(int i=0;i<CgrDev.ImgPosPm.Ysize;i++)
    {
        for(int j=0;j<CgrDev.ImgPosPm.Xsize;j++)
        {
            ImageA[ImgCont][i][j]=qGreen(PixImgA.pixel(j,i));
            ImageB[ImgCont][i][j]=qGreen(PixImgB.pixel(j,i));
        }
    }
#endif
#ifdef  VideoFormartJPG
    Image.loadFromData(PixImgBuf,UsbCamera->MjpgSize,"JPG");
    if(!Image.isNull())
    {
        //Hb
        PixImgHb=Image.copy(CgrDev.ImgPosPm.Xpos,CgrDev.ImgPosPm.Ypos+CgrDev.ImgPosPm.abx,CgrDev.ImgPosPm.Xsize,CgrDev.ImgPosPm.Ysize);
        //Hp
        PixImgHp=Image.copy(CgrDev.ImgPosPm.Xpos,CgrDev.ImgPosPm.Ypos,CgrDev.ImgPosPm.Xsize,CgrDev.ImgPosPm.Ysize);
         //保存图
        if(ImgCont==0)
        {
            CgrDev.CgrVol.HbImagePix=QPixmap::fromImage(PixImgHb);
            CgrDev.CgrVol.HpImagePix=QPixmap::fromImage(PixImgHp);
#ifdef WY
//            QImage  ImgApix=PixImgA;
//            QImage  ImgBpix=PixImgB;
//            for(int i=0;i<CgrDev.ImgPosPm.Ysize;i++)
//            {
//                for(int j=0;j<CgrDev.ImgPosPm.Xsize;j++)
//                {
//                    int Ra=(255-qRed(PixImgA.pixel(j,i)));
//                    int Ga=(255-qGreen(PixImgA.pixel(j,i)));
//                    int Ba=(255-qBlue(PixImgA.pixel(j,i)));
//                    int Rb=(255-qRed(PixImgB.pixel(j,i)));
//                    int Gb=(255-qGreen(PixImgB.pixel(j,i)));
//                    int Bb=(255-qBlue(PixImgB.pixel(j,i)));
////                    int Ra=(qRed(PixImgA.pixel(j,i)));
////                    int Ga=(qGreen(PixImgA.pixel(j,i)));
////                    int Ba=(qBlue(PixImgA.pixel(j,i)));
////                    int Rb=(qRed(PixImgB.pixel(j,i)));
////                    int Gb=(qGreen(PixImgB.pixel(j,i)));
////                    int Bb=(qBlue(PixImgB.pixel(j,i)));
//                    ImgApix.setPixel(j,i,qGray(Ra,Ga,Ba));
//                    ImgBpix.setPixel(j,i,qGray(Rb,Gb,Bb));
//                }
//            }
//            CgrDev.CgrVol.HbImagePix=QPixmap::fromImage(ImgApix);
//            CgrDev.CgrVol.HpImagePix=QPixmap::fromImage(ImgBpix);
#endif
        }
        //读取图像的G值
        for(int i=0;i<CgrDev.ImgPosPm.Ysize;i++)
        {
            for(int j=0;j<CgrDev.ImgPosPm.Xsize;j++)
            {
//                ImageA[ImgCont][i][j]=qGreen(PixImgA.pixel(j,i));
//                ImageB[ImgCont][i][j]=qGreen(PixImgB.pixel(j,i));
//                ImageA[ImgCont][i][j]=255-qGreen(PixImgA.pixel(j,i));
//                ImageB[ImgCont][i][j]=255-qGreen(PixImgB.pixel(j,i));
//                ImageA[ImgCont][i][j]=50-qRed(PixImgA.pixel(j,i));
//                ImageB[ImgCont][i][j]=50-qRed(PixImgB.pixel(j,i));
//                ImageA[ImgCont][i][j]=255-qBlue(PixImgA.pixel(j,i));
//                ImageB[ImgCont][i][j]=255-qBlue(PixImgB.pixel(j,i));
//                ImageA[ImgCont][i][j]=(255-qGreen(PixImgA.pixel(j,i)));//-qRed(PixImgA.pixel(j,i));
//                ImageB[ImgCont][i][j]=(255-qGreen(PixImgB.pixel(j,i)));//-qRed(PixImgB.pixel(j,i));
//                ImageA[ImgCont][i][j]=(255-qGreen(PixImgA.pixel(j,i)))+50-qRed(PixImgA.pixel(j,i));
//                ImageB[ImgCont][i][j]=(255-qGreen(PixImgB.pixel(j,i)))+50-qRed(PixImgB.pixel(j,i));
//                ImageA[ImgCont][i][j]=(255-qGreen(PixImgA.pixel(j,i)))+(255-qRed(PixImgA.pixel(j,i)))+(255-qBlue(PixImgA.pixel(j,i)));
//                ImageB[ImgCont][i][j]=(255-qGreen(PixImgB.pixel(j,i)))+(255-qRed(PixImgB.pixel(j,i)))+(255-qBlue(PixImgA.pixel(j,i)));
                double Ra=(255-qRed(PixImgHb.pixel(j,i)));
                double Ga=(255-qGreen(PixImgHb.pixel(j,i)));
                double Ba=(255-qBlue(PixImgHb.pixel(j,i)));
                double Graya=Ra*0.3+Ga*0.6+Ba*0.1;

                double Rb=(255-qRed(PixImgHp.pixel(j,i)));
                double Gb=(255-qGreen(PixImgHp.pixel(j,i)));
                double Bb=(255-qBlue(PixImgHp.pixel(j,i)));
                double Grayb=Rb*0.3+Gb*0.6+Bb*0.1;

                ImageA[ImgCont][i][j]=Graya;
                ImageB[ImgCont][i][j]=Grayb;
            }
        }
        if(ImgCont <10)
        {
            ImgCont++;
        }
    }
#endif
#ifdef  VideoFormartRGB
    if(ImgCont <40)
    {
        ImgCont++;
        return false ;
    }
#endif
#ifdef  VideoFormartJPG
    if(ImgCont <10)
    {
        return false ;
    }
#endif
    ImgCont=0;
    CgrDev.Timer->stop();
    CgrDev.State = CgrCalcImg;
    if(GetConfig("CGR_SaveJPG")=="Yes")
    {
        if(CgrDev.CgrVol.HbImagePix.save("./ImageA.jpg")==false)
        {
            qDebug("ImageA.jpg save error!");
        }
        if(CgrDev.CgrVol.HpImagePix.save("./ImageB.jpg")==false)
        {
            qDebug("ImageB.jpg save error!");
        }
    }
    qDebug()<<"//-----------------------//";
    cgr_calc(CgrDev);
    qDebug()<<"//-----------------------//";
    CgrDev.State = CgrCalcEnd;
#ifdef  VideoFormartRGB
    CgrDev.Timer->setInterval(150);
#endif
#ifdef  VideoFormartJPG
    CgrDev.Timer->setInterval(300);
#endif
    CgrDev.Timer->start();
    return true;
}

bool APP_dev::cgr_calc(CgrDevStr &CgrDev)
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    if(!CgrCode.Head.contains("YX-",Qt::CaseSensitive))
    {
        qDebug()<<"2 code is error!";
        return false;
    }
    qDebug("Ysize=%d,Xsize=%d,",CgrDev.ImgPosPm.Ysize,CgrDev.ImgPosPm.Xsize);
    qDebug()<<"cgr_calc = 1";
    //数据分布情况：0T0|0C0
    //CgrDev.ImgPosPm.Ysize=30;//50
    //CgrDev.ImgPosPm.Xsize=300;//350
    //求均值图像:对40副图片的30*600个像素点进行均值
    double ImgA[60][600];
    double ImgB[60][600];
    memset((char*)ImgA,0,60*600*sizeof(double));
    memset((char*)ImgB,0,60*600*sizeof(double));
    for(int index=0;index<CgrDev.ImgPosPm.Ysize;index++)
    {
         //求一个均值像素点
#ifdef VideoFormartRGB
        for(int indey=0;indey<CgrDev.ImgPosPm.Xsize;indey++)
        {
            int TempA[40];
            int TempB[40];
            for(int i=0;i<50;i++)
            {
                TempA[i]=ImageA[i][index][indey];
                TempB[i]=ImageB[i][index][indey];
            }
            for(int i=0;i<40;i++)
            {
                for(int j=0;j<40;j++)
                {
                    if(TempA[j]>TempA[i])
                    {
                        int temp=TempA[i];
                        TempA[i]=TempA[j];
                        TempA[j]=temp;
                    }
                    if(TempB[j]>TempB[i])
                    {
                        int temp=TempB[i];
                        TempB[i]=TempB[j];
                        TempB[j]=temp;
                    }
                }
            }
            for(int i=10;i<30;i++)
            {
                ImgA[index][indey] +=(double)TempA[i];
                ImgB[index][indey] +=(double)TempB[i];
            }
            ImgA[index][indey] /=20.0;
            ImgB[index][indey] /=20.0;
         }
#endif
#ifdef VideoFormartJPG
        for(int indey=0;indey<CgrDev.ImgPosPm.Xsize;indey++)
        {
            int TempA[10];
            int TempB[10];
            for(int i=0;i<10;i++)
            {
                TempA[i]=ImageA[i][index][indey];
                TempB[i]=ImageB[i][index][indey];
            }
            for(int i=0;i<10;i++)
            {
                for(int j=0;j<10;j++)
                {
                    if(TempA[j]>TempA[i])
                    {
                        int temp=TempA[i];
                        TempA[i]=TempA[j];
                        TempA[j]=temp;
                    }
                    if(TempB[j]>TempB[i])
                    {
                        int temp=TempB[i];
                        TempB[i]=TempB[j];
                        TempB[j]=temp;
                    }
                }
            }
            for(int i=3;i<=6;i++)
            {
                ImgA[index][indey] +=(double)TempA[i];
                ImgB[index][indey] +=(double)TempB[i];
            }
            ImgA[index][indey] /=4.0;
            ImgB[index][indey] /=4.0;
        }
#endif
    }
    qDebug()<<"cgr_calc = 2";

    //均值线
    for(int indey=0;indey<CgrDev.ImgPosPm.Xsize;indey++)
    {
#ifdef VideoFormartRGB
        CgrDev.CgrVol.ImageA[indey]=0;
        CgrDev.CgrVol.ImageB[indey]=0;
        for(int index=10;index<20;index++)
        {
            CgrDev.CgrVol.ImageA[indey] +=ImgA[index][indey];
            CgrDev.CgrVol.ImageB[indey] +=ImgB[index][indey];
        }
        CgrDev.CgrVol.ImageA[indey] /=(10);
        CgrDev.CgrVol.ImageB[indey] /=(10);
#endif
#ifdef VideoFormartJPG
        CgrDev.CgrVol.ImageA[indey]=0;
        CgrDev.CgrVol.ImageB[indey]=0;
        for(int index=20;index<40;index++)
        {
             CgrDev.CgrVol.ImageA[indey] +=ImgA[index][indey];
             CgrDev.CgrVol.ImageB[indey] +=ImgB[index][indey];
        }
        CgrDev.CgrVol.ImageA[indey] /=(20);
        CgrDev.CgrVol.ImageB[indey] /=(20);
#endif
    }
    qDebug()<<"cgr_calc = 3";

    double AtMaxVol=0;
    double AcMaxVol=0;
    double BtMaxVol=0;
    double BcMaxVol=0;
    int AtMaxPos=0;
    int AcMaxPos=0;
    int BtMaxPos=0;
    int BcMaxPos=0;

    double AtMinVol=10000;
    double AcMinVol=10000;
    double BtMinVol=10000;
    double BcMinVol=10000;
    int AtMinPos=0;
    int AcMinPos=0;
    int BtMinPos=0;
    int BcMinPos=0;
//     int AtStart=CgrCode.XiangMuPm[0].tqidian.toInt();//Hb
//     int AtEnd=CgrCode.XiangMuPm[0].tzhongdian.toInt();
//     int AcStart=CgrCode.XiangMuPm[0].cqidian.toInt();
//     int AcEnd=CgrCode.XiangMuPm[0].czhongdian.toInt();

//     int BtStart=CgrCode.XiangMuPm[1].tqidian.toInt();//Hp
//     int BtEnd=CgrCode.XiangMuPm[1].tzhongdian.toInt();
//     int BcStart=CgrCode.XiangMuPm[1].cqidian.toInt();
//     int BcEnd=CgrCode.XiangMuPm[1].czhongdian.toInt();
#ifdef VideoFormartRGB
     if(AtStart <=0 || AtStart>=350)  { AtStart=50; }
     if(AtEnd  <=0 || AtEnd>=350)  { AtEnd=100; }
     if(AcStart <=0 || AcStart>=350)  { AcStart=200; }
     if(AcEnd  <=0 || AcEnd>=350)  { AcEnd=250; }

     if(BtStart <=0 || BtStart>=350)  { BtStart=50; }
     if(BtEnd  <=0  || BtEnd>=350)  { BtEnd=100; }
     if(BcStart <=0 || BcStart>=350)  { BcStart=200; }
     if(BcEnd  <=0 || BcEnd>=350)  { BcEnd=250; }
#endif
#ifdef VideoFormartJPG
//     if(AtStart <0 || AtStart>600)  { AtStart=150; }
//     if(AtEnd  <0 || AtEnd>600)  { AtEnd=250; }
//     if(AcStart<0 || AcStart>600)  { AcStart=350; }
//     if(AcEnd <0 || AcEnd>600)  { AcEnd=450; }

//     if(BtStart <0 || BtStart>600)  { BtStart=150; }
//     if(BtEnd  <0  || BtEnd>600)  { BtEnd=250; }
//     if(BcStart<0 || BcStart>600)  { BcStart=350; }
//     if(BcEnd <0 || BcEnd>600)  { BcEnd=450; }
#endif

//     qDebug()<<"AtStart="<<AtStart;
//     qDebug()<<"AtEnd="<<AtEnd;
//     qDebug()<<"AcStart="<<AcStart;
//     qDebug()<<"AcEnd="<<AcEnd;

//     qDebug()<<"BtStart="<<BtStart;
//     qDebug()<<"BtEnd="<<BtEnd;
//     qDebug()<<"BcStart="<<BcStart;
//     qDebug()<<"BcEnd="<<BcEnd;
//     qDebug()<<"cgr_calc = 4";
     //计算 T and C 峰值
    //T
//    for(int index=AtStart;index<=AtEnd;index++)
//    {
//        //A-Max
//        if(CgrDev.CgrVol.ImageA[index]>AtMaxVol)
//        {
//            AtMaxPos=index;
//            AtMaxVol=CgrDev.CgrVol.ImageA[index];
//        }
//        //A-Min
//        if(CgrDev.CgrVol.ImageA[index]<AtMinVol)
//        {
//            AtMinPos=index;
//            AtMinVol=CgrDev.CgrVol.ImageA[index];
//        }
//    }
//    for(int index=BtStart;index<=BtEnd;index++)
//    {
//        //B-Max
//        if(CgrDev.CgrVol.ImageB[index]>BtMaxVol)
//        {
//            BtMaxPos=index;
//            BtMaxVol=CgrDev.CgrVol.ImageB[index];
//        }
//        //B-Min
//        if(CgrDev.CgrVol.ImageB[index]<BtMinVol)
//        {
//            BtMinPos=index;
//            BtMinVol=CgrDev.CgrVol.ImageB[index];
//        }
//    }
    //C
//    for(int index=AcStart;index<=AcEnd;index++)
//    {
//        //A-Max
//        if(CgrDev.CgrVol.ImageA[index]>AcMaxVol)
//        {
//            AcMaxPos=index;
//            AcMaxVol=CgrDev.CgrVol.ImageA[index];
//        }
//        //A-Min
//        if(CgrDev.CgrVol.ImageA[index]<AcMinVol)
//        {
//            AcMinPos=index;
//            AcMinVol=CgrDev.CgrVol.ImageA[index];
//        }
//    }
//    for(int index=BcStart;index<=BcEnd;index++)
//    {
//        //B-Max
//        if(CgrDev.CgrVol.ImageB[index]>BcMaxVol)
//        {
//            BcMaxPos=index;
//            BcMaxVol=CgrDev.CgrVol.ImageB[index];
//        }
//        //B-Min
//        if(CgrDev.CgrVol.ImageB[index]<BcMinVol)
//        {
//            BcMinPos=index;
//            BcMinVol=CgrDev.CgrVol.ImageB[index];
//        }
//    }
    qDebug()<<"cgr_calc = 5";
    qDebug()<<"AcMaxVol="<<AcMaxVol;
    qDebug()<<"AcMinVol="<<AcMinVol;
    qDebug()<<"AtMaxVol="<<AtMaxVol;
    qDebug()<<"AtMinVol="<<AtMinVol;

    qDebug()<<"BcMaxVol="<<BcMaxVol;
    qDebug()<<"BcMinVol="<<BcMinVol;
    qDebug()<<"BtMaxVol="<<BtMaxVol;
    qDebug()<<"BtMinVol="<<BtMinVol;
    qDebug()<<"cgr_calc = 6";
    CgrDev.CgrVol.ImageATpos=AtMaxPos;
    CgrDev.CgrVol.ImageBTpos=BtMaxPos;
    qDebug()<<"AtMinPos="<<AtMinPos;
    qDebug()<<"BtMinPos="<<BtMinPos;
    qDebug()<<"AtMaxPos="<<AtMaxPos;
    qDebug()<<"BtMaxPos="<<BtMaxPos;

    double TVol[2];
    double CVol[2];
    TVol[0]=0;
    TVol[0]+=(CgrDev.CgrVol.ImageA[AtMaxPos -1])-(CgrDev.CgrVol.ImageA[AtMinPos]);
    TVol[0]+=(CgrDev.CgrVol.ImageA[AtMaxPos+0])-(CgrDev.CgrVol.ImageA[AtMinPos]);
    TVol[0]+=(CgrDev.CgrVol.ImageA[AtMaxPos+1])-(CgrDev.CgrVol.ImageA[AtMinPos]);
    TVol[0]+=(CgrDev.CgrVol.ImageA[AtMaxPos+2])-(CgrDev.CgrVol.ImageA[AtMinPos]);
    TVol[0]+=(CgrDev.CgrVol.ImageA[AtMaxPos+3])-(CgrDev.CgrVol.ImageA[AtMinPos]);
    TVol[0]+=(CgrDev.CgrVol.ImageA[AtMaxPos+4])-(CgrDev.CgrVol.ImageA[AtMinPos]);
    TVol[0]+=(CgrDev.CgrVol.ImageA[AtMaxPos+5])-(CgrDev.CgrVol.ImageA[AtMinPos]);
    TVol[0]+=(CgrDev.CgrVol.ImageA[AtMaxPos+6])-(CgrDev.CgrVol.ImageA[AtMinPos]);
    TVol[0]+=(CgrDev.CgrVol.ImageA[AtMaxPos+7])-(CgrDev.CgrVol.ImageA[AtMinPos]);

    CVol[0]=0;
    CVol[0]+=(CgrDev.CgrVol.ImageA[AcMaxPos -1])-(CgrDev.CgrVol.ImageA[AcMinPos]);
    CVol[0]+=(CgrDev.CgrVol.ImageA[AcMaxPos+0])-(CgrDev.CgrVol.ImageA[AcMinPos]);
    CVol[0]+=(CgrDev.CgrVol.ImageA[AcMaxPos+1])-(CgrDev.CgrVol.ImageA[AcMinPos]);
    CVol[0]+=(CgrDev.CgrVol.ImageA[AcMaxPos+2])-(CgrDev.CgrVol.ImageA[AcMinPos]);
    CVol[0]+=(CgrDev.CgrVol.ImageA[AcMaxPos+3])-(CgrDev.CgrVol.ImageA[AcMinPos]);
    CVol[0]+=(CgrDev.CgrVol.ImageA[AcMaxPos+4])-(CgrDev.CgrVol.ImageA[AcMinPos]);
    CVol[0]+=(CgrDev.CgrVol.ImageA[AcMaxPos+5])-(CgrDev.CgrVol.ImageA[AcMinPos]);
    CVol[0]+=(CgrDev.CgrVol.ImageA[AcMaxPos+6])-(CgrDev.CgrVol.ImageA[AcMinPos]);
    CVol[0]+=(CgrDev.CgrVol.ImageA[AcMaxPos+7])-(CgrDev.CgrVol.ImageA[AcMinPos]);

    TVol[1]=0;
    TVol[1]+=(CgrDev.CgrVol.ImageB[BtMaxPos -1])-(CgrDev.CgrVol.ImageB[BtMinPos]);
    TVol[1]+=(CgrDev.CgrVol.ImageB[BtMaxPos+0])-(CgrDev.CgrVol.ImageB[BtMinPos]);
    TVol[1]+=(CgrDev.CgrVol.ImageB[BtMaxPos+1])-(CgrDev.CgrVol.ImageB[BtMinPos]);
    TVol[1]+=(CgrDev.CgrVol.ImageB[BtMaxPos+2])-(CgrDev.CgrVol.ImageB[BtMinPos]);
    TVol[1]+=(CgrDev.CgrVol.ImageB[BtMaxPos+3])-(CgrDev.CgrVol.ImageB[BtMinPos]);
    TVol[1]+=(CgrDev.CgrVol.ImageB[BtMaxPos+4])-(CgrDev.CgrVol.ImageB[BtMinPos]);
    TVol[1]+=(CgrDev.CgrVol.ImageB[BtMaxPos+5])-(CgrDev.CgrVol.ImageB[BtMinPos]);
    TVol[1]+=(CgrDev.CgrVol.ImageB[BtMaxPos+6])-(CgrDev.CgrVol.ImageB[BtMinPos]);
    TVol[1]+=(CgrDev.CgrVol.ImageB[BtMaxPos+7])-(CgrDev.CgrVol.ImageB[BtMinPos]);

    CVol[1]=0;
    CVol[1]+=(CgrDev.CgrVol.ImageB[BcMaxPos -1])-(CgrDev.CgrVol.ImageB[BcMinPos]);
    CVol[1]+=(CgrDev.CgrVol.ImageB[BcMaxPos+0])-(CgrDev.CgrVol.ImageB[BcMinPos]);
    CVol[1]+=(CgrDev.CgrVol.ImageB[BcMaxPos+1])-(CgrDev.CgrVol.ImageB[BcMinPos]);
    CVol[1]+=(CgrDev.CgrVol.ImageB[BcMaxPos+2])-(CgrDev.CgrVol.ImageB[BcMinPos]);
    CVol[1]+=(CgrDev.CgrVol.ImageB[BcMaxPos+3])-(CgrDev.CgrVol.ImageB[BcMinPos]);
    CVol[1]+=(CgrDev.CgrVol.ImageB[BcMaxPos+4])-(CgrDev.CgrVol.ImageB[BcMinPos]);
    CVol[1]+=(CgrDev.CgrVol.ImageB[BcMaxPos+5])-(CgrDev.CgrVol.ImageB[BcMinPos]);
    CVol[1]+=(CgrDev.CgrVol.ImageB[BcMaxPos+6])-(CgrDev.CgrVol.ImageB[BcMinPos]);
    CVol[1]+=(CgrDev.CgrVol.ImageB[BcMaxPos+7])-(CgrDev.CgrVol.ImageB[BcMinPos]);
    //--------------------//
    for(int index=0;index<2;index++)
    {
        double Xt=TVol[index];
        double Xc=CVol[index];
        qDebug("[XT%d]=[%6.2f],[XC%d]=[%6.2f] \r\n" ,index,Xt,index,Xc);
        double Yt=0;
        double Yc=0;
        double A=CgrCode.XiangMuPm[index].canshua.toDouble();
        double B=CgrCode.XiangMuPm[index].canshub.toDouble();
        double C=CgrCode.XiangMuPm[index].canshuc.toDouble();
        double D=CgrCode.XiangMuPm[index].canshud.toDouble();

        if(CgrCode.XiangMuPm[index].jisuanfangfa=="直线方程"
                || CgrCode.XiangMuPm[index].jisuanfangfa=="直线")
        {
            Yt=A*Xt+B;
            Yc=A*Xc+B;
        }
        else if(CgrCode.XiangMuPm[index].jisuanfangfa=="二次方程"
                || CgrCode.XiangMuPm[index].jisuanfangfa=="二次")
        {
            Yt=A*Xt*Xt+B*Xt+C;
            Yc=A*Xc*Xc+B*Xc+C;
        }
        else if(CgrCode.XiangMuPm[index].jisuanfangfa=="三次方程"
                || CgrCode.XiangMuPm[index].jisuanfangfa=="三次")
        {
            Yt=A*Xt*Xt*Xt+B*Xt*Xt+C*Xt+D;
            Yc=A*Xc*Xc*Xc+B*Xc*Xc+C*Xc+D;
        }
        else if(CgrCode.XiangMuPm[index].jisuanfangfa=="四参数方程"
                || CgrCode.XiangMuPm[index].jisuanfangfa=="四参数")
        {
            if(C==0)
            {
                qDebug("CH%d 2Dcode  C of pm is error !",index);
            }
            else
            {
                Yt=(A-D)/(1+qPow(Xt/C,B))+D;
                Yc=(A-D)/(1+qPow(Xc/C,B))+D;
            }
        }
        else
        {
           qDebug("CH%d 2Dcode  funcation is error !",index);
        }
        //浓度
        if(Yt<0)
        {
            Yt=0;
        }
        if(Yc<0)
        {
            Yc=0;
        }
        TVol[index]=Yt;
        CVol[index]=Yc;
        qDebug("[YT%d]=[%6.2f],[YC%d]=[%6.2f] \r\n" ,index,Yt,index,Yc);
    }
    //Hp
    CgrDev.CgrVol.HbtVol=TVol[0];//Hb-A
    CgrDev.CgrVol.HbcVol=CVol[0];
    //Hb
    CgrDev.CgrVol.HptVol=TVol[1];//Hp-B
    CgrDev.CgrVol.HpcVol=CVol[1];
    return true;
}

//bool APP_dev::cgr_calc0(CgrDevStr &CgrDev)
//{
//    if(AppSta!=CGR_DEV)
//    {
//        return false;
//    }
//    qDebug("Ysize=%d,Xsize=%d,",CgrDev.ImgPosPm.Ysize,CgrDev.ImgPosPm.Xsize);
//    qDebug()<<"cgr_calc = 1";
//    //数据分布情况：0T0|0C0
//    //CgrDev.ImgPosPm.Ysize=30;
//    //CgrDev.ImgPosPm.Xsize=300;
//    //求均值图像:对40副图片的30*300个像素点进行均值
//    double ImgA[40][300];
//    double ImgB[40][300];
//    memset((char*)ImgA,0,40*300*sizeof(double));
//    memset((char*)ImgB,0,40*300*sizeof(double));
//    for(int index=0;index<CgrDev.ImgPosPm.Ysize;index++)
//    {
//        for(int indey=0;indey<CgrDev.ImgPosPm.Xsize;indey++)
//        {
//            //求一个均值像素点
//            int TempA[40];
//            int TempB[40];
//            for(int i=0;i<40;i++)
//            {
//                TempA[i]=ImageA[i][index][indey];
//                TempB[i]=ImageB[i][index][indey];
//            }
//            for(int i=0;i<40;i++)
//            {
//                for(int j=0;j<40;j++)
//                {
//                    if(TempA[j]>TempA[i])
//                    {
//                        int temp=TempA[i];
//                        TempA[i]=TempA[j];
//                        TempA[j]=temp;
//                    }
//                    if(TempB[j]>TempB[i])
//                    {
//                        int temp=TempB[i];
//                        TempB[i]=TempB[j];
//                        TempB[j]=temp;
//                    }
//                }
//            }
//            for(int i=10;i<30;i++)
//            {
//                ImgA[index][indey] +=(double)TempA[i];
//                ImgB[index][indey] +=(double)TempB[i];
//            }
//            ImgA[index][indey] /=20.0;
//            ImgB[index][indey] /=20.0;
//        }
//    }
//    qDebug()<<"cgr_calc = 2";
//    //get dImageA and dImageB
////    double dImageA[40][300];
////    double dImageB[40][300];
////    memset((char*)dImageA,0,40*300*sizeof(double));
////    memset((char*)dImageB,0,40*300*sizeof(double));
////    for(int index=0;index<CgrDev.ImgPosPm.Ysize;index++)
////    {
////        for(int indey=CgrDev.ImgPosPm.Xsize-1;indey>0;indey--)
////        {
////            dImageA[index][indey] =(ImgA[index][indey]-ImgA[index][indey-1]);
////            dImageB[index][indey] =(ImgB[index][indey]-ImgB[index][indey-1]);
////        }
////    }
////    //均值dy
////    for(int indey=0;indey<CgrDev.ImgPosPm.Xsize;indey++)
////    {
////        //a
////        CgrDev.CgrVol.dImagA[indey]=0;
////        for(int index=0;index<CgrDev.ImgPosPm.Ysize;index++)
////        {
////            CgrDev.CgrVol.dImagA[indey] +=dImageA[index][indey];
////        }
////        CgrDev.CgrVol.dImagA[indey] /=(CgrDev.ImgPosPm.Ysize);
////        //b
////        CgrDev.CgrVol.dImagB[indey]=0;
////        for(int index=0;index<CgrDev.ImgPosPm.Ysize;index++)
////        {
////            CgrDev.CgrVol.dImagB[indey] +=dImageB[index][indey];
////        }
////        CgrDev.CgrVol.dImagB[indey] /=(CgrDev.ImgPosPm.Ysize);
////    }
//    //均值线
//    for(int indey=0;indey<CgrDev.ImgPosPm.Xsize;indey++)
//    {
//        //a
//        CgrDev.CgrVol.ImageA[indey]=0;
//        for(int index=10;index<20;index++)
//        {
//            CgrDev.CgrVol.ImageA[indey] +=ImgA[index][indey];
//        }
//        CgrDev.CgrVol.ImageA[indey] /=(10);
//        //b
//        CgrDev.CgrVol.ImageB[indey]=0;
//        for(int index=10;index<20;index++)
//        {
//            CgrDev.CgrVol.ImageB[indey] +=ImgB[index][indey];
//        }
//        CgrDev.CgrVol.ImageB[indey] /=(10);
//    }
//    qDebug()<<"cgr_calc = 3";
//     //均值dy
////    double dImageA[300];
////    double dImageB[300];
////    memset((char*)dImageA,0,300*sizeof(double));
////    memset((char*)dImageB,0,300*sizeof(double));
////    for(int indey=299;indey>0;indey--)
////    {
////        dImageA[indey] =(CgrDev.CgrVol.ImageA[indey]-CgrDev.CgrVol.ImageA[indey-1]);
////        dImageB[indey] =(CgrDev.CgrVol.ImageB[indey]-CgrDev.CgrVol.ImageB[indey-1]);
////        CgrDev.CgrVol.dImagA[indey] =dImageA[indey];
////        CgrDev.CgrVol.dImagB[indey] =dImageB[indey];
////    }
////    for(int indey=0;indey<299;indey++)
////    {
////        dImageA[indey] =(CgrDev.CgrVol.ImageA[indey+1]-CgrDev.CgrVol.ImageA[indey]);
////        dImageB[indey] =(CgrDev.CgrVol.ImageB[indey+1]-CgrDev.CgrVol.ImageB[indey]);
////        CgrDev.CgrVol.dImagA[indey] =dImageA[indey];
////        CgrDev.CgrVol.dImagB[indey] =dImageB[indey];
////    }
//    //get C,T [peakpos,peakvol]
////    int ACpeakpos[2];
////    int BCpeakpos[2];
////    int ATpeakpos[2];
////    int BTpeakpos[2];
////    double damax=0;
////    double damin=0;
////    double dbmax=0;
////    double dbmin=0;
////    //T
////    damax=0;
////    damin=0;
////    dbmax=0;
////    dbmin=0;
////    for(int indey= 60;indey<140;indey++)
////    {
////        if(CgrDev.CgrVol.dImagA[indey]<damin)//A
////        {
////            damin=CgrDev.CgrVol.dImagA[indey];
////            ATpeakpos[0]=indey;
////        }
////        if(CgrDev.CgrVol.dImagA[indey]>damax)
////        {
////            damax=CgrDev.CgrVol.dImagA[indey];
////            ATpeakpos[1]=indey;
////        }
////        if(CgrDev.CgrVol.dImagB[indey]<dbmin)//B
////        {
////            dbmin=CgrDev.CgrVol.dImagB[indey];
////            BTpeakpos[0]=indey;
////        }
////        if(CgrDev.CgrVol.dImagB[indey]>dbmax)
////        {
////            dbmax=CgrDev.CgrVol.dImagB[indey];
////            BTpeakpos[1]=indey;
////        }
////    }
////    //C
////    damax=0;
////    damin=0;
////    dbmax=0;
////    dbmin=0;
////    for(int indey= 160;indey<280;indey++)
////    {
////        if(CgrDev.CgrVol.dImagA[indey]<damin)//A
////        {
////            damin=CgrDev.CgrVol.dImagA[indey];
////            ACpeakpos[0]=indey;
////        }
////        if(CgrDev.CgrVol.dImagA[indey]>damax)
////        {
////            damax=CgrDev.CgrVol.dImagA[indey];
////            ACpeakpos[1]=indey;
////        }
////        if(CgrDev.CgrVol.dImagB[indey]<dbmin)//B
////        {
////            dbmin=CgrDev.CgrVol.dImagB[indey];
////            BCpeakpos[0]=indey;
////        }
////        if(CgrDev.CgrVol.dImagB[indey]>dbmax)
////        {
////            dbmax=CgrDev.CgrVol.dImagB[indey];
////            BCpeakpos[1]=indey;
////        }
////    }
//    qDebug()<<"cgr_calc = 6";
//    //计算 C and T 面积和峰值
////    double Acsvol=0;
////    double Atsvol=0;
////    double Bcsvol=0;
////    double Btsvol=0;
//    double Acpvol=0;
//    double Atpvol=0;
//    double Bcpvol=0;
//    double Btpvol=0;

////    for(int indey=ACpeakpos[0];indey<ACpeakpos[1];indey++)
////    {
////        Acsvol += CgrDev.CgrVol.dImagA[indey];
////    }
////    for(int indey=ATpeakpos[0];indey<ATpeakpos[1];indey++)
////    {
////        Atsvol += CgrDev.CgrVol.dImagA[indey];
////    }
////    for(int indey=BCpeakpos[0];indey<BCpeakpos[1];indey++)
////    {
////        Bcsvol += CgrDev.CgrVol.dImagB[indey];
////    }
////    for(int indey=BTpeakpos[0];indey<BTpeakpos[1];indey++)
////    {
////        Btsvol += CgrDev.CgrVol.dImagB[indey];
////    }
////    qDebug("Acsvol=%f,Atsvol=%f,Acpvol=%f,Atpvol=%f",Acsvol,Atsvol,Acpvol,Atpvol);
////    qDebug("Bcsvol=%f,Btsvol=%f,Bcpvol=%f,Btpvol=%f",Bcsvol,Btsvol,Bcpvol,Btpvol);
////    qDebug()<<"cgr_calc = 7";
////    double x=0;
////    double y=0;
//    //Hp
//    //x=Atsvol;
//    //y=1.45523846550667*qPow(10,-7)*x*x*x-0.0006378668*x*x+1.1350981392*x-17.5175236697;
////    CgrDev.CgrVol.ImageACsVol=Acsvol;//Hp
////    CgrDev.CgrVol.ImageATsVol=Atsvol;
//    CgrDev.CgrVol.ImageACpVol=Acpvol;//Hp
//    CgrDev.CgrVol.ImageATpVol=Atpvol;
//    //Hb
//    //x=Btsvol;
//    //y=-0.0001361796*x*x+0.6299761052*x+7.2718997588;
////    CgrDev.CgrVol.ImageBCsVol=Bcsvol;//Hb
////    CgrDev.CgrVol.ImageBTsVol=Btsvol;
//    CgrDev.CgrVol.ImageBCpVol=Bcpvol;//Hb
//    CgrDev.CgrVol.ImageBTpVol=Btpvol;
//    //C线的中线，T线的左右分界线
////    CgrDev.CgrVol.ImageAcPos=0;
////    CgrDev.CgrVol.ImageBcPos=0;
////    CgrDev.CgrVol.ImageAtPosL=0;
////    CgrDev.CgrVol.ImageBtPosL=0;
////    CgrDev.CgrVol.ImageAtPosR=0;
////    CgrDev.CgrVol.ImageBtPosR=0;
////    CgrDev.CgrVol.ImageAtPosL +=ATpeakpos[0];
////    CgrDev.CgrVol.ImageAtPosR +=ATpeakpos[2];
////    CgrDev.CgrVol.ImageBtPosL +=BTpeakpos[0];
////    CgrDev.CgrVol.ImageBtPosR +=BTpeakpos[2];
////    qDebug()<<"cgr_calc = 8";
//    return true;
//}

QString APP_dev::cgr_get_one()
{
    if(UsbCamera<0)
    {
        return "No";
    }
    QImage Image;
    UsbCamera->GetBuffer(PixImgBuf);
    Image.loadFromData(PixImgBuf,UsbCamera->MjpgSize,"JPG");
    //Image= QImage((uchar *) PixImgBuf, 640, 480, QImage::Format_RGB888);//Format_RGB32 Format_RGB888
    while(Image.isNull())
    {
        Delayms(50);
        UsbCamera->GetBuffer(PixImgBuf);
        Image.loadFromData(PixImgBuf,UsbCamera->MjpgSize,"JPG");
    }
    QZXing zxing;
    qDebug()<<"get png!";
    int Xpos=280;
    int Ypos=260;
    int Xsize=730;
    int Ysize=30;
    QImage ImageCode=Image.copy(Xpos,Ypos,Xsize,Ysize);
    qDebug()<<"decoder!";
    QString Txt=zxing.decodeImage(ImageCode);
    qDebug()<<"Code="<<Txt;
    if(Txt.length()<=8)
    {
        return Txt;
    }
    else
    {
        return "No";
    }
}

bool APP_dev::cgr_start()
{
    return true;
}

void APP_dev::cgr_work()
{
    return;
}

void APP_dev::cgr_com()
{
    return;
}

double APP_dev::Log10(double x)
{
    return qLn(x)/qLn(10);
}

void APP_dev::FmqON()
{
    if(LedDev<0)
    {
        return;
    }
    ioctl(LedDev, 1, 2);//ON
    return;
}

void APP_dev::FmqOFF()
{
    if(LedDev<0)
    {
        return;
    }
    ioctl(LedDev, 0, 2);//OFF
    return;
}

void APP_dev::LedON()
{
    if(LedDev<0)
    {
        return;
    }
    ioctl(LedDev, 1, 1);//ON
    return;
}

void APP_dev::LedOFF()
{
    if(LedDev<0)
    {
        return;
    }
    ioctl(LedDev, 0, 1);//OFF
    return;
}

void APP_dev::Delayms(uint del)
{
    QTime ReachTime = QTime::currentTime().addMSecs(del);
    while(QTime::currentTime()<ReachTime)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents,100);
    }
}

void APP_dev::run()
{
    //-----------------------------------//
    if(AppSta==NOP_DEV)
    {
        qDebug()<<"app device is stop !";
        return;
    }
    if(AppSta==AGL_DEV)
    {
        qDebug()<<"agl device is start !";
    }
    if(AppSta==CGR_DEV)
    {
        qDebug()<<"cgr device is start !";
    }
    //-----------------------------------//
}

u_int16_t APP_dev::GetCRC(char *pchMsg, u_int16_t wDataLen)
{
    const uchar chCRCHTalbe[] =                                 // CRC 高位字节值表
    {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40
    };

    const uchar chCRCLTalbe[] =                                 // CRC 低位字节值表
    {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
    0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
    0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
    0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
    0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
    0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
    0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
    0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
    0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
    0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
    0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
    0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
    0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
    0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
    0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
    0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
    0x41, 0x81, 0x80, 0x40
    };
    uchar chCRCHi = 0xFF; // 高CRC字节初始化
    uchar chCRCLo = 0xFF; // 低CRC字节初始化
    u_int16_t wIndex=0;     // CRC循环中的索引
    while (wDataLen>0)
    {
            wDataLen--;
            // 计算CRC
            wIndex = chCRCLo ^ *pchMsg++ ;
            chCRCLo = chCRCHi ^ chCRCHTalbe[wIndex];
            chCRCHi = chCRCLTalbe[wIndex] ;
    }
    u_int16_t CRC16= ((chCRCHi << 8) | chCRCLo);
    return  CRC16;
}

bool APP_dev::SetPinIOSta(uchar Pin, uchar Sta)
{
    //Pin =0~15,Stat=0,1
    if(GpioDev<0)
    {
        qDebug()<<"Set pin state is error!";
        return false;
    }
    ioctl(GpioDev, Pin,Sta);//1=output,0=input
    return true;
}

bool APP_dev::SetPinState(uchar Pin, uchar Sta)
{
    if(GpioDev<0)
    {
        return false;
    }
    uchar val[2];
    ioctl(GpioDev, Pin, 1);//output
    val[0]=Pin;
    if(Sta==0)
    {
        val[1]=0;
    }
    else
    {
        val[1]=1;
    }
    write(GpioDev, val, 2);
    return true;
}


uchar APP_dev::GetPinState(uchar Pin)
{
    if (GpioDev < 0)
   {
           qDebug("GpioDev open is error !");
           return 2;
    }
    //Pin =0~15,Stat=0,1
    unsigned char dat[2];
    read(GpioDev, dat, 2);
    Delayms(10);
    read(GpioDev, dat, 2);
    u_int16_t PinSate = (u_int16_t)dat[0]*255+(u_int16_t)dat[1];
    if(PinSate  & (0x0001<<Pin))
    {
        #if WY_Debug > 0
        qDebug()<<"Pin state is 1 !";
        #endif
        return 1;
    }
    else
    {
        #if WY_Debug > 0
        qDebug()<<"Pin state is 0 !";
        #endif
        return 0;
    }

}

bool APP_dev::SetCom3State(Com3Xstr Com3X)
{
    if(LedDev<0)
    {
        qDebug()<<"Comx swich is error";
        return false;
    }
    if(Com3X==Com1D)
    {
        ioctl(LedDev, 0, 3);//INH
        ioctl(LedDev, 0, 4);//A
        ioctl(LedDev, 0, 5);//B
    }
    if(Com3X==Com2D)
    {
        ioctl(LedDev, 0, 3);//INH
        ioctl(LedDev, 1, 4);//A
        ioctl(LedDev, 0, 5);//B
    }
    if(Com3X==ComPrinter)
    {
        ioctl(LedDev, 0, 3);//INH
        ioctl(LedDev, 0, 4);//A
        ioctl(LedDev, 1, 5);//B
    }
    if(Com3X==ComExt)
    {
        ioctl(LedDev, 0, 3);//INH
        ioctl(LedDev, 1, 4);//A
        ioctl(LedDev, 1, 5);//B
    }
    return true;
}

void APP_dev::setTermios(termios *pNewtio, unsigned short uBaudRate)
{
    bzero(pNewtio,sizeof(struct termios));
    pNewtio->c_cflag = uBaudRate|CS8|CREAD|CLOCAL;
    pNewtio->c_iflag = IGNPAR;
    pNewtio->c_oflag = 0;
    pNewtio->c_lflag = 0;
    pNewtio->c_cc[VINTR] = 0;
    pNewtio->c_cc[VQUIT] = 0;
    pNewtio->c_cc[VERASE] = 0;
    pNewtio->c_cc[VKILL] = 0;
    pNewtio->c_cc[VEOF] = 4;
    pNewtio->c_cc[VTIME] = 5;
    pNewtio->c_cc[VMIN] = 0;
    pNewtio->c_cc[VSWTC] = 0;
    pNewtio->c_cc[VSTART] = 0;
    pNewtio->c_cc[VSTOP] = 0;
    pNewtio->c_cc[VSUSP] = 0;
    pNewtio->c_cc[VEOL] = 0;
    pNewtio->c_cc[VREPRINT] = 0;
    pNewtio->c_cc[VDISCARD] = 0;
    pNewtio->c_cc[VWERASE] = 0;
    pNewtio->c_cc[VLNEXT] = 0;
    pNewtio->c_cc[VEOL2] = 0;
}

bool isConnectedToNetwork()
{
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    bool isConnected = false;

    for (int i = 0; i < ifaces.count(); i++)
    {
        QNetworkInterface iface = ifaces.at(i);
        if (       iface.flags().testFlag(QNetworkInterface::IsUp)
             && iface.flags().testFlag(QNetworkInterface::IsRunning)
             && (iface.flags().testFlag(QNetworkInterface::IsLoopBack)==false)
              )
        {
            // this loop is important
            for (int j=0; j<iface.addressEntries().count(); j++)
            {
                // we have an interface that is up, and has an ip address
                // therefore the link is present
                // we will only enable this check on first positive,
                // all later results are incorrect
                if (isConnected == false)
                {
                    isConnected = true;
                }
            }
        }
    }
    return isConnected;
}
void APP_dev::TimerOut()
{
    static bool ledflag=false;
    if(ledflag)
    {
        ledflag=false;
        LedOFF();
    }
    else
    {
        ledflag=true;
        LedON();
    }
    //-------------------------------//
    if (AdcDev > 0)
    {
        ioctl(AdcDev, CMD_BIT_SELECTED, 12);//设置12位数据
        ioctl(AdcDev, CMD_PORT_SELECTED, 0);//设置通道0
        int ibuffer;
        double dValue;
        char cData[10];
        int ret = read(AdcDev, &ibuffer, sizeof(ibuffer));//读ad转换后的数据
        if (ret > 0)//如果读出来的数据有效
        {
            dValue =((float) ibuffer * 3.3)/4095*11+1.1;//模数换算
            if(dValue>9.5)
            {
//                yx_power=2.00;
                yx_power=1.00;
            }
            if(dValue>8.0 && dValue<9.0)
            {
                yx_power=1.00;
            }
            if(dValue>7.6 && dValue<7.8)
            {
                yx_power=0.75;
            }
             if(dValue>7.2 && dValue<7.4)
            {
                yx_power=0.50;
            }
             if(dValue>6.8 && dValue<7.0)
            {
                yx_power=0.25;
            }
            sprintf(cData, "%.2f", dValue);
            QString AdcStr = QString(cData);
            AdcStr.append('V');
            //qDebug()<<AdcStr;
        }
    }
    //-------------------------------//
    bool state=isConnectedToNetwork();
    if(state!=yx_network)
    {
        yx_network=state;
        if(yx_network==false)
        {
            qDebug()<<"net disconnected";
        }
        else
        {
            qDebug()<<"net connected";
        }
    }
    //-------------------------------//
    static bool UdiskFlag=false;
    static QString UdiskName;
    if(UdiskFlag)
    {
        QFile UdiskDev(UdiskName);
        if(!UdiskDev.exists())
        {
            UdiskName.clear();
            qDebug()<<"remove udisk";
            UdiskFlag=false;
            SysCall("umount /yx-udisk");
            SysCall("rm -rf /yx-udisk");
            yx_udisk=false;
        }
        UdiskDev.close();
    }
    else
    {
        QString FileName;
        QFile UdiskDev;
        if(!UdiskFlag)
        {
            FileName="/dev/sd";
            char Dat='a';
            for(char indey=0;indey<26;indey++)
            {
                QString Name=FileName;
                Name.append(Dat+indey);
                for(char index=0;index<10;index++)
                {
                    UdiskName=Name+QString::number(index,10);
                    UdiskDev.setFileName(UdiskName);
                    if(UdiskDev.exists())
                    {
                        index=100;
                        indey=100;
                        UdiskFlag=true;
                    }
                }
            }
        }
        UdiskDev.close();
        if(UdiskFlag)
        {
            qDebug()<<"insert udisk";
            UdiskFlag=true;
            SysCall("umount /yx-udisk");
            Delayms(100);
            SysCall("rm -rf /yx-udisk");
            Delayms(100);
            SysCall("mkdir /yx-udisk");
            Delayms(100);
            QString Cmd="mount -t vfat -o rw " +UdiskName +" /yx-udisk";
            SysCall(Cmd);
            //SysCall("mount  /dev/sd*1 /yx-udisk");
            //SysCall("mount -t vfat -o iocharset=utf8 /dev/sd*1 /yx-udisk");
            yx_udisk=true;
        }
    }
    //-------------------------------//
    static bool SdiskFlag=false;
    QFile SdiskDev("/dev/mmcblk0p1");
    if(SdiskDev.exists())
    {
        if(!SdiskFlag)
        {
            qDebug()<<"insert sdisk";
            SdiskFlag=true;
            SysCall("umount /yx-sdisk");
            Delayms(100);
            SysCall("rm -rf /yx-sdisk");
            Delayms(100);
            SysCall("mkdir /yx-sdisk");
            Delayms(100);
            //SysCall("mount -t vfat -o iocharset=utf8 /dev/mmcblk0p1 /yx-sdisk");
            SysCall("mount -o rw /dev/mmcblk0p1 /yx-sdisk");
            yx_sdisk=true;
        }
    }
    else
    {
        if(SdiskFlag)
        {
            qDebug()<<"remove sdisk";
            SdiskFlag=false;
            SysCall("umount /yx-sdisk");
            SysCall("rm -rf /yx-sdisk");
            yx_sdisk=false;
        }
    }
    //-----------------------------------//
}

bool APP_dev::OpenLed()
{
    LedDev = open("/dev/led", QIODevice::ReadWrite);//打开设备
    if (LedDev < 0)
    {
        printf("led is error !\n");
        //QMessageBox::warning(this, trUtf8("提示:"),trUtf8("LED打开失败！"), QMessageBox::Ok);
        return false;
    }
    printf("led is ok !\n");
    ioctl(LedDev, 1, 3);//INH
    ioctl(LedDev, 0, 4);//A
    ioctl(LedDev, 0, 5);//B
    return true;
}

bool APP_dev::CloseLed()
{
    if (LedDev < 0)
   {
         close(LedDev);
         return true;
    }
    ioctl(LedDev, 1, 3);//INH
    ioctl(LedDev, 0, 4);//A
    ioctl(LedDev, 0, 5);//B
    close(LedDev);
    return true;
}

bool APP_dev::OpenGpio()
{
    GpioDev = open("/dev/yx_gpio", O_RDWR);
    if (GpioDev < 0)
   {
           qDebug("gpio is error !");
           return false;
    }
    qDebug("gpio is ok!");
    SetPinIOSta(Scan1D_RstPin,1);
    SetPinIOSta(Scan2D_RstPin,1);
    SetPinIOSta(Scan1D_TrgPin,1);
    SetPinIOSta(Scan2D_TrgPin,1);

    SetPinState(Scan1D_TrgPin,1);
    SetPinState(Scan2D_TrgPin,1);
    SetPinState(Scan1D_RstPin,0);
    SetPinState(Scan2D_RstPin,0);
    Delayms(10);
    SetPinState(Scan1D_RstPin,1);
    SetPinState(Scan2D_RstPin,1);
    Delayms(10);
    if(AppSta==CGR_DEV)
    {
        SetPinIOSta(CardInsert_Pin,0);
        Delayms(10);
    }
    return true;
}

bool APP_dev::CloseGpio()
{
    close(GpioDev);
    return true;
}

bool APP_dev::OpenAdc()
{
    AdcDev = open("/dev/adc", O_RDWR);
    if(AdcDev <0 )
    {
        qDebug()<<"adc is error !";
        return false;
    }
    qDebug()<<"adc is Ok !";
    ioctl(AdcDev, CMD_BIT_SELECTED, 12);//设置12位数据
    ioctl(AdcDev, CMD_PORT_SELECTED, 0);//设置通道0
//        int ibuffer;
//        double dValue;
//        int ret = read(AdcDev, &ibuffer, sizeof(ibuffer));//读ad转换后的数据
//        if (ret > 0)//如果读出来的数据有效
//        {
//            dValue =((float) ibuffer * 3.3)/4095;//模数换算
//        }
    return true;
}

bool APP_dev::CloseAdc()
{
    close(AdcDev);
    return true;
}

bool APP_dev::OpenTem()
{
    TemDev = open("/dev/ds18b20", O_RDWR);
    if (TemDev < 0)
    {
        qDebug()<<"tem  is error !";
        return false;
    }
    qDebug()<<"tem is ok!";
    return true;
}

bool APP_dev::CloseTem()
{
    close(TemDev);
    return true;
}


bool APP_dev::OpenCom()
{
    SetCom3State(ComExt);
    Delayms(10);

#ifdef WY_COM
    ComPort1 = new ComStr();
    memset(ComPort1,0,sizeof(ComStr));
    sprintf(ComPort1->Dev,"/dev/ttySAC1"); // Linux一般串口名 /dev/ttyS0 一般USB名/dev/ttyUSB0

/*    O_RDWR 已读写方式打开
 *    标志O_NOCTTY可以告诉UNIX这个程序不会成为这个端口上的“控制终端”。如果不这样做的话，所有的输入，
 *    比如键盘上过来的Ctrl+C中止信号等等，会影响到你的进程。而有些程序比如getty(1M/8)则会在打开登录进程的时候使用这个特性，
 *    但是通常情况下，用户程序不会使用这个行为。
 *    O_NDELAY标志则是告诉UNIX，这个程序并不关心DCD信号线的状态——也就是不关心端口另一端是否已经连接。
 *    如果不指定这个标志的话，除非DCD信号线上有space电压否则这个程序会一直睡眠。
 */
     if((ComPort1->Port=open(ComPort1->Dev,O_RDWR|O_NOCTTY|O_NDELAY))<0)
     {
        printf("com1 open error!\r\n");
        ComPort1->State=false;
     }
     else
     {
         tcgetattr(ComPort1->Port,&ComPort1->oldtio);  // 获取与终端相关的参数，入参1为文件描述符，如参2 是termios 结构体，用于存放相关参数的结果
         setTermios(&ComPort1->newtio,B115200);          // 设置输入模式
         tcflush(ComPort1->Port,TCIFLUSH);                       // 清空终端未完成的输入/输出请求及数据。
         tcsetattr(ComPort1->Port,TCSANOW,&ComPort1->newtio);
         printf("com1 open ok!\r\n");
         ComPort1->State=true;
     }

     ComPort2 =new ComStr();
     memset(ComPort2,0,sizeof(ComStr));
     sprintf(ComPort2->Dev,"/dev/ttySAC2");
      if((ComPort2->Port=open(ComPort2->Dev,O_RDWR|O_NOCTTY|O_NDELAY))<0)
      {
         printf("com2 open error!\r\n");
         ComPort2->State=false;
      }
      else
      {
          tcgetattr(ComPort2->Port,&ComPort2->oldtio);
          setTermios(&ComPort2->newtio,B115200);
          tcflush(ComPort2->Port,TCIFLUSH);
          tcsetattr(ComPort2->Port,TCSANOW,&ComPort2->newtio);
          printf("com2 open ok!\r\n");
          ComPort2->State=true;
          QTimer *Com2Timer=new QTimer();
          connect(Com2Timer,SIGNAL(timeout()),this,SLOT(Com2Read()));//连接槽
          Com2Timer->setInterval(100);
          Com2Timer->start();
      }

      ComPort3 = new ComStr();
      memset(ComPort3,0,sizeof(ComStr));
      sprintf(ComPort3->Dev,"/dev/ttySAC3");
       if((ComPort3->Port=open(ComPort3->Dev,O_RDWR|O_NOCTTY|O_NDELAY))<0)
       {
          printf("com3 open error!\r\n");
          ComPort3->State=false;
       }
       else
       {
           tcgetattr(ComPort3->Port,&ComPort3->oldtio);
           setTermios(&ComPort3->newtio,B9600);
           tcflush(ComPort3->Port,TCIFLUSH);
           tcsetattr(ComPort3->Port,TCSANOW,&ComPort3->newtio);
           printf("com3 open ok!\r\n");
           ComPort3->State=true;
       }
//----------------------------------------Add-----------------for test-----------------------------------//
//       ComPortTest = new ComStr();
//       memset(ComPortTest,0,sizeof(ComStr));
//       sprintf(ComPortTest->Dev,"/dev/ttySAC0");
//        if((ComPortTest->Port=open(ComPortTest->Dev,O_RDWR|O_NOCTTY|O_NDELAY))<0)
//        {
//           printf("ComPortTest open error!\r\n");
//           ComPortTest->State=false;
//        }
//        else
//        {
//            tcgetattr(ComPortTest->Port,&ComPortTest->oldtio);
//            setTermios(&ComPortTest->newtio,B115200);
//            tcflush(ComPortTest->Port,TCIFLUSH);
//            tcsetattr(ComPortTest->Port,TCSANOW,&ComPortTest->newtio);
//            printf("ComPortTest open ok!\r\n");
//            ComPortTest->State=true;
//        }
//------------------------------------------------------------------------------------------------------------//
      return (ComPort1->State && ComPort2->State&&ComPort3->State);
#else
    bool State1=false;
    ComPort1 = new QSerialPort("/dev/ttySAC1");
    if(ComPort1->open(QIODevice::ReadWrite))
    {
        ComPort1->setBaudRate(QSerialPort::Baud115200);//默认MCU=115200
        ComPort1->setParity(QSerialPort::NoParity);//无奇偶校验
        ComPort1->setDataBits(QSerialPort::Data8); //数据位
        ComPort1->setStopBits(QSerialPort::OneStop);//1停止位
        ComPort1->setFlowControl(QSerialPort::NoFlowControl);//无控制
        ComPort1->setReadBufferSize(2048);
        ComPort1->readAll();
        //connect(ComPort1,SIGNAL(readyRead()),this,SLOT(Com1Read()));//连接槽
        ComPort1->write("com1 is Ok !\r\n");
        qDebug()<<"com1 is Ok !";
        State1=true;
    }
    else
    {
        qDebug()<<"Com1 open is error !";
    }
    Delayms(10);
    bool State2=false;
    ComPort2 = new QSerialPort("/dev/ttySAC2");
    if(ComPort2->open(QIODevice::ReadWrite))
    {
        ComPort2->setBaudRate(QSerialPort::Baud115200);//默认=115200
        ComPort2->setParity(QSerialPort::NoParity);//无奇偶校验
        ComPort2->setDataBits(QSerialPort::Data8); //数据位
        ComPort2->setStopBits(QSerialPort::OneStop);//1停止位
        ComPort2->setFlowControl(QSerialPort::NoFlowControl);//无控制
        ComPort2->setReadBufferSize(2048);
        ComPort2->readAll();
        connect(ComPort2,SIGNAL(readyRead()),this,SLOT(Com2Read()));//连接槽
        qDebug()<<"Com2 is Ok !";
        State2=true;
    }
    else
    {
        qDebug()<<"Com2 open is error !";
    }
    Delayms(10);
    bool State3=false;
    ComPort3 = new QSerialPort("/dev/ttySAC3");
    if(ComPort3->open(QIODevice::ReadWrite))
    {
        ComPort3->setBaudRate(QSerialPort::Baud9600);//默认Scan2D=9600
        ComPort3->setParity(QSerialPort::NoParity);//无奇偶校验
        ComPort3->setDataBits(QSerialPort::Data8); //数据位
        ComPort3->setStopBits(QSerialPort::OneStop);//1停止位
        ComPort3->setFlowControl(QSerialPort::NoFlowControl);//无控制
        ComPort3->setReadBufferSize(2048);
        ComPort3->readAll();
        //connect(ComPort3,SIGNAL(readyRead()),this,SLOT(Com3Read()));//连接槽
        ComPort3->write("com3 is Ok !\r\n");
        qDebug()<<"com3 is Ok !";
        State3=true;
    }
    else
    {
        qDebug()<<"Com3 open is error !";
    }
    return (State1 && State2 && State3);
#endif
}

bool APP_dev::CloseCom()
{
#ifdef WY_COM
    ComPort1->State=false;
    tcsetattr(ComPort1->Port,TCSANOW,&ComPort1->oldtio);
    close(ComPort1->Port);
    ComPort2->State=false;
    tcsetattr(ComPort2->Port,TCSANOW,&ComPort2->oldtio);
    close(ComPort2->Port);
    ComPort3->State=false;
    tcsetattr(ComPort3->Port,TCSANOW,&ComPort3->oldtio);
    close(ComPort3->Port);
    return true;
#else
    ComPort1->close();
    ComPort2->close();
    ComPort3->close();
    ComPort1->deleteLater();
    ComPort2->deleteLater();
    ComPort3->deleteLater();
    return true;
#endif
}
/*
 *(1k+2K+1K)*3+5k+3K=20k
 * (3k+2K+1K)+1K=7k
 *(0.5+0.5)*2=2k
 * (5k+5k)=10K
 *
 *
 * 20k+10k=30k,
 *
 * 7.3k-1.8=5.5k
 * 4*1.5=6k
 * 13.3*
 *
*/

bool APP_dev::OpenCamera()
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    bool state=false;
    CameraPmStr CameraPm =CgrDev.CameraPm;
    UsbCamera = new QUsbCamera(CAMERA_CMOS, "/dev/video0", 0);
    if (UsbCamera)
    {
#ifdef  VideoFormartRGB
        UsbCamera->SetImageSize(640, 480);
#endif
#ifdef  VideoFormartJPG
        UsbCamera->SetImageSize(1280, 720);
#endif
        if (UsbCamera->OpenDevice())
        {
            Delayms(1000);
#ifdef  VideoFormartRGB
            UsbCamera->SetFocusingMenu(true);//手动对焦
            Delayms(1000);
            UsbCamera->SetExposureMenu(true);//手动曝光
            Delayms(1000);
            UsbCamera->SetWhiteBalanceMenu(true);//手动白平衡
            Delayms(1000);
            UsbCamera->SetGain(CameraPm.Agc); //增益
            Delayms(1000);
            UsbCamera->SetExposure(CameraPm.Exp); //曝光
            Delayms(20);
            UsbCamera->SetWhiteBalance(CameraPm.WhiteBalance); //白平衡
            Delayms(20);
            UsbCamera->SetBrightness(CameraPm.Brightness);  //亮度
            Delayms(20);
            UsbCamera->SetContrast(CameraPm.Contrast); //对比度
            Delayms(20);
            UsbCamera->SetSaturation (CameraPm.Saturation); //饱和度
            Delayms(20);
            UsbCamera->SetHue(CameraPm.Hue); //色调
            Delayms(20);
            UsbCamera->SetSharpness(CameraPm.Sharpness); //清晰度
            Delayms(20);
            UsbCamera->SetGamma(CameraPm.Gamma); //伽玛
            Delayms(20);
            UsbCamera->SetFocusing(CameraPm.Focus); //焦点
            Delayms(20);
            UsbCamera->GetBuffer(PixImgBuf);//get camera image
#endif
#ifdef  VideoFormartJPG0
            UsbCamera->SetFocusingMenu(true);//手动对焦
            Delayms(1000);
            UsbCamera->SetExposureMenu(true);//手动曝光
            Delayms(1000);
            UsbCamera->SetWhiteBalanceMenu(true);//手动白平衡
            Delayms(1000);
            UsbCamera->SetGain(CameraPm.Agc); //增益
            Delayms(1000);
            UsbCamera->SetExposure(CameraPm.Exp); //曝光
            Delayms(20);
            UsbCamera->SetWhiteBalance(CameraPm.WhiteBalance); //白平衡
            Delayms(20);
            UsbCamera->SetBrightness(CameraPm.Brightness);  //亮度
            Delayms(20);
            UsbCamera->SetContrast(CameraPm.Contrast); //对比度
            Delayms(20);
            UsbCamera->SetSaturation (CameraPm.Saturation); //饱和度
            Delayms(20);
            UsbCamera->SetHue(CameraPm.Hue); //色调
            Delayms(20);
            UsbCamera->SetSharpness(CameraPm.Sharpness); //清晰度
            Delayms(20);
            UsbCamera->SetGamma(CameraPm.Gamma); //伽玛
            Delayms(20);
            UsbCamera->SetFocusing(CameraPm.Focus); //焦点
            Delayms(20);
            UsbCamera->GetBuffer(PixImgBuf);//get camera image
#endif
            state = true;
            qDebug()<<"camera open is ok!";
        }
        else
        {
            state = false;
            qDebug()<<"camera open is error!";
        }
    }
    else
    {
        state = false;
        qDebug()<<"camera malloc is error!";
    }
    if(!state)
    {
        UsbCamera->CloseDevice();
        delete UsbCamera;
        //QMessageBox::warning(MainWindowP, trUtf8("提示:"),trUtf8("摄像头打开失败！"), QMessageBox::Ok);
    }
    return state;
}

bool APP_dev::CloseCamera()
{
    if(AppSta!=CGR_DEV)
    {
        return false;
    }
    UsbCamera->CloseDevice();
    delete UsbCamera;
    return true;
}

bool APP_dev::OpenDevice()
{
    if(AppSta==NOP_DEV)
    {
        qDebug("App state is error!");
        return false;
    }
    cgr_load_pm();
    agl_Load_pm();
    OpenGpio();
    OpenLed();
    OpenAdc();
    //OpenTem();
    OpenCom();
    if(AppSta==CGR_DEV)
    {
        OpenCamera();
        cgr_rd_led_stat();
        cgr_rd_led_pm();
        cgr_led_on();
        cgr_open();
    }
    if(AppSta==AGL_DEV)
    {
//        agl_rd_stat();
//        agl_rd_pm();
    }
    OpenNetwork();
    FmqON();
    Delayms(300);
    FmqOFF();
    return true;
}

bool APP_dev::CloseDevice()
{
    if(AppSta==CGR_DEV)
    {
        this->CloseCamera();
    }
    this->CloseCom();
    this->CloseAdc();
    this->CloseTem();
    this->CloseLed();
    this->CloseGpio();
    return true;
}

void APP_dev::Com1Read()
{
//    qDebug()<<ComPort1->readAll();
}

void APP_dev::Com2Read()
{
    if(AppSta==AGL_DEV)
    {
        agl_com();
    }
    else if(AppSta==CGR_DEV)
    {
        cgr_com();
    }
}

void APP_dev::Com3Read()
{
//    qDebug()<<ComPort3->readAll();
}

void APP_dev::OpenNetwork()
{
    return;
    //-------------------------------//
    //QHostAddress Address=GetLocalIPaddr();
    //qDebug()<<"QHostAddress="<<Address;
    //-------------------------------//
    //创建UDP服务器
    //UdpServerP = new QUdpSocket();
    //UdpServerP->bind(Address,UdpServerPort);
    //connect(UdpServerP, SIGNAL(readyRead()),this, SLOT(ReadyReadUdpServer()));
    //-------------------------------//
    //创建TCP服务器
    NetServerP = new QTcpServer(this);
    if(NetServerP)
    {
        qDebug()<<"NetServerP is OK!";
        if(NetServerP->listen(QHostAddress::Any,TcpServerPort))
        {
            qDebug()<<"NetServerP listen is ok!";
            connect(NetServerP,SIGNAL(newConnection()),this,SLOT(newConnect()));
        }
        else
        {
            qDebug()<<"NetServerP listen is not ok!";
        }
    }
    //-------------------------------//
    //TestNetWork();
    //TcpClientP;
    //-------------------------------//
    //UdpClientP;
    //UdpClientP = new QUdpSocket(this);
    //UdpClientP->bind(UdpClientPort);
    //-------------------------------//
    QTimer * Timer2 = new QTimer();
    Timer2->setInterval(3000);
    connect(Timer2,SIGNAL(timeout()),this,SLOT(TimerOut2s()));//连接槽
    Timer2->start();
    //-------------------------------//
}

void APP_dev::TestNetWork()
{
    qDebug()<<"TestNetWork !";
    int port=30000 ;
    int listenfd = socket(AF_INET, SOCK_STREAM,0);
    //创建服务器端套接字描述符，用于监听客户端请求
    //struct hostent *hp;
    struct sockaddr_in serveraddr;
    bzero((char *)&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);
    //创建用于服务的服务器端套接字，注意与 客户端创建的套接字的区别  IP段里，这里是可以为任何IP提供服务的，客户端里的IP是请求的端点

    bind(listenfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));
    //将服务器端的套接字描述符和服务器端套接字相关联

    listen(listenfd,1024);
    //打开监听
    char temp[100];
    struct sockaddr_in clientaddr;
    int clientlen, connfd;
    int Dat=0;
    clientlen = sizeof(clientaddr);
    connfd = accept(listenfd,(struct sockaddr *)&clientaddr, (socklen_t *__restrict)&clientlen);
    //等待接受客户端的请求，这是一个阻塞函数。若成功接受客户端请求则返回新的套接字描述符，这个描述符用于 send和recv函数
    while(1)
    {

       Dat= recv(connfd,temp,100,0);
       if(Dat>0)
       {
            printf("%s\r\n",temp);
            send(connfd,temp,100,0);
            memset(temp,0,100);
       }
       msleep(1000);
       //Delayms(1000);
    }
    //recv接收客户端发送过来的字符串，send发送给客户端字符串
     //EXIT_SUCCESS;
}

void APP_dev::CloseNetWork()
{
    UdpClientP->disconnect();
    UdpClientP->close();
    UdpClientP->deleteLater();

    UdpServerP->disconnect();
    UdpServerP->close();
    UdpServerP->deleteLater();

    NetServerP->disconnect();
    NetServerP->close();
    NetServerP->deleteLater();

}

void APP_dev::TimerOut2s()
{
    if(AppSta==NOP_DEV)
    {
        return;
    }
    if(!yx_network)
    {
        if(UdpClientP!=NULL)
        {

            UdpClientP->disconnect();
            UdpClientP->close();
            UdpClientP->deleteLater();
            UdpClientP=NULL;
        }
        return;
    }
    //UdpClientP;
    if(UdpClientP==NULL)
    {
        QHostAddress Address=GetLocalIPaddr();
        UdpClientP = new QUdpSocket(this);
        UdpClientP->bind(Address,UdpClientPort);
    }
    QByteArray TxdDat;
    if(AppSta==AGL_DEV)
    {
        TxdDat.append("AGL;");
    }
    if(AppSta==CGR_DEV)
    {
        TxdDat.append("CGR;");
    }
    TxdDat += (AppCfg->SN+";");
    TxdDat += (AppCfg->LocalIp+";");
    TxdDat +=(QString::number(TcpServerPort,10)+";\r\n");
    //qDebug()<<"Txd"<<TxdDat;
    //UDP广播
    UdpClientP->writeDatagram(TxdDat.data(),TxdDat.size(),QHostAddress::Broadcast,UdpServerPort);
    //TCP
    if(NetServerP->isListening())
    {
        qDebug()<<"NetServerP isListening!";
    }
    else
    {
        qDebug()<<"NetServerP is not Listening!";
    }
    if(NetServerP->hasPendingConnections())
    {
        qDebug()<<"NetServerP have new connect!";
        TcpServerP = NetServerP->nextPendingConnection();
        connect(TcpServerP, SIGNAL(readyRead()),this, SLOT(ReadyReadTcpServer()));
        connect(TcpServerP, SIGNAL(disconnected()),this, SLOT(disconnected()));
    }
    else
    {
        qDebug()<<"NetServerP have not new connect!";
    }

}

//接收来自UDP服务端的数据
void APP_dev::ReadyReadUdpServer()
{
    qDebug()<<"UDP_Server";
    while (UdpServerP->hasPendingDatagrams())
    {
        QByteArray UdpServerDat;
        QHostAddress Address;
        quint16 Port;
        UdpServerDat.resize(UdpServerP->pendingDatagramSize());
        UdpServerP->readDatagram(UdpServerDat.data(), UdpServerDat.size(),&Address,&Port);
        QByteArray TxdDat;
        if(UdpServerDat.toStdString()=="[YiXinBIO]")
        {
            QString Str="[YiXinBIO][20161010-123456]";
            TxdDat.append(Str);
            TxdDat.append("["+GetLocalIP()+"-"+QString("%1").arg(TcpServerPort)+"]\r\n");
        }
        else
        {
            QString Str=("[YX-"+GetRtcTime()+"]\r\n");
            TxdDat.append(Str);
        }
        qDebug()<<"udpRxdAddr="<<Address.toString();
        qDebug()<<"udpRxdPort="<<Port;
        qDebug()<<"udpRxdData="<<UdpServerDat;
        qDebug()<<"udpTxd"<<TxdDat;
        UdpServerP->writeDatagram(TxdDat.data(),TxdDat.size(),Address,Port);
    }
}
//TCP新连接
void APP_dev::newConnect()
{
    qDebug()<<"newConnect";
    return;
    if(AppCfg->LoginPort==0)
    {
        TcpServerP = NetServerP->nextPendingConnection();
        connect(TcpServerP, SIGNAL(readyRead()),this, SLOT(ReadyReadTcpServer()));
        connect(TcpServerP, SIGNAL(disconnected()),this, SLOT(disconnected()));
        AppCfg->Timer=0;
        AppCfg->Timeout=new QTimer();
        AppCfg->Timeout->setInterval(6000);
        connect(AppCfg->Timeout, SIGNAL(timeout()),this, SLOT(CheckTcpTimeOut()));
        AppCfg->Timeout->start();
    }
    else if(AppCfg->LoginPort==1)
    {
        qDebug()<<" newConnection Disconnected !";
        QTcpSocket *TcpSocketP=NetServerP->nextPendingConnection();
        TcpSocketP->write("Dev is Busy !\r\n");
        Delayms(100);
        TcpSocketP->disconnect();
        TcpSocketP->close();
        TcpSocketP->deleteLater();
    }
    else if(AppCfg->LoginPort==2)
    {
        qDebug()<<" newConnection Disconnected !";
        QTcpSocket *TcpSocketP=NetServerP->nextPendingConnection();
        TcpSocketP->write("Dev is Busy !\r\n");
        Delayms(100);
        TcpSocketP->disconnect();
        TcpSocketP->close();
        TcpSocketP->deleteLater();
    }
}
//接受TCP数据
void APP_dev::ReadyReadTcpServer()
{    
    QByteArray  ComCmd = TcpServerP->readAll();
    TcpServerP->write(ComCmd);
    return;
    if(AppCfg->LoginPort==0)
    {
        //用户名，密码，登陆
        //Login:Admin;Passwd:123;\r\n
        QString LoginTxt="Login:"+AppCfg->LoginName+";Password:"+AppCfg->LoginPasswd+";\r\n";
        if(ComCmd==LoginTxt)
        {
            AppCfg->LoginPort=2;
            TcpServerP->write("Welcome Login !\r\n");
        }
        else
        {
            TcpServerP->write("Hello,Please Login First !\r\n");
        }
    }
    else if(AppCfg->LoginPort==1)
    {
        TcpServerP->write("Dev is Busy !\r\n");
        Delayms(100);
        TcpServerP->disconnect();
        TcpServerP->close();
        TcpServerP->deleteLater();
    }
    else if(AppCfg->LoginPort==2)
    {
        if(ComCmd=="LoginOff\r\n")//退出设备登陆
        {
            AppCfg->LoginPort=0;
            TcpServerP->write("Login Off ByeBye !\r\n");
        }
        else if(ComCmd=="STX,GETDEVNAME,ETX\r\n")//读取设备名称
        {
            QString Ret="STX,"+AppCfg->DevName+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,GETDEVSN,ETX\r\n")//读取设备ID号
        {
            QString Ret="STX,"+AppCfg->SN+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,GETSWVER,ETX\r\n")//读取设备软件版本
        {
            QString Ret="STX,"+AppCfg->SoftVersion+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,GETHWVER,ETX\r\n")//读取设备硬件版本
        {
            QString Ret="STX,"+AppCfg->HardVersion+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,GETTIME,ETX\r\n")//读取设备时间
        {
            QString Ret="STX,"+GetDateRtcTime()+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,GETLOCALIP,ETX\r\n")//
        {
            QString Ret="STX,"+AppCfg->LocalIp+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,GETLISIP,ETX\r\n")//
        {
            QString Ret="STX,"+AppCfg->LisIP+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,GETLISPORT,ETX\r\n")//
        {
            QString Ret="STX,"+AppCfg->LisPort+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,GETLISMARK,ETX\r\n")//
        {
            QString Ret="STX,"+AppCfg->LisMark+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,GETLAN,ETX\r\n")//读取设备语言标识
        {
            QString Ret="STX,UTF8-English,ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,READBCD,ETX\r\n")//读取二维码
        {
            QString ACK="STX,OK,ETX\r\n";
            TcpServerP->write(ACK.toLatin1());
            QByteArray Code;
            Scan2D_Code(Code);
            QString CodeTxt=Code;
            QString Ret="STX,"+CodeTxt+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,READCCD,ETX\r\n")//读取一维码
        {
            QString ACK="STX,OK,ETX\r\n";
            TcpServerP->write(ACK.toLatin1());
            QByteArray Code;
            Scan1D_Code(Code);
            QString CodeTxt=Code;
            QString Ret="STX,"+CodeTxt+",ETX\r\n";
            TcpServerP->write(Ret.toLatin1());
        }
        else if(ComCmd=="STX,OPENTRAY,ETX\r\n")//设备出仓
        {
            if(AppCfg->AdcStep==0 || AppCfg->AdcStep==6)
            {
                QString ACK="STX,OK,ETX\r\n";
                TcpServerP->write(ACK.toLatin1());
                agl_out();
            }
            else
            {
                QString ACK="STX,DEVISBUSY,ETX\r\n";
                TcpServerP->write(ACK.toLatin1());
            }
        }
        else if(ComCmd=="STX,CLOSETRAY,ETX\r\n")//设备入仓
        {
            if(AppCfg->AdcStep==0 || AppCfg->AdcStep==6)
            {
                QString ACK="STX,OK,ETX\r\n";
                TcpServerP->write(ACK.toLatin1());
                agl_int();
            }
            else
            {
                QString ACK="STX,DEVISBUSY,ETX\r\n";
                TcpServerP->write(ACK.toLatin1());
            }
        }
        else if(ComCmd=="STX,START,ETX\r\n")//启动通道采集
        {
            if(AppCfg->AdcStep==0 || AppCfg->AdcStep==6)
            {
                QString ACK="STX,OK,ETX\r\n";
                TcpServerP->write(ACK.toLatin1());
                agl_start();
            }
            else
            {
                QString ACK="STX,DEVISBUSY,ETX\r\n";
                TcpServerP->write(ACK.toLatin1());
            }
        }
        else if(ComCmd=="STX,GETRESUALTS,ETX\r\n")//读取T线，C线数值，浓度值。
        {
            if(AppCfg->AdcStep==0)
            {
                QString ACK="STX,ISNULL,ETX\r\n";
                TcpServerP->write(ACK.toLatin1());
            }
            if(AppCfg->AdcStep==6)
            {
                if(AppCfg->RetString=="OK")
                {
                    QString ACK="STX,OK,ETX\r\n";
                    TcpServerP->write(ACK.toLatin1());
                }
                else if(AppCfg->RetString=="ER1")
                {
                    QString ACK="STX,ER1,ETX\r\n";
                    TcpServerP->write(ACK.toLatin1());
                }
                else if(AppCfg->RetString=="ER2")
                {
                    QString ACK="STX,ER2,ETX\r\n";
                    TcpServerP->write(ACK.toLatin1());
                }
                else if(AppCfg->RetString=="ER3")
                {
                    QString ACK="STX,ER3,ETX\r\n";
                    TcpServerP->write(ACK.toLatin1());
                }
            }
            else
            {
                QString ACK="STX,DEVISBUSY,ETX\r\n";
                TcpServerP->write(ACK.toLatin1());
            }
        }
        else//错误帧格式应答
        {
            QString Star=GetKey(0,ComCmd);
            QString Cmd=GetKey(1,ComCmd);
            if(Star=="STX" && Cmd=="SAMPLEID")//发送样本ID
            {
                AppCfg->SampleID=GetKey(2,ComCmd);
                QString ACK="STX,OK,ETX\r\n";
                TcpServerP->write(ACK.toLatin1());
            }
            else
            {
                QString NCK="STX,ER0,ETX\r\n";
                TcpServerP->write(NCK.toLatin1());
            }
        }
    }
}
//断开连接
void APP_dev::disconnected()
{
    qDebug()<<"disconnected";
    TcpServerP->disconnect();
    TcpServerP->close();
    TcpServerP->deleteLater();
}

void APP_dev::CheckTcpTimeOut()
{
    //30min
    if(++AppCfg->Timer>=10*30)
//    if(++AppCfg->Timer>=10*1)
    {
        qDebug() << "time is 1 min";
        AppCfg->Timer=0;
//        AppCfg->Timeout->stop();
//        qDebug() << "time is stop";
//        AppCfg->Timeout->deleteLater();
        qDebug() << "time is delete";
        if(AppCfg->LoginPort==1)
        {
            AppCfg->LoginPort=0;
            qDebug() << "5696 loginport is 0";
        }
        else if(AppCfg->LoginPort==2)
        {
            AppCfg->LoginPort=0;
            qDebug() << "5702 loginport is 0";
//            disconnected();
            qDebug() << "use disconnect function";
        }
    }
}

bool APP_dev::ReloadPassConfig()
{
    PasswdList->clear();
    QFile MessageFile("./password.txt");
    if (!MessageFile.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        return false;
    }
    while(!MessageFile.atEnd())
    {
        QString lineString=QString(MessageFile.readLine());
        if(lineString.left(1) == "#")
        {
            continue;
        }
        QStringList StrTxt = lineString.split("#");
        lineString = StrTxt.at(0);
        StrTxt = lineString.split("=");
        *PasswdList +=StrTxt;
    }
    MessageFile.close();
    return true;
}

bool APP_dev::ReloadPasswdFile(QString FileData)
{
    if(FileData.isEmpty())
    {
        return false;
    }
    QStringList ListTemp = FileData.split("#\r\n");
    int FileLengh = ListTemp.length();
    if(FileLengh == 1)
    {
        return false;
    }
    PasswdList->clear();
    QString StrTemp;
    QStringList ListLoad;
    QStringList ListBuf;
    for(int i = 0; i < FileLengh - 1; i++)
    {
        StrTemp = ListTemp.at(i);
        ListLoad = StrTemp.split("=");
        ListBuf.append(ListLoad.at(0));
        ListBuf.append(ListLoad.at(1));
    }
    *PasswdList = ListBuf;
    return true;
}

bool APP_dev::LoadPasswdFile()
{
    PasswdList = new  QStringList();
    PasswdList->clear();
    QFile MessageFile("./password.txt");
    if (!MessageFile.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        return false;
    }
    while(!MessageFile.atEnd())
    {
        QString lineString=QString(MessageFile.readLine());
        if(lineString.left(1) == "#")
        {
            continue;
        }
        QStringList StrTxt = lineString.split("#");
        lineString = StrTxt.at(0);
        StrTxt = lineString.split("=");
        *PasswdList += StrTxt;
    }
    MessageFile.close();
    return true;
}

bool APP_dev::ReWriteFile(QString Key, QString Data)
{
    Key = "@"+Key;
    QStringList List_Temp;
    List_Temp.clear();
    int iLengh = PasswdList->length();
    qDebug() << "iLengh is " << iLengh;

    if(0 == iLengh)
    {
        return false;
    }
    for(int i = 0; i < iLengh - 1; i+=2)
    {
        if(PasswdList->at(i) == Key)
        {
            List_Temp.append(Key);
            List_Temp.append(Data);
            continue;
        }
        List_Temp.append(PasswdList->at(i));
        List_Temp.append(PasswdList->at(i+1));
    }
    PasswdList->clear();
    *PasswdList = List_Temp;
    return true;
}

QString APP_dev::GetPasswd(QString key)
{
    for(int i=0;i<PasswdList->size();i++)
    {
        if(key==PasswdList->at(i))
        {
            QString Txt=PasswdList->at(i+1);
            if(Txt.length()>0)
            {
                return Txt;
            }
            else
            {
                return "No";
            }
        }
    }
    return "No";
}

bool APP_dev::LoadConfigFile()
{
    ConfigList = new  QStringList();
    ConfigList->clear();
    QString FileName;
    QString Langeuage = GetPasswd("@Languag");
    if(Langeuage == "English")
    {
        FileName = "./config_ENG.txt";
    }
    else if(Langeuage == "Chinese")
    {
        FileName = "./config_CHN.txt";
    }
    QFile ConfigFile(FileName);
    if (!ConfigFile.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        return false;
    }
    while(!ConfigFile.atEnd())
    {
        QString lineString=QString(ConfigFile.readLine());
        QStringList StrTxt = lineString.split("=");
        for(int i=0;i<StrTxt.size();i++)
        {
             QString Str=StrTxt.at(i);
             QStringList Note= Str.split("#");
             *ConfigList +=Note;
        }
    }
    ConfigFile.close();
    return true;
}

QString APP_dev::GetKey(QString Key, QString Txt)
{
    QStringList StrTxt = Txt.split("=");
    for(int i=0;i<StrTxt.size();i++)
    {
        QString Str=StrTxt.at(i);
         QStringList Note= Str.split("#");
         *ConfigList +=Note;
    }
    return Key;
}

QString APP_dev::GetKey(int index, QString Txt)
{
    QStringList TxtList = Txt.split(",");
    QString Ret=TxtList.at(index);
    return Ret;
}

QString APP_dev::GetConfig(QString key)
{
    for(int i=0;i<ConfigList->size();i++)
    {
        if(key==ConfigList->at(i))
        {
            QString Txt=ConfigList->at(i+1);
            if(Txt.length()>0)
            {
                //qDebug()<<key+"="+Txt;
                return Txt;
            }
            else
            {
                return "No";
            }
        }
    }
    return "No";
}
//-----Add-------liuyouwei--------------//
//QStringList APP_dev :: GetAll_Data()
//{
//    QString AllData;
//    QString RetData;
//    QStringList RetList;
//    QFile *config = new QFile;
//    config->setFileName("password.txt");
//    if(!config->exists())
//    {
//        qDebug() << "The file not found!";
//        delete config;
//        return RetList;
//    }
//    if(!config->open(QIODevice :: ReadOnly | QIODevice :: Text))
//    {
//        qDebug() << "The file open Error!";
//        delete config;
//        return RetList;
//    }
//    QTextStream in(config);
//    while(!in.atEnd())
//    {
//        AllData = in.readLine();
//        if(AllData.left(1) == "#" ||AllData.left(1) == "@")
//        {
//            continue;
//        }
//        RetData += AllData;
//    }
//   RetList = RetData.split("#");
//   config->close();
//   delete config;
//   return RetList;
//}

void APP_dev::Rank_Data(FoundPeakVol *arr, int lengh)
{
    int i, j;
    int m = lengh;
    double Temp_data = 0;
    int Temp_location = 0;
    for (i = 0; i < lengh; i++)
    {
        m -= 1;
        for (j = 0; j < m; j++)
        {
            if (arr[j].ReadData <= arr[j+1].ReadData)
            {
                Temp_data = arr[j].ReadData;
                Temp_location =arr[j].Location;
                arr[j].Location = arr[j+1].Location;
                arr[j].ReadData = arr[j+1].ReadData;
                arr[j+1].ReadData = Temp_data;
                arr[j+1].Location = Temp_location;
            }
        }
    }
}

void APP_dev::Rank_Location(FoundPeakVol *arr, int lengh)
{
    int i, j;
    int m = lengh;
    double Temp_data = 0;
    int Temp_location = 0;
    for (i = 0; i < lengh; i++)
    {
        m -= 1;
        for (j = 0; j < m; j++)
        {
            if (arr[j].Location <= arr[j+1].Location)
            {
                Temp_data = arr[j].ReadData;
                Temp_location =arr[j].Location;
                arr[j].Location = arr[j+1].Location;
                arr[j].ReadData = arr[j+1].ReadData;
                arr[j+1].ReadData = Temp_data;
                arr[j+1].Location = Temp_location;
            }
        }
    }
}

void APP_dev::Rank_TCData(double *arr, int lengh)
{
    int i, j;
    int m = lengh;
    double Temp_data = 0;
    for (i = 0; i < lengh; i++)
    {
        m -= 1;
        for (j = 0; j < m; j++)
        {
            if (arr[j] >= arr[j+1])
            {
                Temp_data = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = Temp_data;
            }
        }
    }
}

int APP_dev::FoundTCData(double Data, double *Buf)
{
    int iRet = 0;
    int i = 0;
    for(; i < 8;  i++)
    {
        if(Buf[i] == Data)
        {
            break;
        }
        else
        {
            iRet++;
        }
    }
    if(iRet == 8)
    {
        return -1;
    }
    else
    {
        return i;
    }
}

QStringList APP_dev::GetAllUserData()   // 获得password.txt配置文件的用户内容
{
    QStringList RetList;
    QString RetStr;
    for(int i = 0; i < PasswdList->length()-1; i+=2)
    {
        if(PasswdList->at(i).left(1) != "@")
        {
            RetStr = PasswdList->at(i) + "=" + PasswdList->at(i+1);
            RetList.append(RetStr);
        }
    }
    RetList.append(" ");
    return RetList;
}

QString APP_dev::GetAllPass()       // 获得password.txt配置文件的所有内容
{
    QString LineData;
    QString AllData;
    for(int i = 0; i < PasswdList->length() - 1; i+=2)
    {
        LineData = PasswdList->at(i) + "=" + PasswdList->at(i+1) + "#\r\n";
        AllData += LineData;
    }
    return AllData;
}

QString APP_dev::GetAllParadata()  // 获得password.txt配置文件除用户之外的内容
{
    QString LineData;
    QString AllData;
    for(int i = 0; i < PasswdList->length()-1; i+=2)
    {
        if(PasswdList->at(i).left(1) == "@")
        {
            LineData = PasswdList->at(i) + "=" + PasswdList->at(i+1) + "#\r\n";
            AllData += LineData;
        }
    }
    return AllData;
}

////----------------------------------Add-11-16------------------------------------------//
//void APP_dev::RecordInterpetation(QString Peretation)
//{
//    QFile TimeFile("./PG1.txt");
//    if(!TimeFile.open(QFile::WriteOnly | QFile::Append))
//    {
//        qDebug() << "open file Error";
//        return;
//    }
//    QTextStream Stream(&TimeFile);
//    Stream << Peretation;
//    TimeFile.close();
//}
////------------------------------------------------------------------------------------------//

//---------------------------------------------//
//QString APP_dev::CopyUpDataFile(QString key)//复制更新文件到系统中。
//{
//    key+="qq";
//    static bool Flag=false;
//    if(Flag==false)
//    {
//        QFile ConfigFile("/yx-udisk/system.txt");
//        if(ConfigFile.exists())
//        {
//            QStringList *CfigList=new  QStringList();
//            if (!ConfigFile.open(QIODevice::ReadOnly|QIODevice::Text))
//            {
//                return false;
//            }
//            while(!ConfigFile.atEnd())
//            {
//                QString lineString=QString(ConfigFile.readLine());
//                QStringList StrTxt = lineString.split("=");
//                for(int i=0;i<StrTxt.size();i++)
//                {
//                    QString Str=StrTxt.at(i);
//                     QStringList Note= Str.split("#");
//                     *CfigList +=Note;
//                }
//            }
//            ConfigFile.close();
//            QString Key="UpDataFile";
//            QString Txt;
//            for(int i=0;i<CfigList->size();i++)
//            {
//                if(Key==CfigList->at(i))
//                {
//                    QString String=CfigList->at(i+1);
//                    if(String.length()>0)
//                    {
//                        Txt=String;
//                    }
//                }
//            }
//            QString Code;
//            Code ="cp " +Txt +" ./ \r\n";
//            char * cmd=(char *)Code.toStdString().data();
//            SysCall((const char *)cmd);
//            Delayms(100);
//            Code ="chmod 777 " +Txt +" \r\n";
//            SysCall((const char *)cmd);
//        }
//        ConfigFile.close();
//    }
//    return "ok";
//}

