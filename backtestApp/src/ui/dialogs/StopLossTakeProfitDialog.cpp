#include "ui/dialogs/StopLossTakeProfitDialog.h"

StopLossTakeProfitDialog::StopLossTakeProfitDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Stop Loss / Take Profit Configuration");
    setMinimumWidth(600);
    setupUI();
}

void StopLossTakeProfitDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Common ATR period (top)
    QFormLayout* atrLayout = new QFormLayout();
    m_atrPeriodSpin = new QSpinBox(this);
    m_atrPeriodSpin->setRange(1, 1000);
    m_atrPeriodSpin->setValue(14);
    m_atrPeriodSpin->setVisible(false);
    m_atrLabel = new QLabel("ATR Period:", this);
    m_atrLabel->setVisible(false);
    atrLayout->addRow(m_atrLabel, m_atrPeriodSpin);
    mainLayout->addLayout(atrLayout);
    
    // Stop Loss group
    QGroupBox* slGroup = new QGroupBox("Stop Loss", this);
    QFormLayout* slLayout = new QFormLayout();
    
    // Calculation method for Stop Loss
    m_slMethodCombo = new QComboBox(this);
    m_slMethodCombo->addItems({"None", "Fixed", "ATR", "Min/Max"});
    m_slMethodCombo->setCurrentIndex(1);
    slLayout->addRow(new QLabel("Method:", this), m_slMethodCombo);
    
    // Stop Loss Distance (fixed)
    m_stopLossDistanceSpin = new QDoubleSpinBox(this);
    m_stopLossDistanceSpin->setDecimals(4);
    m_stopLossDistanceSpin->setRange(0, 10000);
    m_stopLossDistanceSpin->setSingleStep(1);
    m_stopLossDistanceSpin->setValue(20);
    m_slDistanceLabel = new QLabel("Distance [pts]:", this);
    slLayout->addRow(m_slDistanceLabel, m_stopLossDistanceSpin);
    
    // ATR parameters - SL multiplier
    m_slAtrMultiplierSpin = new QDoubleSpinBox(this);
    m_slAtrMultiplierSpin->setDecimals(2);
    m_slAtrMultiplierSpin->setRange(0.1, 1000.0);
    m_slAtrMultiplierSpin->setSingleStep(0.1);
    m_slAtrMultiplierSpin->setValue(2.0);
    m_slAtrMultiplierSpin->setVisible(false);
    m_slAtrMultiplierLabel = new QLabel("ATR Multiplier SL:", this);
    m_slAtrMultiplierLabel->setVisible(false);
    slLayout->addRow(m_slAtrMultiplierLabel, m_slAtrMultiplierSpin);
    
    // Min/Max SL parameters
    m_slMinmaxCoefAtr = new QDoubleSpinBox(this);
    m_slMinmaxCoefAtr->setDecimals(2);
    m_slMinmaxCoefAtr->setRange(0, 1000.0);
    m_slMinmaxCoefAtr->setValue(5.0);
    m_slMinmaxCoefAtr->setVisible(false);
    m_slMinmaxCoefLabel = new QLabel("Min/Max Delta Coefficient:", this);
    m_slMinmaxCoefLabel->setVisible(false);
    slLayout->addRow(m_slMinmaxCoefLabel, m_slMinmaxCoefAtr);
    
    m_slMinmaxPeriodsSpin = new QSpinBox(this);
    m_slMinmaxPeriodsSpin->setRange(1, 1000);
    m_slMinmaxPeriodsSpin->setValue(5);
    m_slMinmaxPeriodsSpin->setVisible(false);
    m_slMinmaxPeriodsLabel = new QLabel("Min/Max Periods:", this);
    m_slMinmaxPeriodsLabel->setVisible(false);
    slLayout->addRow(m_slMinmaxPeriodsLabel, m_slMinmaxPeriodsSpin);
    
    // Minimum SL
    m_minStopLossDistanceSpin = new QDoubleSpinBox(this);
    m_minStopLossDistanceSpin->setDecimals(4);
    m_minStopLossDistanceSpin->setRange(0, 1000.0);
    m_minStopLossDistanceSpin->setValue(5.0);
    slLayout->addRow(new QLabel("Minimum SL [pts]:", this), m_minStopLossDistanceSpin);
    
    slGroup->setLayout(slLayout);
    mainLayout->addWidget(slGroup);
    
    // Take Profit group
    QGroupBox* tpGroup = new QGroupBox("Take Profit", this);
    QFormLayout* tpLayout = new QFormLayout();
    
    // Calculation method for Take Profit
    m_tpMethodCombo = new QComboBox(this);
    m_tpMethodCombo->addItems({"None", "Fixed", "ATR", "SL Ratio", "SuperTrend", "RL", "Nth Heikin-Ashi"});
    m_tpMethodCombo->setCurrentIndex(1);
    tpLayout->addRow(new QLabel("Method:", this), m_tpMethodCombo);
    
    // Take Profit Distance (fixed)
    m_takeProfitDistanceSpin = new QDoubleSpinBox(this);
    m_takeProfitDistanceSpin->setDecimals(4);
    m_takeProfitDistanceSpin->setRange(0, 10000);
    m_takeProfitDistanceSpin->setSingleStep(1);
    m_takeProfitDistanceSpin->setValue(30);
    m_tpDistanceLabel = new QLabel("Distance [pts]:", this);
    tpLayout->addRow(m_tpDistanceLabel, m_takeProfitDistanceSpin);
    
    // ATR parameters - TP multiplier
    m_tpAtrMultiplierSpin = new QDoubleSpinBox(this);
    m_tpAtrMultiplierSpin->setDecimals(2);
    m_tpAtrMultiplierSpin->setRange(0, 1000.0);
    m_tpAtrMultiplierSpin->setSingleStep(0.1);
    m_tpAtrMultiplierSpin->setValue(3.0);
    m_tpAtrMultiplierSpin->setVisible(false);
    m_tpAtrMultiplierLabel = new QLabel("ATR Multiplier TP:", this);
    m_tpAtrMultiplierLabel->setVisible(false);
    tpLayout->addRow(m_tpAtrMultiplierLabel, m_tpAtrMultiplierSpin);
    
    // SL Ratio parameters - TP multiplier
    m_tpSlRatioSpin = new QDoubleSpinBox(this);
    m_tpSlRatioSpin->setDecimals(2);
    m_tpSlRatioSpin->setRange(0.1, 100.0);
    m_tpSlRatioSpin->setSingleStep(0.1);
    m_tpSlRatioSpin->setValue(2.0);
    m_tpSlRatioSpin->setVisible(false);
    m_tpSlRatioLabel = new QLabel("TP/SL Ratio:", this);
    m_tpSlRatioLabel->setVisible(false);
    tpLayout->addRow(m_tpSlRatioLabel, m_tpSlRatioSpin);
    
    // RL parameters - lookback periods
    m_rlLookbackPeriodsSpin = new QSpinBox(this);
    m_rlLookbackPeriodsSpin->setRange(1, 150);
    m_rlLookbackPeriodsSpin->setVisible(false);
    m_rlLookbackLabel = new QLabel("RL Lookback Periods:", this);
    m_rlLookbackLabel->setVisible(false);
    tpLayout->addRow(m_rlLookbackLabel, m_rlLookbackPeriodsSpin);
    
    // Minimum TP
    m_minTakeProfitDistanceSpin = new QDoubleSpinBox(this);
    m_minTakeProfitDistanceSpin->setDecimals(1);
    m_minTakeProfitDistanceSpin->setRange(0, 1000.0);
    m_minTakeProfitDistanceSpin->setValue(5.0);
    tpLayout->addRow(new QLabel("Minimum TP [pts]:", this), m_minTakeProfitDistanceSpin);
    
    tpGroup->setLayout(tpLayout);
    mainLayout->addWidget(tpGroup);
    
    // OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    
    // Connect signals for dynamic visibility
    auto updateSlMethodVisibility = [this](int index) {
        bool isFixed = (index == 1);
        bool isAtr = (index == 2);
        bool isMinMax = (index == 3);

        m_slDistanceLabel->setVisible(isFixed);
        m_stopLossDistanceSpin->setVisible(isFixed);
        m_slAtrMultiplierLabel->setVisible(isAtr);
        m_slAtrMultiplierSpin->setVisible(isAtr);
        m_slMinmaxPeriodsLabel->setVisible(isMinMax);
        m_slMinmaxPeriodsSpin->setVisible(isMinMax);
        m_slMinmaxCoefLabel->setVisible(isMinMax);
        m_slMinmaxCoefAtr->setVisible(isMinMax);

        bool atrNeeded = isAtr || isMinMax || (m_tpMethodCombo->currentIndex() == 2);
        m_atrLabel->setVisible(atrNeeded);
        m_atrPeriodSpin->setVisible(atrNeeded);
    };

    connect(m_slMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateSlMethodVisibility);
    updateSlMethodVisibility(m_slMethodCombo->currentIndex());
    
    auto updateTpMethodVisibility = [this](int index) {
        bool isFixed = (index == 1);
        bool isAtr = (index == 2);
        bool isRatio = (index == 3);
        bool isRL = (index == 5);
        
        m_tpDistanceLabel->setVisible(isFixed);
        m_takeProfitDistanceSpin->setVisible(isFixed);
        m_tpAtrMultiplierLabel->setVisible(isAtr);
        m_tpAtrMultiplierSpin->setVisible(isAtr);
        m_tpSlRatioLabel->setVisible(isRatio);
        m_tpSlRatioSpin->setVisible(isRatio);
        m_rlLookbackLabel->setVisible(isRL);
        m_rlLookbackPeriodsSpin->setVisible(isRL);

        bool atrNeeded = isAtr || (m_slMethodCombo->currentIndex() == 2) || (m_slMethodCombo->currentIndex() == 3);
        m_atrLabel->setVisible(atrNeeded);
        m_atrPeriodSpin->setVisible(atrNeeded);
    };

    connect(m_tpMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateTpMethodVisibility);
    updateTpMethodVisibility(m_tpMethodCombo->currentIndex());
}

