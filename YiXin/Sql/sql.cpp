#include "sql.h"
#include <stdio.h>

TestSql::TestSql(QThread *parent)
    : QThread(parent)
{
    isLimit = false;
}

TestSql::~TestSql()
{
    this->exit();
    this->quit();
}

void TestSql::run()
{
    qDebug() << "TestSql thread is runing";
}

bool TestSql::init_BatchSql(QString Key)
{
    qDebug() << "init Batch Db";
    QSqlDatabase *BatchDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Batch_connection"+Key));  // 创建数据库链接
    BatchDB->setDatabaseName("./batch.db");
    BatchDB->setUserName("Top1");
    BatchDB->setPassword("2588");
    if(!BatchDB->open())
    {
        qDebug() << "Batch Sql open Error";
        return false;
    }

    QSqlQuery *qurey = new QSqlQuery(*BatchDB);      // QSqlQuery类实现了执行SQL语句
    QStringList List  = BatchDB->tables();                          // 获得该数据库文件中的所有表格信息
    if(0 == List.length())                                                        // 如果当前数据库文件中没有表格
    {
//       bool code = qurey->exec("create table d2code (Batch vchar, Yiweima vchar, Item vchar, Youxiaoshijian vchar, Combatch vchar)");
       bool code = qurey->exec("create table d2code (Batch vchar, Yiweima vchar, Item vchar, Youxiaoshijian vchar, Combatch vchar, USEtime int)");
       if(code == false)
       {
           QString Connect_name = BatchDB->connectionName();
           BatchDB->close();
           delete BatchDB;
           QSqlDatabase::removeDatabase(Connect_name);
           qDebug() << "Creat d2code Error";
           return false;
       }
    }
    QString Connect_name  = BatchDB->connectionName();
    BatchDB->close();
    delete BatchDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return true;
}

bool TestSql::InsertBatchSql(Code2d *p2dcode, int UseTime)
{
    QSqlDatabase *BatchDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Batch_connection"));  // 创建数据库链接
    BatchDB->setDatabaseName("./batch.db");
    BatchDB->setUserName("Top1");
    BatchDB->setPassword("2588");
    if(!BatchDB->open())
    {
        qDebug() << "Batch Sql open Error";
        return false;
    }
    QString code2D = p2dcode->Code2D;
    QStringList d2Data = code2D.split(",");
    QString Batchs =d2Data.at(0);
    QString Yiweimas =d2Data.at(1);
    QString Items =d2Data.at(6);
    QString Youxiaoshijians =d2Data.at(4);

    QSqlQuery *qurey = new QSqlQuery(*BatchDB);
//    qurey->prepare("INSERT INTO d2code (Batch, Yiweima, Item, Youxiaoshijian, Combatch) "
//                         "VALUES (:Batch, :Yiweima, :Item, :Youxiaoshijian, :Combatch)");
    qurey->prepare("INSERT INTO d2code (Batch, Yiweima, Item, Youxiaoshijian, Combatch, USEtime) "
                         "VALUES (:BatchData, :YiweimaData, :ItemData, :YouxiaoshijianData, :CombatchData, :USEtimeData)");
    qurey->bindValue(":BatchData", Batchs);
    qurey->bindValue(":YiweimaData", Yiweimas);
    qurey->bindValue(":ItemData",Items);
    qurey->bindValue(":YouxiaoshijianData", Youxiaoshijians);
    qurey->bindValue(":CombatchData", p2dcode->Code2D);
    qurey->bindValue(":USEtimeData", UseTime);
    if(!qurey->exec())
    {
        QString Connect_name = BatchDB->connectionName();
        BatchDB->close();
        delete BatchDB;
        QSqlDatabase::removeDatabase(Connect_name);
        qDebug() << "Insert Batch Db Error";
        return false;
    }
    QString Connect_name  = BatchDB->connectionName();
    BatchDB->close();
    delete BatchDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return true;
}

int TestSql::CountBatchSql()
{
    int iCount = 0;
    QSqlDatabase *BatchDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Batch_connection"));  // 创建数据库链接
    BatchDB->setDatabaseName("./batch.db");
    BatchDB->setUserName("Top1");
    BatchDB->setPassword("2588");
    if(!BatchDB->open())
    {
        qDebug() << "Batch Sql open Error";
        return false;
    }
    QSqlQuery *qurey = new QSqlQuery(*BatchDB);
    bool  ok = qurey->exec("select * from d2code");
    if(ok == false)
    {
        QString Connect_name  = BatchDB->connectionName();
        BatchDB->close();
        delete BatchDB;
        QSqlDatabase::removeDatabase(Connect_name);
        qDebug() << "Count batch error, because " << qurey->lastError().text();
        return ERROR_ERR;
    }
    while(qurey->next())
    {
        iCount ++;
    }
    QString Connect_name  = BatchDB->connectionName();
    BatchDB->close();
    delete BatchDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return iCount;
}

bool TestSql::ListBatchSql(QByteArray &List, int i)
{
    int iCount = 0;
    int n = 0;
    QSqlDatabase *BatchDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Batch_connection"));  // 创建数据库链接
    BatchDB->setDatabaseName("./batch.db");
    BatchDB->setUserName("Top1");
    BatchDB->setPassword("2588");
    if(!BatchDB->open())
    {
        qDebug() << "Batch Sql open Error";
        return false;
    }
    QSqlQuery *qurey = new QSqlQuery(*BatchDB);
//    bool ok = qurey->exec("select Batch from d2code");
    bool ok = qurey->exec("select Batch, USETime from d2code");
    if(ok == false)
    {
        QString Connect_name  = BatchDB->connectionName();
        BatchDB->close();
        delete BatchDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return false;
    }
    else
    {
        while(qurey->next())
       {
             iCount ++;
       }
       qurey->seek(iCount );
       while(qurey->previous())
       {
           if(n == i)
           {
               break;
           }
           QString Batchs = qurey->value(0).toString();
           int USeTime = qurey->value(1).toInt();
           Batchs += "//" + QString::number(USeTime) + "///";
//          QString Batchs = qurey->value(0).toString() + "///";
          List.append(Batchs.toLatin1().data(),Batchs.length());
          n++;
       }
       QString Connect_name  = BatchDB->connectionName();
       BatchDB->close();
       delete BatchDB;
       QSqlDatabase::removeDatabase(Connect_name);
       return true;
    }
}
/*  根据批号，一维码，项目名称其中的一项来查询批号数据库，返回该条件对应的一维码，批号，有效时间，项目的总的信息
 *   返回字串字符
 */
bool TestSql::SelectBatchSql(QByteArray &Sel,int Index,QString IndexText)
{
    QSqlDatabase *BatchDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Batch_connection"));  // 创建数据库链接
    BatchDB->setDatabaseName("./batch.db");
    BatchDB->setUserName("Top1");
    BatchDB->setPassword("2588");
    if(!BatchDB->open())
    {
        qDebug() << "Batch Sql open Error";
        return false;
    }
    QSqlQuery *query = new QSqlQuery(*BatchDB);
    QString Indexn = IndexText.replace("*","%");
    bool ok = false;
    switch(Index)
    {
        case 0:
        {
            ok = query->exec(QObject::tr("select Batch, Yiweima, Item, Youxiaoshijian from d2code where Batch like '%1'").arg(Indexn));
            if(ok)
            {
                while(query->next())
                {
                    QString Batchs = query->value(0).toString()+",";
                    QString Yiweimas=query->value(1).toString()+",";
                    QString Items=query->value(2).toString()+",";
                    QString Youxiaoshijians=query->value(3).toString()+"@";

                    Sel.append(Batchs.toLatin1().data(),Batchs.length());
                    Sel.append(Yiweimas.toLatin1().data(),Yiweimas.length());
                    Sel.append(Items.toLatin1().data(),Items.length());
                    Sel.append(Youxiaoshijians.toLatin1().data(),Youxiaoshijians.length());
                }
                QString Connect_name  = BatchDB->connectionName();
                BatchDB->close();
                delete BatchDB;
                QSqlDatabase::removeDatabase(Connect_name);
                return true;
            }
            else
            {
                QString Connect_name  = BatchDB->connectionName();
                BatchDB->close();
                delete BatchDB;
                QSqlDatabase::removeDatabase(Connect_name);
                return false;
            }
            break;
        }
        case 1:
        {
            ok=query->exec(QObject::tr("select Batch, Yiweima, Item, Youxiaoshijian from d2code where Yiweima like '%1'").arg(Indexn));
            if(ok)
            {
                while(query->next())
                {
                    QString Batchs = query->value(0).toString()+",";
                    QString Yiweimas=query->value(1).toString()+",";
                    QString Items=query->value(2).toString()+",";
                    QString Youxiaoshijians=query->value(3).toString()+"@";

                    Sel.append(Batchs.toLatin1().data(),Batchs.length());
                    Sel.append(Yiweimas.toLatin1().data(),Yiweimas.length());
                    Sel.append(Items.toLatin1().data(),Items.length());
                    Sel.append(Youxiaoshijians.toLatin1().data(),Youxiaoshijians.length());
                 }
                QString Connect_name  = BatchDB->connectionName();
                BatchDB->close();
                delete BatchDB;
                QSqlDatabase::removeDatabase(Connect_name);
                return true;
            }
            else
            {
                QString Connect_name  = BatchDB->connectionName();
                BatchDB->close();
                delete BatchDB;
                QSqlDatabase::removeDatabase(Connect_name);
                return false;
            }
            break;
        }
        case 2:
        {
            ok = query->exec(QObject::tr("select Batch, Yiweima, Item, Youxiaoshijian from d2code where Item like '%1'").arg(IndexText));
            if(ok)
            {
                while(query->next())
                {
                    QString Batchs = query->value(0).toString()+",";
                    QString Yiweimas=query->value(1).toString()+",";
                    QString Items=query->value(2).toString()+",";
                    QString Youxiaoshijians=query->value(3).toString()+"@";

                    Sel.append(Batchs.toLatin1().data(),Batchs.length());
                    Sel.append(Yiweimas.toLatin1().data(),Yiweimas.length());
                    Sel.append(Items.toLatin1().data(),Items.length());
                    Sel.append(Youxiaoshijians.toLatin1().data(),Youxiaoshijians.length());
                }
                QString Connect_name  = BatchDB->connectionName();
                BatchDB->close();
                delete BatchDB;
                QSqlDatabase::removeDatabase(Connect_name);
                 return true;
            }
            else
            {
                QString Connect_name  = BatchDB->connectionName();
                BatchDB->close();
                delete BatchDB;
                QSqlDatabase::removeDatabase(Connect_name);
                return false;
            }
            break;
        }
        case 3:
        {
            ok=query->exec(QObject::tr("select Batch, Yiweima, Item, Youxiaoshijian from d2code where Youxiaoshijian like '%1'").arg(IndexText));
            if(ok)
            {
                while(query->next())
                {
                    QString Batchs = query->value(0).toString()+",";
                    QString Yiweimas=query->value(1).toString()+",";
                    QString Items=query->value(2).toString()+",";
                    QString Youxiaoshijians=query->value(3).toString()+"@";

                    Sel.append(Batchs.toLatin1().data(),Batchs.length());
                    Sel.append(Yiweimas.toLatin1().data(),Yiweimas.length());
                    Sel.append(Items.toLatin1().data(),Items.length());
                    Sel.append(Youxiaoshijians.toLatin1().data(),Youxiaoshijians.length());
                }
                QString Connect_name  = BatchDB->connectionName();
                BatchDB->close();
                delete BatchDB;
                QSqlDatabase::removeDatabase(Connect_name);
                return true;
            }
            else
            {
                QString Connect_name  = BatchDB->connectionName();
                BatchDB->close();
                delete BatchDB;
                QSqlDatabase::removeDatabase(Connect_name);
                return false;
            }
            break;
        }
    }
    QString Connect_name  = BatchDB->connectionName();
    BatchDB->close();
    delete BatchDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return true;
}

