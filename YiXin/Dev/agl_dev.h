#ifndef AGL_DEV_H
#define AGL_DEV_H
//------------------------------------------------------------------------//
//DevP = new APP_dev(this);
//DevP->AppSta=AGL_DEV;
//DevP->OpenDevice();
//DevP->start();
//------------------------------------------------------------------------//
#define __APP_LIB_
//------------------------------------------------------------------------//
#ifdef __APP_LIB_
    #include <QtCore/qglobal.h>
    #if defined(CGR_LIB_LIBRARY)
    #  define CGR_LIBSHARED_EXPORT Q_DECL_EXPORT
    #else
    #  define CGR_LIBSHARED_EXPORT Q_DECL_IMPORT
    #endif
#endif
//------------------------------------------------------------------------//
#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QThread>
#include <QCoreApplication>
#include <QByteArray>
#include <QDebug>
#include <QTimer>
#include  <QTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QtMath>
#include <QProcess>
#include <QObject>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QByteArray>
#include <QDebug>
#include <QNetworkInterface>
//------------------------------------------------------------------------//
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "stdio.h"
#include "string.h"
//------------------------------------------------------------------------//
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
//------------------------------------------------------------------------//
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "stdio.h"
#include "string.h"
//------------------------------------------------------------------------//
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "stdio.h"
#include "string.h"
//------------------------------------------------------------------------//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
//------------------------------------------------------------------------//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
//------------------------------------------------------------------------//
#include "qzxing.h"
#include "../Sql/sql.h"
//------------------------------------------------------------------------//
#define WY_COM     1
//------------------------------------------------------------------------//
//------------------------------------------------------------------------//
#define VideoFormartJPG     1
//#define VideoFormartRGB    1
//------------------------------------------------------------------------//

typedef struct FoundPeakVol
{
    int Location;
    double ReadData;
}FoundPeakVol;

typedef struct
{
    int Port;//串口文件
    char Dev[16];//  "/dev/ttySAC3"
    struct termios oldtio;
    struct termios newtio;
    char Buf[16*1024];//接收数据缓存
    bool State;//串口状态
} ComStr;//串口数据结构

enum AppStaStr{
    NOP_DEV=0x00,//空闲
    AGL_DEV =0x01,//荧光设备标识
    CGR_DEV=0x02,//金标设备标识
};
enum Com3Xstr{
    ComNC=0x00,
    Com1D=0x01,//一维扫描仪
    Com2D=0x02,//二维扫描仪
    ComPrinter=0x03,//热敏打印机
    ComExt=0x04,//备用TTL串口
};

//Com1=AGL+CGR
//Com2=PC
//Com30=Scan1D
//Com31=Scan2D
//Com32=Printer
//Com33=Extern

//-----------------------Add-------------------------//
enum NumberIndex
{
    Index1 = 0x0001,
    Index2 = 0x0002,
    Index3 = 0x0003,
    Index4 = 0x0004,
};
//-----------------------------------------------------//

enum CheckErrorCode
{
    Error_OK = 0x00,        // 自检通过，无异常
    Error_SPI,                    // 001 SPI通信出错
    Error_X,                       //  002 X轴电机线没接好
    Error_Y,                       //  003 Y轴电机线没接好
    Error_Y_Max,              //  004 Y轴运动过程中过零位误差过大
    Error_Chip,                 //  005 参数写入存储芯片错误
    Error_Err,                    //  自检通信出错
    Error_NoCom             //  串口通信出错
};

