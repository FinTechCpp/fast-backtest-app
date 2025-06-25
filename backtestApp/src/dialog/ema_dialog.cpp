#include "dialog/ema_dialog.h"

EMADialog::EMADialog(QWidget* parent, ChartWidget* chartWidget, int emaId, const EMAInstance& ema)
    : IndicatorDialog(parent, chartWidget, "EMA Settings")
    , m_emaId(emaId)
    , m_originalEma(ema)
    , m_currentEma(ema)
{
    setupUI();
    connectSignals();
}

EMADialog::~EMADialog()
{
}

void EMADialog::setupUI()
{
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 200);
    m_periodSpinBox->setValue(m_currentEma.period);
    m_formLayout->addRow("Period:", m_periodSpinBox);
    
    // Couleur de la ligne
    m_colorButton = new QPushButton();
    updateColorButtonStyle(m_colorButton, m_currentEma.color);
    m_formLayout->addRow("Line Color:", m_colorButton);
}

void EMADialog::connectSignals()
{
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &EMADialog::onPeriodChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &EMADialog::onColorButtonClicked);
}

void EMADialog::onPeriodChanged(int period)
{
    m_currentEma.period = period;
    applyChanges(); // Appliquer immédiatement les changements
}

void EMADialog::onColorButtonClicked()
{
    QColor color = openColorDialog(m_currentEma.color, "Select Line Color");
    
    if (color.isValid()) {
        m_currentEma.color = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_colorButton, m_currentEma.color);
        applyChanges(); // Appliquer immédiatement les changements
    }
}

void EMADialog::applyChanges() {
    m_chartWidget->updateIndicator(m_currentEma);
}

void EMADialog::cancelChanges() {
    m_chartWidget->updateIndicator(m_originalEma);
}