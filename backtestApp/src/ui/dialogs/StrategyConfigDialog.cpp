#include "ui/dialogs/StrategyConfigDialog.h"
#include "ui/panels/FiltersWidget.h"
#include <QScrollArea>
#include <QDebug>

StrategyConfigDialog::StrategyConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    // Initialiser le tableau des jours de trading (Lun-Ven activés par défaut)
    for (int i = 0; i < 7; ++i) {
        m_config.trading_days_array[i] = (i < 5);
    }
    
    setWindowTitle("Configuration de la stratégie");
    setMinimumSize(1200, 800);
    
    setupUI();
}

void StrategyConfigDialog::setConfig(const StrategyConfig& config) {
    m_config = config;
    
    // Update all widgets with the new config
    // Stop Loss
    m_slMethodCombo->setCurrentIndex(static_cast<int>(m_config.sl_method));
    m_stopLossDistanceSpin->setValue(m_config.stop_loss_distance);
    m_slAtrMultiplierSpin->setValue(m_config.stop_loss_atr_multiplier);
    m_slMinmaxPeriodsSpin->setValue(m_config.sl_minmax_periods);
    m_slMinmaxCoefAtr->setValue(m_config.sl_minmax_delta_coef_atr);
    m_minStopLossDistanceSpin->setValue(m_config.min_stop_loss_distance);
    
    // Take Profit
    m_tpMethodCombo->setCurrentIndex(static_cast<int>(m_config.tp_method));
    m_takeProfitDistanceSpin->setValue(m_config.take_profit_distance);
    m_tpAtrMultiplierSpin->setValue(m_config.take_profit_atr_multiplier);
    m_tpSlRatioSpin->setValue(m_config.tp_sl_ratio);
    m_rlLookbackPeriodsSpin->setValue(m_config.rl_lookback_periods);
    m_minTakeProfitDistanceSpin->setValue(m_config.min_take_profit_distance);
    
    // ATR
    m_atrPeriodSpin->setValue(m_config.atr_period);
    
    // Filters
    m_buyFiltersWidget->setFilters(m_config.buyFilters);
    m_sellFiltersWidget->setFilters(m_config.sellFilters);
    m_resaleFiltersWidget->setFilters(m_config.resaleFilters);
    m_rebuyFiltersWidget->setFilters(m_config.rebuyFilters);
    
    // Trading hours
    m_tradingFromTime->setTime(QTime(m_config.trading_from.hour, m_config.trading_from.minute));
    m_tradingToTime->setTime(QTime(m_config.trading_to.hour, m_config.trading_to.minute));
    for (int i = 0; i < 7; ++i) {
        m_tradingDayCheckboxes[i]->setChecked(m_config.trading_days_array[i]);
    }
    
    // Risk management
    m_useRiskBasedSizingCheck->setChecked(m_config.use_risk_based_sizing);
    m_riskPercentageSpin->setValue(m_config.risk_percentage);
    m_useDailyMaxLossCheck->setChecked(m_config.use_daily_max_loss);
    m_dailyMaxLossPercentageSpin->setValue(m_config.daily_max_loss_percentage);
    m_useDailyMaxProfitCheck->setChecked(m_config.use_daily_max_profit);
    m_dailyMaxProfitPercentageSpin->setValue(m_config.daily_max_profit_percentage);
    m_useDailyMaxDrawdownCheck->setChecked(m_config.use_daily_max_drawdown);
    m_dailyMaxDrawdownPercentageSpin->setValue(m_config.daily_max_drawdown_percentage);
    m_useBreakEvenCheck->setChecked(m_config.use_break_even);
    m_breakEvenThresholdSpin->setValue(m_config.break_even_threshold);
    m_breakEvenOffsetSpin->setValue(m_config.break_even_offset_per_mille);
    
    // Advanced
    m_enableLoggingCheck->setChecked(m_config.enable_logging);
}

void StrategyConfigDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create scroll area for the content
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* scrollContent = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(scrollContent);
    
    // Setup sections
    setupAdvancedOptionsSection(contentLayout);
    
    // Filters section at the top (2x2 grid)
    QGroupBox* filtersGroup = new QGroupBox("Configuration des filtres", scrollContent);
    QGridLayout* filtersLayout = new QGridLayout();
    setupFiltersSection(filtersLayout);
    filtersGroup->setLayout(filtersLayout);
    contentLayout->addWidget(filtersGroup);
    
    // Configuration sections below
    setupStopLossTakeProfitSection(contentLayout);
    setupTradingHoursSection(contentLayout);
    setupRiskManagementSection(contentLayout);
    
    contentLayout->addStretch();
    
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);
    
    // Buttons at the bottom (outside scroll area)
    setupButtons(mainLayout);
}

