#include "ui/dialogs/indicators/emaDialog.h"

EMADialog::EMADialog(QWidget* parent, ChartWidget* chartWidget, const indicators::EMAInstance& ema)
    : IndicatorDialog<indicators::EMAInstance>(parent, "EMA", chartWidget, ema)
{
    initialize();
}

EMADialog::~EMADialog()
{
}

void EMADialog::setupUI()
{
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 3000);
    m_formLayout->addRow("Period:", m_periodSpinBox);
    
    // Couleur de la ligne
    m_colorButton = new QPushButton();
    m_formLayout->addRow("Line Color:", m_colorButton);
}

void EMADialog::connectSignals()
{
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &EMADialog::onPeriodChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &EMADialog::onColorButtonClicked);
}

void EMADialog::updateUIFromInstance()
{
    m_periodSpinBox->setValue(m_currentIndicator.period);
    updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
}

void EMADialog::onPeriodChanged(int period)
{
    m_currentIndicator.period = period;
    applyChanges(); // Appliquer immédiatement les changements
}

void EMADialog::onColorButtonClicked()
{
    QColor color = openColorDialog(m_currentIndicator.color, "Select Line Color");
    
    if (color.isValid()) {
        m_currentIndicator.color = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
        applyChanges(); // Appliquer immédiatement les changements
    }
}
