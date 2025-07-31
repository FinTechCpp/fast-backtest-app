#pragma once

#include <QGroupBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTimeEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QFrame>
#include <QMap>
#include <QString>
#include <QVariant>
#include "ui/panels/basePanel.h"
#include "common.h"
#include "ui/panels/PropertyBinderFactory.h"

/**
 * @brief Panel pour les paramètres communs à toutes les stratégies
 * 
 * Ce panel contient les paramètres communs comme le stop loss, le take profit,
 * les heures de trading, la gestion du risque, etc.
 */
class StrategyBasePanel : public BasePanel
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    StrategyBasePanel(QWidget* parent = nullptr);
    
    // Methode a supprimer
    QMap<QString, QVariant> getValues() override;
    void setValues(const QMap<QString, QVariant>& values) override;

    StrategyBaseConfig getConfig();
    void setConfig(const StrategyBaseConfig& config);


private slots:
    // Méthodes pour gérer l'interface utilisateur en fonction des checkboxes
    // void _toggleRiskControls(bool checked);
    // void _toggleBreakEvenControls(bool checked);
    // void _toggleDailyMaxLossControls(bool checked);
    // void _toggleDailyMaxProfitControls(bool checked);
    // void _toggleDailyMaxDrawdownControls(bool checked);
    void _toggleSlMethod(int index);
    void _toggleTpMethod(int index);
    void _updateAtrPeriodStatus();

private:
    StrategyBaseConfig m_config;
    std::vector<std::unique_ptr<PropertyBinder>> m_bindings;

    void setupUI();
    
    void initializeBindings();
    void updateConfigFromWidgets();
    void updateWidgetsFromConfig();
    void addBinding(std::unique_ptr<PropertyBinder> binding);
    void createDependencyGroup(QCheckBox* checkbox, const std::vector<QWidget*>& dependentWidgets);
};