void StrategyConfigDialog::setupFiltersSection(QGridLayout* gridLayout) {
    // Create the 4 filter widgets in a 2x2 grid
    m_buyFiltersWidget = new FiltersWidget(this, "Filtres d'achat");
    m_sellFiltersWidget = new FiltersWidget(this, "Filtres de vente");
    m_resaleFiltersWidget = new FiltersWidget(this, "Filtres de revente");
    m_rebuyFiltersWidget = new FiltersWidget(this, "Filtres de rachat");
    
    gridLayout->addWidget(m_buyFiltersWidget, 0, 0);
    gridLayout->addWidget(m_sellFiltersWidget, 0, 1);
    gridLayout->addWidget(m_resaleFiltersWidget, 1, 0);
    gridLayout->addWidget(m_rebuyFiltersWidget, 1, 1);
    
    // Make columns equal width
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);
}

void StrategyConfigDialog::setupStopLossTakeProfitSection(QVBoxLayout* mainLayout) {
    QGroupBox* slTpGroup = new QGroupBox("Stop Loss et Take Profit", this);
    QVBoxLayout* slTpLayout = new QVBoxLayout();
    
    // Période ATR commune (en haut du groupe)
    QFormLayout* atrLayout = new QFormLayout();
    m_atrPeriodSpin = new QSpinBox(this);
    m_atrPeriodSpin->setRange(1, 1000);
    m_atrPeriodSpin->setValue(14);
    m_atrPeriodSpin->setVisible(false);
    m_atrLabel = new QLabel("Période ATR:", this);
    m_atrLabel->setVisible(false);
    atrLayout->addRow(m_atrLabel, m_atrPeriodSpin);
    slTpLayout->addLayout(atrLayout);
    
    // Groupe Stop Loss
    QGroupBox* slGroup = new QGroupBox("Stop Loss", this);
    QFormLayout* slLayout = new QFormLayout();
    
    // Méthode de calcul pour le Stop Loss
    m_slMethodCombo = new QComboBox(this);
    m_slMethodCombo->addItems({"Fixe", "ATR", "Min/Max"});
    m_slMethodCombo->setCurrentIndex(0);
    slLayout->addRow(new QLabel("Méthode:", this), m_slMethodCombo);
    
    // Stop Loss Distance (fixe)
    m_stopLossDistanceSpin = new QDoubleSpinBox(this);
    m_stopLossDistanceSpin->setDecimals(4);
    m_stopLossDistanceSpin->setRange(0, 10000);
    m_stopLossDistanceSpin->setSingleStep(1);
    m_stopLossDistanceSpin->setValue(20);
    m_slDistanceLabel = new QLabel("Distance [pts]:", this);
    slLayout->addRow(m_slDistanceLabel, m_stopLossDistanceSpin);
    
    // Paramètres ATR - Multiplicateur SL
    m_slAtrMultiplierSpin = new QDoubleSpinBox(this);
    m_slAtrMultiplierSpin->setDecimals(2);
    m_slAtrMultiplierSpin->setRange(0.1, 1000.0);
    m_slAtrMultiplierSpin->setSingleStep(0.1);
    m_slAtrMultiplierSpin->setValue(2.0);
    m_slAtrMultiplierSpin->setVisible(false);
    m_slAtrMultiplierLabel = new QLabel("Multiplicateur ATR SL:", this);
    m_slAtrMultiplierLabel->setVisible(false);
    slLayout->addRow(m_slAtrMultiplierLabel, m_slAtrMultiplierSpin);
    
    // Paramètres Min/Max SL
    m_slMinmaxCoefAtr = new QDoubleSpinBox(this);
    m_slMinmaxCoefAtr->setDecimals(2);
    m_slMinmaxCoefAtr->setRange(0, 1000.0);
    m_slMinmaxCoefAtr->setValue(5.0);
    m_slMinmaxCoefAtr->setVisible(false);
    m_slMinmaxCoefLabel = new QLabel("Coefficient Delta Min/Max:", this);
    m_slMinmaxCoefLabel->setVisible(false);
    slLayout->addRow(m_slMinmaxCoefLabel, m_slMinmaxCoefAtr);
    
    m_slMinmaxPeriodsSpin = new QSpinBox(this);
    m_slMinmaxPeriodsSpin->setRange(1, 1000);
    m_slMinmaxPeriodsSpin->setValue(5);
    m_slMinmaxPeriodsSpin->setVisible(false);
    m_slMinmaxPeriodsLabel = new QLabel("Périodes Min/Max:", this);
    m_slMinmaxPeriodsLabel->setVisible(false);
    slLayout->addRow(m_slMinmaxPeriodsLabel, m_slMinmaxPeriodsSpin);
    
    // SL minimum
    m_minStopLossDistanceSpin = new QDoubleSpinBox(this);
    m_minStopLossDistanceSpin->setDecimals(4);
    m_minStopLossDistanceSpin->setRange(0, 1000.0);
    m_minStopLossDistanceSpin->setValue(5.0);
    slLayout->addRow(new QLabel("SL Minimum [pts]:", this), m_minStopLossDistanceSpin);
    
    slGroup->setLayout(slLayout);
    slTpLayout->addWidget(slGroup);
    
    // Groupe Take Profit
    QGroupBox* tpGroup = new QGroupBox("Take Profit", this);
    QFormLayout* tpLayout = new QFormLayout();
    
    // Méthode de calcul pour le Take Profit
    m_tpMethodCombo = new QComboBox(this);
    m_tpMethodCombo->addItems({"Fixe", "ATR", "Ratio SL", "SuperTrend", "RL", "Nth Heikin-Ashi"});
    m_tpMethodCombo->setCurrentIndex(0);
    tpLayout->addRow(new QLabel("Méthode:", this), m_tpMethodCombo);
    
    // Take Profit Distance (fixe)
    m_takeProfitDistanceSpin = new QDoubleSpinBox(this);
    m_takeProfitDistanceSpin->setDecimals(4);
    m_takeProfitDistanceSpin->setRange(0, 10000);
    m_takeProfitDistanceSpin->setSingleStep(1);
    m_takeProfitDistanceSpin->setValue(30);
    m_tpDistanceLabel = new QLabel("Distance [pts]:", this);
    tpLayout->addRow(m_tpDistanceLabel, m_takeProfitDistanceSpin);
    
    // Paramètres ATR - Multiplicateur TP
    m_tpAtrMultiplierSpin = new QDoubleSpinBox(this);
    m_tpAtrMultiplierSpin->setDecimals(2);
    m_tpAtrMultiplierSpin->setRange(0, 1000.0);
    m_tpAtrMultiplierSpin->setSingleStep(0.1);
    m_tpAtrMultiplierSpin->setValue(3.0);
    m_tpAtrMultiplierSpin->setVisible(false);
    m_tpAtrMultiplierLabel = new QLabel("Multiplicateur ATR TP:", this);
    m_tpAtrMultiplierLabel->setVisible(false);
    tpLayout->addRow(m_tpAtrMultiplierLabel, m_tpAtrMultiplierSpin);
    
    // Paramètres Ratio SL - Multiplicateur TP
    m_tpSlRatioSpin = new QDoubleSpinBox(this);
    m_tpSlRatioSpin->setDecimals(2);
    m_tpSlRatioSpin->setRange(0.1, 100.0);
    m_tpSlRatioSpin->setSingleStep(0.1);
    m_tpSlRatioSpin->setValue(2.0);
    m_tpSlRatioSpin->setVisible(false);
    m_tpSlRatioLabel = new QLabel("Ratio TP/SL:", this);
    m_tpSlRatioLabel->setVisible(false);
    tpLayout->addRow(m_tpSlRatioLabel, m_tpSlRatioSpin);
    
    // Paramètres RL - Périodes de lookback
    m_rlLookbackPeriodsSpin = new QSpinBox(this);
    m_rlLookbackPeriodsSpin->setRange(1, 150);
    m_rlLookbackPeriodsSpin->setVisible(false);
    m_rlLookbackLabel = new QLabel("Périodes lookback RL:", this);
    m_rlLookbackLabel->setVisible(false);
    tpLayout->addRow(m_rlLookbackLabel, m_rlLookbackPeriodsSpin);
    
    // TP minimum
    m_minTakeProfitDistanceSpin = new QDoubleSpinBox(this);
    m_minTakeProfitDistanceSpin->setDecimals(1);
    m_minTakeProfitDistanceSpin->setRange(0, 1000.0);
    m_minTakeProfitDistanceSpin->setValue(5.0);
    tpLayout->addRow(new QLabel("TP Minimum [pts]:", this), m_minTakeProfitDistanceSpin);
    
    tpGroup->setLayout(tpLayout);
    slTpLayout->addWidget(tpGroup);
    
    slTpGroup->setLayout(slTpLayout);
    mainLayout->addWidget(slTpGroup);
    
    // Connecter les signaux pour la visibilité dynamique
    auto updateSlMethodVisibility = [this](int index) {
        bool isFixed = (index == 0);
        bool isAtr = (index == 1);
        bool isMinMax = (index == 2);

        m_slDistanceLabel->setVisible(isFixed);
        m_stopLossDistanceSpin->setVisible(isFixed);
        m_slAtrMultiplierLabel->setVisible(isAtr);
        m_slAtrMultiplierSpin->setVisible(isAtr);
        m_slMinmaxPeriodsLabel->setVisible(isMinMax);
        m_slMinmaxPeriodsSpin->setVisible(isMinMax);
        m_slMinmaxCoefLabel->setVisible(isMinMax);
        m_slMinmaxCoefAtr->setVisible(isMinMax);

        bool atrNeeded = isAtr || isMinMax || (m_tpMethodCombo->currentIndex() == 1);
        m_atrLabel->setVisible(atrNeeded);
        m_atrPeriodSpin->setVisible(atrNeeded);
    };

    connect(m_slMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateSlMethodVisibility);
    updateSlMethodVisibility(m_slMethodCombo->currentIndex());
    
    auto updateTpMethodVisibility = [this](int index) {
        bool isFixed = (index == 0);
        bool isAtr = (index == 1);
        bool isRatio = (index == 2);
        bool isRL = (index == 4);
        
        m_tpDistanceLabel->setVisible(isFixed);
        m_takeProfitDistanceSpin->setVisible(isFixed);
        m_tpAtrMultiplierLabel->setVisible(isAtr);
        m_tpAtrMultiplierSpin->setVisible(isAtr);
        m_tpSlRatioLabel->setVisible(isRatio);
        m_tpSlRatioSpin->setVisible(isRatio);
        m_rlLookbackLabel->setVisible(isRL);
        m_rlLookbackPeriodsSpin->setVisible(isRL);

        bool atrNeeded = isAtr || (m_slMethodCombo->currentIndex() == 1) || (m_slMethodCombo->currentIndex() == 2);
        m_atrLabel->setVisible(atrNeeded);
        m_atrPeriodSpin->setVisible(atrNeeded);
    };

    connect(m_tpMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateTpMethodVisibility);
    updateTpMethodVisibility(m_tpMethodCombo->currentIndex());
}

