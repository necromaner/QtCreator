#ifndef YIXIN_H
#define YIXIN_H

#define TIMES 6

#include <QMainWindow>
#include <QEventLoop>
#include <QThread>
#include <QDebug>
#include <QTimer>
#include <QDate>
#include <QTime>
#include <QTimeEdit>
#include <QDateEdit>
#include <QPalette>
#include <QPixmap>
#include <QProgressBar>
#include <QBrush>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QImage>
#include <QTextEdit>
#include <QBitmap>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QColor>
#include <QRadioButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QPainterPath>
#include <QRectF>
#include <QPolygon>
#include <QPoint>
#include <QRegion>
#include <QListWidgetItem>
#include <QList>
#include <QSpinBox>
#include <QTextStream>
#include <QFile>
#include <QApplication>
#include <QAbstractSocket>
#include <QDesktopWidget>
#include <QScreen>
#include <QTextEdit>
#include <QScrollArea>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QByteArray>
#include <QStyle>
#include <QScrollBar>
#include <QStyleFactory>
#include <QLCDNumber>
#include <QTcpSocket>
#include <QCoreApplication>
#include <QObject>
#include <QDir>
#include <iostream>
#include <math.h>
//-------------------------------//
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_legend.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>
#include <qwt_legend.h>
#include <qwt_legend_label.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_grid.h>
#include <QPolygonF>
#include <QPointF>
#include <QAbstractButton>
//-------------------------------//
#include "string.h"
#include "../Sql/sql.h"
#include "./myclass.h"
#include "../Dev/agl_dev.h"
#include "../Dev/uUsbCamera.h"

enum UiWidgetType
{
    StarBmpUi = 0x0000,     // 开机界面                             0
    LoginUi,                           // 登陆界面
    MainUi,                           // 主界面
    Setui,                               // 设置界面
    Testui,                             //  试验界面
    TestKeyui,                       //  样本ID输入键盘                5
    TesTResui,                      //  测试结果界面
    COSui,                            // 仪器设置界面
    Selectui,                         //  查询界面
    Passui,                            //   密码修改
    Usrui,                              //   用户设置                           10
    Usrkeyui,                        //   用户设置键盘
    NewPassui,                    //   新密码
    NewPassui2,                  //   新密码确认
    SelectKeyui,                    //    查询界面带字母键盘
    UserKeyui,                      //   用户管理带字母键盘         15
    PassKeyui,                      //   用户管理密码用键盘
    NewKeyui,                      //   账户申请密码键盘
    SetUserui,                      //   账户管理界面
    Inforui,                           //   信息确认界面
    InforKeyui,                     //   倒计时键盘                        20
    TestNumui,                    //   样本输入界面数字键盘界面
    Updateui,                      //   升级界面
    HostIpui,                       //   当从本机IP输入框打开设备管理键盘
    LisIpui,                           //   当从LisIP输入框打开设备管理键盘
    LisPortui,                        //   当从Lis端口输入框打开设备管理键盘
    T1ui,
    T2ui,
    T3ui,
    T4ui,
    C1ui,
    C2ui,
    C3ui,
    C4ui,
    LotKeyui,
};

enum UpdaType
{
    Update_APP = 0x00,     // 软件更新
    Update_Gujian,             // 固件更新
    Out_APP,                       // 软件导出
    Out_Database,              // 数据导出
    IN_Database,                // 数据更新
    Out_Config,                   // 配置导出
};

enum CheckType
{
    Check_OutDoor = 0x00,     // 出舱门自检
    Check_InDoor,                    // 入舱门自检
    Check_InDoor_Test,           // 测试入舱门自检
    Check_Run,                         // 测试自检
    Check_Calib_Run,               // 校准自检
    Check_NoNeed,                 //  不需要自检时
    Check_Start,                       //  开机自检
};

class PctGuiThread : public QThread
{
    Q_OBJECT

public:
    explicit PctGuiThread(QThread *parent = 0);
    ~PctGuiThread();    

//protected:
//    void installEventFilter(QObject *Object);
//    virtual bool eventFilter(QObject *Object, QEvent *Event);

signals:

    void LocationOK();                          // 下位机走到最里面发送该信号
    void Move_Motor();                        //  扫描失败发送移动信号

private slots:

