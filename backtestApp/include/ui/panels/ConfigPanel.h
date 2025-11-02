#pragma once

#include <QWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <vector>
#include <memory>
#include "ui/panels/PropertyBinderFactory.h"

template<typename ConfigType>
class ConfigPanel : public QGroupBox {
public:
    ConfigPanel(const QString& title, QWidget* parent = nullptr) : QGroupBox(title, parent) {}
    virtual ~ConfigPanel() = default;
    
    // Directly returns the configuration
    ConfigType getConfig() {
        updateConfigFromWidgets();
        return m_config;
    }
    
    // Sets the configuration and updates all widgets
    void setConfig(const ConfigType& config) {
        m_config = config;
        updateWidgetsFromConfig();
    }

protected:
    // The current configuration
    ConfigType m_config;
    
    // List of bindings between widgets and properties
    std::vector<std::unique_ptr<PropertyBinder>> m_bindings;
    
    // Adds a binding to the list
    void addBinding(std::unique_ptr<PropertyBinder> binding) {
        m_bindings.push_back(std::move(binding));
    }
    
    // Updates all widgets based on the current configuration
    virtual void updateWidgetsFromConfig() {
        for (auto& binding : m_bindings) {
            binding->updateWidgetFromProperty();
        }
    }
    
    // Updates the configuration based on the widgets
    void updateConfigFromWidgets() {
        for (auto& binding : m_bindings) {
            binding->updatePropertyFromWidget();
        }
    }
    
    // Method to create a dependency group
    // (widgets that are enabled/disabled based on a checkbox)
    void createDependencyGroup(QCheckBox* checkbox, const std::vector<QWidget*>& dependentWidgets) {
        auto updateFunc = [checkbox, dependentWidgets]() {
            bool checked = checkbox->isChecked();
            for (QWidget* widget : dependentWidgets) {
                widget->setEnabled(checked);
                // Update the style
                if (checked)
                    widget->setStyleSheet("background-color: #ffffff; color: #000000;");
                else
                    widget->setStyleSheet("background-color: #f0f0f0; color: #888888;");
            }
        };
        
        // Connect the toggled signal to the callback
        connect(checkbox, &QCheckBox::toggled, this, updateFunc);
        
        // Apply the initial state
        updateFunc();
    }
};