//------------------------------------------------------------------------//
typedef struct
{
    int Xpos;
    int Ypos;
    int Xsize;
    int Ysize;
    int abx;
}  ImgPosStr;//金标图像分割算法结构
typedef struct
{
    int Exp;//曝光值
    int WhiteBalance;//白平衡值
    int Brightness;//亮度
    int Contrast;//对比度
    int Saturation;//饱和度
    int Hue;//色调
    int Sharpness;//清晰度
    int Agc;//增益
    int Gamma;//伽玛
    int Focus;//焦点
} CameraPmStr;//金标摄像头参数
typedef struct
{
    int ImgAC_Pos;
    int ImgBC_Pos;
    double ImgACVol;
    double ImgBCVol;
    double ImgATVol;
    double ImgBTVol;
}   CgrCalcStr;//金标计算结果
typedef struct
{
    char Version[50];
    char LedOnoff;

    char Led1SetVol;
    char Led2SetVol;
    char Led3SetVol;
    char Led4SetVol;

    char Led1AdcVol;
    char Led2AdcVol;
    char Led3AdcVol;
    char Led4AdcVol;

    char LedTemp;
}   CgrLedStr;//金标LED板的参数
typedef struct
{    
    double HbtVol;//Hb-t
    double HbcVol;//Hb-c
    double HptVol;//Hp-t
    double HpcVol;//Hp-c
    QPixmap HbImagePix;
    QPixmap HpImagePix;
    int ImageATpos;//
    int ImageBTpos;//    
    double ImageA[600];
    double ImageB[600];
    int ImgAbk[60][600];
    int ImgBbk[60][600];
} CgrVolStr;//金标诊断结果的数据结构
enum CgrState{
    CgrNop=0x00,//空闲
    CgrIsOk =0x01,//
    CgrSavImg=0x02,//保存图片
    CgrCalcImg=0x03,//计算结果
    CgrCalcEnd=0x04,//计算完成
    CgrIsError =0x05,//
};//金标虚拟设备状态
typedef struct
{
    CgrState State;
    CgrVolStr CgrVol;
    CgrLedStr LedState;
    ImgPosStr ImgPosPm;
    CameraPmStr CameraPm;
    QTimer * Timer;
} CgrDevStr;//金标虚拟设备结构
//------------------------------------------------------------------------//
typedef struct
{
    int MinPos;//荧光曲线的谷底位置
    int MaxPos;//荧光曲线的峰值位置
    double MinVol;//荧光曲线最小值
    double MaxVol;//荧光曲线最大值
    int CPeakPos;//曲线中C线位置
    int TPeakPos;//曲线中T线位置
    double CPeakVol;//C值
    double TPeakVol; //T值
    double YSumVol;//荧光结果浓度
}   AdcCalcStr;//荧光计算结果
typedef struct
{
    //项目
    QString xiangmu;//项目名称
    QString xiangmudanwei;//项目单位
    QString czuidizhi;//C线极限低值
    QString czuigaozhi;//C线极限高值
//    QString cqidian;//C线起点
//    QString czhongdian;//C线终点
//    QString tqidian;//T线起点
//    QString tzhongdian;//T线终点
    QString WB_Para;// 全血系数
    QString Equation_Limit;//方程最低值
    QString Equation_Max;//方程最高值
    QString NOP;                //数组0为批号显示的项目，其余保留
    QString anongduzhi;//浓度参考值A
    QString bnongduzhi;//浓度参考值B
    QString cankaoxingshi;//浓度参考显示形式
    QString nongdudizhi;//浓度极限低值
    QString nongdugaozhi;//浓度极限高值
    QString jisuanfangshi;//计算方式
    QString jisuanfangfa;//计算浓度方法
    QString canshua;//参数A
    QString canshub;//参数B
    QString canshuc;//参数C
    QString canshud;//参数D
} XiangMuPmStr;//二维码里面的项目参数

typedef struct
{
    QString Head;//YX-
    QString Pihao;//批号
    QString Yiweima;//一维码
    QString Xiangmushuliang;//项目数量
    QString PiLiang;//批量
    QString youxiaoshijian;//有效时间
    QString yanshi;//有效时间
    XiangMuPmStr XiangMuPm[4];
} TwoCodeStr;//二维码参数结构
typedef struct
{
    int Tstart;
    int Tend;
    int Cstart;
    int Cend;
} TlineClinePosStr;//C line and T line Pos

//AglMcuBin=AglMcu.bin#
typedef struct
{
    char Head[10];    //格式="[YiXinBIO]";
    char Ver[10];       //格式="[AGL-V1.0]";
    char Date[12];     //格式="[2017-10-10]";    
    char MD5[32];     //格式="1234....32"
} BinHeadStr;//