void StrategyConfigDialog::setupTradingHoursSection(QVBoxLayout* mainLayout) {
    QGroupBox* tradingHoursGroup = new QGroupBox("Heures de trading", this);
    QFormLayout* tradingHoursLayout = new QFormLayout();
    
    m_tradingFromTime = new QTimeEdit(this);
    m_tradingFromTime->setTime(QTime(15, 30));
    m_tradingFromTime->setDisplayFormat("hh:mm");
    tradingHoursLayout->addRow(new QLabel("Heure de début:", this), m_tradingFromTime);
    
    m_tradingToTime = new QTimeEdit(this);
    m_tradingToTime->setTime(QTime(22, 0));
    m_tradingToTime->setDisplayFormat("hh:mm");
    tradingHoursLayout->addRow(new QLabel("Heure de fin:", this), m_tradingToTime);
    
    // Jours de trading
    QWidget* tradingDaysWidget = new QWidget(this);
    QHBoxLayout* tradingDaysLayout = new QHBoxLayout(tradingDaysWidget);
    
    QStringList dayNames = {"Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};
    m_tradingDayCheckboxes.clear();
    
    for (int i = 0; i < 7; ++i) {
        QCheckBox* dayCheckbox = new QCheckBox(dayNames[i], this);
        dayCheckbox->setChecked(i < 5);
        tradingDaysLayout->addWidget(dayCheckbox);
        m_tradingDayCheckboxes.push_back(dayCheckbox);
    }
    
    tradingHoursLayout->addRow(new QLabel("Jours de trading:", this), tradingDaysWidget);
    
    tradingHoursGroup->setLayout(tradingHoursLayout);
    mainLayout->addWidget(tradingHoursGroup);
}

void StrategyConfigDialog::setupRiskManagementSection(QVBoxLayout* mainLayout) {
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
}

void StrategyConfigDialog::setupAdvancedOptionsSection(QVBoxLayout* mainLayout) {
    QGroupBox* advancedGroup = new QGroupBox("Options avancées", this);
    QVBoxLayout* advancedLayout = new QVBoxLayout();
    
    m_enableLoggingCheck = new QCheckBox("Activer la journalisation (logs)", this);
    m_enableLoggingCheck->setChecked(false);
    advancedLayout->addWidget(m_enableLoggingCheck);
    
    advancedGroup->setLayout(advancedLayout);
    mainLayout->addWidget(advancedGroup);
}

void StrategyConfigDialog::setupButtons(QVBoxLayout* mainLayout) {
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        // Save all config values from widgets
        // Stop Loss
        m_config.sl_method = static_cast<StopLossMethod>(m_slMethodCombo->currentIndex());
        m_config.stop_loss_distance = m_stopLossDistanceSpin->value();
        m_config.stop_loss_atr_multiplier = m_slAtrMultiplierSpin->value();
        m_config.sl_minmax_periods = m_slMinmaxPeriodsSpin->value();
        m_config.sl_minmax_delta_coef_atr = m_slMinmaxCoefAtr->value();
        m_config.min_stop_loss_distance = m_minStopLossDistanceSpin->value();
        
        // Take Profit
        m_config.tp_method = static_cast<TakeProfitMethod>(m_tpMethodCombo->currentIndex());
        m_config.take_profit_distance = m_takeProfitDistanceSpin->value();
        m_config.take_profit_atr_multiplier = m_tpAtrMultiplierSpin->value();
        m_config.tp_sl_ratio = m_tpSlRatioSpin->value();
        m_config.rl_lookback_periods = m_rlLookbackPeriodsSpin->value();
        m_config.min_take_profit_distance = m_minTakeProfitDistanceSpin->value();
        
        // ATR
        m_config.atr_period = m_atrPeriodSpin->value();
        
        // Filters
        m_config.buyFilters = m_buyFiltersWidget->getFilters();
        m_config.sellFilters = m_sellFiltersWidget->getFilters();
        m_config.resaleFilters = m_resaleFiltersWidget->getFilters();
        m_config.rebuyFilters = m_rebuyFiltersWidget->getFilters();
        
        // Trading hours
        QTime fromTime = m_tradingFromTime->time();
        m_config.trading_from = Time{fromTime.hour(), fromTime.minute(), 0};
        QTime toTime = m_tradingToTime->time();
        m_config.trading_to = Time{toTime.hour(), toTime.minute(), 0};
        for (int i = 0; i < 7; ++i) {
            m_config.trading_days_array[i] = m_tradingDayCheckboxes[i]->isChecked();
        }
        
        // Risk management
        m_config.use_risk_based_sizing = m_useRiskBasedSizingCheck->isChecked();
        m_config.risk_percentage = m_riskPercentageSpin->value();
        m_config.use_daily_max_loss = m_useDailyMaxLossCheck->isChecked();
        m_config.daily_max_loss_percentage = m_dailyMaxLossPercentageSpin->value();
        m_config.use_daily_max_profit = m_useDailyMaxProfitCheck->isChecked();
        m_config.daily_max_profit_percentage = m_dailyMaxProfitPercentageSpin->value();
        m_config.use_daily_max_drawdown = m_useDailyMaxDrawdownCheck->isChecked();
        m_config.daily_max_drawdown_percentage = m_dailyMaxDrawdownPercentageSpin->value();
        m_config.use_break_even = m_useBreakEvenCheck->isChecked();
        m_config.break_even_threshold = m_breakEvenThresholdSpin->value();
        m_config.break_even_offset_per_mille = m_breakEvenOffsetSpin->value();
        
        // Advanced
        m_config.enable_logging = m_enableLoggingCheck->isChecked();
        
        accept();
    });
    
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
}

void StrategyConfigDialog::createDependencyGroup(QCheckBox* checkbox, const std::vector<QWidget*>& widgets) {
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
