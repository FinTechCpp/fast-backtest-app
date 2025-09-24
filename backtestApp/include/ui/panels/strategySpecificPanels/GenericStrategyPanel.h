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
#include "Strategies/generic_strategy.hpp"


class GenericStrategyPanel : public ConfigPanel<GenericStrategyConfig>
{
    Q_OBJECT

public:
    GenericStrategyPanel(QWidget* parent = nullptr);

private:    
    void setupUI();
};