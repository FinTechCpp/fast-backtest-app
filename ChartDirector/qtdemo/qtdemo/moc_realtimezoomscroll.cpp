/****************************************************************************
** Meta object code from reading C++ file 'realtimezoomscroll.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "realtimezoomscroll.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'realtimezoomscroll.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RealTimeZoomScroll_t {
    QByteArrayData data[15];
    char stringdata0[202];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RealTimeZoomScroll_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RealTimeZoomScroll_t qt_meta_stringdata_RealTimeZoomScroll = {
    {
QT_MOC_LITERAL(0, 0, 18), // "RealTimeZoomScroll"
QT_MOC_LITERAL(1, 19, 19), // "onMouseUsageChanged"
QT_MOC_LITERAL(2, 39, 0), // ""
QT_MOC_LITERAL(3, 40, 16), // "QAbstractButton*"
QT_MOC_LITERAL(4, 57, 1), // "b"
QT_MOC_LITERAL(5, 59, 6), // "onSave"
QT_MOC_LITERAL(6, 66, 21), // "onUpdatePeriodChanged"
QT_MOC_LITERAL(7, 88, 19), // "onMouseMovePlotArea"
QT_MOC_LITERAL(8, 108, 12), // "QMouseEvent*"
QT_MOC_LITERAL(9, 121, 5), // "event"
QT_MOC_LITERAL(10, 127, 11), // "onDataTimer"
QT_MOC_LITERAL(11, 139, 18), // "onChartUpdateTimer"
QT_MOC_LITERAL(12, 158, 17), // "onViewPortChanged"
QT_MOC_LITERAL(13, 176, 19), // "onHScrollBarChanged"
QT_MOC_LITERAL(14, 196, 5) // "value"

    },
    "RealTimeZoomScroll\0onMouseUsageChanged\0"
    "\0QAbstractButton*\0b\0onSave\0"
    "onUpdatePeriodChanged\0onMouseMovePlotArea\0"
    "QMouseEvent*\0event\0onDataTimer\0"
    "onChartUpdateTimer\0onViewPortChanged\0"
    "onHScrollBarChanged\0value"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RealTimeZoomScroll[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x08 /* Private */,
       5,    1,   57,    2, 0x08 /* Private */,
       6,    1,   60,    2, 0x08 /* Private */,
       7,    1,   63,    2, 0x08 /* Private */,
      10,    0,   66,    2, 0x08 /* Private */,
      11,    0,   67,    2, 0x08 /* Private */,
      12,    0,   68,    2, 0x08 /* Private */,
      13,    1,   69,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   14,

       0        // eod
};

void RealTimeZoomScroll::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RealTimeZoomScroll *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onMouseUsageChanged((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 1: _t->onSave((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->onUpdatePeriodChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onMouseMovePlotArea((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 4: _t->onDataTimer(); break;
        case 5: _t->onChartUpdateTimer(); break;
        case 6: _t->onViewPortChanged(); break;
        case 7: _t->onHScrollBarChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject RealTimeZoomScroll::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_RealTimeZoomScroll.data,
    qt_meta_data_RealTimeZoomScroll,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *RealTimeZoomScroll::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RealTimeZoomScroll::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RealTimeZoomScroll.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int RealTimeZoomScroll::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
