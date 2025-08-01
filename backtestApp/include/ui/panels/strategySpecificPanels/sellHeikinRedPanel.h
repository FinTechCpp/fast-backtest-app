#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>
#include "ui/panels/ConfigPanel.h"
#include "Strategies/sell_heikin_red.hpp"

/**
 * @brief Panel spécifique à la stratégie SellHeikinRed
 * 
 * Ce panel contient les paramètres spécifiques à la stratégie SellHeikinRed.
 */
class SellHeikinRedPanel : public ConfigPanel<SellHeikinRedConfig>
{
    Q_OBJECT

public:
    SellHeikinRedPanel(QWidget* parent = nullptr);

private:
    QMap<QString, QWidget*> m_widgets;

    void setupUI();
    void initializeBindings();
};