void StopLossTakeProfitDialog::setConfig(const StrategyConfig& config) {
    // Stop Loss
    m_slMethodCombo->setCurrentIndex(static_cast<int>(config.sl_method) + 1);
    m_stopLossDistanceSpin->setValue(config.stop_loss_distance);
    m_slAtrMultiplierSpin->setValue(config.stop_loss_atr_multiplier);
    m_slMinmaxPeriodsSpin->setValue(config.sl_minmax_periods);
    m_slMinmaxCoefAtr->setValue(config.sl_minmax_delta_coef_atr);
    m_minStopLossDistanceSpin->setValue(config.min_stop_loss_distance);
    
    // Take Profit
    m_tpMethodCombo->setCurrentIndex(static_cast<int>(config.tp_method) + 1);
    m_takeProfitDistanceSpin->setValue(config.take_profit_distance);
    m_tpAtrMultiplierSpin->setValue(config.take_profit_atr_multiplier);
    m_tpSlRatioSpin->setValue(config.tp_sl_ratio);
    m_rlLookbackPeriodsSpin->setValue(config.rl_lookback_periods);
    m_minTakeProfitDistanceSpin->setValue(config.min_take_profit_distance);
    
    // ATR
    m_atrPeriodSpin->setValue(config.atr_period);
}

void StopLossTakeProfitDialog::updateConfig(StrategyConfig& config) const {
    // Stop Loss
    config.sl_method = static_cast<StopLossMethod>(m_slMethodCombo->currentIndex() - 1);
    config.stop_loss_distance = m_stopLossDistanceSpin->value();
    config.stop_loss_atr_multiplier = m_slAtrMultiplierSpin->value();
    config.sl_minmax_periods = m_slMinmaxPeriodsSpin->value();
    config.sl_minmax_delta_coef_atr = m_slMinmaxCoefAtr->value();
    config.min_stop_loss_distance = m_minStopLossDistanceSpin->value();
    
    // Take Profit
    config.tp_method = static_cast<TakeProfitMethod>(m_tpMethodCombo->currentIndex() - 1);
    config.take_profit_distance = m_takeProfitDistanceSpin->value();
    config.take_profit_atr_multiplier = m_tpAtrMultiplierSpin->value();
    config.tp_sl_ratio = m_tpSlRatioSpin->value();
    config.rl_lookback_periods = m_rlLookbackPeriodsSpin->value();
    config.min_take_profit_distance = m_minTakeProfitDistanceSpin->value();
    
    // ATR
    config.atr_period = m_atrPeriodSpin->value();
}
