// PropertyBinderFactory.h
#pragma once

#include "PropertyBinder.h"
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>

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

    // Ajouter d'autres bindings selon vos besoins...
};