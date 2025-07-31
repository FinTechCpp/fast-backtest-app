#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>
#include "ui/panels/basePanel.h"
#include "Strategies/sell_heikin_red.hpp"

/**
 * @brief Panel spécifique à la stratégie SellHeikinRed
 * 
 * Ce panel contient les paramètres spécifiques à la stratégie SellHeikinRed.
 */
class SellHeikinRedPanel : public BasePanel
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    SellHeikinRedPanel(QWidget* parent = nullptr);
    
    // DEPRECATED
    QMap<QString, QVariant> getValues() override;
    void setValues(const QMap<QString, QVariant>& values) override;

    /**
     * @brief Récupère la configuration de la stratégie SellHeikinRed
     * @return Structure SellHeikinRedConfig remplie avec les valeurs du panel
     */
    SellHeikinRedConfig getConfig();
    void setConfig(const SellHeikinRedConfig& config);

private:
    SellHeikinRedConfig m_config;
    std::vector<std::unique_ptr<PropertyBinder>> m_bindings;

    void setupUI();
    
    void initializeBindings();
    void updateConfigFromWidgets();
    void updateWidgetsFromConfig();
    void addBinding(std::unique_ptr<PropertyBinder> binding);
    void createDependencyGroup(QCheckBox* checkbox, const std::vector<QWidget*>& dependentWidgets);
    
    // Anciennes méthodes qui seront remplacées par createDependencyGroup
    // void _toggleWidgetGroup(const QStringList& widgets, bool enabled);
};
