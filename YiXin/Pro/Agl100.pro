#----------------------------------------------------------------------
#
# Project created by wangyong 2017-04-10 15:44:23
#
#----------------------------------------------------------------------

QT       += core gui network  sql widgets  serialport multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Agl100_App
TEMPLATE = app

include(../App/App.pri)
include(../Dev/Dev.pri)
include(../Sql/Sql.pri)

LIBS += -L$$PWD/../../qwt-6.1.2-arm/_install/lib/ -lqwt
INCLUDEPATH += $$PWD/../../qwt-6.1.2-arm/_install/include
DEPENDPATH += $$PWD/../../qwt-6.1.2-arm/_install/include

TRANSLATIONS = YiXin_ZH_CN.ts;

#unix:!macx: LIBS += -L$$PWD/../Dev/ -lcharset

#INCLUDEPATH += $$PWD/../Dev
#DEPENDPATH += $$PWD/../Dev

#unix:!macx: LIBS += -L$$PWD/../Dev/ -liconv

#INCLUDEPATH += $$PWD/../Dev
#DEPENDPATH += $$PWD/../Dev

#unix:!macx: LIBS += -L$$PWD/../Dev/ -lQZXing

#INCLUDEPATH += $$PWD/../Dev
#DEPENDPATH += $$PWD/../Dev
