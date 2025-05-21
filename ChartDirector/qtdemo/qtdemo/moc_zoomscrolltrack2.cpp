/****************************************************************************
** Meta object code from reading C++ file 'zoomscrolltrack2.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "zoomscrolltrack2.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'zoomscrolltrack2.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ZoomScrollTrack2_t {
    QByteArrayData data[15];
    char stringdata0[200];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ZoomScrollTrack2_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ZoomScrollTrack2_t qt_meta_stringdata_ZoomScrollTrack2 = {
    {
QT_MOC_LITERAL(0, 0, 16), // "ZoomScrollTrack2"
QT_MOC_LITERAL(1, 17, 18), // "onSelectDataSeries"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 18), // "onStartDateChanged"
QT_MOC_LITERAL(4, 56, 4), // "date"
QT_MOC_LITERAL(5, 61, 16), // "onEndDateChanged"
QT_MOC_LITERAL(6, 78, 19), // "onHScrollBarChanged"
QT_MOC_LITERAL(7, 98, 5), // "value"
QT_MOC_LITERAL(8, 104, 19), // "onMouseUsageChanged"
QT_MOC_LITERAL(9, 124, 16), // "QAbstractButton*"
QT_MOC_LITERAL(10, 141, 1), // "b"
QT_MOC_LITERAL(11, 143, 17), // "onViewPortChanged"
QT_MOC_LITERAL(12, 161, 19), // "onMouseMovePlotArea"
QT_MOC_LITERAL(13, 181, 12), // "QMouseEvent*"
QT_MOC_LITERAL(14, 194, 5) // "event"

    },
    "ZoomScrollTrack2\0onSelectDataSeries\0"
    "\0onStartDateChanged\0date\0onEndDateChanged\0"
    "onHScrollBarChanged\0value\0onMouseUsageChanged\0"
    "QAbstractButton*\0b\0onViewPortChanged\0"
    "onMouseMovePlotArea\0QMouseEvent*\0event"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ZoomScrollTrack2[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   49,    2, 0x08 /* Private */,
       3,    1,   50,    2, 0x08 /* Private */,
       5,    1,   53,    2, 0x08 /* Private */,
       6,    1,   56,    2, 0x08 /* Private */,
       8,    1,   59,    2, 0x08 /* Private */,
      11,    0,   62,    2, 0x08 /* Private */,
      12,    1,   63,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QDateTime,    4,
    QMetaType::Void, QMetaType::QDateTime,    4,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 13,   14,

       0        // eod
};

void ZoomScrollTrack2::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ZoomScrollTrack2 *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onSelectDataSeries(); break;
        case 1: _t->onStartDateChanged((*reinterpret_cast< QDateTime(*)>(_a[1]))); break;
        case 2: _t->onEndDateChanged((*reinterpret_cast< QDateTime(*)>(_a[1]))); break;
        case 3: _t->onHScrollBarChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->onMouseUsageChanged((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 5: _t->onViewPortChanged(); break;
        case 6: _t->onMouseMovePlotArea((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractButton* >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ZoomScrollTrack2::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ZoomScrollTrack2.data,
    qt_meta_data_ZoomScrollTrack2,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ZoomScrollTrack2::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ZoomScrollTrack2::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZoomScrollTrack2.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int ZoomScrollTrack2::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
