#include "dialog/supertrendDialog.h"

SupertrendDialog::SupertrendDialog(QWidget* parent, ChartWidget* chartWidget, int supertrendId, const SuperTrendInstance& supertrend)
    : IndicatorDialog(parent, chartWidget, "Supertrend Settings")
    , m_supertrendId(supertrendId)
    , m_originalSupertrend(supertrend)
    , m_currentSupertrend(supertrend)
{
    setupUI();
    connectSignals();
}

SupertrendDialog::~SupertrendDialog()
{
}

void SupertrendDialog::setupUI()
{
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 100);
    m_periodSpinBox->setValue(m_currentSupertrend.period);
    m_formLayout->addRow("ATR Period:", m_periodSpinBox);
    
    // Multiplicateur
    m_multiplierSpinBox = new QDoubleSpinBox();
    m_multiplierSpinBox->setRange(0.1, 10.0);
    m_multiplierSpinBox->setSingleStep(0.1);
    m_multiplierSpinBox->setDecimals(1);
    m_multiplierSpinBox->setValue(m_currentSupertrend.multiplier);
    m_formLayout->addRow("Multiplier:", m_multiplierSpinBox);
    
    // Couleur tendance haussière
    m_upColorButton = new QPushButton();
    updateColorButtonStyle(m_upColorButton, m_currentSupertrend.upColor);
    m_formLayout->addRow("Up Trend Color:", m_upColorButton);
    
    // Couleur tendance baissière
    m_downColorButton = new QPushButton();
    updateColorButtonStyle(m_downColorButton, m_currentSupertrend.downColor);
    m_formLayout->addRow("Down Trend Color:", m_downColorButton);
}

void SupertrendDialog::connectSignals()
{
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SupertrendDialog::onPeriodChanged);
    connect(m_multiplierSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SupertrendDialog::onMultiplierChanged);
    connect(m_upColorButton, &QPushButton::clicked, this, &SupertrendDialog::onUpColorButtonClicked);
    connect(m_downColorButton, &QPushButton::clicked, this, &SupertrendDialog::onDownColorButtonClicked);
}

void SupertrendDialog::onPeriodChanged(int period) {
    m_currentSupertrend.period = period;
    applyChanges();
}

void SupertrendDialog::onMultiplierChanged(double multiplier) {
    m_currentSupertrend.multiplier = multiplier;
    applyChanges();
}

void SupertrendDialog::onUpColorButtonClicked() {
    QColor color = openColorDialog(m_currentSupertrend.upColor, "Select Up Trend Color");
    
    if (color.isValid()) {
        m_currentSupertrend.upColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_upColorButton, m_currentSupertrend.upColor);
        applyChanges();
    }
}

void SupertrendDialog::onDownColorButtonClicked() {
    QColor color = openColorDialog(m_currentSupertrend.downColor, "Select Down Trend Color");
    
    if (color.isValid()) {
        m_currentSupertrend.downColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_downColorButton, m_currentSupertrend.downColor);
        applyChanges();
    }
}

void SupertrendDialog::applyChanges() {
    m_chartWidget->updateIndicator(m_currentSupertrend);
}

void SupertrendDialog::cancelChanges() {
    m_chartWidget->updateIndicator(m_originalSupertrend);
}