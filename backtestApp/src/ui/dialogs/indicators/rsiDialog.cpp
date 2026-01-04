#include "ui/dialogs/indicators/rsiDialog.h"

RSIDialog::RSIDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::RSIInstance& rsi)
    : IndicatorDialog<indicators::RSIInstance>(parent, "RSI", chartWidget, rsi)
{
    initialize();
}

RSIDialog::~RSIDialog() 
{
}

void RSIDialog::setupUI() {
    // Period
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 1000);
    m_formLayout->addRow("Period:", m_periodSpinBox);
    
    // Height
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_formLayout->addRow("Height:", m_heightSpinBox);
    
    // Overbought Level
    m_overboughtLevelSpinBox = new QSpinBox();
    m_overboughtLevelSpinBox->setRange(50, 100);
    m_formLayout->addRow("Overbought Level:", m_overboughtLevelSpinBox);
    
    // Oversold Level
    m_oversoldLevelSpinBox = new QSpinBox();
    m_oversoldLevelSpinBox->setRange(0, 50);
    m_formLayout->addRow("Oversold Level:", m_oversoldLevelSpinBox);

    // Checkbox for resetting on new day
    m_resetOnNewDayCheckBox = new QCheckBox();
    m_formLayout->addRow("Reset on New Day:", m_resetOnNewDayCheckBox);
    
    // Line Color
    m_colorButton = new QPushButton();
    m_formLayout->addRow("Line Color:", m_colorButton);
    
    // Upper Zone Color
    m_upperColorButton = new QPushButton();
    m_formLayout->addRow("Upper Zone Color:", m_upperColorButton);
    
    // Lower Zone Color
    m_lowerColorButton = new QPushButton();
    m_formLayout->addRow("Lower Zone Color:", m_lowerColorButton);
}

void RSIDialog::connectSignals() {
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onHeightChanged);
    connect(m_overboughtLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onOverboughtLevelChanged);
    connect(m_oversoldLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onOversoldLevelChanged);
    connect(m_resetOnNewDayCheckBox, &QCheckBox::checkStateChanged, this, &RSIDialog::onResetOnNewDayChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &RSIDialog::onColorButtonClicked);
    connect(m_upperColorButton, &QPushButton::clicked, this, &RSIDialog::onUpperColorButtonClicked);
    connect(m_lowerColorButton, &QPushButton::clicked, this, &RSIDialog::onLowerColorButtonClicked);
}

void RSIDialog::updateUIFromInstance()
{
    m_periodSpinBox->setValue(m_currentIndicator.period);
    m_heightSpinBox->setValue(m_currentIndicator.height);
    m_overboughtLevelSpinBox->setValue(m_currentIndicator.overboughtLevel);
    m_oversoldLevelSpinBox->setValue(m_currentIndicator.oversoldLevel);
    m_resetOnNewDayCheckBox->setCheckState(m_currentIndicator.resetOnNewDay ? Qt::Checked : Qt::Unchecked);
    updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
    updateColorButtonStyle(m_upperColorButton, m_currentIndicator.upperColor);
    updateColorButtonStyle(m_lowerColorButton, m_currentIndicator.lowerColor);
}

void RSIDialog::onPeriodChanged(int period) {
    m_currentIndicator.period = period;
    applyChanges();
}

void RSIDialog::onHeightChanged(int height) {
    m_currentIndicator.height = height;
    applyChanges();
}

void RSIDialog::onOverboughtLevelChanged(int level) {
    m_currentIndicator.overboughtLevel = level;
    applyChanges();
}

void RSIDialog::onOversoldLevelChanged(int level) {
    m_currentIndicator.oversoldLevel = level;
    applyChanges();
}

void RSIDialog::onResetOnNewDayChanged(int state) {
    m_currentIndicator.resetOnNewDay = (state == Qt::Checked);
    applyChanges();
}

void RSIDialog::onColorButtonClicked() {
    QColor color = openColorDialog(m_currentIndicator.color, "Select Line Color");
    
    if (color.isValid()) {
        m_currentIndicator.color = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
    }
}

void RSIDialog::onUpperColorButtonClicked() {
    QColor color = openColorDialog(m_currentIndicator.upperColor, "Select Upper Zone Color");

    if (color.isValid()) {
        m_currentIndicator.upperColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_upperColorButton, m_currentIndicator.upperColor);
        applyChanges();
    }
}

void RSIDialog::onLowerColorButtonClicked() {
    QColor color = openColorDialog(m_currentIndicator.lowerColor, "Select Lower Zone Color");

    if (color.isValid()) {
        m_currentIndicator.lowerColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_lowerColorButton, m_currentIndicator.lowerColor);
        applyChanges();
    }
}