bool TestSql::GetDetailInforBatchSql(QString &buf,QString yiweima)
{
    QSqlDatabase *BatchDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Batch_connection"));  // 创建数据库链接
    BatchDB->setDatabaseName("./batch.db");
    BatchDB->setUserName("Top1");
    BatchDB->setPassword("2588");
    if(!BatchDB->open())
    {
        qDebug() << "Batch Sql open Error";
        return false;
    }
    buf.clear();
    QSqlQuery *query = new QSqlQuery(*BatchDB);
    bool ok = query->exec(QObject::tr("select Combatch from d2code where Yiweima='%0'").arg(yiweima));
    if(ok)
    {
        if(!query->first())
        {
            QString Connect_name  = BatchDB->connectionName();
            BatchDB->close();
            delete BatchDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "GetDetailInforBatchSql  value Error, because " << query->lastError().text();
            return false;
        }
        buf = query->value(0).toString();
         if(buf.isEmpty())
         {
             QString Connect_name  = BatchDB->connectionName();
             BatchDB->close();
             delete BatchDB;
             QSqlDatabase::removeDatabase(Connect_name);
             return false;
         }
         else
         {
             QString Connect_name  = BatchDB->connectionName();
             BatchDB->close();
             delete BatchDB;
             QSqlDatabase::removeDatabase(Connect_name);
             return true;
         }
    }
    else
    {
        QString Connect_name  = BatchDB->connectionName();
         BatchDB->close();
         delete BatchDB;
         QSqlDatabase::removeDatabase(Connect_name);
         return false;
    }
}

bool TestSql::GetPiliang(QString &buf, QString Batch)
{
    QSqlDatabase *BatchDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Batch_connection"));  // 创建数据库链接
    BatchDB->setDatabaseName("./batch.db");
    BatchDB->setUserName("Top1");
    BatchDB->setPassword("2588");
    if(!BatchDB->open())
    {
        qDebug() << "Batch Sql open Error";
        return false;
    }
    buf.clear();
    QSqlQuery *query = new QSqlQuery(*BatchDB);
    bool ok = query->exec(QObject::tr("select Combatch from d2code where Batch='%0'").arg(Batch));
    if(ok)
    {
        if(!query->first())
        {
            QString Connect_name  = BatchDB->connectionName();
            BatchDB->close();
            delete BatchDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "GetDetailInforBatchSql  value Error, because " << query->lastError().text();
            return false;
        }
        buf = query->value(0).toString();
         if(buf.isEmpty())
         {
             QString Connect_name  = BatchDB->connectionName();
             BatchDB->close();
             delete BatchDB;
             QSqlDatabase::removeDatabase(Connect_name);
             return false;
         }
         else
         {
             QString Connect_name  = BatchDB->connectionName();
             BatchDB->close();
             delete BatchDB;
             QSqlDatabase::removeDatabase(Connect_name);
             return true;
         }
    }
    else
    {
        QString Connect_name  = BatchDB->connectionName();
         BatchDB->close();
         delete BatchDB;
         QSqlDatabase::removeDatabase(Connect_name);
         return false;
    }
}

bool TestSql::DeleteBatchSql(QString Batch)
{
    QSqlDatabase *BatchDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Batch_connection"));  // 创建数据库链接
    BatchDB->setDatabaseName("./batch.db");
    BatchDB->setUserName("Top1");
    BatchDB->setPassword("2588");
    if(!BatchDB->open())
    {
        qDebug() << "Batch Sql open Error";
        return false;
    }
    QSqlQuery *query = new QSqlQuery(*BatchDB);
    bool ok = query->exec(QObject::tr("delete from d2code where Batch = '%1'").arg(Batch));
    if(ok)
    {
        QString Connect_name  = BatchDB->connectionName();
        BatchDB->close();
        delete BatchDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return true;
    }
    else
    {
        QString Connect_name  = BatchDB->connectionName();
        BatchDB->close();
        delete BatchDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return false;
    }
}

bool TestSql::GetBatchUseTime(QString Yiweima, int &UseTime)
{
    QSqlDatabase *BatchDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Batch_connection"));  // 创建数据库链接
    BatchDB->setDatabaseName("./batch.db");
    BatchDB->setUserName("Top1");
    BatchDB->setPassword("2588");
    if(!BatchDB->open())
    {
        qDebug() << "Batch Sql open Error";
        return false;
    }
    QSqlQuery *query = new QSqlQuery(*BatchDB);
    bool ok = query->exec(QObject::tr("select USEtime from d2code where Yiweima = '%0'").arg(Yiweima));
    if(ok)
    {
        if(query->first() == false)
        {
            QString Connect_name = BatchDB->connectionName();
            BatchDB->close();
            delete BatchDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "GetBatchUseTime value Error, because " << query->lastError().text();
            return false;
        }
         UseTime = query->value(0).toInt(&ok);
         if(ok == false)
         {
             QString Connect_name  = BatchDB->connectionName();
             BatchDB->close();
             delete BatchDB;
             QSqlDatabase::removeDatabase(Connect_name);
             qDebug() << "457 Value to int Error";
             return false;
         }
         else
         {
             QString Connect_name  = BatchDB->connectionName();
             BatchDB->close();
             delete BatchDB;
             QSqlDatabase::removeDatabase(Connect_name);
             return true;
         }
    }
    else
    {
        QString Connect_name  = BatchDB->connectionName();
         BatchDB->close();
         delete BatchDB;
         QSqlDatabase::removeDatabase(Connect_name);
         return false;
    }
}

bool TestSql::UpdateBatUseTime(QString Yiweima,int CurrentTimes)
{
    QSqlDatabase *BatchDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Batch_connection"));  // 创建数据库链接
    BatchDB->setDatabaseName("./batch.db");
    BatchDB->setUserName("Top1");
    BatchDB->setPassword("2588");
    if(!BatchDB->open())
    {
        qDebug() << "Batch Sql open Error";
        return false;
    }
    QSqlQuery *query = new QSqlQuery(*BatchDB);
    bool ok = query->exec(QObject::tr("update d2code set USEtime = '%0' where Yiweima = '%1'").arg(CurrentTimes).arg(Yiweima));
    if(ok == false)
    {
        QString Connect_name = BatchDB->connectionName();
        BatchDB->close();
        delete BatchDB;
        QSqlDatabase::removeDatabase(Connect_name);
        qDebug() << "GetBatchUseTime value Error, because " << query->lastError().text();
        return false;
    }

    QString Connect_name = BatchDB->connectionName();
    BatchDB->close();
    delete BatchDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return true;
}

