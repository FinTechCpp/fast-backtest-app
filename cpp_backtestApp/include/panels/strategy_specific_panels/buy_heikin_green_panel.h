#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include "panels/base_panel.h"

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
    // Nouveau: gestionnaires d'événements pour les checkboxes
    void onEmaShortFilterToggled(bool checked);
    void onEmaLongFilterToggled(bool checked);
    void onRsiFilterToggled(bool checked);
    void onStochFilterToggled(bool checked);

private:
    /**
     * @brief Active/désactive un groupe de widgets
     * @param widgets Liste des widgets à configurer
     * @param enabled État d'activation à appliquer
     */
    void _toggleWidgetGroup(const QStringList& widgets, bool enabled);
};

