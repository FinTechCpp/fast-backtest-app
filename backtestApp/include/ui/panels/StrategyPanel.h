#pragma once

#include <QGroupBox>
#include <QVBoxLayout>
#include <QPushButton>
#include "ui/panels/ConfigPanel.h"
#include "common.h"

/**
 * @brief Panel pour les paramètres communs à toutes les stratégies
 * 
 * Ce panel contient un bouton pour ouvrir le dialog de configuration complet de la stratégie
 */
class StrategyPanel : public ConfigPanel<StrategyConfig>
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    StrategyPanel(QWidget* parent = nullptr);

private:
    void setupUI();
    void openStrategyConfigDialog();
};