bool TestSql::init_ResultSql()
{
    qDebug() << "init ResultSql";
    int iGoto = 2;
    bool ok = false;
    FileMessage Sql_Message;
    if(!GetFileMessage(Sql_Message))
    {
        qDebug() << "------666 Get file Message error";
        return false;
    }
    QString Current = (QString)Sql_Message.Filename;
    int Result_count = CountResultSql();
    QSqlDatabase *ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Resultbase_connection"));
    loop :
    iGoto--;
    ResultDateDB->setDatabaseName("./Yixin1.db");
    ResultDateDB->setUserName("Top2");
    ResultDateDB->setPassword("2345");
    if(!ResultDateDB->open())
    {
        if(Current == "Yixin1")
        {
            if(!RestoreSqlMessageFile())
            {
                QString Connect_name  = ResultDateDB->connectionName();
                ResultDateDB->close();
                delete ResultDateDB;
                QSqlDatabase::removeDatabase(Connect_name);
                qDebug() << "Restore file open error";
                return false;
            }
        }
        if(!RestoreFile("Yixin1"))
        {
            QString Connect_name  = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "Restore file open error";
            return false;
        }
        if(iGoto != 0)
        {
            goto loop;
        }
        else
        {
            QString Connect_name  = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "Restore file open error";
            return false;
        }
    }
    QStringList List  = ResultDateDB->tables();                                     // 获得该数据库文件中的所有表格信息
    if(0 == List.length() && Result_count == 0)                                      // 结果数据库表格为空，并且数据条数为0
    {
        QSqlQuery *query = new QSqlQuery(*ResultDateDB);
//        ok = query->exec(QObject :: tr("create table testresult (ID int primary key, SamId vchar, Item vchar, Property vchar, Date vchar, DetailInfor BLOB)"));
        ok = query->exec(QObject :: tr("create table testresult (ID int primary key, SamId vchar, Item vchar, Property vchar, Date vchar, DetailInfor CLOB)"));
        if(ok == false)
        {
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "306 Creat table teseresult Error";
            return false;
        }
        //---------------------------Add---------------------------//
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        //------------------------------------------------------------//
        return true;
    }
    else if(0 == List.length() && Result_count != 0)                    // 结果数据库表格为空，数据条数不为0
    {
        FileMessage File_message;
        if(!GetFileMessage(File_message))
        {
            qDebug() << "------421 initResult function Get datebase error";
            QString Connect_name  = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
        QString DateFile = (QString)File_message.Filename;
        if(!RestoreSqlMessageFile())
        {
            qDebug() << "------429initResult Restore file Error";
            QString Connect_name  = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
        if(!RestoreFile(DateFile))
        {
            qDebug() << "------436initResult Restore file Error";
            QString Connect_name  = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
    }
    else  if(0 != List.length() && Result_count == 0)                                                                                          // 结果数据库表格不为空， 条数为0条
    {
        qDebug() << "Result_count is " << Result_count;
        qDebug() << "List.length() is " << List.length();
        iGoto = 2;
        Loop_new:
        iGoto--;
        FileMessage File_message;
        if(!GetFileMessage(File_message))
        {
            qDebug() << "init function Get datebase error";\
            //---------------------Add---------------------//
            QString Connect_name  = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            //-----------------------------------------------//
            return false;
        }
        QString DateFile = (QString)File_message.Filename;
        QString FileName = DateFile + ".db";
        QFile *File = new QFile(FileName);
        QString Connect_name  = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Resultbase_connection"+FileName));
        ResultDateDB->setDatabaseName(FileName);
        ResultDateDB->setUserName("Top"+FileName);
        ResultDateDB->setPassword("2345");
        if(!File->exists() || !ResultDateDB->open() )
        {
            if(!RestoreSqlMessageFile())
            {
                qDebug() << "-------------478 initResult RetoreDatebase Error";
                delete File;
                QString Connect_name  = ResultDateDB->connectionName();
                ResultDateDB->close();
                delete ResultDateDB;
                QSqlDatabase::removeDatabase(Connect_name);
                return false;
            }
            if(!RestoreFile(DateFile))
            {
                qDebug() << "AAAA ---- 359 Restore File Error";
                delete File;
                QString Connect_name  = ResultDateDB->connectionName();
                ResultDateDB->close();
                delete ResultDateDB;
                QSqlDatabase::removeDatabase(Connect_name);
                return false;
            }
            if(iGoto != 0)
            {
                goto Loop_new;
            }
            else
            {
                QString Connect_name  = ResultDateDB->connectionName();
                ResultDateDB->close();
                delete ResultDateDB;
                QSqlDatabase::removeDatabase(Connect_name);
                qDebug() << "368 Restore file open error";
                delete File;
                return false;
            }
        }
        QStringList List_table  = ResultDateDB->tables();
        QSqlQuery *query1 = new QSqlQuery(*ResultDateDB);
        if(0 == List_table.length())
        {
            ok = query1->exec(QObject :: tr("create table testresult (ID int primary key, SamId vchar, Item vchar, Property vchar, Date vchar, DetailInfor BLOB)"));
            if(ok == false)
            {
                if(!RestoreSqlMessageFile())
                {
                    qDebug() << "-------------478 initResult RetoreDatebase Error";
                    delete File;
                    QString Connect_name  = ResultDateDB->connectionName();
                    ResultDateDB->close();
                    delete ResultDateDB;
                    QSqlDatabase::removeDatabase(Connect_name);
                    return false;
                }
                if(!RestoreFile(DateFile))
                {
                    qDebug() << "--------initResult function Restore file Error";
                    delete File;
                    QString Connect_name  = ResultDateDB->connectionName();
                    ResultDateDB->close();
                    delete ResultDateDB;
                    QSqlDatabase::removeDatabase(Connect_name);
                    return false;
                }
                ok = query1->exec(QObject :: tr("create table testresult (ID int primary key, SamId vchar, Item vchar, Property vchar, Date vchar, DetailInfor BLOB)"));
                {
                    if(ok == false)
                    {
                        FileMessage File_message;
                        if(!GetFileMessage(File_message))
                        {
                            qDebug() << "init function Get datebase error";
                            delete File;
                            QString Connect_name  = ResultDateDB->connectionName();
                            ResultDateDB->close();
                            delete ResultDateDB;
                            QSqlDatabase::removeDatabase(Connect_name);
                            return false;
                        }
                        QString DateFile = (QString)File_message.Filename;
                        QString FileName = DateFile + ".db";
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Resultbase_connection"+FileName));
                        ResultDateDB->setDatabaseName(FileName);
                        ResultDateDB->setUserName("Top"+FileName);
                        ResultDateDB->setPassword("2345");
                        if(!ResultDateDB->open())
                        {
                            qDebug() << "###550---------------------Open Error";
                            delete File;
                            QString Connect_name  = ResultDateDB->connectionName();
                            ResultDateDB->close();
                            delete ResultDateDB;
                            QSqlDatabase::removeDatabase(Connect_name);
                            return false;
                        }
                        QStringList List_table  = ResultDateDB->tables();
                        if(0 == List_table.length())
                        {
                            query1 = new QSqlQuery(*ResultDateDB);
                            ok = query1->exec(QObject :: tr("create table testresult (ID int primary key, SamId vchar, Item vchar, Property vchar, Date vchar, DetailInfor BLOB)"));
                            if(ok == false)
                            {
                                QString Connect_name  = ResultDateDB->connectionName();
                                ResultDateDB->close();
                                delete ResultDateDB;
                                QSqlDatabase::removeDatabase(Connect_name);
                                qDebug() << "------##560 Error";
                                delete File;
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
    QString Connect_name  = ResultDateDB->connectionName();
    ResultDateDB->close();
    delete ResultDateDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return true;
}

int TestSql::CountResultSql()
{
    int ret = 0;
    for(int i = 1; i <= 25; i++)
    {
        int iData = SelecDatebase(i);
        if(iData == ERROR_ERR)
        {
            return ERROR_ERR;
        }
        ret += iData;
        if(0 == iData)
        {
            break;
        }
    }
    return ret;
}

bool TestSql::InsertResultSql(Results *pResult)
{
    bool ok = false;
    int iGoto = 2;
    int iCount = CountResultSql();
    if(iCount > 5000)
    {
        qDebug() << "392 Not enough storage";
        return false;
    }
    FileMessage file_message;
    if(!GetFileMessage(file_message))
    {
        qDebug() << "Get file Message error";
        return false;
    }
    QString Datebasefile = (QString)file_message.Filename;
    QString FileName = Datebasefile+ ".db";
    int Id = file_message.FileCount + 1;;
    QString Index;
    if(Datebasefile.length() == 6)
    {
        Index = Datebasefile.mid(Datebasefile.length() - 1, 1);
    }
    else if(Datebasefile.length() == 7)
    {
        Index = Datebasefile.mid(Datebasefile.length() - 2, 2);
    }
    int Allfile = Index.toInt(&ok);                                                             // 当前最后一个应该被使用的文件
    if(ok == false)
    {
        qDebug() << "670 Get file message error";
        return false;
    }
    int Allfile_Count = SelecDatebase(Allfile);
    loop:
    QSqlDatabase *ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", FileName));
    iGoto--;
    ResultDateDB->setDatabaseName(FileName);
    ResultDateDB->setUserName("Top"+FileName);
    ResultDateDB->setPassword("2345");

    if(!ResultDateDB->open())
    {
        delete ResultDateDB;
        qDebug() << "------609 open Error";
        if(!RestoreFile(Datebasefile))
        {
            qDebug() << "439 Restore File Error";
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
        if(iGoto != 0)
        {
            goto loop;
        }
        else
        {
            qDebug() << "448 Restore file open error";
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
    }
    QStringList List  = ResultDateDB->tables();
    QSqlQuery *query1 = new QSqlQuery(*ResultDateDB);
    if(List.length() == 0)      // 如果表格为空
    {
        if(Allfile_Count == 0)      // 如果该表格中数据为空
        {
            ok = query1->exec(QObject :: tr("create table testresult (ID int primary key, SamId vchar, Item vchar, Property vchar, Date vchar, DetailInfor BLOB)"));
            if(ok == false)
            {
                ResultDateDB->close();
                QString Connect_name = ResultDateDB->connectionName();
                delete ResultDateDB;
                QSqlDatabase::removeDatabase(Connect_name);
                qDebug() << "427 Creat table teseresult Error";
                return false;
            }
        }
        else                    // 如果表格为空，但数据不为空
        {
            iGoto = 2;
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            if(!RestoreFile(Datebasefile))
            {
                qDebug() << "439 Restore File Error";
                return false;
            }
            ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", FileName));
            ResultDateDB->setDatabaseName(FileName);
            ResultDateDB->setUserName("Top"+FileName);
            ResultDateDB->setPassword("2345");
            if(!ResultDateDB->open())
            {
                qDebug() << "584 打开文件失败";
                QString Connect_name = ResultDateDB->connectionName();
                ResultDateDB->close();
                delete ResultDateDB;
                QSqlDatabase::removeDatabase(Connect_name);
                return false;
            }
            query1 = new QSqlQuery(*ResultDateDB);
        }
    }
    query1->prepare("INSERT INTO testresult (ID, SamId, Item, Property, Date,DetailInfor) "
                    "VALUES (:ID, :SamId, :Item, :Property, :Date, :DetailInfor)");
    pResult->Id = Id;
    query1->bindValue(":ID", pResult->Id);
    query1->bindValue(":SamId", pResult->SamId);
    query1->bindValue(":Item", pResult->Item);
    query1->bindValue(":Property",pResult->Porperty);
    query1->bindValue(":Date", pResult->Date);
    char *Dat = (char *)pResult;
    QByteArray data;
    int index = 0;
    int Result_size = sizeof(Results);
    for(index=0; index < Result_size; index++)
    {
        data.append(Dat[index]);
    }
    QVariant var(data);
    query1->bindValue(":DetailInfor",var);
    ok = query1->exec();
    if(ok == false)
    {
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        qDebug() << "Inser Result Data Error";
        return false;
    }
    if(!UpdateDatebaseMessage(Datebasefile, Id))
    {
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        qDebug() << "457 Update Datebase DB Error";
        return false;
    }
    if(!SaveDatebase())
    {
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        qDebug() << "------ 704 SaveDatebase Error";
        return false;
    }
    QString Connect_name = ResultDateDB->connectionName();
    ResultDateDB->close();
    delete ResultDateDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return true;
}

bool TestSql::ListResultSql(ResultIndex &List, int inDex)
{
    bool ok = false;
    int iGoto = 2;
    FileMessage Sql_Message;
    if(!GetFileMessage(Sql_Message))
    {
        qDebug() << "------666 Get file Message error";
        return false;
    }
    QString Current = (QString)Sql_Message.Filename;
    Loop:
    iGoto--;
    FileMessage file_message;
    if(!GetShowFile(file_message, inDex))
    {
        qDebug() << "Get file Message error";
        return false;
    }
    QString Datebasefile = (QString)file_message.Filename;
    QString FileName = Datebasefile+ ".db";
    int file_Count = file_message.FileCount;
    QSqlDatabase *ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", FileName));
    ResultDateDB->setDatabaseName(FileName);
    ResultDateDB->setUserName("Top"+FileName);
    ResultDateDB->setPassword("2345");
//    QFile *DBFile = new QFile(FileName);
    QFile DBFile(FileName);
    if(!DBFile.exists() || !ResultDateDB->open())
    {
        qDebug() << "-----------------------682 Open filr Error";
        delete ResultDateDB;
        if(Current == Datebasefile)
        {
            qDebug() << "Datebasefile = Current = " << Datebasefile;
            if(!RestoreSqlMessageFile())
            {
                qDebug() << "------------------------707Restore File Error";
                return false;
            }
        }
        if(!RestoreFile(Datebasefile))
        {
            qDebug() << "528 Restore File Error";
            return false;
        }
        if(iGoto != 0)
        {
            qDebug() << "------ 850 goto Loop";
            goto Loop;
        }
        else
        {
            qDebug() << "537 Restore file open error";
            return false;
        }
    }
    QSqlQuery *query1 = new QSqlQuery(*ResultDateDB);
    ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where ID = %0").arg(file_Count));
    if(ok == false)
    {
        qDebug() << "----------------------704 Select Result Error,because " << query1->lastError().text();
        if(Current == Datebasefile)
        {
            if(!RestoreSqlMessageFile())
            {
                qDebug() << "------------------------707Restore File Error";
                return false;
            }
        }
        if(!RestoreFile(Datebasefile))
        {
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "596 Restore File Error";
            return false;
        }
        FileMessage file_message;
        if(!GetShowFile(file_message, inDex))
        {
            qDebug() << "Get file Message error";
            return false;
        }
        QString Datebasefile = (QString)file_message.Filename;
        QString FileName = Datebasefile+ ".db";
        int file_Count = file_message.FileCount;
        qDebug() << "----------error  file_Count is " << file_Count;
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", FileName));
        ResultDateDB->setDatabaseName(FileName);
        ResultDateDB->setUserName("Top"+FileName);
        ResultDateDB->setPassword("2345");
        if(!ResultDateDB->open())
        {
            qDebug() << "606 open Restore file error";
            return false;
        }
        QSqlQuery *query = new QSqlQuery(*ResultDateDB);
        ok = query->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where ID = %0").arg(file_Count));
        if(ok == false)
        {
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "------792Select Restore filr Error";
            return false;
        }
        else
        {
            if(query->first())
            {
                 List.Item  = query->value(2).toString();
                 List.SamId  = query->value(1).toString();
                 List.Porperty  = query->value(3).toString();
                 List.Date  = query->value(4).toString();
                 QString Connect_name = ResultDateDB->connectionName();
                 ResultDateDB->close();
                 delete ResultDateDB;
                 QSqlDatabase::removeDatabase(Connect_name);
                 return true;
             }
            else
            {
                QString Connect_name = ResultDateDB->connectionName();
                ResultDateDB->close();
                delete ResultDateDB;
                QSqlDatabase::removeDatabase(Connect_name);
                qDebug() << "------809Select result first error";
                return false;
            }
        }
    }
    else
    {
        if(query1->first())
        {
             List.Item  = query1->value(2).toString();
             List.SamId  = query1->value(1).toString();
             List.Porperty  = query1->value(3).toString();
             List.Date  = query1->value(4).toString();
             QString Connect_name = ResultDateDB->connectionName();
             ResultDateDB->close();
             delete ResultDateDB;
             QSqlDatabase::removeDatabase(Connect_name);
             return true;
         }
        else
        {
            if(Current == Datebasefile)
            {
                if(!RestoreSqlMessageFile())
                {
                    qDebug() << "------------------------707Restore File Error";
                    return false;
                }
            }
            if(!RestoreFile(Datebasefile))
            {
                QString Connect_name = ResultDateDB->connectionName();
                ResultDateDB->close();
                delete ResultDateDB;
                QSqlDatabase::removeDatabase(Connect_name);
                qDebug() << "596 Restore File Error";
                return false;
            }
            FileMessage file_message;
            if(!GetShowFile(file_message, inDex))
            {
                qDebug() << "Get file Message error";
                return false;
            }
            QString SqlMess = (QString)file_message.Filename;
            QString FileName = SqlMess+ ".db";
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Resultbase_connection" + FileName));
            ResultDateDB->setDatabaseName(FileName);
            ResultDateDB->setUserName("Top"+FileName);
            ResultDateDB->setPassword("2345");
            if(!ResultDateDB->open())
            {
                qDebug() << "606 open Restore file error";
                return false;
            }
            query1 = new QSqlQuery(*ResultDateDB);
            ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where ID = %0").arg(file_Count));
            if(ok == false)
            {
                if(Current == Datebasefile)
                {
                    if(!RestoreSqlMessageFile())
                    {
                        qDebug() << "------------------------707Restore File Error";
                        return false;
                    }
                }
                if(!RestoreFile(Datebasefile))
                {
                    QString Connect_name = ResultDateDB->connectionName();
                    ResultDateDB->close();
                    delete ResultDateDB;
                    QSqlDatabase::removeDatabase(Connect_name);
                    qDebug() << "596 Restore File Error";
                    return false;
                }
                FileMessage file_message;
                if(!GetShowFile(file_message, inDex))
                {
                    qDebug() << "Get file Message error";
                    return false;
                }
                QString SqlMess = (QString)file_message.Filename;
                QString FileName = SqlMess+ ".db";
                QString Connect_name = ResultDateDB->connectionName();
                ResultDateDB->close();
                delete ResultDateDB;
                QSqlDatabase::removeDatabase(Connect_name);
                ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Resultbase_connection" + FileName));
                ResultDateDB->setDatabaseName(FileName);
                ResultDateDB->setUserName("Top"+FileName);
                ResultDateDB->setPassword("2345");
                if(!ResultDateDB->open())
                {
                    qDebug() << "---------------900 open error";
                    return false;
                }
                query1 = new QSqlQuery(*ResultDateDB);
                ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where ID = %0").arg(file_Count));
                if(ok == false)
                {
                    qDebug() << "---------------906 slect error";
                    return false;
                }
            }
            if(query1->first())
            {
                 List.Item  = query1->value(2).toString();
                 List.SamId  = query1->value(1).toString();
                 List.Porperty  = query1->value(3).toString();
                 List.Date  = query1->value(4).toString();
                 QString Connect_name = ResultDateDB->connectionName();
                 ResultDateDB->close();
                 delete ResultDateDB;
                 QSqlDatabase::removeDatabase(Connect_name);
                 return true;
             }
            else
            {
                QString Connect_name = ResultDateDB->connectionName();
                ResultDateDB->close();
                delete ResultDateDB;
                QSqlDatabase::removeDatabase(Connect_name);
                qDebug() << "------809Select result first error";
                return false;
            }
//            qDebug() << "------828Select result first error";
        }
    }
}

bool TestSql::SelectResultSql(QString &Sel, int Index, QString IndexText)
{
    int iGoto = 2;
    bool ok = false;
    Loop:
    iGoto--;
    FileMessage file_message;

    QString Indexn = IndexText.replace("*","%");
    if(!GetFileMessage(file_message))
    {
        qDebug() << "506 Get file Message error";
        return false;
    }
    QString FileName = (QString)file_message.Filename;
    QString Dex;
    if(FileName.length() == 6)
    {
        Dex = FileName.mid(FileName.length() - 1, 1);
    }
    else if(FileName.length() == 7)
    {
        Dex = FileName.mid(FileName.length() - 2, 2);
    }
    int Allfile = Dex.toInt(&ok);                                                             // 当前最后一个应该被使用的文件
    int Current_file = Allfile;
    int iRet = 0;
    int AllID = 0;
    if(ok == false)
    {
        qDebug() << "519 Get file message error";
        return false;
    }
    QSqlDatabase *ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", FileName));
    for(; Allfile > 0; Allfile--)
    {
        QCoreApplication::processEvents();
        FileName = "Yixin" + QString::number(Allfile);
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Resultbase_connection" + FileName));
        ResultDateDB->setDatabaseName(FileName + ".db");
        ResultDateDB->setUserName("Top"+FileName);
        ResultDateDB->setPassword("2345");

        QFile DBFile(FileName + ".db");
        if(!DBFile.exists() || !ResultDateDB->open())
        {
            delete ResultDateDB;
            qDebug() << "-------------------------793  open file error";
            qDebug() << "601 DatebaseDB Sql open Error";
            if(Allfile == Current_file)
            {
                if(!RestoreSqlMessageFile())
                {
                    qDebug() << "--------------------874 Restore File Error";
                    return false;
                }
            }
            if(!RestoreFile(FileName))
            {
                qDebug() << "528 Restore File Error";
                return false;
            }
            if(iGoto != 0)
            {
                goto Loop;
            }
            else
            {
                qDebug() << "537 Restore file open error";
                return false;
            }
        }
        QSqlQuery *query1 = new QSqlQuery(*ResultDateDB);
        switch(Index)
        {
            QCoreApplication::processEvents();
            case 0:
            {
                QCoreApplication::processEvents();
                ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where ID='%1' order by ID desc").arg(Indexn));  // 通过ID 降序排列
                if(ok == false)
                {
                    qDebug() << "SelectResult function select Error 0";
                    if(Allfile == Current_file)
                    {
                        if(!RestoreSqlMessageFile())
                        {
                            qDebug() << "--------------------874 Restore File Error";
                            return false;
                        }
                    }
                    if(!RestoreFile(FileName))
                    {
                        qDebug() << "720 Restore File Error";
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        return false;
                    }
                    if(!GetFileMessage(file_message))
                    {
                        qDebug() << "506 Get file Message error";
                        return false;
                    }
                    QString FileName = (QString)file_message.Filename;
                    ResultDateDB->addDatabase("QSQLITE", "Resultbase_connection" + FileName);
                    ResultDateDB->setDatabaseName(FileName + ".db");
                    ResultDateDB->setUserName("Top"+FileName);
                    ResultDateDB->setPassword("2345");
                    if(!ResultDateDB->open())
                    {
                        qDebug() << "730 Restore File Error";
                        return false;
                    }
                    ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where ID='%1' order by ID desc").arg(Indexn));
                    if(ok == false)
                    {
                        qDebug() << "740 Select Error";  
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        return false;
                    }
                }
                QCoreApplication::processEvents();
                break;
            }
            case 1:
            {
                QCoreApplication::processEvents();
                ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where SamId like '%1' order by ID desc").arg(Indexn));
                if(ok == false)
                {
                    qDebug() << "751 SelectResult function select Error 1";
                    if(Allfile == Current_file)
                    {
                        if(!RestoreSqlMessageFile())
                        {
                            qDebug() << "--------------------874 Restore File Error";
                            return false;
                        }
                    }
                    if(!RestoreFile(FileName))
                    {
                        qDebug() << "754 Restore File Error";
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        return false;
                    }
                    if(!GetFileMessage(file_message))
                    {
                        qDebug() << "506 Get file Message error";
                        return false;
                    }
                    QString FileName = (QString)file_message.Filename;
                    ResultDateDB->addDatabase("QSQLITE", "Resultbase_connection" + FileName);
                    ResultDateDB->setDatabaseName(FileName + ".db");
                    ResultDateDB->setUserName("Top"+FileName);
                    ResultDateDB->setPassword("2345");
                    if(!ResultDateDB->open())
                    {
                        qDebug() << "764 Restore File Error";
                        return false;
                    }
                    ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where SamId like '%1' order by ID desc").arg(Indexn));
                    if(ok == false)
                    {
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        qDebug() << "771 Select Error";
                        return false;
                    }
                }
                QCoreApplication::processEvents();
                break;
            }
            case 2:
            {
                QCoreApplication::processEvents();
                ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where Item like '%1' order by ID desc").arg(Indexn));
                if(ok == false)
                {
                    qDebug() << "SelectResult function select Error 2, because" << query1->lastError().text();
                    if(Allfile == Current_file)
                    {
                        if(!RestoreSqlMessageFile())
                        {
                            qDebug() << "--------------------874 Restore File Error";
                            return false;
                        }
                    }
                    if(!RestoreFile(FileName))
                    {
                        qDebug() << "785 Restore File Error";
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();    
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        return false;
                    }
                    if(!GetFileMessage(file_message))
                    {
                        qDebug() << "506 Get file Message error";
                        return false;
                    }
                    QString FileName = (QString)file_message.Filename;
                    ResultDateDB->addDatabase("QSQLITE", "Resultbase_connection" + FileName);
                    ResultDateDB->setDatabaseName(FileName + ".db");
                    ResultDateDB->setUserName("Top"+FileName);
                    ResultDateDB->setPassword("2345");
                    if(!ResultDateDB->open())
                    {
                        qDebug() << "795 Restore File Error";
                        return false;
                    }
                    ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where Item like '%1' order by ID desc").arg(Indexn));
                    if(ok == false)
                    {
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        qDebug() << "802 Select Error";
                        return false;
                    }
                }
                QCoreApplication::processEvents();
                break;
            }
            case 3:
            {
                QCoreApplication::processEvents();
                ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where Property like '%1' order by ID desc").arg(Indexn));
                if(ok == false)
                {
                    qDebug() << "SelectResult function select Error 3";
                    if(Allfile == Current_file)
                    {
                        if(!RestoreSqlMessageFile())
                        {
                            qDebug() << "--------------------874 Restore File Error";
                            return false;
                        }
                    }
                    if(!RestoreFile(FileName))
                    {
                        qDebug() << "816 Restore File Error";
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        return false;
                    }
                    if(!GetFileMessage(file_message))
                    {
                        qDebug() << "506 Get file Message error";
                        return false;
                    }
                    QString FileName = (QString)file_message.Filename;
                    ResultDateDB->addDatabase("QSQLITE", "Resultbase_connection" + FileName);
                    ResultDateDB->setDatabaseName(FileName + ".db");
                    ResultDateDB->setUserName("Top"+FileName);
                    ResultDateDB->setPassword("2345");
                    if(!ResultDateDB->open())
                    {
                        qDebug() << "826 Restore File Error";
                        return false;
                    }
                    ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where Property like '%1' order by ID desc").arg(Indexn));
                    if(ok == false)
                    {
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        qDebug() << "833 Select Error";
                        return false;
                    }
                }
                QCoreApplication::processEvents();
                break;
            }
            case 4:
            {
                QCoreApplication::processEvents();
                ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where Date like '%1' order by ID desc").arg(Indexn));
                if(ok == false)
                {
                    qDebug() << "SelectResult function select Error 4";
                    if(Allfile == Current_file)
                    {
                        if(!RestoreSqlMessageFile())
                        {
                            qDebug() << "--------------------874 Restore File Error";
                            return false;
                        }
                    }
                    if(!RestoreFile(FileName))
                    {
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        qDebug() << "847 Restore File Error";
                        return false;
                    }
                    if(!GetFileMessage(file_message))
                    {
                        qDebug() << "506 Get file Message error";
                        return false;
                    }
                    QString FileName = (QString)file_message.Filename;
                    ResultDateDB->addDatabase("QSQLITE", "Resultbase_connection" + FileName);
                    ResultDateDB->setDatabaseName(FileName + ".db");
                    ResultDateDB->setUserName("Top"+FileName);
                    ResultDateDB->setPassword("2345");
                    if(!ResultDateDB->open())
                    {
                        qDebug() << "857 Restore File Error";
                        return false;
                    }
                    ok = query1->exec(QObject::tr("select ID, SamId, Item, Property, Date from testresult where Date like '%1' order by ID desc").arg(Indexn));
                    if(ok == false)
                    {
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        qDebug() << "864 Select Error";
                        return false;
                    }
                }
                QCoreApplication::processEvents();
                break;
            }
        }
        QCoreApplication::processEvents();
        while(query1->next())
        {        
            QCoreApplication::processEvents();
            iRet = 0;
            if(!query1->value(0).isNull())
            {
                for(int i = 1; i <  Allfile; i++)
                {
                    iRet += SelecDatebase(i);
                    if(iRet == ERROR_ERR)
                    {
                        QString Connect_name = ResultDateDB->connectionName();
                        ResultDateDB->close();
                        delete ResultDateDB;
                        QSqlDatabase::removeDatabase(Connect_name);
                        qDebug() << "597 Select datebase number Error";
                        return false;
                    }
                }
            }
            AllID = CountResultSql() - iRet - query1->value(0).toInt() + 1;
            QString IDs = QString::number(AllID)+",";
            QString SamIds = query1->value(1).toString()+",";
            QString Items = query1->value(2).toString()+",";
            QString Propertys = query1->value(3).toString()+",";
            QString Dates = query1->value(4).toString()+"@";
            Sel.append(IDs);
            Sel.append(SamIds);
            Sel.append(Items);
            Sel.append(Propertys);
            Sel.append(Dates);
            QCoreApplication::processEvents();
         }
        QCoreApplication::processEvents();
    }
    QStringList Res_list = Sel.split("@");
    if(Res_list.length() == 1)
    {
        qDebug() << "Select is Empty";
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return false;
    }
    QString Connect_name = ResultDateDB->connectionName();
    ResultDateDB->close();
    delete ResultDateDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return true;
}

bool TestSql::DeleteResultSql(int ID)
{
    if(ID < 1 || ID > 5000)
    {
        qDebug() << "Delete function para error";
        return false;
    }
    int iGoto = 2;
    bool ok = false;
    Loop:
    iGoto--;
    FileMessage Sql_message;
    if(!GetFileMessage(Sql_message))
    {
        qDebug() << "Get file Message error";
        return false;
    }
    QString Current = (QString)Sql_message.Filename;
    QString iCurrent;
    if(Current.length() == 6)
    {
        iCurrent = Current.mid(Current.length() - 1, 1);
    }
    else if(Current.length() == 7)
    {
        iCurrent = Current.mid(Current.length() - 2, 2);
    }
    int Current_file = iCurrent.toInt(&ok);                                                             // 当前最后一个应该被使用的文件
    if(ok == false)
    {
        qDebug() << "670 Get file message error";
        return false;
    }
    qDebug() << "--------------1175 Current file is " << Current_file;
    FileMessage file_message;
    if(!GetShowFile(file_message, ID))
    {
        qDebug() << "Get file Message error";
        return false;
    }
    QString FileName = (QString)file_message.Filename;
    QString Index;
    if(FileName.length() == 6)
    {
        Index = FileName.mid(FileName.length() - 1, 1);
    }
    else if(FileName.length() == 7)
    {
        Index = FileName.mid(FileName.length() - 2, 2);
    }
    int Allfile = Index.toInt(&ok);                                                             // 当前应该操作的文件
    if(ok == false)
    {
        qDebug() << "670 Get file message error";
        return false;
    }

    int file_Count = file_message.FileCount;
    int All_Count = SelecDatebase(Allfile);
    QSqlDatabase *ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", FileName));
    qDebug() << "Filename is " << FileName;
    ResultDateDB->setDatabaseName(FileName+".db");
    ResultDateDB->setUserName("Top"+FileName);
    ResultDateDB->setPassword("2345");
    if(!ResultDateDB->open())
    {
        delete ResultDateDB;
        qDebug() << "745 ResultDB open Error";
        if(Allfile == Current_file)
        {
            if(!RestoreSqlMessageFile())
            {
                qDebug() << "------1216 Restore SqlMessage Error";
                return false;
            }
        }
        if(!RestoreFile(FileName))
        {
            qDebug() << "748 Restore File Error";
            return false;
        }
        if(iGoto != 0)
        {
            goto Loop;
        }
        else
        {
            qDebug() << "757 Restore file open error";
            return false;
        }
    }
    QSqlQuery *query = new QSqlQuery(*ResultDateDB);
    ok = query->exec(QObject::tr("delete from testresult where ID='%1'").arg(file_Count));
    if(ok == false)
    {
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        qDebug() << "Delete ID Error";
        return false;
    }
    for(;file_Count < All_Count; file_Count++)
    {
        ok = UpdateResultSql(ResultDateDB, file_Count+1, file_Count);
        if(ok == false)
        {
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "Update Result Error";
            return false;
        }
    }
    ok = UpdateDatebaseMessage(FileName, All_Count-1);
    if(ok == false)
    {
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        qDebug() << "680 Update Datebase Error";
        return false;
    }
    if(!SaveDatebase())
    {
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        qDebug() << "------ 1457 SaveDatebase Error";
        return false;
    }
    QString Connect_name = ResultDateDB->connectionName();
    ResultDateDB->close();
    delete ResultDateDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return true;
}

bool TestSql::GetDetailInforResultSql(Results &ret, int id)
{
    if(id < 1 || id > 5000)
    {
        qDebug() << "GetDetailInforResultSql function para Error";
        return false;
    }
    bool ok = false;
    int iGoto = 2;
    Loop:
    iGoto--;
    FileMessage Sql_message;
    if(!GetFileMessage(Sql_message))
    {
        qDebug() << "Get file Message error";
        return false;
    }
    QString Current = (QString)Sql_message.Filename;
    qDebug() << "1294 Current file is " << Current;
    FileMessage file_message;
    if(!GetShowFile(file_message, id))
    {
        qDebug() << "Get file Message error";
        return false;
    }
    QString Datebasefile = (QString)file_message.Filename;
    QString FileName = Datebasefile+ ".db";
    int file_Count = file_message.FileCount;
    QSqlDatabase *ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", FileName));
    qDebug() << "Filename is " << FileName;
    ResultDateDB->setDatabaseName(FileName);
    ResultDateDB->setUserName("Top"+FileName);
    ResultDateDB->setPassword("2345");
    if(!ResultDateDB->open())
    {
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        qDebug() << "984 ResultDB open Error";
        if(Datebasefile == Current)
        {
            if(!RestoreSqlMessageFile())
            {
                qDebug() << "------1324 Restore SqlMessage Error";
                return false;
            }
        }
        if(!RestoreFile(Datebasefile))
        {
            qDebug() << "987 Restore File Error";
            return false;
        }
        if(iGoto != 0)
        {
            goto Loop;
        }
        else
        {
            qDebug() << "996Restore file open error";
            return false;
        }
    }
    QSqlQuery *query = new QSqlQuery(*ResultDateDB);
    ok = query->exec(QObject::tr("select DetailInfor from testresult where ID=%0").arg(file_Count));
    if(ok == false)
    {
        if(Datebasefile == Current)
        {
            if(!RestoreSqlMessageFile())
            {
                qDebug() << "------1324 Restore SqlMessage Error";
                return false;
            }
        }
        if(!RestoreFile(Datebasefile))
        {
            qDebug() << "1007 Restore File Error";
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
        FileMessage Sql_message;
        if(!GetFileMessage(Sql_message))
        {
            qDebug() << "Get file Message error";
            return false;
        }
        QString Datebasefile = (QString)file_message.Filename;
        QString FileName = Datebasefile+ ".db";
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Resultbase_connection" + FileName));
        ResultDateDB->setDatabaseName(FileName);
        ResultDateDB->setUserName("Top"+FileName);
        ResultDateDB->setPassword("2345");
        if(!ResultDateDB->open())
        {
            qDebug() << "1012 open Restore File Error";
            return false;
        }
        query = new QSqlQuery(*ResultDateDB);
        ok = query->exec(QObject::tr("select DetailInfor from testresult where ID=%0").arg(file_Count));
        if(ok == false)
        {
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "1018 Select Error";
            return false;
        }
        if(!query->first())
        {
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "1026 Select Error";
            return false;
        }
        QByteArray Data2  = query->value(0).toByteArray();
        char *datp = (char *)&ret;
        int index = 0;
        for(; index < Data2.length(); index++)
        {
            datp[index] = Data2.at(index);
        }
        return true;
    }
    else
    {
        if(!query->first())
        {
            QString Connect_name = ResultDateDB->connectionName();
            ResultDateDB->close();
            delete ResultDateDB;
            QSqlDatabase::removeDatabase(Connect_name);
            qDebug() << "1026 Select Error";
            return false;
        }
        QByteArray Data2  = query->value(0).toByteArray();
        char *datp = (char *)&ret;
        int index = 0;
        for(; index < Data2.length(); index++)
        {
            datp[index] = Data2.at(index);
        }
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return true;
    }
}

bool TestSql::UpdateResultSql(QSqlDatabase *DB, int OldId, int NewId)
{
    bool ok = false;
    QSqlQuery *query = new QSqlQuery(*DB);
    ok = query->exec(QObject::tr("update testresult set ID='%0' where ID='%1'").arg(NewId).arg(OldId));
    if(ok == false)
    {
        qDebug() << "Update Result Error";
        return false;
    }
    return true;
}

bool TestSql::init_DatebaseMessage()                // 初始化存储结果的数据库文件信息
{
    QSqlDatabase *DatebaseDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Datebase_connection"));
    DatebaseDB->setDatabaseName("./SqlMessage.db");
    DatebaseDB->setUserName("Top3");
    DatebaseDB->setPassword("1234");
    if(!DatebaseDB->open())
    {
        qDebug() << "514 DatebaseDB Sql open Error";
        if(!RestoreDatebase())
        {
            qDebug() << "------ 1634 RestoreDatebase Error";
            QString Connect_name = DatebaseDB->connectionName();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
        if(!DatebaseDB->open())
        {
            qDebug() << "------ 1639 DatebaseDB open Error";
            QString Connect_name = DatebaseDB->connectionName();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
    }
    bool ok = false;
    QString File;
    QSqlQuery *qurey = new QSqlQuery(*DatebaseDB);                 // QSqlQuery类实现了执行SQL语句
    QStringList List  = DatebaseDB->tables();                                     // 获得该数据库文件中的所有表格信息
    if(0 == List.length())                                                                         // 为空是创建表格
    {
       bool Datebase = qurey->exec(tr("CREATE TABLE DBMessage (Filename vchar, Count int)"));
       if(false == Datebase)
       {
           QString Connect_name = DatebaseDB->connectionName();
           DatebaseDB->close();
           delete DatebaseDB;
           QSqlDatabase::removeDatabase(Connect_name);
           qDebug() << "Creat DBMessage Error,beacuse " << qurey->lastError().text();
           if(!RestoreDatebase())
           {
               qDebug() << "------ 1656 RestoreDatebase Error";
               return false;
           }
           DatebaseDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Datebase_connection"));
           DatebaseDB->setDatabaseName("./SqlMessage.db");
           DatebaseDB->setUserName("Top3");
           DatebaseDB->setPassword("1234");
           if(!DatebaseDB->open())
           {
               qDebug() << "------ 1663 DatebaseDB open Error";
               QString Connect_name = DatebaseDB->connectionName();
               delete DatebaseDB;
               QSqlDatabase::removeDatabase(Connect_name);
               return false;
           }
           qurey = new QSqlQuery(*DatebaseDB);
           Datebase = qurey->exec(tr("CREATE TABLE DBMessage (Filename vchar, Count int)"));
           if(false == Datebase)
           {
               qDebug() << "------ 1669 CREATE TABLE Error";
               QString Connect_name = DatebaseDB->connectionName();
               DatebaseDB->close();
               delete DatebaseDB;
               QSqlDatabase::removeDatabase(Connect_name);
               return false;
           }
       }
       for(int i = 1; i <= 25 ;i++)                                                              // 循环插入默认数据0
       {
           File = "Yixin"+ QString::number(i);
           ok = qurey->prepare(tr("INSERT INTO DBMessage(Filename , Count) VALUES (:Filename, :Count)"));
           if(ok == false)
           {
               qDebug() << "prepare Datebase DB Error, because " << qurey->lastError().text();
               QString Connect_name = DatebaseDB->connectionName();
               DatebaseDB->close();
               delete DatebaseDB;
               QSqlDatabase::removeDatabase(Connect_name);
               return false;
           }
           qurey->bindValue(":Filename", File);
           qurey->bindValue(":Count", 0);
           ok = qurey->exec();
           if(ok == false)
           {
               qDebug() << "insert Datebase DB Error, because " << DatebaseDB->lastError().text();
                QString Connect_name = DatebaseDB->connectionName();
               DatebaseDB->close();
               delete DatebaseDB;
               QSqlDatabase::removeDatabase(Connect_name);
               if(!RestoreDatebase())
               {
                   qDebug() << "------ 1697 RestoreDatebase Error";
                   return false;
               }
               DatebaseDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Datebase_connection"));
               DatebaseDB->setDatabaseName("./SqlMessage.db");
               DatebaseDB->setUserName("Top3");
               DatebaseDB->setPassword("1234");
               if(!DatebaseDB->open())
               {
                   qDebug() << "------ 1703 DatebaseDB open Error";
                   QString Connect_name = DatebaseDB->connectionName();
                   delete DatebaseDB;
                   QSqlDatabase::removeDatabase(Connect_name);
                   return false;
               }
               qurey = new QSqlQuery(*DatebaseDB);
               ok = qurey->prepare(tr("INSERT INTO DBMessage(Filename , Count) VALUES (:Filename, :Count)"));
               if(ok == false)
               {
                   qDebug() << "prepare Datebase DB Error, because " << qurey->lastError().text();
                   QString Connect_name = DatebaseDB->connectionName();
                   DatebaseDB->close();
                   delete DatebaseDB;
                   QSqlDatabase::removeDatabase(Connect_name);
                   return false;
               }
               qurey->bindValue(":Filename", File);
               qurey->bindValue(":Count", 0);
               ok = qurey->exec();
               if(ok == false)
               {
                   qDebug() << "------ 1719 insert Datebase DB Error, because " << DatebaseDB->lastError().text();
                   QString Connect_name = DatebaseDB->connectionName();
                   DatebaseDB->close();
                   delete DatebaseDB;
                   QSqlDatabase::removeDatabase(Connect_name);
                   return false;
               }
           }
       }
    } 
    QString Connect_name = DatebaseDB->connectionName();
    DatebaseDB->close();
    delete DatebaseDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return true;
}

bool TestSql::UpdateDatebaseMessage(QString File, int FileCount)            // 更改结果信息数据库内容
{
    QSqlDatabase *DatebaseDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Datebase_connection"));
    DatebaseDB->setDatabaseName("./SqlMessage.db");
    DatebaseDB->setUserName("Top3");
    DatebaseDB->setPassword("1234");
    if(!DatebaseDB->open())
    {
        qDebug() << "------ 1743 DatebaseDB Sql open Error";
        if(!RestoreDatebase())
        {
            qDebug() << "------ 1747 RestoreDatebase Error";
            QString Connect_name = DatebaseDB->connectionName();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
        if(!DatebaseDB->open())
        {
            qDebug() << "------ 1752 DatebaseDB Sql open Error";
            QString Connect_name = DatebaseDB->connectionName();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
    }
    QSqlQuery *qurey = new QSqlQuery(*DatebaseDB);
    bool ok = qurey->exec(tr("update DBMessage set Count = '%0' where Filename = '%1' " ).arg(FileCount).arg(File));
    if(ok == false)
    {
        qDebug() << "Update DBmessage error";
        QString Connect_name = DatebaseDB->connectionName();
        DatebaseDB->close();
        delete DatebaseDB;
        QSqlDatabase::removeDatabase(Connect_name);
        if(!RestoreDatebase())
        {
            qDebug() << "------ 1767 RestoreDatebase Error";
            return false;
        }

        DatebaseDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Datebase_connection"));
        DatebaseDB->setDatabaseName("./SqlMessage.db");
        DatebaseDB->setUserName("Top3");
        DatebaseDB->setPassword("1234");
        if(!DatebaseDB->open())
        {
            qDebug() << "------ 1772 DatebaseDB Sql open Error";
            QString Connect_name = DatebaseDB->connectionName();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
        qurey = new QSqlQuery(*DatebaseDB);
        ok = qurey->exec(tr("update DBMessage set Count = '%0' where Filename = '%1' " ).arg(FileCount).arg(File));
        if(ok == false)
        {
            qDebug() << "------ 1778 Update DBmessage error";
            QString Connect_name = DatebaseDB->connectionName();
            DatebaseDB->close();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return false;
        }
    }
    QString Connect_name = DatebaseDB->connectionName();
    DatebaseDB->close();
    delete DatebaseDB;
    QSqlDatabase::removeDatabase(Connect_name);
    return true;
}

int TestSql::SelecDatebase(int Index)
{
    if(Index < 1 || Index > 25)
    {
        qDebug() << "Para is Error,Selecet Datebase error";
        return ERROR_ERR;
    }
    QSqlDatabase *DatebaseDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Datebase_connection"));
    DatebaseDB->setDatabaseName("./SqlMessage.db");
    DatebaseDB->setUserName("Top3");
    DatebaseDB->setPassword("1234");
    if(!DatebaseDB->open())
    {
        qDebug() << "------ 1802 DatebaseDB Sql open Error";
        if(!RestoreDatebase())
        {
            qDebug() << "------ 1806 RestoreDatebase Error";
            QString Connect_name = DatebaseDB->connectionName();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return ERROR_ERR;
        }
        if(!DatebaseDB->open())
        {
            qDebug() << "------ 1811 DatebaseDB Sql open Error";
            QString Connect_name = DatebaseDB->connectionName();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return ERROR_ERR;
        }
    }
    QString Filename = "Yixin" + QString::number(Index);
    QSqlQuery *qurey = new QSqlQuery(*DatebaseDB);
    bool ok = qurey->exec(tr("select Count from DBMessage where Filename = '%0' ").arg(Filename));
    if(ok == false)
    {
        qDebug() << "------ 1822 Select Datebase Error, because " << qurey->lastError().text();
        QString Connect_name = DatebaseDB->connectionName();
        DatebaseDB->close();
        delete DatebaseDB;
        QSqlDatabase::removeDatabase(Connect_name);
        if(!RestoreDatebase())
        {
            qDebug() << "------ 1827 RestoreDatebase Error";
            return ERROR_ERR;
        }
        DatebaseDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Datebase_connection"));
        DatebaseDB->setDatabaseName("./SqlMessage.db");
        DatebaseDB->setUserName("Top3");
        DatebaseDB->setPassword("1234");
        if(!DatebaseDB->open())
        {
            qDebug() << "------ 1832 DatebaseDB Sql open Error";
            return ERROR_ERR;
        }
        qurey = new QSqlQuery(*DatebaseDB);
        ok = qurey->exec(tr("select Count from DBMessage where Filename = '%0' ").arg(Filename));
        if(ok == false)
        {
            qDebug() << "------ 1838 Select Datebase Error, because " << qurey->lastError().text();
            QString Connect_name = DatebaseDB->connectionName();
            DatebaseDB->close();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return ERROR_ERR;
        }
    }
    if(!qurey->first())
    {
        qDebug() << "------ 1847 Datebase  value Error, because " << qurey->lastError().text();
        QString Connect_name = DatebaseDB->connectionName();
        DatebaseDB->close();
        delete DatebaseDB;
        QSqlDatabase::removeDatabase(Connect_name);
        if(!RestoreDatebase())
        {
            qDebug() << "------ 1852 RestoreDatebase Error";
            return ERROR_ERR;
        }
        DatebaseDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Datebase_connection"));
        DatebaseDB->setDatabaseName("./SqlMessage.db");
        DatebaseDB->setUserName("Top3");
        DatebaseDB->setPassword("1234");
        if(!DatebaseDB->open())
        {
            qDebug() << "------ 1857 DatebaseDB Sql open Error";
            return ERROR_ERR;
        }
        qurey = new QSqlQuery(*DatebaseDB);
        ok = qurey->exec(tr("select Count from DBMessage where Filename = '%0' ").arg(Filename));
        if(ok == false)
        {
            qDebug() << "------ 1863 Select Datebase Error, because " << qurey->lastError().text();
            QString Connect_name = DatebaseDB->connectionName();
            DatebaseDB->close();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return ERROR_ERR;
        }
        if(!qurey->first())
        {
            qDebug() << "------ 1869 Datebase  value Error, because " << qurey->lastError().text();
            QString Connect_name = DatebaseDB->connectionName();
            DatebaseDB->close();
            delete DatebaseDB;
            QSqlDatabase::removeDatabase(Connect_name);
            return ERROR_ERR;
        }
    }

    int Number = qurey->value(0).toInt(&ok);
    if(ok == true)
    {
        QString Connect_name = DatebaseDB->connectionName();
        DatebaseDB->close();
        delete DatebaseDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return Number;
    }
    else
    {
        QString Connect_name = DatebaseDB->connectionName();
        DatebaseDB->close();
        delete DatebaseDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return ERROR_ERR;
    }
}

bool TestSql::isDatebaseEmpty()
{
    QSqlDatabase *DatebaseDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Datebase_connection"));
    DatebaseDB->setDatabaseName("./SqlMessage.db");
    DatebaseDB->setUserName("Top3");
    DatebaseDB->setPassword("1234");
    if(!DatebaseDB->open())
    {
        qDebug() << "1323 DatebaseDB Sql open Error";
        QString Connect_name = DatebaseDB->connectionName();
        delete DatebaseDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return false;
    }
    QStringList List  = DatebaseDB->tables();                                     // 获得该数据库文件中的所有表格信息
    if(0 == List.length())
    {
        QString Connect_name = DatebaseDB->connectionName();
        DatebaseDB->close();
        delete DatebaseDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return true;
    }
    else
    {
        QString Connect_name = DatebaseDB->connectionName();
        DatebaseDB->close();
        delete DatebaseDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return false;
    }
}

bool TestSql::GetFileMessage(FileMessage &Message)
{
    isLimit = false;
    QString Name;
    int iFile = 25;
    int File_Count = 0;
    for(; iFile > 0; iFile--)
    {
        File_Count = SelecDatebase(iFile);
        if(-1 == File_Count)
        {
            qDebug() << "------ 1246 Select Error";
            return false;
        }
        else if(0 == File_Count)
        {
            if(1 == iFile)
            {
                Name = "Yixin" + QString::number(iFile);
                strcpy(Message.Filename, Name.toLatin1().data());
                Message.FileCount = File_Count;
            }
            else
            {
                continue;
            }
        }
        else if(200 == File_Count)
        {
            if(25 == iFile)
            {
                qDebug() << "run out of memory,内存不足";
                isLimit = true;
                Name = "Yixin" + QString::number(iFile);
                strcpy(Message.Filename, Name.toLatin1().data());
                Message.FileCount = 200;
                break;
            }
            else
            {
                Name = "Yixin" + QString::number(iFile+1);
                strcpy(Message.Filename, Name.toLatin1().data());
                Message.FileCount = 0;
                break;
            }
        }
        else
        {
            Name = "Yixin" + QString::number(iFile);
            strcpy(Message.Filename, Name.toLatin1().data());
            Message.FileCount = File_Count;
            break;
        }
    }
    return true;
}

bool TestSql::GetShowFile(FileMessage &Message, int iShow)          // 获得数据库倒序排序下某条数对应的文件与索引
{
    FileMessage Mess;
    int Allfile_Count = 0;
    int Index_over = 0;
    int ret_count = 0;
    if(!GetFileMessage(Mess))
    {
        qDebug() << "Show File function Get Datebase Error";
        return false;
    }
    bool ok = false;
    QString Filename = (QString)Mess.Filename;
    QString Index;
    if(Filename.length() == 6)
    {
        Index = Filename.mid(Filename.length() - 1, 1);
    }
    else if(Filename.length() == 7)
    {
        Index = Filename.mid(Filename.length() - 2, 2);
    }
    int Allfile = Index.toInt(&ok);                                                             // 当前最后一个应该被使用的文件
    if(ok == false)
    {
        qDebug() << "670 Get file message error";
        return false;
    }
    Allfile_Count = SelecDatebase(Allfile);
    for(; Allfile > 0; Allfile--)
    {
        if(ERROR_ERR == Allfile_Count)
        {
            qDebug() << "679 Select Datebase Error";
            return false;
        }
        Index_over = Allfile_Count - iShow;
        if(Index_over >= 0)
        {
            Index_over += 1;
            strcpy(Message.Filename, Filename.toLatin1().data());
            Message.FileCount = Index_over;
            break;
        }
        else
        {
            ret_count = SelecDatebase(Allfile-1);
            if(ERROR_ERR == ret_count)
            {
                qDebug() << "700 Select Datebase Error";
                return false;
            }
            Allfile_Count += ret_count;
            Filename = "Yixin" + QString::number(Allfile-1);
        }
    }
    return true;
}

bool TestSql::SaveFile()
{
    FileMessage File_message;
    if(!GetFileMessage(File_message))
    {
        qDebug() << "init function Get datebase error";
        return false;
    }
    QString DateFile = (QString)File_message.Filename;
    qDebug() << "------ 2064 File name is " << DateFile;
    QFile file(DateFile + ".db");
    if(!file.exists())
    {
        qDebug() << "the file not exists";
        return false;
    }
    QString FileIndex;
    if(DateFile.length() == 6)
    {
        FileIndex = DateFile.mid(DateFile.length() - 1, 1);
    }
    else if(DateFile.length() == 7)
    {
        FileIndex = DateFile.mid(DateFile.length() - 2, 2);
    }
    qDebug() << "------ 2083 FileIndex is " << FileIndex;
    bool ok = false;
    int DeleteIndex = FileIndex.toInt(&ok);
    if(ok == false)
    {
        qDebug() << "------ 2065 Delete Error";
        return false;
    }
    QString Deletefile = QString::number(DeleteIndex-1);
    QFile *DeleFile = new QFile();
    DeleFile->setFileName("Yixin" + Deletefile + ".bak");
    int Delefile = 0;
    int statue = 0;
    int Save = 0;
    if(DeleFile->exists())
    {
        QString DelShell = "rm -rf Yixin" + Deletefile + ".bak";
        char *cmd_dele = DelShell.toLatin1().data();
        Delefile = system(cmd_dele);

        if(!(-1 != Delefile && WIFEXITED(Delefile) == 1 && 0 == WEXITSTATUS(Delefile)))
        {
            qDebug() << "------ 2080 save function Use system Error";
            delete DeleFile;
            return false;
        }
    }
    //----------------Add-------------------------------------------------------------//
    QSqlDatabase *ResultDateDB = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "Resultbase_connection"));
    ResultDateDB->setDatabaseName(DateFile + ".db");
    ResultDateDB->setUserName("Top2");
    ResultDateDB->setPassword("2345");
    if(!ResultDateDB->open())
    {
        qDebug() << "------2292 save function open Sqldatebase Error";
        delete DeleFile;
        QString Connect_name = ResultDateDB->connectionName();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        return false;
    }
    QStringList List  = ResultDateDB->tables();                                     // 获得该数据库文件中的所有表格信息
    if(0 != List.length())
    {
        QString Shell = "cp " + DateFile + ".db " +  DateFile + ".bak";
        char *cmd_char = Shell.toLatin1().data();
        statue = system(cmd_char);

        QString SaveSql = "cp SqlMessage.db SqlMessage.bak";
        char *Save_char = SaveSql.toLatin1().data();
        Save = system(Save_char);
    }
    //------------------------------------------------------------------------------------//
    if((-1 != statue && WIFEXITED(statue) == 1 && 0 == WEXITSTATUS(statue)) && (-1 != Save && WIFEXITED(Save) == 1 && 0 == WEXITSTATUS(Save)) )
    {
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        delete DeleFile;
        return true;
    }
    else
    {
        qDebug() << "------ 2100 save function Use system Error";
        QString Connect_name = ResultDateDB->connectionName();
        ResultDateDB->close();
        delete ResultDateDB;
        QSqlDatabase::removeDatabase(Connect_name);
        delete DeleFile;
        return false;
    }
}

bool TestSql::RestoreFile(QString FileName)
{
    qDebug() << "use Restore function file Name is " << FileName + ".bak";
    QFile *file = new QFile();
    file->setFileName(FileName + ".bak");
    if(!file->exists())
    {
        qDebug() << "the file not exists";
        delete file;
        return false;
    }
    QString Shell = "cp " + FileName + ".bak " +  FileName + ".db";
    char *cmd_char = Shell.toLatin1().data();
    int statue = system(cmd_char);

    if(-1 != statue && WIFEXITED(statue) == 1 && 0 == WEXITSTATUS(statue))
    {
        qDebug() << "------ 2293 RestoreFile Function is over";
        delete file;
        return true;
    }
    else
    {
        qDebug() << "restore function Use system Error";
        delete file;
        return false;
    }
}

bool TestSql::RestoreSqlMessageFile()
{
    qDebug() << "use Restore SqlMessage file function";
    QFile *file = new QFile();
    file->setFileName("SqlMessage.bak");
    if(!file->exists())
    {
        qDebug() << "the file not exists";
        delete file;
        return false;
    }
    QString SaveSql = "cp SqlMessage.bak SqlMessage.db";
    char *Save_char = SaveSql.toLatin1().data();
    int Save = system(Save_char);

    QString SaveSql_bak = "cp SqlMessage.db SqlMessageSave.db";
    char *Save_CMD = SaveSql_bak.toLatin1().data();
    int Save_bak = system(Save_CMD);

    if((-1 != Save && WIFEXITED(Save) == 1 && 0 == WEXITSTATUS(Save)) && (-1 != Save_bak && WIFEXITED(Save_bak) == 1 && 0 == WEXITSTATUS(Save_bak)))
    {
        delete file;
        return true;
    }
    else
    {
        qDebug() << "restore function Use system Error";
        delete file;
        return false;
    }
}

bool TestSql::SaveDatebase()
{
    QFile *file = new QFile();
    file->setFileName("SqlMessage.db");
    if(!file->exists())
    {
        qDebug() << "the file not exists";
        delete file;
        return false;
    }
    QString SaveSql = "cp SqlMessage.db SqlMessageSave.db";
    char *Save_char = SaveSql.toLatin1().data();
    int Save = system(Save_char);
    if(-1 != Save && WIFEXITED(Save) == 1 && 0 == WEXITSTATUS(Save))
    {
        delete file;
        return true;
    }
    else
    {
        qDebug() << "restore function Use system Error";
        delete file;
        return false;
    }
}

bool TestSql::RestoreDatebase()
{
    QFile *file = new QFile();
    file->setFileName("SqlMessageSave.db");
    if(!file->exists())
    {
        qDebug() << "the file not exists";
        delete file;
        return false;
    }
    QString SaveSql = "cp SqlMessageSave.db SqlMessage.db";
    char *Save_char = SaveSql.toLatin1().data();
    int Save = system(Save_char);
    if(-1 != Save && WIFEXITED(Save) == 1 && 0 == WEXITSTATUS(Save))
    {
        delete file;
        return true;
    }
    else
    {
        qDebug() << "restore function Use system Error";
        delete file;
        return false;
    }
}
