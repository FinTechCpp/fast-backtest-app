#include "ui/dialogs/rsiDialog.h"

RSIDialog::RSIDialog(QWidget* parent, ChartWidget* chartWidget, const RSIInstance& rsi)
    : IndicatorDialog<RSIInstance>(parent, "RSI", chartWidget, rsi)
{
    setupUI();
    connectSignals();
}

void RSIDialog::setupUI() {
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 100);
    m_periodSpinBox->setValue(m_currentIndicator.period);
    m_formLayout->addRow("Period:", m_periodSpinBox);
    
    // Hauteur
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_heightSpinBox->setValue(m_currentIndicator.height);
    m_formLayout->addRow("Height:", m_heightSpinBox);
    
    // Niveau de surachat
    m_overboughtLevelSpinBox = new QSpinBox();
    m_overboughtLevelSpinBox->setRange(50, 100);
    m_overboughtLevelSpinBox->setValue(m_currentIndicator.overboughtLevel);
    m_formLayout->addRow("Overbought Level:", m_overboughtLevelSpinBox);
    
    // Niveau de survente
    m_oversoldLevelSpinBox = new QSpinBox();
    m_oversoldLevelSpinBox->setRange(0, 50);
    m_oversoldLevelSpinBox->setValue(m_currentIndicator.oversoldLevel);
    m_formLayout->addRow("Oversold Level:", m_oversoldLevelSpinBox);
    
    // Couleur principale
    m_colorButton = new QPushButton();
    updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
    m_formLayout->addRow("Line Color:", m_colorButton);
    
    // Couleur zone supérieure
    m_upperColorButton = new QPushButton();
    updateColorButtonStyle(m_upperColorButton, m_currentIndicator.upperColor);
    m_formLayout->addRow("Upper Zone Color:", m_upperColorButton);
    
    // Couleur zone inférieure
    m_lowerColorButton = new QPushButton();
    updateColorButtonStyle(m_lowerColorButton, m_currentIndicator.lowerColor);
    m_formLayout->addRow("Lower Zone Color:", m_lowerColorButton);
}

void RSIDialog::connectSignals() {
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onHeightChanged);
    connect(m_overboughtLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onOverboughtLevelChanged);
    connect(m_oversoldLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onOversoldLevelChanged);
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
    updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
    updateColorButtonStyle(m_upperColorButton, m_currentIndicator.upperColor);
    updateColorButtonStyle(m_lowerColorButton, m_currentIndicator.lowerColor);
}

RSIDialog::~RSIDialog()
{
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