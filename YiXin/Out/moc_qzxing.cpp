/****************************************************************************
** Meta object code from reading C++ file 'qzxing.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Dev/qzxing.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qzxing.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_QZXing_t {
    QByteArrayData data[28];
    char stringdata[398];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_QZXing_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_QZXing_t qt_meta_stringdata_QZXing = {
    {
QT_MOC_LITERAL(0, 0, 6), // "QZXing"
QT_MOC_LITERAL(1, 7, 15), // "decodingStarted"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 16), // "decodingFinished"
QT_MOC_LITERAL(4, 41, 9), // "succeeded"
QT_MOC_LITERAL(5, 51, 8), // "tagFound"
QT_MOC_LITERAL(6, 60, 3), // "tag"
QT_MOC_LITERAL(7, 64, 11), // "decodeImage"
QT_MOC_LITERAL(8, 76, 5), // "image"
QT_MOC_LITERAL(9, 82, 14), // "decodeImageQML"
QT_MOC_LITERAL(10, 97, 8), // "imageUrl"
QT_MOC_LITERAL(11, 106, 17), // "decodeSubImageQML"
QT_MOC_LITERAL(12, 124, 7), // "offsetX"
QT_MOC_LITERAL(13, 132, 7), // "offsetY"
QT_MOC_LITERAL(14, 140, 5), // "width"
QT_MOC_LITERAL(15, 146, 6), // "height"
QT_MOC_LITERAL(16, 153, 13), // "DecoderFormat"
QT_MOC_LITERAL(17, 167, 18), // "DecoderFormat_None"
QT_MOC_LITERAL(18, 186, 21), // "DecoderFormat_QR_CODE"
QT_MOC_LITERAL(19, 208, 25), // "DecoderFormat_DATA_MATRIX"
QT_MOC_LITERAL(20, 234, 19), // "DecoderFormat_UPC_E"
QT_MOC_LITERAL(21, 254, 19), // "DecoderFormat_UPC_A"
QT_MOC_LITERAL(22, 274, 19), // "DecoderFormat_EAN_8"
QT_MOC_LITERAL(23, 294, 20), // "DecoderFormat_EAN_13"
QT_MOC_LITERAL(24, 315, 22), // "DecoderFormat_CODE_128"
QT_MOC_LITERAL(25, 338, 21), // "DecoderFormat_CODE_39"
QT_MOC_LITERAL(26, 360, 17), // "DecoderFormat_ITF"
QT_MOC_LITERAL(27, 378, 19) // "DecoderFormat_Aztec"

    },
    "QZXing\0decodingStarted\0\0decodingFinished\0"
    "succeeded\0tagFound\0tag\0decodeImage\0"
    "image\0decodeImageQML\0imageUrl\0"
    "decodeSubImageQML\0offsetX\0offsetY\0"
    "width\0height\0DecoderFormat\0"
    "DecoderFormat_None\0DecoderFormat_QR_CODE\0"
    "DecoderFormat_DATA_MATRIX\0DecoderFormat_UPC_E\0"
    "DecoderFormat_UPC_A\0DecoderFormat_EAN_8\0"
    "DecoderFormat_EAN_13\0DecoderFormat_CODE_128\0"
    "DecoderFormat_CODE_39\0DecoderFormat_ITF\0"
    "DecoderFormat_Aztec"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_QZXing[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       1,  112, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   64,    2, 0x06 /* Public */,
       3,    1,   65,    2, 0x06 /* Public */,
       5,    1,   68,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    1,   71,    2, 0x0a /* Public */,
       9,    1,   74,    2, 0x0a /* Public */,
      11,    5,   77,    2, 0x0a /* Public */,
      11,    4,   88,    2, 0x2a /* Public | MethodCloned */,
      11,    3,   97,    2, 0x2a /* Public | MethodCloned */,
      11,    2,  104,    2, 0x2a /* Public | MethodCloned */,
      11,    1,  109,    2, 0x2a /* Public | MethodCloned */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    4,
    QMetaType::Void, QMetaType::QString,    6,

 // slots: parameters
    QMetaType::QString, QMetaType::QImage,    8,
    QMetaType::QString, QMetaType::QUrl,   10,
    QMetaType::QString, QMetaType::QUrl, QMetaType::Double, QMetaType::Double, QMetaType::Double, QMetaType::Double,   10,   12,   13,   14,   15,
    QMetaType::QString, QMetaType::QUrl, QMetaType::Double, QMetaType::Double, QMetaType::Double,   10,   12,   13,   14,
    QMetaType::QString, QMetaType::QUrl, QMetaType::Double, QMetaType::Double,   10,   12,   13,
    QMetaType::QString, QMetaType::QUrl, QMetaType::Double,   10,   12,
    QMetaType::QString, QMetaType::QUrl,   10,

 // enums: name, flags, count, data
      16, 0x0,   11,  116,

 // enum data: key, value
      17, uint(QZXing::DecoderFormat_None),
      18, uint(QZXing::DecoderFormat_QR_CODE),
      19, uint(QZXing::DecoderFormat_DATA_MATRIX),
      20, uint(QZXing::DecoderFormat_UPC_E),
      21, uint(QZXing::DecoderFormat_UPC_A),
      22, uint(QZXing::DecoderFormat_EAN_8),
      23, uint(QZXing::DecoderFormat_EAN_13),
      24, uint(QZXing::DecoderFormat_CODE_128),
      25, uint(QZXing::DecoderFormat_CODE_39),
      26, uint(QZXing::DecoderFormat_ITF),
      27, uint(QZXing::DecoderFormat_Aztec),

       0        // eod
};