typedef struct
{
    bool state;//虚拟荧光设备状态
    char Version[64];//虚拟荧光版本
    char LedCurrent;//荧光灯的电流
    char PdTemperature;//光敏温度
    char LedTemperature;//荧光灯的温度
    char OutIntState;//出仓，入仓状态
    char AdcState;//荧光采集是否完成状态。

    char MotorDX;//X轴电机参数
    char MotorDY;//Y轴电机参数
    char LedSetVol;//荧光灯的设定电流
    char ChxVol;//荧光设置的通道个数

    double ChxBuf[4][2000];//荧光采集的初始值
    double BakBuf[4][2000];//荧光采集的本底值
    double VolBuf[4][2000]; //荧光采集的最终值
    TwoCodeStr TwoDimCode;//二维码参数
    AdcCalcStr AdcCalc[4];//荧光计算结果
}   AglDevStr;//虚拟荧光设备
typedef struct
{
    int8_t LoginPort;//=0 NuLL ; =1 Com,Dev is Busy1!;  =2Net,Dev is Busy2!;=3 Dev is Busy3!
    QString SN;
    QString LocalIp;
    QString LisMark;
    QString LisIP;
    QString LisPort;
    QString DevName;
    QString LoginName;
    QString LoginPasswd;

    QString HardVersion;
    QString SoftVersion;

    bool TcpState;//服务器状态
    int    Timer;
    QTimer *Timeout;

    QString RetString;
    QString Code1D;
    QString SampleID;
    TwoCodeStr TwoDimCode;
    QTimer *TimerP;
    int8_t AdcStep;
} ConfigStr;

typedef struct
{
    int ListenSocket;
    int ListenPort;
} QTcpStr;

typedef struct
{
    QString BinName;// updata file in udisk "***.bin"
    QString BinVersion;//updata file version in "***.bin"
    QString BinDate;//updata file date in "***.bin"
    QByteArray BinDat;
    QByteArray RxdDat;
    QTimer  *TimerP;
    int16_t   MaxNum;//page number of "***.bin",1 page=1024 bytes
    int16_t   PreNum;//
    int16_t   DatStar;
    int16_t   DatLen;
    QString  State;//=ok,=waiting;=other is error;
} McuUpDataStr;

