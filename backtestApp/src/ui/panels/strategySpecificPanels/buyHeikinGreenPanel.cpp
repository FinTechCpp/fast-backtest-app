#include "ui/panels/strategySpecificPanels/buyHeikinGreenPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QDebug>

BuyHeikinGreenPanel::BuyHeikinGreenPanel(QWidget* parent)
    : BasePanel("Paramètres BuyHeikinGreen", parent)
{
    // Le constructeur appelle setTitle avec le titre fourni
    // L'initialisation complète se fait dans initialize()
}

void BuyHeikinGreenPanel::initialize()
{
    QVBoxLayout* strategyLayout = new QVBoxLayout(this);
    strategyLayout->setSpacing(10);
    strategyLayout->setContentsMargins(10, 15, 10, 15);
    
    // SECTION EMA COURT
    QGridLayout* emaShortLayout = new QGridLayout();
    emaShortLayout->setContentsMargins(5, 0, 5, 5);
    
    m_widgets["ema_short_filter_check"] = new QCheckBox("Activer filtre EMA Court", this);
    static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->setChecked(true);
    connect(static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"]), &QCheckBox::toggled,
            this, &BuyHeikinGreenPanel::onEmaShortFilterToggled);
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
    connect(static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"]), &QCheckBox::toggled,
            this, &BuyHeikinGreenPanel::onEmaLongFilterToggled);
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
    connect(static_cast<QCheckBox*>(m_widgets["rsi_filter_check"]), &QCheckBox::toggled,
            this, &BuyHeikinGreenPanel::onRsiFilterToggled);
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
    connect(static_cast<QCheckBox*>(m_widgets["stoch_filter_check"]), &QCheckBox::toggled,
            this, &BuyHeikinGreenPanel::onStochFilterToggled);
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
    connect(static_cast<QCheckBox*>(m_widgets["supertrend_filter_check"]), &QCheckBox::toggled, this, &BuyHeikinGreenPanel::onSupertrendFilterToggled);
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
    connect(static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"]), &QCheckBox::toggled,
            this, &BuyHeikinGreenPanel::onPreviousHaCandleRedFilterToggled);
    strategyLayout->addLayout(otherFiltersLayout);

    // Stretch pour prendre l'espace restant
    strategyLayout->addStretch(1);
    
    // Initialiser l'état des widgets
    onEmaShortFilterToggled(static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->isChecked());
    onEmaLongFilterToggled(static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->isChecked());
    onRsiFilterToggled(static_cast<QCheckBox*>(m_widgets["rsi_filter_check"])->isChecked());
    onStochFilterToggled(static_cast<QCheckBox*>(m_widgets["stoch_filter_check"])->isChecked());
    onSupertrendFilterToggled(static_cast<QCheckBox*>(m_widgets["supertrend_filter_check"])->isChecked());
    onPreviousHaCandleRedFilterToggled(static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"])->isChecked());
}

// Les méthodes getValues et setValues restent inchangées

QMap<QString, QVariant> BuyHeikinGreenPanel::getValues()
{
    QMap<QString, QVariant> values;
    
    // EMA Court
    values["use_ema_short_filter"] = static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->isChecked();
    values["ema_short_period"] = static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->value();
    
    // EMA Long
    values["use_ema_long_filter"] = static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->isChecked();
    values["ema_long_period"] = static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->value();
    
    // RSI
    values["use_rsi_filter"] = static_cast<QCheckBox*>(m_widgets["rsi_filter_check"])->isChecked();
    values["rsi_period"] = static_cast<QSpinBox*>(m_widgets["rsi_period_spin"])->value();
    values["rsi_threshold"] = static_cast<QSpinBox*>(m_widgets["rsi_threshold_spin"])->value();
    
    // Stochastique
    values["use_stoch_filter"] = static_cast<QCheckBox*>(m_widgets["stoch_filter_check"])->isChecked();
    values["stoch_fastk"] = static_cast<QSpinBox*>(m_widgets["fastk_spin"])->value();
    values["stoch_slowk"] = static_cast<QSpinBox*>(m_widgets["slowk_spin"])->value();
    values["stoch_slowd"] = static_cast<QSpinBox*>(m_widgets["slowd_spin"])->value();
    values["stoch_threshold"] = static_cast<QSpinBox*>(m_widgets["stoch_threshold_spin"])->value();
    
    // Supertrend
    values["use_supertrend_filter"] = static_cast<QCheckBox*>(m_widgets["supertrend_filter_check"])->isChecked();
    values["supertrend_atr_period"] = static_cast<QSpinBox*>(m_widgets["supertrend_period_spin"])->value();
    values["supertrend_multiplier"] = static_cast<QDoubleSpinBox*>(m_widgets["supertrend_multiplier_spin"])->value();
    
    // Filtre bougie
    values["use_previous_ha_candle_red_filter"] = static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"])->isChecked();
    values["previous_ha_candle_red_filter_n"] = static_cast<QSpinBox*>(m_widgets["previous_ha_candle_red_filter_spin"])->value();
    return values;
}

void BuyHeikinGreenPanel::setValues(const QMap<QString, QVariant>& values)
{
    // EMA Court
    if (values.contains("use_ema_short_filter")) {
        static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->setChecked(values["use_ema_short_filter"].toBool());
    }
    
    if (values.contains("ema_short_period")) {
        static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->setValue(values["ema_short_period"].toInt());
    }
    
    // EMA Long
    if (values.contains("use_ema_long_filter")) {
        static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->setChecked(values["use_ema_long_filter"].toBool());
    }
    
    if (values.contains("ema_long_period")) {
        static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->setValue(values["ema_long_period"].toInt());
    }
    
    // RSI
    if (values.contains("use_rsi_filter")) {
        static_cast<QCheckBox*>(m_widgets["rsi_filter_check"])->setChecked(values["use_rsi_filter"].toBool());
    }
    
    if (values.contains("rsi_period")) {
        static_cast<QSpinBox*>(m_widgets["rsi_period_spin"])->setValue(values["rsi_period"].toInt());
    }
    
    if (values.contains("rsi_threshold")) {
        static_cast<QSpinBox*>(m_widgets["rsi_threshold_spin"])->setValue(values["rsi_threshold"].toInt());
    }
    
    // Stochastique
    if (values.contains("use_stoch_filter")) {
        static_cast<QCheckBox*>(m_widgets["stoch_filter_check"])->setChecked(values["use_stoch_filter"].toBool());
    }
    
    if (values.contains("stoch_fastk")) {
        static_cast<QSpinBox*>(m_widgets["fastk_spin"])->setValue(values["stoch_fastk"].toInt());
    }
    
    if (values.contains("stoch_slowk")) {
        static_cast<QSpinBox*>(m_widgets["slowk_spin"])->setValue(values["stoch_slowk"].toInt());
    }
    
    if (values.contains("stoch_slowd")) {
        static_cast<QSpinBox*>(m_widgets["slowd_spin"])->setValue(values["stoch_slowd"].toInt());
    }
    
    if (values.contains("stoch_threshold")) {
        static_cast<QSpinBox*>(m_widgets["stoch_threshold_spin"])->setValue(values["stoch_threshold"].toInt());
    }
    
    // Supertrend
    if (values.contains("use_supertrend_filter")) {
        static_cast<QCheckBox*>(m_widgets["supertrend_filter_check"])->setChecked(values["use_supertrend_filter"].toBool());
    }
    
    if (values.contains("supertrend_atr_period")) {
        static_cast<QSpinBox*>(m_widgets["supertrend_period_spin"])->setValue(values["supertrend_atr_period"].toInt());
    }
    
    if (values.contains("supertrend_multiplier")) {
        static_cast<QDoubleSpinBox*>(m_widgets["supertrend_multiplier_spin"])->setValue(values["supertrend_multiplier"].toDouble());
    }
    
    // Filtre bougie
    if (values.contains("use_previous_ha_candle_red_filter")) {
        static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"])->setChecked(
            values["use_previous_ha_candle_red_filter"].toBool());
    }
    if (values.contains("previous_ha_candle_red_filter_n")) {
        static_cast<QSpinBox*>(m_widgets["previous_ha_candle_red_filter_spin"])->setValue(
            values["previous_ha_candle_red_filter_n"].toInt());
    }
}

