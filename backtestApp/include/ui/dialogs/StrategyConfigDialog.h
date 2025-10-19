#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTimeEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include "common.h"
#include "ui/panels/ConfigPanel.h"

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
    void setupStopLossTakeProfitSection(QVBoxLayout* mainLayout);
    void setupTradingHoursSection(QVBoxLayout* mainLayout);
    void setupRiskManagementSection(QVBoxLayout* mainLayout);
    void setupAdvancedOptionsSection(QVBoxLayout* mainLayout);
    void setupButtons(QVBoxLayout* mainLayout);
    
    // Helper to create dependency groups
    void createDependencyGroup(QCheckBox* checkbox, const std::vector<QWidget*>& widgets);

private:
    StrategyConfig m_config;
    
    // Widgets for Stop Loss
    QComboBox* m_slMethodCombo;
    QLabel* m_slDistanceLabel;
    QDoubleSpinBox* m_stopLossDistanceSpin;
    QLabel* m_slAtrMultiplierLabel;
    QDoubleSpinBox* m_slAtrMultiplierSpin;
    QLabel* m_slMinmaxPeriodsLabel;
    QSpinBox* m_slMinmaxPeriodsSpin;
    QLabel* m_slMinmaxCoefLabel;
    QDoubleSpinBox* m_slMinmaxCoefAtr;
    QDoubleSpinBox* m_minStopLossDistanceSpin;
    
    // Widgets for Take Profit
    QComboBox* m_tpMethodCombo;
    QLabel* m_tpDistanceLabel;
    QDoubleSpinBox* m_takeProfitDistanceSpin;
    QLabel* m_tpAtrMultiplierLabel;
    QDoubleSpinBox* m_tpAtrMultiplierSpin;
    QLabel* m_tpSlRatioLabel;
    QDoubleSpinBox* m_tpSlRatioSpin;
    QLabel* m_rlLookbackLabel;
    QSpinBox* m_rlLookbackPeriodsSpin;
    QDoubleSpinBox* m_minTakeProfitDistanceSpin;
    
    // Common ATR period
    QLabel* m_atrLabel;
    QSpinBox* m_atrPeriodSpin;
    
    // Filters widgets
    FiltersWidget* m_buyFiltersWidget;
    FiltersWidget* m_sellFiltersWidget;
    FiltersWidget* m_resaleFiltersWidget;
    FiltersWidget* m_rebuyFiltersWidget;
    
    // Trading hours
    QTimeEdit* m_tradingFromTime;
    QTimeEdit* m_tradingToTime;
    std::vector<QCheckBox*> m_tradingDayCheckboxes;
    
    // Risk management
    QCheckBox* m_useRiskBasedSizingCheck;
    QDoubleSpinBox* m_riskPercentageSpin;
    QCheckBox* m_useDailyMaxLossCheck;
    QDoubleSpinBox* m_dailyMaxLossPercentageSpin;
    QCheckBox* m_useDailyMaxProfitCheck;
    QDoubleSpinBox* m_dailyMaxProfitPercentageSpin;
    QCheckBox* m_useDailyMaxDrawdownCheck;
    QDoubleSpinBox* m_dailyMaxDrawdownPercentageSpin;
    QCheckBox* m_useBreakEvenCheck;
    QDoubleSpinBox* m_breakEvenThresholdSpin;
    QDoubleSpinBox* m_breakEvenOffsetSpin;
    
    // Advanced options
    QCheckBox* m_enableLoggingCheck;
};
