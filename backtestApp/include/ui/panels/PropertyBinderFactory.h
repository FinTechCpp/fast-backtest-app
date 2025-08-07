#pragma once

#include "PropertyBinder.h"
#include "common.h"
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTimeEdit>
#include <QDateEdit>

class PropertyBinderFactory {
public:
    // Créer un binding pour un QCheckBox et un bool
    static std::unique_ptr<PropertyBinder> createBoolBinding(QCheckBox* widget, bool* property) {
        auto setter = [](QCheckBox* w, const bool& value) { w->setChecked(value); };
        auto getter = [](QCheckBox* w) -> bool { return w->isChecked(); };
        return std::make_unique<TypedPropertyBinder<QCheckBox, bool>>(widget, property, setter, getter);
    }
    
    // Créer un binding pour un QSpinBox et un int
    static std::unique_ptr<PropertyBinder> createIntBinding(QSpinBox* widget, int* property) {
        auto setter = [](QSpinBox* w, const int& value) { w->setValue(value); };
        auto getter = [](QSpinBox* w) -> int { return w->value(); };
        return std::make_unique<TypedPropertyBinder<QSpinBox, int>>(widget, property, setter, getter);
    }
    
    // Créer un binding pour un QDoubleSpinBox et un double
    static std::unique_ptr<PropertyBinder> createDoubleBinding(QDoubleSpinBox* widget, double* property) {
        auto setter = [](QDoubleSpinBox* w, const double& value) { w->setValue(value); };
        auto getter = [](QDoubleSpinBox* w) -> double { return w->value(); };
        return std::make_unique<TypedPropertyBinder<QDoubleSpinBox, double>>(widget, property, setter, getter);
    }

    // Créer un binding pour un QTimeEdit et un Time
    static std::unique_ptr<PropertyBinder> createTimeBinding(QTimeEdit* widget, Time* property) {
        auto setter = [](QTimeEdit* w, const Time& value) { w->setTime(QTime(value.hour, value.minute, value.second)); };
        auto getter = [](QTimeEdit* w) -> Time { 
            QTime qtime = w->time();
            return Time(qtime.hour(), qtime.minute(), qtime.second());
        };
        return std::make_unique<TypedPropertyBinder<QTimeEdit, Time>>(widget, property, setter, getter);
    }

    // Créer un binding pour un QComboBox et un enum
    template<typename EnumType>
    static std::unique_ptr<PropertyBinder> createEnumComboBinding(QComboBox* widget, EnumType* property) {        
        // Définir comment convertir enum vers QComboBox
        auto setter = [](QComboBox* w, const EnumType& value) {
            w->setCurrentIndex(static_cast<int>(value));
        };
        
        // Définir comment convertir QComboBox vers enum
        auto getter = [](QComboBox* w) -> EnumType {
            return static_cast<EnumType>(w->currentIndex());
        };
        
        return std::make_unique<TypedPropertyBinder<QComboBox, EnumType>>(widget, property, setter, getter);
    }

    // Créer un binding pour un QComboBox et un QString
    static std::unique_ptr<PropertyBinder> createStringComboBinding(QComboBox* widget, QString* property) {
        // Définir comment convertir QString vers QComboBox
        auto setter = [](QComboBox* w, const QString& value) {
            int index = w->findText(value);
            if (index >= 0) {
                w->setCurrentIndex(index);
            }
        };
        
        // Définir comment convertir QComboBox vers QString
        auto getter = [](QComboBox* w) -> QString {
            return w->currentText();
        };
        
        return std::make_unique<TypedPropertyBinder<QComboBox, QString>>(widget, property, setter, getter);
    }

    // Créer un binding pour un QComboBox et un std::string
    static std::unique_ptr<PropertyBinder> createStringComboBinding(QComboBox* widget, std::string* property) {
        // Définir comment convertir std::string vers QComboBox
        auto setter = [](QComboBox* w, const std::string& value) {
            int index = w->findText(QString::fromStdString(value));
            if (index >= 0) {
                w->setCurrentIndex(index);
            }
        };

        // Définir comment convertir QComboBox vers std::string
        auto getter = [](QComboBox* w) -> std::string {
            return w->currentText().toStdString();
        };

        return std::make_unique<TypedPropertyBinder<QComboBox, std::string>>(widget, property, setter, getter);
    }
    
    // Crée un binding pour un QDateEdit et un QDateTime
    static std::unique_ptr<PropertyBinder> createDateTimeBinding(QDateEdit* widget, QDateTime* property) {
        auto setter = [](QDateEdit* w, const QDateTime& value) { w->setDate(value.date()); };
        auto getter = [](QDateEdit* w) -> QDateTime { return QDateTime(w->date(), QTime()); };
        return std::make_unique<TypedPropertyBinder<QDateEdit, QDateTime>>(widget, property, setter, getter);
    }

    // Ajouter d'autres bindings selon vos besoins...
};