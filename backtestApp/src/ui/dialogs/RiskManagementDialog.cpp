#include "ui/dialogs/RiskManagementDialog.h"

RiskManagementDialog::RiskManagementDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Risk Management Configuration");
    setMinimumWidth(500);
    setupUI();
}

void RiskManagementDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // ===== Capital Allocation Group =====
    QGroupBox* capitalGroup = new QGroupBox("Capital Allocation", this);
    QFormLayout* capitalLayout = new QFormLayout();
    
    // Cash allocation in percentage
    m_cashAllocationPercentageSpin = new QDoubleSpinBox(this);
    m_cashAllocationPercentageSpin->setDecimals(1);
    m_cashAllocationPercentageSpin->setRange(0.1, 100.0);
    m_cashAllocationPercentageSpin->setValue(100.0);
    m_cashAllocationPercentageSpin->setSuffix("%");
    capitalLayout->addRow(new QLabel("Cash allocation:", this), m_cashAllocationPercentageSpin);
    
    // Custom leverage
    m_useCustomLeverageCheck = new QCheckBox("Custom leverage", this);
    capitalLayout->addRow(m_useCustomLeverageCheck);
    
    m_customLeverageSpin = new QDoubleSpinBox(this);
    m_customLeverageSpin->setDecimals(1);
    m_customLeverageSpin->setRange(1.0, 500.0);
    m_customLeverageSpin->setValue(1.0);
    m_customLeverageSpin->setEnabled(false);
    capitalLayout->addRow(new QLabel("Leverage limit:", this), m_customLeverageSpin);
    
    createDependencyGroup(m_useCustomLeverageCheck, {m_customLeverageSpin});
    
    capitalGroup->setLayout(capitalLayout);
    mainLayout->addWidget(capitalGroup);
    
    // ===== Risk Management Group =====
    QGroupBox* riskGroup = new QGroupBox("Risk Management", this);
    QFormLayout* riskLayout = new QFormLayout();
    
    // Risk-based sizing
    m_useRiskBasedSizingCheck = new QCheckBox("Risk-based sizing", this);
    riskLayout->addRow(m_useRiskBasedSizingCheck);
    
    m_riskPercentageSpin = new QDoubleSpinBox(this);
    m_riskPercentageSpin->setDecimals(2);
    m_riskPercentageSpin->setRange(0.1, 100.0);
    m_riskPercentageSpin->setValue(1.0);
    m_riskPercentageSpin->setSuffix("%");
    m_riskPercentageSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Risk per trade:", this), m_riskPercentageSpin);
    
    createDependencyGroup(m_useRiskBasedSizingCheck, {m_riskPercentageSpin});
    
    // Daily max loss
    m_useDailyMaxLossCheck = new QCheckBox("Daily max loss", this);
    riskLayout->addRow(m_useDailyMaxLossCheck);
    
    m_dailyMaxLossPercentageSpin = new QDoubleSpinBox(this);
    m_dailyMaxLossPercentageSpin->setDecimals(2);
    m_dailyMaxLossPercentageSpin->setRange(0.1, 100.0);
    m_dailyMaxLossPercentageSpin->setValue(2.0);
    m_dailyMaxLossPercentageSpin->setSuffix("%");
    m_dailyMaxLossPercentageSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Daily max loss:", this), m_dailyMaxLossPercentageSpin);
    
    createDependencyGroup(m_useDailyMaxLossCheck, {m_dailyMaxLossPercentageSpin});
    
    // Daily max profit
    m_useDailyMaxProfitCheck = new QCheckBox("Daily max profit", this);
    riskLayout->addRow(m_useDailyMaxProfitCheck);
    
    m_dailyMaxProfitPercentageSpin = new QDoubleSpinBox(this);
    m_dailyMaxProfitPercentageSpin->setDecimals(2);
    m_dailyMaxProfitPercentageSpin->setRange(0.1, 100.0);
    m_dailyMaxProfitPercentageSpin->setValue(5.0);
    m_dailyMaxProfitPercentageSpin->setSuffix("%");
    m_dailyMaxProfitPercentageSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Daily max profit:", this), m_dailyMaxProfitPercentageSpin);
    
    createDependencyGroup(m_useDailyMaxProfitCheck, {m_dailyMaxProfitPercentageSpin});
    
    // Daily max drawdown
    m_useDailyMaxDrawdownCheck = new QCheckBox("Daily max drawdown", this);
    riskLayout->addRow(m_useDailyMaxDrawdownCheck);
    
    m_dailyMaxDrawdownPercentageSpin = new QDoubleSpinBox(this);
    m_dailyMaxDrawdownPercentageSpin->setDecimals(2);
    m_dailyMaxDrawdownPercentageSpin->setRange(0.1, 100.0);
    m_dailyMaxDrawdownPercentageSpin->setValue(3.0);
    m_dailyMaxDrawdownPercentageSpin->setSuffix("%");
    m_dailyMaxDrawdownPercentageSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Daily max drawdown:", this), m_dailyMaxDrawdownPercentageSpin);
    
    createDependencyGroup(m_useDailyMaxDrawdownCheck, {m_dailyMaxDrawdownPercentageSpin});
    
    // Break even
    m_useBreakEvenCheck = new QCheckBox("Enable Break Even", this);
    riskLayout->addRow(m_useBreakEvenCheck);
    
    m_breakEvenThresholdSpin = new QDoubleSpinBox(this);
    m_breakEvenThresholdSpin->setDecimals(2);
    m_breakEvenThresholdSpin->setRange(0.0, 10.0);
    m_breakEvenThresholdSpin->setSingleStep(0.05);
    m_breakEvenThresholdSpin->setValue(0.7);
    m_breakEvenThresholdSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Break Even threshold:", this), m_breakEvenThresholdSpin);
    
    m_breakEvenOffsetSpin = new QDoubleSpinBox(this);
    m_breakEvenOffsetSpin->setDecimals(3);
    m_breakEvenOffsetSpin->setRange(-50.0, 50.0);
    m_breakEvenOffsetSpin->setSingleStep(0.05);
    m_breakEvenOffsetSpin->setValue(0.0);
    m_breakEvenOffsetSpin->setSuffix("‰");
    m_breakEvenOffsetSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Break Even Offset (‰):", this), m_breakEvenOffsetSpin);
    
    createDependencyGroup(m_useBreakEvenCheck, {m_breakEvenThresholdSpin, m_breakEvenOffsetSpin});
    
    riskGroup->setLayout(riskLayout);
    mainLayout->addWidget(riskGroup);
    
    // OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void RiskManagementDialog::createDependencyGroup(QCheckBox* checkbox, const std::vector<QWidget*>& widgets) {
    auto updateWidgets = [checkbox, widgets](Qt::CheckState state) {
        bool enabled = (state == Qt::Checked);
        for (QWidget* widget : widgets) {
            widget->setEnabled(enabled);
            if (enabled) {
                widget->setStyleSheet("");
            } else {
                widget->setStyleSheet("QDoubleSpinBox, QSpinBox { background-color: #f0f0f0; color: #888888; }");
            }
        }
    };
    
    connect(checkbox, &QCheckBox::checkStateChanged, updateWidgets);
    updateWidgets(checkbox->checkState());
}