void BuyHeikinGreenPanel::onEmaShortFilterToggled(bool checked) {
    _toggleWidgetGroup({"ema_short_spin"}, checked);
}

void BuyHeikinGreenPanel::onEmaLongFilterToggled(bool checked) {
    _toggleWidgetGroup({"ema_long_spin"}, checked);
}

void BuyHeikinGreenPanel::onRsiFilterToggled(bool checked) {
    _toggleWidgetGroup({"rsi_period_spin", "rsi_threshold_spin"}, checked);
}

void BuyHeikinGreenPanel::onStochFilterToggled(bool checked) {
    _toggleWidgetGroup({"fastk_spin", "slowk_spin", "slowd_spin", "stoch_threshold_spin"}, checked);
}

void BuyHeikinGreenPanel::onSupertrendFilterToggled(bool checked) {
    _toggleWidgetGroup({"supertrend_period_spin", "supertrend_multiplier_spin"}, checked);
}

void BuyHeikinGreenPanel::onPreviousHaCandleRedFilterToggled(bool checked) {
    _toggleWidgetGroup({"previous_ha_candle_red_filter_spin"}, checked);
}

void BuyHeikinGreenPanel::_toggleWidgetGroup(const QStringList& widgets, bool enabled)
{
    for (const QString& widgetName : widgets) {
        if (m_widgets.contains(widgetName)) {
            m_widgets[widgetName]->setEnabled(enabled);
            if(enabled) 
                m_widgets[widgetName]->setStyleSheet("QSpinBox, QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
             else 
                m_widgets[widgetName]->setStyleSheet("QSpinBox, QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
        }
    }
}