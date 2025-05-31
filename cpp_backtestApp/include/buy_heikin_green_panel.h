#pragma once


#include <QObject>  // AJOUT OBLIGATOIRE
#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include "base_panel.h"

/**
 * @brief Panel spécifique à la stratégie BuyHeikinGreen
 * 
 * Ce panel contient les paramètres spécifiques à la stratégie BuyHeikinGreen comme
 * les filtres EMA, les paramètres stochastiques et RSI, etc.
 */
class BuyHeikinGreenPanel : public QObject, public BasePanel  // CORRECTION: Hériter de QObject
{
    Q_OBJECT  // AJOUT OBLIGATOIRE

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    BuyHeikinGreenPanel(QWidget* parent = nullptr);
    
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

private:
    /**
     * @brief Active/désactive un groupe de widgets
     * @param widgets Liste des widgets à configurer
     * @param enabled État d'activation à appliquer
     */
    void _toggleWidgetGroup(const QStringList& widgets, bool enabled);
};

