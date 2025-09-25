#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QPushButton>
#include <QListWidget>
#include <QDebug>
#include "ui/panels/ConfigPanel.h"
#include "Strategies/generic_strategy.hpp"

class StrategyCreationDialog; // Forward declaration

class GenericStrategyPanel : public ConfigPanel<GenericStrategyConfig>
{
    Q_OBJECT

public:
    GenericStrategyPanel(QWidget* parent = nullptr);

signals:
    void configChanged();

private slots:
    void onCreateStrategy();
    void onEditStrategy();
    void onClearStrategy();

private:    
    void setupUI();
    void updateUI();
    void updateConfig(const GenericStrategyConfig& newConfig);

    // UI Components
    QLabel* m_currentStrategyLabel;
    QPushButton* m_createStrategyBtn;
    QPushButton* m_editStrategyBtn;
    QPushButton* m_clearStrategyBtn;
    QListWidget* m_filtersListWidget;
};