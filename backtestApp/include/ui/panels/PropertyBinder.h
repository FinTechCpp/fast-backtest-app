#pragma once

#include <QWidget>
#include <functional>
#include <memory>

// Classe abstraite qui représente un binding entre un widget et une propriété
class PropertyBinder {
public:
    virtual ~PropertyBinder() = default;
    
    // Méthodes pour synchroniser dans les deux directions
    virtual void updateWidgetFromProperty() = 0;
    virtual void updatePropertyFromWidget() = 0;
    
    // Gestion de l'activation/désactivation
    virtual void setEnabled(bool enabled) = 0;
};

// Classe template qui implémente PropertyBinder pour différents types de widgets/propriétés
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
        
        // Style pour indiquer visuellement si le widget est activé
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