#pragma once

#include <QWidget>
#include <functional>
#include <memory>

// Abstract class that represents a binding between a widget and a property
class PropertyBinder {
public:
    virtual ~PropertyBinder() = default;
    
    // Methods to synchronize in both directions
    virtual void updateWidgetFromProperty() = 0;
    virtual void updatePropertyFromWidget() = 0;
    
    // Enable/disable management
    virtual void setEnabled(bool enabled) = 0;
};

// Template class that implements PropertyBinder for different types of widgets/properties
template<typename WidgetType, typename PropType>
class TypedPropertyBinder : public PropertyBinder {
public:
    TypedPropertyBinder(
        WidgetType* widget, 
        PropType* property,
        std::function<void(WidgetType*, const PropType&)> widgetSetter,
        std::function<PropType(WidgetType*)> widgetGetter
    ) : 
        m_widget(widget), 
        m_property(property),
        m_widgetSetter(widgetSetter),
        m_widgetGetter(widgetGetter) {}
    
    void updateWidgetFromProperty() override {
        if (!m_widget || !m_property) 
            return;
        m_widgetSetter(m_widget, *m_property);
    }
    
    void updatePropertyFromWidget() override {
        if (!m_widget || !m_property) 
            return;
        *m_property = m_widgetGetter(m_widget);
    }
    
    void setEnabled(bool enabled) override {
        if (!m_widget) return;

        m_widget->setEnabled(enabled);
        
        // Style to visually indicate whether the widget is enabled
        if (enabled) {
            m_widget->setStyleSheet("");
        } else {
            m_widget->setStyleSheet("background-color: #f0f0f0; color: #888888;");
        }
    }

private:
    WidgetType* m_widget;
    PropType* m_property;
    std::function<void(WidgetType*, const PropType&)> m_widgetSetter;
    std::function<PropType(WidgetType*)> m_widgetGetter;
};