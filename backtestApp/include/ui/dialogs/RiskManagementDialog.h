#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <vector>
#include "common.h"

class RiskManagementDialog : public QDialog {
    Q_OBJECT

public:
    explicit RiskManagementDialog(QWidget* parent = nullptr);
    ~RiskManagementDialog() override = default;

    void setConfig(const StrategyConfig& config);
    void updateConfig(StrategyConfig& config) const;

private:
    void setupUI();
    void createDependencyGroup(QCheckBox* checkbox, const std::vector<QWidget*>& widgets);
    
    // Capital allocation
    QDoubleSpinBox* m_cashAllocationPercentageSpin;
    QCheckBox* m_useCustomLeverageCheck;
    QLabel* m_customLeverageLabel;
    QDoubleSpinBox* m_customLeverageSpin;
    
    // Risk-based sizing
    QCheckBox* m_useRiskBasedSizingCheck;
    QLabel* m_riskPercentageLabel;
    QDoubleSpinBox* m_riskPercentageSpin;
    
    // Daily limits
    QCheckBox* m_useDailyMaxLossCheck;
    QLabel* m_dailyMaxLossLabel;
    QDoubleSpinBox* m_dailyMaxLossPercentageSpin;
    QCheckBox* m_useDailyMaxProfitCheck;
    QLabel* m_dailyMaxProfitLabel;
    QDoubleSpinBox* m_dailyMaxProfitPercentageSpin;
    QCheckBox* m_useDailyMaxDrawdownCheck;
    QLabel* m_dailyMaxDrawdownLabel;
    QDoubleSpinBox* m_dailyMaxDrawdownPercentageSpin;
    
    // Break even
    QCheckBox* m_useBreakEvenCheck;
    QLabel* m_breakEvenThresholdLabel;
    QDoubleSpinBox* m_breakEvenThresholdSpin;
    QLabel* m_breakEvenOffsetLabel;
    QDoubleSpinBox* m_breakEvenOffsetSpin;
};
