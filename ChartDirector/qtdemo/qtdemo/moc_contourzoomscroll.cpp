/****************************************************************************
** Meta object code from reading C++ file 'contourzoomscroll.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "contourzoomscroll.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'contourzoomscroll.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ContourZoomScroll_t {
    QByteArrayData data[9];
    char stringdata0[106];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ContourZoomScroll_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ContourZoomScroll_t qt_meta_stringdata_ContourZoomScroll = {
    {
QT_MOC_LITERAL(0, 0, 17), // "ContourZoomScroll"
QT_MOC_LITERAL(1, 18, 19), // "onMouseUsageChanged"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 16), // "QAbstractButton*"
QT_MOC_LITERAL(4, 56, 1), // "b"
QT_MOC_LITERAL(5, 58, 6), // "onSave"
QT_MOC_LITERAL(6, 65, 16), // "onZoomBarChanged"
QT_MOC_LITERAL(7, 82, 5), // "value"
QT_MOC_LITERAL(8, 88, 17) // "onViewPortChanged"

    },
    "ContourZoomScroll\0onMouseUsageChanged\0"
    "\0QAbstractButton*\0b\0onSave\0onZoomBarChanged\0"
    "value\0onViewPortChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ContourZoomScroll[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x08 /* Private */,
       5,    1,   37,    2, 0x08 /* Private */,
       6,    1,   40,    2, 0x08 /* Private */,
       8,    0,   43,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,

       0        // eod
};

void ContourZoomScroll::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ContourZoomScroll *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onMouseUsageChanged((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 1: _t->onSave((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->onZoomBarChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onViewPortChanged(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ContourZoomScroll::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ContourZoomScroll.data,
    qt_meta_data_ContourZoomScroll,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ContourZoomScroll::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ContourZoomScroll::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ContourZoomScroll.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int ContourZoomScroll::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
