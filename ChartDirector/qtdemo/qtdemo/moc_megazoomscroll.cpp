/****************************************************************************
** Meta object code from reading C++ file 'megazoomscroll.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "megazoomscroll.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'megazoomscroll.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MegaZoomScroll_t {
    QByteArrayData data[12];
    char stringdata0[155];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MegaZoomScroll_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MegaZoomScroll_t qt_meta_stringdata_MegaZoomScroll = {
    {
QT_MOC_LITERAL(0, 0, 14), // "MegaZoomScroll"
QT_MOC_LITERAL(1, 15, 16), // "onClickPlotChart"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 19), // "onHScrollBarChanged"
QT_MOC_LITERAL(4, 53, 5), // "value"
QT_MOC_LITERAL(5, 59, 19), // "onMouseUsageChanged"
QT_MOC_LITERAL(6, 79, 16), // "QAbstractButton*"
QT_MOC_LITERAL(7, 96, 1), // "b"
QT_MOC_LITERAL(8, 98, 17), // "onViewPortChanged"
QT_MOC_LITERAL(9, 116, 19), // "onMouseMovePlotArea"
QT_MOC_LITERAL(10, 136, 12), // "QMouseEvent*"
QT_MOC_LITERAL(11, 149, 5) // "event"

    },
    "MegaZoomScroll\0onClickPlotChart\0\0"
    "onHScrollBarChanged\0value\0onMouseUsageChanged\0"
    "QAbstractButton*\0b\0onViewPortChanged\0"
    "onMouseMovePlotArea\0QMouseEvent*\0event"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MegaZoomScroll[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x08 /* Private */,
       3,    1,   42,    2, 0x08 /* Private */,
       5,    1,   45,    2, 0x08 /* Private */,
       8,    0,   48,    2, 0x08 /* Private */,
       9,    1,   49,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 10,   11,

       0        // eod
};

void MegaZoomScroll::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MegaZoomScroll *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onClickPlotChart((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->onHScrollBarChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->onMouseUsageChanged((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 3: _t->onViewPortChanged(); break;
        case 4: _t->onMouseMovePlotArea((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MegaZoomScroll::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_MegaZoomScroll.data,
    qt_meta_data_MegaZoomScroll,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MegaZoomScroll::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MegaZoomScroll::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MegaZoomScroll.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int MegaZoomScroll::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
