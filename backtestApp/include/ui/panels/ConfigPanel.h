// ConfigPanel.h
#pragma once

#include <QWidget>
#include <QCheckBox>
#include <vector>
#include <memory>
#include "PropertyBinder.h"

template<typename ConfigType>
class ConfigPanel : public QWidget {
public:
    ConfigPanel(QWidget* parent = nullptr) : QWidget(parent) {}
    virtual ~ConfigPanel() = default;
    
    // Retourne directement la configuration
    ConfigType getConfig() {
        updateConfigFromWidgets();
        return m_config;
    }
    
    // Définit la configuration et met à jour tous les widgets
    void setConfig(const ConfigType& config) {
        m_config = config;
        updateWidgetsFromConfig();
    }

protected:
    // La configuration actuelle
    ConfigType m_config;
    
    // Liste des bindings entre widgets et propriétés
    std::vector<std::unique_ptr<PropertyBinder>> m_bindings;
    
    // Ajoute un binding à la liste
    void addBinding(std::unique_ptr<PropertyBinder> binding) {
        m_bindings.push_back(std::move(binding));
    }
    
    // Met à jour tous les widgets en fonction de la configuration actuelle
    void updateWidgetsFromConfig() {
        for (auto& binding : m_bindings) {
            binding->updateWidgetFromProperty();
        }
    }
    
    // Met à jour la configuration en fonction des widgets
    void updateConfigFromWidgets() {
        for (auto& binding : m_bindings) {
            binding->updatePropertyFromWidget();
        }
    }
    
    // Méthode pour créer un groupe de dépendance
    // (widgets qui sont activés/désactivés en fonction d'une case à cocher)
    void createDependencyGroup(QCheckBox* checkbox, std::vector<PropertyBinder*> dependentBinders) {
        auto updateFunc = [checkbox, dependentBinders]() {
            bool checked = checkbox->isChecked();
            for (auto binder : dependentBinders) {
                binder->setEnabled(checked);
            }
        };
        
        // Connecter le signal toggled au callback
        connect(checkbox, &QCheckBox::toggled, this, updateFunc);
        
        // Appliquer l'état initial
        updateFunc();
    }
};