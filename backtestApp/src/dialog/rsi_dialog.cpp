#include "dialog/rsi_dialog.h"

RSIDialog::RSIDialog(QWidget* parent, ChartWidget* chartWidget, int rsiId, const RSIInstance& rsi)
    : IndicatorDialog(parent, chartWidget, "RSI Settings")
    , m_rsiId(rsiId)
    , m_originalRsi(rsi)
    , m_currentRsi(rsi)
{    

    setupUI();
    connectSignals();
}

void RSIDialog::setupUI() {
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 100);
    m_periodSpinBox->setValue(m_currentRsi.period);
    m_formLayout->addRow("Period:", m_periodSpinBox);
    
    // Hauteur
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_heightSpinBox->setValue(m_currentRsi.height);
    m_formLayout->addRow("Height:", m_heightSpinBox);
    
    // Range
    m_rangeSpinBox = new QDoubleSpinBox();
    m_rangeSpinBox->setRange(5, 40);
    m_rangeSpinBox->setSingleStep(1);
    m_rangeSpinBox->setValue(m_currentRsi.range);
    m_formLayout->addRow("Range:", m_rangeSpinBox);
    
    // Couleur principale
    m_colorButton = new QPushButton();
    updateColorButtonStyle(m_colorButton, m_currentRsi.color);
    m_formLayout->addRow("Line Color:", m_colorButton);
    
    // Couleur zone supérieure
    m_upperColorButton = new QPushButton();
    updateColorButtonStyle(m_upperColorButton, m_currentRsi.upperColor);
    m_formLayout->addRow("Upper Zone Color:", m_upperColorButton);
    
    // Couleur zone inférieure
    m_lowerColorButton = new QPushButton();
    updateColorButtonStyle(m_lowerColorButton, m_currentRsi.lowerColor);
    m_formLayout->addRow("Lower Zone Color:", m_lowerColorButton);
}

void RSIDialog::connectSignals() {
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onHeightChanged);
    connect(m_rangeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RSIDialog::onRangeChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &RSIDialog::onColorButtonClicked);
    connect(m_upperColorButton, &QPushButton::clicked, this, &RSIDialog::onUpperColorButtonClicked);
    connect(m_lowerColorButton, &QPushButton::clicked, this, &RSIDialog::onLowerColorButtonClicked);
}

RSIDialog::~RSIDialog()
{
}

void RSIDialog::onPeriodChanged(int period) {
    m_currentRsi.period = period;
    applyChanges();
}

void RSIDialog::onHeightChanged(int height) {
    m_currentRsi.height = height;
    applyChanges();
}

void RSIDialog::onRangeChanged(double range) {
    m_currentRsi.range = range;
    applyChanges();
}

void RSIDialog::onColorButtonClicked() {
    QColor color = openColorDialog(m_currentRsi.color, "Select Line Color");
    
    if (color.isValid()) {
        m_currentRsi.color = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_colorButton, m_currentRsi.color);
    }
}

void RSIDialog::onUpperColorButtonClicked() {
    QColor color = openColorDialog(m_currentRsi.upperColor, "Select Upper Zone Color");

    if (color.isValid()) {
        m_currentRsi.upperColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_upperColorButton, m_currentRsi.upperColor);
        applyChanges();
    }
}

void RSIDialog::onLowerColorButtonClicked() {
    QColor color = openColorDialog(m_currentRsi.lowerColor, "Select Lower Zone Color");

    if (color.isValid()) {
        m_currentRsi.lowerColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_lowerColorButton, m_currentRsi.lowerColor);
        applyChanges();
    }
}

void RSIDialog::applyChanges() {
    m_chartWidget->setRSIConfig(m_currentRsi);
}

void RSIDialog::cancelChanges() {
    m_chartWidget->setRSIConfig(m_originalRsi);
}