//------------------------------------------------------------------------//
#ifdef __APP_LIB_
class CGR_LIBSHARED_EXPORT APP_dev : public QThread
#else
class APP_dev : public QThread
#endif
{
    Q_OBJECT
public:
    explicit APP_dev(QThread *parent = 0);
    ~APP_dev();
public:
    //-----------Add------------//
    bool isTestContrl;
    bool isCalibration;
    NumberIndex Index_number;
    //----------------------------//
    QTcpServer *NetServerP;
    QTcpSocket *TcpServerP;
    QTcpSocket *TcpClientP;
    QUdpSocket *UdpServerP;
    QUdpSocket *UdpClientP;

    AppStaStr  AppSta;//虚拟设备标识，AGL ，CGR
    ConfigStr   *AppCfg;
    AglDevStr  AglDev;//虚拟荧光
    CgrDevStr CgrDev;//虚拟金标
    TestSql *SqlDataP;
    McuUpDataStr *McuUpDataP;
//-----------Add----liuyouwe-----------------//
   bool isFoundPasscfg;
//   QStringList GetAll_Data();

   QString Location_C;
   bool ReWriteFile(QString Key, QString Data); // 将新的内容写入文件链表
   void Rank_Data(FoundPeakVol *arr, int lengh);
   void Rank_Location(FoundPeakVol *arr, int lengh);
   void Rank_TCData(double *arr, int lengh);
   int FoundTCData(double Data, double *Buf);

   int iPeakTimes;
   bool Card_index[4];
   bool isDoor;
//--------------------------------------------------//
    bool yx_udisk;//“yx-udisk”
    bool yx_sdisk;//“yx-sdisk”
    bool yx_network;//true ，false
    double yx_power;//2.00 ,1.00,0.75,0.50，0.2

    QString GetVersion();//获得虚拟设备版本
    bool PrintData(QByteArray DatStr);//打印字符
//    bool PrintData(std::string DatStr);//打印字符
    bool Scan1D_Code(QByteArray &Code);//扫描一位码
    bool Scan2D_Code(QByteArray &Code);//扫描二位码
    bool GetRtcTime(QString &TimeStr);//获取系统时间
    QString GetRtcTime();//获取系统时间
    QString GetDateRtcTime();
    bool SetRtcTime(QString TimeStr);//设置系统时间"2015-5-8 19:48:00"
    QString GetLocalIP(void);//获得本机IP
    QHostAddress GetLocalIPaddr(void);//获得本机IP
    void ReBoot();//系统重启
    void  SysCall(QString Cmd);//系统调用
    bool TS_Calibrate(void);//重新校准屏幕
    bool SetLocalIp(QString Ip);//"192.168.2.200"

    QString UpDataApp(void); //copy app from udisk;"UpDataApp=***#"
    QString UpDataCfg(void);  //copy config file from udisk;"UpDataCfg=***#"
    QString UpDataResualtdb(void); //copy Resualt database file from udisk;"TestResult=***#"
    QString UpDataBatchdb(void); //copy Batch database file from udisk;"Batch=***#"

//    QString OutDataApp(void);//copy app to udisk;"UpDataApp=***#"
    QString OutDataCfg(void);//copy config file to udisk;
    QString OutDataResualtdb(void);//copy Resualt database file to udisk;
    QString OutDataBatchdb(void);//copy Batch database file to udisk;

    QString File1ToFile2(QString File1Name,QString File2Name);//ret="ok";ret="error:***"
    //----------------Add-------------------//
    bool File1ToFile2_Shell(QString Dir);
    //----------------------------------------//

    bool DeleteFile(QString FileName);//FileName="./passwd.txt"
    bool CreatFile(QString FileName);//FileName="passwd.txt"
    bool WriteTxtToFile(QString FileName, QString Txt);//FileName="./passwd.txt",Txt="hello world!"
    bool ReadTxtFromFile(QString FileName, QString &Txt);//FileName="./passwd.txt",Txt="hello world!"
    //------------------Add-----Process-----//
    void SysCall_process(QString CMD);
    //--------------------------------------------//
public slots:
    QString agl_updata_mcu();//=ok; =waiting;=other is error;
    void UpDataTimeout();
    bool agl_rd_ver();//获取荧光电机控制板的程序版本
    bool agl_rd_stat();//获取荧光电机控制板的状态
    bool agl_rd_pm();//获取荧光电机控制板的电机，荧光电流，温度参数
    bool agl_wr_pm();//设置荧光电机控制板的
    bool agl_out();//电机出仓
    bool agl_int();//电机入仓
    bool agl_adc(char CarNumb);//荧光电机控制板启动采集,CarNumb=二维码中的项目个数1,4。
    bool agl_get();//读取荧光电机控制板的采集值
    bool agl_read_pm();//荧光读取存储参数
    bool agl_save_pm();//荧光保存存储参数
    bool agl_Load_pm();//荧光加载存储参数
    bool agl_save_back();//荧光保存本底
    bool agl_load_back();//荧光加载本底
    bool agl_clear_back();//荧光清除本底
    TwoCodeStr agl_get_two(QString Code1D);
    bool agl_start();
    void agl_work();
    void agl_com();
    bool agl_calc_vol(TwoCodeStr TwoDimCode);//计算分析荧光的结果值
    bool agl_calc_buf(TlineClinePosStr TcPos,double *bufP,int Length,AdcCalcStr &CalcVol);//荧光数据的分析
    std::string GetInterpapreTation(double PG1, double PG2, double G17, double HPG, int post);
    QString GetCsvInterpretation(double PG1,double PG2,double HPG,double G17, int post);

    bool cgr_load_back();//金标加载本底
    bool cgr_save_back();//金标保存本底
    bool cgr_load_pm();//金标加载参数
    bool cgr_save_pm();//金标保存参数
    uchar cgr_get_insert();//金标获取卡槽插入状态
    bool cgr_rd_led_ver();//金标读取灯控板的版本
    bool cgr_rd_led_stat();//金标读取灯控板的状态
    bool cgr_rd_led_pm();//金标读取灯控板的参数
    bool cgr_wr_led_pm();//金标设置灯控板的
    bool cgr_led_onoff(uchar Stat);//金标读取灯控板的开灯，关灯
    bool cgr_led_on();//金标读取灯控板的开灯
    bool cgr_led_off();//金标读取灯控板的关灯
    bool cgr_set_cam(CameraPmStr &CameraPm);//金标设置摄像头的参数
    bool cgr_get_image(QPixmap &ImagePix);//金标读取摄像头影像数据
    bool cgr_get_imgab(QPixmap &ImagePixA, QPixmap &ImagePixB);//金标读取摄像头分割后的影像数据
    bool cgr_get_imgab(QImage &ImageA, QImage &ImageB);//
    bool cgr_open();//金标虚拟设备打开
    bool cgr_start(TwoCodeStr TwoDimCode);//金标虚拟设备启动
    bool cgr_timeout();//金标定时器超时
    bool cgr_calc(CgrDevStr &CgrDev);//金标结果分析计算
    QString cgr_get_one();//摄像头识别一维码
    bool cgr_start();
    void cgr_work();
    void cgr_com();
public slots:
    QString GetPasswd(QString key);//根据关键词，读取提示消息
    QString GetConfig(QString key);//根据关键词，读取配置参数

    double Log10(double x);//对x进行log运算
    void FmqON();//蜂鸣器开
    void FmqOFF();//蜂鸣器关
    void LedON();//指示灯开
    void LedOFF();//指示灯关

private:
    void run();//线程运行
    void Delayms(uint del);    //yan sh
    u_int16_t GetCRC(char *pchMsg,u_int16_t wDataLen);    
    bool SetPinIOSta(uchar Pin,uchar Sta);//Pin =0~15,Stat=0,1
    bool SetPinState(uchar Pin,uchar Sta);//Pin =0~15,Stat=0,1
    uchar GetPinState(uchar Pin);//Pin =0~15,Stat=0,1
    bool SetCom3State(Com3Xstr Com3X);
    void setTermios(termios *pNewtio, unsigned short uBaudRate);//serial port set function

    int WriteCom(ComStr *ComX,char *DatBuf,int DatLen);
    int ReadCom(ComStr *ComX,char *DatBuf,int DatLen);
    QByteArray ReadComTimeout(QSerialPort *ComP,int Timeout);
    QByteArray ReadComTimeout(ComStr *ComP,int Timeout);


public:
    bool agl_wr_data(char *DatBuf,int DatLen);
    QByteArray agl_rd_updata(uchar Cmd,int Timeout);
    QByteArray agl_rd_data(uchar Cmd,int Timeout);
    bool cgr_wr_data(char *DatBuf,int DatLen);
    QByteArray cgr_rd_data(uchar Cmd,int Timeout);

    QByteArray pc_rd_data(void);
    bool pc_wr_data(QByteArray Data);
public slots:
    bool OpenDevice();//打开设备
    bool CloseDevice();//关闭设备
private slots:
    void TimerOut();
    void TimerOut2s();
    bool OpenLed(void);
    bool CloseLed(void);
    bool OpenGpio(void);
    bool CloseGpio(void);
    bool OpenAdc(void);
    bool CloseAdc(void);
    bool OpenTem(void);
    bool CloseTem(void);
    bool OpenCom(void);
    bool CloseCom(void);
    bool OpenCamera();
    bool CloseCamera();
    void Com1Read();
    void Com2Read();
    void Com3Read();
public slots:
    void OpenNetwork();
    void TestNetWork();
    void CloseNetWork();
    void ReadyReadUdpServer();
    void ReadyReadTcpServer();
    void disconnected();
    void CheckTcpTimeOut();
    void newConnect();

public:
    QStringList *PasswdList;
    QString GetAllParadata();
    QString GetAllPass();
    QStringList GetAllUserData();
    bool ReloadPasswdFile(QString FileData); // 将修改后的Passwd文件的内容（还未保存至文件）加载到链表
    bool ReloadPassConfig();  // 将新的文件内容（已经保存在文件中）加载值链表

    bool GetCheckError();
    CheckErrorCode Error_Code;

//    QString Gas_Show;
//    void RecordInterpetation(QString Peretation);

//-----------------------------for test------------------------------//
//    QByteArray ReadPcComData(void);
//---------------------------------------------------------------------//
private:
//    QStringList *PasswdList;
    bool LoadPasswdFile();
    QStringList *ConfigList;
    bool LoadConfigFile();
    QString GetKey(QString Key,QString Txt);
    QString GetKey(int index,QString Txt);
};

#endif // APP_DEV_H
