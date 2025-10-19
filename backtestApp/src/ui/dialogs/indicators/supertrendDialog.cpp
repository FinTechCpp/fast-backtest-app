#include "ui/dialogs/indicators/supertrendDialog.h"

SupertrendDialog::SupertrendDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::SuperTrendInstance& supertrend)
    : IndicatorDialog<indicators::SuperTrendInstance>(parent, "Supertrend", chartWidget, supertrend)
{
    initialize();
}

SupertrendDialog::~SupertrendDialog()
{
}

void SupertrendDialog::setupUI()
{
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 200);
    m_periodSpinBox->setSingleStep(1);
    m_formLayout->addRow("ATR Period:", m_periodSpinBox);
    
    // Multiplicateur
    m_multiplierSpinBox = new QDoubleSpinBox();
    m_multiplierSpinBox->setRange(0.1, 200.0);
    m_multiplierSpinBox->setSingleStep(1);
    m_multiplierSpinBox->setDecimals(1);
    m_formLayout->addRow("Multiplier:", m_multiplierSpinBox);
    
    // Couleur tendance haussière
    m_upColorButton = new QPushButton();
    m_formLayout->addRow("Up Trend Color:", m_upColorButton);
    
    // Couleur tendance baissière
    m_downColorButton = new QPushButton();
    m_formLayout->addRow("Down Trend Color:", m_downColorButton);
}

void SupertrendDialog::connectSignals()
{
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SupertrendDialog::onPeriodChanged);
    connect(m_multiplierSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SupertrendDialog::onMultiplierChanged);
    connect(m_upColorButton, &QPushButton::clicked, this, &SupertrendDialog::onUpColorButtonClicked);
    connect(m_downColorButton, &QPushButton::clicked, this, &SupertrendDialog::onDownColorButtonClicked);
}

void SupertrendDialog::updateUIFromInstance()
{
    m_periodSpinBox->setValue(m_currentIndicator.period);
    m_multiplierSpinBox->setValue(m_currentIndicator.multiplier);
    updateColorButtonStyle(m_upColorButton, m_currentIndicator.upColor);
    updateColorButtonStyle(m_downColorButton, m_currentIndicator.downColor);
}

void SupertrendDialog::onPeriodChanged(int period)
{
    m_currentIndicator.period = period;
    applyChanges();
}

void SupertrendDialog::onMultiplierChanged(double multiplier) {
    m_currentIndicator.multiplier = multiplier;
    applyChanges();
}

void SupertrendDialog::onUpColorButtonClicked() {
    QColor color = openColorDialog(m_currentIndicator.upColor, "Select Up Trend Color");
    
    if (color.isValid()) {
        m_currentIndicator.upColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_upColorButton, m_currentIndicator.upColor);
        applyChanges();
    }
}

void SupertrendDialog::onDownColorButtonClicked() {
    QColor color = openColorDialog(m_currentIndicator.downColor, "Select Down Trend Color");
    
    if (color.isValid()) {
        m_currentIndicator.downColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_downColorButton, m_currentIndicator.downColor);
        applyChanges();
    }
}
