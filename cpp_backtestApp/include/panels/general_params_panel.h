#pragma once

#include <QObject>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QMap>
#include <QString>
#include <QVariant>
#include "panels/base_panel.h"

/**
 * @brief Panel des paramètres généraux du backtest
 * 
 * Ce panel contient les paramètres généraux comme le symbole,
 * la période, l'intervalle, le spread, etc.
 */
class GeneralParamsPanel : public QObject, public BasePanel
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    GeneralParamsPanel(QWidget* parent = nullptr);
    
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
    
    /**
     * @brief Récupère un widget par son nom
     * @param name Le nom du widget à récupérer
     * @return Pointeur vers le widget correspondant, ou nullptr si introuvable
     */
    QWidget* getWidgetByName(const QString& name) const;

private:
    // Dictionnaire associant les noms de stratégies à leurs classes
    QMap<QString, QString> m_strategyMap;
    
    // Initialisation du dictionnaire des stratégies
    void initStrategyMap();
    
    /**
     * @brief Configure les paramètres de symbole et de période
     * @param layout Le layout dans lequel les paramètres sont ajoutés
     * @param row La ligne actuelle du layout
     */
    void setupSymbolAndPeriod(QGridLayout* layout, int& row);
    
    /**
     * @brief Configure les paramètres de trading
     * @param layout Le layout dans lequel les paramètres sont ajoutés
     * @param row La ligne actuelle du layout
     */
    void setupTradingParameters(QGridLayout* layout, int& row);
};

