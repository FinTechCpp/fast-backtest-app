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
    // Period
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 3000);
    m_formLayout->addRow("Period:", m_periodSpinBox);

    // Checkbox for resetting on new day
    m_resetOnNewDayCheckBox = new QCheckBox();
    m_formLayout->addRow("Reset on New Day:", m_resetOnNewDayCheckBox);
    
    // Line color
    m_colorButton = new QPushButton();
    m_formLayout->addRow("Line Color:", m_colorButton);
}

void EMADialog::connectSignals()
{
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &EMADialog::onPeriodChanged);
    connect(m_resetOnNewDayCheckBox, &QCheckBox::checkStateChanged, this, &EMADialog::onResetOnNewDayChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &EMADialog::onColorButtonClicked);
}

void EMADialog::updateUIFromInstance()
{
    m_periodSpinBox->setValue(m_currentIndicator.period);
    m_resetOnNewDayCheckBox->setCheckState(m_currentIndicator.resetOnNewDay ? Qt::Checked : Qt::Unchecked);
    updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
}

void EMADialog::onPeriodChanged(int period)
{
    m_currentIndicator.period = period;
    applyChanges(); // Apply changes immediately
}

void EMADialog::onResetOnNewDayChanged(int state)
{
    m_currentIndicator.resetOnNewDay = (state == Qt::Checked);
    applyChanges(); // Apply changes immediately
}

void EMADialog::onColorButtonClicked()
{
    QColor color = openColorDialog(m_currentIndicator.color, "Select Line Color");
    
    if (color.isValid()) {
        m_currentIndicator.color = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
        applyChanges(); // Apply changes immediately
    }
}
