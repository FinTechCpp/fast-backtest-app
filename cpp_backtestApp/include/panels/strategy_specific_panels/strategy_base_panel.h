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
#include <QFrame>
#include <QMap>
#include <QString>
#include <QVariant>
#include "panels/base_panel.h"

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
    
    /**
     * @brief Initialise le contenu du panel
     */
    void initialize() override;
    
    /**
     * @brief Récupère les valeurs des widgets du panel
     * @return Map contenant les valeurs sous forme de QVariant
     */
    QMap<QString, QVariant> getValues() override;
    
    /**
     * @brief Définit les valeurs des widgets du panel
     * @param values Map contenant les valeurs à affecter aux widgets
     */
    void setValues(const QMap<QString, QVariant>& values) override;

private slots:
    // Méthodes pour gérer l'interface utilisateur en fonction des checkboxes
    void _toggleRiskControls(bool checked);
    void _toggleBreakEvenControls(bool checked);
    void _toggleDailyMaxLossControls(bool checked);
    void _toggleSlMethod(int index);
    void _toggleTpMethod(int index);
    void _updateAtrPeriodStatus();
};

