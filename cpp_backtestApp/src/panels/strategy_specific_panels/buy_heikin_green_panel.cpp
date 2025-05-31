#include "panels/strategy_specific_panels/buy_heikin_green_panel.h"
#include <QDebug>

BuyHeikinGreenPanel::BuyHeikinGreenPanel(QWidget* parent)
    : QObject(parent), BasePanel(parent)
{
}

QGroupBox* BuyHeikinGreenPanel::create()
{
    QGroupBox* strategyGroup = new QGroupBox("Paramètres BuyHeikinGreen");
    QVBoxLayout* strategyLayout = new QVBoxLayout();
    
    // Section EMA Court
    QGroupBox* emaShortGroup = new QGroupBox("EMA Court");
    QVBoxLayout* emaShortLayout = new QVBoxLayout();
    
    // Checkbox pour activer/désactiver le filtre EMA court
    m_widgets["ema_short_filter_check"] = new QCheckBox("Activer le filtre EMA court");
    static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->setChecked(true);
    connect(static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"]), &QCheckBox::toggled,
            [this](bool checked) {
                _toggleWidgetGroup({"ema_short_spin"}, checked);
            });
    emaShortLayout->addWidget(m_widgets["ema_short_filter_check"]);
    
    // Frame pour les paramètres de l'EMA court
    QFrame* emaShortParams = new QFrame();
    QHBoxLayout* emaShortParamsLayout = new QHBoxLayout();
    emaShortParamsLayout->addWidget(new QLabel("Période:"));
    m_widgets["ema_short_spin"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->setRange(1, 500);
    static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->setValue(20);
    emaShortParamsLayout->addWidget(m_widgets["ema_short_spin"]);
    emaShortParamsLayout->addStretch();
    emaShortParams->setLayout(emaShortParamsLayout);
    emaShortLayout->addWidget(emaShortParams);
    
    emaShortGroup->setLayout(emaShortLayout);
    strategyLayout->addWidget(emaShortGroup);
    
    // Section EMA Long
    QGroupBox* emaLongGroup = new QGroupBox("EMA Long");
    QVBoxLayout* emaLongLayout = new QVBoxLayout();
    
    // Checkbox pour activer/désactiver le filtre EMA long
    m_widgets["ema_long_filter_check"] = new QCheckBox("Activer le filtre EMA long");
    static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->setChecked(true);
    connect(static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"]), &QCheckBox::toggled,
            [this](bool checked) {
                _toggleWidgetGroup({"ema_long_spin"}, checked);
            });
    emaLongLayout->addWidget(m_widgets["ema_long_filter_check"]);
    
    // Frame pour les paramètres de l'EMA long
    QFrame* emaLongParams = new QFrame();
    QHBoxLayout* emaLongParamsLayout = new QHBoxLayout();
    emaLongParamsLayout->addWidget(new QLabel("Période:"));
    m_widgets["ema_long_spin"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->setRange(1, 500);
    static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->setValue(200);
    emaLongParamsLayout->addWidget(m_widgets["ema_long_spin"]);
    emaLongParamsLayout->addStretch();
    emaLongParams->setLayout(emaLongParamsLayout);
    emaLongLayout->addWidget(emaLongParams);
    
    emaLongGroup->setLayout(emaLongLayout);
    strategyLayout->addWidget(emaLongGroup);
    
    // Section RSI
    QGroupBox* rsiGroup = new QGroupBox("RSI");
    QVBoxLayout* rsiLayout = new QVBoxLayout();
    
    // Checkbox pour activer/désactiver le filtre RSI
    m_widgets["rsi_filter_check"] = new QCheckBox("Activer le filtre RSI");
    static_cast<QCheckBox*>(m_widgets["rsi_filter_check"])->setChecked(true);
    connect(static_cast<QCheckBox*>(m_widgets["rsi_filter_check"]), &QCheckBox::toggled,
            [this](bool checked) {
                _toggleWidgetGroup({"rsi_period_spin", "rsi_threshold_spin"}, checked);
            });
    rsiLayout->addWidget(m_widgets["rsi_filter_check"]);
    
    // Frame pour les paramètres du RSI
    QFrame* rsiParams = new QFrame();
    QGridLayout* rsiParamsLayout = new QGridLayout();
    
    rsiParamsLayout->addWidget(new QLabel("Période:"), 0, 0);
    m_widgets["rsi_period_spin"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["rsi_period_spin"])->setRange(2, 100);
    static_cast<QSpinBox*>(m_widgets["rsi_period_spin"])->setValue(14);
    rsiParamsLayout->addWidget(m_widgets["rsi_period_spin"], 0, 1);
    
    rsiParamsLayout->addWidget(new QLabel("Seuil filtre:"), 1, 0);
    m_widgets["rsi_threshold_spin"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["rsi_threshold_spin"])->setRange(1, 99);
    static_cast<QSpinBox*>(m_widgets["rsi_threshold_spin"])->setValue(30);
    rsiParamsLayout->addWidget(m_widgets["rsi_threshold_spin"], 1, 1);
    
    rsiParams->setLayout(rsiParamsLayout);
    rsiLayout->addWidget(rsiParams);
    
    rsiGroup->setLayout(rsiLayout);
    strategyLayout->addWidget(rsiGroup);
    
    // Section Stochastique
    QGroupBox* stochGroup = new QGroupBox("Stochastique");
    QVBoxLayout* stochLayout = new QVBoxLayout();
    
    // Checkbox pour activer/désactiver le filtre Stochastique
    m_widgets["stoch_filter_check"] = new QCheckBox("Activer le filtre Stochastique");
    static_cast<QCheckBox*>(m_widgets["stoch_filter_check"])->setChecked(true);
    connect(static_cast<QCheckBox*>(m_widgets["stoch_filter_check"]), &QCheckBox::toggled,
            [this](bool checked) {
                _toggleWidgetGroup({"fastk_spin", "slowk_spin", "slowd_spin", "stoch_threshold_spin"}, checked);
            });
    stochLayout->addWidget(m_widgets["stoch_filter_check"]);
    
    // Frame pour les paramètres du Stochastique
    QFrame* stochParams = new QFrame();
    QGridLayout* stochParamsLayout = new QGridLayout();
    
    stochParamsLayout->addWidget(new QLabel("Fast %K:"), 0, 0);
    m_widgets["fastk_spin"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["fastk_spin"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["fastk_spin"])->setValue(10);
    stochParamsLayout->addWidget(m_widgets["fastk_spin"], 0, 1);
    
    stochParamsLayout->addWidget(new QLabel("Slow %K:"), 1, 0);
    m_widgets["slowk_spin"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["slowk_spin"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["slowk_spin"])->setValue(7);
    stochParamsLayout->addWidget(m_widgets["slowk_spin"], 1, 1);
    
    stochParamsLayout->addWidget(new QLabel("Slow %D:"), 2, 0);
    m_widgets["slowd_spin"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["slowd_spin"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["slowd_spin"])->setValue(3);
    stochParamsLayout->addWidget(m_widgets["slowd_spin"], 2, 1);
    
    stochParamsLayout->addWidget(new QLabel("Seuil:"), 3, 0);
    m_widgets["stoch_threshold_spin"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["stoch_threshold_spin"])->setRange(1, 99);
    static_cast<QSpinBox*>(m_widgets["stoch_threshold_spin"])->setValue(20);
    stochParamsLayout->addWidget(m_widgets["stoch_threshold_spin"], 3, 1);
    
    stochParams->setLayout(stochParamsLayout);
    stochLayout->addWidget(stochParams);
    
    stochGroup->setLayout(stochLayout);
    strategyLayout->addWidget(stochGroup);
    
    // Section Filtre Bougie Précédente
    QGroupBox* candleFilterGroup = new QGroupBox("Filtre de bougie");
    QVBoxLayout* candleFilterLayout = new QVBoxLayout();
    
    m_widgets["previous_ha_candle_red_filter_check"] = new QCheckBox("La bougie précédente doit être rouge");
    candleFilterLayout->addWidget(m_widgets["previous_ha_candle_red_filter_check"]);
    
    candleFilterGroup->setLayout(candleFilterLayout);
    strategyLayout->addWidget(candleFilterGroup);
    
    // Ajouter un espace extensible en bas
    strategyLayout->addStretch();
    
    strategyGroup->setLayout(strategyLayout);
    return strategyGroup;
}

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
    
    // Filtre bougie
    values["use_previous_ha_candle_red_filter"] = static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"])->isChecked();
    
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
    
    // Filtre bougie
    if (values.contains("use_previous_ha_candle_red_filter")) {
        static_cast<QCheckBox*>(m_widgets["previous_ha_candle_red_filter_check"])->setChecked(
            values["use_previous_ha_candle_red_filter"].toBool());
    }
    
    // Mettre à jour l'état d'activation des widgets en fonction des checkboxes
    _toggleWidgetGroup({"ema_short_spin"}, 
                     static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->isChecked());
    
    _toggleWidgetGroup({"ema_long_spin"}, 
                     static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->isChecked());
    
    _toggleWidgetGroup({"rsi_period_spin", "rsi_threshold_spin"}, 
                     static_cast<QCheckBox*>(m_widgets["rsi_filter_check"])->isChecked());
    
    _toggleWidgetGroup({"fastk_spin", "slowk_spin", "slowd_spin", "stoch_threshold_spin"}, 
                     static_cast<QCheckBox*>(m_widgets["stoch_filter_check"])->isChecked());
}

void BuyHeikinGreenPanel::_toggleWidgetGroup(const QStringList& widgets, bool enabled)
{
    for (const QString& widgetName : widgets) {
        if (m_widgets.contains(widgetName)) {
            m_widgets[widgetName]->setEnabled(enabled);
        }
    }
}