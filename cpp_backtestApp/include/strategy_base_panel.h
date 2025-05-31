#pragma once

#include <QObject>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QFormLayout>  // AJOUT OBLIGATOIRE
#include <QGridLayout>
#include <QLabel>
#include <QTimeEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>    // AJOUT OBLIGATOIRE
#include <QFrame>       // AJOUT OBLIGATOIRE
#include <QMap>
#include <QString>
#include <QVariant>
#include "base_panel.h"

/**
 * @brief Panel pour les paramètres communs à toutes les stratégies
 * 
 * Ce panel contient les paramètres communs comme le stop loss, le take profit,
 * les heures de trading, la gestion du risque, etc.
 */
class StrategyBasePanel : public QObject, public BasePanel  // CORRECTION: Hériter de QObject
{
    Q_OBJECT  // AJOUT OBLIGATOIRE pour les signaux/slots

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    StrategyBasePanel(QWidget* parent = nullptr);
    
    /**
     * @brief Crée et retourne l'interface graphique du panel
     * @return QGroupBox contenant les widgets du panel
     */
    QGroupBox* create() override;
    
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

private:
    void setupTradingHours(QGridLayout* layout, int& row);
    void setupStopLossAndTakeProfit(QGridLayout* layout, int& row);
    void setupRiskManagement(QGridLayout* layout, int& row);
};

