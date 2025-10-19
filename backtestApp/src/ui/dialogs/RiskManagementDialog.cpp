#include "ui/dialogs/RiskManagementDialog.h"

RiskManagementDialog::RiskManagementDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Configuration de la gestion du risque");
    setMinimumWidth(500);
    setupUI();
}

void RiskManagementDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* riskGroup = new QGroupBox("Gestion du risque", this);
    QFormLayout* riskLayout = new QFormLayout();
    
    // Taille basée sur le risque
    m_useRiskBasedSizingCheck = new QCheckBox("Taille basée sur le risque", this);
    riskLayout->addRow(m_useRiskBasedSizingCheck);
    
    m_riskPercentageSpin = new QDoubleSpinBox(this);
    m_riskPercentageSpin->setDecimals(2);
    m_riskPercentageSpin->setRange(0.1, 100.0);
    m_riskPercentageSpin->setValue(1.0);
    m_riskPercentageSpin->setSuffix("%");
    m_riskPercentageSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Risque par trade:", this), m_riskPercentageSpin);
    
    createDependencyGroup(m_useRiskBasedSizingCheck, {m_riskPercentageSpin});
    
    // Perte max journalière
    m_useDailyMaxLossCheck = new QCheckBox("Perte max journalière", this);
    riskLayout->addRow(m_useDailyMaxLossCheck);
    
    m_dailyMaxLossPercentageSpin = new QDoubleSpinBox(this);
    m_dailyMaxLossPercentageSpin->setDecimals(2);
    m_dailyMaxLossPercentageSpin->setRange(0.1, 100.0);
    m_dailyMaxLossPercentageSpin->setValue(2.0);
    m_dailyMaxLossPercentageSpin->setSuffix("%");
    m_dailyMaxLossPercentageSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Perte max journalière:", this), m_dailyMaxLossPercentageSpin);
    
    createDependencyGroup(m_useDailyMaxLossCheck, {m_dailyMaxLossPercentageSpin});
    
    // Profit max journalier
    m_useDailyMaxProfitCheck = new QCheckBox("Profit max journalier", this);
    riskLayout->addRow(m_useDailyMaxProfitCheck);
    
    m_dailyMaxProfitPercentageSpin = new QDoubleSpinBox(this);
    m_dailyMaxProfitPercentageSpin->setDecimals(2);
    m_dailyMaxProfitPercentageSpin->setRange(0.1, 100.0);
    m_dailyMaxProfitPercentageSpin->setValue(5.0);
    m_dailyMaxProfitPercentageSpin->setSuffix("%");
    m_dailyMaxProfitPercentageSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Profit max journalier:", this), m_dailyMaxProfitPercentageSpin);
    
    createDependencyGroup(m_useDailyMaxProfitCheck, {m_dailyMaxProfitPercentageSpin});
    
    // Drawdown max journalier
    m_useDailyMaxDrawdownCheck = new QCheckBox("Drawdown max journalier", this);
    riskLayout->addRow(m_useDailyMaxDrawdownCheck);
    
    m_dailyMaxDrawdownPercentageSpin = new QDoubleSpinBox(this);
    m_dailyMaxDrawdownPercentageSpin->setDecimals(2);
    m_dailyMaxDrawdownPercentageSpin->setRange(0.1, 100.0);
    m_dailyMaxDrawdownPercentageSpin->setValue(3.0);
    m_dailyMaxDrawdownPercentageSpin->setSuffix("%");
    m_dailyMaxDrawdownPercentageSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Drawdown max journalier:", this), m_dailyMaxDrawdownPercentageSpin);
    
    createDependencyGroup(m_useDailyMaxDrawdownCheck, {m_dailyMaxDrawdownPercentageSpin});
    
    // Break even
    m_useBreakEvenCheck = new QCheckBox("Activer Break Even", this);
    riskLayout->addRow(m_useBreakEvenCheck);
    
    m_breakEvenThresholdSpin = new QDoubleSpinBox(this);
    m_breakEvenThresholdSpin->setDecimals(2);
    m_breakEvenThresholdSpin->setRange(0.0, 10.0);
    m_breakEvenThresholdSpin->setSingleStep(0.05);
    m_breakEvenThresholdSpin->setValue(0.7);
    m_breakEvenThresholdSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Seuil Break Even:", this), m_breakEvenThresholdSpin);
    
    m_breakEvenOffsetSpin = new QDoubleSpinBox(this);
    m_breakEvenOffsetSpin->setDecimals(3);
    m_breakEvenOffsetSpin->setRange(-50.0, 50.0);
    m_breakEvenOffsetSpin->setSingleStep(0.05);
    m_breakEvenOffsetSpin->setValue(0.0);
    m_breakEvenOffsetSpin->setSuffix("‰");
    m_breakEvenOffsetSpin->setEnabled(false);
    riskLayout->addRow(new QLabel("Offset Break Even (‰):", this), m_breakEvenOffsetSpin);
    
    createDependencyGroup(m_useBreakEvenCheck, {m_breakEvenThresholdSpin, m_breakEvenOffsetSpin});
    
    riskGroup->setLayout(riskLayout);
    mainLayout->addWidget(riskGroup);
    
    // Boutons OK/Cancel
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
    m_useRiskBasedSizingCheck->setChecked(config.use_risk_based_sizing);
    m_riskPercentageSpin->setValue(config.risk_percentage);
    m_useDailyMaxLossCheck->setChecked(config.use_daily_max_loss);
    m_dailyMaxLossPercentageSpin->setValue(config.daily_max_loss_percentage);
    m_useDailyMaxProfitCheck->setChecked(config.use_daily_max_profit);
    m_dailyMaxProfitPercentageSpin->setValue(config.daily_max_profit_percentage);
    m_useDailyMaxDrawdownCheck->setChecked(config.use_daily_max_drawdown);
    m_dailyMaxDrawdownPercentageSpin->setValue(config.daily_max_drawdown_percentage);
    m_useBreakEvenCheck->setChecked(config.use_break_even);
    m_breakEvenThresholdSpin->setValue(config.break_even_threshold);
    m_breakEvenOffsetSpin->setValue(config.break_even_offset_per_mille);
}

void RiskManagementDialog::updateConfig(StrategyConfig& config) const {
    config.use_risk_based_sizing = m_useRiskBasedSizingCheck->isChecked();
    config.risk_percentage = m_riskPercentageSpin->value();
    config.use_daily_max_loss = m_useDailyMaxLossCheck->isChecked();
    config.daily_max_loss_percentage = m_dailyMaxLossPercentageSpin->value();
    config.use_daily_max_profit = m_useDailyMaxProfitCheck->isChecked();
    config.daily_max_profit_percentage = m_dailyMaxProfitPercentageSpin->value();
    config.use_daily_max_drawdown = m_useDailyMaxDrawdownCheck->isChecked();
    config.daily_max_drawdown_percentage = m_dailyMaxDrawdownPercentageSpin->value();
    config.use_break_even = m_useBreakEvenCheck->isChecked();
    config.break_even_threshold = m_breakEvenThresholdSpin->value();
    config.break_even_offset_per_mille = m_breakEvenOffsetSpin->value();
}
