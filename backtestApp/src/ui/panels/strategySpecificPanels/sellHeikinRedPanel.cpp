#include "ui/panels/strategySpecificPanels/sellHeikinRedPanel.h"
#include <QDebug>

SellHeikinRedPanel::SellHeikinRedPanel(QWidget* parent)
    : ConfigPanel<SellHeikinRedConfig>("Paramètres SellHeikinRed", parent)
{
    setupUI();
}

void SellHeikinRedPanel::setupUI()
{
    QVBoxLayout* strategyLayout = new QVBoxLayout(this);
    strategyLayout->setSpacing(10);
    strategyLayout->setContentsMargins(10, 15, 10, 15);
    
    // SECTION EMA COURT
    QGridLayout* emaShortLayout = new QGridLayout();
    emaShortLayout->setContentsMargins(5, 0, 5, 5);
    
    QCheckBox* emaShortFilterCheck = new QCheckBox("Activer filtre EMA Court", this);
    emaShortFilterCheck->setChecked(true);
    emaShortLayout->addWidget(emaShortFilterCheck, 0, 0, 1, 2);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        emaShortFilterCheck, 
        &m_config.use_ema_short_filter)
    );
    
    emaShortLayout->addWidget(new QLabel("Période:", this), 1, 0);
    QSpinBox* emaShortSpin = new QSpinBox(this);
    emaShortSpin->setRange(1, 500);
    emaShortSpin->setValue(20);
    emaShortLayout->addWidget(emaShortSpin, 1, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        emaShortSpin, 
        &m_config.ema_short_period)
    );
    
    createDependencyGroup(
        emaShortFilterCheck,
        {emaShortSpin}
    );
    
    emaShortLayout->setColumnStretch(2, 1);
    strategyLayout->addLayout(emaShortLayout);
    
    // Ligne de séparation
    QFrame* line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    strategyLayout->addWidget(line1);
    
    // SECTION EMA LONG
    QGridLayout* emaLongLayout = new QGridLayout();
    emaLongLayout->setContentsMargins(5, 5, 5, 5);
    
    QCheckBox* emaLongFilterCheck = new QCheckBox("Activer filtre EMA Long", this);
    emaLongFilterCheck->setChecked(true);
    emaLongLayout->addWidget(emaLongFilterCheck, 0, 0, 1, 2);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        emaLongFilterCheck, 
        &m_config.use_ema_long_filter)
    );
    
    emaLongLayout->addWidget(new QLabel("Période:", this), 1, 0);
    QSpinBox* emaLongSpin = new QSpinBox(this);
    emaLongSpin->setRange(1, 500);
    emaLongSpin->setValue(200);
    emaLongLayout->addWidget(emaLongSpin, 1, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        emaLongSpin, 
        &m_config.ema_long_period)
    );
    
    createDependencyGroup(
        emaLongFilterCheck,
        {emaLongSpin}
    );
    
    emaLongLayout->setColumnStretch(2, 1);
    strategyLayout->addLayout(emaLongLayout);
    
    // Ligne de séparation
    QFrame* line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    strategyLayout->addWidget(line2);
    
    // SECTION RSI
    QGridLayout* rsiLayout = new QGridLayout();
    rsiLayout->setContentsMargins(5, 5, 5, 5);
    
    QCheckBox* rsiFilterCheck = new QCheckBox("Activer filtre RSI", this);
    rsiFilterCheck->setChecked(true);
    rsiLayout->addWidget(rsiFilterCheck, 0, 0, 1, 2);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        rsiFilterCheck, 
        &m_config.use_rsi_filter)
    );
    
    rsiLayout->addWidget(new QLabel("Période:", this), 1, 0);
    QSpinBox* rsiPeriodSpin = new QSpinBox(this);
    rsiPeriodSpin->setRange(2, 100);
    rsiPeriodSpin->setValue(14);
    rsiLayout->addWidget(rsiPeriodSpin, 1, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        rsiPeriodSpin, 
        &m_config.rsi_period)
    );
    
    rsiLayout->addWidget(new QLabel("Seuil:", this), 2, 0);
    QSpinBox* rsiThresholdSpin = new QSpinBox(this);
    rsiThresholdSpin->setRange(1, 99);
    rsiThresholdSpin->setValue(70);
    rsiLayout->addWidget(rsiThresholdSpin, 2, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        rsiThresholdSpin, 
        &m_config.rsi_threshold)
    );
    
    rsiLayout->addWidget(new QLabel("Périodes d'historique:", this), 3, 0);
    QSpinBox* rsiHistoryPeriodsSpin = new QSpinBox(this);
    rsiHistoryPeriodsSpin->setRange(1, 20);
    rsiHistoryPeriodsSpin->setValue(3);
    rsiLayout->addWidget(rsiHistoryPeriodsSpin, 3, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        rsiHistoryPeriodsSpin, 
        &m_config.rsi_history_periods)
    );
    
    createDependencyGroup(
        rsiFilterCheck,
        {rsiPeriodSpin, rsiThresholdSpin, rsiHistoryPeriodsSpin}
    );
    
    rsiLayout->setColumnStretch(2, 1);
    strategyLayout->addLayout(rsiLayout);
    
    // Ligne de séparation
    QFrame* line3 = new QFrame(this);
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);
    strategyLayout->addWidget(line3);
    
    // SECTION STOCHASTIQUE
    QGridLayout* stochLayout = new QGridLayout();
    stochLayout->setContentsMargins(5, 5, 5, 5);
    
    QCheckBox* stochFilterCheck = new QCheckBox("Activer filtre Stochastique", this);
    stochFilterCheck->setChecked(true);
    stochLayout->addWidget(stochFilterCheck, 0, 0, 1, 2);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        stochFilterCheck, 
        &m_config.use_stoch_filter)
    );
    
    stochLayout->addWidget(new QLabel("Fast %K:", this), 1, 0);
    QSpinBox* fastkSpin = new QSpinBox(this);
    fastkSpin->setRange(1, 100);
    fastkSpin->setValue(10);
    stochLayout->addWidget(fastkSpin, 1, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        fastkSpin, 
        &m_config.stoch_fastk)
    );
    
    stochLayout->addWidget(new QLabel("Slow %K:", this), 2, 0);
    QSpinBox* slowkSpin = new QSpinBox(this);
    slowkSpin->setRange(1, 100);
    slowkSpin->setValue(7);
    stochLayout->addWidget(slowkSpin, 2, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        slowkSpin, 
        &m_config.stoch_slowk)
    );
    
    stochLayout->addWidget(new QLabel("Slow %D:", this), 3, 0);
    QSpinBox* slowdSpin = new QSpinBox(this);
    slowdSpin->setRange(1, 100);
    slowdSpin->setValue(3);
    stochLayout->addWidget(slowdSpin, 3, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        slowdSpin, 
        &m_config.stoch_slowd)
    );
    
    stochLayout->addWidget(new QLabel("Seuil:", this), 4, 0);
    QSpinBox* stochThresholdSpin = new QSpinBox(this);
    stochThresholdSpin->setRange(1, 99);
    stochThresholdSpin->setValue(80);
    stochLayout->addWidget(stochThresholdSpin, 4, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        stochThresholdSpin, 
        &m_config.stoch_threshold)
    );
    
    stochLayout->addWidget(new QLabel("Périodes d'historique:", this), 5, 0);
    QSpinBox* stochHistoryPeriodsSpin = new QSpinBox(this);
    stochHistoryPeriodsSpin->setRange(1, 20);
    stochHistoryPeriodsSpin->setValue(4); // Valeur par défaut
    stochLayout->addWidget(stochHistoryPeriodsSpin, 5, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        stochHistoryPeriodsSpin, 
        &m_config.stoch_history_periods)
    );
    
    createDependencyGroup(
        stochFilterCheck,
        {fastkSpin, slowkSpin, slowdSpin, stochThresholdSpin, stochHistoryPeriodsSpin}
    );
    
    stochLayout->setColumnStretch(2, 1);
    strategyLayout->addLayout(stochLayout);
    
    // Ligne de séparation
    QFrame* line4 = new QFrame(this);
    line4->setFrameShape(QFrame::HLine);
    line4->setFrameShadow(QFrame::Sunken);
    strategyLayout->addWidget(line4);
    
    // SECTION SUPERTREND
    QGridLayout* supertrendLayout = new QGridLayout();
    supertrendLayout->setContentsMargins(5, 5, 5, 5);
    
    QCheckBox* supertrendFilterCheck = new QCheckBox("Activer filtre Supertrend", this);
    supertrendLayout->addWidget(supertrendFilterCheck, 0, 0, 1, 2);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        supertrendFilterCheck, 
        &m_config.use_supertrend_filter)
    );
    
    supertrendLayout->addWidget(new QLabel("Période:", this), 1, 0);
    QSpinBox* supertrendPeriodSpin = new QSpinBox(this);
    supertrendPeriodSpin->setRange(1, 100);
    supertrendPeriodSpin->setValue(10);
    supertrendLayout->addWidget(supertrendPeriodSpin, 1, 1);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        supertrendPeriodSpin, 
        &m_config.supertrend_atr_period)
    );
    
    supertrendLayout->addWidget(new QLabel("Multiplicateur:", this), 2, 0);
    QDoubleSpinBox* supertrendMultiplierSpin = new QDoubleSpinBox(this);
    supertrendMultiplierSpin->setRange(0.1, 10.0);
    supertrendMultiplierSpin->setValue(3.0);
    supertrendMultiplierSpin->setSingleStep(0.1);
    supertrendMultiplierSpin->setDecimals(1);
    supertrendLayout->addWidget(supertrendMultiplierSpin, 2, 1);
    
    addBinding(PropertyBinderFactory::createDoubleBinding(
        supertrendMultiplierSpin, 
        &m_config.supertrend_multiplier)
    );
    
    createDependencyGroup(
        supertrendFilterCheck,
        {supertrendPeriodSpin, supertrendMultiplierSpin}
    );
    
    supertrendLayout->setColumnStretch(2, 1);
    strategyLayout->addLayout(supertrendLayout);
    
    // Ligne de séparation
    QFrame* line5 = new QFrame(this);
    line5->setFrameShape(QFrame::HLine);
    line5->setFrameShadow(QFrame::Sunken);
    strategyLayout->addWidget(line5);
    
    // SECTION AUTRES FILTRES
    QVBoxLayout* otherFiltersLayout = new QVBoxLayout();
    otherFiltersLayout->setContentsMargins(5, 5, 5, 5);
    
    QCheckBox* previousHaCandleGreenFilterCheck = new QCheckBox("Activer filtre précédente(s) bougie(s) verte(s)", this);
    otherFiltersLayout->addWidget(previousHaCandleGreenFilterCheck);
    
    addBinding(PropertyBinderFactory::createBoolBinding(
        previousHaCandleGreenFilterCheck, 
        &m_config.use_previous_ha_candle_green_filter)
    );
    
    QSpinBox* previousHaCandleGreenFilterSpin = new QSpinBox(this);
    previousHaCandleGreenFilterSpin->setRange(1, 100);
    previousHaCandleGreenFilterSpin->setValue(2);
    otherFiltersLayout->addWidget(previousHaCandleGreenFilterSpin);
    
    addBinding(PropertyBinderFactory::createIntBinding(
        previousHaCandleGreenFilterSpin, 
        &m_config.previous_ha_candle_green_filter_n)
    );
    
    createDependencyGroup(
        previousHaCandleGreenFilterCheck,
        {previousHaCandleGreenFilterSpin}
    );
    
    strategyLayout->addLayout(otherFiltersLayout);
    
    // Stretch pour prendre l'espace restant
    strategyLayout->addStretch(1);
}