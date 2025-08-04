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
#include "ui/panels/ConfigPanel.h"
#include "common.h"

/**
 * @brief Panel pour les paramètres communs à toutes les stratégies
 * 
 * Ce panel contient les paramètres communs comme le stop loss, le take profit,
 * les heures de trading, la gestion du risque, etc.
 */
class StrategyBasePanel : public ConfigPanel<StrategyBaseConfig>
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    StrategyBasePanel(QWidget* parent = nullptr);

private slots:
    // Méthodes pour gérer l'interface utilisateur en fonction des checkboxes
    void _toggleSlMethod(int index);
    void _toggleTpMethod(int index);
    void _updateAtrPeriodStatus();

private:
    QMap<QString, QWidget*> m_widgets;

    void setupUI();
    
    void initializeBindings();
    void updateWidgetsFromConfig() override;
};

