#include "ui/panels/StrategyPanel.h"

#include "ui/panels/FiltersWidget.h"
#include <QDebug>

StrategyPanel::StrategyPanel(QWidget* parent)
    : ConfigPanel("Paramètres de base", parent)
{
    // Initialiser le tableau des jours de trading (Lun-Ven activés par défaut)
    for (int i = 0; i < 7; ++i) {
        m_config.trading_days_array[i] = (i < 5); // Jours 0-4 (Lun-Ven) activés
    }
    
    setupUI();
}

void StrategyPanel::setupUI() {    
    // Utiliser "this" comme conteneur principal au lieu de créer un nouveau QGroupBox
    QVBoxLayout* baseLayout = new QVBoxLayout(this);

    // Section Paramètres avancés
    // Option pour activer/désactiver les logs
    QCheckBox* enableLoggingCheck = new QCheckBox("Activer la journalisation (logs)", this);
    enableLoggingCheck->setChecked(false); // Désactivé par défaut
    baseLayout->addWidget(enableLoggingCheck);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        enableLoggingCheck,
        &m_config.enable_logging
    ));

    // Section Direction de trading
    QHBoxLayout* directionLayout = new QHBoxLayout();
    directionLayout->addWidget(new QLabel("Direction de trading:", this));
    QComboBox* strategyDirectionCombo = new QComboBox(this);
    strategyDirectionCombo->addItems({"", "Long", "Short"});
    strategyDirectionCombo->setCurrentIndex(1); // Long par défaut
    directionLayout->addWidget(strategyDirectionCombo);

    baseLayout->addLayout(directionLayout);

    addBinding(PropertyBinderFactory::createEnumComboBinding(
        strategyDirectionCombo,
        &m_config.tradeDirection
    ));

    // Section SL/TP
    QGroupBox* slTpGroup = new QGroupBox("Stop Loss et Take Profit", this);
    QVBoxLayout* slTpLayout = new QVBoxLayout();
    
    // Période ATR commune (en haut du groupe)
    QFormLayout* atrLayout = new QFormLayout();
    QSpinBox* atrPeriodSpin = new QSpinBox(this);
    atrPeriodSpin->setRange(1, 1000);
    atrPeriodSpin->setValue(14);
    atrPeriodSpin->setEnabled(false);
    atrPeriodSpin->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    atrLayout->addRow(new QLabel("Période ATR:", this), atrPeriodSpin);
    slTpLayout->addLayout(atrLayout);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        atrPeriodSpin,
        &m_config.atr_period
    ));
    
    // Groupe Stop Loss
    QGroupBox* slGroup = new QGroupBox("Stop Loss", slTpGroup);
    QFormLayout* slLayout = new QFormLayout();
    
    // Méthode de calcul pour le Stop Loss
    QComboBox* slMethodCombo = new QComboBox(this);
    slMethodCombo->addItems({"Fixe", "ATR", "Min/Max"});
    slMethodCombo->setCurrentIndex(0);
    slLayout->addRow(new QLabel("Méthode:", this), slMethodCombo);
    
    addBinding(PropertyBinderFactory::createEnumComboBinding<StopLossMethod>(
        slMethodCombo,
        &m_config.sl_method
    ));
    
    // Stop Loss Distance (fixe)
    QDoubleSpinBox* stopLossDistanceSpin = new QDoubleSpinBox(this);
    stopLossDistanceSpin->setDecimals(4);
    stopLossDistanceSpin->setRange(0, 10000);
    stopLossDistanceSpin->setSingleStep(1);
    stopLossDistanceSpin->setValue(20);
    slLayout->addRow(new QLabel("Distance [pts]:", this), stopLossDistanceSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        stopLossDistanceSpin,
        &m_config.stop_loss_distance
    ));
    
    // Paramètres ATR - Multiplicateur SL
    QDoubleSpinBox* slAtrMultiplierSpin = new QDoubleSpinBox(this);
    slAtrMultiplierSpin->setDecimals(2);
    slAtrMultiplierSpin->setRange(0.1, 1000.0);
    slAtrMultiplierSpin->setSingleStep(0.1);
    slAtrMultiplierSpin->setValue(2.0);
    slAtrMultiplierSpin->setEnabled(false);
    slAtrMultiplierSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    slLayout->addRow(new QLabel("Multiplicateur ATR SL:", this), slAtrMultiplierSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        slAtrMultiplierSpin,
        &m_config.stop_loss_atr_multiplier
    ));

    // Paramètres ATR - Coefficient Min/Max SL pour delta
    QDoubleSpinBox* slMinmaxCoefAtr = new QDoubleSpinBox(this);
    slMinmaxCoefAtr->setDecimals(2);
    slMinmaxCoefAtr->setRange(0, 1000.0);
    slMinmaxCoefAtr->setValue(5.0);
    slMinmaxCoefAtr->setEnabled(false);
    slMinmaxCoefAtr->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    slLayout->addRow(new QLabel("Coefficient Delta Min/Max:", this), slMinmaxCoefAtr);

    addBinding(PropertyBinderFactory::createDoubleBinding(
        slMinmaxCoefAtr,
        &m_config.sl_minmax_delta_coef_atr
    ));

    // Paramètres Min/Max SL
    QSpinBox* slMinmaxPeriodsSpin = new QSpinBox(this);
    slMinmaxPeriodsSpin->setRange(1, 1000);
    slMinmaxPeriodsSpin->setValue(5);
    slMinmaxPeriodsSpin->setEnabled(false);
    slMinmaxPeriodsSpin->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    slLayout->addRow(new QLabel("Périodes Min/Max:", this), slMinmaxPeriodsSpin);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        slMinmaxPeriodsSpin,
        &m_config.sl_minmax_periods
    ));
    
    QDoubleSpinBox* slMinmaxDeltaSpin = new QDoubleSpinBox(this);
    slMinmaxDeltaSpin->setDecimals(2);
    slMinmaxDeltaSpin->setRange(0, 1000.0);
    slMinmaxDeltaSpin->setValue(5.0);
    slMinmaxDeltaSpin->setEnabled(false);
    slMinmaxDeltaSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    slLayout->addRow(new QLabel("Delta Min/Max:", this), slMinmaxDeltaSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        slMinmaxDeltaSpin,
        &m_config.sl_minmax_delta_coef_atr
    ));
    
    // SL minimum
    QDoubleSpinBox* minStopLossDistanceSpin = new QDoubleSpinBox(this);
    minStopLossDistanceSpin->setDecimals(4);
    minStopLossDistanceSpin->setRange(0, 1000.0);
    minStopLossDistanceSpin->setValue(5.0);
    slLayout->addRow(new QLabel("SL Minimum [pts]:", this), minStopLossDistanceSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        minStopLossDistanceSpin,
        &m_config.min_stop_loss_distance
    ));
    
    slGroup->setLayout(slLayout);
    slTpLayout->addWidget(slGroup);
    
    // Groupe Take Profit
    QGroupBox* tpGroup = new QGroupBox("Take Profit", slTpGroup);
    QFormLayout* tpLayout = new QFormLayout();
    
    // Méthode de calcul pour le Take Profit
    QComboBox* tpMethodCombo = new QComboBox(this);
    tpMethodCombo->addItems({"Fixe", "ATR", "Ratio SL", "SuperTrend", "RL", "Nth Heikin-Ashi"});
    tpMethodCombo->setCurrentIndex(0);
    tpLayout->addRow(new QLabel("Méthode:", this), tpMethodCombo);
    
    addBinding(PropertyBinderFactory::createEnumComboBinding<TakeProfitMethod>(
        tpMethodCombo,
        &m_config.tp_method
    ));
    
    // Take Profit Distance (fixe)
    QDoubleSpinBox* takeProfitDistanceSpin = new QDoubleSpinBox(this);
    takeProfitDistanceSpin->setDecimals(4);
    takeProfitDistanceSpin->setRange(0, 10000);
    takeProfitDistanceSpin->setSingleStep(1);
    takeProfitDistanceSpin->setValue(30);
    tpLayout->addRow(new QLabel("Distance [pts]:", this), takeProfitDistanceSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        takeProfitDistanceSpin,
        &m_config.take_profit_distance
    ));
    
    // Paramètres ATR - Multiplicateur TP
    QDoubleSpinBox* tpAtrMultiplierSpin = new QDoubleSpinBox(this);
    tpAtrMultiplierSpin->setDecimals(2);
    tpAtrMultiplierSpin->setRange(0, 1000.0);
    tpAtrMultiplierSpin->setSingleStep(0.1);
    tpAtrMultiplierSpin->setValue(3.0);
    tpAtrMultiplierSpin->setEnabled(false);
    tpAtrMultiplierSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Multiplicateur ATR TP:", this), tpAtrMultiplierSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        tpAtrMultiplierSpin,
        &m_config.take_profit_atr_multiplier
    ));
    
    // Paramètres Ratio SL - Multiplicateur TP
    QDoubleSpinBox* tpSlRatioSpin = new QDoubleSpinBox(this);
    tpSlRatioSpin->setDecimals(2);
    tpSlRatioSpin->setRange(0.1, 100.0);
    tpSlRatioSpin->setSingleStep(0.1);
    tpSlRatioSpin->setValue(2.0);
    tpSlRatioSpin->setEnabled(false);
    tpSlRatioSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Ratio TP/SL:", this), tpSlRatioSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        tpSlRatioSpin,
        &m_config.tp_sl_ratio
    ));
    
    // Paramètres SuperTrend - Période ATR
    QSpinBox* tpSupertrendAtrPeriodSpin = new QSpinBox(this);
    tpSupertrendAtrPeriodSpin->setRange(1, 1000);
    tpSupertrendAtrPeriodSpin->setValue(14);
    tpSupertrendAtrPeriodSpin->setEnabled(false);
    tpSupertrendAtrPeriodSpin->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Période ATR SuperTrend:", this), tpSupertrendAtrPeriodSpin);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        tpSupertrendAtrPeriodSpin,
        &m_config.tp_supertrend_atr_period
    ));
    
    // Paramètres SuperTrend - Multiplicateur
    QDoubleSpinBox* tpSupertrendMultiplierSpin = new QDoubleSpinBox(this);
    tpSupertrendMultiplierSpin->setDecimals(1);
    tpSupertrendMultiplierSpin->setRange(0.1, 1000.0);
    tpSupertrendMultiplierSpin->setSingleStep(0.1);
    tpSupertrendMultiplierSpin->setValue(3.0);
    tpSupertrendMultiplierSpin->setEnabled(false);
    tpSupertrendMultiplierSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Multiplicateur SuperTrend:", this), tpSupertrendMultiplierSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        tpSupertrendMultiplierSpin,
        &m_config.tp_supertrend_multiplier
    ));
    
    // Paramètres RL - Périodes de lookback
    QSpinBox* rlLookbackPeriodsSpin = new QSpinBox(this);
    rlLookbackPeriodsSpin->setRange(1, 150);
    rlLookbackPeriodsSpin->setEnabled(false);
    rlLookbackPeriodsSpin->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Périodes lookback RL:", this), rlLookbackPeriodsSpin);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        rlLookbackPeriodsSpin,
        &m_config.rl_lookback_periods
    ));

    // Paramètres nth Heikin-Ashi - Nombre de bougies opposées
    QSpinBox* nthHeikinAshiCountSpin = new QSpinBox(this);
    nthHeikinAshiCountSpin->setRange(1, 50);
    nthHeikinAshiCountSpin->setValue(3);
    nthHeikinAshiCountSpin->setEnabled(false);
    nthHeikinAshiCountSpin->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Nb bougies Heikin-Ashi:", this), nthHeikinAshiCountSpin);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        nthHeikinAshiCountSpin,
        &m_config.nth_heikin_ashi_count
    ));
    
    // TP minimum
    QDoubleSpinBox* minTakeProfitDistanceSpin = new QDoubleSpinBox(this);
    minTakeProfitDistanceSpin->setDecimals(1);
    minTakeProfitDistanceSpin->setRange(0, 1000.0);
    minTakeProfitDistanceSpin->setValue(5.0);
    tpLayout->addRow(new QLabel("TP Minimum [pts]:", this), minTakeProfitDistanceSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        minTakeProfitDistanceSpin,
        &m_config.min_take_profit_distance
    ));
    
    tpGroup->setLayout(tpLayout);
    slTpLayout->addWidget(tpGroup);
    
    slTpGroup->setLayout(slTpLayout);
    baseLayout->addWidget(slTpGroup);
    
    // Connecter les signaux pour activer/désactiver les widgets en fonction des sélections
    connect(slMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int index) {
                bool isFixed = (index == 0);
                bool isAtr = (index == 1);
                bool isMinMax = (index == 2);
                
                stopLossDistanceSpin->setEnabled(isFixed);
                stopLossDistanceSpin->setStyleSheet(isFixed ? 
                    "QDoubleSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                slAtrMultiplierSpin->setEnabled(isAtr);
                slAtrMultiplierSpin->setStyleSheet(isAtr ? 
                    "QDoubleSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                slMinmaxPeriodsSpin->setEnabled(isMinMax);
                slMinmaxPeriodsSpin->setStyleSheet(isMinMax ? 
                    "QSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                slMinmaxCoefAtr->setEnabled(isMinMax);
                slMinmaxCoefAtr->setStyleSheet(isMinMax ? 
                    "QDoubleSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                // Mettre à jour le statut de la période ATR
                bool atrNeeded = isAtr || isMinMax || (tpMethodCombo->currentIndex() == 1);
                atrPeriodSpin->setEnabled(atrNeeded);
                atrPeriodSpin->setStyleSheet(atrNeeded ? 
                    "QSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QSpinBox { background-color: #f0f0f0; color: #888888; }");
            });
    
    connect(tpMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int index) {
                bool isFixed = (index == 0);
                bool isAtr = (index == 1);
                bool isRatio = (index == 2);
                bool isSupertrend = (index == 3);
                bool isRL = (index == 4);
                bool isNthHeikinAshi = (index == 5);
                
                takeProfitDistanceSpin->setEnabled(isFixed);
                takeProfitDistanceSpin->setStyleSheet(isFixed ? 
                    "QDoubleSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                tpAtrMultiplierSpin->setEnabled(isAtr);
                tpAtrMultiplierSpin->setStyleSheet(isAtr ? 
                    "QDoubleSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                tpSlRatioSpin->setEnabled(isRatio);
                tpSlRatioSpin->setStyleSheet(isRatio ? 
                    "QDoubleSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                tpSupertrendAtrPeriodSpin->setEnabled(isSupertrend);
                tpSupertrendAtrPeriodSpin->setStyleSheet(isSupertrend ? 
                    "QSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                tpSupertrendMultiplierSpin->setEnabled(isSupertrend);
                tpSupertrendMultiplierSpin->setStyleSheet(isSupertrend ? 
                    "QDoubleSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                rlLookbackPeriodsSpin->setEnabled(isRL);
                rlLookbackPeriodsSpin->setStyleSheet(isRL ? 
                    "QSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                nthHeikinAshiCountSpin->setEnabled(isNthHeikinAshi);
                nthHeikinAshiCountSpin->setStyleSheet(isNthHeikinAshi ? 
                    "QSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QSpinBox { background-color: #f0f0f0; color: #888888; }");
                
                // Mettre à jour le statut de la période ATR
                bool atrNeeded = isAtr || (slMethodCombo->currentIndex() == 1) || (slMethodCombo->currentIndex() == 2);
                atrPeriodSpin->setEnabled(atrNeeded);
                atrPeriodSpin->setStyleSheet(atrNeeded ? 
                    "QSpinBox { background-color: #ffffff; color: #000000; }" : 
                    "QSpinBox { background-color: #f0f0f0; color: #888888; }");
            });
    
    // Section Heures de trading
    QGroupBox* tradingHoursGroup = new QGroupBox("Heures de trading", this);
    QFormLayout* tradingHoursLayout = new QFormLayout();
    
    QTimeEdit* tradingFromTime = new QTimeEdit(this);
    tradingFromTime->setTime(QTime(15, 30));
    tradingFromTime->setDisplayFormat("hh:mm");
    tradingHoursLayout->addRow(new QLabel("Heure de début:", this), tradingFromTime);
    
    addBinding(PropertyBinderFactory::createTimeBinding(
        tradingFromTime,
        &m_config.trading_from
    ));
    
    QTimeEdit* tradingToTime = new QTimeEdit(this);
    tradingToTime->setTime(QTime(22, 0));
    tradingToTime->setDisplayFormat("hh:mm");
    tradingHoursLayout->addRow(new QLabel("Heure de fin:", this), tradingToTime);
    
    addBinding(PropertyBinderFactory::createTimeBinding(
        tradingToTime,
        &m_config.trading_to
    ));
    
    // Jours de trading
    QWidget* tradingDaysWidget = new QWidget(this);
    QHBoxLayout* tradingDaysLayout = new QHBoxLayout(tradingDaysWidget);
    
    QStringList dayNames = {"Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};
    std::vector<QCheckBox*> tradingDayCheckboxes;
    
    for (int i = 0; i < 7; ++i) {
        QCheckBox* dayCheckbox = new QCheckBox(dayNames[i], this);
        dayCheckbox->setChecked(i < 5); // Par défaut, activer du lundi au vendredi
        tradingDaysLayout->addWidget(dayCheckbox);
        tradingDayCheckboxes.push_back(dayCheckbox);
        
        addBinding(PropertyBinderFactory::createBoolBinding(
            dayCheckbox,
            &m_config.trading_days_array[i]
        ));
    }
    
    tradingHoursLayout->addRow(new QLabel("Jours de trading:", this), tradingDaysWidget);
    
    tradingHoursGroup->setLayout(tradingHoursLayout);
    baseLayout->addWidget(tradingHoursGroup);
    
    // Section Gestion du risque
    QGroupBox* riskGroup = new QGroupBox("Gestion du risque", this);
    QFormLayout* riskLayout = new QFormLayout();
    
    // Taille basée sur le risque
    QCheckBox* useRiskBasedSizingCheck = new QCheckBox("Taille basée sur le risque", this);
    riskLayout->addRow(useRiskBasedSizingCheck);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        useRiskBasedSizingCheck,
        &m_config.use_risk_based_sizing
    ));
    
    QDoubleSpinBox* riskPercentageSpin = new QDoubleSpinBox(this);
    riskPercentageSpin->setDecimals(2);
    riskPercentageSpin->setRange(0.1, 100.0);
    riskPercentageSpin->setValue(1.0);
    riskPercentageSpin->setSuffix("%");
    riskPercentageSpin->setEnabled(false);
    riskPercentageSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    riskLayout->addRow(new QLabel("Risque par trade:", this), riskPercentageSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        riskPercentageSpin,
        &m_config.risk_percentage
    ));
    
    createDependencyGroup(
        useRiskBasedSizingCheck,
        {riskPercentageSpin}
    );
    
    // Perte max journalière
    QCheckBox* useDailyMaxLossCheck = new QCheckBox("Perte max journalière", this);
    riskLayout->addRow(useDailyMaxLossCheck);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        useDailyMaxLossCheck,
        &m_config.use_daily_max_loss
    ));
    
    QDoubleSpinBox* dailyMaxLossPercentageSpin = new QDoubleSpinBox(this);
    dailyMaxLossPercentageSpin->setDecimals(2);
    dailyMaxLossPercentageSpin->setRange(0.1, 100.0);
    dailyMaxLossPercentageSpin->setValue(2.0);
    dailyMaxLossPercentageSpin->setSuffix("%");
    dailyMaxLossPercentageSpin->setEnabled(false);
    dailyMaxLossPercentageSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    riskLayout->addRow(new QLabel("Perte max journalière:", this), dailyMaxLossPercentageSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        dailyMaxLossPercentageSpin,
        &m_config.daily_max_loss_percentage
    ));
    
    createDependencyGroup(
        useDailyMaxLossCheck,
        {dailyMaxLossPercentageSpin}
    );
    
    // Profit max journalier
    QCheckBox* useDailyMaxProfitCheck = new QCheckBox("Profit max journalier", this);
    riskLayout->addRow(useDailyMaxProfitCheck);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        useDailyMaxProfitCheck,
        &m_config.use_daily_max_profit
    ));
    
    QDoubleSpinBox* dailyMaxProfitPercentageSpin = new QDoubleSpinBox(this);
    dailyMaxProfitPercentageSpin->setDecimals(2);
    dailyMaxProfitPercentageSpin->setRange(0.1, 100.0);
    dailyMaxProfitPercentageSpin->setValue(5.0);
    dailyMaxProfitPercentageSpin->setSuffix("%");
    dailyMaxProfitPercentageSpin->setEnabled(false);
    dailyMaxProfitPercentageSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    riskLayout->addRow(new QLabel("Profit max journalier:", this), dailyMaxProfitPercentageSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        dailyMaxProfitPercentageSpin,
        &m_config.daily_max_profit_percentage
    ));
    
    createDependencyGroup(
        useDailyMaxProfitCheck,
        {dailyMaxProfitPercentageSpin}
    );
    
    // Drawdown max journalier
    QCheckBox* useDailyMaxDrawdownCheck = new QCheckBox("Drawdown max journalier", this);
    riskLayout->addRow(useDailyMaxDrawdownCheck);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        useDailyMaxDrawdownCheck,
        &m_config.use_daily_max_drawdown
    ));
    
    QDoubleSpinBox* dailyMaxDrawdownPercentageSpin = new QDoubleSpinBox(this);
    dailyMaxDrawdownPercentageSpin->setDecimals(2);
    dailyMaxDrawdownPercentageSpin->setRange(0.1, 100.0);
    dailyMaxDrawdownPercentageSpin->setValue(3.0);
    dailyMaxDrawdownPercentageSpin->setSuffix("%");
    dailyMaxDrawdownPercentageSpin->setEnabled(false);
    dailyMaxDrawdownPercentageSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    riskLayout->addRow(new QLabel("Drawdown max journalier:", this), dailyMaxDrawdownPercentageSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        dailyMaxDrawdownPercentageSpin,
        &m_config.daily_max_drawdown_percentage
    ));
    
    createDependencyGroup(
        useDailyMaxDrawdownCheck,
        {dailyMaxDrawdownPercentageSpin}
    );
    
    // Break even
    QCheckBox* useBreakEvenCheck = new QCheckBox("Activer Break Even", this);
    riskLayout->addRow(useBreakEvenCheck);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        useBreakEvenCheck,
        &m_config.use_break_even
    ));
    
    QDoubleSpinBox* breakEvenThresholdSpin = new QDoubleSpinBox(this);
    breakEvenThresholdSpin->setDecimals(2);
    breakEvenThresholdSpin->setRange(0.0, 10.0);
    breakEvenThresholdSpin->setSingleStep(0.05);
    breakEvenThresholdSpin->setValue(0.7);
    breakEvenThresholdSpin->setEnabled(false);
    breakEvenThresholdSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    riskLayout->addRow(new QLabel("Seuil Break Even:", this), breakEvenThresholdSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        breakEvenThresholdSpin,
        &m_config.break_even_threshold
    ));
    
    QDoubleSpinBox* breakEvenOffsetSpin = new QDoubleSpinBox(this);
    breakEvenOffsetSpin->setDecimals(3);
    breakEvenOffsetSpin->setRange(-50.0, 50.0);
    breakEvenOffsetSpin->setSingleStep(0.05);
    breakEvenOffsetSpin->setValue(0.0);
    breakEvenOffsetSpin->setSuffix("‰");
    breakEvenOffsetSpin->setEnabled(false);
    breakEvenOffsetSpin->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    riskLayout->addRow(new QLabel("Offset Break Even (‰):", this), breakEvenOffsetSpin);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        breakEvenOffsetSpin,
        &m_config.break_even_offset_per_mille
    ));
    
    createDependencyGroup(
        useBreakEvenCheck,
        {breakEvenThresholdSpin, breakEvenOffsetSpin}
    );
    
    riskGroup->setLayout(riskLayout);
    baseLayout->addWidget(riskGroup);


    // Section de configuration des filtres
    FiltersWidget* filtersWidget = new FiltersWidget(this);
    filtersWidget->setFilters(m_config.filters);
    QVBoxLayout* filtersLayout = new QVBoxLayout();
    filtersLayout->addWidget(filtersWidget);
    baseLayout->addLayout(filtersLayout);

    addBinding(PropertyBinderFactory::createFiltersBinding(
        filtersWidget,
        &m_config.filters
    ));
    
    setLayout(baseLayout);
}

