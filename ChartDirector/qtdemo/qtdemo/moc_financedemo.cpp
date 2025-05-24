/****************************************************************************
** Meta object code from reading C++ file 'financedemo.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "financedemo.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'financedemo.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FinanceDemo_t {
    QByteArrayData data[12];
    char stringdata0[176];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_FinanceDemo_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_FinanceDemo_t qt_meta_stringdata_FinanceDemo = {
    {
QT_MOC_LITERAL(0, 0, 11), // "FinanceDemo"
QT_MOC_LITERAL(1, 12, 19), // "onMouseUsageChanged"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 16), // "QAbstractButton*"
QT_MOC_LITERAL(4, 50, 1), // "b"
QT_MOC_LITERAL(5, 52, 17), // "onComboBoxChanged"
QT_MOC_LITERAL(6, 70, 17), // "onCheckBoxChanged"
QT_MOC_LITERAL(7, 88, 17), // "onLineEditChanged"
QT_MOC_LITERAL(8, 106, 19), // "onMouseMovePlotArea"
QT_MOC_LITERAL(9, 126, 12), // "QMouseEvent*"
QT_MOC_LITERAL(10, 139, 18), // "onTimeRangeChanged"
QT_MOC_LITERAL(11, 158, 17) // "onViewPortChanged"

    },
    "FinanceDemo\0onMouseUsageChanged\0\0"
    "QAbstractButton*\0b\0onComboBoxChanged\0"
    "onCheckBoxChanged\0onLineEditChanged\0"
    "onMouseMovePlotArea\0QMouseEvent*\0"
    "onTimeRangeChanged\0onViewPortChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FinanceDemo[] = {

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
       1,    1,   49,    2, 0x08 /* Private */,
       5,    1,   52,    2, 0x08 /* Private */,
       6,    0,   55,    2, 0x08 /* Private */,
       7,    0,   56,    2, 0x08 /* Private */,
       8,    1,   57,    2, 0x08 /* Private */,
      10,    1,   60,    2, 0x08 /* Private */,
      11,    0,   63,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,

       0        // eod
};

void FinanceDemo::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FinanceDemo *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onMouseUsageChanged((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 1: _t->onComboBoxChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->onCheckBoxChanged(); break;
        case 3: _t->onLineEditChanged(); break;
        case 4: _t->onMouseMovePlotArea((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 5: _t->onTimeRangeChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->onViewPortChanged(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractButton* >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject FinanceDemo::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_FinanceDemo.data,
    qt_meta_data_FinanceDemo,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *FinanceDemo::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FinanceDemo::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FinanceDemo.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int FinanceDemo::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