    void SetMotor_Move();
    void SetMotor_Start_Location();     // 让下位机走到开始扫描位置
    void ReadScanStation();                  // 轮询读下位机数据（是否走到开始最里面位置）
    bool HandleScanOK(QByteArray ScanData);                     // 如果扫描成功
    void HandleScanError();
    void StartTestWidget();

    void Input_Key_0();
    void Input_Key_1();
    void Input_Key_2();
    void Input_Key_3();
    void Input_Key_4();
    void Input_Key_5();
    void Input_Key_6();
    void Input_Key_7();
    void Input_Key_8();
    void Input_Key_9();
    void Input_Key_OK();
    void Input_Key_Delete();

public:

//     bool isReadOK;
     bool isBarCodeGetOK;
     bool isClickedCloseDoor;
     bool isTimer_CodeStart;
     bool isGetBarCodeDataError;
//     bool isBarCodeDoor;
     CheckType check_type;
//     bool isShouldCheck;                // 正常运行下，测试时，出入舱门时需要自检

     int AddCheck;

     bool isIPAddrErr;
     bool isMessage;
     bool isMessage_child;
     bool isMessage_Save;
     QMessageBox *msgBox;
     QMessageBox *msgBox_child;
     QMessageBox *msgBox_Save;

     bool ThreadState;
     bool isAdmin;
     bool isRoot;
     bool isClicked;
     PctGuiThread *PctGuiP;
     MyWidget *MainWindowP;
     MyWidget *MessageWidget;
     QTcpSocket *LisSocket;
//#########################################//
     Mybutton *Button_pic;
     QPixmap PIX;
//#########################################//
     void run(void);                           // 线程运行
//---------------------------------------------------------------------------------//  开机画面对象
      QProgressBar *ProgressBar;  //开机进度条
      QTimer * UiTimer;
      QTimer *Time_Widget;
      QTimer *CheckTime;
//      int Door_flush;
//      TextEdit *SocketEdit;
      int OBR_index;
      int iMessage;
      int PID_index;
      int OBX_index;
      int QRD_index;

      bool isControl;
      bool isPress;
      bool  isKeyWidget;
      bool isCalibrationKey;
//---------------------------------------------------------------------------------//   登陆界面对象
      MyWidget *LoginWidetP;
      QWidget *Widget;
      QString text;
      myLineEdit *LineEdit_input;
      ComBox *combox;
      QString LoginUserName;
//---------------------------------------------------------------------------------//   主界面对象      
      int Count_test;
      MyWidget *mainWind;
      Mybutton *Button_Door;
      QLabel *Label_net;
      QLabel *Label_SD;
      QLabel *Label_U;                      // U盘
      QLabel *Label_Q;                      // 电量
      QLabel *Label_time;                // 实时时间
      QLabel *Label_station;            // 状态显示
      QLabel *Label_Control;           // 告知控制状态
      QLabel *Label_title;                 // 主界面不显示
//      QString station;

      QTimer *timer_time ;
      QProgressBar *TestBar;           //  测试中进度条
      QWidget *ResWidget;               //  测试结果界面
      QWidget *AnaWidget;
      QPixmap Mainpix_door;
      QPixmap Mainpix_door_open;
      QPixmap Mainpix_door_close;
      QPixmap Mainpix_3;
      QPixmap Mainpix_2;
      QPixmap Mainpix_1;
      QPixmap MainSetpix_Code_2;
      QPixmap MainSetpix_Save;
      QPixmap MainSetpix_Home;
      QPixmap MainSetpix_3;
      QPixmap MainSetpix_2;
      QPixmap MainSetpix_1;
      QPixmap AppSetpix_3;
      QPixmap AppSetpix_2;
      QPixmap AppSetpix_1;
      QPixmap pix_Usr;
      QPixmap pix_Pass;
      QPixmap Selectpix_3;
      QPixmap Selectpix_2;
      QPixmap Selectpix_1;
      QPixmap Adduserpix;
      QPixmap Deluserpix;
      QPixmap Passchgepix;
      QPixmap Calibpix;
      QPixmap Reportpix;
      QPixmap Linepix;
      QPixmap QCpix;
      QPixmap Nextpix;
      QPixmap Startpix;
      QPixmap Printpix;
      QPixmap Delpix;
      QImage Clockpix;

