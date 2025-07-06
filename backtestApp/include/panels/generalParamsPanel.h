#pragma once

#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QMap>
#include <QString>
#include <QVariant>
#include "panels/basePanel.h"

/**
 * @brief Panel des paramètres généraux du backtest
 * 
 * Ce panel contient les paramètres généraux comme le symbole,
 * la période, l'intervalle, le spread, etc.
 */
class GeneralParamsPanel : public BasePanel
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    GeneralParamsPanel(QWidget* parent = nullptr);
    
    /**
     * @brief Initialise le contenu du panel
     * Cette méthode configure l'interface graphique du panel
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
};

