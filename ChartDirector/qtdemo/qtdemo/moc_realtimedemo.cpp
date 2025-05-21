/****************************************************************************
** Meta object code from reading C++ file 'realtimedemo.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "realtimedemo.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'realtimedemo.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RealTimeDemo_t {
    QByteArrayData data[9];
    char stringdata0[104];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RealTimeDemo_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RealTimeDemo_t qt_meta_stringdata_RealTimeDemo = {
    {
QT_MOC_LITERAL(0, 0, 12), // "RealTimeDemo"
QT_MOC_LITERAL(1, 13, 18), // "onRunFreezeChanged"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 16), // "QAbstractButton*"
QT_MOC_LITERAL(4, 50, 1), // "b"
QT_MOC_LITERAL(5, 52, 21), // "onUpdatePeriodChanged"
QT_MOC_LITERAL(6, 74, 7), // "getData"
QT_MOC_LITERAL(7, 82, 11), // "updateChart"
QT_MOC_LITERAL(8, 94, 9) // "drawChart"

    },
    "RealTimeDemo\0onRunFreezeChanged\0\0"
    "QAbstractButton*\0b\0onUpdatePeriodChanged\0"
    "getData\0updateChart\0drawChart"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RealTimeDemo[] = {

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
       5,    1,   42,    2, 0x08 /* Private */,
       6,    0,   45,    2, 0x08 /* Private */,
       7,    0,   46,    2, 0x08 /* Private */,
       8,    0,   47,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void RealTimeDemo::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RealTimeDemo *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onRunFreezeChanged((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 1: _t->onUpdatePeriodChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->getData(); break;
        case 3: _t->updateChart(); break;
        case 4: _t->drawChart(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject RealTimeDemo::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_RealTimeDemo.data,
    qt_meta_data_RealTimeDemo,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *RealTimeDemo::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RealTimeDemo::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RealTimeDemo.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int RealTimeDemo::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