      int iCount_ID;                            //  样本ID界面用来记录Item个数
      int iChange[11];
      int iChange_socket;
      int iCount_icon;
      int iPower;
      int Door_State;                          //  记录仓门打开或者关闭的状态
      QTimer *door_time;
      int Door_flush;
      bool isDoorClicked;
      bool isCanOpenDoor;
      bool isCalib;
      int state_door;
//--------------------------------------------//  主界面刷新显示图标
      QImage result_net[2];
      QImage result_U;
      QImage result_SD;
      QImage result_Q[5];
      QImage image_net[2];
      QImage image_U;
      QImage image_SD;
      QImage image_Q[5];
      QImage image_Socket;
      QImage result_socket;
//---------------------------------------------------------------------------------//  设置界面对象
      Mybutton *DevButton_dot;
      QTimer *Time_Update;

      QString isUpdateOK;
      QString UpDatebase;
      bool isDatebaseOK;
      UpdaType Update_type;
      QProgressBar *UpdateBar;
      Mybutton *Out_app;
      Mybutton *Up_Dateout;
      Mybutton *Out_config;
      QLabel * Lab_datout;
      QLabel *Lab_outapp;
      QLabel *Lab_outcon;
      QLabel *Label_Update;

//      QLabel *Label_C_show;
//      QLabel *label_deviation_show;
//      QLabel *Label_DX_show;

      myLineEdit *Edit_IP;
      myLineEdit *Edit_LisIP;
      myLineEdit *Edit_LisPort;
      DateEdit *Edit_data;
      TimeEdit *Edit_time;
      ComBox *box_Lan;
      CheckBox *Box_Send;
      CheckBox *Box_QRcode;

      bool isSave;
      bool isUpdate;
      bool isSaveDev;
      QWidget *UpWidget;
      QWidget *SetWidget;
      QWidget *COSWidget;
      //------------------Add------------------//
      TextEdit *CaliText;
      QWidget *Widget_contrl;                          //  试剂卡校准界面  
      myLineEdit *Edit_T1;
      myLineEdit *Edit_T2;
      myLineEdit *Edit_T3;
      myLineEdit *Edit_T4;
      myLineEdit *Edit_C1;
      myLineEdit *Edit_C2;
      myLineEdit *Edit_C3;
      myLineEdit *Edit_C4;
      //-----------------------------------------//
      QWidget *UsrWidget;                               //    用户管理
      QWidget *USERWidget;                            //    账户管理
      QWidget *UserKeyWidget;                       //    账户管理带字母键盘
      QWidget *PassKeyWidget;                       //    账户管理密码键盘
      QWidget *NewKeyWidget;                       //    账户申请密码键盘
      QWidget *Widget_num;
      MyGroup*Group_COS;
      QTableWidgetItem *item[20];
      QTableWidgetItem *item_text;
      QTableWidgetItem *Item_User[10];
      QTableWidgetItem *Item_Pass[10];
//      QTableWidget *table_num;
//      QTableWidget *Table_Use;
      MyTableWidget *table_num;
      MyTableWidget *Table_Use;
      myLineEdit *Edit_pro;
      myLineEdit *UserEdit;
      myLineEdit *PassEdit;
      myLineEdit *Edit_last;
      myLineEdit *Edit_max;
      myLineEdit *Edit_made;
      myLineEdit *Edit_remark;
      QLabel *Ltext;
      QLabel *Label_ADD;
      MyRadioButton *Radio_alon;
      MyRadioButton *Radio_four;
      QString Str_Flag;
      QString Str_line;
      QString Pass1;
      QString Pass2;
      QString User_old;
      QString Pass_old;
      QString User_New[10];
      QString Pass_New[10];
      QString BatNu[20];

      int iCount_User;                                     //    记录当前用户个数
      int iTime;                                                 //    记录密码输错次数
      int iRow;                                                  //    记录账户管理表格选中的行号
      int iEditTime;                                          //    记录Edit_time时间改变次数
      int iDate;                                                 //    记录Edit_Date时间改变次数
      QString OldUserData;
//---------------------------------------------------------------------------------//  试验界面对象
      Results RES_Data;
      myLineEdit *Edit_pla;
      QWidget *TestWidget;             // 试验主界面
      QWidget *SamIDWidget;        // 样本ID界面
      QWidget *InfoWidget;            //  信息确认界面
      QWidget *KeyWidget;
      QWidget *TestKeyWidget;
      QWidget *TestNumKey;          // 样本信息输入数字键盘
      QWidget *DevNumKey;          //  设备管理纯数字键盘
      QWidget *CalibrationKey;     //  校准界面数字键盘
      QWidget *TimeNumKey;       // 倒计时输入键盘
      QWidget *CardKeyWidget;    // 输入试剂卡批号键盘

