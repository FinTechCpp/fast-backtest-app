#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>
#include "panels/base_panel.h"
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

    /**
     * @brief Récupère la configuration de la stratégie SellHeikinRed
     * @return Structure SellHeikinRedConfig remplie avec les valeurs du panel
     */
    SellHeikinRedConfig getConfig() {
        return convertToConfig<SellHeikinRedConfig>(getValues());
    }

private slots:
    /**
     * @brief Gère l'activation/désactivation du filtre EMA court
     * @param checked État de la checkbox
     */
    void onEmaShortFilterToggled(bool checked);

private:
    /**
     * @brief Active/désactive un groupe de widgets
     * @param widgets Liste des widgets à configurer
     * @param enabled État d'activation à appliquer
     */
    void _toggleWidgetGroup(const QStringList& widgets, bool enabled);
};

