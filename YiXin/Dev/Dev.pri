#-------------------------------------------------
#
# Project created by wangyong 2017-04-10 15:11:00
#
#-------------------------------------------------

SOURCES += \
    $$PWD/agl_dev.cpp \
    $$PWD/uUsbCamera.cpp

HEADERS += \
    $$PWD/agl_dev.h \
    $$PWD/libcharset.h \
    $$PWD/localcharset.h \
    $$PWD/qzxing.h \
    $$PWD/qzxing_global.h \
    $$PWD/uUsbCamera.h

LIBS += -L$$PWD/ -lcharset
LIBS += -L$$PWD/ -liconv
LIBS += -L$$PWD/ -lQZXing