      myLineEdit *Edid_LOT ;
      QTimer *Timer_ScanLOT;
      MyGroup *GroupSam;
      MyGroup *GroupBat;

      QString TextDat;
      QString TextShow;
      QString TC_Value;

      QString Str_Pre;
      QString CSV_information;

      MyRadioButton *Radio_Pla1;
      MyRadioButton *Radio_Pla2;

      MyRadioButton *Radio_fasting;
      MyRadioButton *Radio_Post;
      CheckBox *Check_PPI;

      TextEdit *ResText;
      QTime Num_now;
      QTime Ttime;
      QString Stime;
      int iTimer;
      bool isStart;
      bool isCount;
      bool isNext;
      char dat[50];

//      MyGroup *Group_time;
      QTimer *LCD_time;
//      QLabel *Label_num;
      QLabel *Label_Ti;
      QLabel *Label_Mea;
      myLineEdit *Edit_Mea;

      QLabel *Label_Sam_show;
      QLabel *Label_Sam_1;
      QLabel *Label_Sam_2;
      QLabel *Label_Sam_3;
      QLabel *Label_Sam_4;

      Mybutton *SamBut_next;
      myLineEdit *Edit_key;
      myLineEdit *SamEdit;

//      QLabel *PGI;
//      QLabel *PGII;
//      QLabel *PG;
//      QLabel *G17;

      QLabel *Label_T1;
      QLabel *Label_C1;
      QLabel *Label_T2;
      QLabel *Label_C2;
      QLabel *Label_T3;
      QLabel *Label_C3;
      QLabel *Label_T4;
      QLabel *Label_C4;

      QLabel *Label_resUseShow;
      QLabel *Label_ReaShow;
      QLabel *Label_War;
      QLabel *Label_testing;
      QLabel *Label_ResShow;
      QLabel *Label_InfoSam_show;
      QLabel *Label_InfoTit;
      QLabel *Label_CCTTShow;
      QLabel *Label_Pro2;
      QLabel *Label_CCTT2;
      QLabel *Label_unit2;

      QLabel *Label_Pro3 ;
      QLabel *Label_CCTT3 ;
      QLabel *Label_unit3;

      QLabel *Label_Pro4;
      QLabel *Label_CCTT4;
      QLabel *Label_unit4;    

      MyRadioButton  *Radio_after;
      MyRadioButton  *Radio_now;
      QLabel *Label_unitShow;
      QLabel *Label_proShow;
      Mybutton *InfoBut_ret;
      Mybutton *InfoBut_start;
      QTableWidget *Table_Sam;
      QTableWidgetItem *item_sam[20];
      myLineEdit *Edit_Bat;
      myLineEdit *Edit_Batitem;
      myLineEdit *Edit_Battime;
      myLineEdit *Edit_Batdate;
      QString Text_ID[20];
      QString Sam_ID;
      QString Num;
      QString SamID;
      QString SamIDShow;
      QString LineSam;

      QString gas;
      QString gas1;
      QString gas2;
      QString gas3;
      QString gas4;

      int iBat;                                       // 批号设置表格行数
//      int CurveFlag;
      QLabel *Label_HB;                    //PG1
      QLabel *Label_PG2 ;                 //PG2
      QLabel *Label_Hb ;                   //Hp
      QLabel *Label_17 ;                    //G17
      QwtPlot  *AqwtplotP;               //坐标系A
      QwtPlot  *BqwtplotP;
      QwtPlot  *CqwtplotP;
      QwtPlot  *DqwtplotP;
      QwtPlotCurve * AyvLine;         //原值曲线
      QwtPlotCurve * ByvLine;         //原值曲线
      QwtPlotCurve * CyvLine;
      QwtPlotCurve * DyvLine;

//---------------------------------------------------------------------------------//  查询界面对象
      int iShow;
      int iPage;
      int PageAll;
      int PageNow;
      int iMode;
      int iOver;

      QStringList Res_list;
      int Select_Res_show;
      int Select_Res_All;

      QLabel *Label_cou;
      QList<QTableWidgetItem *> ResList;
      QString TC_Text;
      QString Pro_text;

      bool isSelectRes;

