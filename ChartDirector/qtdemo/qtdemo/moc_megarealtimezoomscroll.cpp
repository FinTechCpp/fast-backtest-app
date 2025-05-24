/****************************************************************************
** Meta object code from reading C++ file 'megarealtimezoomscroll.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "megarealtimezoomscroll.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'megarealtimezoomscroll.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MegaRealTimeZoomScroll_t {
    QByteArrayData data[13];
    char stringdata0[182];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MegaRealTimeZoomScroll_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MegaRealTimeZoomScroll_t qt_meta_stringdata_MegaRealTimeZoomScroll = {
    {
QT_MOC_LITERAL(0, 0, 22), // "MegaRealTimeZoomScroll"
QT_MOC_LITERAL(1, 23, 16), // "onClickPlotChart"
QT_MOC_LITERAL(2, 40, 0), // ""
QT_MOC_LITERAL(3, 41, 19), // "onHScrollBarChanged"
QT_MOC_LITERAL(4, 61, 5), // "value"
QT_MOC_LITERAL(5, 67, 19), // "onMouseUsageChanged"
QT_MOC_LITERAL(6, 87, 16), // "QAbstractButton*"
QT_MOC_LITERAL(7, 104, 1), // "b"
QT_MOC_LITERAL(8, 106, 17), // "onViewPortChanged"
QT_MOC_LITERAL(9, 124, 19), // "onMouseMovePlotArea"
QT_MOC_LITERAL(10, 144, 12), // "QMouseEvent*"
QT_MOC_LITERAL(11, 157, 5), // "event"
QT_MOC_LITERAL(12, 163, 18) // "onChartUpdateTimer"

    },
    "MegaRealTimeZoomScroll\0onClickPlotChart\0"
    "\0onHScrollBarChanged\0value\0"
    "onMouseUsageChanged\0QAbstractButton*\0"
    "b\0onViewPortChanged\0onMouseMovePlotArea\0"
    "QMouseEvent*\0event\0onChartUpdateTimer"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MegaRealTimeZoomScroll[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x08 /* Private */,
       3,    1,   47,    2, 0x08 /* Private */,
       5,    1,   50,    2, 0x08 /* Private */,
       8,    0,   53,    2, 0x08 /* Private */,
       9,    1,   54,    2, 0x08 /* Private */,
      12,    0,   57,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void,

       0        // eod
};

void MegaRealTimeZoomScroll::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MegaRealTimeZoomScroll *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onClickPlotChart((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->onHScrollBarChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->onMouseUsageChanged((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 3: _t->onViewPortChanged(); break;
        case 4: _t->onMouseMovePlotArea((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 5: _t->onChartUpdateTimer(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MegaRealTimeZoomScroll::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_MegaRealTimeZoomScroll.data,
    qt_meta_data_MegaRealTimeZoomScroll,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MegaRealTimeZoomScroll::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MegaRealTimeZoomScroll::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MegaRealTimeZoomScroll.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int MegaRealTimeZoomScroll::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
