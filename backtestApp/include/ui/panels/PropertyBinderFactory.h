#pragma once

#include "PropertyBinder.h"
#include "common.h"
#include "ui/panels/FiltersWidget.h"
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTimeEdit>
#include <QDateEdit>

class PropertyBinderFactory {
public:
    // Create a binding for a QCheckBox and a bool
    static std::unique_ptr<PropertyBinder> createBoolBinding(QCheckBox* widget, bool* property) {
        auto setter = [](QCheckBox* w, const bool& value) { w->setChecked(value); };
        auto getter = [](QCheckBox* w) -> bool { return w->isChecked(); };
        return std::make_unique<TypedPropertyBinder<QCheckBox, bool>>(widget, property, setter, getter);
    }

    static std::unique_ptr<PropertyBinder> createOptionalBoolBinding(QCheckBox* widget, std::optional<bool>* property) {
        // Setter: from optional to widget
        auto setter = [](QCheckBox* w, const std::optional<bool>& value) { 
            if (value.has_value()) {
                w->setChecked(value.value());
                w->setTristate(false);
            } else {
                w->setTristate(true);
                w->setCheckState(Qt::PartiallyChecked);
            }
        };
        
        // Getter: from widget to optional
        auto getter = [](QCheckBox* w) -> std::optional<bool> { 
            if (w->checkState() == Qt::PartiallyChecked) {
                return std::nullopt;
            }
            return w->isChecked();
        };
        
        return std::make_unique<TypedPropertyBinder<QCheckBox, std::optional<bool>>>(widget, property, setter, getter);
    }

    static std::unique_ptr<PropertyBinder> createFiltersBinding(FiltersWidget* widget, std::vector<filter::GenericFilter>* property) {
        auto setter = [](FiltersWidget* w, const std::vector<filter::GenericFilter>& value) { w->setFilters(value); };
        auto getter = [](FiltersWidget* w) -> std::vector<filter::GenericFilter> { return w->getFilters(); };
        return std::make_unique<TypedPropertyBinder<FiltersWidget, std::vector<filter::GenericFilter>>>(widget, property, setter, getter);
    }
    
    // Create a binding for a QSpinBox and an int
    static std::unique_ptr<PropertyBinder> createIntBinding(QSpinBox* widget, int* property) {
        auto setter = [](QSpinBox* w, const int& value) { w->setValue(value); };
        auto getter = [](QSpinBox* w) -> int { return w->value(); };
        return std::make_unique<TypedPropertyBinder<QSpinBox, int>>(widget, property, setter, getter);
    }
    
    // Create a binding for a QDoubleSpinBox and a double
    static std::unique_ptr<PropertyBinder> createDoubleBinding(QDoubleSpinBox* widget, double* property) {
        auto setter = [](QDoubleSpinBox* w, const double& value) { w->setValue(value); };
        auto getter = [](QDoubleSpinBox* w) -> double { return w->value(); };
        return std::make_unique<TypedPropertyBinder<QDoubleSpinBox, double>>(widget, property, setter, getter);
    }

    // Create a binding for a QDoubleSpinBox and a float
    static std::unique_ptr<PropertyBinder> createFloatBinding(QDoubleSpinBox* widget, float* property) {
        auto setter = [](QDoubleSpinBox* w, const float& value) { w->setValue(static_cast<double>(value)); };
        auto getter = [](QDoubleSpinBox* w) -> float { return static_cast<float>(w->value()); };
        return std::make_unique<TypedPropertyBinder<QDoubleSpinBox, float>>(widget, property, setter, getter);
    }

    // Create a binding for a QLineEdit and a std::string
    static std::unique_ptr<PropertyBinder> createStringBinding(QLineEdit* widget, std::string* property) {
        auto setter = [](QLineEdit* w, const std::string& value) { w->setText(QString::fromStdString(value)); };
        auto getter = [](QLineEdit* w) -> std::string { return w->text().toStdString(); };
        return std::make_unique<TypedPropertyBinder<QLineEdit, std::string>>(widget, property, setter, getter);
    }

    // Create a binding for a QTimeEdit and a Time
    static std::unique_ptr<PropertyBinder> createTimeBinding(QTimeEdit* widget, Time* property) {
        auto setter = [](QTimeEdit* w, const Time& value) { w->setTime(QTime(value.hour, value.minute, value.second)); };
        auto getter = [](QTimeEdit* w) -> Time { 
            QTime qtime = w->time();
            return Time(qtime.hour(), qtime.minute(), qtime.second());
        };
        return std::make_unique<TypedPropertyBinder<QTimeEdit, Time>>(widget, property, setter, getter);
    }

    // Create a binding for a QComboBox and an enum
    template<typename EnumType>
    static std::unique_ptr<PropertyBinder> createEnumComboBinding(QComboBox* widget, EnumType* property) {        
        // Define how to convert enum to QComboBox
        auto setter = [](QComboBox* w, const EnumType& value) {
            w->setCurrentIndex(static_cast<int>(value));
        };
        
        // Define how to convert QComboBox to enum
        auto getter = [](QComboBox* w) -> EnumType {
            return static_cast<EnumType>(w->currentIndex());
        };
        
        return std::make_unique<TypedPropertyBinder<QComboBox, EnumType>>(widget, property, setter, getter);
    }

    // Create a binding for a QComboBox and a QString
    static std::unique_ptr<PropertyBinder> createStringComboBinding(QComboBox* widget, QString* property) {
        // Define how to convert QString to QComboBox
        auto setter = [](QComboBox* w, const QString& value) {
            int index = w->findText(value);
            if (index >= 0) {
                w->setCurrentIndex(index);
            }
        };
        
        // Define how to convert QComboBox to QString
        auto getter = [](QComboBox* w) -> QString {
            return w->currentText();
        };
        
        return std::make_unique<TypedPropertyBinder<QComboBox, QString>>(widget, property, setter, getter);
    }

    // Create a binding for a QComboBox and a std::string
    static std::unique_ptr<PropertyBinder> createStringComboBinding(QComboBox* widget, std::string* property) {
        // Define how to convert std::string to QComboBox
        auto setter = [](QComboBox* w, const std::string& value) {
            int index = w->findText(QString::fromStdString(value));
            if (index >= 0) {
                w->setCurrentIndex(index);
            }
        };

        // Define how to convert QComboBox to std::string
        auto getter = [](QComboBox* w) -> std::string {
            return w->currentText().toStdString();
        };

        return std::make_unique<TypedPropertyBinder<QComboBox, std::string>>(widget, property, setter, getter);
    }
    
    // Create a binding for a QDateEdit and a QDateTime
    static std::unique_ptr<PropertyBinder> createDateTimeBinding(QDateEdit* widget, QDateTime* property) {
        auto setter = [](QDateEdit* w, const QDateTime& value) { w->setDate(value.date()); };
        auto getter = [](QDateEdit* w) -> QDateTime { return QDateTime(w->date(), QTime()); };
        return std::make_unique<TypedPropertyBinder<QDateEdit, QDateTime>>(widget, property, setter, getter);
    }

    // Add other bindings as needed...
};