#include "ui/panels/strategySpecificPanels/buyHeikinGreenPanel.h"


BuyHeikinGreenPanel::BuyHeikinGreenPanel(QWidget* parent)
    : ConfigPanel<BuyHeikinGreenConfig>("Paramètres BuyHeikinGreen", parent)
{
    setupUI();
    initializeBindings();
}

void BuyHeikinGreenPanel::initializeBindings() {
    // EMA Court
    auto emaShortFilterBinder = PropertyBinderFactory::createBoolBinding(
        static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"]), 
        &m_config.use_ema_short_filter);
    addBinding(std::move(emaShortFilterBinder));
    
    auto emaShortPeriodBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["ema_short_spin"]), 
        &m_config.ema_short_period);
    addBinding(std::move(emaShortPeriodBinder));
    
    // Création d'un groupe de dépendance pour le filtre EMA Court
    createDependencyGroup(
        static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"]),
        {m_widgets["ema_short_spin"]}
    );
    
    // EMA Long
    auto emaLongFilterBinder = PropertyBinderFactory::createBoolBinding(
        static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"]), 
        &m_config.use_ema_long_filter);
    addBinding(std::move(emaLongFilterBinder));
    
    auto emaLongPeriodBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["ema_long_spin"]), 
        &m_config.ema_long_period);
    addBinding(std::move(emaLongPeriodBinder));
    
    // Création d'un groupe de dépendance pour le filtre EMA Long
    createDependencyGroup(
        static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"]),
        {m_widgets["ema_long_spin"]}
    );
    
    // RSI
    auto rsiFilterBinder = PropertyBinderFactory::createBoolBinding(
        static_cast<QCheckBox*>(m_widgets["rsi_filter_check"]), 
        &m_config.use_rsi_filter);
    addBinding(std::move(rsiFilterBinder));
    
    auto rsiPeriodBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["rsi_period_spin"]), 
        &m_config.rsi_period);
    addBinding(std::move(rsiPeriodBinder));
    
    auto rsiThresholdBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["rsi_threshold_spin"]), 
        &m_config.rsi_threshold);
    addBinding(std::move(rsiThresholdBinder));
    
    auto rsiHistoryPeriodsBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["rsi_history_periods_spin"]), 
        &m_config.rsi_history_periods);
    addBinding(std::move(rsiHistoryPeriodsBinder));
    
    // Création d'un groupe de dépendance pour le filtre RSI
    createDependencyGroup(
        static_cast<QCheckBox*>(m_widgets["rsi_filter_check"]),
        {m_widgets["rsi_period_spin"], 
         m_widgets["rsi_threshold_spin"], 
         m_widgets["rsi_history_periods_spin"]}
    );
    
    // Stochastique
    auto stochFilterBinder = PropertyBinderFactory::createBoolBinding(
        static_cast<QCheckBox*>(m_widgets["stoch_filter_check"]), 
        &m_config.use_stoch_filter);
    addBinding(std::move(stochFilterBinder));
    
    auto stochFastkBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["fastk_spin"]), 
        &m_config.stoch_fastk);
    addBinding(std::move(stochFastkBinder));
    
    auto stochSlowkBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["slowk_spin"]), 
        &m_config.stoch_slowk);
    addBinding(std::move(stochSlowkBinder));
    
    auto stochSlowdBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["slowd_spin"]), 
        &m_config.stoch_slowd);
    addBinding(std::move(stochSlowdBinder));
    
    auto stochThresholdBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["stoch_threshold_spin"]), 
        &m_config.stoch_threshold);
    addBinding(std::move(stochThresholdBinder));
    
    auto stochHistoryPeriodsBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["stoch_history_periods_spin"]), 
        &m_config.stoch_history_periods);
    addBinding(std::move(stochHistoryPeriodsBinder));
    
    // Création d'un groupe de dépendance pour le filtre Stochastique
    createDependencyGroup(
        static_cast<QCheckBox*>(m_widgets["stoch_filter_check"]),
        {m_widgets["fastk_spin"], 
         m_widgets["slowk_spin"], 
         m_widgets["slowd_spin"], 
         m_widgets["stoch_threshold_spin"],
         m_widgets["stoch_history_periods_spin"]}
    );
    
    // SuperTrend
    auto supertrendFilterBinder = PropertyBinderFactory::createBoolBinding(
        static_cast<QCheckBox*>(m_widgets["supertrend_filter_check"]), 
        &m_config.use_supertrend_filter);
    addBinding(std::move(supertrendFilterBinder));
    
    auto supertrendPeriodBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["supertrend_period_spin"]), 
        &m_config.supertrend_atr_period);
    addBinding(std::move(supertrendPeriodBinder));
    
    auto supertrendMultiplierBinder = PropertyBinderFactory::createDoubleBinding(
        static_cast<QDoubleSpinBox*>(m_widgets["supertrend_multiplier_spin"]), 
        &m_config.supertrend_multiplier);
    addBinding(std::move(supertrendMultiplierBinder));
    
    // Création d'un groupe de dépendance pour le filtre SuperTrend
    createDependencyGroup(
        static_cast<QCheckBox*>(m_widgets["supertrend_filter_check"]),
        {m_widgets["supertrend_period_spin"], 
         m_widgets["supertrend_multiplier_spin"]}
    );
    
    // Filtre bougie précédente rouge
    auto previousHaCandleRedFilterBinder = PropertyBinderFactory::createBoolBinding(
        static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"]), 
        &m_config.use_previous_ha_candle_red_filter);
    addBinding(std::move(previousHaCandleRedFilterBinder));
    
    auto previousHaCandleRedFilterNBinder = PropertyBinderFactory::createIntBinding(
        static_cast<QSpinBox*>(m_widgets["previous_ha_candle_red_filter_spin"]), 
        &m_config.previous_ha_candle_red_filter_n);
    addBinding(std::move(previousHaCandleRedFilterNBinder));
    
    // Création d'un groupe de dépendance pour le filtre bougies précédentes rouges
    createDependencyGroup(
        static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"]),
        {m_widgets["previous_ha_candle_red_filter_spin"]}
    );
}