      TextEdit *SelectText;
      QWidget *SelecWidget;
      QWidget *SelecLineWidget;
      MyGroup *Group_SetInfor;
      QWidget *Group_Infor;
      QWidget *Group_Res;
      QWidget *Group_More;

      MyGroup *Group_Key;
      MyRadioButton *RadioPtID;
      MyRadioButton *RadioDate;
//      MyRadioButton *RadioTest;
      MyTableWidget *Table_Res;
      QTableWidgetItem *Item_ID;
      myLineEdit *Edit_Select;
      QWidget *SelecKeyWidget;
      Mybutton *retuButton;
      Mybutton *DownButton;
      myLineEdit *Edit_Sel;
      Mybutton *pushButton[26];
      Mybutton *pushButton_Up;
      Mybutton *But_PgUp;
      Mybutton *But_PgDown;
      QImage Ima_up[2];
      QImage Res_up[2];
      int iSelect;
      QString SelectID;

      QString Text_Select;
      QString Print_Text;
      QString Pro_Item;

      QByteArray selec1;
      QByteArray selec2;
      QByteArray selec3;
      QByteArray selec4;

      QString ID;
      QString KeyInput;
      char SelectKey[26];
//      int KeyStation;
      unsigned long KeyStation;
      int iSearCh;

      bool isConnect;
      bool isDisconnect;
      bool isUpload;
//      QString MSH_Mess;
      QString SEND_Sam;
      QString SEND_Res;

      QString OBX_4;  // PG1 项目名称
      QString OBX_5;  // PG1 结果浓度
      QString OBX_6;  // PG1 结果值的单位
      QString OBX_7;  // PG1 参考范围
      QString OBX_8;  // PG1 结果标志L-偏低H-偏高N-正常

      QString OBX_4_1;
      QString OBX_5_1;
      QString OBX_6_1;
      QString OBX_7_1;
      QString OBX_8_1;

      QString OBX_4_b;
      QString OBX_5_b;
      QString OBX_6_b;
      QString OBX_7_b;
      QString OBX_8_b;

      QString OBX_4_2;
      QString OBX_5_2;
      QString OBX_6_2;
      QString OBX_7_2;
      QString OBX_8_2;

      QString OBX_4_3;
      QString OBX_5_3;
      QString OBX_6_3;
      QString OBX_7_3;
      QString OBX_8_3;

      QwtPlotCurve * Line1;
      QwtPlotCurve * Line2;
      QwtPlotCurve * Line3;
      QwtPlotCurve * Line4;

      int Interpretation_post;

//      MyMessageBox *msgBox;
//      MyMessageBox *msgBox_child;
//      QAbstractButton * ABSbutton;
//---------------------------------------------------------------------------------//   公开的方法
public:
//      int iPic;

      QString GetMSH(QString MSH_head);
      QString GetPID(void);
      QString GetOBR(void);
      QString GetOBX(QString OBX_Data1,QString OBX_Data2,QString OBX_Data3,QString OBX_Data4,QString OBX_Data5);
      QString GetQRD(QString Select_ID);
      QString GetQRF(void);
      QString GetERR(void);

      QStringList CharacterStr(QString SpaceStr);
      void Button_SetPix(Mybutton *button, QPixmap pixmap);
      void SaveSet(void);                      // 保存对仪器的修改
      void LoginWindow(void);            // 登录界面
      void DispStarBmp(void);             // 开机界面
      void CreatWindow(void);            // 主界面
      void CreatSetWidget(void);         // 设置界面
      void CreatTestWidget(void);       //  试验界面
      void CreatInforWidget(void);     //  信息确认
      void CreatSelecWidget(void);     //  查询界面
      //--------------Add---------------//
      void CreatPatientWidget(void);
      void CreatQCWidget(void);
      void CreatSeleResultWidget(void);
      //----------------------------------//
      void CreatResuWidget(void);      // 试验结果界面
      void CreatCOSWidget(void);       // 仪器设置
      void CreatUSERWidget(void);     //  账户设置界面
      void CreatSelectKey(void);          //  带字母的键盘
      void CreatUserKey(void);            //  用户界面带字母键盘
      void CreatTestKey(void);             // 试剂界面键盘
      void CreatSelecLine(void);          // 查询部分曲线图
      void CreatUpdate(void);             //  升级界面
      void CreatDevNumKey(void);     //  设备管理界面，纯数字键盘
      void CreatCalibrationKey(void); //  校准界面，纯数字键盘

