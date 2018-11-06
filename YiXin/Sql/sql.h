#ifndef TESTSQL_H
#define TESTSQL_H

#include <QtCore/qglobal.h>

#if defined(TestSql_LIBRARY)
#  define TestSqlSHARED_EXPORT Q_DECL_EXPORT
#else
#  define TestSqlSHARED_EXPORT Q_DECL_IMPORT
#endif

#include <QMainWindow>
#include <QThread>
#include <QtSql>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <stdlib.h>
#include <QReadWriteLock>
#include <QMutexLocker>
#include <QMutex>
#include <QFile>
#include <QCoreApplication>


#define SIZE_COMP 100*1024
#define DOU_SIZE (sizeof(double))

enum SQL_ERROE
{
    ERROR_OK = -2,
    ERROR_ERR
};

struct ResultIndex//诊断结果索引项结构
{
//    QString Id;//数据库ID，递增
    QString SamId;//样本ID
    QString Item;//项目
    QString Porperty;//结果
    QString Date;//日期
};

struct Results//诊断结果数据体结构
{
    int Id;//数据库ID，递增
    char SamId[16];//样本ID
    char Item[16];//项目
    char Porperty[160];//存放CSV文件的个别信息;
    char Date[32];//日期
    char Report[2048];//诊断报告字符串
    char DatBuf[16000];//数据体,double
};

typedef struct Message
{
    char Filename[7];
    int FileCount;
} FileMessage;

typedef struct code2d{
    QString Code2D;
}Code2d;

class TestSql : public QThread
{
    Q_OBJECT

public:
    TestSql(QThread *parent = 0);
    ~TestSql();
    void run();

public:

    bool isLimit;
    bool init_BatchSql(QString Key);
    bool InsertBatchSql(Code2d *p2dcode, int UseTime);                                       //插入一条批号记录
    int CountBatchSql();                                                                                               //获得批号db文件记录个数
    bool ListBatchSql(QByteArray &List, int i);                                                           //列出当前批号数据库中的所有二维码
    bool SelectBatchSql(QByteArray &Sel, int Index, QString IndexText);              //按索引信息查询批号数据库
    bool GetDetailInforBatchSql(QString &buf, QString yiweima);                          //一位码匹配二维码
    bool GetPiliang(QString &buf, QString Batch);                                                    // 根据批号返回二维码
    bool DeleteBatchSql(QString Batch);                                                                    //删除一条批号记录
    bool GetBatchUseTime(QString Yiweima, int &UseTime);                                  // 获取对应批号的测试次数
    bool UpdateBatUseTime(QString Yiweima,int CurrentTimes);                          // 更新批号数据库的测试次数

    bool init_ResultSql(void);
    int CountResultSql(void);                                                                                      // 获得结果数据库当前的数据条数
    bool InsertResultSql(Results *pResult);                                                              // 插入数据库
    bool ListResultSql(ResultIndex &List, int inDex);                                               // 根据ID，查询出相应的结果
    bool SelectResultSql(QString &Sel, int Index, QString IndexText);                  // 根据如参的样本，项目等查询结果
    bool DeleteResultSql(int ID);                                                                               // 删除数据库中的某一条数据
    bool GetDetailInforResultSql(Results &ret, int id);
    bool init_DatebaseMessage(void);
    bool SaveFile(void);                                                                                               // 保存文件

private:

    bool RestoreFile(QString FileName);                                                                   // 文件还原
    bool RestoreSqlMessageFile(void);
    bool UpdateResultSql(QSqlDatabase *DB, int OldId, int NewId);
    int SelecDatebase(int Index);
    bool UpdateDatebaseMessage(QString File, int FileCount);
    bool isDatebaseEmpty(void);
    bool GetFileMessage(FileMessage &Message);                       // 获得应该插入数据时的文件与条数
    bool GetShowFile(FileMessage &Message, int iShow);           // 获得应该显示数据时的文件与条数
    bool SaveDatebase(void);
    bool RestoreDatebase(void);
};

#endif // TESTSQL_H
