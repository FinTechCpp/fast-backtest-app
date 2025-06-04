#include "panels/strategy_specific_panels/sell_heikin_red_panel.h"
#include <QDebug>

SellHeikinRedPanel::SellHeikinRedPanel(QWidget* parent)
    : BasePanel("Paramètres SellHeikinRed", parent)
{
    // Le constructeur appelle uniquement le constructeur de la classe parente
}

void SellHeikinRedPanel::initialize()
{
    // Utiliser "this" comme conteneur principal
    QVBoxLayout* strategyLayout = new QVBoxLayout(this);
    
    // Section EMA Court
    QGroupBox* emaShortGroup = new QGroupBox("EMA Court", this);
    QVBoxLayout* emaShortLayout = new QVBoxLayout(emaShortGroup);
    
    // Checkbox pour activer/désactiver le filtre EMA court
    m_widgets["ema_short_filter_check"] = new QCheckBox("Activer le filtre EMA court", emaShortGroup);
    static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->setChecked(true);
    connect(static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"]), &QCheckBox::toggled,
            this, &SellHeikinRedPanel::onEmaShortFilterToggled);
    emaShortLayout->addWidget(m_widgets["ema_short_filter_check"]);
    
    // Frame pour les paramètres de l'EMA court
    QFrame* emaShortParams = new QFrame(emaShortGroup);
    QHBoxLayout* emaShortParamsLayout = new QHBoxLayout(emaShortParams);
    emaShortParamsLayout->addWidget(new QLabel("Période:", emaShortParams));
    m_widgets["ema_short_spin"] = new QSpinBox(emaShortParams);
    static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->setRange(1, 500);
    static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->setValue(20);
    emaShortParamsLayout->addWidget(m_widgets["ema_short_spin"]);
    emaShortParamsLayout->addStretch();
    
    emaShortLayout->addWidget(emaShortParams);
    
    strategyLayout->addWidget(emaShortGroup);
    
    // Section EMA Long (ajout pour compléter le panel)
    QGroupBox* emaLongGroup = new QGroupBox("EMA Long", this);
    QVBoxLayout* emaLongLayout = new QVBoxLayout(emaLongGroup);
    
    m_widgets["ema_long_filter_check"] = new QCheckBox("Activer le filtre EMA long", emaLongGroup);
    static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->setChecked(true);
    emaLongLayout->addWidget(m_widgets["ema_long_filter_check"]);
    
    QFrame* emaLongParams = new QFrame(emaLongGroup);
    QHBoxLayout* emaLongParamsLayout = new QHBoxLayout(emaLongParams);
    emaLongParamsLayout->addWidget(new QLabel("Période:", emaLongParams));
    m_widgets["ema_long_spin"] = new QSpinBox(emaLongParams);
    static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->setRange(1, 500);
    static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->setValue(200);
    emaLongParamsLayout->addWidget(m_widgets["ema_long_spin"]);
    emaLongParamsLayout->addStretch();
    
    emaLongLayout->addWidget(emaLongParams);
    
    strategyLayout->addWidget(emaLongGroup);
    
    // Ajouter un stretch pour remplir l'espace restant
    strategyLayout->addStretch(1);
    
    // Initialiser l'état des widgets
    onEmaShortFilterToggled(static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->isChecked());
}

QMap<QString, QVariant> SellHeikinRedPanel::getValues()
{
    QMap<QString, QVariant> values;
    
    // EMA Court
    values["ema_short_filter_enabled"] = static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->isChecked();
    values["ema_short_period"] = static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->value();
    
    // EMA Long
    values["ema_long_filter_enabled"] = static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->isChecked();
    values["ema_long_period"] = static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->value();
    
    return values;
}

void SellHeikinRedPanel::setValues(const QMap<QString, QVariant>& values)
{
    // EMA Court
    if (values.contains("ema_short_filter_enabled")) {
        static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->setChecked(
            values["ema_short_filter_enabled"].toBool());
    }
    
    if (values.contains("ema_short_period")) {
        static_cast<QSpinBox*>(m_widgets["ema_short_spin"])->setValue(
            values["ema_short_period"].toInt());
    }
    
    // EMA Long
    if (values.contains("ema_long_filter_enabled")) {
        static_cast<QCheckBox*>(m_widgets["ema_long_filter_check"])->setChecked(
            values["ema_long_filter_enabled"].toBool());
    }
    
    if (values.contains("ema_long_period")) {
        static_cast<QSpinBox*>(m_widgets["ema_long_spin"])->setValue(
            values["ema_long_period"].toInt());
    }
}

void SellHeikinRedPanel::onEmaShortFilterToggled(bool checked)
{
    _toggleWidgetGroup({"ema_short_spin"}, checked);
}

void SellHeikinRedPanel::_toggleWidgetGroup(const QStringList& widgets, bool enabled)
{
    for (const QString& widgetName : widgets) {
        if (m_widgets.contains(widgetName)) {
            m_widgets[widgetName]->setEnabled(enabled);
        }
    }
}