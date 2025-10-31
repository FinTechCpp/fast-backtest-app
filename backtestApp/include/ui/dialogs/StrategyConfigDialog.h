#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include "common.h"

class FiltersWidget;

class StrategyConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit StrategyConfigDialog(QWidget* parent = nullptr);
    ~StrategyConfigDialog() override = default;

    // Get the configured strategy config
    const StrategyConfig& getConfig() const { return m_config; }
    
    // Set the strategy config to edit
    void setConfig(const StrategyConfig& config);

private:
    void setupUI();
    void setupFiltersSection(QGridLayout* gridLayout);
    void setupConfigButtonsSection(QVBoxLayout* mainLayout);
    void setupAdvancedOptionsSection(QVBoxLayout* mainLayout);
    void setupButtons(QVBoxLayout* mainLayout);
    
    void openStopLossTakeProfitDialog();
    void openTradingHoursDialog();
    void openRiskManagementDialog();
    void openMLConfigDialog();

private:
    StrategyConfig m_config;
    
    // Strategy name
    QLineEdit* m_nameEdit;
    
    // Filters widgets
    FiltersWidget* m_buyFiltersWidget;
    FiltersWidget* m_sellFiltersWidget;
    FiltersWidget* m_resaleFiltersWidget;
    FiltersWidget* m_rebuyFiltersWidget;
    
    // Configuration buttons
    QPushButton* m_stopLossTakeProfitButton;
    QPushButton* m_tradingHoursButton;
    QPushButton* m_riskManagementButton;
    QPushButton* m_mlConfigButton;
    
    // Advanced options
    QCheckBox* m_enableLoggingCheck;
};