void QZXing::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        QZXing *_t = static_cast<QZXing *>(_o);
        switch (_id) {
        case 0: _t->decodingStarted(); break;
        case 1: _t->decodingFinished((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->tagFound((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: { QString _r = _t->decodeImage((*reinterpret_cast< QImage(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 4: { QString _r = _t->decodeImageQML((*reinterpret_cast< const QUrl(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 5: { QString _r = _t->decodeSubImageQML((*reinterpret_cast< const QUrl(*)>(_a[1])),(*reinterpret_cast< const double(*)>(_a[2])),(*reinterpret_cast< const double(*)>(_a[3])),(*reinterpret_cast< const double(*)>(_a[4])),(*reinterpret_cast< const double(*)>(_a[5])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 6: { QString _r = _t->decodeSubImageQML((*reinterpret_cast< const QUrl(*)>(_a[1])),(*reinterpret_cast< const double(*)>(_a[2])),(*reinterpret_cast< const double(*)>(_a[3])),(*reinterpret_cast< const double(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 7: { QString _r = _t->decodeSubImageQML((*reinterpret_cast< const QUrl(*)>(_a[1])),(*reinterpret_cast< const double(*)>(_a[2])),(*reinterpret_cast< const double(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 8: { QString _r = _t->decodeSubImageQML((*reinterpret_cast< const QUrl(*)>(_a[1])),(*reinterpret_cast< const double(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 9: { QString _r = _t->decodeSubImageQML((*reinterpret_cast< const QUrl(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (QZXing::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&QZXing::decodingStarted)) {
                *result = 0;
            }
        }
        {
            typedef void (QZXing::*_t)(bool );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&QZXing::decodingFinished)) {
                *result = 1;
            }
        }
        {
            typedef void (QZXing::*_t)(QString );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&QZXing::tagFound)) {
                *result = 2;
            }
        }
    }
}

const QMetaObject QZXing::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QZXing.data,
      qt_meta_data_QZXing,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *QZXing::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QZXing::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_QZXing.stringdata))
        return static_cast<void*>(const_cast< QZXing*>(this));
    return QObject::qt_metacast(_clname);
}

int QZXing::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void QZXing::decodingStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void QZXing::decodingFinished(bool _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void QZXing::tagFound(QString _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_END_MOC_NAMESPACE