      void AnalyTest(void);
      void AnalyValue(void);
      QString ExportCSVFile(void);

public slots:

     void Delayms(uint del);
     void UiTimeOut();
     void ChangeWidget();
     void CheckRunError();
//     void MessageBox(QAbstractButton *);
//#########################################//
//     void pressButton_pic(void);
//     void Showbutton(void);
//#########################################//

private slots:
//---------------------------------------------------------------------------------//  开机界面槽
     void Socket_connected();
     void Socket_disconnected();
     void Socket_Error(QAbstractSocket :: SocketError);

// void pressstartbutton();
// void pressclearbutton();
// void presssendbutton();

//     void UiTimeOut();
//---------------------------------------------------------------------------------//  登陆界面槽
     void showButton_0();
     void showButton_1();
     void showButton_2();
     void showButton_3();
     void showButton_4();
     void showButton_5();
     void showButton_6();
     void showButton_7();
     void showButton_8();
     void showButton_9();
     void showButton_clear();
     void showButton_sure();
//     void presscombox();
//---------------------------------------------------------------------------------//   主界面槽
     void flush_door_time(void);
     void flush_time(void);
     void flush_icon(void);
     void pressButton_1(void);
     void pressButton_2(void);
     void pressButton_3(void);
     void pressButton_Door(void);
     void Voice_pressKey(void);               // 按键声音
     void Voice_ScanSucess(void);           // 扫描正确声音
     void Voice_ScanError(void);             // 扫描失败声音
     void Voice_LowBattery(void);           // 低电量声音
     void Voice_ChangStation(void);       // 状态改变声音
//---------------------------------------------------------------------------------//   设置界面槽
     void pressCalibrationOK(void);
     void pressDevButton_0(void);
     void pressDevButton_1(void);
     void pressDevButton_2(void);
     void pressDevButton_3(void);
     void pressDevButton_4(void);
     void pressDevButton_5(void);
     void pressDevButton_6(void);
     void pressDevButton_7(void);
     void pressDevButton_8(void);
     void pressDevButton_9(void);
     void pressDevButton_OK(void);
     void pressDevButton_Del(void);
     void pressDevButton_dot(void);
     void pressBut_Inf_restore(void);
     void pressUsrBut_save(void);
     void pressEdit_hostIP(void);
     void pressEdit_LisIP(void);
     void pressEdit_LisPort(void);
     void pressButton_Save(void);
     void pressBut_dev_cali(void);
     void pressOut_app(void);
     void pressOut_config(void);
     void pressUp_Config(void);
     void pressUp_Dateout(void);
     void pressUp_Datein(void);
     void pressUp_update(void);
     void pressUp_Ret(void);
     void pressUp_Home(void);
     void pressUp_DateGu(void);
     void pressSamEdit(void);
     void pressButton_Infor(void);
     void pressBut_Inf_home(void);
     void pressDev_Home(void);
     void Press_Set_home(void);
     void pressUSR_Home(void);
     void pressChoNumBut_ret(void);
     void pressChoNumBut_net(void);
     void pressButton_Usr_ret(void);
     void pressButton_return(void);
     void pressButton_NumRet(void);
     void ClickedItem_Set(void);
     void pressbutton_close(void);
     void pressButton_Pass(void);
     void pressButton_Usr(void);
     void pressButt_Ret(void);
     void pressBut_Add(void);
     void pressBut_Set(void);
     void pressBut_Del(void);
     void pressButton_sweep(void);
     void ClickedItem_Use(void);
     void pressbox_Lan(void);
     void pressEdit_time(void);
     void pressEdit_data(void);
     void pressButton_set(void);
     void pressButton_dev(void);
     void pressButton_Lot_Home(void);
     void pressButt_Home(void);
     void Time_Update_out(void);
     void ConfigWidget_Home(void);
     void CalibrationDoor(void);
     void Radio_test(void);

//     void pressTestButton(void);
//---------------------------------------------------------------------------------//   试验界面槽
     void pressEdit_Mea(void);
     void pressEdit_pla(void);
     void pressLine_home(void);
     void pressHomeButton(void);
     void LCDtimer(void);
     void pressRadio_Pla1(void);
     void pressRadio_Pla2(void);
     void pressSamBut_ret(void);
     void pressSamBut_next(void);
     void pressInfoBut_ret(void);
     void pressInfoBut_home(void);
     void pressSamBut_home(void);
     void pressInfoBut_start(void);
     void pressExitButton(void);
     void pressAnaButton(void);
     void pressRetButton(void);
     void pressreButton(void);
     void pressSamBut_input(void);
     void pressbutton_ret(void);
     void pressButton_in(void);
     void pressButton_out(void);
     void presspritButton(void);
     void pressRadio_after(void);
     void pressRadio_now(void);
     void presscombox_Info(void);
     void pressUpload_Button(void);
     //-------------------------Add-----------------------//
     void pressButtonContrl(void);
     void pressBut_con_ret(void);
     void pressBut_con_home(void);
     void pressButton_start(void);
//     void pressButton_Calib_C(void);

