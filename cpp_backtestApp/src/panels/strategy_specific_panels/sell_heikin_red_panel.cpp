#include "panels/strategy_specific_panels/sell_heikin_red_panel.h"
#include <QDebug>

SellHeikinRedPanel::SellHeikinRedPanel(QWidget* parent)
    : QObject(parent), BasePanel(parent)
{
}

QGroupBox* SellHeikinRedPanel::create()
{
    QGroupBox* strategyGroup = new QGroupBox("Paramètres SellHeikinRed");
    QVBoxLayout* strategyLayout = new QVBoxLayout();
    
    // Section EMA Court
    QGroupBox* emaShortGroup = new QGroupBox("EMA Court");
    QVBoxLayout* emaShortLayout = new QVBoxLayout();
    
    // Checkbox pour activer/désactiver le filtre EMA court
    m_widgets["ema_short_filter_check"] = new QCheckBox("Activer le filtre EMA court");
    static_cast<QCheckBox*>(m_widgets["ema_short_filter_check"])->setChecked(true);
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
    
    strategyGroup->setLayout(strategyLayout);
    return strategyGroup;
}

QMap<QString, QVariant> SellHeikinRedPanel::getValues()
{
    QMap<QString, QVariant> values;
    // TODO: Implémenter
    return values;
}

void SellHeikinRedPanel::setValues(const QMap<QString, QVariant>& values)
{
    Q_UNUSED(values)
    // TODO: Implémenter
}

void SellHeikinRedPanel::_toggleWidgetGroup(const QStringList& widgets, bool enabled)
{
    Q_UNUSED(widgets)
    Q_UNUSED(enabled)
    // TODO: Implémenter
}