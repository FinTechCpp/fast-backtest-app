#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include "ui/panels/basePanel.h"
#include "Strategies/buy_heikin_green.hpp"
// #include "ui/panels/PropertyBinderFactory.h"


/**
 * @brief Panel spécifique à la stratégie BuyHeikinGreen
 * 
 * Ce panel contient les paramètres spécifiques à la stratégie BuyHeikinGreen comme
 * les filtres EMA, les paramètres stochastiques et RSI, etc.
 */
class BuyHeikinGreenPanel : public BasePanel
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    BuyHeikinGreenPanel(QWidget* parent = nullptr);
    
    // DEPRECATED
    QMap<QString, QVariant> getValues() override;
    void setValues(const QMap<QString, QVariant>& values) override;


    BuyHeikinGreenConfig getConfig();
    void setConfig(const BuyHeikinGreenConfig& config);

private:
    BuyHeikinGreenConfig m_config;
    std::vector<std::unique_ptr<PropertyBinder>> m_bindings;

    void setupUI();

    void initializeBindings();
    void updateConfigFromWidgets();
    void updateWidgetsFromConfig();
    void addBinding(std::unique_ptr<PropertyBinder> binding);
    void createDependencyGroup(QCheckBox* checkbox, const std::vector<QWidget*>& dependentWidgets);
};

