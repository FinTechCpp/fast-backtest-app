#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QDebug>
#include "ui/panels/ConfigPanel.h"
#include "Strategies/buy_heikin_green.hpp"


class BuyHeikinGreenPanel : public ConfigPanel<BuyHeikinGreenConfig>
{
    Q_OBJECT

public:
    BuyHeikinGreenPanel(QWidget* parent = nullptr);

private:    
    // QMap<QString, QWidget*> m_widgets;

    void setupUI();
    // void initializeBindings();
};