void BuyHeikinGreenPanel::setupUI()
{
    QWidget* parent = qobject_cast<QWidget*>(this);

    QVBoxLayout* strategyLayout = new QVBoxLayout(this);
    strategyLayout->setSpacing(10);
    strategyLayout->setContentsMargins(10, 15, 10, 15);
    
    // SECTION EMA COURT
    QGridLayout* emaShortLayout = new QGridLayout();
    emaShortLayout->setContentsMargins(5, 0, 5, 5);
    
    m_widgets["ema_short_filter_check"] = new QCheckBox("Activer filtre EMA Court", this);
    static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->setChecked(true);
    // connect(static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"]), &QCheckBox::toggled,
    //         this, &BuyHeikinGreenPanel::onEmaShortFilterToggled);
    emaShortLayout->addWidget(m_widgets["ema_short_filter_check"], 0, 0, 1, 2);
    
    emaShortLayout->addWidget(new QLabel("Période:", this), 1, 0);
    m_widgets["ema_short_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->setRange(1, 500);
    static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->setValue(20);
    emaShortLayout->addWidget(m_widgets["ema_short_spin"], 1, 1);
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
    
    m_widgets["ema_long_filter_check"] = new QCheckBox("Activer filtre EMA Long", this);
    static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->setChecked(true);
    // connect(static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"]), &QCheckBox::toggled,
    //         this, &BuyHeikinGreenPanel::onEmaLongFilterToggled);
    emaLongLayout->addWidget(m_widgets["ema_long_filter_check"], 0, 0, 1, 2);
    
    emaLongLayout->addWidget(new QLabel("Période:", this), 1, 0);
    m_widgets["ema_long_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->setRange(1, 500);
    static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->setValue(200);
    emaLongLayout->addWidget(m_widgets["ema_long_spin"], 1, 1);
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
    
    m_widgets["rsi_filter_check"] = new QCheckBox("Activer filtre RSI", this);
    static_cast<QCheckBox*>(m_widgets["rsi_filter_check"])->setChecked(true);
    // connect(static_cast<QCheckBox*>(m_widgets["rsi_filter_check"]), &QCheckBox::toggled,
    //         this, &BuyHeikinGreenPanel::onRsiFilterToggled);
    rsiLayout->addWidget(m_widgets["rsi_filter_check"], 0, 0, 1, 2);
    
    rsiLayout->addWidget(new QLabel("Période:", this), 1, 0);
    m_widgets["rsi_period_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["rsi_period_spin"])->setRange(2, 100);
    static_cast<QSpinBox*>(m_widgets["rsi_period_spin"])->setValue(14);
    rsiLayout->addWidget(m_widgets["rsi_period_spin"], 1, 1);
    
    rsiLayout->addWidget(new QLabel("Seuil:", this), 2, 0);
    m_widgets["rsi_threshold_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["rsi_threshold_spin"])->setRange(1, 99);
    static_cast<QSpinBox*>(m_widgets["rsi_threshold_spin"])->setValue(30);
    rsiLayout->addWidget(m_widgets["rsi_threshold_spin"], 2, 1);
    
    rsiLayout->addWidget(new QLabel("Périodes d'historique:", this), 3, 0);
    m_widgets["rsi_history_periods_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["rsi_history_periods_spin"])->setRange(1, 20);
    static_cast<QSpinBox*>(m_widgets["rsi_history_periods_spin"])->setValue(3); // Valeur par défaut
    rsiLayout->addWidget(m_widgets["rsi_history_periods_spin"], 3, 1);

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
    
    m_widgets["stoch_filter_check"] = new QCheckBox("Activer filtre Stochastique", this);
    static_cast<QCheckBox*>(m_widgets["stoch_filter_check"])->setChecked(true);
    // connect(static_cast<QCheckBox*>(m_widgets["stoch_filter_check"]), &QCheckBox::toggled,
    //         this, &BuyHeikinGreenPanel::onStochFilterToggled);
    stochLayout->addWidget(m_widgets["stoch_filter_check"], 0, 0, 1, 2);
    
    stochLayout->addWidget(new QLabel("Fast %K:", this), 1, 0);
    m_widgets["fastk_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["fastk_spin"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["fastk_spin"])->setValue(10);
    stochLayout->addWidget(m_widgets["fastk_spin"], 1, 1);
    
    stochLayout->addWidget(new QLabel("Slow %K:", this), 2, 0);
    m_widgets["slowk_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["slowk_spin"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["slowk_spin"])->setValue(7);
    stochLayout->addWidget(m_widgets["slowk_spin"], 2, 1);
    
    stochLayout->addWidget(new QLabel("Slow %D:", this), 3, 0);
    m_widgets["slowd_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["slowd_spin"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["slowd_spin"])->setValue(3);
    stochLayout->addWidget(m_widgets["slowd_spin"], 3, 1);
    
    stochLayout->addWidget(new QLabel("Seuil:", this), 4, 0);
    m_widgets["stoch_threshold_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["stoch_threshold_spin"])->setRange(1, 99);
    static_cast<QSpinBox*>(m_widgets["stoch_threshold_spin"])->setValue(20);
    stochLayout->addWidget(m_widgets["stoch_threshold_spin"], 4, 1);
    
    stochLayout->addWidget(new QLabel("Périodes d'historique:", this), 5, 0);
    m_widgets["stoch_history_periods_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["stoch_history_periods_spin"])->setRange(1, 20);
    static_cast<QSpinBox*>(m_widgets["stoch_history_periods_spin"])->setValue(4); // Valeur par défaut
    stochLayout->addWidget(m_widgets["stoch_history_periods_spin"], 5, 1);

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
    
    m_widgets["supertrend_filter_check"] = new QCheckBox("Activer filtre Supertrend", this);
    // connect(static_cast<QCheckBox*>(m_widgets["supertrend_filter_check"]), &QCheckBox::toggled, this, &BuyHeikinGreenPanel::onSupertrendFilterToggled);
    supertrendLayout->addWidget(m_widgets["supertrend_filter_check"], 0, 0, 1, 2);
    
    supertrendLayout->addWidget(new QLabel("Période:", this), 1, 0);
    m_widgets["supertrend_period_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["supertrend_period_spin"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["supertrend_period_spin"])->setValue(10);
    supertrendLayout->addWidget(m_widgets["supertrend_period_spin"], 1, 1);
    
    supertrendLayout->addWidget(new QLabel("Multiplicateur:", this), 2, 0);
    m_widgets["supertrend_multiplier_spin"] = new QDoubleSpinBox(this);
    static_cast<QDoubleSpinBox*>(m_widgets["supertrend_multiplier_spin"])->setRange(0.1, 10.0);
    static_cast<QDoubleSpinBox*>(m_widgets["supertrend_multiplier_spin"])->setValue(3.0);
    static_cast<QDoubleSpinBox*>(m_widgets["supertrend_multiplier_spin"])->setSingleStep(0.1);
    static_cast<QDoubleSpinBox*>(m_widgets["supertrend_multiplier_spin"])->setDecimals(1);
    supertrendLayout->addWidget(m_widgets["supertrend_multiplier_spin"], 2, 1);
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
    
    m_widgets["previous_ha_candle_red_filter_check"] = new QCheckBox("Activer filtre précédente(s) bougie(s) rouge(s)", this);
    otherFiltersLayout->addWidget(m_widgets["previous_ha_candle_red_filter_check"]);

    // Paramètres n bougies précédentes
    m_widgets["previous_ha_candle_red_filter_spin"] = new QSpinBox(this);
    static_cast<QSpinBox*>(m_widgets["previous_ha_candle_red_filter_spin"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["previous_ha_candle_red_filter_spin"])->setValue(2);
    otherFiltersLayout->addWidget(m_widgets["previous_ha_candle_red_filter_spin"]);
    // connect(static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"]), &QCheckBox::toggled,
    //         this, &BuyHeikinGreenPanel::onPreviousHaCandleRedFilterToggled);
    strategyLayout->addLayout(otherFiltersLayout);

    // Stretch pour prendre l'espace restant
    strategyLayout->addStretch(1);
}

// Les méthodes getValues et setValues restent inchangées

// QMap<QString, QVariant> BuyHeikinGreenPanel::getValues()
// {
//     QMap<QString, QVariant> values;
    
//     // EMA Court
//     values["use_ema_short_filter"] = static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->isChecked();
//     values["ema_short_period"] = static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->value();
    
//     // EMA Long
//     values["use_ema_long_filter"] = static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->isChecked();
//     values["ema_long_period"] = static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->value();
    
//     // RSI
//     values["use_rsi_filter"] = static_cast<QCheckBox*>(m_widgets["rsi_filter_check"])->isChecked();
//     values["rsi_period"] = static_cast<QSpinBox*>(m_widgets["rsi_period_spin"])->value();
//     values["rsi_threshold"] = static_cast<QSpinBox*>(m_widgets["rsi_threshold_spin"])->value();
//     values["rsi_history_periods"] = static_cast<QSpinBox*>(m_widgets["rsi_history_periods_spin"])->value();

//     // Stochastique
//     values["use_stoch_filter"] = static_cast<QCheckBox*>(m_widgets["stoch_filter_check"])->isChecked();
//     values["stoch_fastk"] = static_cast<QSpinBox*>(m_widgets["fastk_spin"])->value();
//     values["stoch_slowk"] = static_cast<QSpinBox*>(m_widgets["slowk_spin"])->value();
//     values["stoch_slowd"] = static_cast<QSpinBox*>(m_widgets["slowd_spin"])->value();
//     values["stoch_threshold"] = static_cast<QSpinBox*>(m_widgets["stoch_threshold_spin"])->value();
//     values["stoch_history_periods"] = static_cast<QSpinBox*>(m_widgets["stoch_history_periods_spin"])->value();

//     // Supertrend
//     values["use_supertrend_filter"] = static_cast<QCheckBox*>(m_widgets["supertrend_filter_check"])->isChecked();
//     values["supertrend_atr_period"] = static_cast<QSpinBox*>(m_widgets["supertrend_period_spin"])->value();
//     values["supertrend_multiplier"] = static_cast<QDoubleSpinBox*>(m_widgets["supertrend_multiplier_spin"])->value();
    
//     // Filtre bougie
//     values["use_previous_ha_candle_red_filter"] = static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"])->isChecked();
//     values["previous_ha_candle_red_filter_n"] = static_cast<QSpinBox*>(m_widgets["previous_ha_candle_red_filter_spin"])->value();
//     return values;
// }

// void BuyHeikinGreenPanel::setValues(const QMap<QString, QVariant>& values)
// {
//     // EMA Court
//     if (values.contains("use_ema_short_filter")) {
//         static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->setChecked(values["use_ema_short_filter"].toBool());
//     }
    
//     if (values.contains("ema_short_period")) {
//         static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->setValue(values["ema_short_period"].toInt());
//     }
    
//     // EMA Long
//     if (values.contains("use_ema_long_filter")) {
//         static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->setChecked(values["use_ema_long_filter"].toBool());
//     }
    
//     if (values.contains("ema_long_period")) {
//         static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->setValue(values["ema_long_period"].toInt());
//     }
    
//     // RSI
//     if (values.contains("use_rsi_filter")) {
//         static_cast<QCheckBox*>(m_widgets["rsi_filter_check"])->setChecked(values["use_rsi_filter"].toBool());
//     }
//     if (values.contains("rsi_period")) {
//         static_cast<QSpinBox*>(m_widgets["rsi_period_spin"])->setValue(values["rsi_period"].toInt());
//     }
//     if (values.contains("rsi_threshold")) {
//         static_cast<QSpinBox*>(m_widgets["rsi_threshold_spin"])->setValue(values["rsi_threshold"].toInt());
//     }
//     if (values.contains("rsi_history_periods")) {
//         static_cast<QSpinBox*>(m_widgets["rsi_history_periods_spin"])->setValue(values["rsi_history_periods"].toInt());
//     }
    
//     // Stochastique
//     if (values.contains("use_stoch_filter")) {
//         static_cast<QCheckBox*>(m_widgets["stoch_filter_check"])->setChecked(values["use_stoch_filter"].toBool());
//     }
//     if (values.contains("stoch_fastk")) {
//         static_cast<QSpinBox*>(m_widgets["fastk_spin"])->setValue(values["stoch_fastk"].toInt());
//     }
//     if (values.contains("stoch_slowk")) {
//         static_cast<QSpinBox*>(m_widgets["slowk_spin"])->setValue(values["stoch_slowk"].toInt());
//     }
//     if (values.contains("stoch_slowd")) {
//         static_cast<QSpinBox*>(m_widgets["slowd_spin"])->setValue(values["stoch_slowd"].toInt());
//     }
//     if (values.contains("stoch_threshold")) {
//         static_cast<QSpinBox*>(m_widgets["stoch_threshold_spin"])->setValue(values["stoch_threshold"].toInt());
//     }
//     if (values.contains("stoch_history_periods")) {
//         static_cast<QSpinBox*>(m_widgets["stoch_history_periods_spin"])->setValue(values["stoch_history_periods"].toInt());
//     }
    
    
//     // Supertrend
//     if (values.contains("use_supertrend_filter")) {
//         static_cast<QCheckBox*>(m_widgets["supertrend_filter_check"])->setChecked(values["use_supertrend_filter"].toBool());
//     }
    
//     if (values.contains("supertrend_atr_period")) {
//         static_cast<QSpinBox*>(m_widgets["supertrend_period_spin"])->setValue(values["supertrend_atr_period"].toInt());
//     }
    
//     if (values.contains("supertrend_multiplier")) {
//         static_cast<QDoubleSpinBox*>(m_widgets["supertrend_multiplier_spin"])->setValue(values["supertrend_multiplier"].toDouble());
//     }
    
//     // Filtre bougie
//     if (values.contains("use_previous_ha_candle_red_filter")) {
//         static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"])->setChecked(
//             values["use_previous_ha_candle_red_filter"].toBool());
//     }
//     if (values.contains("previous_ha_candle_red_filter_n")) {
//         static_cast<QSpinBox*>(m_widgets["previous_ha_candle_red_filter_spin"])->setValue(
//             values["previous_ha_candle_red_filter_n"].toInt());
//     }
// }