void RiskManagementDialog::setConfig(const StrategyConfig& config) {
    // Capital allocation - only set if value is valid (> 0)
    if (config.cash_allocation_percentage > 0) 
        m_cashAllocationPercentageSpin->setValue(config.cash_allocation_percentage);
    
    // Custom leverage
    if (config.leverage_limit > 0) {
        m_useCustomLeverageCheck->setChecked(true);
        m_customLeverageSpin->setValue(config.leverage_limit);
    } else {
        m_useCustomLeverageCheck->setChecked(false);
    }
    
    // Risk-based sizing
    m_useRiskBasedSizingCheck->setChecked(config.use_risk_based_sizing);
    if (config.risk_percentage > 0) 
        m_riskPercentageSpin->setValue(config.risk_percentage);
    
    // Daily max loss
    m_useDailyMaxLossCheck->setChecked(config.use_daily_max_loss);
    if (config.daily_max_loss_percentage > 0) 
        m_dailyMaxLossPercentageSpin->setValue(config.daily_max_loss_percentage);
    
    // Daily max profit
    m_useDailyMaxProfitCheck->setChecked(config.use_daily_max_profit);
    if (config.daily_max_profit_percentage > 0) 
        m_dailyMaxProfitPercentageSpin->setValue(config.daily_max_profit_percentage);
    
    // Daily max drawdown
    m_useDailyMaxDrawdownCheck->setChecked(config.use_daily_max_drawdown);
    if (config.daily_max_drawdown_percentage > 0) 
        m_dailyMaxDrawdownPercentageSpin->setValue(config.daily_max_drawdown_percentage);
    
    // Break even
    m_useBreakEvenCheck->setChecked(config.use_break_even);
    if (config.break_even_threshold > 0) 
        m_breakEvenThresholdSpin->setValue(config.break_even_threshold);
    // break_even_offset_per_mille can be 0, so always set it
    m_breakEvenOffsetSpin->setValue(config.break_even_offset_per_mille);
}

void RiskManagementDialog::updateConfig(StrategyConfig& config) const {
    // Capital allocation
    config.cash_allocation_percentage = m_cashAllocationPercentageSpin->value();
    
    // Custom leverage - only update if checkbox is checked
    if (m_useCustomLeverageCheck->isChecked()) 
        config.leverage_limit = m_customLeverageSpin->value();
    else 
        config.leverage_limit = 0; // Indicate that leverage is not set
    
    // Risk-based sizing
    config.use_risk_based_sizing = m_useRiskBasedSizingCheck->isChecked();
    config.risk_percentage = m_riskPercentageSpin->value();
    
    // Daily max loss
    config.use_daily_max_loss = m_useDailyMaxLossCheck->isChecked();
    config.daily_max_loss_percentage = m_dailyMaxLossPercentageSpin->value();
    
    // Daily max profit
    config.use_daily_max_profit = m_useDailyMaxProfitCheck->isChecked();
    config.daily_max_profit_percentage = m_dailyMaxProfitPercentageSpin->value();
    
    // Daily max drawdown
    config.use_daily_max_drawdown = m_useDailyMaxDrawdownCheck->isChecked();
    config.daily_max_drawdown_percentage = m_dailyMaxDrawdownPercentageSpin->value();
    
    // Break even
    config.use_break_even = m_useBreakEvenCheck->isChecked();
    config.break_even_threshold = m_breakEvenThresholdSpin->value();
    config.break_even_offset_per_mille = m_breakEvenOffsetSpin->value();
}