     void pressEdit_T1(void);
     void pressEdit_T2(void);
     void pressEdit_T3(void);
     void pressEdit_T4(void);
     void pressEdit_C1(void);
     void pressEdit_C2(void);
     void pressEdit_C3(void);
     void pressEdit_C4(void);
     void press_SaveTC_Button(void);
     void press_Resote_TC(void);
     //-----------------------------------------------------//
//---------------------------------------------------------------------------------//   查询界面槽
     void pressBut_PgUp(void);
     void pressBut_PgDown(void);
     void pressSqlPrint(void);
     void pressDelButton(void);
     void SelectOK(void);
     void pressSelecLin_Ret(void);
     void pressMore_line(void);
     void pressSelecLin_Home(void);
     void pressBut_Inf_ret(void);
     void pressretuButton(void);
     void pressSaveButton(void);
     void pressMore_Ret(void);
     void pressMore_Home(void);
     void pressRadio(void);
     void pressSelButton(void);
     void ClickedItemTable_Res(void);
     void CloseKey(void);
     void ClickedHeardview(void);
//--------------------------------------------------------//   查询界面键盘按钮槽
     void presspushButton_Mao(void);
     void presspushButton_del(void);
     void presspushButton_Ret(void);
     void presspushButton_sure(void);
     void pressKeyButton_up(void);
     void pressKeyButton_a(void);
     void pressKeyButton_b(void);
     void pressKeyButton_c(void);
     void pressKeyButton_d(void);
     void pressKeyButton_e(void);
     void pressKeyButton_f(void);
     void pressKeyButton_g(void);
     void pressKeyButton_h(void);
     void pressKeyButton_i(void);
     void pressKeyButton_j(void);
     void pressKeyButton_k(void);
     void pressKeyButton_l(void);
     void pressKeyButton_m(void);
     void pressKeyButton_n(void);
     void pressKeyButton_o(void);
     void pressKeyButton_p(void);
     void pressKeyButton_q(void);
     void pressKeyButton_r(void);
     void pressKeyButton_s(void);
     void pressKeyButton_t(void);
     void pressKeyButton_u(void);
     void pressKeyButton_v(void);
     void pressKeyButton_w(void);
     void pressKeyButton_x(void);
     void pressKeyButton_y(void);
     void pressKeyButton_z(void);
     void pressKeyButton_0(void);
     void pressKeyButton_1(void);
     void pressKeyButton_2(void);
     void pressKeyButton_3(void);
     void pressKeyButton_4(void);
     void pressKeyButton_5(void);
     void pressKeyButton_6(void);
     void pressKeyButton_7(void);
     void pressKeyButton_8(void);
     void pressKeyButton_9(void);
     void pressKeyButton_space(void);
     void pressKeyButton_star(void);    
     void pressKeyButton_dot(void);

private:
    UiWidgetType UiType;             // 窗口标识
    UiWidgetType CalibrationType;
    //#############################################//           Device
public:

//    void RecordTime(QString Time);


     APP_dev *AglDeviceP;
     TwoCodeStr Analy;
     TwoCodeStr AnaFound;
     QStringList Result;
     QString Sam;
     QString Bat;
     bool Code2Aanly(QString Str, TwoCodeStr *Code);
     int BatchTimes;
     bool isBatchError;
     bool isLanguageChange;
     QString DefaultLanguage;
    //#############################################//
    bool isYmaxError;
    bool isCalibration;
    bool isCalibrationFourCard;
    //##################################################//    Sql
public:
     TestSql *Sql;

//----------------------------------------------------------------------------------------------//

    //###################################################//
};




#endif // YIXIN_H
