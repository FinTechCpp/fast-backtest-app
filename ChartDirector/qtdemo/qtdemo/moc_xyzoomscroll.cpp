/****************************************************************************
** Meta object code from reading C++ file 'xyzoomscroll.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "xyzoomscroll.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'xyzoomscroll.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_XYZoomScroll_t {
    QByteArrayData data[13];
    char stringdata0[153];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_XYZoomScroll_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_XYZoomScroll_t qt_meta_stringdata_XYZoomScroll = {
    {
QT_MOC_LITERAL(0, 0, 12), // "XYZoomScroll"
QT_MOC_LITERAL(1, 13, 19), // "onMouseUsageChanged"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 16), // "QAbstractButton*"
QT_MOC_LITERAL(4, 51, 1), // "b"
QT_MOC_LITERAL(5, 53, 6), // "onSave"
QT_MOC_LITERAL(6, 60, 16), // "onZoomBarChanged"
QT_MOC_LITERAL(7, 77, 5), // "value"
QT_MOC_LITERAL(8, 83, 19), // "onMouseMovePlotArea"
QT_MOC_LITERAL(9, 103, 12), // "QMouseEvent*"
QT_MOC_LITERAL(10, 116, 5), // "event"
QT_MOC_LITERAL(11, 122, 17), // "onViewPortChanged"
QT_MOC_LITERAL(12, 140, 12) // "onClickChart"

    },
    "XYZoomScroll\0onMouseUsageChanged\0\0"
    "QAbstractButton*\0b\0onSave\0onZoomBarChanged\0"
    "value\0onMouseMovePlotArea\0QMouseEvent*\0"
    "event\0onViewPortChanged\0onClickChart"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_XYZoomScroll[] = {

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
       5,    1,   47,    2, 0x08 /* Private */,
       6,    1,   50,    2, 0x08 /* Private */,
       8,    1,   53,    2, 0x08 /* Private */,
      11,    0,   56,    2, 0x08 /* Private */,
      12,    1,   57,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9,   10,

       0        // eod
};

void XYZoomScroll::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<XYZoomScroll *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onMouseUsageChanged((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 1: _t->onSave((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->onZoomBarChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onMouseMovePlotArea((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 4: _t->onViewPortChanged(); break;
        case 5: _t->onClickChart((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject XYZoomScroll::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_XYZoomScroll.data,
    qt_meta_data_XYZoomScroll,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *XYZoomScroll::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *XYZoomScroll::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_XYZoomScroll.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int XYZoomScroll::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
