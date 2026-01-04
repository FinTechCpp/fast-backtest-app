#include "ui/dialogs/indicators/atrDialog.h"

ATRDialog::ATRDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::ATRInstance& atr)
    : IndicatorDialog<indicators::ATRInstance>(parent, "ATR", chartWidget, atr)
{
    initialize();
}

ATRDialog::~ATRDialog()
{
}

void ATRDialog::setupUI() {
    // Period
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 1000);
    m_formLayout->addRow("Period:", m_periodSpinBox);
    
    // Height
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_formLayout->addRow("Height:", m_heightSpinBox);
    
    // Checkbox for logarithmic scale
    m_useLogScaleCheckBox = new QCheckBox();
    m_formLayout->addRow("Use Logarithmic Scale:", m_useLogScaleCheckBox);

    //Checkbox for resetting on new day
    m_resetOnNewDayCheckBox = new QCheckBox();
    m_formLayout->addRow("Reset on New Day:", m_resetOnNewDayCheckBox);
    
    // Line color
    m_colorButton = new QPushButton();
    m_formLayout->addRow("Line Color:", m_colorButton);
}

void ATRDialog::connectSignals() {
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ATRDialog::onPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ATRDialog::onHeightChanged);
    connect(m_useLogScaleCheckBox, &QCheckBox::checkStateChanged, this, &ATRDialog::onLogScaleChanged);
    connect(m_resetOnNewDayCheckBox, &QCheckBox::checkStateChanged, this, &ATRDialog::onResetOnNewDayChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &ATRDialog::onColorButtonClicked);
}

void ATRDialog::updateUIFromInstance()
{
    m_periodSpinBox->setValue(m_currentIndicator.period);
    m_heightSpinBox->setValue(m_currentIndicator.height);
    m_useLogScaleCheckBox->setChecked(m_currentIndicator.useLogScale);
    m_resetOnNewDayCheckBox->setChecked(m_currentIndicator.resetOnNewDay);
    updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
}

void ATRDialog::onPeriodChanged(int period) {
    m_currentIndicator.period = period;
    applyChanges(); // Apply changes immediately
}

void ATRDialog::onHeightChanged(int height) {
    m_currentIndicator.height = height;
    applyChanges(); // Apply changes immediately
}

void ATRDialog::onLogScaleChanged(int state) {
    m_currentIndicator.useLogScale = (state == Qt::Checked);
    applyChanges(); // Apply changes immediately
}

void ATRDialog::onResetOnNewDayChanged(int state) {
    m_currentIndicator.resetOnNewDay = (state == Qt::Checked);
    applyChanges(); // Apply changes immediately
}

void ATRDialog::onColorButtonClicked() {
    QColor color = openColorDialog(m_currentIndicator.color, "Select Line Color");
    
    if (color.isValid()) {
        m_currentIndicator.color = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
        applyChanges(); // Apply changes immediately
    }
}
